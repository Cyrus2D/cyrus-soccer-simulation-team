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

#include "view_tactical.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/view_synch.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>

#include <rcsc/common/server_param.h>
#include <rcsc/common/logger.h>

using namespace rcsc;

/*-------------------------------------------------------------------*/
/*!

 */
bool
View_Tactical::execute( PlayerAgent * agent )
{
    dlog.addText( Logger::TEAM,
                  __FILE__": View_Tactical" );

    // when our free kick mode, view width is set to WIDE
    switch ( agent->world().gameMode().type() ) {
    case GameMode::BeforeKickOff:
    case GameMode::AfterGoal_:
    case GameMode::PenaltySetup_:
    case GameMode::PenaltyReady_:
    case GameMode::PenaltyMiss_:
    case GameMode::PenaltyScore_:
        dlog.addText( Logger::TEAM,
                      __FILE__": doNormalChange. wide view" );
        return View_Wide().execute( agent );

    case GameMode::PenaltyTaken_:
        return View_Synch().execute( agent );

    case GameMode::GoalieCatch_:
        // our goalie kick
        if ( agent->world().gameMode().side() == agent->world().ourSide() )
        {
            return doOurGoalieFreeKick( agent );
        }

    case GameMode::BackPass_:
    case GameMode::FreeKickFault_:
    case GameMode::CatchFault_:
    case GameMode::GoalKick_:
    case GameMode::KickOff_:
    case GameMode::KickIn_:
    case GameMode::CornerKick_:
    case GameMode::FreeKick_:
    case GameMode::OffSide_:
    case GameMode::IndFreeKick_:
    default:
        break;
    }

    return doDefault( agent );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
View_Tactical::doDefault( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    if ( ! wm.ball().posValid() )
    {
        // for field scan
        return agent->doChangeView( ViewWidth::WIDE );
    }

    int self_min = wm.interceptTable()->selfReachCycle();
    int mate_min = wm.interceptTable()->teammateReachCycle();
    int opp_min = wm.interceptTable()->opponentReachCycle();
    int ball_reach_cycle = std::min( self_min, std::min( mate_min, opp_min ) );

    if (self_min < mate_min && self_min <= opp_min){
        if (self_min <= 2){
            return agent->doChangeView( ViewWidth::NARROW );
        }
        if (self_min == 3){
            return agent->doChangeView( ViewWidth::NORMAL );
        }
        if (self_min <= 5){
            return agent->doChangeView( ViewWidth::WIDE );
        }
    }
    const Vector2D ball_pos = wm.ball().inertiaPoint( ball_reach_cycle );
    const double ball_dist = agent->effector().queuedNextSelfPos().dist( ball_pos );

    if ( wm.self().goalie()
         && ! wm.self().isKickable() )
    {
        const Vector2D goal_pos( - ServerParam::i().pitchHalfLength(), 0.0 );
        if ( ball_dist > 10.0
             || ball_pos.x > ServerParam::i().ourPenaltyAreaLineX()
             || ball_pos.dist( goal_pos ) > 20.0 )
        {
            AngleDeg ball_angle
                    = agent->effector().queuedNextAngleFromBody( agent->effector().queuedNextBallPos() );
            double angle_diff = ball_angle.abs() - ServerParam::i().maxNeckAngle();
            if ( angle_diff > ViewWidth::width( ViewWidth::NORMAL ) * 0.5 - 3.0 )
            {
                dlog.addText( Logger::TEAM,
                              __FILE__": (goalie) ball_angle=%.1f angle_diff=%.1f change to wide",
                              ball_angle.degree(), angle_diff );
                return agent->doChangeView( ViewWidth::WIDE );
            }
            else if ( angle_diff > ViewWidth::width( ViewWidth::NARROW ) * 0.5 - 3.0 )
            {
                dlog.addText( Logger::TEAM,
                              __FILE__": (goalie) ball_angle=%.1f angle_diff=%.1f change to normal",
                              ball_angle.degree(), angle_diff );
                return agent->doChangeView( ViewWidth::NORMAL );
            }

            dlog.addText( Logger::TEAM,
                          __FILE__": (goalie) ball_angle=%.1f angle_diff=%.1f default",
                          ball_angle.degree(), angle_diff );
        }
    }

    static std::vector<ViewWidth> last_view;
    ViewWidth res = ViewWidth::WIDE;
    bool select = false;
    // ball dist is too far
    if ( ball_dist > 40.0 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": far ball_dist=%.2f wide",
                      ball_dist );
        res = ViewWidth::WIDE;
        select = true;
//        return agent->doChangeView( ViewWidth::WIDE );
    }
    else if ( self_min < mate_min && self_min < opp_min){
        if (self_min > 2){
            res = ViewWidth::WIDE;
            select = true;
        }
    }
    if(!select){
        if ( ball_dist > 20.0 )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": far ball_dist=%.2f normal",
                          ball_dist );
            res = ViewWidth::NORMAL;
            select = true;
        }

        else if ( ball_dist > 10.0 )
        {
            AngleDeg ball_angle
                    = agent->effector().queuedNextAngleFromBody( agent->effector().queuedNextBallPos() );
            if ( ball_angle.abs() > 120.0 )
            {
                dlog.addText( Logger::TEAM,
                              __FILE__": big angle_diff=%.1f wide",
                              ball_angle.abs() );
                res = ViewWidth::WIDE;
                select = true;
            }
        }else{
            //const bool teammate_kickable = agent->world().existKickableTeammate();
            //const bool opponent_kickable = agent->world().existKickableOpponent();

            double teammate_ball_dist = 1000.0;
            double opponent_ball_dist = 1000.0;

            if ( ! agent->world().teammatesFromBall().empty() )
            {
                teammate_ball_dist
                        = agent->world().teammatesFromBall().front()->distFromBall();
            }

            if ( ! agent->world().opponentsFromBall().empty() )
            {
                opponent_ball_dist
                        = agent->world().opponentsFromBall().front()->distFromBall();
            }

            if ( ! wm.self().goalie()
                 && teammate_ball_dist > 5.0
                 && opponent_ball_dist > 5.0
                 && ball_dist > 10.0
                 && wm.ball().distFromSelf() > 10.0 )
            {
                dlog.addText( Logger::TEAM,
                              __FILE__": teammate_dist=%.2f opponent_dist=%.2f normal",
                              teammate_ball_dist, opponent_ball_dist );
                select = true;
                res = ViewWidth::NORMAL;
            }
        }
    }


    if(select){

        if(last_view.size()==0){
            last_view.push_back(res);
        }else{
            if(last_view.at(last_view.size()-1) == ViewWidth::NORMAL){
                res = ViewWidth::NORMAL;
                last_view.clear();
            }else if(last_view.size()>=2 && last_view.at(last_view.size()-1) == ViewWidth::WIDE && last_view.at(last_view.size()-2) == ViewWidth::WIDE){
                res = ViewWidth::WIDE;
                last_view.clear();
            }
        }
        agent->doChangeView(res);
        return true;
    }else{
        dlog.addText( Logger::TEAM,
                      __FILE__": default synch" );
        return View_Synch().execute( agent );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
View_Tactical::doOurGoalieFreeKick( PlayerAgent * agent )
{
    if ( agent->world().self().goalie() )
    {
        return View_Wide().execute( agent );
    }

    return doDefault( agent );
}
#include "chain_action/action_chain_holder.h"
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
