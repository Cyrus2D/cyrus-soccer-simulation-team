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

#include "role_goalie.h"
#include "../move_def/bhv_basic_tackle.h"

#include "../goalie/bhv_goalie_basic_move.h"
#include "../goalie/bhv_goalie_chase_ball.h"
#include "../goalie/bhv_goalie_free_kick.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/body_clear_ball.h>
#include <rcsc/action/body_intercept.h>
#include <rcsc/common/audio_memory.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/player/world_model.h>
#include "../neck/neck_offensive_intercept_neck.h"

#include "../strategy.h"
#include "role_player.h"
#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/rect_2d.h>
#include <rcsc/geom/ray_2d.h>
#include <rcsc/player/intercept_table.h>
#include "../chain_action/bhv_chain_action.h"
using namespace rcsc;

const std::string RoleGoalie::NAME("Goalie");

/*-------------------------------------------------------------------*/
/*!

 */
namespace {
rcss::RegHolder role = SoccerRole::creators().autoReg(&RoleGoalie::create,
		RoleGoalie::name());
}

/*-------------------------------------------------------------------*/
/*!

 */
bool RoleGoalie::execute(PlayerAgent * agent) {
	const WorldModel & wm = agent->world();

	static const Rect2D our_penalty(Vector2D(-54, -19.98),
			Vector2D(-36.02, 19.98));

	const PlayerPtrCont & opps = wm.opponentsFromSelf();
	const PlayerObject * nearest_opp = (
			opps.empty() ? static_cast<PlayerObject *>(0) : opps.front());
	const double nearest_opp_dist = (
			nearest_opp ? nearest_opp->distFromSelf() : 1000.0);
	const Vector2D nearest_opp_pos = (
			nearest_opp ? nearest_opp->pos() : Vector2D(-1000.0, 0.0));

	const rcsc::PlayerType * player_type = wm.self().playerTypePtr();
	float kickableArea = player_type->kickableArea();

	static bool kicking = false;

	//////////////////////////////////////////////////////////////
	// play_on play

	static bool isDanger = false;
	static int dangerCycle = 0;

	rcsc::Vector2D ball = wm.ball().pos();
	rcsc::Vector2D prevBall = ball + wm.ball().rposPrev();
	rcsc::Vector2D nextBall = ball + wm.ball().vel();

	if (wm.lastKickerSide() == wm.ourSide()) {
		isDanger = true;
		dangerCycle = wm.time().cycle();
	}

    if ( wm.audioMemory().passTime() == wm.time()
         && !wm.audioMemory().pass().empty())
    {
        isDanger = true;
        dangerCycle = wm.time().cycle();
    }

	if (isDanger) {
		if (wm.gameMode().type() != rcsc::GameMode::PlayOn)
			isDanger = false;

		if (wm.time().cycle() - dangerCycle > 33 && wm.ball().vel().r() > 1.0)
			isDanger = false;

		if (wm.existKickableOpponent() || wm.existKickableTeammate())
			isDanger = false;
	}

	// catchable
	if (agent->world().time().cycle()
			> agent->world().self().catchTime().cycle()
					+ ServerParam::i().catchBanCycle()
			&& agent->world().ball().distFromSelf()
					< ServerParam::i().catchableArea() - 0.05
            && ((wm.ball().pos() - wm.self().pos()).th() - wm.self().body()).abs() < 90
			&& our_penalty.contains(agent->world().ball().pos())) {

		Vector2D nearestTmmPos = Vector2D(100, 100);
		if (!wm.teammatesFromSelf().empty()&& wm.teammatesFromSelf().front()!=NULL) {
			nearestTmmPos = wm.teammatesFromSelf().front()->pos();
		}

		if (our_penalty.contains(agent->world().ball().pos())
				&& (nearestTmmPos.dist(wm.ball().pos()) > 2.0
						|| wm.existKickableOpponent()) && !kicking
				&& (!isDanger
						|| (wm.ball().vel().r() > 2.6
								&& agent->world().ball().inertiaPoint(5).x
										< -52.5))) {
			kicking = false;
			isDanger = false;
            dlog.addText(Logger::TEAM, __FILE__"Want to catch");
			agent->doCatch();
			agent->setNeckAction(new Neck_TurnToBall());
		} else {
			kicking = true;
			isDanger = false;
            dlog.addText(Logger::TEAM, __FILE__"Want to kick");
			doKick(agent);
		}
	} else if (wm.self().isKickable()) {
		kicking = true;
		isDanger = false;
        dlog.addText(Logger::TEAM, __FILE__"is kickable");
		doKick(agent);
	} else {
		kicking = false;
        dlog.addText(Logger::TEAM, __FILE__"Want to move");
		doMove(agent);
	}

	return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
void RoleGoalie::doKick(PlayerAgent * agent) {
	if (Bhv_ChainAction().execute(agent)) {
		dlog.addText(Logger::TEAM,
		__FILE__": (execute) do chain action");
		agent->debugClient().addMessage("ChainAction");
		return;
	}

	Body_ClearBall().execute(agent);
	agent->setNeckAction(new Neck_ScanField());
}

/*-------------------------------------------------------------------*/
/*!

 */
void RoleGoalie::doMove(PlayerAgent * agent) {

    const WorldModel & wm = agent->world();
    const InterceptTable * iTable = wm.interceptTable();

	int ourReachCycle = std::min(
            iTable->teammateReachCycle(),
            iTable->selfReachCycle());

	bool ball_will_be_in_our_goal = false, isGoal = false;
	const ServerParam & SP = ServerParam::i();
	const Ray2D ball_ray(agent->world().ball().pos(),
			agent->world().ball().vel().th());
	const Line2D goal_line(
			Vector2D(-SP.pitchHalfLength(), SP.goalHalfWidth() + 2.0),
			Vector2D(-SP.pitchHalfLength(), -SP.goalHalfWidth() - 2.0));
	dlog.addLine(Logger::TEAM, ball_ray.origin(),
			agent->world().ball().inertiaPoint(ourReachCycle), "#ff0000");
	const Vector2D intersect = ball_ray.intersection(goal_line);
    if (intersect.isValid() && intersect.absY() < SP.goalHalfWidth() + 2.0
            && wm.ball().inertiaFinalPoint().x < -SP.pitchHalfLength() + 5) {
		ball_will_be_in_our_goal = true;

		dlog.addText(Logger::TEAM,
		__FILE__": ball will be in our goal. intersect=(%.2f %.2f)",
				intersect.x, intersect.y);
	}
    if (ball_will_be_in_our_goal
            && agent->world().ball().inertiaPoint(ourReachCycle).x < -SP.pitchHalfLength() + 1.0) {
		dlog.addText(Logger::TEAM,
		__FILE__": surely ball is goal!!!!");
		isGoal = true;
	}

    if (isGoal && (Bhv_BasicTackle(0.4).execute(agent)
                   || Bhv_BasicTackle(Bhv_BasicTackle::calc_takle_prob(wm)).execute(agent))) {
		return;
	}

    int self_cycle = iTable->selfReachCycle();
    int self_cycle_tackle = iTable->selfReachCycle(); //iTable->selfReachCycleTackle(); oldcyrus
    int opp_cycle = iTable->opponentReachCycle();
    int mate_cycle = iTable->teammateReachCycle();


    bool action_selected = false;
    if (Bhv_GoalieChaseBall::is_ball_chase_situation(agent))
    {
        action_selected = true;
        Bhv_GoalieChaseBall().execute(agent);
    }
    else
    {
        if(self_cycle < mate_cycle
                && self_cycle < opp_cycle - 2
                && agent->world().ball().posCount() < 2)
        {
            if(Body_Intercept(false).execute(agent))
            {
                dlog.addText(Logger::TEAM,
                __FILE__": execute full interception");
                agent->setNeckAction(new Neck_TurnToBall());
                action_selected = true;
            }
        }
    }
    if(!action_selected)
    {
        if(Strategy::i().isgoal_forward())
        {
            RolePlayer().execute(agent);
        }
        else
            Bhv_GoalieBasicMove().execute(agent);
    }
}
