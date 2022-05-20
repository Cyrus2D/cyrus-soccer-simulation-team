// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "bhv_basic_move.h"

#include "strategy.h"

#include "move_def/bhv_basic_tackle.h"
#include "move_off/bhv_unmark.h"
#include "move_off/bhv_scape.h"
#include "move_def/bhv_block.h"
#include "move_def/bhv_mark_execute.h"
#include "move_def/bhv_tackle_intercept.h"
#include "chain_action/field_analyzer.h"

#include "move_def/cyrus_interceptable.h"
#include "chain_action/bhv_pass_kick_find_receiver.h"
#include "chain_action/action_chain_holder.h"
#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/body_intercept.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include "chain_action/shoot_generator.h"
#include "chain_action/bhv_strict_check_shoot.h"
#include <thread>
#include <chrono>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/player/intercept_table.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/common/audio_memory.h>

#include <rcsc/player/abstract_player_object.h>
#include "neck/neck_offensive_intercept_neck.h"
#include "move_off/bhv_offensive_move.h"
#include "neck/next_pass_predictor.h"
#include "neck/neck_decision.h"
using namespace rcsc;

DeepNueralNetwork * NextPassPredictor::pass_prediction = new DeepNueralNetwork();

bool Bhv_BasicMove::set_def_neck_with_ball(PlayerAgent *agent, Vector2D targetPoint, const AbstractPlayerObject *opp,
                                           int blocker) {
    const WorldModel &wm = agent->world();
    Vector2D next_target = Vector2D::INVALIDATED;
    int self_min = wm.interceptTable()->selfReachCycle();
    int ball_pos_count = wm.ball().posCount();
    int ball_pos_count_s = wm.ball().seenPosCount();
    int ball_vel_count = wm.ball().velCount();
    int ball_vel_count_s = wm.ball().seenVelCount();
    Vector2D ball_pos = wm.ball().pos();
    Vector2D self_pos = wm.self().pos();
    AngleDeg self_body = agent->effector().queuedNextMyBody();
    int min_ball_count = std::min(std::min(ball_pos_count, ball_pos_count_s),
                                  std::min(ball_vel_count, ball_vel_count_s));
    AngleDeg ball_angle = (ball_pos - self_pos).th();
    Vector2D ball_iner = wm.ball().inertiaPoint(self_min);
    const double next_view_width = agent->effector().queuedNextViewWidth().width();
    bool should_see_ball = false;
    if (min_ball_count >= std::min(wm.interceptTable()->opponentReachCycle(), 5))
        should_see_ball = true;
    bool can_see_ball = false;
    if ((self_body - ball_angle).abs() < next_view_width * 0.5 + 85)
        can_see_ball = true;
    if (wm.self().isKickable())
        should_see_ball = false;
    std::vector<std::pair<Vector2D, double>> targs;
    for (int i = 0; i < wm.opponentsFromSelf().size() && i <= 3; i++) {
        if (wm.opponentsFromSelf().at(i) != NULL && wm.opponentsFromSelf().at(i)->posCount() > 0)
            targs.emplace_back(wm.opponentsFromSelf().at(i)->pos(), 0.9);
    }
    //
    //    for (int i = 0; i < wm.opponentsFromBall().size() && i <= 3; i++) {
    //        if (wm.opponentsFromBall().at(i)->posCount() > 0)
    //            targs.push_back(make_pair(wm.opponentsFromBall().at(i)->pos(), 0.9));
    //    }

    for (int i = 0; i < wm.teammatesFromSelf().size() && i <= 2; i++) {
        if (wm.teammatesFromSelf().at(i)->posCount() > 0)
            targs.emplace_back(make_pair(wm.teammatesFromSelf().at(i)->pos(), 0.8));
    }
    if (opp->posCount() > 0)
        targs.emplace_back(make_pair(opp->pos(), 1.3));
    //    targs.emplace_back(make_pair(targetPoint, 0.7));
    //    targs.push_back(ball_iner);
    if (wm.interceptTable()->fastestOpponent() != NULL
            && wm.interceptTable()->fastestOpponent()->posCount() > 0) {
        targs.emplace_back(make_pair(wm.interceptTable()->fastestOpponent()->pos(), 1.1));
    }
    if (blocker != 0 && blocker != wm.self().unum() && wm.ourPlayer(blocker) != NULL &&
            wm.ourPlayer(blocker)->posCount() > 0)
        targs.emplace_back(make_pair(wm.ourPlayer(blocker)->pos(), 0.9));
    if (wm.ball().posCount() > 0)
        targs.emplace_back(make_pair(wm.ball().pos(), 1.2));

    vector<const AbstractPlayerObject *> myLine = Strategy::i().myLineTmms(wm, Strategy::i().tm_Line(wm.self().unum()));
    for (auto &i : myLine) {
        if (i->unum() != wm.self().unum() && i->posCount() > 2)
            targs.emplace_back(make_pair(i->pos(), 0.7));
    }
    if (ball_iner.x < -35 && wm.getOurGoalie() != NULL && wm.getOurGoalie()->posCount() > 2) {
        targs.emplace_back(make_pair(wm.getOurGoalie()->pos(), 0.6));
    }


    for (auto &targ : targs) {
        dlog.addText(Logger::TEAM,
                     __FILE__": def neck target=(%.1f %.1f) eval=%.2f", targ.first.x,
                     targ.first.y, targ.second);
    }


    double best_neck = -1000;
    double best_eval = 0;
    for (double neck = self_body.degree() - 90; neck <= self_body.degree() + 90; neck += 10) {
        AngleDeg min_see(neck - next_view_width / 2.0 + 10);
        AngleDeg max_see = (neck + next_view_width / 2.0 - 10);
        double eval = 0;
        if (should_see_ball) {
            if (can_see_ball) {
                if (ball_angle.isWithin(min_see, max_see)) {
                    for (auto &targ : targs) {
                        AngleDeg targ_angle = (targ.first - self_pos).th();
                        if (targ_angle.isWithin(min_see, max_see)) {
                            eval += (wm.dirCount(targ_angle) * targ.second);
                        }
                    }
                }
            } else {
                for (auto &targ : targs) {
                    AngleDeg targ_angle = (targ.first - self_pos).th();
                    if (targ_angle.isWithin(min_see, max_see)) {
                        eval += (wm.dirCount(targ_angle) * targ.second);
                    }
                }
            }
        } else {
            for (auto &targ : targs) {
                AngleDeg targ_angle = (targ.first - self_pos).th();
                if (targ_angle.isWithin(min_see, max_see)) {
                    eval += (wm.dirCount(targ_angle) * targ.second);
                }
            }
        }
        if (eval > best_eval) {
            best_eval = eval;
            best_neck = neck;
        }
    }
    dlog.addText(Logger::TEAM,
                 __FILE__": def neck %.1f eval=%.2f", best_neck, best_eval);
    if (best_eval != 0) {
        agent->setNeckAction(new Neck_TurnToPoint(self_pos + Vector2D::polar2vector(10, best_neck)));
    } else if (should_see_ball && can_see_ball) {
        agent->setNeckAction(new Neck_TurnToBall());
    } else {
        agent->setNeckAction(new Neck_TurnToBallOrScan(1));
    }
    return true;
}

bool Bhv_BasicMove::intercept_plan(rcsc::PlayerAgent *agent, bool from_block) {
    const WorldModel &wm = agent->world();
    /*--------------------------------------------------------*/
    // chase ball
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();

    auto intercept_tackle_info = bhv_tackle_intercept::intercept_cycle(wm);
    int cycle_intercept_tackle = intercept_tackle_info.first;

    {
        int self_tackle_min = wm.interceptTable()->selfReachCycleTackle();
        Vector2D balliner = wm.ball().inertiaPoint(self_min);
        Vector2D balliner_tackle = wm.ball().inertiaPoint(self_tackle_min);
        Vector2D balltackle = wm.ball().inertiaPoint(cycle_intercept_tackle);
        dlog.addCircle(Logger::INTERCEPT, balliner,1,0,0,0, true);
        dlog.addCircle(Logger::INTERCEPT, balliner_tackle,0.5,255,255,255, true);
        dlog.addCircle(Logger::INTERCEPT, balltackle,0.25,255,0,0, true);
    }

    Vector2D tm_catch_ball = wm.ball().inertiaPoint(std::min(self_min, mate_min));
    if (tm_catch_ball.x < -52.5) {
        if (wm.ball().vel().r() > 0.1) {
            Line2D ball_move = Line2D(wm.ball().pos(), wm.ball().vel().th());
            Line2D goal_line = Line2D(Vector2D(-52.5, 0), 90);
            Vector2D inters = ball_move.intersection(goal_line);
            if (inters.isValid()) {
                if (inters.absY() < 7.5) {
                    if (wm.ball().inertiaPoint(cycle_intercept_tackle).x > -54) {
                        if (bhv_tackle_intercept().execute(agent)) {
                            agent->debugClient().addMessage("tackle inter for prevent goal");
                            return true;
                        }
                    }
                }
            }
        }
    }
    Vector2D next_pass = Vector2D::INVALIDATED;
    Vector2D face = Vector2D(52, 0);
    if (self_min < opp_min + 1) {
        bool shoot_is_best = false;
        if(wm.ball().pos().dist(Vector2D(52,0)) < 20){
            Vector2D shoot_tar = Bhv_StrictCheckShoot(0).get_best_shoot(wm);
            if(shoot_tar.isValid()){
                next_pass = Vector2D(52, 0);
                face = shoot_tar;
                shoot_is_best = true;
            }
        }
        if(!shoot_is_best){
            const ActionChainGraph &chain_graph = ActionChainHolder::i().graph();
            const CooperativeAction &first_action = chain_graph.getFirstAction();
            switch (first_action.category()) {
            case CooperativeAction::Shoot: {
                next_pass = Vector2D(52, 0);
                face = first_action.targetPoint();
                break;
            }

            case CooperativeAction::Dribble: {
                face = first_action.targetPoint();
                break;
            }

            case CooperativeAction::Hold: {
                break;
            }

            case CooperativeAction::Pass: {
                face = first_action.targetPoint();
                next_pass = first_action.targetPoint();
                if (self_min == 1) {
                    Bhv_PassKickFindReceiver(chain_graph).doSayPrePass(agent, first_action);
                }
                break;
            }
            }
        }
    }
    bool tm_drible = false;
    bool tm_pass = false;

    if (wm.lastKickerSide() == wm.ourSide()) {
        int cycle_for_drible = 2;
        const int real_set_play_count = wm.time().cycle() - wm.lastSetPlayStartTime().cycle();
        if (wm.ball().pos().x > 45 && wm.ball().pos().absY() > 30)
            if (real_set_play_count < 100)
                cycle_for_drible = 2;
        if (!wm.audioMemory().dribble().empty()
                && wm.audioMemory().dribbleTime().cycle()
                > wm.time().cycle() - cycle_for_drible) {
            if (wm.audioMemory().pass().empty()
                    || (!wm.audioMemory().pass().empty()
                        && wm.audioMemory().passTime().cycle()
                        < wm.audioMemory().dribbleTime().cycle())) {
                if (opp_min > mate_min) {
                    if (wm.audioMemory().dribble().front().sender_
                            != wm.self().unum())
                        tm_drible = true;
                }
            }
        }
        int cycle_for_pass = 2;
        if (!wm.audioMemory().pass().empty()
                && wm.audioMemory().passTime().cycle()
                > wm.time().cycle() - cycle_for_pass) {
            if (opp_min > mate_min && mate_min - 3 < self_min) {
                if (wm.audioMemory().pass().front().receiver_
                        != wm.self().unum())
                    tm_pass = true;
            }
        }
    }
    Vector2D inertia_ball_pos = wm.ball().inertiaPoint(
                wm.interceptTable()->opponentReachCycle());
    int oppCycles = 0;
    vector<Vector2D> ball_cache = CyrusPlayerIntercept::createBallCache(wm);
    CyrusPlayerIntercept ourpredictor(wm, ball_cache);
    const PlayerObject *it = wm.interceptTable()->fastestOpponent();
    if (it != NULL && (*it).unum() >= 2) {
        const PlayerType *player_type = (*it).playerTypePtr();
        vector<CyrusOppInterceptTable> pred = ourpredictor.predict(*it, *player_type, 1000);
        CyrusOppInterceptTable tmp = CyrusPlayerIntercept::getBestIntercept(wm, pred);
        oppCycles = tmp.cycle;
        inertia_ball_pos = tmp.current_position;
        if (oppCycles > 100 || !inertia_ball_pos.isValid()) {
            oppCycles = wm.interceptTable()->opponentReachCycle();
            inertia_ball_pos = wm.ball().inertiaPoint(oppCycles);
        }
    }
    int dif = 2;
    if (opp_min < mate_min && self_min < mate_min && opp_min < self_min) {
        if (Strategy::i().self_Line() == Strategy::PostLine::forward
                && wm.ball().inertiaPoint(opp_min).x > 25) {
            dif = 5;
        }
    }

    bool use_tackle_intercept = false;
    if (wm.existKickableTeammate())
        return false;
    if (wm.ball().pos().x > -15 || wm.lastKickerSide() == wm.ourSide()) {
        if (!tm_drible && !tm_pass) {
            if (self_min <= 1) {
                if (!Body_Intercept2009(false, face).execute(agent)){
                    if (!Body_GoToPoint(wm.ball().inertiaPoint(1), 0.1, 100).execute(agent)){
                        Body_TurnToPoint(wm.ball().inertiaPoint(1)).execute(agent);
                    }
                }
                NeckDecisionWithBall().setNeck(agent);
                agent->debugClient().addMessage("Intercept->Z");
                return true;
            } else if (self_min <= 3) {
                if (self_min <= mate_min) {
                    if (self_min < opp_min + dif) {
                        if (self_min <= opp_min) {
                            if (!Body_Intercept2009(false, face).execute(agent)){
                                if (!Body_GoToPoint(wm.ball().inertiaPoint(self_min), 0.1, 100).execute(agent)){
                                    Body_TurnToPoint(wm.ball().inertiaPoint(1)).execute(agent);
                                }
                            }
                            agent->debugClient().addMessage("Intercept->A");
                        } else {
                            if (!from_block && bhv_block().execute(agent)) {
                                agent->debugClient().addMessage("Intercept->Block");
                                return true;
                            }
                            Vector2D opp_target = wm.ball().inertiaPoint(opp_min);
                            if (!Body_GoToPoint(opp_target, 0.5, 100).execute(agent)) {
                                Body_TurnToPoint(face).execute(agent);
                            }
                            agent->debugClient().addMessage("Intercept->B");
                        }
                        NeckDecisionWithBall().setNeck(agent);
                        return true;
                    } else {
                        use_tackle_intercept = true;
                    }
                }
            } else {
                if (self_min <= mate_min) {
                    if (self_min < opp_min + dif) {
                        if (self_min <= opp_min) {
                            if (!Body_Intercept2009(false, face).execute(agent)){
                                if (!Body_GoToPoint(wm.ball().inertiaPoint(1), 0.1, 100).execute(agent)){
                                    Body_TurnToPoint(wm.ball().inertiaPoint(1)).execute(agent);
                                }
                            }
                            agent->debugClient().addMessage("Intercept->C");
                        } else {
                            if (!from_block && bhv_block().execute(agent)) {
                                agent->debugClient().addMessage("Intercept->Block");
                                return true;
                            }
                            Vector2D opp_target = wm.ball().inertiaPoint(opp_min);
                            if (!Body_GoToPoint(opp_target, 0.5, 100).execute(agent)) {
                                Body_TurnToPoint(face).execute(agent);
                            }
                            agent->debugClient().addMessage("Intercept->D");

                        }
                        NeckDecisionWithBall().setNeck(agent);
                        return true;
                    } else {
                        use_tackle_intercept = true;
                    }
                }
            }

        }
    } else {
        if (self_min <= 1) {
            if (!Body_Intercept2009(false).execute(agent)){
                if (!Body_GoToPoint(wm.ball().inertiaPoint(1), 0.1, 100).execute(agent)){
                    Body_TurnToPoint(wm.ball().inertiaPoint(1)).execute(agent);
                }
            }
            NeckDecisionWithBall().setNeck(agent);
            agent->debugClient().addMessage("Intercept->Y");
            return true;
        } else if (self_min <= mate_min && self_min < opp_min + 3) {
            if (self_min <= opp_min) {
                if (!Body_Intercept2009(false).execute(agent)){
                    if (!Body_GoToPoint(wm.ball().inertiaPoint(1), 0.1, 100).execute(agent)){
                        Body_TurnToPoint(wm.ball().inertiaPoint(1)).execute(agent);
                    }
                }
                agent->debugClient().addMessage("Intercept->F");
            } else {
                if (!from_block && bhv_block().execute(agent)) {
                    agent->debugClient().addMessage("Intercept->Block");
                    return true;
                }
            }
            //            if (fabs((inertia_ball_pos - wm.self().pos()).th().degree()
            //                        - wm.self().body().degree()) > 20
            //                    && opp_min <= self_min) {
            //                double dir = (inertia_ball_pos - wm.self().pos()).th().degree() - wm.self().body().degree();

            //                agent->doDash(100, dir);
            //                agent->debugClient().addMessage("Intercept->E");
            //            } else{
            //                Body_Intercept2009(false).execute(agent);
            //                agent->debugClient().addMessage("Intercept->F");
            //            }
            NeckDecisionWithBall().setNeck(agent);
            return true;
        } else if (self_min <= mate_min) {
            use_tackle_intercept = true;
        }
    }

    //    if(use_tackle_intercept){
    int diff = 2;
    if (wm.ball().inertiaPoint(oppCycles).x > 35 && wm.ball().inertiaPoint(oppCycles).absY() < 20)
        diff = 0;
    agent->debugClient().addMessage("tc:%d", cycle_intercept_tackle);
    int self_min_tackle = wm.interceptTable()->selfReachCycle();

    if (self_min_tackle < 10 && self_min_tackle < opp_min - diff &&
            self_min_tackle < mate_min - diff) {
        agent->debugClient().addMessage("TackleIntercept");
        return Body_Intercept2009(false).executeTackle(agent);
    }
    if (cycle_intercept_tackle != 1000 && cycle_intercept_tackle < opp_min - diff &&
            cycle_intercept_tackle < mate_min - diff) {
        agent->debugClient().addMessage("Intercept->Tackle");
        return bhv_tackle_intercept().execute(agent);
    }
    //    }
    return false;

}

void Bhv_BasicMove::offense_set_neck_action(PlayerAgent *agent) {
    const WorldModel &wm = agent->world();
    if (wm.existKickableOpponent()
            && wm.ball().distFromSelf() < 18.0) {
        agent->setNeckAction(new Neck_TurnToBall());
    } else {
        agent->setNeckAction(new Neck_TurnToBallOrScan());
    }
    return;


    //	const WorldModel & wm = agent->world();
    if (wm.ball().posCount() >= 3) {
        agent->setNeckAction(new Neck_TurnToBall());
    } else {
        agent->setNeckAction(new Neck_TurnToBallOrScan(2));
    }
}

bool
Bhv_BasicMove::execute(PlayerAgent *agent) {

    //    if(ServerParam::i().synchMode() == true){
    //        std::this_thread::sleep_for(std::chrono::seconds(1));
    //        std::cout<<"Deep Learning for Move is Already RUN"<<std::endl;
    //    }
    dlog.addText(Logger::TEAM,
                 __FILE__": Bhv_BasicMove");
    const WorldModel &wm = agent->world();
    //-----------------------------------------------
    // tackle


    if (Bhv_BasicTackle(Bhv_BasicTackle::calc_takle_prob(wm)).execute(agent)) {
        return true;
    }

    /*--------------------------------------------------------*/
    // chase ball
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();
    Vector2D ball_iner = wm.ball().inertiaPoint(std::min(std::min(opp_min, mate_min), self_min));
    int unum = wm.self().unum();
    int stamina = wm.self().stamina();
    Vector2D target_point = Strategy::i().getPosition(wm.self().unum());
    double dash_power = Strategy::get_normal_dash_power(wm);
    Vector2D self_pos = wm.self().pos();


    if (intercept_plan(agent)) {
        return true;
    }
    if (TurnToTackle(agent)) {
        return true;
    }

    bool can_5_go_forward = true;
    if(wm.self().unum() == 5){
        if (!Strategy::i().isDefSit(wm, wm.self().unum())){
            if(ball_iner.x > 0 && self_pos.x < 0){
                if(stamina < 5500){
                    can_5_go_forward = false;
                    target_point.x = std::min(target_point.x, 0.0);
                }
            }
        }
    }
    if(wm.self().unum() < 5){
        if (!Strategy::i().isDefSit(wm, wm.self().unum())){
            if(ball_iner.x > 0 && self_pos.x < 0){
                if(stamina < 6000){
                    target_point.x = std::min(target_point.x, -1.0);
                }
            }
        }
    }
    if(wm.self().unum() == 6){
        if(wm.ourPlayer(5)!= nullptr && wm.ourPlayer(5)->unum() > 0){
            if(Strategy::i().getPosition(5).dist(wm.ourPlayer(5)->pos())>10){
                if(ball_iner.x > 20){
                    if(!Strategy::i().isDefSit(wm, wm.self().unum())){
                        Strategy::i().set_position(6, (Strategy::i().getPosition(5) + target_point)/ 2.0);
                        target_point = Strategy::i().getPosition(wm.self().unum());
                    }
                }
            }
        }
    }
    if (Strategy::i().isDefSit(wm, wm.self().unum()) ||
            (Strategy::i().tm_Line(wm.self().unum()) == Strategy::i().PostLine::back && wm.ball().inertiaPoint(opp_min).x > 30)) {
        if (DefSitPlan(agent))
            return true;
    } else {
        if(can_5_go_forward)
            if (cyrus_offensive_move().execute(agent, this)) {
                return true;
            }
    }


    double dist_thr = wm.ball().distFromSelf() * 0.1;
    if (dist_thr < 1.0) dist_thr = 1.0;

    dlog.addText(Logger::TEAM,
                 __FILE__": Bhv_BasicMove target=(%.1f %.1f) dist_thr=%.2f",
                 target_point.x, target_point.y,
                 dist_thr);

    agent->debugClient().addMessage("BasicMove%.0f", dash_power);
    agent->debugClient().setTarget(target_point);
    agent->debugClient().addCircle(target_point, dist_thr);

    if (!Body_GoToPoint(target_point, dist_thr, dash_power
                        ).execute(agent)) {
        Body_TurnToBall().execute(agent);
    }

    if (wm.existKickableOpponent()
            && wm.ball().distFromSelf() < 18.0) {
        agent->setNeckAction(new Neck_TurnToBall());
    } else {
        agent->setNeckAction(new Neck_TurnToBallOrScan());
    }

    return true;
}

bool Bhv_BasicMove::TurnToTackle(rcsc::PlayerAgent *agent) {
    const WorldModel &wm = agent->world();
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int self_min_tackle = wm.interceptTable()->selfReachCycleTackle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();

    if (mate_min <= opp_min)
        return false;
    if (self_min <= opp_min)
        return false;
    if (self_min <= 2)
        return false;
    if(self_min_tackle < opp_min && self_min_tackle < mate_min)
    {
        Body_Intercept2009(false).executeTackle(agent);
        agent->setNeckAction(new Neck_TurnToBall());
        agent->debugClient().addMessage("tackle intercept execute");
        return true;
    }
    Vector2D next_ball = wm.ball().inertiaPoint(1);
    if (next_ball.dist(wm.self().inertiaPoint(1)) < 1.7) {
        Body_TurnToPoint(next_ball).execute(agent);
        agent->setNeckAction(new Neck_TurnToBall());
        agent->debugClient().addMessage("turn to tackle");
        return true;
    }
    return false;
}

bool can_opp_shoot_to_goal(const WorldModel &wm) {
    int opp_min = wm.interceptTable()->opponentReachCycle();
    Vector2D ballpos = wm.ball().inertiaPoint(opp_min);
    if (!ServerParam::i().ourPenaltyArea().contains(ballpos))
        return false;
    bool twostepshoot = false;
    if (opp_min > 0 && wm.ball().vel().r() * std::pow(ServerParam::i().ballDecay(), opp_min) > 0.5) {
        AngleDeg opp2goal = (ballpos - Vector2D(-52, 0)).th();
        AngleDeg opp2ball = (ballpos - wm.ball().pos()).th();
        if ((opp2goal - opp2ball).abs() < 9)
            twostepshoot = true;
    }
    bool can_goal = false;
    for (double y = -6.5; y < 6.6; y += 0.5) {
        Vector2D target(-52.5, y);
        Vector2D ball_vel_sim = Vector2D::polar2vector(2.9, (target - ballpos).th());
        Vector2D ball_pos_sim = ballpos;
        bool can_intercept = false;
        for (int c = 1; c <= 30; c++) {
            ball_pos_sim += ball_vel_sim;
            if (ball_pos_sim.x < -52.5)
                can_goal = true;
            for (int t = 1; t <= 5; t++) {
                const AbstractPlayerObject *tm = wm.ourPlayer(t);
                if (tm == NULL || tm->unum() != t)
                    continue;
                if (tm->pos().dist(ball_pos_sim) / tm->playerTypePtr()->realSpeedMax() > c + 2)
                    continue;
                int dc;
                int dt;
                int dv;
                int intercept_cycle = tm->cycles_to_cut_ball(wm, ball_pos_sim, c, true, dc, dt, dv);
                if (intercept_cycle <= c) {
                    can_intercept = true;
                    break;
                }
            }
            if (can_intercept)
                break;
            ball_vel_sim *= ServerParam::i().ballDecay();
        }
        if (can_intercept) {
            dlog.addCircle(Logger::SHOOT, target, 0.2, 0, 255, 0, true);
        } else {
            dlog.addCircle(Logger::SHOOT, target, 0.2, 255, 0, 0, true);
        }
    }
    return can_goal;
}


vector<int> Bhv_BasicMove::who_goto_goal(PlayerAgent *agent) {
    const WorldModel &wm = agent->world();
    vector<int> results;
    int opp_min = wm.interceptTable()->opponentReachCycle();
    Vector2D ballpos = wm.ball().inertiaPoint(opp_min);
    if (can_opp_shoot_to_goal(wm)) {
        double min_dist = 1000;
        Vector2D tar;
        for (int i = 2; i <= 5; i++) {
            if (ballpos.y > 5) {
                tar = Vector2D(-52, -4);
            } else if (ballpos.y < -5) {
                tar = Vector2D(-52, +4);
            } else {
                if (Strategy::i().getPosition(i).y > 0) {
                    tar = Vector2D(-52, +5);
                } else {
                    tar = Vector2D(-52, -5);
                }
            }
            int min_unum = 0;
            for (int t = 2; t <= 5; t++) {
                const AbstractPlayerObject *tm = wm.ourPlayer(t);
                if (tm == NULL || tm->unum() != t)
                    continue;
                if (tm->pos().dist(wm.ball().inertiaPoint(wm.interceptTable()->opponentReachCycle())) < 3)
                    continue;
                Vector2D pos = tm->pos();
                double dist = pos.dist(tar);
                if (dist < min_dist) {
                    min_dist = dist;
                    min_unum = t;
                }
            }
            if (min_unum == i)
                results.push_back(i);
        }
    }
    return results;
}


bool go_to_goal(PlayerAgent *agent) {
    const WorldModel &wm = agent->world();
    int opp_min = wm.interceptTable()->opponentReachCycle();
    Vector2D ballpos = wm.ball().inertiaPoint(opp_min);
    if (wm.self().pos().dist(wm.ball().inertiaPoint(wm.interceptTable()->opponentReachCycle())) < 3)
        return false;
    if (can_opp_shoot_to_goal(wm)) {
        double min_dist = 1000;
        int min_unum = 0;
        Vector2D tar;
        if (ballpos.y > 5) {
            tar = Vector2D(-52, -4);
        } else if (ballpos.y < -5) {
            tar = Vector2D(-52, +4);
        } else {
            if (Strategy::i().getPosition(wm.self().unum()).y > 0) {
                tar = Vector2D(-52, +5);
            } else {
                tar = Vector2D(-52, -5);
            }
        }
        for (int t = 2; t <= 5; t++) {
            const AbstractPlayerObject *tm = wm.ourPlayer(t);
            if (tm == NULL || tm->unum() != t)
                continue;
            Vector2D pos = tm->pos();
            double dist = pos.dist(tar);
            if (dist < min_dist) {
                min_dist = dist;
                min_unum = t;
            }
        }

        if (min_unum == wm.self().unum()) {
            Vector2D self_pos = wm.self().pos();

            if (ballpos.y > 5) {
                if (ballpos.x < -42) {
                    tar = Vector2D(-51, 0);
                } else if (ballpos.x < -39) {
                    tar = Vector2D(-51, +4);
                }
            } else if (ballpos.y < -5) {
                if (ballpos.x < -42) {
                    tar = Vector2D(-51, 0);
                } else if (ballpos.x < -39) {
                    tar = Vector2D(-52, -4);
                }
            } else {
                if (ballpos.x < -42) {
                    if (tar.y < 0)
                        tar = Vector2D(-51, -4);
                    else
                        tar = Vector2D(-51, +4);
                } else if (ballpos.x < -39) {
                    if (tar.y < 0)
                        tar = Vector2D(-51, -5);
                    else
                        tar = Vector2D(-51, +5);
                }
            }

            if (!Body_GoToPoint(tar, 1.5, 100, 2, 1, false, 20).execute(agent)) {
                if (self_pos.dist(tar) < 1.0 && wm.interceptTable()->opponentReachCycle() <= 1 &&
                        wm.self().body().abs() > 80 && wm.self().body().abs() < 100) {
                    agent->doDash(100, (tar - wm.self().pos()).th() - wm.self().body());
                } else {
                    if (ballpos.y > wm.self().pos().y) {
                        Body_TurnToAngle(90).execute(agent);
                    } else {
                        Body_TurnToAngle(-90).execute(agent);
                    }
                }
            }
            agent->debugClient().addMessage("goto goal");
            if (wm.existKickableOpponent() && wm.ball().distFromSelf() < 18.0 || opp_min <= 2) {
                agent->setNeckAction(new Neck_TurnToBall());
            } else {
                agent->setNeckAction(new Neck_TurnToBallOrScan());
            }
            return true;
        }
    }
    return false;
}

bool Bhv_BasicMove::DefSitPlan(rcsc::PlayerAgent *agent) {

    const WorldModel &wm = agent->world();
    /*--------------------------------------------------------*/
    // chase ball
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();

    //	cout<<"*****"<<wm.time().cycle()<<endl;
    if (wm.interceptTable()->fastestOpponent() == NULL || wm.interceptTable()->fastestOpponent()->unum() < 1)
        return false;

    bool mark_or_block = true;
    if (Strategy::i().self_Line() == Strategy::PostLine::forward) {

        if (wm.ball().pos().x < -20) {
            if (wm.self().stamina() < 4000) {
                mark_or_block = false;
            }
        } else if (wm.self().stamina() < 5000) {
            mark_or_block = false;
        }
    }
    if (Strategy::i().self_Line() == Strategy::PostLine::back) {
        if (go_to_goal(agent)){
            agent->debugClient().addMessage("GoToGoal");
            return true;
        }
    }
    if (mark_or_block) {
        dlog.addText(Logger::BLOCK, "block or mark");
        if (bhv_mark_execute().execute(agent)){
            agent->debugClient().addMessage("MarkEx");
            return true;
        }
    }

    double min_x_hpos = 1000;
    for (int i = 2; i <= 11; i++) {
        double hposx = Strategy::i().getPosition(i).x;
        if (hposx < min_x_hpos)
            min_x_hpos = hposx;
    }

    Vector2D inertia_ball_pos = wm.ball().inertiaPoint(
                wm.interceptTable()->opponentReachCycle());
    double mark_x_line = std::max(wm.ourDefenseLineX(), min_x_hpos);

    Vector2D target_point = Strategy::i().getPosition(wm.self().unum());

    if (wm.audioMemory().waitRequest().size() > 0) {
        if (wm.audioMemory().waitRequestTime().cycle() > wm.time().cycle() - 4) {
            if (Strategy::i().tm_Line(wm.self().unum()) == Strategy::i().PostLine::back) {
                if (wm.audioMemory().waitRequest().front().sender_ != wm.self().unum()) {
                    const AbstractPlayerObject *tm = wm.ourPlayer(wm.audioMemory().waitRequest().front().sender_);
                    if (tm != NULL && tm->unum() > 1) {
                        target_point.x = tm->pos().x;
                    }
                }
            }
        }
    }
    //    if (wm.opponentTeamName().compare("KN2C") == 0) {
    //        int our_score = (wm.ourSide() == LEFT
    //                         ? wm.gameMode().scoreLeft()
    //                         : wm.gameMode().scoreRight());
    //        int opp_score = (wm.ourSide() == LEFT
    //                         ? wm.gameMode().scoreRight()
    //                         : wm.gameMode().scoreLeft());
    //        if (opp_score > our_score) {
    //            vector<int> unums;
    //            unums.push_back(0);
    //            unums.push_back(0);
    //            unums.push_back(11);
    //            unums.push_back(10);
    //            unums.push_back(9);
    //            unums.push_back(8);
    //            unums.push_back(7);
    //            unums.push_back(5);
    //            unums.push_back(4);
    //            unums.push_back(3);
    //            unums.push_back(2);
    //            unums.push_back(6);
    //            int u = unums.at(wm.self().unum());
    //            if (wm.theirPlayer(u) != NULL && wm.theirPlayer(u)->unum() == u) {
    //                target_point = wm.theirPlayer(u)->pos() - Vector2D(1, 0);
    //            }
    //        }
    //    }

    double min_x_strategy = 100;
    for (int i = 2; i <= 11; i++) {
        double x = Strategy::i().getPosition(i).x;
        if (x < min_x_strategy)
            min_x_strategy = x;
    }

    double dash_power = Strategy::get_normal_dash_power(wm);
    if (wm.ball().pos().x < wm.self().pos().x
            && Strategy::i().self_Line() == Strategy::PostLine::back) {
        //		target_point.x = min(-42.0, inertia_ball_pos.x);
        dash_power = 100;
    }
    /*if (wm.ourDefenseLineX() < target_point.x && wm.self().unum() < 6) {
     target_point.x = wm.ourDefenseLineX();
     dash_power = 100;
     }*/
    if (wm.self().pos().x < wm.ourDefenseLineX()
            && wm.ball().pos().x > wm.ourDefenseLineX()) {
        //		target_point.x = (wm.ourDefenseLineX() - 1);
    }
    if (Strategy::i().self_Line() == Strategy::PostLine::back || Strategy::i().self_Line() == Strategy::PostLine::half) {
        if (wm.ball().inertiaPoint(opp_min).x < -20
                && (wm.self().pos().dist(target_point) > 4
                    || wm.self().pos().x > target_point.x + 2)) {
            dash_power = 100;
        }
    }

    if (wm.self().pos().x < min_x_strategy)
        dash_power = 100;
    if (Strategy::i().self_line == Strategy::PostLine::back && abs(inertia_ball_pos.x - wm.ourDefenseLineX()) < 20) {
        dash_power = 100;
    }
    double dist_thr = wm.ball().distFromSelf() * 0.1;
    if (dist_thr < 1.0)
        dist_thr = 1.0;

    if(wm.self().unum() < 5){
        if(inertia_ball_pos.x > 0 && wm.self().pos().x < 0){
            if(wm.self().stamina() < 6000){
                target_point.x = std::min(target_point.x, -1.0);
            }
        }
    }
    dlog.addText(Logger::TEAM,
                 __FILE__": Bhv_BasicMove target=(%.1f %.1f) dist_thr=%.2f", target_point.x,
                 target_point.y, dist_thr);

    agent->debugClient().addMessage("BasicMoveDef%.0f", dash_power);
    agent->debugClient().setTarget(target_point);
    agent->debugClient().addCircle(target_point, dist_thr);

    if (wm.self().stamina() < 4500 && wm.self().pos().x < Strategy::i().getPosition(2).x) {
        agent->addSayMessage(new WaitRequestMessage());
    }
    if (!Body_GoToPoint(target_point, dist_thr, dash_power).execute(agent)) {
        Body_TurnToPoint(target_point).execute(agent);
    }
    AbstractPlayerObject *nearest_opp = wm.opponentsFromSelf().at(0);
    if (nearest_opp != NULL)
        set_def_neck_with_ball(agent, wm.ball().pos(), nearest_opp, 0);
    else {
        if (wm.existKickableOpponent() && wm.ball().distFromSelf() < 18.0) {
            agent->setNeckAction(new Neck_TurnToBall());
        } else {
            agent->setNeckAction(new Neck_TurnToBallOrScan());
        }
    }
    return true;
}


#include "sample_communication.h"
