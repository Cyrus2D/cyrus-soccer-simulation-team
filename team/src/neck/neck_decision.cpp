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

bool NeckDecisionWithBall::setNeck(PlayerAgent *agent) {
    const WorldModel & wm = agent->world();
    init(agent);
    addShootTargets(wm);
    addChainTargets(wm);
    addPredictorTargets(wm);
    addHandyPlayerTargets(wm);
    addHandyTargets(wm);
    neckEvaluator(wm);
    execute(agent);
    return true;
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
        dlog.addText(Logger::ROLE,"can see ball");
        M_can_see_ball = true;
    }
}

void NeckDecisionWithBall::addShootTargets(const WorldModel & wm){
    if(wm.ball().pos().dist(Vector2D(52,0)) > 20)
        return;
    Vector2D shoot_tar = Bhv_StrictCheckShoot(0).get_best_shoot(wm);
    if (shoot_tar.isValid() && M_self_min <= 3)
        addTarget(shoot_tar, 10.0);
}

void NeckDecisionWithBall::addChainTargets(const WorldModel & wm){
    const ActionChainGraph &chain_graph = ActionChainHolder::i().graph();
    const CooperativeAction &first_action = chain_graph.getFirstAction();
    const std::vector<ActionStatePair> &path = chain_graph.getAllChain();
    if (M_self_min > 3)
        return;
    M_find_action_by_chain = false;
    int length = path.size();
    length = 1;
    for (int i = 0; i < length; i++) {
        auto action_state = path.at(i);
        M_find_action_by_chain = true;
        double z = 1.0 / static_cast<double>(i + 1);
        if (path.at(i).action().category() == CooperativeAction::Shoot) {
            dlog.addText(Logger::ROLE, "**next action is Shoot");
            auto next_target = action_state.action().targetPoint();
            addTarget(next_target, 10 * z);
            if (wm.getOpponentGoalie() != nullptr && wm.getOpponentGoalie()->unum() > 0)
                addTarget(wm.getOpponentGoalie()->pos(), 10.0 * z);
        }

        if (path.at(i).action().category() == CooperativeAction::Dribble) {
            dlog.addText(Logger::ROLE, "**next action is Dribble");
            auto next_target = action_state.action().targetPoint();
            addTarget(next_target, 5 * z);
            Vector2D start_pos = M_ball_inertia;
            if (i > 0)
                start_pos = path.at(i - 1).action().targetPoint();
            vector<Vector2D> opp_positions;
            getDribblingOppDanger(wm, start_pos, next_target, opp_positions);
            for (int i = 0; i < opp_positions.size() && i <= 2; i++) {
                addTarget(opp_positions.at(i), (3 - i) * 3 * z);
            }
        }

        if (path.at(i).action().category() == CooperativeAction::Hold) {
            dlog.addText(Logger::ROLE, "**next action is Hold");
            for (int i = 0; i < wm.opponentsFromBall().size() && i <= 2; i++) {
                if (wm.opponentsFromBall().at(i)->pos().dist(M_ball_inertia) - min(5, wm.opponentsFromBall().at(i)->posCount()) > 5.0)
                    break;
                addTarget(wm.opponentsFromBall().at(i)->pos(), (3 - i) * 3 * z);
            }
        }

        if (path.at(i).action().category() == CooperativeAction::Pass) {
            dlog.addText(Logger::ROLE, "**next action is Pass");
            auto next_target = action_state.action().targetPoint();
            addTarget(next_target, 10 * z);
            Vector2D start_pos = M_ball_inertia;
            if (i >= 1)
                start_pos = path[i - 1].state().ball().pos();
            vector<Vector2D> opp_positions;
            getPassingOppDanger(wm, start_pos, next_target, opp_positions);
            for (int i = 0; i < opp_positions.size() && i <= 2; i++) {
                addTarget(opp_positions.at(i), (M_self_min <= 1 ? 20 : 10) * z);
            }
        }
    }
}

void NeckDecisionWithBall::addPredictorTargets(const WorldModel & wm){
    auto next_candidates = NextPassPredictor().nextReceiverCandidates(wm);
    if (next_candidates.empty())
        return;
    Vector2D start_pos = M_ball_inertia;
    for (int i = 0; i < next_candidates.size() && i < 2; i++){
        Vector2D target = next_candidates.at(i);
        addTarget(target, 20);
        vector<Vector2D> opp_positions;
        getPassingOppDanger(wm, start_pos, target, opp_positions);
        for (int i = 0; i < opp_positions.size() && i <= 1; i++) {
            addTarget(opp_positions.at(i), (M_self_min <= 1 ? 20 : 10));
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
                      (target - start).th() - 25,
                      (target - start).th() + 25);
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
        if (wm.getOpponentGoalie() != NULL && wm.getOpponentGoalie()->unum() > 0) {
            if (wm.getOpponentGoalie()->seenPosCount() > 2) {
                target = wm.getOpponentGoalie()->pos();
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

void NeckDecisionWithBall::neckEvaluator(const WorldModel & wm){
    M_best_neck = AngleDeg::INVALIDATED;
    M_best_eval = -1;
    for (double neck = M_self_body.degree() - 90.0; neck <= M_self_body.degree() + 90.0; neck += 10.0) {
        AngleDeg min_see(neck - M_next_view_width / 2.0 + 5);
        AngleDeg max_see = (neck + M_next_view_width / 2.0 - 5);
        dlog.addText(Logger::ROLE, "angle:%.1f, min:%.1f, max:%.1f", neck, min_see.degree(), max_see.degree());
        double eval = 0;
        std::cout<<M_target.size()<<std::endl;
        for (auto & target: M_target) {
            AngleDeg target_angle = (target.target - M_self_pos).th();
            if (target_angle.isWithin(min_see, max_see)) {
                double e = wm.dirCount(target_angle) * target.eval;
                dlog.addText(Logger::ROLE, "--(%.1f,%.1f):%.1f", target.target.x, target.target.y, e);
                eval += e;
            }
        }
        if (M_should_check_ball && M_can_see_ball)
            if (!M_ball_angle.isWithin(min_see, max_see))
                eval = -1;

        dlog.addText(Logger::ROLE, "eV:%.1f", eval);
        if (eval > M_best_eval) {
            M_best_eval = eval;
            M_best_neck = AngleDeg(neck);
        }
    }
}

void NeckDecisionWithBall::execute(PlayerAgent * agent){
    const WorldModel & wm = agent->world();
    if (M_best_neck.isValid()) {
        dlog.addLine(Logger::WORLD, M_self_pos, M_self_pos + Vector2D::polar2vector(30, M_best_neck), 255, 0, 0);
        agent->setNeckAction(new Neck_TurnToPoint(M_self_pos + Vector2D::polar2vector(10, M_best_neck)));
    } else if (M_should_check_ball && M_can_see_ball) {
        agent->setNeckAction(new Neck_TurnToBall());
    } else {
        agent->setNeckAction(new Neck_TurnToBallOrScan(1));
    }
}