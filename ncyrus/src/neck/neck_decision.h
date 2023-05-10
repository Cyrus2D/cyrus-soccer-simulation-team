//
// Created by nader on 2022-05-20.
//

#ifndef SRC_NECK_DECISION_H
#define SRC_NECK_DECISION_H

#include <rcsc/player/player_agent.h>
#include <rcsc/geom.h>
#include <vector>

using namespace std;
using namespace rcsc;
class ObserveTargetEval{
public:
    Vector2D target;
    double eval;
    ObserveTargetEval(Vector2D target_, double eval_){
        target = target_;
        eval = eval_;
    }
};

enum NeckDecisionType{
    intercept,
    passing,
    dribbling,
    unmarking,
    chain_action,
    offensive_move,
    chain_action_first
};

class NeckDecisionWithBall{
public:
    bool setNeck(PlayerAgent * agent, NeckDecisionType type);
private:
    void init(PlayerAgent *agent);
    void advancedWithBall(PlayerAgent *agent);
    void simpleWithBall(PlayerAgent *agent);
    void neckToReceiver(PlayerAgent *agent);
    void addTarget(ObserveTargetEval target){
        M_target.push_back(target);
    }
    void addTarget(Vector2D target, double eval){
        addTarget(ObserveTargetEval(target, eval));
    }
    void addShootTargets(const WorldModel & wm);
    void addChainTargets(const WorldModel & wm);
    void addPredictorTargets(const WorldModel & wm);
    void addHandyTargets(const WorldModel & wm);
    void addHandyPlayerTargets(const WorldModel & wm);
    bool neckEvaluator(const WorldModel & wm);
    void execute(PlayerAgent * agent, bool isValid);
    void getDribblingOppDanger(const WorldModel & wm,
                               Vector2D & start,
                               Vector2D & target,
                               vector<Vector2D> & opp_positions);
    void getPassingOppDanger(const WorldModel & wm,
                               Vector2D & start,
                               Vector2D & target,
                               vector<Vector2D> & opp_positions);
    int M_self_min;
    int M_ball_pos_count;
    int M_ball_vel_count;
    int M_ball_count;
    Vector2D M_ball_pos;
    Vector2D M_self_pos;
    AngleDeg M_self_body;
    AngleDeg M_ball_angle;
    Vector2D M_ball_inertia;
    double M_next_view_width;
    bool M_should_check_ball;
    bool M_can_see_ball;
    vector<ObserveTargetEval> M_target;
    bool M_find_action_by_chain;
    vector<Vector2D> M_predictor_targets;
    AngleDeg M_best_neck;
    double M_best_eval;
    bool M_use_pass_predictor;
    bool M_use_pass_predictor_if_chain_find_pass;
    bool M_ignore_chain_pass_target_for_predictor;
    bool M_ignore_chain_pass_target_for_predictor_dist;


};
#endif //SRC_NECK_DECISION_H
