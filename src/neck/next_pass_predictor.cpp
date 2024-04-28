//
// Created by nader on 2022-05-23.
//

#include "next_pass_predictor.h"
#include "../setting.h"
#include "../data_extractor/DEState.h"
#include "../data_extractor/offensive_data_extractor.h"
#include "basic_actions/neck_turn_to_point.h"


DeepNueralNetwork * NextPassPredictor::pass_prediction = new DeepNueralNetwork();
void NextPassPredictor::load_dnn(){
    static bool load_dnn = false;
    if(!load_dnn){
        load_dnn = true;
        pass_prediction->ReadFromKeras(Setting::i()->mNeck->mPredictionDNNPath);
    }
}
vector<pass_prob> NextPassPredictor::predict_pass(vector<double> & features, vector<int> ignored_player, int kicker){
    load_dnn();
    MatrixXd input(463,1); // 463 12
    for (int i = 0; i < 463; i += 1){
        input(i ,0) = features[i];
    }
    pass_prediction->Calculate(input);
    vector<pass_prob> predict;
    for (int i = 0; i < 12; i++){
        if (i == 0){
            dlog.addText(Logger::POSITIONING, "##### Neck Pass from %d to %d : %.6f NOK(0)", kicker, i, pass_prediction->mOutput(i));
        }else if(std::find(ignored_player.begin(), ignored_player.end(), i) == std::end(ignored_player)){
            dlog.addText(Logger::POSITIONING, "##### Neck Pass from %d to %d : %.6f OKKKK", kicker, i, pass_prediction->mOutput(i));
            predict.push_back(pass_prob(pass_prediction->mOutput(i), kicker, i));
        }else{
            dlog.addText(Logger::POSITIONING, "##### Neck Pass from %d to %d : %.6f NOK(ignored)", kicker, i, pass_prediction->mOutput(i));
        }
    }
    std::sort(predict.begin(), predict.end(), pass_prob::ProbCmpBiggestFirst);
    return predict;
}
vector<Vector2D> NextPassPredictor::nextReceiverCandidates(const WorldModel &wm){
    vector<Vector2D> res;
    DEState state = DEState(wm);

    int fastest_tm = wm.self().unum();
    int tm_reach_cycle = wm.interceptTable().selfStep();
    if (!state.updateKicker(fastest_tm, wm.ball().inertiaPoint(tm_reach_cycle)))
        return res;

    vector<int> ignored_player;
    string ignored = "";
    for (int i = 1; i <= 11; i++){
        if (wm.ourPlayer(i) == nullptr || wm.ourPlayer(i)->unum() < 1 || not wm.ourPlayer(i)->pos().isValid() || wm.self().unum() == i){
            ignored_player.push_back(i);
            ignored += std::to_string(i) + ",";
        }
    }
    auto features = OffensiveDataExtractor::i().get_data(state);
    auto passes = predict_pass(features, ignored_player, wm.self().unum());
    if (passes.empty())
        return res;

    for (auto &p: passes){
        int tm_unum = p.pass_getter;
        if (p.prob > 0.3 || res.size() == 0)
            res.push_back(wm.ourPlayer(tm_unum)->pos());
        dlog.addText(Logger::POSITIONING, "%d %.3f", p.pass_getter, p.prob);
    }
    return res;
}
int NextPassPredictor::next_receiver(const WorldModel &wm){
    DEState state = DEState(wm);

    int fastest_tm = wm.self().unum();
    int tm_reach_cycle = wm.interceptTable().selfStep();
    if (!state.updateKicker(fastest_tm, wm.ball().inertiaPoint(tm_reach_cycle)))
        return 0;

    vector<int> ignored_player;
    string ignored = "";
    for (int i = 1; i <= 11; i++){
        if (wm.ourPlayer(i) == nullptr || wm.ourPlayer(i)->unum() < 1 || not wm.ourPlayer(i)->pos().isValid() || wm.self().unum() == i){
            ignored_player.push_back(i);
            ignored += std::to_string(i) + ",";
        }
    }
    auto features = OffensiveDataExtractor::i().get_data(state);
    auto passes = predict_pass(features, ignored_player, wm.self().unum());
    int next_target_unum = 0;
    if (!passes.empty()){
        for (auto &p: passes){
            dlog.addText(Logger::POSITIONING, "%d %.3f", p.pass_getter, p.prob);
        }
        next_target_unum = passes.front().pass_getter;
        dlog.addLine(Logger::BLOCK, wm.self().pos(), wm.ourPlayer(next_target_unum)->pos());
    }

    return next_target_unum;
}
bool NextPassPredictor::can_see_position(PlayerAgent * agent, const WorldModel & wm, Vector2D & target, double neck_angle){
    Vector2D self_pos = wm.self().pos();
    AngleDeg self_body = agent->effector().queuedNextMyBody();
    const double next_view_width = agent->effector().queuedNextViewWidth().width();
    bool can_see = false;
    if (neck_angle <=-360){
        if ((self_body - (target - self_pos).th()).abs() < next_view_width * 0.5 + 85.0){
            can_see = true;
        }
    }else{
        if ((neck_angle - (target - self_pos).th()).abs() < next_view_width * 0.5 - 5.0){
            can_see = true;
        }
    }

    return can_see;
}
bool NextPassPredictor::pass_predictor_neck(rcsc::PlayerAgent * agent){
    dlog.addText(Logger::PLAN, "pass_predictor_neck");
    const WorldModel &wm = agent->world();
    int next_target_unum = next_receiver(wm);
    if (next_target_unum == 0)
        return false;
    agent->debugClient().addMessage("neck to %d", next_target_unum);
    int self_min = wm.interceptTable().selfStep();
    int ball_pos_count = wm.ball().posCount();
    int ball_vel_count = wm.ball().velCount();
    Vector2D ball_pos = wm.ball().inertiaPoint(self_min);
    const AbstractPlayerObject * tm = wm.ourPlayer(next_target_unum);
    if (tm == nullptr or tm->unum() < 1)
        return false;
    Vector2D next_target = tm->pos();
    bool should_see_tm = true;
    if (tm->posCount() == 0)
        should_see_tm = false;
    int danger_opp = 0;
    double danger_opp_eval = -1;
    for(auto & opp: wm.theirPlayers()){
        if (opp == nullptr)
            continue;
        if (opp->unum() < 1)
            continue;
        Vector2D opp_pos = opp->pos();
        double dist = opp_pos.dist(ball_pos);
        if (dist > next_target.dist(ball_pos) + 10)
            continue;
        if (!can_see_position(agent, wm, opp_pos))
            continue;
        double diff_angle = ((opp_pos - ball_pos).th() - (next_target - ball_pos).th()).abs();
        if (diff_angle > 90)
            continue;
        double opp_eval = dist / std::max(diff_angle, 0.1);
        if (opp_eval > danger_opp_eval){
            danger_opp_eval = opp_eval;
            danger_opp = opp->unum();
        }
    }
    bool can_see_opp = false;
    bool should_see_opp = false;
    Vector2D opp_pos = Vector2D::INVALIDATED;
    if (danger_opp > 0){
        can_see_opp = true;
        opp_pos = wm.theirPlayer(danger_opp)->pos();
        if (wm.theirPlayer(danger_opp)->posCount() > 1){
            should_see_opp = true;
        }
    }


    Vector2D self_pos = wm.self().pos();
    AngleDeg self_body = agent->effector().queuedNextMyBody();
    AngleDeg ball_angle = (ball_pos - self_pos).th();
    Vector2D ball_iner = wm.ball().inertiaPoint(self_min);
    const double next_view_width = agent->effector().queuedNextViewWidth().width();
    bool should_see_ball = false;
    if (ball_pos_count > 2 || ball_vel_count > 2)
        should_see_ball = true;
    if (wm.self().isKickable())
        should_see_ball = false;
    dlog.addText(Logger::ANALYZER, "Find Best Neck, next neck %.1f", next_view_width);
    double best_neck = -90;
    double best_eval = 0;
    for (double neck = -90; neck <= 90; neck += 10){

        bool can_neck_see_ball = false;
        bool can_neck_see_tm = false;
        bool can_neck_see_opp = false;
        double next_neck = (self_body + AngleDeg(neck)).degree();

        if (((ball_iner - self_pos).th() - AngleDeg(next_neck)).abs() < next_view_width / 2.0 - 5.0)
            can_neck_see_ball = true;
        if (opp_pos.isValid()){
            if (((opp_pos - self_pos).th() - AngleDeg(next_neck)).abs() < next_view_width / 2.0 - 5.0)
                can_neck_see_opp = true;
        }
        if (((next_target - self_pos).th() - AngleDeg(next_neck)).abs() < next_view_width / 2.0 - 5.0)
            can_neck_see_tm = true;
        double eval = 0;
        if (can_neck_see_ball && should_see_ball)
            eval += 10;
        else if (can_neck_see_ball)
            eval += 3;
        if (can_neck_see_opp && should_see_opp)
            eval += 7;
        else if (can_neck_see_opp)
            eval += 2;
        if (can_neck_see_tm && should_see_tm)
            eval += 10;
        else if (can_neck_see_tm)
            eval += 3;
        if (eval > best_eval){
            best_eval = eval;
            best_neck = next_neck;
        }
        dlog.addText(Logger::ANALYZER, ">>>>> Angle %.1f %.1f %d B%d T%d(%d) O%d(%d) E%.1f", neck, next_neck, next_target_unum, can_neck_see_ball, can_neck_see_tm, next_target_unum, can_neck_see_opp, danger_opp, eval);
    }
    if (best_eval > 0){
        agent->setNeckAction(new Neck_TurnToPoint(self_pos + Vector2D::polar2vector(10, best_neck)));
        return true;
    }
    return false;
}