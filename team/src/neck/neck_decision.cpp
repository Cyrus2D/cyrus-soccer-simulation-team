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

bool NeckDecisionWithBall::setNeck(PlayerAgent *agent) {
    const WorldModel & wm = agent->world();
    init(agent);
    addShootTargets(wm);
    addChainTargets(wm);
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
    bool find_pass = false;
    for (int i = 0; i < path.size() && !find_pass; i++) {
        M_find_action_by_chain = true;
        if (path.at(i).action().category() == CooperativeAction::Shoot) {
            dlog.addText(Logger::ROLE, "**next action is Shoot");
            auto next_target = first_action.targetPoint();
            addTarget(next_target, 10);
            if (wm.getOpponentGoalie() != nullptr && wm.getOpponentGoalie()->unum() > 0)
                addTarget(wm.getOpponentGoalie()->pos(), 10);
            break;
        }

        if (path.at(i).action().category() == CooperativeAction::Dribble) {
            dlog.addText(Logger::ROLE, "**next action is Dribble");
            auto next_target = first_action.targetPoint();
            addTarget(next_target, 5);
            for (int i = 0; i < wm.opponentsFromBall().size() && i <= 2; i++) {
                addTarget(wm.opponentsFromBall().at(i)->pos(), (3 - i) * 3);
            }
            break;
        }

        if (path.at(i).action().category() == CooperativeAction::Hold) {
            dlog.addText(Logger::ROLE, "**next action is Hold");
            for (int i = 0; i < wm.opponentsFromBall().size() && i <= 2; i++) {
                addTarget(wm.opponentsFromBall().at(i)->pos(), (3 - i) * 3);
            }
            break;
        }

        if (path.at(i).action().category() == CooperativeAction::Pass) {
            dlog.addText(Logger::ROLE, "**next action is Pass");
            auto next_target = first_action.targetPoint();
            addTarget(next_target, 10);
            Vector2D start_ball = M_ball_inertia;
            if (i >= 1) {
                start_ball = path[i - 1].state().ball().pos();
            }
            Sector2D pass_sec(start_ball, 0, next_target.dist(M_ball_inertia) + 5, (next_target - M_ball_inertia).th() - 20,
                              (next_target - M_ball_inertia).th() + 20);
            for (int i = 0; i < wm.opponentsFromBall().size(); i++) {
                if (pass_sec.contains(wm.opponentsFromBall().at(i)->pos())) {
                    addTarget(wm.opponentsFromBall().at(i)->pos(), (M_self_min <= 1 ? 20 : 10));
                }
            }
            find_pass = true;
            break;
        }
        if (path[i].state().ballHolderUnum() != wm.self().unum())
            find_pass = true;
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