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

#include "bhv_goalie_chase_ball.h"

#include "bhv_goalie_basic_move.h"
#include "../move_def/bhv_basic_tackle.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/body_stop_dash.h>
#include <rcsc/action/body_intercept.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/debug_client.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/line_2d.h>
#include "../move_def/bhv_tackle_intercept.h"

using namespace rcsc;

/*-------------------------------------------------------------------*/
/*!
  execute action
*/
bool
Bhv_GoalieChaseBall::execute( PlayerAgent * agent )
{
    dlog.addText( Logger::TEAM,
                  __FILE__": Bhv_GoalieChaseBall" );
    const WorldModel & wm = agent->world();
    //////////////////////////////////////////////////////////////////
    // tackle
    if ( Bhv_BasicTackle( Bhv_BasicTackle::calc_takle_prob(wm)).execute( agent ) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": tackle" );
        return true;
    }

    const ServerParam & SP = ServerParam::i();


    ////////////////////////////////////////////////////////////////////////
    // get active interception catch point

    Vector2D my_int_pos = wm.ball().inertiaPoint( wm.interceptTable()->selfReachCycle() );
    dlog.addText( Logger::TEAM,
                  __FILE__": execute. intercept point=(%.2f %.2f)",
                  my_int_pos.x, my_int_pos.y );

    double pen_thr = wm.ball().distFromSelf() * 0.1 + 1.0;
    if ( pen_thr < 1.0 ) pen_thr = 1.0;

    //----------------------------------------------------------
    const Line2D ball_line( wm.ball().pos(), wm.ball().vel().th() );
    const Line2D defend_line( Vector2D( -52, -10.0 ),
                              Vector2D( -52, 10.0 ) );

    if ( my_int_pos.x > - SP.pitchHalfLength() - 1.0
         && my_int_pos.x < SP.ourPenaltyAreaLineX() - pen_thr
         && my_int_pos.absY() < SP.penaltyAreaHalfWidth() - pen_thr )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": execute normal intercept1" );
        agent->debugClient().addMessage( "Intercept(0)" );
        Body_Intercept( false ).execute( agent );
        agent->setNeckAction( new Neck_TurnToBall() );
        return true;
    }


    if ( my_int_pos.x < - SP.pitchHalfLength() + 0.5
         && is_ball_shoot_moving(agent))
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": execute normal intercept2" );

        // Vector2D my_int_tackle_pos = wm.ball().inertiaPoint(wm.interceptTable()->selfReachCycleTackle());  // CURUS_LIB
        // if ( my_int_tackle_pos.x > - SP.pitchHalfLength() - 1.0
        //      && my_int_tackle_pos.x < SP.ourPenaltyAreaLineX() - pen_thr
        //      && my_int_tackle_pos.absY() < SP.penaltyAreaHalfWidth() - pen_thr )
        // {
        //     dlog.addText( Logger::TEAM,
        //                   __FILE__": execute tackle intercept" );
        //     agent->debugClient().addMessage( "Intercept tackle" );
        //     Body_Intercept( false ).executeTackle( agent );  // CURUS_LIB
        //     agent->setNeckAction( new Neck_TurnToBall() );
        //     return true;
        // }
        if(bhv_tackle_intercept::intercept_cycle(wm).first!=1000 && wm.ball().inertiaPoint(bhv_tackle_intercept::intercept_cycle(wm).first).x > -52.5){
            if(bhv_tackle_intercept().execute(agent)){
                dlog.addText( Logger::TEAM,
                              __FILE__": execute bhv tackle intercept" );
                agent->debugClient().addMessage( "tackle intercept" );
                agent->setNeckAction( new Neck_TurnToBall() );
                return true;
            }
        }
    }

    int self_goalie_min = wm.interceptTable()->selfReachCycle();
    int opp_min_cyc = wm.interceptTable()->opponentReachCycle();

//    if (self_goalie_min <= 2){
//        if (wm.ball().inertiaPoint(self_goalie_min).x > -52.5){
//            if ( Body_Intercept( false ).execute( agent ) )
//            {
//                dlog.addText( Logger::TEAM,
//                              __FILE__": execute normal interception" );
//                agent->debugClient().addMessage( "Intercept(11)" );
//                agent->setNeckAction( new Neck_TurnToBall() );
//                return true;
//            }
//        }
//    }
    Vector2D intersection = ball_line.intersection( defend_line );
    if ( ! intersection.isValid()
         || ball_line.dist( wm.self().pos() ) < SP.catchableArea() * 0.8
         || intersection.absY() > SP.goalHalfWidth() + 3.0
         )
    {
        if ( ! wm.kickableOpponent() )
        {
            if ( self_goalie_min <= opp_min_cyc + 2
                 && my_int_pos.x > -SP.pitchHalfLength() - 2.0
                 && my_int_pos.x < SP.ourPenaltyAreaLineX() - pen_thr
                 && my_int_pos.absY() < SP.penaltyAreaHalfWidth() - pen_thr
                 )
            {
                if ( Body_Intercept( false ).execute( agent ) )
                {
                    dlog.addText( Logger::TEAM,
                                  __FILE__": execute normal interception" );
                    agent->debugClient().addMessage( "Intercept(1)" );
                    agent->setNeckAction( new Neck_TurnToBall() );
                    return true;
                }
            }

            dlog.addText( Logger::TEAM,
                          __FILE__": execute. ball vel has same slope to my body??"
                                  " myvel-ang=%f body=%f. go to ball direct",
                          wm.ball().vel().th().degree(),
                          wm.self().body().degree() );
            // ball vel angle is same to my body angle
            agent->debugClient().addMessage( "GoToCatch(1)" );
            doGoToCatchPoint( agent, wm.ball().inertiaPoint(1) );
            return true;
        }
    }

    //----------------------------------------------------------
    // body is already face to intersection
    // only dash to intersection

    // check catch point
    if ( intersection.absX() > SP.pitchHalfLength() + SP.catchableArea()
         || intersection.x > SP.ourPenaltyAreaLineX() - SP.catchableArea()
         || intersection.absY() > SP.penaltyAreaHalfWidth() - SP.catchableArea()
         )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": execute intersection(%.2f, %.2f) over range.",
                      intersection.x, intersection.y );
        if ( Body_Intercept( false ).execute( agent ) )
        {
            agent->debugClient().addMessage( "Intercept(2)" );
            agent->setNeckAction( new Neck_TurnToBall() );
            return true;
        }
        else
        {
            return Bhv_GoalieBasicMove().execute( agent );
        }
    }

    //----------------------------------------------------------
    // check already there
    const Vector2D my_inertia_final_pos
            = wm.self().pos()
            + wm.self().vel()
            / (1.0 - wm.self().playerType().playerDecay());
    double dist_thr = 0.2 + wm.ball().distFromSelf() * 0.1;
    if ( dist_thr < 0.5 ) dist_thr = 0.5;

    // if already intersection point stop dash
    if ( my_inertia_final_pos.dist( intersection ) < dist_thr )
    {
        agent->debugClient().addMessage( "StopForChase" );
        Body_StopDash( false ).execute( agent );
        agent->setNeckAction( new Neck_TurnToBall() );
        return true;
    }

    //----------------------------------------------------------
    // forward or backward

    dlog.addText( Logger::TEAM,
                  __FILE__": slide move. point=(%.1f, %.1f)",
                  intersection.x, intersection.y );
    if ( wm.ball().pos().x > -35.0 )
    {
        if ( wm.ball().pos().y * intersection.y < 0.0 ) // opposite side
        {
            intersection.y = 0.0;
        }
        else
        {
            intersection.y *= 0.5;
        }
    }

    agent->debugClient().addMessage( "GoToCatch(2)" );
    doGoToCatchPoint( agent, intersection );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Bhv_GoalieChaseBall::doGoToCatchPoint( PlayerAgent * agent,
                                       const Vector2D & target_point )
{
    const ServerParam & SP = ServerParam::i();
    const WorldModel & wm = agent->world();

    double dash_power = 0.0;

    Vector2D rel = target_point - wm.self().pos();
    rel.rotate( - wm.self().body() );
    AngleDeg rel_angle = rel.th();
    const double angle_buf
            = std::fabs( AngleDeg::atan2_deg( SP.catchableArea() * 0.9, rel.r() ) );

    dlog.addText( Logger::TEAM,
                  __FILE__": GoToCatchPoint. (%.1f, %.1f). angle_diff=%.1f. angle_buf=%.1f",
                  target_point.x, target_point.y,
                  rel_angle.degree(), angle_buf );

    agent->debugClient().setTarget( target_point );

    // forward dash
    if ( rel_angle.abs() < angle_buf )
    {
        dash_power = std::min( wm.self().stamina() + wm.self().playerType().extraStamina(),
                               SP.maxDashPower() );
        dlog.addText( Logger::TEAM,
                      __FILE__": forward dash" );
        agent->debugClient().addMessage( "GoToCatch:Forward" );
        agent->doDash( dash_power );
    }
    // back dash
    else if ( rel_angle.abs() > 180.0 - angle_buf )
    {
        dash_power = SP.minDashPower();

        double required_stamina = ( SP.minDashPower() < 0.0
                                    ? SP.minDashPower() * -2.0
                                    : SP.minDashPower() );
        if ( wm.self().stamina() + wm.self().playerType().extraStamina()
             < required_stamina )
        {
            dash_power = wm.self().stamina() + wm.self().playerType().extraStamina();
            if ( SP.minDashPower() < 0.0 )
            {
                dash_power *= -0.5;
                if ( dash_power < SP.minDashPower() )
                {
                    dash_power = SP.minDashPower();
                }
            }
        }

        dlog.addText( Logger::TEAM,
                      __FILE__": back dash. power=%.1f",
                      dash_power );
        agent->debugClient().addMessage( "GoToCatch:Back" );
        agent->doDash( dash_power );
    }
    // forward dash turn
    else if ( rel_angle.abs() < 90.0 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": turn %.1f for forward dash",
                      rel_angle.degree() );
        agent->debugClient().addMessage( "GoToCatch:F-Turn" );
        agent->doTurn( rel_angle );
    }
    else
    {
        rel_angle -= 180.0;
        dlog.addText( Logger::TEAM,
                      __FILE__": turn %.1f for back dash",
                      rel_angle.degree() );
        agent->debugClient().addMessage( "GoToCatch:B-Turn" );
        agent->doTurn( rel_angle );
    }

    agent->setNeckAction( new Neck_TurnToBall() );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_GoalieChaseBall::is_ball_chase_situation( PlayerAgent  * agent )
{
    const WorldModel & wm = agent->world();

    if ( wm.gameMode().type() != GameMode::PlayOn )
    {
        return false;
    }

    const ServerParam & SP = ServerParam::i();

    int self_min = wm.interceptTable()->selfReachCycle();
    int self_min_tackle = wm.interceptTable()->selfReachCycle(); // selfReachCycleTackle(); // CYRUS_LIB
    int opp_min = wm.interceptTable()->opponentReachCycle();
    int mate_min = wm.interceptTable()->teammateReachCycle();

    Vector2D self_min_pos = wm.ball().inertiaPoint(self_min);
    Vector2D mate_min_pos = wm.ball().inertiaPoint(mate_min);
    Vector2D opp_min_pos = wm.ball().inertiaPoint(opp_min);
    Vector2D self_min_tackle_pos = wm.ball().inertiaPoint(self_min_tackle);

    Rect2D ourPA = SP.ourPenaltyArea();

    agent->debugClient().addCircle(self_min_tackle_pos,0.5);
    agent->debugClient().addCircle(self_min_pos,0.25);
    ////////////////////////////////////////////////////////////////////////
    // check shoot moving
    if ( is_ball_shoot_moving( agent ) )
    {
        dlog.addText( Logger::TEAM, __FILE__": ball maybe in goal" );
        if ( self_min_pos.x > -52 )
        {
            dlog.addText( Logger::TEAM, __FILE__": golie can intercept before goal" );
            if ( self_min < opp_min + (ourPA.contains(self_min_pos) ? 1 : 0 ) )
            {
                dlog.addText( Logger::TEAM, __FILE__": intercept before opp" );
                if ( self_min <= mate_min + 1)
                {
                    dlog.addText( Logger::TEAM, __FILE__": intercept before tm" );
                    return true;
                }
                else
                {
                    dlog.addText( Logger::TEAM, __FILE__": intercept after tm" );
                    return false;
                }
            }
            else
            {
                dlog.addText( Logger::TEAM, __FILE__": intercept after opp" );
                return false;
            }
        }
        else if ( self_min_tackle_pos.x > -52 )
        {
            dlog.addText( Logger::TEAM, __FILE__": golie can tackle intercept before olie" );
            if ( self_min_tackle < opp_min + (ourPA.contains(self_min_tackle_pos) ? 1 : 0 ) )
            {
                dlog.addText( Logger::TEAM, __FILE__": intercept before opp" );
                if ( self_min_tackle <= mate_min + 1)
                {
                    dlog.addText( Logger::TEAM, __FILE__": intercept before tm" );
                    return true;
                }
                else
                {
                    dlog.addText( Logger::TEAM, __FILE__": intercept after tm" );
                    return false;
                }
            }
            else
            {
                dlog.addText( Logger::TEAM, __FILE__": intercept after opp" );
                return false;
            }
        }
        else if ( opp_min_pos.x > -52 )
        {
            dlog.addText( Logger::TEAM, __FILE__": opp intercept before goal" );
            return false;
        }
        else
        {
            dlog.addText( Logger::TEAM, __FILE__": ball must be in goal" );
            return true;
        }
    }
    ////////////////////////////////////////////////////////////////////////
    // ball is in very dangerous area
    const Vector2D ball_next_pos = wm.ball().pos() + wm.ball().vel();
    if ( ball_next_pos.x < -SP.pitchHalfLength() + 8.0
         && ball_next_pos.absY() < SP.goalHalfWidth() + 3.0 )
    {
        // exist kickable teammate
        // avoid back pass
        if ( wm.kickableTeammate() )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": danger area. exist kickable teammate?" );
            return false;
        }
        else if ( wm.ball().distFromSelf() < 3.0
                  && self_min <= 3 )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": danger area. ball is very near." );
            return true;
        }
        else if ( self_min > opp_min + 1 )
        {
            if ( mate_min <= opp_min)
            {
                dlog.addText( Logger::TEAM,
                              __FILE__": danger area. opponent may get tha ball faster than me" );
                return false;
            }
            else if ( self_min_tackle > opp_min )
            {
                dlog.addText( Logger::TEAM,
                              __FILE__": danger area. opponent may get tha ball faster than me" );
                return false;
            }
        }
        else if ( mate_min < self_min - 2)
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": danger area. teammate may get tha ball faster than me" );
            return false;
        }
        dlog.addText( Logger::TEAM,
                      __FILE__": danger area. chase ball" );
        return true;
    }

    ////////////////////////////////////////////////////////////////////////
    // get active interception catch point

    double pen_thr = wm.ball().distFromSelf() * 0.1 + 1.0;
    if ( pen_thr < 1.0 ) pen_thr = 1.0;
    if ( self_min_pos.absY() > SP.penaltyAreaHalfWidth() - pen_thr
         || self_min_pos.x > SP.ourPenaltyAreaLineX() - pen_thr )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": intercept point is out of penalty" );
        return false;
    }

    ////////////////////////////////////////////////////////////////////////
    // Now, I can chase the ball
    // check the ball possessor

    if ( wm.kickableTeammate()
         && ! wm.kickableOpponent() )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": exist kickable player" );
        return false;
    }

    if ( opp_min <= self_min - 2 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": opponent reach the ball faster than me" );
        return false;
    }

    const double my_dist_to_catch = wm.self().pos().dist( self_min_pos );

    double opp_min_dist = 10000.0;
    wm.getOpponentNearestTo( self_min_pos, 30, &opp_min_dist );

    if ( opp_min_dist < my_dist_to_catch - 2.0 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": opponent is nearer than me. my_dist=%.2f  opp_dist=%.2f",
                      my_dist_to_catch, opp_min_dist );
        return false;
    }

    dlog.addText( Logger::TEAM,
                  __FILE__": exist interception point. try chase." );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_GoalieChaseBall::is_ball_shoot_moving( const PlayerAgent * agent )
{
    const ServerParam & SP = ServerParam::i();
    const WorldModel & wm = agent->world();

    if ( wm.ball().distFromSelf() > 30.0 )
    {
        return false;
    }

    if ( wm.ball().pos().x > -25.0 )
    {
        return false;
    }

    // check opponent kicker
    if ( wm.kickableOpponent() )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": check shoot moving. opponent kickable " );
        return false;
    }
    else if ( wm.kickableTeammate() )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": check shoot moving. teammate kickable" );
        return false;
    }

    const Line2D ball_line( wm.ball().pos(), wm.ball().vel().th() );
    const double intersection_y = ball_line.getY( -SP.pitchHalfLength() );

    if ( std::fabs( ball_line.b() ) > 0.1
         && std::fabs( intersection_y ) < SP.goalHalfWidth() + 2.0 )
    {
        const Vector2D end_point
                = wm.ball().pos()
                + wm.ball().vel() / ( 1.0 - SP.ballDecay());
        if ( end_point.x < -SP.pitchHalfLength() + 2.0 )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": shoot to Y(%.1f). ball_line a=%.1f, b=%.1f, c=%.1f",
                          intersection_y,
                          ball_line.a(),
                          ball_line.b(),
                          ball_line.c() );
            return true;
        }
    }


    return false;
}
