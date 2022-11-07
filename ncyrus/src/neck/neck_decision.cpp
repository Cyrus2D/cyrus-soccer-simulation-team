//
// Created by nader on 2022-05-20.
//

#include "neck_decision.h"
#include <rcsc/player/intercept_table.h>
#include "../chain_action/bhv_strict_check_shoot.h"
#include "../chain_action/action_chain_graph.h"
#include "../chain_action/action_chain_holder.h"
#include "../chain_action/field_analyzer.h"
#include <rcsc/action/neck_turn_to_point.h>
#include <rcsc/action/neck_turn_to_ball.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include "next_pass_predictor.h"
#include "../chain_action/neck_turn_to_receiver.h"
#include "../debugs.h"
#include "../setting.h"


bool NeckDecisionWithBall::setNeck(PlayerAgent *agent, NeckDecisionType type) {
    init(agent);
    switch (type) {
        case intercept:
        case dribbling:
            advancedWithBall(agent);
            break;
        case unmarking:
        case chain_action:
        case offensive_move:
            simpleWithBall(agent);
            break;
        case passing:
        case chain_action_first:
            neckToReceiver(agent);
            break;
    }
    return true;
}

void NeckDecisionWithBall::advancedWithBall(PlayerAgent *agent){
    const WorldModel & wm = agent->world();
    addShootTargets(wm);
    addPredictorTargets(wm);
    addChainTargets(wm);
    addHandyPlayerTargets(wm);
    addHandyTargets(wm);
    bool isValid = neckEvaluator(wm);
    execute(agent, isValid);
}

void NeckDecisionWithBall::simpleWithBall(PlayerAgent *agent) {
    const WorldModel &wm = agent->world();
    if (wm.kickableOpponent()
        && wm.ball().distFromSelf() < 18.0) {
        agent->setNeckAction(new Neck_TurnToBall());
    } else {
        agent->setNeckAction(new Neck_TurnToBallOrScan(0));
    }
}

void NeckDecisionWithBall::neckToReceiver(PlayerAgent *agent) {
    const ActionChainGraph &chain_graph = ActionChainHolder::i().graph();
    agent->setNeckAction( new Neck_TurnToReceiver( chain_graph ) );
}
void NeckDecisionWithBall::init(PlayerAgent *agent){
    const WorldModel & wm = agent->world();
    M_self_min = wm.interceptTable()->selfReachCycle();
    M_ball_pos_count = wm.ball().posCount();
    M_ball_vel_count = wm.ball().velCount();
    M_ball_count = std::min(M_ball_pos_count, M_ball_vel_count);
    M_ball_pos = wm.ball().inertiaPoint(1);
    M_self_pos = agent->effector().queuedNextSelfPos();
    M_self_body = agent->effector().queuedNextMyBody();
    M_ball_angle = (M_ball_pos - M_self_pos).th();
    M_ball_inertia = wm.ball().inertiaPoint(M_self_min);
    M_next_view_width = agent->effector().queuedNextViewWidth();
    M_should_check_ball = (M_ball_vel_count > 2 || M_ball_pos_count > 2) ? true : false;
    M_can_see_ball = false;
    if ((M_self_body - M_ball_angle).abs() < M_next_view_width * 0.5 + 85){
        #ifdef DEBUG_NECK_DECISION
        dlog.addText(Logger::ROLE,"can see ball");
        #endif
        M_can_see_ball = true;
    }
    M_use_pass_predictor = Setting::i()->mNeck->mUsePredictionDNN;
    M_use_pass_predictor_if_chain_find_pass = Setting::i()->mNeck->mUsePPIfChain;
    M_ignore_chain_pass_target_for_predictor = Setting::i()->mNeck->mIgnoreChainPass;
    M_ignore_chain_pass_target_for_predictor_dist = Setting::i()->mNeck->mIgnoreChainPassByDist;
}

void NeckDecisionWithBall::addShootTargets(const WorldModel & wm){
    if(wm.ball().pos().dist(Vector2D(52,0)) > 20)
        return;
    Vector2D shoot_tar = Bhv_StrictCheckShoot(0).get_best_shoot(wm);
    if (shoot_tar.isValid() && M_self_min <= 3){
        #ifdef DEBUG_NECK_DECISION
        dlog.addText(Logger::ROLE, ">>Add shoot target (.1f, .1f) 10.0", shoot_tar.x, shoot_tar.y);
        #endif
        addTarget(shoot_tar, 10.0);
    }

}

void NeckDecisionWithBall::addChainTargets(const WorldModel & wm){
    const ActionChainGraph &chain_graph = ActionChainHolder::i().graph();
    const std::vector<ActionStatePair> &path = chain_graph.getAllChain();
    if (M_self_min > 3)
        return;
    #ifdef DEBUG_NECK_DECISION
    dlog.addText(Logger::ROLE, ">>Add Chain Targets");
    #endif
    M_find_action_by_chain = false;
    int length = path.size();
    length = std::min(length, 1);
    for (int i = 0; i < length; i++) {
        #ifdef DEBUG_NECK_DECISION
        dlog.addText(Logger::ROLE, "#### Level %d", i);
        #endif
        auto action_state = path.at(i);
        M_find_action_by_chain = true;
        double z = 1.0 / static_cast<double>(i + 1);
        if (path.at(i).action().category() == CooperativeAction::Shoot) {
            auto next_target = action_state.action().targetPoint();
            addTarget(next_target, 10.0 * z);
            #ifdef DEBUG_NECK_DECISION
            dlog.addText(Logger::ROLE, "##### Add Shoot (%.1f, %.1f) %.1f", next_target.x, next_target.y, 10.0 * z);
            #endif
            if (wm.getTheirGoalie() != nullptr && wm.getTheirGoalie()->unum() > 0){
                auto opp_pos = wm.getTheirGoalie()->pos();
                addTarget(opp_pos, 10.0 * z);
                #ifdef DEBUG_NECK_DECISION
                dlog.addText(Logger::ROLE, "##### Add Shoot Goalie(%.1f, %.1f) %.1f", opp_pos.x, opp_pos.y, 10.0 * z);
                #endif
            }

        }

        if (path.at(i).action().category() == CooperativeAction::Dribble) {
            auto next_target = action_state.action().targetPoint();
            addTarget(next_target, 5 * z);
            #ifdef DEBUG_NECK_DECISION
            dlog.addText(Logger::ROLE, "##### Add Dribble (%.1f, %.1f) %.1f", next_target.x, next_target.y, 5.0 * z);
            #endif
            Vector2D start_pos = M_ball_inertia;
            if (i > 0)
                start_pos = path.at(i - 1).action().targetPoint();
            vector<Vector2D> opp_positions;
            getDribblingOppDanger(wm, start_pos, next_target, opp_positions);
            for (int i = 0; i < opp_positions.size() && i <= 2; i++) {
                addTarget(opp_positions.at(i), (3.0 - i) * 3.0 * z);
                #ifdef DEBUG_NECK_DECISION
                dlog.addText(Logger::ROLE, "##### Add Dribble Opp (%.1f, %.1f) %.1f", opp_positions.at(i).x, opp_positions.at(i).y, (3.0 - i) * 3.0 * z);
                #endif
            }
        }

        if (path.at(i).action().category() == CooperativeAction::Hold) {

            for (int i = 0; i < wm.opponentsFromBall().size() && i <= 2; i++) {
                if (wm.opponentsFromBall().at(i)->pos().dist(M_ball_inertia) - min(5, wm.opponentsFromBall().at(i)->posCount()) > 5.0)
                    break;
                auto opp_pos = wm.opponentsFromBall().at(i)->pos();
                addTarget(opp_pos, (3.0 - i) * 3.0 * z);
                #ifdef DEBUG_NECK_DECISION
                dlog.addText(Logger::ROLE, "##### Add Hold Opp (%.1f, %.1f) %.1f", opp_pos.x, opp_pos.y, (3.0 - i) * 3.0 * z);
                #endif
            }
        }

        if (path.at(i).action().category() == CooperativeAction::Pass) {
            if (M_ignore_chain_pass_target_for_predictor && M_predictor_targets.size() > 0)
                continue;
            auto next_target = action_state.action().targetPoint();
            bool is_near_to_predictor_targets = false;
            if (M_ignore_chain_pass_target_for_predictor_dist)
                for (auto & predictor_target: M_predictor_targets){
                    if (predictor_target.dist(next_target) < 5.0){
                        is_near_to_predictor_targets = true;
                        break;
                    }
                }
            if (!is_near_to_predictor_targets){
                addTarget(next_target, 10.0 * z);
                #ifdef DEBUG_NECK_DECISION
                dlog.addText(Logger::ROLE, "##### Add Pass (%.1f, %.1f) %.1f", next_target.x, next_target.y, 10.0 * z);
                #endif
                Vector2D start_pos = M_ball_inertia;
                if (i >= 1)
                    start_pos = path[i - 1].state().ball().pos();
                vector<Vector2D> opp_positions;
                getPassingOppDanger(wm, start_pos, next_target, opp_positions);
                for (int i = 0; i < opp_positions.size() && i <= 2; i++) {
                    auto opp_pos = opp_positions.at(i);
                    addTarget(opp_pos, (M_self_min <= 1 ? 20.0 : 10.0) * z);
                    #ifdef DEBUG_NECK_DECISION
                    dlog.addText(Logger::ROLE, "##### Add Pass Opp (%.1f, %.1f) %.1f", opp_pos.x, opp_pos.y, (M_self_min <= 1 ? 20.0 : 10.0) * z);
                    #endif
                }
            }
        }
    }
}

void NeckDecisionWithBall::addPredictorTargets(const WorldModel & wm){
    if (!M_use_pass_predictor)
        return;
    if (M_self_min > 3)
        return;
    const ActionChainGraph &chain_graph = ActionChainHolder::i().graph();
    const std::vector<ActionStatePair> &path = chain_graph.getAllChain();
    if (M_use_pass_predictor_if_chain_find_pass){
        if (path.size() == 0)
            return;
        if (path.front().action().category() != CooperativeAction::Pass)
            return;
    }
    auto next_candidates = NextPassPredictor().nextReceiverCandidates(wm);
    if (next_candidates.empty())
        return;
    Vector2D start_pos = M_ball_inertia;
    for (int i = 0; i < next_candidates.size() && i < 2; i++){
        Vector2D target = next_candidates.at(i);
        addTarget(target, 20);
        #ifdef DEBUG_NECK_DECISION
        dlog.addText(Logger::ROLE, "##### Add Pass Predictor (%.1f, %.1f) %.1f", target.x, target.y, 10.0);
        #endif
        M_predictor_targets.push_back(target);
        vector<Vector2D> opp_positions;
        getPassingOppDanger(wm, start_pos, target, opp_positions);
        for (int i = 0; i < opp_positions.size() && i <= 1; i++) {
            auto opp_pos = opp_positions.at(i);
            addTarget(opp_pos, (M_self_min <= 1 ? 20.0 : 10.0));
            #ifdef DEBUG_NECK_DECISION
            dlog.addText(Logger::ROLE, "##### Add Pass Predictor Opp (%.1f, %.1f) %.1f", opp_pos.x, opp_pos.y, (M_self_min <= 1 ? 20.0 : 10.0));
            #endif
        }
    }
}

bool vectorDoubleVectorSorter(const pair<double, Vector2D> &a,  const pair<double, Vector2D> &b)
{
    return (a.first < b.first);
}

void NeckDecisionWithBall::getDribblingOppDanger(const WorldModel & wm,
                                                 Vector2D & start,
                                                 Vector2D & target,
                                                 vector<Vector2D> & opp_positions){
    vector<pair<double, Vector2D> > positions_eval;
    double dribble_dist = start.dist(target);
    Segment2D dribble_segment(start, target);
    for (auto & opp: wm.opponentsFromBall()) {
        if (opp == nullptr || opp->unum() < 1)
            continue;
        Vector2D pos = opp->pos();
        double opp_dist = dribble_segment.dist(pos);
        if (opp_dist - static_cast<double>(min(8, opp->posCount())) > dribble_dist + 5.0)
            continue;
        positions_eval.push_back(make_pair(opp_dist - static_cast<double>(opp->posCount()), pos));
    }
    sort(positions_eval.begin(), positions_eval.end(), vectorDoubleVectorSorter);
    for (auto & e_p: positions_eval){
        opp_positions.push_back(e_p.second);
        if (opp_positions.size() == 3)
            break;
    }
}

void NeckDecisionWithBall::getPassingOppDanger(const WorldModel & wm,
                                               Vector2D & start,
                                               Vector2D & target,
                                               vector<Vector2D> & opp_positions){
    vector<pair<double, Vector2D> > positions_eval;
    Sector2D pass_sec(start,
                      0,
                      target.dist(start) + 5,
                      (target - start).th() - 35,
                      (target - start).th() + 35);
    for (int i = 0; i < wm.opponentsFromBall().size(); i++) {
        Vector2D pos = wm.opponentsFromBall().at(i)->pos();
        double diff_angle = ((pos - start).th() - (target - start).th()).abs();
        if (pass_sec.contains(pos)) {
            positions_eval.push_back(make_pair(diff_angle / max(1.0, pos.dist(start)), pos));
        }
    }

    sort(positions_eval.begin(), positions_eval.end(), vectorDoubleVectorSorter);
    for (auto & e_p: positions_eval){
        opp_positions.push_back(e_p.second);
        if (opp_positions.size() == 3)
            break;
    }
}

void NeckDecisionWithBall::addHandyTargets(const WorldModel & wm){
    if (M_ball_inertia.x > 30) {
        if (M_ball_inertia.y > 20) {
            addTarget(Vector2D(45, 10), (M_find_action_by_chain ? 2 : 3));
            addTarget(Vector2D(30, 12), (M_find_action_by_chain ? 2 : 3));
        } else if (M_ball_inertia.y < -20) {
            addTarget(Vector2D(45, -10), (M_find_action_by_chain ? 2 : 3));
            addTarget(Vector2D(30, -12), (M_find_action_by_chain ? 2 : 3));
            addTarget(Vector2D(40, -15), (M_find_action_by_chain ? 2 : 3));
        } else {
            addTarget(Vector2D(45, 0), (M_find_action_by_chain ? 2 : 3));
            addTarget(Vector2D(30, 0), (M_find_action_by_chain ? 2 : 3));
        }
    } else if (M_ball_inertia.x > -15) {
        addTarget(Vector2D(30, -25), (M_find_action_by_chain ? 2 : 3));
        addTarget(Vector2D(30, -15), (M_find_action_by_chain ? 2 : 3));
        addTarget(Vector2D(30, +15), (M_find_action_by_chain ? 2 : 3));
        addTarget(Vector2D(30, +25), (M_find_action_by_chain ? 2 : 3));
    } else {
        addTarget(Vector2D(-10, -25), (M_find_action_by_chain ? 2 : 4));
        addTarget(Vector2D(-10, -15), (M_find_action_by_chain ? 2 : 4));
        addTarget(Vector2D(-10, +15), (M_find_action_by_chain ? 2 : 4));
    }

}

void NeckDecisionWithBall::addHandyPlayerTargets(const WorldModel & wm){
    auto target = Vector2D::INVALIDATED;
    if (M_ball_inertia.x > 30 && M_ball_inertia.absY() > 15) {
        target = Vector2D(40, 0);
        addTarget(target, (M_find_action_by_chain ? 3 : 5));
        if (FieldAnalyzer::isFRA(wm)) {
            target = Vector2D(52, 0);
            addTarget(target, (M_find_action_by_chain ? 10 : 10));
        }
    } else if (M_ball_inertia.x > 30 && M_ball_inertia.absY() < 15) {
        if (wm.getTheirGoalie() != NULL && wm.getTheirGoalie()->unum() > 0) {
            if (wm.getTheirGoalie()->seenPosCount() > 2) {
                target = wm.getTheirGoalie()->pos();
                addTarget(target, 5);
            }
        } else {
            if (wm.dirCount((Vector2D(52, 5) - M_self_pos).th()) > 1) {
                addTarget(Vector2D(52, 5), 5);
            }
            if (wm.dirCount((Vector2D(52, -5) - M_self_pos).th()) > 1) {
                addTarget(Vector2D(52, -5), 5);
            }
            target = Vector2D(52, 0);
            addTarget(target, 8);
        }
        for (int i = 0; i < wm.opponentsFromBall().size(); i++) {
            auto opp = wm.opponentsFromBall().at(i);
            if (opp->pos().x > 46 && opp->pos().absY() < 7) {
                addTarget(opp->pos(), 8);
            }
        }
    }
    for (size_t i = 0; i < wm.opponentsFromBall().size() && i <= 2; i++) {
        addTarget(wm.opponentsFromBall().at(i)->pos(), (3 - i) * (M_find_action_by_chain ? 1 : 2));
    }
    for (size_t i = 0; i < wm.teammatesFromBall().size() && i <= 3; i++) {
        addTarget(wm.teammatesFromBall().at(i)->pos(), (4 - i) * (M_find_action_by_chain ? 1 : 1.5));
    }
}

bool NeckDecisionWithBall::neckEvaluator(const WorldModel & wm){
    // M_best_neck = AngleDeg::INVALIDATED; // CYRUS_LIB F0
    // M_best_neck = AngleDeg(numeric_limits<double>::max()); CYRUS_LIB F1
    M_best_neck = AngleDeg();
    bool isValid = false;
    M_best_eval = -1;
    double max_target_eval = 0;
    for (auto & target: M_target){
        if (target.eval > max_target_eval)
            max_target_eval = target.eval;
    }
    #ifdef DEBUG_NECK_DECISION
    for (auto & target: M_target){
        dlog.addCircle(Logger::PLAN, target.target, max(0.2, 3 * target.eval / max(max_target_eval, 0.1)), 0, 0, 255,
                       true);
    }
    #endif
    struct neckEval{
        int index; double eval; double neck_angle;
        neckEval(int index_, double eval_, double neck_angle_){
            index = index_;
            eval = eval_;
            neck_angle = neck_angle_;
        }
    };
    vector<neckEval> evals;
    int angle_index = 0;
    #ifdef DEBUG_NECK_DECISION
    dlog.addText(Logger::PLAN, "My Next Body: %.1f", M_self_body.degree());
    #endif
    for (double neck = M_self_body.degree() - 90.0; neck <= M_self_body.degree() + 90.0; neck += 10.0) {
        AngleDeg min_see(neck - M_next_view_width / 2.0 + 5);
        AngleDeg max_see = (neck + M_next_view_width / 2.0 - 5);
        #ifdef DEBUG_NECK_DECISION
        dlog.addText(Logger::ROLE, ">> ##%d Angle:%.1f, min:%.1f, max:%.1f", angle_index, neck, min_see.degree(), max_see.degree());
        #endif
        double eval = 0;
        for (auto & target: M_target) {
            AngleDeg target_angle = (target.target - M_self_pos).th();
            if (target_angle.isWithin(min_see, max_see)) {
                int dir_count = wm.dirCount(target_angle);
                double e = target.eval;
                if (dir_count == 0)
                    e *= 0.1;
                else
                    e *= static_cast<double>(dir_count);
                #ifdef DEBUG_NECK_DECISION
                dlog.addText(Logger::ROLE, "------(%.1f,%.1f):%.1f", target.target.x, target.target.y, e);
                #endif
                eval += e;
            }
        }
        if (M_should_check_ball && M_can_see_ball)
            if (!M_ball_angle.isWithin(min_see, max_see))
                eval = -1;
        #ifdef DEBUG_NECK_DECISION
        dlog.addText(Logger::ROLE, "##%d eV:%.1f", angle_index, eval);
        #endif
        if (eval > M_best_eval) {
            M_best_eval = eval;
            M_best_neck = AngleDeg(neck);
            isValid = true;
        }
        evals.push_back(neckEval(angle_index, max(eval, 0.0), neck));
        angle_index += 1;
    }
    if (M_best_eval > 0)
        for (auto & n_e: evals){
            Vector2D tmp = Vector2D::polar2vector(n_e.eval / M_best_eval * 20.0, n_e.neck_angle);
            #ifdef DEBUG_NECK_DECISION
            dlog.addLine(Logger::ROLE, M_self_pos, M_self_pos + tmp, 0, 0, 255);
            #endif
        }
    #ifdef DEBUG_NECK_DECISION
    dlog.addText(Logger::ROLE, "Best Neck: %.1f", M_best_neck.degree());
    #endif
    return isValid;
}

void NeckDecisionWithBall::execute(PlayerAgent * agent, bool isValid){
    const WorldModel & wm = agent->world();
    #ifdef DEBUG_NECK_DECISION
    dlog.addText(Logger::PLAN, "Set neck execute");
    #endif
    // if (M_best_neck.isValid()) { // CYRUS_LIB F0
    if (isValid) { //&& M_best_neck != numeric_limits<double>::max()) {  // CYRUS_LIB F1
        #ifdef DEBUG_NECK_DECISION
        dlog.addLine(Logger::PLAN, M_self_pos, M_self_pos + Vector2D::polar2vector(10, M_best_neck), 255, 0, 0);
        dlog.addText(Logger::PLAN, "Set neck to %.1f", M_best_neck.degree());
        #endif
        agent->setNeckAction(new Neck_TurnToPoint(M_self_pos + Vector2D::polar2vector(10, M_best_neck)));
    } else if (M_should_check_ball && M_can_see_ball) {
        #ifdef DEBUG_NECK_DECISION
        dlog.addText(Logger::PLAN, "Set neck turn to ball");
        #endif
        agent->setNeckAction(new Neck_TurnToBall());
    } else {
        #ifdef DEBUG_NECK_DECISION
        dlog.addText(Logger::PLAN, "Set neck scan");
        agent->setNeckAction(new Neck_TurnToBallOrScan(1));
        #endif
    }
}
