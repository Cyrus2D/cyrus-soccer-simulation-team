//
// Created by aref on 11/28/19.
//

#include "DataExtractor.h"
#include "cooperative_action.h"
#include <rcsc/player/world_model.h>
#include <random>
#include <time.h>

#define cm ","
#define ADD_ELEM(key, value) fout << (value) << cm

double invalid_data = -2.0;

using namespace rcsc;

std::vector<std::vector<rcsc::Vector2D>> DataExtractor::history_pos;
std::vector<std::vector<rcsc::Vector2D>> DataExtractor::history_vel;
std::vector<std::vector<rcsc::AngleDeg>> DataExtractor::history_body;
std::vector<std::vector<int>> DataExtractor::history_pos_count;
std::vector<std::vector<int>> DataExtractor::history_vel_count;
std::vector<std::vector<int>> DataExtractor::history_body_count;

DataExtractor::DataExtractor() :
        last_update_cycle(-1) {
}

DataExtractor::~DataExtractor() {
}

#include "../sample_player.h"
void DataExtractor::update_history(const rcsc::PlayerAgent *agent){
    const WorldModel &wm = option.input_worldMode == FULLSTATE ? agent->fullstateWorld() : agent->world();
    static int last_update = -1;
    if (last_update == -1){
        for (int i = 0; i <= 22; i++){
            std::vector<Vector2D> temp_pos(5, Vector2D(0,0));
            std::vector<Vector2D> temp_vel(5, Vector2D(0,0));
            std::vector<AngleDeg> temp_body(5, AngleDeg(0));
            std::vector<int> temp_pos_count(5, -1);
            DataExtractor::history_pos.push_back(temp_pos);
            DataExtractor::history_vel.push_back(temp_vel);
            DataExtractor::history_body.push_back(temp_body);
            DataExtractor::history_pos_count.push_back(temp_pos_count);
            DataExtractor::history_vel_count.push_back(temp_pos_count);
            DataExtractor::history_body_count.push_back(temp_pos_count);
        }
    }
    if (last_update == wm.time().cycle())
        return;
    last_update = wm.time().cycle();
    if (wm.ball().posValid()){
        DataExtractor::history_pos[0].push_back(wm.ball().pos());
        DataExtractor::history_pos_count[0].push_back(wm.ball().posCount());
    }else{
        DataExtractor::history_pos[0].push_back(Vector2D::INVALIDATED);
        DataExtractor::history_pos_count[0].push_back(-1);
    }
    if (wm.ball().velValid()){
        DataExtractor::history_vel[0].push_back(wm.ball().vel());
        DataExtractor::history_vel_count[0].push_back(wm.ball().velCount());
    }else{
        DataExtractor::history_vel[0].push_back(Vector2D::INVALIDATED);
        DataExtractor::history_vel_count[0].push_back(-1);
    }
    for (int i=1; i<=11; i++){
        const AbstractPlayerObject * p = wm.ourPlayer(i);
        if(p == NULL || p->unum() != i){
            DataExtractor::history_pos[i].push_back(Vector2D::INVALIDATED);
            DataExtractor::history_pos_count[i].push_back(-1);
            DataExtractor::history_vel[i].push_back(Vector2D::INVALIDATED);
            DataExtractor::history_vel_count[i].push_back(-1);
            DataExtractor::history_body[i].push_back(AngleDeg(0));
            DataExtractor::history_body_count[i].push_back(-1);
        }else{
            if(p->pos().isValid()){
                DataExtractor::history_pos[i].push_back(p->pos());
                DataExtractor::history_pos_count[i].push_back(p->posCount());
            }else{
                DataExtractor::history_pos[i].push_back(Vector2D::INVALIDATED);
                DataExtractor::history_pos_count[i].push_back(-1);
            }
            if(p->vel().isValid()){
                DataExtractor::history_vel[i].push_back(p->vel());
                DataExtractor::history_vel_count[i].push_back(p->velCount());
            }else{
                DataExtractor::history_vel[i].push_back(Vector2D::INVALIDATED);
                DataExtractor::history_vel_count[i].push_back(-1);
            }
            DataExtractor::history_body[i].push_back(p->body());
            DataExtractor::history_body_count[i].push_back(p->bodyCount());
        }
    }
    for (int i=1; i<=11; i++){
        const AbstractPlayerObject * p = wm.theirPlayer(i);
        if(p == NULL || p->unum() != i){
            DataExtractor::history_pos[i+11].push_back(Vector2D::INVALIDATED);
            DataExtractor::history_pos_count[i+11].push_back(-1);
            DataExtractor::history_vel[i+11].push_back(Vector2D::INVALIDATED);
            DataExtractor::history_vel_count[i+11].push_back(-1);
            DataExtractor::history_body[i+11].push_back(AngleDeg(0));
            DataExtractor::history_body_count[i+11].push_back(-1);
        }else{
            if(p->pos().isValid()){
                DataExtractor::history_pos[i+11].push_back(p->pos());
                DataExtractor::history_pos_count[i+11].push_back(p->posCount());
            }else{
                DataExtractor::history_pos[i+11].push_back(Vector2D::INVALIDATED);
                DataExtractor::history_pos_count[i+11].push_back(-1);
            }
            if(p->vel().isValid()){
                DataExtractor::history_vel[i+11].push_back(p->vel());
                DataExtractor::history_vel_count[i+11].push_back(p->velCount());
            }else{
                DataExtractor::history_vel[i+11].push_back(Vector2D::INVALIDATED);
                DataExtractor::history_vel_count[i+11].push_back(-1);
            }
            DataExtractor::history_body[i+11].push_back(p->body());
            DataExtractor::history_body_count[i+11].push_back(p->bodyCount());
        }
    }
}


void DataExtractor::update(const PlayerAgent *agent, const ActionStatePair *first_layer,bool update_shoot) {
    const WorldModel &wm = option.input_worldMode == FULLSTATE ? agent->fullstateWorld() : agent->world();

    if (last_update_cycle == wm.time().cycle())
        return;
    if (!wm.self().isKickable())
        return;
    if (wm.gameMode().type() != rcsc::GameMode::PlayOn)
        return;
    if (!fout.is_open()) {
        init_file(wm);
    }
    last_update_cycle = wm.time().cycle();
    data.clear();

    if (!update_shoot){
        if (
                ((CooperativeAction &) first_layer->action()).category() > 2
                ||
                !((CooperativeAction &) first_layer->action()).targetPoint().isValid()
                ||
                ((CooperativeAction &) first_layer->action()).targetPlayerUnum() > 11
                ||
                ((CooperativeAction &) first_layer->action()).targetPlayerUnum() < 1
                )
            return;
    }

    // cycle
    ADD_ELEM("cycle", convertor_cycle(last_update_cycle));

    // ball
    extract_ball(wm);

    // kicker
    extract_kicker(wm);

    // players
    extract_players(wm);

    // output
    if (!update_shoot){
        extract_output(wm,
                       ((CooperativeAction &) first_layer->action()).category(),
                       ((CooperativeAction &) first_layer->action()).targetPoint(),
                       ((CooperativeAction &) first_layer->action()).targetPlayerUnum(),
                       ((CooperativeAction &) first_layer->action()).description(),
                       ((CooperativeAction &) first_layer->action()).firstBallSpeed());
        fout << std::endl;
    }
}

DataExtractor &DataExtractor::i() {
    static DataExtractor instance;
    return instance;
}

void DataExtractor::init_file(const rcsc::WorldModel &wm) {
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    std::string dir = "/home/nader/data/robo_data/other_team/";
    strftime(buffer, sizeof(buffer), "%Y-%m-%d-%H-%M-%S", timeinfo);
    std::string str(buffer);
    std::string rand_name = std::to_string(SamplePlayer::player_port);
    str += "_" + std::to_string(wm.self().unum()) + "_" + wm.opponentTeamName() + "_E" + rand_name + ".csv";

    fout = std::ofstream((dir + str).c_str());

    // Cycle and BALL
    std::string header = "cycle,ball_pos_x,ball_pos_y,ball_pos_r,ball_pos_t,ball_kicker_x,ball_kicker_y,ball_kicker_r,ball_kicker_t,ball_vel_x,ball_vel_y,ball_vel_r,ball_vel_t,";
    for (int j = 0; j < option.history_size; j++){
        header += std::string("ball_") + "history_" + std::to_string(j + 1) + "_pos_x,";
        header += std::string("ball_") + "history_" + std::to_string(j + 1) + "_pos_y,";
        header += std::string("ball_") + "history_" + std::to_string(j + 1) + "_pos_r,";
        header += std::string("ball_") + "history_" + std::to_string(j + 1) + "_pos_t,";
        header += std::string("ball_") + "history_" + std::to_string(j + 1) + "_vel_x,";
        header += std::string("ball_") + "history_" + std::to_string(j + 1) + "_vel_y,";
        header += std::string("ball_") + "history_" + std::to_string(j + 1) + "_vel_r,";
        header += std::string("ball_") + "history_" + std::to_string(j + 1) + "_vel_t,";
    }
    header += "offside_count,";

    // Kicker
    if (option.dribleAngle == Kicker)
        for (int i = 0; i < option.nDribleAngle; i++) {
            header += "dribble_angle_" + std::to_string(i) + ",";
        }

    for (int i = 1; i <= 11; i++) {
        if (option.side == TM || option.side == BOTH)
            header += "p_l_" + std::to_string(i) + "_side,";
        if (option.unum == TM || option.unum == BOTH)
            header += "p_l_" + std::to_string(i) + "_unum,";
        if (option.type == TM || option.type == BOTH) {
            header += "p_l_" + std::to_string(i) + "_player_type_dash_rate,";
            header += "p_l_" + std::to_string(i) + "_player_type_effort_max,";
            header += "p_l_" + std::to_string(i) + "_player_type_effort_min,";
            header += "p_l_" + std::to_string(i) + "_player_type_kickable,";
            header += "p_l_" + std::to_string(i) + "_player_type_margin,";
            header += "p_l_" + std::to_string(i) + "_player_type_kick_power,";
            header += "p_l_" + std::to_string(i) + "_player_type_decay,";
            header += "p_l_" + std::to_string(i) + "_player_type_size,";
            header += "p_l_" + std::to_string(i) + "_player_type_speed_max,";
        }
        if (option.body == TM || option.body == BOTH)
            header += "p_l_" + std::to_string(i) + "_body,";
        if (option.face == TM || option.face == BOTH)
            header += "p_l_" + std::to_string(i) + "_face,";
        if (option.tackling == TM || option.tackling == BOTH)
            header += "p_l_" + std::to_string(i) + "_tackling,";
        if (option.kicking == TM || option.kicking == BOTH)
            header += "p_l_" + std::to_string(i) + "_kicking,";
        if (option.card == TM || option.card == BOTH)
            header += "p_l_" + std::to_string(i) + "_card,";
        if (option.pos == TM || option.pos == BOTH) {
            header += "p_l_" + std::to_string(i) + "_pos_x,";
            header += "p_l_" + std::to_string(i) + "_pos_y,";
        }
        if (option.polarPos == TM || option.polarPos == BOTH) {
            header += "p_l_" + std::to_string(i) + "_pos_r,";
            header += "p_l_" + std::to_string(i) + "_pos_t,";
        }
        if (option.relativePos == TM || option.relativePos == BOTH) {
            header += "p_l_" + std::to_string(i) + "_kicker_x,";
            header += "p_l_" + std::to_string(i) + "_kicker_y,";
            header += "p_l_" + std::to_string(i) + "_kicker_r,";
            header += "p_l_" + std::to_string(i) + "_kicker_t,";
        }
        if (option.in_offside == TM)
            header += "p_l_" + std::to_string(i) + "_in_offside,";
        if (option.vel == TM || option.vel == BOTH) {
            header += "p_l_" + std::to_string(i) + "_vel_x,";
            header += "p_l_" + std::to_string(i) + "_vel_y,";
        }
        if (option.polarVel == TM || option.polarVel == BOTH) {
            header += "p_l_" + std::to_string(i) + "_vel_r,";
            header += "p_l_" + std::to_string(i) + "_vel_t,";
        }
        if (option.counts == TM || option.counts == BOTH) {
            header += "p_l_" + std::to_string(i) + "_pos_count,";
            header += "p_l_" + std::to_string(i) + "_vel_count,";
            header += "p_l_" + std::to_string(i) + "_body_count,";
        }
        if (option.isKicker == TM || option.isKicker == BOTH)
            header += "p_l_" + std::to_string(i) + "_is_kicker,";
        if (option.openAnglePass == TM || option.openAnglePass == BOTH) {
            header += "p_l_" + std::to_string(i) + "_pass_dist,";
            header += "p_l_" + std::to_string(i) + "_pass_opp1_dist,";
            header += "p_l_" + std::to_string(i) + "_pass_opp1_angle,";
            header += "p_l_" + std::to_string(i) + "_pass_opp1_dist_line,";
            header += "p_l_" + std::to_string(i) + "_pass_opp1_dist_proj,";
            header += "p_l_" + std::to_string(i) + "_pass_opp1_dist_diffbody,";
            header += "p_l_" + std::to_string(i) + "_pass_opp2_dist,";
            header += "p_l_" + std::to_string(i) + "_pass_opp2_angle,";
            header += "p_l_" + std::to_string(i) + "_pass_opp2_dist_line,";
            header += "p_l_" + std::to_string(i) + "_pass_opp2_dist_proj,";
            header += "p_l_" + std::to_string(i) + "_pass_opp2_dist_diffbody,";
        }
        if (option.nearestOppDist == TM || option.nearestOppDist == BOTH){
            header += "p_l_" + std::to_string(i) + "_near1_opp_dist,";
            header += "p_l_" + std::to_string(i) + "_near1_opp_angle,";
            header += "p_l_" + std::to_string(i) + "_near1_opp_diffbody,";
            header += "p_l_" + std::to_string(i) + "_near2_opp_dist,";
            header += "p_l_" + std::to_string(i) + "_near2_opp_angle,";
            header += "p_l_" + std::to_string(i) + "_near2_opp_diffbody,";
        }
        if (option.polarGoalCenter == TM || option.polarGoalCenter == BOTH) {
            header += "p_l_" + std::to_string(i) + "_angle_goal_center_r,";
            header += "p_l_" + std::to_string(i) + "_angle_goal_center_t,";
        }
        if (option.openAngleGoal == TM || option.openAngleGoal == BOTH)
            header += "p_l_" + std::to_string(i) + "_open_goal_angle,";
        for (int j = 0; j < option.history_size; j++){
            if (option.body == TM || option.body == BOTH)
                header += "p_l_" + std::to_string(i) + "_history_" + std::to_string(j + 1) + "_body,";
            if (option.pos == TM || option.pos == BOTH) {
                header += "p_l_" + std::to_string(i) + "_history_" + std::to_string(j + 1) + "_pos_x,";
                header += "p_l_" + std::to_string(i) + "_history_" + std::to_string(j + 1) + "_pos_y,";
            }
            if (option.polarPos == TM || option.polarPos == BOTH) {
                header += "p_l_" + std::to_string(i) + "_history_" + std::to_string(j + 1) + "_pos_r,";
                header += "p_l_" + std::to_string(i) + "_history_" + std::to_string(j + 1) + "_pos_t,";
            }
            if (option.vel == TM || option.vel == BOTH) {
                header += "p_l_" + std::to_string(i) + "_history_" + std::to_string(j + 1) + "_vel_x,";
                header += "p_l_" + std::to_string(i) + "_history_" + std::to_string(j + 1) + "_vel_y,";
            }
            if (option.polarVel == TM || option.polarVel == BOTH) {
                header += "p_l_" + std::to_string(i) + "_history_" + std::to_string(j + 1) + "_vel_r,";
                header += "p_l_" + std::to_string(i) + "_history_" + std::to_string(j + 1) + "_vel_t,";
            }
            if (option.counts == TM || option.counts == BOTH) {
                header += "p_l_" + std::to_string(i) + "_history_" + std::to_string(j + 1) + "_pos_count,";
                header += "p_l_" + std::to_string(i) + "_history_" + std::to_string(j + 1) + "_vel_count,";
                header += "p_l_" + std::to_string(i) + "_history_" + std::to_string(j + 1) + "_body_count,";
            }
        }
    }
    for (int i = 1; i <= 11; i++) {
        if (option.side == OPP || option.side == BOTH)
            header += "p_r_" + std::to_string(i) + "_side,";
        if (option.unum == OPP || option.unum == BOTH)
            header += "p_r_" + std::to_string(i) + "_unum,";
        if (option.type == OPP || option.type == BOTH) {
            header += "p_r_" + std::to_string(i) + "_player_type_dash_rate,";
            header += "p_r_" + std::to_string(i) + "_player_type_effort_max,";
            header += "p_r_" + std::to_string(i) + "_player_type_effort_min,";
            header += "p_r_" + std::to_string(i) + "_player_type_kickable,";
            header += "p_r_" + std::to_string(i) + "_player_type_margin,";
            header += "p_r_" + std::to_string(i) + "_player_type_kick_power,";
            header += "p_r_" + std::to_string(i) + "_player_type_decay,";
            header += "p_r_" + std::to_string(i) + "_player_type_size,";
            header += "p_r_" + std::to_string(i) + "_player_type_speed_max,";
        }
        if (option.body == OPP || option.body == BOTH)
            header += "p_r_" + std::to_string(i) + "_body,";
        if (option.face == OPP || option.face == BOTH)
            header += "p_r_" + std::to_string(i) + "_face,";
        if (option.tackling == OPP || option.tackling == BOTH)
            header += "p_r_" + std::to_string(i) + "_tackling,";
        if (option.kicking == OPP || option.kicking == BOTH)
            header += "p_r_" + std::to_string(i) + "_kicking,";
        if (option.card == OPP || option.card == BOTH)
            header += "p_r_" + std::to_string(i) + "_card,";
        if (option.pos == OPP || option.pos == BOTH) {
            header += "p_r_" + std::to_string(i) + "_pos_x,";
            header += "p_r_" + std::to_string(i) + "_pos_y,";
        }
        if (option.polarPos == OPP || option.polarPos == BOTH) {
            header += "p_r_" + std::to_string(i) + "_pos_r,";
            header += "p_r_" + std::to_string(i) + "_pos_t,";
        }
        if (option.relativePos == OPP || option.relativePos == BOTH) {
            header += "p_r_" + std::to_string(i) + "_kicker_x,";
            header += "p_r_" + std::to_string(i) + "_kicker_y,";
            header += "p_r_" + std::to_string(i) + "_kicker_r,";
            header += "p_r_" + std::to_string(i) + "_kicker_t,";
        }
        if (option.vel == OPP || option.vel == BOTH) {
            header += "p_r_" + std::to_string(i) + "_vel_x,";
            header += "p_r_" + std::to_string(i) + "_vel_y,";
        }
        if (option.polarVel == OPP || option.polarVel == BOTH) {
            header += std::string("p_r_") + std::to_string(i) + "_vel_r,";
            header += std::string("p_r_") + std::to_string(i) + "_vel_t,";
        }
        if (option.counts == OPP || option.counts == BOTH) {
            header += std::string("p_r_") + std::to_string(i) + "_pos_count,";
            header += std::string("p_r_") + std::to_string(i) + "_vel_count,";
            header += std::string("p_r_") + std::to_string(i) + "_body_count,";
        }
        if (option.isKicker == OPP || option.isKicker == BOTH)
            header += "p_r_" + std::to_string(i) + "_is_kicker,";
        if (option.openAnglePass == OPP || option.openAnglePass == BOTH) {
            header += "p_r_" + std::to_string(i) + "_pass_angle,";
            header += "p_r_" + std::to_string(i) + "_pass_dist,";
        }
        if (option.nearestOppDist == OPP || option.nearestOppDist == BOTH){
            header += "p_r_" + std::to_string(i) + "_near1_opp_dist,";
            header += "p_r_" + std::to_string(i) + "_near1_opp_angle,";
            header += "p_r_" + std::to_string(i) + "_near2_opp_dist,";
            header += "p_r_" + std::to_string(i) + "_near2_opp_angle,";
        }
        if (option.polarGoalCenter == OPP || option.polarGoalCenter == BOTH) {
            header += "p_r_" + std::to_string(i) + "_angle_coal_center_r,";
            header += "p_r_" + std::to_string(i) + "_angle_goal_center_t,";
        }
        if (option.openAngleGoal == OPP || option.openAngleGoal == BOTH)
            header += "p_r_" + std::to_string(i) + "_open_goal_Angle,";
        for (int j = 0; j < option.history_size; j++){
            if (option.body == OPP || option.body == BOTH)
                header += "p_r_" + std::to_string(i) + "_history_" + std::to_string(j + 1) + "_body,";
            if (option.pos == OPP || option.pos == BOTH) {
                header += "p_r_" + std::to_string(i) + "_history_" + std::to_string(j + 1) + "_pos_x,";
                header += "p_r_" + std::to_string(i) + "_history_" + std::to_string(j + 1) + "_pos_y,";
            }
            if (option.polarPos == OPP || option.polarPos == BOTH) {
                header += "p_r_" + std::to_string(i) + "_history_" + std::to_string(j + 1) + "_pos_r,";
                header += "p_r_" + std::to_string(i) + "_history_" + std::to_string(j + 1) + "_pos_t,";
            }
            if (option.vel == OPP || option.vel == BOTH) {
                header += "p_r_" + std::to_string(i) + "_history_" + std::to_string(j + 1) + "_vel_x,";
                header += "p_r_" + std::to_string(i) + "_history_" + std::to_string(j + 1) + "_vel_y,";
            }
            if (option.polarVel == OPP || option.polarVel == BOTH) {
                header += "p_r_" + std::to_string(i) + "_history_" + std::to_string(j + 1) + "_vel_r,";
                header += "p_r_" + std::to_string(i) + "_history_" + std::to_string(j + 1) + "_vel_t,";
            }
            if (option.counts == OPP || option.counts == BOTH) {
                header += "p_r_" + std::to_string(i) + "_history_" + std::to_string(j + 1) + "_pos_count,";
                header += "p_r_" + std::to_string(i) + "_history_" + std::to_string(j + 1) + "_vel_count,";
                header += "p_r_" + std::to_string(i) + "_history_" + std::to_string(j + 1) + "_body_count,";
            }
        }
    }
    header += "out_category,out_target_x,out_target_y,out_unum,out_unum_index,out_ball_speed,out_ball_dir,out_desc,";
    fout << header << std::endl;
}

void DataExtractor::extract_ball(const rcsc::WorldModel &wm) {
    if (wm.ball().posValid()) {
        ADD_ELEM("p_x", convertor_x(wm.ball().pos().x));
        ADD_ELEM("p_y", convertor_y(wm.ball().pos().y));
        ADD_ELEM("p_r", convertor_dist(wm.ball().pos().r()));
        ADD_ELEM("p_t", convertor_angle(wm.ball().pos().th().degree()));
        ADD_ELEM("kicker_x", convertor_dist_x(wm.ball().rpos().x));
        ADD_ELEM("kicker_y", convertor_dist_y(wm.ball().rpos().y));
        ADD_ELEM("kicker_r", convertor_dist(wm.ball().rpos().r()));
        ADD_ELEM("kicker_t", convertor_angle(wm.ball().rpos().th().degree()));
    } else {
        ADD_ELEM("p_x", invalid_data);
        ADD_ELEM("p_y", invalid_data);
        ADD_ELEM("p_r", invalid_data);
        ADD_ELEM("p_t", invalid_data);
        ADD_ELEM("kicker_x", invalid_data);
        ADD_ELEM("kicker_y", invalid_data);
        ADD_ELEM("kicker_r", invalid_data);
        ADD_ELEM("kicker_t", invalid_data);
    }
    if (wm.ball().velValid()) {
        ADD_ELEM("v_x", convertor_bvx(wm.ball().vel().x));
        ADD_ELEM("v_y", convertor_bvy(wm.ball().vel().y));
        ADD_ELEM("v_r", convertor_bv(wm.ball().vel().r()));
        ADD_ELEM("v_t", convertor_angle(wm.ball().vel().th().degree()));
    } else {
        ADD_ELEM("v_x", invalid_data);
        ADD_ELEM("v_y", invalid_data);
        ADD_ELEM("v_r", invalid_data);
        ADD_ELEM("v_t", invalid_data);
    }
    int len = DataExtractor::history_pos_count[0].size();
    for(int i = 0; i < option.history_size; i++){
        if(DataExtractor::history_pos_count[0][len - 2 - i] == -1){
            ADD_ELEM('history_p_x', invalid_data);
            ADD_ELEM('history_p_y', invalid_data);
            ADD_ELEM('history_p_r', invalid_data);
            ADD_ELEM('history_p_t', invalid_data);
        }
        else{
            ADD_ELEM('history_p_x', convertor_x(DataExtractor::history_pos[0][len - 2 - i].x));
            ADD_ELEM('history_p_y', convertor_y(DataExtractor::history_pos[0][len - 2 - i].y));
            ADD_ELEM('history_p_r', convertor_dist(DataExtractor::history_pos[0][len - 2 - i].r()));
            ADD_ELEM('history_p_t', convertor_angle(DataExtractor::history_pos[0][len - 2 - i].th().degree()));
        }
        if(DataExtractor::history_vel_count[0][len - 2 - i] == -1){
            ADD_ELEM('history_v_x', invalid_data);
            ADD_ELEM('history_v_y', invalid_data);
            ADD_ELEM('history_v_r', invalid_data);
            ADD_ELEM('history_v_t', invalid_data);
        }
        else{
            ADD_ELEM('history_v_x', convertor_bvx(DataExtractor::history_vel[0][len - 2 - i].x));
            ADD_ELEM('history_v_y', convertor_bvy(DataExtractor::history_vel[0][len - 2 - i].y));
            ADD_ELEM('history_v_r', convertor_bv(DataExtractor::history_vel[0][len - 2 - i].r()));
            ADD_ELEM('history_v_t', convertor_angle(DataExtractor::history_vel[0][len - 2 - i].th().degree()));
        }
    }
    ADD_ELEM("offside_count", wm.offsideLineCount());
}

void DataExtractor::extract_kicker(const rcsc::WorldModel &wm) {
    extract_drible_angles(wm);
}

void DataExtractor::extract_players(const rcsc::WorldModel &wm) {
    auto players = sort_players(wm);
    for (uint i = 0; i < players.size(); i++) {
        const AbstractPlayerObject *player = players[i];
        if (player == NULL) {
            add_null_player(invalid_data,
                            (i <= 10 ? TM : OPP));
            continue;
        }

        DataSide side = player->side() == wm.ourSide() ? TM : OPP;
        extract_base_data(player, side);
        extract_pos(player, wm, side);
        extract_vel(player, side);
        extract_counts(player, side);

        if (option.isKicker == side || option.isKicker == BOTH) {
            if (player->unum() == wm.self().unum()) {
                ADD_ELEM("is_kicker", 1);
            } else
                ADD_ELEM("is_kicker", 0);
        }

        extract_pass_angle(player, wm, side);
        extract_goal_polar(player, side);
        extract_goal_open_angle(player, wm, side);
        extract_history(player, side);
    }
}

std::vector<const AbstractPlayerObject *>
DataExtractor::sort_players(const rcsc::WorldModel &wm) {
    static int cycle = 0;
    static std::vector<const AbstractPlayerObject *> tms;
    if (wm.time().cycle() == cycle){
        return tms;
    }
    cycle = wm.time().cycle();
    tms.clear();
//    std::vector<const AbstractPlayerObject *> tms;
    std::vector<const AbstractPlayerObject *> opps;
    tms.clear();
    opps.clear();
    for (int i = 1, j = 0; i <= 11 && j <= 1; i++) {
        const AbstractPlayerObject *player;
        if (j == 0)
            player = wm.ourPlayer(i);
        else
            player = wm.theirPlayer(i);

        if (player == NULL
            || player->isGhost()
            || player->unum() < 1
            || !player->pos().isValid()
            || !player->vel().isValid())
            continue;
        if (j == 0){
            if (option.kicker_first){
                if (wm.self().unum()!=i)
                    tms.push_back(player);
            }else{
                tms.push_back(player);
            }
        }
        else
            opps.push_back(player);

        if (i == 11) {
            i = 0;
            j += 1;
        }
    }
    auto unum_sort = [](const AbstractPlayerObject *p1, const AbstractPlayerObject *p2) -> bool {
        return p1->unum() > p2->unum();
    };
    auto x_sort = [](const AbstractPlayerObject *p1, const AbstractPlayerObject *p2) -> bool {
        return p1->pos().x > p2->pos().x;
    };
    const AbstractPlayerObject *self = wm.ourPlayer(wm.self().unum());
    auto degree_sort = [self](const AbstractPlayerObject *p1, const AbstractPlayerObject *p2) -> bool {
        return (p1->pos() - self->pos()).th().abs() < (p2->pos() - self->pos()).th().abs();
    }; // TODO sort from -180 to 180, change it by ABS to change from back to front, IS IT OKEY???!

    if (option.playerSortMode == ANGLE) {
        std::sort(tms.begin(), tms.end(), degree_sort);
        std::sort(opps.begin(), opps.end(), degree_sort);
    } else if (option.playerSortMode == X) {
        std::sort(tms.begin(), tms.end(), x_sort);
        std::sort(opps.begin(), opps.end(), x_sort);
    } else if (option.playerSortMode == UNUM) {
        std::sort(tms.begin(), tms.end(), unum_sort);
        std::sort(opps.begin(), opps.end(), unum_sort);
    } else if (option.playerSortMode == RANDOM){
        std::random_shuffle(tms.begin(), tms.end());
        std::sort(opps.begin(), opps.end(), unum_sort);
    }

    if (option.kicker_first){
        for (; tms.size() < 10; tms.push_back(static_cast<AbstractPlayerObject *>(0)));
        tms.insert(tms.begin(), wm.ourPlayer(wm.self().unum()));
    }else{
        for (; tms.size() < 11; tms.push_back(static_cast<AbstractPlayerObject *>(0)));
    }

    for (; opps.size() < 11; opps.push_back(static_cast<AbstractPlayerObject *>(0)));

    tms.insert(tms.end(), opps.begin(), opps.end());

    return tms;
}


void DataExtractor::add_null_player(int unum, DataSide side) {
    if (option.side == side || option.side == BOTH)
        ADD_ELEM("side", side == OPP ? -1 : 1);
    if (option.unum == side || option.unum == BOTH)
        ADD_ELEM("unum", convertor_unum(unum));
    if (option.type == side || option.type == BOTH) {
        ADD_ELEM("player_type_dash_rate", invalid_data);
        ADD_ELEM("player_type_effort_max", invalid_data);
        ADD_ELEM("player_type_effort_min", invalid_data);
        ADD_ELEM("player_type_kickable", invalid_data);
        ADD_ELEM("player_type_margin", invalid_data);
        ADD_ELEM("player_type_kick_power", invalid_data);
        ADD_ELEM("player_type_decay", invalid_data);
        ADD_ELEM("player_type_size", invalid_data);
        ADD_ELEM("player_type_speed_max", invalid_data);
    }
    if (option.body == side || option.body == BOTH)
        ADD_ELEM("body", invalid_data);
    if (option.face == side || option.face == BOTH)
        ADD_ELEM("face", invalid_data);
    if (option.tackling == side || option.tackling == BOTH)
        ADD_ELEM("tackling", invalid_data);
    if (option.kicking == side || option.kicking == BOTH)
        ADD_ELEM("kicking", invalid_data);
    if (option.card == side || option.card == BOTH)
        ADD_ELEM("card", invalid_data);
    if (option.pos == side || option.pos == BOTH) {
        ADD_ELEM("pos_x", invalid_data);
        ADD_ELEM("pos_y", invalid_data);
    }
    if (option.polarPos == side || option.polarPos == BOTH) {
        ADD_ELEM("pos_r", invalid_data);
        ADD_ELEM("pos_t", invalid_data);
    }
    if (option.relativePos == side || option.relativePos == BOTH) {
        ADD_ELEM("kicker_x", invalid_data);
        ADD_ELEM("kicker_y", invalid_data);
        ADD_ELEM("kicker_r", invalid_data);
        ADD_ELEM("kicker_t", invalid_data);
    }
    if (option.in_offside == side || option.in_offside == BOTH) {
        ADD_ELEM("in_offside", invalid_data);
    }
    if (option.vel == side || option.vel == BOTH) {
        ADD_ELEM("v_x", invalid_data);
        ADD_ELEM("v_y", invalid_data);
    }
    if (option.polarVel == side || option.polarVel == BOTH) {
        ADD_ELEM("v_r", invalid_data);
        ADD_ELEM("v_t", invalid_data);
    }
    if (option.counts == side || option.counts == BOTH) {
        ADD_ELEM("pos_count", invalid_data);
        ADD_ELEM("vel_count", invalid_data);
        ADD_ELEM("body_count", invalid_data);
    }
    if (option.isKicker == side || option.isKicker == BOTH)
        ADD_ELEM("is_kicker", invalid_data);
    if (option.openAnglePass == side || option.openAnglePass == BOTH) {
        ADD_ELEM("pass_dist", invalid_data);
        ADD_ELEM("pass_opp1_dist", invalid_data);
        ADD_ELEM("pass_opp1_angle", invalid_data);
        ADD_ELEM("pass_opp1_dist_line", invalid_data);
        ADD_ELEM("pass_opp1_dist_proj", invalid_data);
        ADD_ELEM("pass_opp1_dist_diffbody", invalid_data);
        ADD_ELEM("pass_opp2_dist", invalid_data);
        ADD_ELEM("pass_opp2_angle", invalid_data);
        ADD_ELEM("pass_opp2_dist_line", invalid_data);
        ADD_ELEM("pass_opp2_dist_proj", invalid_data);
        ADD_ELEM("pass_opp2_dist_diffbody", invalid_data);
    }
    if (option.nearestOppDist == side || option.nearestOppDist == BOTH){
        ADD_ELEM("opp1_dist", invalid_data);
        ADD_ELEM("opp1_angle", invalid_data);
        ADD_ELEM("opp1_diffbody", invalid_data);
        ADD_ELEM("opp2_dist", invalid_data);
        ADD_ELEM("opp2_angle", invalid_data);
        ADD_ELEM("opp2_diffbody", invalid_data);
    }

    if (option.polarGoalCenter == side || option.polarGoalCenter == BOTH) {
        ADD_ELEM("angle_goal_center_r", invalid_data);
        ADD_ELEM("angle_goal_center_t", invalid_data);
    }
    if (option.openAngleGoal == side || option.openAngleGoal == BOTH)
        ADD_ELEM("open_goal_angle", invalid_data);
    for(int i = 0; i < option.history_size; i++) {
        if (option.body == side || option.body == BOTH){
            ADD_ELEM('history_body', invalid_data);
        }
        if (option.pos == side || option.pos == BOTH) {
            ADD_ELEM('history_pos_x', invalid_data);
            ADD_ELEM('history_pos_y', invalid_data);
        }
        if (option.polarPos == side || option.polarPos == BOTH) {
            ADD_ELEM('history_pos_r', invalid_data);
            ADD_ELEM('history_pos_t', invalid_data);
        }


        if (option.vel == side || option.vel == BOTH) {
            ADD_ELEM('history_vel_x', invalid_data);
            ADD_ELEM('history_vel_y', invalid_data);
        }
        if (option.polarVel == side || option.polarVel == BOTH) {
            ADD_ELEM('history_vel_r', invalid_data);
            ADD_ELEM('history_vel_t', invalid_data);
        }
        if (option.counts == side || option.counts == BOTH){
            ADD_ELEM('history_pos_count', invalid_data);
            ADD_ELEM('history_vel_count', invalid_data);
            ADD_ELEM('history_body_count', invalid_data);
        }
    }
}

void DataExtractor::extract_output(const rcsc::WorldModel &wm,
                                   int category,
                                   const rcsc::Vector2D &target,
                                   const int &unum,
                                   const char *desc,
                                   double ball_speed) {
    ADD_ELEM("category", category);
    ADD_ELEM("target_x", convertor_x(target.x));
    ADD_ELEM("target_y", convertor_y(target.y));
    ADD_ELEM("unum", unum);
    ADD_ELEM("unum_index", find_unum_index(wm, unum));
    ADD_ELEM("ball_speed", convertor_bv(ball_speed));
    ADD_ELEM("ball_dir", convertor_angle((target - wm.ball().pos()).th().degree()));
    if (category == 2) {
        if (std::string(desc) == "strictDirect") {
            ADD_ELEM("desc", 0);
        } else if (std::string(desc) == "strictLead") {
            ADD_ELEM("desc", 1);
        } else if (std::string(desc) == "strictThrough") {
            ADD_ELEM("desc", 2);
        } else if (std::string(desc) == "cross") {
            ADD_ELEM("desc", 3);
        }
    } else {
        ADD_ELEM("desc", 4);
    }
}
void DataExtractor::update_for_shoot(const rcsc::PlayerAgent *agent, rcsc::Vector2D target, double ballsp){
    update(agent, NULL, true);
    const WorldModel &wm = option.input_worldMode == FULLSTATE ? agent->fullstateWorld() : agent->world();
    ADD_ELEM("category", 3);
    ADD_ELEM("target_x", convertor_x(target.x));
    ADD_ELEM("target_y", convertor_y(target.y));
    ADD_ELEM("unum", wm.self().unum());
    ADD_ELEM("unum_index", find_unum_index(wm, wm.self().unum()));
    ADD_ELEM("ball_speed", convertor_bv(ballsp));
    ADD_ELEM("ball_dir", convertor_angle((target - wm.ball().pos()).th().degree()));
    ADD_ELEM("desc", 4);
    fout << std::endl;
}
void DataExtractor::extract_pass_angle(const AbstractPlayerObject *player, const WorldModel &wm, DataSide side) {
    Vector2D ball_pos = wm.ball().pos();
    Vector2D tm_pos = player->pos();
    std::vector<std::pair<double, double>> opp_dist_angle;
    std::vector<std::pair<double, double>> opp_dist_body_diff;
    std::vector<std::pair<double, double>> opp_pass_angle_dist;
    std::vector<std::pair<double, double>> opp_pass_projection;
    std::vector<std::pair<double, double>> opp_pass_projection_bodydiff;
    for (int o = 1; o <= 11; o++) {
        const AbstractPlayerObject *opp = wm.theirPlayer(o);
        if (opp == nullptr || opp->unum() != o)
            continue;
        opp_dist_angle.push_back(std::make_pair(opp->pos().dist(ball_pos), (opp->pos() - ball_pos).th().degree()));
        opp_dist_body_diff.push_back(std::make_pair(opp->pos().dist(ball_pos), ((ball_pos - opp->pos()).th() - opp->body()).abs()));
        AngleDeg diff = (tm_pos - ball_pos).th() - (opp->pos() - ball_pos).th();
        if (diff.abs() > 50)
            continue;
        if (opp->pos().dist(ball_pos) > tm_pos.dist(ball_pos) + 1.0)
            continue;
        opp_pass_angle_dist.push_back(std::make_pair(diff.abs(), opp->distFromBall()));
        Vector2D proj_pos = Line2D(ball_pos, tm_pos).projection(opp->pos());
        opp_pass_projection.push_back(std::make_pair(proj_pos.dist(opp->pos()), proj_pos.dist(ball_pos)));
        opp_pass_projection_bodydiff.push_back(std::make_pair(proj_pos.dist(opp->pos()), ((proj_pos - opp->pos()).th() - opp->body()).abs()));
    }
    if (option.openAnglePass == side || option.openAnglePass == BOTH) {
        ADD_ELEM("pass_dist", convertor_dist(ball_pos.dist(tm_pos)));
        std::sort(opp_pass_angle_dist.begin(), opp_pass_angle_dist.end());
        std::sort(opp_pass_projection.begin(), opp_pass_projection.end());
        std::sort(opp_pass_projection_bodydiff.begin(), opp_pass_projection_bodydiff.end());
        if (opp_pass_angle_dist.size() >= 1){
            ADD_ELEM("pass_opp1_dist", convertor_dist(opp_pass_angle_dist[0].second));
            ADD_ELEM("pass_opp1_angle", convertor_angle(opp_pass_angle_dist[0].first));
            ADD_ELEM("pass_opp1_dist_line", convertor_dist(opp_pass_projection[0].first));
            ADD_ELEM("pass_opp1_dist_proj", convertor_dist(opp_pass_projection[0].second));
            ADD_ELEM("pass_opp1_dist_diffbody", convertor_angle(opp_pass_projection_bodydiff[0].second));
        }
        else{
            ADD_ELEM("pass_opp1_dist", invalid_data);
            ADD_ELEM("pass_opp1_angle", invalid_data);
            ADD_ELEM("pass_opp1_dist_line", invalid_data);
            ADD_ELEM("pass_opp1_dist_proj", invalid_data);
            ADD_ELEM("pass_opp1_dist_diffbody", invalid_data);
        }
        if (opp_pass_angle_dist.size() >= 2){
            ADD_ELEM("pass_opp2_dist", convertor_dist(opp_pass_angle_dist[1].second));
            ADD_ELEM("pass_opp2_angle", convertor_angle(opp_pass_angle_dist[1].first));
            ADD_ELEM("pass_opp2_dist_line", convertor_dist(opp_pass_projection[1].first));
            ADD_ELEM("pass_opp2_dist_proj", convertor_dist(opp_pass_projection[1].second));
            ADD_ELEM("pass_opp2_dist_diffbody", convertor_angle(opp_pass_projection_bodydiff[1].second));
        }
        else{
            ADD_ELEM("pass_opp2_dist", invalid_data);
            ADD_ELEM("pass_opp2_angle", invalid_data);
            ADD_ELEM("pass_opp2_dist_line", invalid_data);
            ADD_ELEM("pass_opp2_dist_proj", invalid_data);
            ADD_ELEM("pass_opp2_dist_diffbody", invalid_data);
        }
    }
    if (option.nearestOppDist == side || option.nearestOppDist == BOTH){
        std::sort(opp_dist_angle.begin(), opp_dist_angle.end());
        std::sort(opp_dist_body_diff.begin(), opp_dist_body_diff.end());
        if (opp_dist_angle.size() >= 1){
            ADD_ELEM("opp1_dist", convertor_dist(opp_dist_angle[0].first));
            ADD_ELEM("opp1_angle", convertor_angle(opp_dist_angle[0].second));
            ADD_ELEM("opp1_diffbody", convertor_angle(opp_dist_body_diff[0].second));
        }
        else{
            ADD_ELEM("opp1_dist", invalid_data);
            ADD_ELEM("opp1_angle", invalid_data);
            ADD_ELEM("opp1_diffbody", invalid_data);
        }
        if (opp_dist_angle.size() >= 2){
            ADD_ELEM("opp2_dist", convertor_dist(opp_dist_angle[1].first));
            ADD_ELEM("opp2_angle", convertor_angle(opp_dist_angle[1].second));
            ADD_ELEM("opp2_diffbody", convertor_angle(opp_dist_body_diff[1].second));
        }
        else{
            ADD_ELEM("opp2_dist", invalid_data);
            ADD_ELEM("opp2_angle", invalid_data);
            ADD_ELEM("opp2_diffbody", invalid_data);
        }
    }
}

void DataExtractor::extract_vel(const AbstractPlayerObject *player, DataSide side) {
    if (option.vel == side || option.vel == BOTH) {
        ADD_ELEM("v_x", convertor_pvx(player->vel().x));
        ADD_ELEM("v_y", convertor_pvy(player->vel().y));
    }
    if (option.polarVel == side || option.polarVel == BOTH) {
        ADD_ELEM("v_r", convertor_pv(player->vel().r()));
        ADD_ELEM("v_t", convertor_angle(player->vel().th().degree()));
    }
}

void DataExtractor::extract_pos(const AbstractPlayerObject *player, const WorldModel &wm, DataSide side) {
    if (option.pos == side || option.pos == BOTH) {
        ADD_ELEM("pos_x", convertor_x(player->pos().x));
        ADD_ELEM("pos_y", convertor_y(player->pos().y));
    }
    if (option.polarPos == side || option.polarPos == BOTH) {
        ADD_ELEM("pos_r", convertor_dist(player->pos().r()));
        ADD_ELEM("pos_t", convertor_angle(player->pos().th().degree()));
    }
    Vector2D rpos = player->pos() - wm.self().pos();
    if (option.relativePos == side || option.relativePos == BOTH) {
        ADD_ELEM("kicker_x", convertor_dist_x(rpos.x));
        ADD_ELEM("kicker_y", convertor_dist_y(rpos.y));
        ADD_ELEM("kicker_r", convertor_dist(rpos.r()));
        ADD_ELEM("kicker_t", convertor_angle(rpos.th().degree()));
    }

    if (option.in_offside == side || option.in_offside == BOTH) {
        if (player->pos().x > wm.offsideLineX()) {
            ADD_ELEM("pos_offside", 1);
        } else {
            ADD_ELEM("pos_offside", 0);
        }
    }
}

void DataExtractor::extract_goal_polar(const AbstractPlayerObject *player, DataSide side) {
    if (!(option.polarGoalCenter == side || option.polarGoalCenter == BOTH))
        return;
    Vector2D goal_center = Vector2D(52, 0);
    ADD_ELEM("angle_goal_center_r", convertor_dist((goal_center - player->pos()).r()));
    ADD_ELEM("angle_goal_center_t", convertor_angle((goal_center - player->pos()).th().degree()));
}

void DataExtractor::extract_goal_open_angle(const rcsc::AbstractPlayerObject *player,
                                            const rcsc::WorldModel &wm,
                                            DataSide side) {
    if (!(option.openAngleGoal == side || option.openAngleGoal == BOTH))
        return;
    Vector2D goal_t = Vector2D(52, -7);
    Vector2D goal_b = Vector2D(52, 7);
    Triangle2D player_goal_area = Triangle2D(goal_b, goal_t, player->pos());

    std::vector<Vector2D> players_in_area;

    for (int i = 1; i <= 11; i++) {
        const AbstractPlayerObject *opp = wm.theirPlayer(i);
        if (opp == NULL || opp->unum() < 1)
            continue;
        if (!player_goal_area.contains(opp->pos()))
            continue;

        players_in_area.push_back(opp->pos());
    }
    players_in_area.push_back(goal_t);
    players_in_area.push_back(goal_b);

    std::sort(players_in_area.begin(), players_in_area.end(),
              [player](Vector2D p1, Vector2D p2) -> bool {
                  return (p1 - player->pos()).th().degree() < (p2 - player->pos()).th().degree();
              });

    double max_open_angle = 0;
    for (uint i = 1; i < players_in_area.size(); i++) {
        double angle_diff = fabs((players_in_area[i] - player->pos()).th().degree()
                                 - (players_in_area[i - 1] - player->pos()).th().degree());
        if (angle_diff > max_open_angle)
            max_open_angle = angle_diff;
    }


    ADD_ELEM("goal_open_angle", convertor_angle(max_open_angle));
}

void DataExtractor::extract_base_data(const rcsc::AbstractPlayerObject *player, DataSide side) {
    if (option.side == side || option.side == BOTH)
        ADD_ELEM("side", player->side());
    if (option.unum == side || option.unum == BOTH)
        ADD_ELEM("unum", convertor_unum(player->unum()));
    if (option.type == side || option.type == BOTH)
        extract_type(player, side);
    if (option.body == side || option.body == BOTH)
        ADD_ELEM("body", convertor_angle(player->body().degree()));
    if (option.face == side || option.face == BOTH)
        ADD_ELEM("face", convertor_angle(player->face().degree()));
    if (option.tackling == side || option.tackling == BOTH)
        ADD_ELEM("tackling", player->isTackling());
    if (option.kicking == side || option.kicking == BOTH)
        ADD_ELEM("kicking", player->kicked());
    if (option.card == side || option.card == BOTH)
        ADD_ELEM("card", convertor_card(player->card()));
}

void DataExtractor::extract_type(const AbstractPlayerObject *player, DataSide side) {
    ADD_ELEM("player_type_dash_rate", player->playerTypePtr()->dashPowerRate());
    ADD_ELEM("player_type_effort_max", player->playerTypePtr()->effortMax());
    ADD_ELEM("player_type_effort_min", player->playerTypePtr()->effortMin());
    ADD_ELEM("player_type_kickable", player->playerTypePtr()->kickableArea());
    ADD_ELEM("player_type_margin", player->playerTypePtr()->kickableMargin());
    ADD_ELEM("player_type_kick_power", player->playerTypePtr()->kickPowerRate());
    ADD_ELEM("player_type_decay", player->playerTypePtr()->playerDecay());
    ADD_ELEM("player_type_size", player->playerTypePtr()->playerSize());
    ADD_ELEM("player_type_speed_max", player->playerTypePtr()->realSpeedMax());
}

void DataExtractor::extract_counts(const rcsc::AbstractPlayerObject *player, DataSide side) {
    if (!(option.counts == side || option.counts == BOTH))
        return;

    ADD_ELEM("pos_count", convertor_counts(player->posCount()));
    ADD_ELEM("vel_count", convertor_counts(player->velCount()));
    ADD_ELEM("body_count", convertor_counts(player->bodyCount()));
}

void DataExtractor::extract_history(const rcsc::AbstractPlayerObject *player, DataSide side) {
    int len = DataExtractor::history_pos_count[0].size();
    int unum = player->unum();
    if (side == OPP)
        unum += 11;
    for(int i = 0; i < option.history_size; i++) {
        if (option.body == side || option.body == BOTH){
            if (DataExtractor::history_body_count[unum][len - 2 - i] == -1) {
                ADD_ELEM('history_body', invalid_data);
            }else{
                ADD_ELEM('history_body', convertor_angle(DataExtractor::history_body[unum][len - 2 - i].degree()));
            }
        }
        if (option.pos == side || option.pos == BOTH) {
            if (DataExtractor::history_pos_count[unum][len - 2 - i] == -1) {
                ADD_ELEM('history_pos_x', invalid_data);
                ADD_ELEM('history_pos_y', invalid_data);
            }else{
                ADD_ELEM('history_pos_x', convertor_x(DataExtractor::history_pos[unum][len - 2 - i].x));
                ADD_ELEM('history_pos_y', convertor_y(DataExtractor::history_pos[unum][len - 2 - i].y));
            }
        }
        if (option.polarPos == side || option.polarPos == BOTH) {
            if (DataExtractor::history_pos_count[unum][len - 2 - i] == -1) {
                ADD_ELEM('history_pos_r', invalid_data);
                ADD_ELEM('history_pos_t', invalid_data);
            }else{
                ADD_ELEM('history_pos_r', convertor_dist(DataExtractor::history_pos[unum][len - 2 - i].r()));
                ADD_ELEM('history_pos_t', convertor_angle(DataExtractor::history_pos[unum][len - 2 - i].th().degree()));
            }
        }
        if (option.vel == side || option.vel == BOTH) {
            if (DataExtractor::history_vel_count[unum][len - 2 - i] == -1) {
                ADD_ELEM('history_vel_x', invalid_data);
                ADD_ELEM('history_vel_y', invalid_data);
            }else{
                ADD_ELEM('history_vel_x', convertor_pvx(DataExtractor::history_vel[unum][len - 2 - i].x));
                ADD_ELEM('history_vel_y', convertor_pvy(DataExtractor::history_vel[unum][len - 2 - i].y));
            }
        }
        if (option.polarVel == side || option.polarVel == BOTH) {
            if (DataExtractor::history_vel_count[unum][len - 2 - i] == -1) {
                ADD_ELEM('history_vel_r', invalid_data);
                ADD_ELEM('history_vel_t', invalid_data);
            }else{
                ADD_ELEM('history_vel_r', convertor_pv(DataExtractor::history_vel[unum][len - 2 - i].r()));
                ADD_ELEM('history_vel_t', convertor_angle(DataExtractor::history_vel[unum][len - 2 - i].th().degree()));
            }
        }
        if (option.counts == side || option.counts == BOTH){
            ADD_ELEM('history_pos_count', convertor_counts(DataExtractor::history_pos_count[unum][len - 2 - i]));
            ADD_ELEM('history_vel_count', convertor_counts(DataExtractor::history_pos_count[unum][len - 2 - i]));
            ADD_ELEM('history_body_count', convertor_counts(DataExtractor::history_pos_count[unum][len - 2 - i]));
        }
    }
}

void DataExtractor::extract_drible_angles(const WorldModel &wm) {

//    const PlayerObject *kicker = wm.interceptTable()->fastestTeammate(); // TODO What is error ?!?!
    if (option.dribleAngle != Kicker)
        return;
    const AbstractPlayerObject *kicker = wm.ourPlayer(wm.self().unum());
    if (kicker == NULL || kicker->unum() < 0) {
        for (int i = 1; i <= option.nDribleAngle; i++)
            ADD_ELEM("dribble_angle", -2);
        return;
    }

    double delta = 360.0 / option.nDribleAngle;
    for (double angle = -180; angle < 180; angle += delta) {

        double min_opp_dist = 30;
        for (int i = 1; i <= 11; i++) {
            const AbstractPlayerObject *opp = wm.theirPlayer(i);
            if (opp == NULL || opp->unum() < 0)
                continue;

            Vector2D kicker2opp = (opp->pos() - kicker->pos());
            if (kicker2opp.th().degree() < angle || kicker2opp.th().degree() > angle + delta)
                continue;

            double dist = kicker2opp.r();
            if (dist < min_opp_dist)
                min_opp_dist = dist;
        }

        ADD_ELEM("dribble_angle", convertor_dist(min_opp_dist));
    }
}

double DataExtractor::convertor_x(double x) {
    if (!option.use_convertor)
        return x;
//    return x / 52.5;
    return std::min(std::max((x + 52.5) / 105.0, 0.0), 1.0);
}

double DataExtractor::convertor_y(double y) {
    if (!option.use_convertor)
        return y;
//    return y / 34.0;
    return std::min(std::max((y + 34) / 68.0, 0.0), 1.0);
}

double DataExtractor::convertor_dist(double dist) {
    if (!option.use_convertor)
        return dist;
//    return dist / 63.0 - 1.0;
    return dist / 123.0;
}

double DataExtractor::convertor_dist_x(double dist) {
    if (!option.use_convertor)
        return dist;
//    return dist / 63.0 - 1.0;
    return dist / 105.0;
}

double DataExtractor::convertor_dist_y(double dist) {
    if (!option.use_convertor)
        return dist;
//    return dist / 63.0 - 1.0;
    return dist / 68.0;
}

double DataExtractor::convertor_angle(double angle) {
    if (!option.use_convertor)
        return angle;
//    return angle / 180.0;
    return (angle + 180.0) / 360.0;
}

double DataExtractor::convertor_type(double type) {
    if (!option.use_convertor)
        return type;
//    return type / 9.0 - 1.0;
    return type / 18.0;
}

double DataExtractor::convertor_cycle(double cycle) {
    if (!option.use_convertor)
        return cycle;
//    return cycle / 3000.0 - 1.0;
    return cycle / 6000.0;
}

double DataExtractor::convertor_bv(double bv) {
    if (!option.use_convertor)
        return bv;
//    return bv / 3.0 * 2 - 1;
    return bv / 3.0;
}

double DataExtractor::convertor_bvx(double bvx) {
    if (!option.use_convertor)
        return bvx;
//    return bvx / 3.0;
    return (bvx + 3.0) / 6.0;
}

double DataExtractor::convertor_bvy(double bvy) {
    if (!option.use_convertor)
        return bvy;
//    return bvy / 3.0;
    return (bvy + 3.0) / 6.0;
}

double DataExtractor::convertor_pv(double pv) {
    if (!option.use_convertor)
        return pv;
    return pv / 1.5;
}

double DataExtractor::convertor_pvx(double pvx) {
    if (!option.use_convertor)
        return pvx;
//    return pvx / 1.5;
    return (pvx + 1.5) / 3.0;
}

double DataExtractor::convertor_pvy(double pvy) {
    if (!option.use_convertor)
        return pvy;
//    return pvy / 1.5;
    return (pvy + 1.5) / 3.0;
}

double DataExtractor::convertor_unum(double unum) {
    if (!option.use_convertor)
        return unum;
    return unum / 11.0;
}

double DataExtractor::convertor_card(double card) {
    if (!option.use_convertor)
        return card;
    return card / 2.0;
}

double DataExtractor::convertor_stamina(double stamina) {
    if (!option.use_convertor)
        return stamina;
    return stamina / 8000.0;
}

double DataExtractor::convertor_counts(double count) {
    count = std::min(count, 20.0);
    if (!option.use_convertor)
        return count;
    return count / 20; // TODO I Dont know the MAX???
}

uint DataExtractor::find_unum_index(const rcsc::WorldModel &wm, uint unum) {
    auto players = sort_players(wm);
    for (uint i = 0; i < 11; i++) {
        auto player = players[i];
        if (player == NULL)
            return 0;
        if (player->unum() == unum)
            return i + 1; // TODO add 1 or not??
    }
}

Polar::Polar(rcsc::Vector2D p) {
    teta = p.th().degree();
    r = p.r();
}

DataExtractor::Option::Option() {
    side = NONE;
    unum = BOTH;
    type = BOTH;
    body = BOTH;
    face = BOTH;
    tackling = NONE;
    kicking = NONE;
    card = NONE;
    pos = BOTH;
    relativePos = BOTH;
    polarPos = BOTH;
    vel = BOTH;
    polarVel = BOTH;
    counts = NONE;
    isKicker = TM;
    openAnglePass = TM;
    nearestOppDist = TM;
    polarGoalCenter = TM;
    openAngleGoal = NONE;
    in_offside = TM;

    dribleAngle = NONE;
    nDribleAngle = 12;
    history_size = 0;
    input_worldMode = FULLSTATE;
    output_worldMode = FULLSTATE;
    playerSortMode = UNUM;
    kicker_first = false;
    use_convertor = true;
}
