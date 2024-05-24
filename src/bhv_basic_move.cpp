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
#include <thread>

#include <rcsc/player/player_agent.h>
#include <rcsc/common/logger.h>
#include <rcsc/player/cut_ball_calculator.h>

#include "bhv_basic_move.h"
#include "strategy.h"
#include "neck/neck_decision.h"
#include "move_def/bhv_mark_execute.h"
#include "move_def/bhv_basic_tackle.h"
#include "move_def/bhv_defensive_move.h"
#include "move_off/bhv_offensive_move.h"
#include "chain_action/action_chain_holder.h"
#include "basic_actions/basic_actions.h"
#include "basic_actions/body_go_to_point.h"
#include "basic_actions/neck_turn_to_ball_or_scan.h"
#include "basic_actions/body_turn_to_tackle.h"
#include "basic_actions/body_intercept_plan.h"
#include "chain_action/shoot_generator.h"


#define DEBUG_PRINT
// #define DEBUG_PRINT_INTERCEPT_LIST

#define USE_GOALIE_MODE

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
    Vector2D target_point = Strategy::i().getPosition(wm.self().unum());
    double dash_power = Strategy::getNormalDashPower(wm);
    Vector2D self_pos = wm.self().pos();


    if (Body_InterceptPlan().execute(agent)) {
        return true;
    }
    if (Body_TurnToTackle().execute(agent)) {
        return true;
    }

    bool can_5_join_forward = true;
    updateTarget(wm, target_point, can_5_join_forward);

    if (Strategy::i().isDefenseSituation(wm, wm.self().unum()) ||
        (Strategy::i().tmLine(wm.self().unum()) == PostLine::back && wm.ball().inertiaPoint(opp_min).x > 30)) {
        if (Bhv_DefensiveMove().execute(agent))
            return true;
    } else {
        if(can_5_join_forward)
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

    if (wm.kickableOpponent()
            && wm.ball().distFromSelf() < 18.0) {
        agent->setNeckAction(new Neck_TurnToBall());
    } else {
        agent->setNeckAction(new Neck_TurnToBallOrScan(0));
    }

    return true;
}

void Bhv_BasicMove::updateTarget(const rcsc::WorldModel & wm, rcsc::Vector2D & target_point, bool & can_5_join_offense) {
    // chase ball
    const int self_min = wm.interceptTable().selfStep();
    const int mate_min = wm.interceptTable().teammateStep();
    const int opp_min = wm.interceptTable().opponentStep();
    Vector2D ball_inertia = wm.ball().inertiaPoint(std::min(std::min(opp_min, mate_min), self_min));
    double stamina = wm.self().stamina();
    Vector2D self_pos = wm.self().pos();
    if(wm.self().unum() == 5){
        if (!Strategy::i().isDefenseSituation(wm, wm.self().unum())){
            if(ball_inertia.x > 0 && self_pos.x < 0){
                if(stamina < 5500){
                    can_5_join_offense = false;
                    target_point.x = std::min(target_point.x, 0.0);
                }
            }
        }
    }
    if(wm.self().unum() < 5){
        if (!Strategy::i().isDefenseSituation(wm, wm.self().unum())){
            if(ball_inertia.x > 0 && self_pos.x < 0){
                if(stamina < 6000){
                    target_point.x = std::min(target_point.x, -1.0);
                }
            }
        }
    }
    if(wm.self().unum() == 6){
        if(wm.ourPlayer(5)!= nullptr && wm.ourPlayer(5)->unum() > 0){
            if(Strategy::i().getPosition(5).dist(wm.ourPlayer(5)->pos())>10){
                if(ball_inertia.x > 20){
                    if(!Strategy::i().isDefenseSituation(wm, wm.self().unum())){
                        Strategy::i().setPosition(6, (Strategy::i().getPosition(5) + target_point) / 2.0);
                        target_point = Strategy::i().getPosition(wm.self().unum());
                    }
                }
            }
        }
    }
}
