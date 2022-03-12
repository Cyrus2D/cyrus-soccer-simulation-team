//
// Created by aref on 11/28/19.
//

#include "offensive_data_extractor.h"
#include "../chain_action/cooperative_action.h"
#include "../sample_player.h"
#include "../chain_action/action_state_pair.h"
#include <rcsc/player/world_model.h>
#include <random>
#include <time.h>
#include <vector>
#include <rcsc/common/logger.h>

#define ODEDebug

#define cm ","
//#define ADD_ELEM(key, value) fout << (value) << cm
#define ADD_ELEM(key, value) features.push_back(value)

double invalid_data_ = -2.0;
bool OffensiveDataExtractor::active = false;

using namespace rcsc;

std::vector<std::vector<rcsc::Vector2D>> OffensiveDataExtractor::history_pos;
std::vector<std::vector<rcsc::Vector2D>> OffensiveDataExtractor::history_vel;
std::vector<std::vector<rcsc::AngleDeg>> OffensiveDataExtractor::history_body;
std::vector<std::vector<int>> OffensiveDataExtractor::history_pos_count;
std::vector<std::vector<int>> OffensiveDataExtractor::history_vel_count;
std::vector<std::vector<int>> OffensiveDataExtractor::history_body_count;

OffensiveDataExtractor::OffensiveDataExtractor() :
        last_update_cycle(-1) {
}

OffensiveDataExtractor::~OffensiveDataExtractor() {
}


OffensiveDataExtractor::Option::Option() {
    cycle = true; //
    ball_pos = true;
    ball_vel = false;
    ball_kicker_pos = true;
    offside_count = true;
    side = NONE;
    unum = BOTH;
    type = NONE;
    body = BOTH;
    face = NONE;
    tackling = NONE;
    kicking = NONE;
    card = NONE;
    pos = BOTH;
    relativePos = BOTH;
    polarPos = BOTH;
    vel = NONE;
    polarVel = NONE;
    pos_counts = BOTH;
    vel_counts = NONE;
    body_counts = NONE;
    isKicker = TM;
    isGhost = TM;
    openAnglePass = TM;
    nearestOppDist = TM;
    polarGoalCenter = TM;
    openAngleGoal = NONE;
    in_offside = TM;
    dribleAngle = NONE;
    nDribleAngle = 12;
    history_size = 0;
    input_worldMode = NONE_FULLSTATE;
    output_worldMode = NONE_FULLSTATE;
    playerSortMode = X;
    kicker_first = false;
    use_convertor = false;
}


void OffensiveDataExtractor::init_file(DEState &state) {
    #ifdef ODEDebug
    dlog.addText(Logger::BLOCK, "start init_file");
    #endif
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    std::string dir = "/home/nader/workspace/robo/cyrus/data/";
    strftime(buffer, sizeof(buffer), "%Y-%m-%d-%H-%M-%S", timeinfo);
    std::string str(buffer);
    std::string rand_name = std::to_string(SamplePlayer::player_port);
    str += "_" + std::to_string(state.wm().self().unum()) + "_" + state.wm().opponentTeamName() + "_E" + rand_name + ".csv";

    fout = std::ofstream((dir + str).c_str());
    std::string header = get_header();
    #ifdef ODEDebug
        dlog.addText(Logger::BLOCK, header.c_str());
    #endif
    fout << header << std::endl;
}


std::string OffensiveDataExtractor::get_header() {
    std::string header = "";
    // Cycle and BALL
    if (option.cycle)
        header += "cycle,";
    if (option.ball_pos){
        header += std::string("ball_pos_x,ball_pos_y,ball_pos_r,ball_pos_t,");
    }
    if (option.ball_kicker_pos){
        header += std::string("ball_kicker_x,ball_kicker_y,ball_kicker_r,ball_kicker_t,");
    }
    if (option.ball_vel){
        header += std::string("ball_vel_x,ball_vel_y,ball_vel_r,ball_vel_t,");
    }

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
    if (option.offside_count){
        header += "offside_count,";
    }

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
        if (option.pos_counts == TM || option.pos_counts == BOTH)
            header += "p_l_" + std::to_string(i) + "_pos_count,";
        if (option.vel_counts == TM || option.vel_counts == BOTH)
            header += "p_l_" + std::to_string(i) + "_vel_count,";
        if (option.body_counts == TM || option.body_counts == BOTH)
            header += "p_l_" + std::to_string(i) + "_body_count,";
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

        if (option.isKicker == TM || option.isKicker == BOTH)
            header += "p_l_" + std::to_string(i) + "_is_kicker,";
        if (option.isGhost == TM || option.isGhost == BOTH)
            header += "p_l_" + std::to_string(i) + "_is_ghost,";
        if (option.openAnglePass == TM || option.openAnglePass == BOTH) {
            header += "p_l_" + std::to_string(i) + "_pass_dist,";
            header += "p_l_" + std::to_string(i) + "_pass_opp1_dist,";
            header += "p_l_" + std::to_string(i) + "_pass_opp1_dist_proj_to_opp,";
            header += "p_l_" + std::to_string(i) + "_pass_opp1_dist_proj_to_kicker,";
            header += "p_l_" + std::to_string(i) + "_pass_opp1_open_angle,";
            header += "p_l_" + std::to_string(i) + "_pass_opp1_dist_diffbody,";
            header += "p_l_" + std::to_string(i) + "_pass_opp2_dist,";
            header += "p_l_" + std::to_string(i) + "_pass_opp2_dist_proj_to_opp,";
            header += "p_l_" + std::to_string(i) + "_pass_opp2_dist_proj_to_kicker,";
            header += "p_l_" + std::to_string(i) + "_pass_opp2_open_angle,";
            header += "p_l_" + std::to_string(i) + "_pass_opp2_dist_diffbody,";
            header += "p_l_" + std::to_string(i) + "_pass_opp3_dist,";
            header += "p_l_" + std::to_string(i) + "_pass_opp3_dist_proj_to_opp,";
            header += "p_l_" + std::to_string(i) + "_pass_opp3_dist_proj_to_kicker,";
            header += "p_l_" + std::to_string(i) + "_pass_opp3_open_angle,";
            header += "p_l_" + std::to_string(i) + "_pass_opp3_dist_diffbody,";
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
            if (option.pos_counts == TM || option.pos_counts == BOTH)
                header += "p_l_" + std::to_string(i) + "_history_" + std::to_string(j + 1) + "_pos_count,";
            if (option.vel_counts == TM || option.vel_counts == BOTH)
                header += "p_l_" + std::to_string(i) + "_history_" + std::to_string(j + 1) + "_vel_count,";
            if (option.body_counts == TM || option.body_counts == BOTH)
                header += "p_l_" + std::to_string(i) + "_history_" + std::to_string(j + 1) + "_body_count,";
        }
    }
    for (int i = 1; i <= 15; i++) {
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
        if (option.pos_counts == OPP || option.pos_counts == BOTH)
            header += std::string("p_r_") + std::to_string(i) + "_pos_count,";
        if (option.vel_counts == OPP || option.vel_counts == BOTH)
            header += std::string("p_r_") + std::to_string(i) + "_vel_count,";
        if (option.body_counts == OPP || option.body_counts == BOTH)
            header += std::string("p_r_") + std::to_string(i) + "_body_count,";
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

        if (option.isKicker == OPP || option.isKicker == BOTH)
            header += "p_r_" + std::to_string(i) + "_is_kicker,";
        if (option.isGhost == OPP || option.isGhost == BOTH)
            header += "p_r_" + std::to_string(i) + "_is_ghost,";
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
            if (option.pos_counts == OPP || option.pos_counts == BOTH)
                header += "p_r_" + std::to_string(i) + "_history_" + std::to_string(j + 1) + "_pos_count,";
            if (option.vel_counts == OPP || option.vel_counts == BOTH)
                header += "p_r_" + std::to_string(i) + "_history_" + std::to_string(j + 1) + "_vel_count,";
            if (option.body_counts == OPP || option.body_counts == BOTH)
                header += "p_r_" + std::to_string(i) + "_history_" + std::to_string(j + 1) + "_body_count,";
        }
    }
    header += "out_category,out_target_x,out_target_y,out_unum,out_unum_index,out_ball_speed,out_ball_dir,out_desc,";
    return header;
}
//void OffensiveDataExtractor::update_history(const rcsc::PlayerAgent *agent){
//    DEState &state = option.input_worldMode == FULLSTATE ? agent->fullstateWorld() : agent->world();
//    static int last_update = -1;
//    if (last_update == -1){
//        for (int i = 0; i <= 22; i++){
//            std::vector<Vector2D> temp_pos(5, Vector2D(0,0));
//            std::vector<Vector2D> temp_vel(5, Vector2D(0,0));
//            std::vector<AngleDeg> temp_body(5, AngleDeg(0));
//            std::vector<int> temp_pos_count(5, -1);
//            OffensiveDataExtractor::history_pos.push_back(temp_pos);
//            OffensiveDataExtractor::history_vel.push_back(temp_vel);
//            OffensiveDataExtractor::history_body.push_back(temp_body);
//            OffensiveDataExtractor::history_pos_count.push_back(temp_pos_count);
//            OffensiveDataExtractor::history_vel_count.push_back(temp_pos_count);
//            OffensiveDataExtractor::history_body_count.push_back(temp_pos_count);
//        }
//    }
//    if (last_update == state.time().cycle())
//        return;
//    last_update = state.time().cycle();
//    if (state.ball().posValid()){
//        OffensiveDataExtractor::history_pos[0].push_back(state.ball().pos());
//        OffensiveDataExtractor::history_pos_count[0].push_back(state.ball().posCount());
//    }else{
//        OffensiveDataExtractor::history_pos[0].push_back(Vector2D::INVALIDATED);
//        OffensiveDataExtractor::history_pos_count[0].push_back(-1);
//    }
//    if (state.ball().velValid()){
//        OffensiveDataExtractor::history_vel[0].push_back(state.ball().vel());
//        OffensiveDataExtractor::history_vel_count[0].push_back(state.ball().velCount());
//    }else{
//        OffensiveDataExtractor::history_vel[0].push_back(Vector2D::INVALIDATED);
//        OffensiveDataExtractor::history_vel_count[0].push_back(-1);
//    }
//    for (int i=1; i<=11; i++){
//        const DEPlayer * p = state.ourPlayer(i);
//        if(p == NULL || p->unum() != i){
//            OffensiveDataExtractor::history_pos[i].push_back(Vector2D::INVALIDATED);
//            OffensiveDataExtractor::history_pos_count[i].push_back(-1);
//            OffensiveDataExtractor::history_vel[i].push_back(Vector2D::INVALIDATED);
//            OffensiveDataExtractor::history_vel_count[i].push_back(-1);
//            OffensiveDataExtractor::history_body[i].push_back(AngleDeg(0));
//            OffensiveDataExtractor::history_body_count[i].push_back(-1);
//        }else{
//            if(p->pos().isValid()){
//                OffensiveDataExtractor::history_pos[i].push_back(p->pos());
//                OffensiveDataExtractor::history_pos_count[i].push_back(p->posCount());
//            }else{
//                OffensiveDataExtractor::history_pos[i].push_back(Vector2D::INVALIDATED);
//                OffensiveDataExtractor::history_pos_count[i].push_back(-1);
//            }
//            if(p->vel().isValid()){
//                OffensiveDataExtractor::history_vel[i].push_back(p->vel());
//                OffensiveDataExtractor::history_vel_count[i].push_back(p->velCount());
//            }else{
//                OffensiveDataExtractor::history_vel[i].push_back(Vector2D::INVALIDATED);
//                OffensiveDataExtractor::history_vel_count[i].push_back(-1);
//            }
//            OffensiveDataExtractor::history_body[i].push_back(p->body());
//            OffensiveDataExtractor::history_body_count[i].push_back(p->bodyCount());
//        }
//    }
//    for (int i=1; i<=11; i++){
//        const DEPlayer * p = state.theirPlayer(i);
//        if(p == NULL || p->unum() != i){
//            OffensiveDataExtractor::history_pos[i+11].push_back(Vector2D::INVALIDATED);
//            OffensiveDataExtractor::history_pos_count[i+11].push_back(-1);
//            OffensiveDataExtractor::history_vel[i+11].push_back(Vector2D::INVALIDATED);
//            OffensiveDataExtractor::history_vel_count[i+11].push_back(-1);
//            OffensiveDataExtractor::history_body[i+11].push_back(AngleDeg(0));
//            OffensiveDataExtractor::history_body_count[i+11].push_back(-1);
//        }else{
//            if(p->pos().isValid()){
//                OffensiveDataExtractor::history_pos[i+11].push_back(p->pos());
//                OffensiveDataExtractor::history_pos_count[i+11].push_back(p->posCount());
//            }else{
//                OffensiveDataExtractor::history_pos[i+11].push_back(Vector2D::INVALIDATED);
//                OffensiveDataExtractor::history_pos_count[i+11].push_back(-1);
//            }
//            if(p->vel().isValid()){
//                OffensiveDataExtractor::history_vel[i+11].push_back(p->vel());
//                OffensiveDataExtractor::history_vel_count[i+11].push_back(p->velCount());
//            }else{
//                OffensiveDataExtractor::history_vel[i+11].push_back(Vector2D::INVALIDATED);
//                OffensiveDataExtractor::history_vel_count[i+11].push_back(-1);
//            }
//            OffensiveDataExtractor::history_body[i+11].push_back(p->body());
//            OffensiveDataExtractor::history_body_count[i+11].push_back(p->bodyCount());
//        }
//    }
//}


void OffensiveDataExtractor::update(const PlayerAgent *agent, const CooperativeAction &action,bool update_shoot) {
    if(!OffensiveDataExtractor::active)
        return;
    const WorldModel & wm = option.input_worldMode == FULLSTATE ? agent->fullstateWorld() : agent->world();
    if (last_update_cycle == wm.time().cycle())
        return;
    if (!wm.self().isKickable())
        return;
    if (wm.gameMode().type() != rcsc::GameMode::PlayOn)
        return;

    #ifdef ODEDebug
    dlog.addText(Logger::BLOCK, "start update");
    #endif
    DEState state = DEState(wm);
    if (state.kicker() == nullptr)
        return;

    if (!fout.is_open()) {
        init_file(state);
    }
    last_update_cycle = wm.time().cycle();
    features.clear();

    if (!update_shoot){
        if (
                action.category() > 2
                ||
                !action.targetPoint().isValid()
                ||
                action.targetPlayerUnum() > 11
                ||
                action.targetPlayerUnum() < 1
                )
            return;
    }

    // cycle
    if (option.cycle)
        ADD_ELEM("cycle", convertor_cycle(last_update_cycle));

    // ball
    extract_ball(state);

    // kicker
    extract_kicker(state);

    // players
    extract_players(state);

    // output
    if (!update_shoot){
        extract_output(state,
                       action.category(),
                       action.targetPoint(),
                       action.targetPlayerUnum(),
                       action.description(),
                       action.firstBallSpeed());
    }
    for (int i = 0; i < features.size(); i++){
        if ( i == features.size() - 1){
            fout<<features[i];
        }else{
            fout<<features[i]<<",";
        }
    }
    fout<<std::endl;
}

std::vector<double> OffensiveDataExtractor::get_data(DEState & state){
    features.clear();
    if (option.cycle)
        ADD_ELEM("cycle", convertor_cycle(state.cycle()));

    // ball
    extract_ball(state);

    // kicker
    extract_kicker(state);

    // players
    extract_players(state);
    return features;
}

OffensiveDataExtractor &OffensiveDataExtractor::i() {
    static OffensiveDataExtractor instance;
    return instance;
}


void OffensiveDataExtractor::extract_ball(DEState &state) {
    #ifdef ODEDebug
    dlog.addText(Logger::BLOCK, "start extract_ball");
    #endif
    if (option.ball_pos){
        if (state.ball().posValid()) {
            ADD_ELEM("p_x", convertor_x(state.ball().pos().x));
            ADD_ELEM("p_y", convertor_y(state.ball().pos().y));
            ADD_ELEM("p_r", convertor_dist(state.ball().pos().r()));
            ADD_ELEM("p_t", convertor_angle(state.ball().pos().th().degree()));
            #ifdef ODEDebug
            dlog.addText(Logger::BLOCK, "##add ball pos x y r t");
            #endif
        } else {
            ADD_ELEM("p_x", invalid_data_);
            ADD_ELEM("p_y", invalid_data_);
            ADD_ELEM("p_r", invalid_data_);
            ADD_ELEM("p_t", invalid_data_);
            #ifdef ODEDebug
            dlog.addText(Logger::BLOCK, "##@add ball invalid pos x y r t");
            #endif
        }
    }
    if (option.ball_kicker_pos){
        if (state.ball().rposValid()){
            ADD_ELEM("kicker_x", convertor_dist_x(state.ball().rpos().x));
            ADD_ELEM("kicker_y", convertor_dist_y(state.ball().rpos().y));
            ADD_ELEM("kicker_r", convertor_dist(state.ball().rpos().r()));
            ADD_ELEM("kicker_t", convertor_angle(state.ball().rpos().th().degree()));
            #ifdef ODEDebug
            dlog.addText(Logger::BLOCK, "##add ball rpos x y r t");
            #endif
        }else{
            ADD_ELEM("kicker_x", invalid_data_);
            ADD_ELEM("kicker_y", invalid_data_);
            ADD_ELEM("kicker_r", invalid_data_);
            ADD_ELEM("kicker_t", invalid_data_);
            #ifdef ODEDebug
            dlog.addText(Logger::BLOCK, "##@add ball invalid rpos x y r t");
            #endif
        }
    }
    if (option.ball_vel){
        if (state.ball().velValid()) {
            ADD_ELEM("v_x", convertor_bvx(state.ball().vel().x));
            ADD_ELEM("v_y", convertor_bvy(state.ball().vel().y));
            ADD_ELEM("v_r", convertor_bv(state.ball().vel().r()));
            ADD_ELEM("v_t", convertor_angle(state.ball().vel().th().degree()));
            #ifdef ODEDebug
            dlog.addText(Logger::BLOCK, "##add ball vel x y r t");
            #endif
        } else {
            ADD_ELEM("v_x", invalid_data_);
            ADD_ELEM("v_y", invalid_data_);
            ADD_ELEM("v_r", invalid_data_);
            ADD_ELEM("v_t", invalid_data_);
            #ifdef ODEDebug
            dlog.addText(Logger::BLOCK, "##@add ball invalid vel x y r t");
            #endif
        }
    }
    if (option.offside_count){
        ADD_ELEM("offside_count", convertor_counts(state.offsideLineCount()));
        #ifdef ODEDebug
        dlog.addText(Logger::BLOCK, "##add offside count");
        #endif
    }
}

void OffensiveDataExtractor::extract_kicker(DEState &state) {
    extract_drible_angles(state);
}

void OffensiveDataExtractor::extract_players(DEState &state) {
    auto players = sort_players3(state);
    #ifdef ODEDebug
    dlog.addText(Logger::BLOCK, "start extract_players");
    #endif
    for (uint i = 0; i < players.size(); i++) {
        #ifdef ODEDebug
        dlog.addText(Logger::BLOCK, "------------------------------");
        dlog.addText(Logger::BLOCK, "player %d in players list", i);
        #endif
        DEPlayer *player = players[i];
        if (player == nullptr) {
            add_null_player(invalid_data_,
                            (i <= 10 ? TM : OPP));
            #ifdef ODEDebug
            dlog.addText(Logger::BLOCK, "## add invalid data");
            #endif
            continue;
        }
        #ifdef ODEDebug
            dlog.addText(Logger::BLOCK, "## start extracting for p side%d unum%d", player->side(), player->unum());
        #endif
        ODEDataSide side = player->side() == state.ourSide() ? TM : OPP;
        extract_base_data(player, side, state);
        extract_counts(player, side, state);
        extract_pos(player, state, side);
        extract_vel(player, side, state);


        if (option.isKicker == side || option.isKicker == BOTH) {
            if (player->unum() == state.kicker()->unum()) {
                ADD_ELEM("is_kicker", 1);
            } else
                ADD_ELEM("is_kicker", 0);
        }
        if (option.isGhost == side || option.isGhost == BOTH) {
            if (player->isGhost()) {
                ADD_ELEM("is_ghost", 1);
            } else
                ADD_ELEM("is_ghost", 0);
        }

        extract_pass_angle(player, state, side);
        extract_goal_polar(player, side);
        extract_goal_open_angle(player, state, side);
//        extract_history(player, side);
    }
}


std::vector<DEPlayer *> OffensiveDataExtractor::sort_players(DEState &state) {
    static int cycle = 0;
    static std::vector<DEPlayer *> tms;
    if (state.wm().time().cycle() == cycle){
        return tms;
    }
    cycle = state.wm().time().cycle();
    tms.clear();
//    std::vector<const DEPlayer *> tms;
    std::vector<DEPlayer *> opps;
    tms.clear();
    opps.clear();

    int max_teammate_count = 10;
    for (DEPlayer* player: state.teammates()){
        if (!player->pos().isValid())
            continue;
        tms.push_back(player);
        if (tms.size() == max_teammate_count)
            break;
    }
    if (!option.kicker_first)
        tms.push_back(state.kicker());

    int max_opponent_count = 15;
    for (DEPlayer* player: state.opponents()){
        if (!player->pos().isValid())
            continue;
        opps.push_back(player);
        if (opps.size() == max_opponent_count)
            break;
    }

    auto unum_sort = [](DEPlayer *p1, DEPlayer *p2) -> bool {
        return p1->unum() < p2->unum();
    };
    auto x_sort = [](DEPlayer *p1, DEPlayer *p2) -> bool {
        return p1->pos().x > p2->pos().x;
    };

    if (option.playerSortMode == X) {
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
        for (; tms.size() < 10; tms.push_back(nullptr));
        tms.insert(tms.begin(), state.kicker());
    }else{
        for (; tms.size() < 11; tms.push_back(nullptr));
    }

    for (; opps.size() < 15; opps.push_back(nullptr));

    tms.insert(tms.end(), opps.begin(), opps.end());

    return tms;
}

std::vector<DEPlayer *> OffensiveDataExtractor::sort_players2(DEState &state) {
    static int cycle = 0;
    static std::vector<DEPlayer *> tms;
    if (state.wm().time().cycle() == cycle){
        return tms;
    }
    auto unum_sort = [](DEPlayer *p1, DEPlayer *p2) -> bool {
        return p1->unum() < p2->unum();
    };
    auto x_sort = [](DEPlayer *p1, DEPlayer *p2) -> bool {
        return p1->pos().x > p2->pos().x;
    };

    cycle = state.wm().time().cycle();
    tms.clear();
//    std::vector<const DEPlayer *> tms;
    std::vector<DEPlayer *> opps;
    tms.clear();
    opps.clear();

    for (int i = 1; i <= 11; i++){
        DEPlayer * player = state.ourPlayer(i);
        if (player == nullptr || player->unum() < 0 || !player->pos().isValid() || player->isGhost()){
            tms.push_back(nullptr);
            continue;
        }
        tms.push_back(player);
    }


    if (option.playerSortMode == X) {
        int max_opponent_count = 15;
        for (DEPlayer * player: state.opponents()) {
            if (!player->pos().isValid())
                continue;
            opps.push_back(player);
            if (opps.size() == max_opponent_count)
                break;
        }
        std::sort(opps.begin(), opps.end(), x_sort);
        for (; opps.size() < 15; opps.push_back(nullptr));
    } else if (option.playerSortMode == UNUM){
        for (int i = 1; i <= 11; i++){
            DEPlayer * player = state.theirPlayer(i);
            if (player == nullptr || player->unum() < 0 || !player->pos().isValid()){
                opps.push_back(nullptr);
                continue;
            }
            opps.push_back(player);
        }
        int max_opponent_count = 15;
        for (DEPlayer * player: state.opponents()) {
            if (!player->pos().isValid())
                continue;
            if (player->unum() > 0)
                continue;
            opps.push_back(player);
            if (opps.size() == max_opponent_count)
                break;
        }
        for (; opps.size() < 15; opps.push_back(nullptr));
    }

    tms.insert(tms.end(), opps.begin(), opps.end());

    return tms;
}


std::vector<DEPlayer *> OffensiveDataExtractor::sort_players3(DEState &state) {
    static int cycle = 0;
    static std::vector<DEPlayer *> tms;
    if (state.wm().time().cycle() == cycle){
        return tms;
    }
    #ifdef ODEDebug
    dlog.addText(Logger::BLOCK, "start sorter player 3");
    #endif
    auto unum_sort = [](DEPlayer *p1, DEPlayer *p2) -> bool {
        return p1->unum() < p2->unum();
    };
    auto x_sort = [](DEPlayer *p1, DEPlayer *p2) -> bool {
        return p1->pos().x > p2->pos().x;
    };

    cycle = state.wm().time().cycle();
    tms.clear();
//    std::vector<const DEPlayer *> tms;
    std::vector<DEPlayer *> opps;
    tms.clear();
    opps.clear();

    for (int i = 1; i <= 11; i++){
        DEPlayer * player = state.ourPlayer(i);
        if (player == nullptr || player->unum() < 0 || !player->pos().isValid()){
            tms.push_back(nullptr);
            #ifdef ODEDebug
            dlog.addText(Logger::BLOCK, "##add nullptr for tm %d", i);
            #endif
            continue;
        }
        #ifdef ODEDebug
        dlog.addText(Logger::BLOCK, "## add player for tm %d", i);
        #endif
        tms.push_back(player);
    }


    if (option.playerSortMode == X) {
        int max_opponent_count = 15;
        for (DEPlayer *player: state.theirPlayers()) {
            if (!player->pos().isValid()){
                #ifdef ODEDebug
                dlog.addText(Logger::BLOCK, "## opp sorter is x, player pos is not valid");
                #endif
                continue;
            }
            opps.push_back(player);
            #ifdef ODEDebug
            dlog.addText(Logger::BLOCK, "## opp sorter is x, player pos is valid");
            #endif
            if (opps.size() == max_opponent_count){
                #ifdef ODEDebug
                dlog.addText(Logger::BLOCK, "## opp sorter is x, reach to 15");
                #endif
                break;
            }
        }
        std::sort(opps.begin(), opps.end(), x_sort);
        for (; opps.size() < 15; opps.push_back(nullptr));
    } else if (option.playerSortMode == UNUM){
        for (int i = 1; i <= 11; i++){
            DEPlayer * player = state.theirPlayer(i);
            if (player == nullptr || player->unum() < 0 || !player->pos().isValid() || player->isGhost()){
                opps.push_back(nullptr);
                #ifdef ODEDebug
                dlog.addText(Logger::BLOCK, "## opp sorter is unum, add nullptr for opp %d", i);
                #endif
                continue;
            }
            #ifdef ODEDebug
            dlog.addText(Logger::BLOCK, "#opp sorter is unum, add player for opp %d", i);
            #endif
            opps.push_back(player);
        }
        int max_opponent_count = 15;
        for (DEPlayer *player: state.theirPlayers()) {
            if (!player->pos().isValid())
                continue;
            if (player->unum() > 0)
                continue;
            #ifdef ODEDebug
            dlog.addText(Logger::BLOCK, "## opp sorter is unum, add un opp player");
            #endif
            opps.push_back(player);
            if (opps.size() == max_opponent_count)
                break;
        }
        for (; opps.size() < 15; opps.push_back(nullptr));
    }

    tms.insert(tms.end(), opps.begin(), opps.end());

    return tms;
}


void OffensiveDataExtractor::add_null_player(int unum, ODEDataSide side) {
    if (option.side == side || option.side == BOTH)
        ADD_ELEM("side", invalid_data_);
    if (option.unum == side || option.unum == BOTH)
        ADD_ELEM("unum", unum);
    if (option.type == side || option.type == BOTH) {
        ADD_ELEM("player_type_dash_rate", invalid_data_);
        ADD_ELEM("player_type_effort_max", invalid_data_);
        ADD_ELEM("player_type_effort_min", invalid_data_);
        ADD_ELEM("player_type_kickable", invalid_data_);
        ADD_ELEM("player_type_margin", invalid_data_);
        ADD_ELEM("player_type_kick_power", invalid_data_);
        ADD_ELEM("player_type_decay", invalid_data_);
        ADD_ELEM("player_type_size", invalid_data_);
        ADD_ELEM("player_type_speed_max", invalid_data_);
    }
    if (option.body == side || option.body == BOTH)
        ADD_ELEM("body", invalid_data_);
    if (option.face == side || option.face == BOTH)
        ADD_ELEM("face", invalid_data_);
    if (option.tackling == side || option.tackling == BOTH)
        ADD_ELEM("tackling", invalid_data_);
    if (option.kicking == side || option.kicking == BOTH)
        ADD_ELEM("kicking", invalid_data_);
    if (option.card == side || option.card == BOTH)
        ADD_ELEM("card", invalid_data_);
    if (option.pos_counts == side || option.pos_counts == BOTH)
        ADD_ELEM("pos_count", invalid_data_);
    if (option.vel_counts == side || option.vel_counts == BOTH)
        ADD_ELEM("vel_count", invalid_data_);
    if (option.body_counts == side || option.body_counts == BOTH)
        ADD_ELEM("body_count", invalid_data_);
    if (option.pos == side || option.pos == BOTH) {
        ADD_ELEM("pos_x", invalid_data_);
        ADD_ELEM("pos_y", invalid_data_);
    }
    if (option.polarPos == side || option.polarPos == BOTH) {
        ADD_ELEM("pos_r", invalid_data_);
        ADD_ELEM("pos_t", invalid_data_);
    }
    if (option.relativePos == side || option.relativePos == BOTH) {
        ADD_ELEM("kicker_x", invalid_data_);
        ADD_ELEM("kicker_y", invalid_data_);
        ADD_ELEM("kicker_r", invalid_data_);
        ADD_ELEM("kicker_t", invalid_data_);
    }
    if (option.in_offside == side || option.in_offside == BOTH) {
        ADD_ELEM("in_offside", invalid_data_);
    }
    if (option.vel == side || option.vel == BOTH) {
        ADD_ELEM("v_x", invalid_data_);
        ADD_ELEM("v_y", invalid_data_);
    }
    if (option.polarVel == side || option.polarVel == BOTH) {
        ADD_ELEM("v_r", invalid_data_);
        ADD_ELEM("v_t", invalid_data_);
    }
    if (option.isKicker == side || option.isKicker == BOTH)
        ADD_ELEM("is_kicker", invalid_data_);
    if (option.isGhost == side || option.isGhost == BOTH) {
        ADD_ELEM("is_ghost", invalid_data_);
    }
    if (option.openAnglePass == side || option.openAnglePass == BOTH) {
        ADD_ELEM("pass_dist", invalid_data_);
        ADD_ELEM("pass_opp1_dist", invalid_data_);
        ADD_ELEM("pass_opp1_angle", invalid_data_);
        ADD_ELEM("pass_opp1_dist_line", invalid_data_);
        ADD_ELEM("pass_opp1_dist_proj", invalid_data_);
        ADD_ELEM("pass_opp1_dist_diffbody", invalid_data_);
        ADD_ELEM("pass_opp2_dist", invalid_data_);
        ADD_ELEM("pass_opp2_angle", invalid_data_);
        ADD_ELEM("pass_opp2_dist_line", invalid_data_);
        ADD_ELEM("pass_opp2_dist_proj", invalid_data_);
        ADD_ELEM("pass_opp2_dist_diffbody", invalid_data_);
        ADD_ELEM("pass_opp2_dist", invalid_data_);
        ADD_ELEM("pass_opp2_angle", invalid_data_);
        ADD_ELEM("pass_opp2_dist_line", invalid_data_);
        ADD_ELEM("pass_opp2_dist_proj", invalid_data_);
        ADD_ELEM("pass_opp2_dist_diffbody", invalid_data_);
    }
    if (option.nearestOppDist == side || option.nearestOppDist == BOTH){
        ADD_ELEM("opp1_dist", invalid_data_);
        ADD_ELEM("opp1_angle", invalid_data_);
        ADD_ELEM("opp1_diffbody", invalid_data_);
        ADD_ELEM("opp2_dist", invalid_data_);
        ADD_ELEM("opp2_angle", invalid_data_);
        ADD_ELEM("opp2_diffbody", invalid_data_);
    }

    if (option.polarGoalCenter == side || option.polarGoalCenter == BOTH) {
        ADD_ELEM("angle_goal_center_r", invalid_data_);
        ADD_ELEM("angle_goal_center_t", invalid_data_);
    }
    if (option.openAngleGoal == side || option.openAngleGoal == BOTH)
        ADD_ELEM("open_goal_angle", invalid_data_);
    for(int i = 0; i < option.history_size; i++) {
        if (option.body == side || option.body == BOTH){
            ADD_ELEM('history_body', invalid_data_);
        }
        if (option.pos == side || option.pos == BOTH) {
            ADD_ELEM('history_pos_x', invalid_data_);
            ADD_ELEM('history_pos_y', invalid_data_);
        }
        if (option.polarPos == side || option.polarPos == BOTH) {
            ADD_ELEM('history_pos_r', invalid_data_);
            ADD_ELEM('history_pos_t', invalid_data_);
        }


        if (option.vel == side || option.vel == BOTH) {
            ADD_ELEM('history_vel_x', invalid_data_);
            ADD_ELEM('history_vel_y', invalid_data_);
        }
        if (option.polarVel == side || option.polarVel == BOTH) {
            ADD_ELEM('history_vel_r', invalid_data_);
            ADD_ELEM('history_vel_t', invalid_data_);
        }
        if (option.pos_counts == side || option.pos_counts == BOTH)
            ADD_ELEM('history_pos_count', invalid_data_);
        if (option.vel_counts == side || option.vel_counts == BOTH)
            ADD_ELEM('history_vel_count', invalid_data_);
        if (option.body_counts == side || option.body_counts == BOTH)
            ADD_ELEM('history_body_count', invalid_data_);
    }
}

void OffensiveDataExtractor::extract_output(DEState &state,
                                   int category,
                                   const rcsc::Vector2D &target,
                                   const int &unum,
                                   const char *desc,
                                   double ball_speed) {
    ADD_ELEM("category", category);
    ADD_ELEM("target_x", convertor_x(target.x));
    ADD_ELEM("target_y", convertor_y(target.y));
    ADD_ELEM("unum", unum);
    ADD_ELEM("unum_index", find_unum_index(state, unum));
    ADD_ELEM("ball_speed", convertor_bv(ball_speed));
    ADD_ELEM("ball_dir", convertor_angle((target - state.ball().pos()).th().degree()));
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
void OffensiveDataExtractor::update_for_shoot(const rcsc::PlayerAgent *agent, rcsc::Vector2D target, double ballsp){
//    update(agent, NULL, true);
//    DEState &state = option.input_worldMode == FULLSTATE ? agent->fullstateWorld() : agent->world();
//    ADD_ELEM("category", 3);
//    ADD_ELEM("target_x", convertor_x(target.x));
//    ADD_ELEM("target_y", convertor_y(target.y));
//    ADD_ELEM("unum", state.self().unum());
//    ADD_ELEM("unum_index", find_unum_index(state, state.kicker()->unum()));
//    ADD_ELEM("ball_speed", convertor_bv(ballsp));
//    ADD_ELEM("ball_dir", convertor_angle((target - state.ball().pos()).th().degree()));
//    ADD_ELEM("desc", 4);
//    fout << std::endl;
}

void OffensiveDataExtractor::extract_pass_angle(DEPlayer *player, DEState &state, ODEDataSide side) {
    Vector2D ball_pos = state.ball().pos();
    Vector2D tm_pos = player->pos();
    int max_pos_count = 30;
    if (player->unum() == state.kicker()->unum() && player->side() == state.ourSide()){
        max_pos_count = 20;
    }
    #ifdef ODEDebug
    dlog.addText(Logger::BLOCK, "##start extract pass angle");
    #endif
    if (!ball_pos.isValid() || !tm_pos.isValid() || !state.ball().posValid() || player->posCount() > max_pos_count){
        if (option.openAnglePass == side || option.openAnglePass == BOTH) {
            #ifdef ODEDebug
            dlog.addText(Logger::BLOCK, "#### add invalid data for open angle pass");
            #endif
            ADD_ELEM("pass_dist", invalid_data_);
            ADD_ELEM("pass_opp1_dist", invalid_data_);
            ADD_ELEM("pass_opp1_angle", invalid_data_);
            ADD_ELEM("pass_opp1_dist_line", invalid_data_);
            ADD_ELEM("pass_opp1_dist_proj", invalid_data_);
            ADD_ELEM("pass_opp1_dist_diffbody", invalid_data_);
            ADD_ELEM("pass_opp2_dist", invalid_data_);
            ADD_ELEM("pass_opp2_angle", invalid_data_);
            ADD_ELEM("pass_opp2_dist_line", invalid_data_);
            ADD_ELEM("pass_opp2_dist_proj", invalid_data_);
            ADD_ELEM("pass_opp2_dist_diffbody", invalid_data_);
            ADD_ELEM("pass_opp3_dist", invalid_data_);
            ADD_ELEM("pass_opp3_angle", invalid_data_);
            ADD_ELEM("pass_opp3_dist_line", invalid_data_);
            ADD_ELEM("pass_opp3_dist_proj", invalid_data_);
            ADD_ELEM("pass_opp3_dist_diffbody", invalid_data_);
        }
        if (option.nearestOppDist == side || option.nearestOppDist == BOTH){
            #ifdef ODEDebug
            dlog.addText(Logger::BLOCK, "#### add invalid for nearest opp dist");
            #endif
            ADD_ELEM("opp1_dist", invalid_data_);
            ADD_ELEM("opp1_angle", invalid_data_);
            ADD_ELEM("opp1_diffbody", invalid_data_);
            ADD_ELEM("opp2_dist", invalid_data_);
            ADD_ELEM("opp2_angle", invalid_data_);
            ADD_ELEM("opp2_diffbody", invalid_data_);
        }
        return;
    }
    std::vector<std::pair<double, double>> opp_dist_angle;
    std::vector<std::pair<double, double>> opp_dist_body_diff;
    std::vector<ODEOpenAngle> candidates;
    for (const auto& opp: state.opponents()) {
        ODEOpenAngle candid;
        #ifdef ODEDebug
        dlog.addText(Logger::BLOCK, "######want to check opp %d", opp->unum());
        #endif
        if (!opp->pos().isValid()) continue;
        candid.dist_self_to_opp = opp->pos().dist(ball_pos);
        opp_dist_angle.push_back(std::make_pair(opp->pos().dist(ball_pos), (opp->pos() - ball_pos).th().degree()));
        if (opp->bodyValid())
            opp_dist_body_diff.push_back(std::make_pair(opp->pos().dist(ball_pos), ((ball_pos - opp->pos()).th() - opp->body()).abs()));
        else
            opp_dist_body_diff.push_back(std::make_pair(opp->pos().dist(ball_pos), invalid_data_));
        AngleDeg diff = (tm_pos - ball_pos).th() - (opp->pos() - ball_pos).th();
        #ifdef ODEDebug
        dlog.addText(Logger::BLOCK, "######check opp %d in %.1f,%.1f, diff:%.1f", opp->unum(), opp->pos().x, opp->pos().y, diff.degree());
        #endif
        if (diff.abs() > 60)
            continue;
        if (opp->pos().dist(ball_pos) > tm_pos.dist(ball_pos) + 10.0)
            continue;
        candid.open_angle = diff.abs();
        Vector2D proj_pos = Line2D(ball_pos, tm_pos).projection(opp->pos());
        candid.dist_opp_proj = proj_pos.dist(opp->pos());
        candid.dist_self_to_opp_proj = proj_pos.dist(state.kicker()->pos());
        if (opp->bodyValid())
            candid.opp_body_diff = ((proj_pos - opp->pos()).th() - opp->body()).abs();
        else
            candid.opp_body_diff = invalid_data_;
        candidates.push_back(candid);

    }
    if (option.openAnglePass == side || option.openAnglePass == BOTH) {
        auto open_angle_sorter = [](ODEOpenAngle &p1, ODEOpenAngle &p2) -> bool {
            return p1.open_angle < p2.open_angle;
        };
        std::sort(candidates.begin(), candidates.end(), open_angle_sorter);

        ADD_ELEM("pass_dist", convertor_dist(ball_pos.dist(tm_pos)));
        if (candidates.size() >= 1){
            #ifdef ODEDebug
            dlog.addText(Logger::BLOCK, "###### add opp angle pass first");
            #endif
            ADD_ELEM("pass_opp1_dist", convertor_dist(candidates[0].dist_self_to_opp));
            ADD_ELEM("pass_opp1_dist_proj_to_opp", convertor_dist(candidates[0].dist_opp_proj));
            ADD_ELEM("pass_opp1_dist_proj_to_kicker", convertor_dist(candidates[0].dist_self_to_opp_proj));
            ADD_ELEM("pass_opp1_open_angle", convertor_angle(candidates[0].open_angle));
            if (candidates[0].opp_body_diff != invalid_data_)
                ADD_ELEM("pass_opp1_dist_diffbody", convertor_angle(candidates[0].opp_body_diff));
            else
                ADD_ELEM("pass_opp1_dist_diffbody", invalid_data_);
        }
        else{
            #ifdef ODEDebug
            dlog.addText(Logger::BLOCK, "###### add opp angle pass first invalid");
            #endif
            ADD_ELEM("pass_opp1_dist", invalid_data_);
            ADD_ELEM("pass_opp1_angle", invalid_data_);
            ADD_ELEM("pass_opp1_dist_line", invalid_data_);
            ADD_ELEM("pass_opp1_dist_proj", invalid_data_);
            ADD_ELEM("pass_opp1_dist_diffbody", invalid_data_);
        }
        if (candidates.size() >= 2){
            #ifdef ODEDebug
            dlog.addText(Logger::BLOCK, "###### add opp angle pass second");
            #endif
            ADD_ELEM("pass_opp2_dist", convertor_dist(candidates[1].dist_self_to_opp));
            ADD_ELEM("pass_opp2_dist_proj_to_opp", convertor_dist(candidates[1].dist_opp_proj));
            ADD_ELEM("pass_opp2_dist_proj_to_kicker", convertor_dist(candidates[1].dist_self_to_opp_proj));
            ADD_ELEM("pass_opp2_open_angle", convertor_angle(candidates[1].open_angle));
            if (candidates[1].opp_body_diff != invalid_data_)
                ADD_ELEM("pass_opp2_dist_diffbody", convertor_angle(candidates[1].opp_body_diff));
            else
                ADD_ELEM("pass_opp2_dist_diffbody", invalid_data_);
        }
        else{
            #ifdef ODEDebug
            dlog.addText(Logger::BLOCK, "###### add opp angle pass second invalid");
            #endif
            ADD_ELEM("pass_opp2_dist", invalid_data_);
            ADD_ELEM("pass_opp2_angle", invalid_data_);
            ADD_ELEM("pass_opp2_dist_line", invalid_data_);
            ADD_ELEM("pass_opp2_dist_proj", invalid_data_);
            ADD_ELEM("pass_opp2_dist_diffbody", invalid_data_);
        }
        if (candidates.size() >= 3){
            #ifdef ODEDebug
            dlog.addText(Logger::BLOCK, "###### add opp angle pass second");
            #endif
            ADD_ELEM("pass_opp3_dist", convertor_dist(candidates[2].dist_self_to_opp));
            ADD_ELEM("pass_opp3_dist_proj_to_opp", convertor_dist(candidates[2].dist_opp_proj));
            ADD_ELEM("pass_opp3_dist_proj_to_kicker", convertor_dist(candidates[2].dist_self_to_opp_proj));
            ADD_ELEM("pass_opp3_open_angle", convertor_angle(candidates[2].open_angle));
            if (candidates[2].opp_body_diff != invalid_data_)
                ADD_ELEM("pass_opp3_dist_diffbody", convertor_angle(candidates[2].opp_body_diff));
            else
                ADD_ELEM("pass_opp3_dist_diffbody", invalid_data_);
        }
        else{
            #ifdef ODEDebug
            dlog.addText(Logger::BLOCK, "###### add opp angle pass second invalid");
            #endif
            ADD_ELEM("pass_opp3_dist", invalid_data_);
            ADD_ELEM("pass_opp3_angle", invalid_data_);
            ADD_ELEM("pass_opp3_dist_line", invalid_data_);
            ADD_ELEM("pass_opp3_dist_proj", invalid_data_);
            ADD_ELEM("pass_opp3_dist_diffbody", invalid_data_);
        }
    }
    if (option.nearestOppDist == side || option.nearestOppDist == BOTH){
        std::sort(opp_dist_angle.begin(), opp_dist_angle.end());
        std::sort(opp_dist_body_diff.begin(), opp_dist_body_diff.end());
        if (opp_dist_angle.size() >= 1){
            #ifdef ODEDebug
            dlog.addText(Logger::BLOCK, "###### add opp pass dist first");
            #endif
            ADD_ELEM("opp1_dist", convertor_dist(opp_dist_angle[0].first));
            ADD_ELEM("opp1_angle", convertor_angle(opp_dist_angle[0].second));
            if (opp_dist_body_diff[0].second != invalid_data_)
                ADD_ELEM("opp1_diffbody", convertor_angle(opp_dist_body_diff[0].second));
            else
                ADD_ELEM("opp1_diffbody", invalid_data_);
        }
        else{
            #ifdef ODEDebug
            dlog.addText(Logger::BLOCK, "###### add opp pass dist first invalid");
            #endif
            ADD_ELEM("opp1_dist", invalid_data_);
            ADD_ELEM("opp1_angle", invalid_data_);
            ADD_ELEM("opp1_diffbody", invalid_data_);
        }
        if (opp_dist_angle.size() >= 2){
            #ifdef ODEDebug
            dlog.addText(Logger::BLOCK, "###### add opp pass dist second");
            #endif
            ADD_ELEM("opp2_dist", convertor_dist(opp_dist_angle[1].first));
            ADD_ELEM("opp2_angle", convertor_angle(opp_dist_angle[1].second));
            if (opp_dist_body_diff[1].second != invalid_data_)
                ADD_ELEM("opp2_diffbody", convertor_angle(opp_dist_body_diff[1].second));
            else
                ADD_ELEM("opp2_diffbody", invalid_data_);
        }
        else{
            #ifdef ODEDebug
            dlog.addText(Logger::BLOCK, "###### add opp pass dist second invalid");
            #endif
            ADD_ELEM("opp2_dist", invalid_data_);
            ADD_ELEM("opp2_angle", invalid_data_);
            ADD_ELEM("opp2_diffbody", invalid_data_);
        }
    }
}

void OffensiveDataExtractor::extract_vel(DEPlayer *player, ODEDataSide side, DEState &state) {
    int max_vel_count = 5;
    if (player->unum() == state.kicker()->unum() && player->side() == state.ourSide())
        max_vel_count = 10;
    if (option.vel == side || option.vel == BOTH) {
        if (player->vel().isValid() && player->velCount() <= max_vel_count){
            ADD_ELEM("v_x", convertor_pvx(player->vel().x));
            ADD_ELEM("v_y", convertor_pvy(player->vel().y));
        }
        else {
            ADD_ELEM("v_x", invalid_data_);
            ADD_ELEM("v_y", invalid_data_);
        }
    }
    if (option.polarVel == side || option.polarVel == BOTH) {
        if (player->vel().isValid() && player->velCount() <= max_vel_count){
            ADD_ELEM("v_r", convertor_pv(player->vel().r()));
            ADD_ELEM("v_t", convertor_angle(player->vel().th().degree()));
        }
        else {
            ADD_ELEM("v_r", invalid_data_);
            ADD_ELEM("v_t", invalid_data_);
        }
    }
}

void OffensiveDataExtractor::extract_pos(DEPlayer *player, DEState &state, ODEDataSide side) {
    int max_pos_count = 30;
    if (player->unum() == state.kicker()->unum() && player->side() == state.ourSide())
        max_pos_count = 20;

    if (player->posCount() <= max_pos_count and player->pos().isValid()){
        if (option.pos == side || option.pos == BOTH) {
            ADD_ELEM("pos_x", convertor_x(player->pos().x));
            ADD_ELEM("pos_y", convertor_y(player->pos().y));
        }
        if (option.polarPos == side || option.polarPos == BOTH) {
            ADD_ELEM("pos_r", convertor_dist(player->pos().r()));
            ADD_ELEM("pos_t", convertor_angle(player->pos().th().degree()));
        }
        Vector2D rpos = player->pos() - state.kicker()->pos();
        if (option.relativePos == side || option.relativePos == BOTH) {
            ADD_ELEM("kicker_x", convertor_dist_x(rpos.x));
            ADD_ELEM("kicker_y", convertor_dist_y(rpos.y));
            ADD_ELEM("kicker_r", convertor_dist(rpos.r()));
            ADD_ELEM("kicker_t", convertor_angle(rpos.th().degree()));
        }
        if (option.in_offside == side || option.in_offside == BOTH) {
            if (player->pos().x > state.offsideLineX()) {
                ADD_ELEM("pos_offside", 1);
            } else {
                ADD_ELEM("pos_offside", 0);
            }
        }
    }else{
        if (option.pos == side || option.pos == BOTH) {
            ADD_ELEM("pos_x", invalid_data_);
            ADD_ELEM("pos_y", invalid_data_);
        }
        if (option.polarPos == side || option.polarPos == BOTH) {
            ADD_ELEM("pos_r", invalid_data_);
            ADD_ELEM("pos_t", invalid_data_);
        }
        if (option.relativePos == side || option.relativePos == BOTH) {
            ADD_ELEM("kicker_x", invalid_data_);
            ADD_ELEM("kicker_y", invalid_data_);
            ADD_ELEM("kicker_r", invalid_data_);
            ADD_ELEM("kicker_t", invalid_data_);
        }
        if (option.in_offside == side || option.in_offside == BOTH) {
            ADD_ELEM("pos_offside", invalid_data_);
        }
    }
}

void OffensiveDataExtractor::extract_goal_polar(DEPlayer *player, ODEDataSide side) {
    if (!(option.polarGoalCenter == side || option.polarGoalCenter == BOTH))
        return;
    if (!player->pos().isValid()){
        ADD_ELEM("angle_goal_center_r", invalid_data_);
        ADD_ELEM("angle_goal_center_t", invalid_data_);
        return;
    }
    Vector2D goal_center = Vector2D(52, 0);
    ADD_ELEM("angle_goal_center_r", convertor_dist((goal_center - player->pos()).r()));
    ADD_ELEM("angle_goal_center_t", convertor_angle((goal_center - player->pos()).th().degree()));
}

void OffensiveDataExtractor::extract_goal_open_angle(DEPlayer *player,
                                            DEState &state,
                                            ODEDataSide side) {
    if (!(option.openAngleGoal == side || option.openAngleGoal == BOTH))
        return;
    if (!player->pos().isValid()){
        ADD_ELEM("goal_open_angle", invalid_data_);
        return;
    }
    Vector2D goal_t = Vector2D(52, -7);
    Vector2D goal_b = Vector2D(52, 7);
    Triangle2D player_goal_area = Triangle2D(goal_b, goal_t, player->pos());

    std::vector<Vector2D> players_in_area;

    for (const auto& opp: state.theirPlayers()){
        if (!opp->pos().isValid())
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

void OffensiveDataExtractor::extract_base_data(DEPlayer *player, ODEDataSide side, DEState &state) {
    if (option.side == side || option.side == BOTH){
        #ifdef ODEDebug
        dlog.addText(Logger::BLOCK, "#### add side %d", player->side());
        #endif
        ADD_ELEM("side", player->side());
    }
    if (option.unum == side || option.unum == BOTH){
        if (player->unum() == -1){
            ADD_ELEM("unum", invalid_data_);
            #ifdef ODEDebug
            dlog.addText(Logger::BLOCK, "#### add invalid unum");
            #endif
        }else{
            ADD_ELEM("unum", convertor_unum(player->unum()));
            #ifdef ODEDebug
            dlog.addText(Logger::BLOCK, "#### add unum %d", player->unum());
            #endif
        }
    }
    if (option.type == side || option.type == BOTH){
        #ifdef ODEDebug
                dlog.addText(Logger::BLOCK, "#### adding type");
        #endif
        extract_type(player, side);
    }
    int max_face_count = 5;
    if (player->unum() == state.kicker()->unum() && player->side() == state.ourSide())
        max_face_count = 2;
    if (option.body == side || option.body == BOTH) {
        if (player->bodyCount() <= max_face_count) {
            ADD_ELEM("body", convertor_angle(player->body().degree()));
            #ifdef ODEDebug
            dlog.addText(Logger::BLOCK, "#### add body");
            #endif
        } else {
            ADD_ELEM("body", invalid_data_);
            #ifdef ODEDebug
            dlog.addText(Logger::BLOCK, "#### add invalid body");
            #endif
        }
    }
    if (option.face == side || option.face == BOTH){
        if (player->faceCount() <= max_face_count){
            ADD_ELEM("face", convertor_angle(player->face().degree()));
            #ifdef ODEDebug
            dlog.addText(Logger::BLOCK, "#### add face");
            #endif
        }else{
            ADD_ELEM("face", invalid_data_);
            #ifdef ODEDebug
            dlog.addText(Logger::BLOCK, "#### add invalid face");
            #endif
        }
    }
    if (option.tackling == side || option.tackling == BOTH){
        #ifdef ODEDebug
            dlog.addText(Logger::BLOCK, "#### add is tackling");
        #endif
        ADD_ELEM("tackling", player->isTackling());
    }
    if (option.kicking == side || option.kicking == BOTH){
        #ifdef ODEDebug
        dlog.addText(Logger::BLOCK, "#### add kicked");
        #endif
        ADD_ELEM("kicking", player->kicked());
    }
    if (option.card == side || option.card == BOTH){
        #ifdef ODEDebug
        dlog.addText(Logger::BLOCK, "#### add card");
        #endif
        ADD_ELEM("card", convertor_card(player->player()->card()));
    }
}

void OffensiveDataExtractor::extract_type(DEPlayer *player, ODEDataSide side) {
    if (player->unum() < 0 || player->playerTypePtr() == nullptr){
        ADD_ELEM("player_type_dash_rate", invalid_data_);
        ADD_ELEM("player_type_effort_max", invalid_data_);
        ADD_ELEM("player_type_effort_min", invalid_data_);
        ADD_ELEM("player_type_kickable", invalid_data_);
        ADD_ELEM("player_type_margin", invalid_data_);
        ADD_ELEM("player_type_kick_power", invalid_data_);
        ADD_ELEM("player_type_decay", invalid_data_);
        ADD_ELEM("player_type_size", invalid_data_);
        ADD_ELEM("player_type_speed_max", invalid_data_);
    }
    else{
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
}

void OffensiveDataExtractor::extract_counts(DEPlayer *player, ODEDataSide side, DEState &state) {
    int max_pos_count = 30;
    int max_vel_count = 5;
    int max_face_count = 2;
    if (player->unum() == state.kicker()->unum() && player->side() == state.ourSide()){
        max_pos_count = 20;
        max_vel_count = 10;
        max_face_count = 5;
    }
    if (option.pos_counts == side || option.pos_counts == BOTH){
        if (player->posCount() <= max_pos_count){
            ADD_ELEM("pos_count", convertor_counts(player->posCount()));
        }else{
            ADD_ELEM("pos_count", invalid_data_);
        }
    }
    if (option.vel_counts == side || option.vel_counts == BOTH){
        if (player->velCount() <= max_vel_count){
            ADD_ELEM("vel_count", convertor_counts(player->velCount()));
        }else{
            ADD_ELEM("vel_count", invalid_data_);
        }
    }
    if (option.body_counts == side || option.body_counts == BOTH){
        if (player->bodyCount() <= max_face_count){
            ADD_ELEM("body_count", convertor_counts(player->bodyCount()));
        }else{
            ADD_ELEM("body_count", invalid_data_);
        }
    }
}

void OffensiveDataExtractor::extract_history(DEPlayer *player, ODEDataSide side) {
    int len = OffensiveDataExtractor::history_pos_count[0].size();
    int unum = player->unum();
    if (side == OPP)
        unum += 11;
    for(int i = 0; i < option.history_size; i++) {
        if (option.body == side || option.body == BOTH){
            if (OffensiveDataExtractor::history_body_count[unum][len - 2 - i] == -1) {
                ADD_ELEM('history_body', invalid_data_);
            }else{
                ADD_ELEM('history_body', convertor_angle(OffensiveDataExtractor::history_body[unum][len - 2 - i].degree()));
            }
        }
        if (option.pos == side || option.pos == BOTH) {
            if (OffensiveDataExtractor::history_pos_count[unum][len - 2 - i] == -1) {
                ADD_ELEM('history_pos_x', invalid_data_);
                ADD_ELEM('history_pos_y', invalid_data_);
            }else{
                ADD_ELEM('history_pos_x', convertor_x(OffensiveDataExtractor::history_pos[unum][len - 2 - i].x));
                ADD_ELEM('history_pos_y', convertor_y(OffensiveDataExtractor::history_pos[unum][len - 2 - i].y));
            }
        }
        if (option.polarPos == side || option.polarPos == BOTH) {
            if (OffensiveDataExtractor::history_pos_count[unum][len - 2 - i] == -1) {
                ADD_ELEM('history_pos_r', invalid_data_);
                ADD_ELEM('history_pos_t', invalid_data_);
            }else{
                ADD_ELEM('history_pos_r', convertor_dist(OffensiveDataExtractor::history_pos[unum][len - 2 - i].r()));
                ADD_ELEM('history_pos_t', convertor_angle(OffensiveDataExtractor::history_pos[unum][len - 2 - i].th().degree()));
            }
        }
        if (option.vel == side || option.vel == BOTH) {
            if (OffensiveDataExtractor::history_vel_count[unum][len - 2 - i] == -1) {
                ADD_ELEM('history_vel_x', invalid_data_);
                ADD_ELEM('history_vel_y', invalid_data_);
            }else{
                ADD_ELEM('history_vel_x', convertor_pvx(OffensiveDataExtractor::history_vel[unum][len - 2 - i].x));
                ADD_ELEM('history_vel_y', convertor_pvy(OffensiveDataExtractor::history_vel[unum][len - 2 - i].y));
            }
        }
        if (option.polarVel == side || option.polarVel == BOTH) {
            if (OffensiveDataExtractor::history_vel_count[unum][len - 2 - i] == -1) {
                ADD_ELEM('history_vel_r', invalid_data_);
                ADD_ELEM('history_vel_t', invalid_data_);
            }else{
                ADD_ELEM('history_vel_r', convertor_pv(OffensiveDataExtractor::history_vel[unum][len - 2 - i].r()));
                ADD_ELEM('history_vel_t', convertor_angle(OffensiveDataExtractor::history_vel[unum][len - 2 - i].th().degree()));
            }
        }
        if (option.pos_counts == side || option.pos_counts == BOTH)
            ADD_ELEM('history_pos_count', convertor_counts(OffensiveDataExtractor::history_pos_count[unum][len - 2 - i]));
        if (option.vel_counts == side || option.vel_counts == BOTH)
            ADD_ELEM('history_vel_count', convertor_counts(OffensiveDataExtractor::history_pos_count[unum][len - 2 - i]));
        if (option.body_counts == side || option.body_counts == BOTH)
            ADD_ELEM('history_body_count', convertor_counts(OffensiveDataExtractor::history_pos_count[unum][len - 2 - i]));
    }
}

void OffensiveDataExtractor::extract_drible_angles(DEState &state) {

//    const PlayerObject *kicker = state.interceptTable()->fastestTeammate(); // TODO What is error ?!?!
    if (option.dribleAngle != Kicker)
        return;
    DEPlayer *kicker = state.ourPlayer(state.kickerUnum());
    if (kicker == nullptr || kicker->unum() < 0) {
        for (int i = 1; i <= option.nDribleAngle; i++)
            ADD_ELEM("dribble_angle", -2);
        return;
    }

    double delta = 360.0 / option.nDribleAngle;
    for (double angle = -180; angle < 180; angle += delta) {

        double min_opp_dist = 30;
        for (auto& opp: state.opponents()){
            if (opp->isGhost() || !opp->pos().isValid())
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

double OffensiveDataExtractor::convertor_x(double x) {
    if (!option.use_convertor)
        return x;
//    return x / 52.5;
    return std::min(std::max((x + 52.5) / 105.0, 0.0), 1.0);
}

double OffensiveDataExtractor::convertor_y(double y) {
    if (!option.use_convertor)
        return y;
//    return y / 34.0;
    return std::min(std::max((y + 34) / 68.0, 0.0), 1.0);
}

double OffensiveDataExtractor::convertor_dist(double dist) {
    if (!option.use_convertor)
        return dist;
//    return dist / 63.0 - 1.0;
    return dist / 123.0;
}

double OffensiveDataExtractor::convertor_dist_x(double dist) {
    if (!option.use_convertor)
        return dist;
//    return dist / 63.0 - 1.0;
    return dist / 105.0;
}

double OffensiveDataExtractor::convertor_dist_y(double dist) {
    if (!option.use_convertor)
        return dist;
//    return dist / 63.0 - 1.0;
    return dist / 68.0;
}

double OffensiveDataExtractor::convertor_angle(double angle) {
    if (!option.use_convertor)
        return angle;
//    return angle / 180.0;
    return (angle + 180.0) / 360.0;
}

double OffensiveDataExtractor::convertor_type(double type) {
    if (!option.use_convertor)
        return type;
//    return type / 9.0 - 1.0;
    return type / 18.0;
}

double OffensiveDataExtractor::convertor_cycle(double cycle) {
    if (!option.use_convertor)
        return cycle;
//    return cycle / 3000.0 - 1.0;
    return cycle / 6000.0;
}

double OffensiveDataExtractor::convertor_bv(double bv) {
    if (!option.use_convertor)
        return bv;
//    return bv / 3.0 * 2 - 1;
    return bv / 3.0;
}

double OffensiveDataExtractor::convertor_bvx(double bvx) {
    if (!option.use_convertor)
        return bvx;
//    return bvx / 3.0;
    return (bvx + 3.0) / 6.0;
}

double OffensiveDataExtractor::convertor_bvy(double bvy) {
    if (!option.use_convertor)
        return bvy;
//    return bvy / 3.0;
    return (bvy + 3.0) / 6.0;
}

double OffensiveDataExtractor::convertor_pv(double pv) {
    if (!option.use_convertor)
        return pv;
    return pv / 1.5;
}

double OffensiveDataExtractor::convertor_pvx(double pvx) {
    if (!option.use_convertor)
        return pvx;
//    return pvx / 1.5;
    return (pvx + 1.5) / 3.0;
}

double OffensiveDataExtractor::convertor_pvy(double pvy) {
    if (!option.use_convertor)
        return pvy;
//    return pvy / 1.5;
    return (pvy + 1.5) / 3.0;
}

double OffensiveDataExtractor::convertor_unum(double unum) {
    if (!option.use_convertor)
        return unum;
    if (unum == -1)
        return unum;
    return unum / 11.0;
}

double OffensiveDataExtractor::convertor_card(double card) {
    if (!option.use_convertor)
        return card;
    return card / 2.0;
}

double OffensiveDataExtractor::convertor_stamina(double stamina) {
    if (!option.use_convertor)
        return stamina;
    return stamina / 8000.0;
}

double OffensiveDataExtractor::convertor_counts(double count) {
    count = std::min(count, 20.0);
    if (!option.use_convertor)
        return count;
    return count / 20; // TODO I Dont know the MAX???
}

uint OffensiveDataExtractor::find_unum_index(DEState &state, uint unum) {
    auto players = sort_players3(state);
    if (players.size() < 11)
        std::cout<<state.kicker()->unum()<<" "<<"size problems"<<players.size()<<std::endl;
    for (uint i = 0; i < 11; i++) {
        auto player = players[i];
        if (player == nullptr)
            continue;
        if (player->unum() == unum)
            return i + 1; // TODO add 1 or not??
    }

    std::cout<<state.kicker()->unum()<<" "<<"not match"<<players.size()<<std::endl;
    return 0;
}


ODEPolar::ODEPolar(rcsc::Vector2D p) {
    teta = p.th().degree();
    r = p.r();
}

