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

#include "bhv_set_play.h"

#include "../strategy.h"

#include "../goalie/bhv_goalie_free_kick.h"
#include "bhv_set_play_free_kick.h"
#include "bhv_set_play_goal_kick.h"
#include "bhv_set_play_kick_off.h"
#include "bhv_set_play_kick_in.h"
#include "bhv_set_play_indirect_free_kick.h"
#include "bhv_their_goal_kick_move.h"
#include "../move_def/bhv_mark_execute.h"
#include "../chain_action/field_analyzer.h"
#include "basic_actions/basic_actions.h"
#include "basic_actions/bhv_before_kick_off.h"
#include "basic_actions/bhv_scan_field.h"
#include "basic_actions/body_go_to_point.h"
#include "basic_actions/neck_scan_field.h"
#include "basic_actions/neck_turn_to_ball_or_scan.h"
#include "bhv_mark_decision_greedy.h"

#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>

#include <rcsc/common/audio_memory.h>
#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/circle_2d.h>
#include <rcsc/geom/line_2d.h>
#include <rcsc/geom/segment_2d.h>

#include <vector>
#include <algorithm>
#include <limits>
#include <cstdio>

// #define DEBUG_PRINT
using namespace rcsc;
using namespace std;
/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_SetPlay::execute( PlayerAgent * agent )
{
    dlog.addText( Logger::TEAM,
                  __FILE__": Bhv_SetPlay" );

    const WorldModel & wm = agent->world();

#ifdef DEBUG_PRINT
    Vector2D home_pos = Strategy::i().getPosition( wm.self().unum() );
    if ( ! home_pos.isValid() )
    {
        std::cerr << agent->config().teamName() << ": "
                  << wm.self().unum()
                  << " ***ERROR*** illegal home position."
                  << std::endl;
        home_pos.assign( 0.0, 0.0 );
    }

    dlog.addCircle( Logger::TEAM,
                    home_pos, 0.5, "#0000ff", true );
#endif


    if ( wm.self().goalie())
    {
        if(wm.gameMode().type() == GameMode::GoalKick_)
            if ( wm.gameMode().side() == wm.ourSide() )
            {
                return Bhv_SetPlayGoalKick().execute( agent );
            }
        if ( wm.gameMode().type() != GameMode::BackPass_
             && wm.gameMode().type() != GameMode::IndFreeKick_ )
        {
            Bhv_GoalieFreeKick().execute( agent );
        }
        else
        {
            Bhv_SetPlayIndirectFreeKick().execute( agent );
        }

        return true;
    }

    switch ( wm.gameMode().type() ) {
    case GameMode::KickOff_:
        if ( wm.gameMode().side() == wm.ourSide() )
        {
            return Bhv_SetPlayKickOff().execute( agent );
        }
        else
        {
            doBasicTheirSetPlayMove( agent );
            return true;
        }
        break;
    case GameMode::KickIn_:
    case GameMode::CornerKick_:
        if ( wm.gameMode().side() == wm.ourSide() )
        {
            return Bhv_SetPlayKickIn().execute( agent );
        }
        else
        {
            doBasicTheirSetPlayMove( agent );
            return true;
        }
        break;
    case GameMode::GoalKick_:
        if ( wm.gameMode().side() == wm.ourSide() ) 
        {
            return Bhv_SetPlayGoalKick().execute( agent );
        }
        else
        {
            return Bhv_TheirGoalKickMove().execute( agent );
        }
        break;
    case GameMode::BackPass_:
    case GameMode::IndFreeKick_:
        return Bhv_SetPlayIndirectFreeKick().execute( agent );
        break;
    case GameMode::FoulCharge_:
    case GameMode::FoulPush_:
        if ( wm.ball().pos().x < ServerParam::i().ourPenaltyAreaLineX() + 1.0
             && wm.ball().pos().absY() < ServerParam::i().penaltyAreaHalfWidth() + 1.0 )
        {
            return Bhv_SetPlayIndirectFreeKick().execute( agent );
        }
        else if ( wm.ball().pos().x > ServerParam::i().theirPenaltyAreaLineX() - 1.0
                  && wm.ball().pos().absY() < ServerParam::i().penaltyAreaHalfWidth() + 1.0 )
        {
            return Bhv_SetPlayIndirectFreeKick().execute( agent );
        }
        break;
#if 0
    case GameMode::FreeKick_:
    case GameMode::CornerKick_:
    case GameMode::GoalieCatch_: // after catch
    case GameMode::Offside_:
    case GameMode::FreeKickFault_:
    case GameMode::CatchFault_:
#endif
    default:
        break;
    }

    if ( wm.gameMode().isOurSetPlay( wm.ourSide() ) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": our set play" );
        return Bhv_SetPlayFreeKick().execute( agent );
    }
    else
    {
        doBasicTheirSetPlayMove( agent );
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
double
Bhv_SetPlay::get_set_play_dash_power( const PlayerAgent * agent )
{
#if 1
    const WorldModel & wm = agent->world();

    if ( ! wm.gameMode().isOurSetPlay( wm.ourSide() ) )
    {
        Vector2D target_point = Strategy::i().getPosition( wm.self().unum() );
        if ( target_point.x > wm.self().pos().x )
        {
            if ( wm.ball().pos().x < -30.0
                 && target_point.x < wm.ball().pos().x )
            {
                return wm.self().getSafetyDashPower( ServerParam::i().maxDashPower() );
            }

            double rate = 0.0;
            if ( wm.self().stamina() > ServerParam::i().staminaMax() * 0.8 )
            {
                rate = 1.5 * wm.self().stamina() / ServerParam::i().staminaMax();
            }
            else
            {
                rate = 0.9
                        * ( wm.self().stamina() - ServerParam::i().recoverDecThrValue() )
                        / ServerParam::i().staminaMax();
                rate = std::max( 0.0, rate );
            }

            return ( wm.self().playerType().staminaIncMax()
                     * wm.self().recovery()
                     * rate );
        }
    }

    return wm.self().getSafetyDashPower( ServerParam::i().maxDashPower() );
#else
    if ( agent->world().gameMode().type() == GameMode::PenaltySetup_ )
    {
        return agent->world().self().getSafetyDashPower( ServerParam::i().maxDashPower() );
    }

    double rate;
    if ( agent->world().self().stamina() > ServerParam::i().staminaMax() * 0.8 )
    {
        rate = 1.5
                * agent->world().self().stamina()
                / ServerParam::i().staminaMax();
    }
    else
    {
        rate = 0.9
                * ( agent->world().self().stamina()
                    - ServerParam::i().recoverDecThrValue() )
                / ServerParam::i().staminaMax();
        rate = std::max( 0.0, rate );
    }

    return ( agent->world().self().playerType().staminaIncMax()
             * agent->world().self().recovery()
             * rate );
#endif
}

namespace {

bool
can_go_to( const int count,
           const WorldModel & wm,
           const Circle2D & ball_circle,
           const Vector2D & target_point )
{
    Segment2D move_line( wm.self().pos(), target_point );

    int n_intersection = ball_circle.intersection( move_line, NULL, NULL );

    dlog.addText( Logger::TEAM,
                  "%d: (can_go_to) check target=(%.2f %.2f) intersection=%d",
                  count, target_point.x, target_point.y,
                  n_intersection );
    dlog.addLine( Logger::TEAM,
                  wm.self().pos(), target_point,
                  "#0000ff" );
    char num[8];
    snprintf( num, 8, "%d", count );
    dlog.addMessage( Logger::TEAM,
                     target_point, num,
                     "#0000ff" );

    if ( n_intersection == 0 )
    {
        dlog.addText( Logger::TEAM,
                      "%d: (can_go_to) ok(1)", count );
        return true;
    }

    if ( n_intersection == 1 )
    {
        AngleDeg angle = ( target_point - wm.self().pos() ).th();

        dlog.addText( Logger::TEAM,
                      "%d: (can_go_to) intersection=1 angle_diff=%.1f",
                      count,
                      ( angle - wm.ball().angleFromSelf() ).abs() );
        if ( ( angle - wm.ball().angleFromSelf() ).abs() > 80.0 )
        {
            dlog.addText( Logger::TEAM,
                          "%d: (can_go_to) ok(2)", count );
            return true;
        }
    }

    return false;
}

} // end noname namespace


Vector2D
Bhv_SetPlay::get_avoid_circle_point( const WorldModel & wm,
                                     const Vector2D & target_point )
{
    const ServerParam & SP = ServerParam::i();

    const double avoid_radius
            = SP.centerCircleR()
            + wm.self().playerType().playerSize();
    const Circle2D ball_circle( wm.ball().pos(), avoid_radius );

#ifdef DEBUG_PRINT
    dlog.addText( Logger::TEAM,
                  __FILE__": (get_avoid_circle_point) first_target=(%.2f %.2f)",
                  target_point.x, target_point.y );
    dlog.addCircle( Logger::TEAM,
                    wm.ball().pos(), avoid_radius,
                    "#ffffff" );
#endif

    if ( can_go_to( -1, wm, ball_circle, target_point ) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (get_avoid_circle_point) ok, first point" );
        return target_point;
    }

    AngleDeg target_angle = ( target_point - wm.self().pos() ).th();
    AngleDeg ball_target_angle = ( target_point - wm.ball().pos() ).th();
    bool ball_is_left = wm.ball().angleFromSelf().isLeftOf( target_angle );

    const int ANGLE_DIVS = 6;
    std::vector< Vector2D > subtargets;
    subtargets.reserve( ANGLE_DIVS );

    const int angle_step = ( ball_is_left ? 1 : -1 );

    int count = 0;
    int a = angle_step;

#ifdef DEBUG_PRINT
    dlog.addText( Logger::TEAM,
                  __FILE__": (get_avoid_circle_point) ball_target_angle=%.1f angle_step=%d",
                  ball_target_angle.degree(), angle_step );
#endif

    for ( int i = 1; i < ANGLE_DIVS; ++i, a += angle_step, ++count )
    {
        AngleDeg angle = ball_target_angle + (180.0/ANGLE_DIVS)*a;
        Vector2D new_target = wm.ball().pos()
                + Vector2D::from_polar( avoid_radius + 1.0, angle );

        dlog.addText( Logger::TEAM,
                      "%d: a=%d angle=%.1f (%.2f %.2f)",
                      count, a, angle.degree(),
                      new_target.x, new_target.y );

        if ( new_target.absX() > SP.pitchHalfLength() + SP.pitchMargin() - 1.0
             || new_target.absY() > SP.pitchHalfWidth() + SP.pitchMargin() - 1.0 )
        {
            dlog.addText( Logger::TEAM,
                          "%d: out of field",
                          count );
            break;
        }

        if ( can_go_to( count, wm, ball_circle, new_target ) )
        {
            return new_target;
        }
    }

    a = -angle_step;
    for ( int i = 1; i < ANGLE_DIVS*2; ++i, a -= angle_step, ++count )
    {
        AngleDeg angle = ball_target_angle + (180.0/ANGLE_DIVS)*a;
        Vector2D new_target = wm.ball().pos()
                + Vector2D::from_polar( avoid_radius + 1.0, angle );

#ifdef DEBUG_PRINT
        dlog.addText( Logger::TEAM,
                      "%d: a=%d angle=%.1f (%.2f %.2f)",
                      count, a, angle.degree(),
                      new_target.x, new_target.y );
#endif
        if ( new_target.absX() > SP.pitchHalfLength() + SP.pitchMargin() - 1.0
             || new_target.absY() > SP.pitchHalfWidth() + SP.pitchMargin() - 1.0 )
        {
#ifdef DEBUG_PRINT
            dlog.addText( Logger::TEAM,
                          "%d: out of field",
                          count );
#endif
            break;
        }

        if ( can_go_to( count, wm, ball_circle, new_target ) )
        {
            return new_target;
        }
    }

    return target_point;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_SetPlay::is_kicker( const PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    //if ( getSetPlayCount() < 1 )
    //     if ( wm.lastSetPlayStartTime().cycle() > wm.time().cycle() - 2 )
    //     {
    //         return false;
    //     }

    if ( wm.gameMode().type() == GameMode::GoalieCatch_
         && wm.gameMode().side() == wm.ourSide()
         && ! wm.self().goalie() )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (is_kicker) goalie free kick" );
        return false;
    }
    if( !wm.self().goalie() && wm.gameMode().type()==GameMode::GoalKick_ && wm.gameMode().side() == wm.ourSide())
        return false;
    if( wm.self().goalie() && wm.gameMode().type()==GameMode::GoalKick_ && wm.gameMode().side() == wm.ourSide())
        return true;

    int kicker_unum = 0;
    double min_dist2 = std::numeric_limits< double >::max();
    int second_kicker_unum = 0;
    double second_min_dist2 = std::numeric_limits< double >::max();
    for ( int unum = 1; unum <= 11; ++unum )
    {
        if ( unum == wm.ourGoalieUnum() ) continue;

        Vector2D home_pos = Strategy::i().getPosition( unum );
        if ( ! home_pos.isValid() ) continue;

        double d2 = home_pos.dist2( wm.ball().pos() );
        if ( d2 < second_min_dist2 )
        {
            second_kicker_unum = unum;
            second_min_dist2 = d2;

            if ( second_min_dist2 < min_dist2 )
            {
                std::swap( second_kicker_unum, kicker_unum );
                std::swap( second_min_dist2, min_dist2 );
            }
        }
    }

    dlog.addText( Logger::TEAM,
                  __FILE__": (is_kicker) kicker_unum=%d second_kicker_unum=%d",
                  kicker_unum, second_kicker_unum );

    const AbstractPlayerObject * kicker = static_cast< AbstractPlayerObject* >( 0 );
    const AbstractPlayerObject * second_kicker = static_cast< AbstractPlayerObject* >( 0 );

    if ( kicker_unum != 0 )
    {
        kicker = wm.ourPlayer( kicker_unum );
    }

    if ( second_kicker_unum != 0 )
    {
        second_kicker = wm.ourPlayer( second_kicker_unum );
    }

    if ( ! kicker )
    {
        if ( ! wm.teammatesFromBall().empty()
             && wm.teammatesFromBall().front()->distFromBall() < wm.ball().distFromSelf() * 0.9 )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": (is_kicker) first kicker",
                          kicker_unum, second_kicker_unum );
            return false;
        }

        dlog.addText( Logger::TEAM,
                      __FILE__": (is_kicker) self(1)" );
        return true;
    }

    if ( kicker
         && second_kicker
         && ( kicker->unum() == wm.self().unum()
              || second_kicker->unum() == wm.self().unum() ) )
    {
        if ( std::sqrt( min_dist2 ) < std::sqrt( second_min_dist2 ) * 0.95 )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": (is_kicker) kicker->unum=%d  (1)",
                          kicker->unum() );
            return ( kicker->unum() == wm.self().unum() );
        }
        else if ( kicker->distFromBall() < second_kicker->distFromBall() * 0.95 )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": (is_kicker) kicker->unum=%d  (2)",
                          kicker->unum() );
            return ( kicker->unum() == wm.self().unum() );
        }
        else if ( second_kicker->distFromBall() < kicker->distFromBall() * 0.95 )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": (is_kicker) kicker->unum=%d  (3)",
                          kicker->unum() );
            return ( second_kicker->unum() == wm.self().unum() );
        }
        else  if ( ! wm.teammatesFromBall().empty()
                   && wm.teammatesFromBall().front()->distFromBall() < wm.self().distFromBall() * 0.95 )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": (is_kicker) other kicker",
                          kicker->unum() );
            return false;
        }
        else
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": (is_kicker) self(2)" );
            return true;
        }
    }

    dlog.addText( Logger::TEAM,
                  __FILE__": (is_kicker) kicker->unum=%d",
                  kicker->unum() );

    return ( kicker->unum() == wm.self().unum() );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_SetPlay::is_delaying_tactics_situation( const PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    if(FieldAnalyzer::isGLD(wm))
    {
        return false;
    }
#if 1
    const int real_set_play_count = wm.time().cycle() - wm.lastSetPlayStartTime().cycle();
    const int wait_buf = ( wm.gameMode().type() == GameMode::GoalKick_
                           ? 15
                           : 2 );

    if ( real_set_play_count >= ServerParam::i().dropBallTime() - wait_buf )
    {
        return false;
    }
#endif

    int our_score = ( wm.ourSide() == LEFT
                      ? wm.gameMode().scoreLeft()
                      : wm.gameMode().scoreRight() );
    int opp_score = ( wm.ourSide() == LEFT
                      ? wm.gameMode().scoreRight()
                      : wm.gameMode().scoreLeft() );

#if 1
    if ( wm.audioMemory().recoveryTime().cycle() >= wm.time().cycle() - 15 )
    {
        return true;
    }
#endif
    if ( wm.audioMemory().waitRequestTime().cycle() >= wm.time().cycle() - 15 )
    {
        return true;
    }

    long cycle_thr = std::max( 0,
                               ServerParam::i().nrNormalHalfs()
                               * ( ServerParam::i().halfTime() * 10 )
                               - 500 );

    if ( wm.time().cycle() < cycle_thr )
    {
        return false;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!
  execute action
 */
void
Bhv_SetPlay::doBasicTheirSetPlayMove( PlayerAgent * agent )
{

    const WorldModel & wm = agent->world();
    
    std::string name = wm.theirTeamName();

    bool isFRA = false;

    if( (name.find("F") != std::string::npos || name.find("f") != std::string::npos)
            && (name.find("R") != std::string::npos || name.find("r") != std::string::npos)
            && (name.find("A") != std::string::npos || name.find("a") != std::string::npos)
            && (name.find("U") != std::string::npos || name.find("u") != std::string::npos)
            && (name.find("N") != std::string::npos || name.find("n") != std::string::npos) ){
        isFRA = true;
    }
    Vector2D new_taget_point;
    if (isFRA)
    {
        new_taget_point = FRA_set_play(agent);
        if (new_taget_point != Vector2D(-100,-100))
        {
            dlog.addCircle(Logger::ROLE,new_taget_point,0.5 , 255,0,255,true);
            new_taget_point = get_avoid_circle_point(wm,new_taget_point);
            dlog.addCircle(Logger::ROLE,new_taget_point,0.5 , 0,255,255,true);
        }
    }

    Vector2D target_point = Strategy::i().getPosition( wm.self().unum() ) ;


    dlog.addText( Logger::TEAM,
                  __FILE__": their set play. HomePosition=(%.2f, %.2f)",
                  target_point.x, target_point.y );

    double dash_power = Bhv_SetPlay::get_set_play_dash_power( agent );


//    if (wm.interceptTable().opponentStep() < 1000 && wm.ball().pos().x < -25 && bhv_mark_execute().execute(agent) && agent->effector().queuedNextMyPos().dist(wm.ball().pos()) > 11)
//            return;

    if (wm.interceptTable().opponentStep() < 1000 && wm.ball().pos().x < -25 && bhv_mark_execute().execute(agent)) {
        auto dist =  agent->effector().queuedNextMyPos().dist(wm.ball().pos());
        if(dist > 10 || true){
            dlog.addText( Logger::TEAM,
                          __FILE__": their set play. mark executed");
            return;
        }

        if(dist < 10) {
            dlog.addText( Logger::TEAM,
                          __FILE__": their set play. execute mark, cancel for dist");
            Body_TurnToBall().execute(agent);
            return;
        }
    }

    Vector2D ball_to_target = target_point - wm.ball().pos();

    if ( ball_to_target.r() < 11.0 )
    {
        double xdiff = std::sqrt( std::pow( 11.0, 2 ) - std::pow( ball_to_target.y, 2 ) );
        target_point.x = wm.ball().pos().x - xdiff;

        dlog.addText( Logger::TEAM,
                      __FILE__": avoid circle(1). adjust x. x_diff=%.1f newPos=(%.2f %.2f)",
                      xdiff,
                      target_point.x, target_point.y );

        if ( target_point.x < -45.0 )
        {
            target_point = wm.ball().pos();
            target_point += ball_to_target.setLengthVector( 11.0 );

            dlog.addText( Logger::TEAM,
                          __FILE__": avoid circle(2). adjust len. new_pos=(%.2f %.2f)",
                          target_point.x, target_point.y );
        }
    }

    //
    // avoid kickoff offside
    //
    if ( wm.gameMode().type() == GameMode::KickOff_
         && ServerParam::i().kickoffOffside() )
    {
        target_point.x = std::min( -1.0e-5, target_point.x );

        dlog.addText( Logger::TEAM,
                      __FILE__": avoid kickoff offside. (%.2f %.2f)",
                      target_point.x, target_point.y );
    }

    //
    // find sub target
    //
    dlog.addText( Logger::TEAM,
                  __FILE__": find sub target to avoid ball circle" );
    Vector2D adjusted_point = get_avoid_circle_point( wm, target_point );

    //
    // go to the target point
    //
    double dist_thr = wm.ball().distFromSelf() * 0.1;
    if ( dist_thr < 0.7 ) dist_thr = 0.7;

    if ( adjusted_point != target_point
         && wm.ball().pos().dist( target_point ) > 10.0
         && wm.self().inertiaFinalPoint().dist( adjusted_point ) < dist_thr )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": reverted to the first target point" );
        adjusted_point = target_point;
    }

    //agent->debugClient().addMessage( "GoTo" );
    agent->debugClient().setTarget( target_point );
    agent->debugClient().addCircle( target_point, dist_thr );
    dlog.addCircle(Logger::ROLE,adjusted_point,0.5 , 0,0,255,true);

    if (adjusted_point.dist(new_taget_point) < 6 && new_taget_point != Vector2D(-100,-100))
    {
        adjusted_point = new_taget_point;
    }

    if ( ! Body_GoToPoint( adjusted_point,
                           dist_thr,
                           dash_power
                           ).execute( agent ) )
    {
        // already there
        AngleDeg body_angle = wm.ball().angleFromSelf();
        if ( body_angle.degree() < 0.0 ) body_angle -= 90.0;
        else body_angle += 90.0;

        Body_TurnToAngle( body_angle ).execute( agent );
    }

    agent->setNeckAction( new Neck_TurnToBall() );
}

Vector2D Bhv_SetPlay::FRA_set_play( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();
    
    //=========================my find who am i near to======================

    std::vector <pair<int,int>> mark_tm_unum;
    std::vector <pair<float,int>> mark_opp_unum;
    const AbstractPlayerObject *ball_haver = wm.interceptTable().firstOpponent();
    //cout<<"hello"<<endl;
    if (ball_haver != NULL && ball_haver->unum() > 0 )
    {
        //cout<<"hello2"<<endl;
        for(int i = 1 ; i < 12 ; i ++)
        {
            const AbstractPlayerObject *opp = wm.theirPlayer(i);
            if (i != ball_haver->unum() && opp != NULL && opp->unum() > 0)
            {
                pair <float,int> tmp;
                tmp.first = opp->pos().dist(ball_haver->pos());
                tmp.second = i;
                mark_opp_unum.push_back(tmp);
            }
        }
        sort(mark_opp_unum.begin() , mark_opp_unum.end());
        int max_for = 3;
        if (max_for > mark_opp_unum.size())
        {
            max_for = mark_opp_unum.size();
        }
        //cout<<"hello3"<<endl;
        for (int i = 0 ; i < max_for ; i++)
        {
            int opp_unum = mark_opp_unum[i].second;
            dlog.addText(Logger::ROLE,"i[%d] = %d",i,opp_unum);
            const AbstractPlayerObject *opp = wm.theirPlayer(opp_unum);
            if (opp_unum != ball_haver->unum() && opp != NULL && opp->unum() > 0)
            {
                float min_dist = 999;
                int close_unum = -1;
                for(int j = 1 ; j < 12 ; j ++)
                {
                    bool check = false;
                    for (auto z : mark_tm_unum)
                    {
                        if (z.first == j)
                        {
                            check = true;
                            break;
                        }

                    }

                    if (check)
                    {
                        continue;
                    }

                    const AbstractPlayerObject *tm = wm.ourPlayer(j);

                    if (tm != NULL && tm->unum() > 0)
                    {
                        if (!tm->goalie())
                        {
                            if (min_dist > tm->pos().dist(opp->pos()))
                            {
                                min_dist = tm->pos().dist(opp->pos());
                                close_unum = j;
                            }
                        }
                    }
                }
                if(close_unum != -1)
                {
                    pair <float,int> tmp;
                    tmp.first = close_unum;
                    tmp.second = opp_unum;
                    dlog.addText(Logger::ROLE,"%d should get %d",close_unum,opp_unum);
                    mark_tm_unum.push_back(tmp);
                }
            }
        }
    }
    int mark_unum = -1;
    for (int i = 0 ; i < mark_tm_unum.size() ; i++)
    {
        if (mark_tm_unum[i].first == wm.self().unum())
        {
            mark_unum = mark_tm_unum[i].second;
        }
    }
    const AbstractPlayerObject * mark_opp = wm.theirPlayer(mark_unum);
    Vector2D new_target_point;
    if (mark_opp == NULL || mark_opp->unum() < 1)
    {
        mark_unum = -1;
    }
    else 
    {
        if (wm.self().unum() > 5)
        {
            new_target_point = mark_opp->pos() + Vector2D::polar2vector(0.6, (wm.ball().pos() - mark_opp->pos()).dir());
        }
        else 
        {
            new_target_point = mark_opp->pos() + Vector2D::polar2vector(0.6, (-wm.ball().pos() + mark_opp->pos()).dir());
        }
    }
    
    if (mark_unum == -1)
    {
        return Vector2D(-100,-100);
    }
    dlog.addCircle(Logger::ROLE , new_target_point , 0.5 , 255 , 0 , 0 , true);
    const ServerParam & SP = ServerParam::i();

    const double avoid_radius
                = SP.centerCircleR()
                + wm.self().playerType().playerSize();
        const Circle2D ball_circle( wm.ball().pos(), avoid_radius );

    dlog.addCircle(Logger::ROLE , ball_circle,255,0,0,false);
    if ( can_go_to( -1, wm, ball_circle, new_target_point ) )
    {
        dlog.addText( Logger::TEAM,
                    __FILE__": (get_avoid_circle_point) ok, first point" );
        return new_target_point;
    }
    else 
    {
        new_target_point = new_target_point + Vector2D::polar2vector(avoid_radius - new_target_point.dist(wm.ball().pos()) + 0.1 , (new_target_point - wm.ball().pos()).dir());
    }
    dlog.addCircle(Logger::ROLE , new_target_point , 1 , 255 , 100 , 0 , true);

    return new_target_point;
}