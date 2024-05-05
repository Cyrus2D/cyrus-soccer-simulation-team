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

#include <thread>
#include <chrono>
#include <rcsc/player/player_agent.h>
#include <rcsc/common/logger.h>
#include <rcsc/player/cut_ball_calculator.h>

#include "basic_actions/basic_actions.h"
#include "basic_actions/body_go_to_point.h"
#include "basic_actions/body_intercept2009.h"
#include "basic_actions/neck_turn_to_ball_or_scan.h"
#include "move_def/basic_defensive_move.h"
#include "move_def/bhv_basic_tackle.h"
#include "move_def/bhv_block.h"
#include "move_def/bhv_tackle_intercept.h"
#include "move_def/cyrus_interceptable.h"
#include "move_off/bhv_offensive_move.h"
#include "chain_action/bhv_pass_kick_find_receiver.h"
#include "chain_action/action_chain_holder.h"
#include "chain_action/shoot_generator.h"
#include "chain_action/bhv_strict_check_shoot.h"
#include "neck/neck_decision.h"
#include "strategy.h"

#define DEBUG_PRINT

using namespace rcsc;

bool
Bhv_BasicMove::execute(PlayerAgent *agent) {

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
    const int self_min = wm.interceptTable().selfStep();
    const int mate_min = wm.interceptTable().teammateStep();
    const int opp_min = wm.interceptTable().opponentStep();
    Vector2D ball_inertia = wm.ball().inertiaPoint(std::min(std::min(opp_min, mate_min), self_min));
    double dash_power = Strategy::get_normal_dash_power(wm);
    Vector2D self_pos = wm.self().pos();

    if (intercept_plan(agent))
        return true;

    if (TurnToTackle(agent))
        return true;

    update_target_points(agent);

    if (Bhv_DefensiveMove().execute(agent))
        return true;

    if (cyrus_offensive_move().execute(agent, this))
        return true;

    Vector2D target_point = get_target_point(agent);
    double dist_thr = wm.ball().distFromSelf() * 0.1;
    if (dist_thr < 1.0) dist_thr = 1.0;

    dlog.addText(Logger::TEAM,
                 __FILE__": Bhv_BasicMove target=(%.1f %.1f) dist_thr=%.2f",
                 target_point.x, target_point.y,
                 dist_thr);

    agent->debugClient().addMessage("BasicMove%.0f", dash_power);
    agent->debugClient().setTarget(target_point);
    agent->debugClient().addCircle(target_point, dist_thr);

    if (!Body_GoToPoint(target_point, dist_thr, dash_power).execute(agent))
        Body_TurnToBall().execute(agent);

    if (wm.kickableOpponent() &&
        wm.ball().distFromSelf() < 18.0)
    {
        agent->setNeckAction(new Neck_TurnToBall());
    }
    else
    {
        agent->setNeckAction(new Neck_TurnToBallOrScan(0));
    }

    return true;
}

bool Bhv_BasicMove::intercept_plan(rcsc::PlayerAgent *agent, bool from_block) {
    const WorldModel &wm = agent->world();
    /*--------------------------------------------------------*/
    // chase ball
    const int self_min = wm.interceptTable().selfStep();
    const int mate_min = wm.interceptTable().teammateStep();
    const int opp_min = wm.interceptTable().opponentStep();

    auto intercept_tackle_info = bhv_tackle_intercept::intercept_cycle(wm);
    int cycle_intercept_tackle = intercept_tackle_info.first;

    {
        int self_tackle_min = wm.interceptTable().selfStepTackle();
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
            if (!chain_graph.getAllChain().empty()){
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
                wm.interceptTable().opponentStep());
    int oppCycles = 0;
    vector<Vector2D> ball_cache = CyrusPlayerIntercept::createBallCache(wm);
    CyrusPlayerIntercept ourpredictor(wm, ball_cache);
    const PlayerObject *it = wm.interceptTable().firstOpponent();
    if (it != NULL && (*it).unum() >= 2) {
        const PlayerType *player_type = (*it).playerTypePtr();
        vector<CyrusOppInterceptTable> pred = ourpredictor.predict(*it, *player_type, 1000);
        CyrusOppInterceptTable tmp = CyrusPlayerIntercept::getBestIntercept(wm, pred);
        oppCycles = tmp.cycle;
        inertia_ball_pos = tmp.current_position;
        if (oppCycles > 100 || !inertia_ball_pos.isValid()) {
            oppCycles = wm.interceptTable().opponentStep();
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
    if (wm.kickableTeammate())
        return false;
    if (wm.ball().pos().x > -15 || wm.lastKickerSide() == wm.ourSide()) {
        if (!tm_drible && !tm_pass) {
            if (self_min <= 1) {
                if (!Body_Intercept2009(false, face).execute(agent)){
                    if (!Body_GoToPoint(wm.ball().inertiaPoint(1), 0.1, 100).execute(agent)){
                        Body_TurnToPoint(wm.ball().inertiaPoint(1)).execute(agent);
                    }
                }
                NeckDecisionWithBall().setNeck(agent, NeckDecisionType::intercept);
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
                        NeckDecisionWithBall().setNeck(agent, NeckDecisionType::intercept);
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
                        NeckDecisionWithBall().setNeck(agent, NeckDecisionType::intercept);
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
            NeckDecisionWithBall().setNeck(agent, NeckDecisionType::intercept);
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
            //                Body_Intercept2022(false).execute(agent);
            //                agent->debugClient().addMessage("Intercept->F");
            //            }
            NeckDecisionWithBall().setNeck(agent, NeckDecisionType::intercept);
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
     int self_min_tackle = wm.interceptTable().selfStep();
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

bool Bhv_BasicMove::TurnToTackle(rcsc::PlayerAgent *agent) {
    const WorldModel &wm = agent->world();
    const int self_min = wm.interceptTable().selfStep();
     const int self_min_tackle = wm.interceptTable().selfStepTackle();
    const int mate_min = wm.interceptTable().teammateStep();
    const int opp_min = wm.interceptTable().opponentStep();

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

void Bhv_BasicMove::update_target_points(rcsc::PlayerAgent * agent){
    const WorldModel &wm = agent->world();
    // chase ball
    const int self_min = wm.interceptTable().selfStep();
    const int mate_min = wm.interceptTable().teammateStep();
    const int opp_min = wm.interceptTable().opponentStep();
    Vector2D ball_inertia = wm.ball().inertiaPoint(std::min(std::min(opp_min, mate_min), self_min));
    Vector2D target_point = Strategy::i().getPosition(wm.self().unum());
    if (Strategy::i().get_formation_type() == Strategy::FormationType::F433){
        if(wm.self().unum() == 6){
            if(wm.ourPlayer(5)!= nullptr && wm.ourPlayer(5)->unum() > 0){
                if(Strategy::i().getPosition(5).dist(wm.ourPlayer(5)->pos())>10){
                    if(ball_inertia.x > 20){
                        if(!Strategy::i().isDefSit(wm, wm.self().unum())){
                            Strategy::i().set_position(6, (Strategy::i().getPosition(5) + target_point)/ 2.0);
                        }
                    }
                }
            }
        }
        // const int self_min = wm.interceptTable().selfStep();
        // const int mate_min = wm.interceptTable().teammateStep();
        // const int opp_min = wm.interceptTable().opponentStep();
        // Vector2D ball_inertia = wm.ball().inertiaPoint(std::min(std::min(opp_min, mate_min), self_min));
        // if (Strategy::i().get_formation_type() == Strategy::FormationType::F433){
        //     for (const AbstractPlayerObject * p : wm.ourPlayers()){
        //         if (p == nullptr) continue;
        //         if (p->unum() < 1) continue;
        //         int num = p->unum();
        //         double stamina = p->seenStamina();
        //         Vector2D pos = p->pos();
        //         Vector2D home_pos = Strategy::i().getPosition(p->unum());
        //         if(num < 5)
        //         {
        //             if (!Strategy::i().isDefSit(wm, num))
        //                 if(ball_inertia.x > 0 && pos.x < 0)
        //                     if(stamina < 6000)
        //                         Strategy::i().set_position(num, Vector2D(std::min(home_pos.x, -1.0), home_pos.y));
        //         }
        //         else if(num == 5)
        //         {
        //             if (!Strategy::i().isDefSit(wm, num))
        //                 if(ball_inertia.x > 0 && pos.x < 0)
        //                     if(stamina < 5500)
        //                         Strategy::i().set_position(num, Vector2D(std::min(home_pos.x, 0.0), home_pos.y));
        //         }
        //         else if(num == 6){
        //             bool tm5_exist = wm.ourPlayer(5)!= nullptr && wm.ourPlayer(5)->unum() > 0;
        //             if(tm5_exist)
        //                 if(Strategy::i().getPosition(5).dist(wm.ourPlayer(5)->pos())>10)
        //                     if(ball_inertia.x > 20)
        //                         if(!Strategy::i().isDefSit(wm, num))
        //                             Strategy::i().set_position(num, (Strategy::i().getPosition(5) + home_pos)/ 2.0);
        //         }
        //     }
        // }
    }
}

Vector2D Bhv_BasicMove::get_target_point(rcsc::PlayerAgent * agent){
    const WorldModel &wm = agent->world();
    // chase ball
    const int self_min = wm.interceptTable().selfStep();
    const int mate_min = wm.interceptTable().teammateStep();
    const int opp_min = wm.interceptTable().opponentStep();
    Vector2D ball_inertia = wm.ball().inertiaPoint(std::min(std::min(opp_min, mate_min), self_min));
    Vector2D self_pos = wm.self().pos();
    double stamina = wm.self().stamina();
    Vector2D target_point = Strategy::i().getPosition(wm.self().unum());
    if(wm.self().unum() == 5){
        if (!Strategy::i().isDefSit(wm, wm.self().unum())){
            if(ball_inertia.x > 0 && self_pos.x < 0){
                if(stamina < 5500){
                    target_point.x = std::min(target_point.x, 0.0);
                }
            }
        }
    }
    if(wm.self().unum() < 5){
        if (!Strategy::i().isDefSit(wm, wm.self().unum())){
            if(ball_inertia.x > 0 && self_pos.x < 0){
                if(stamina < 6000){
                    target_point.x = std::min(target_point.x, -1.0);
                }
            }
        }
    }
    return target_point;
}