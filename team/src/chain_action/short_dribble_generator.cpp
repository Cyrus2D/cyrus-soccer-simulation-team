// -*-c++-*-

/*!
  \file short_dribble_generator.cpp
  \brief short step dribble course generator Source File
*/

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

#include "short_dribble_generator.h"

#include "dribble.h"
#include "field_analyzer.h"

#include <rcsc/player/world_model.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/common/server_param.h>
#include <rcsc/common/logger.h>
#include <rcsc/geom/circle_2d.h>
#include <rcsc/geom/segment_2d.h>
#include <rcsc/timer.h>
#include <rcsc/player/player_agent.h>
#include <algorithm>
#include <limits>
#include "setting.h"
#include <cmath>

//#define DEBUG_PROFILE
//#define DEBUG_PRINT
//#define DEBUG_PRINT_SIMULATE_DASHES
// #define DEBUG_PRINT_OPPONENT

#define DEBUG_PRINT_SUCCESS_COURSE
#define DEBUG_PRINT_FAILED_COURSE

using namespace rcsc;

namespace {

inline
void
debug_paint_failed( const int count,
                    const Vector2D & receive_point )
{
    dlog.addCircle( Logger::DRIBBLE,
                    receive_point.x, receive_point.y, 0.1,
                    "#ff0000" );
    char num[8];
    snprintf( num, 8, "%d", count );
    dlog.addMessage( Logger::DRIBBLE,
                     receive_point, num );
}

}

/*-------------------------------------------------------------------*/
/*!

 */
ShortDribbleGenerator::ShortDribbleGenerator()
    : M_queued_action_time( 0, 0 )
{
    M_courses.reserve( 128 );

    clear();
}

/*-------------------------------------------------------------------*/
/*!

 */
ShortDribbleGenerator &
ShortDribbleGenerator::instance()
{
    static ShortDribbleGenerator s_instance;
    return s_instance;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
ShortDribbleGenerator::clear()
{
    M_total_count = 0;
    M_first_ball_pos = Vector2D::INVALIDATED;
    M_first_ball_vel.assign( 0.0, 0.0 );
    M_courses.clear();
}

/*-------------------------------------------------------------------*/
/*!

 */
void
ShortDribbleGenerator::generate( const WorldModel & wm )
{
    if ( M_update_time == wm.time() )
    {
        return;
    }
    M_update_time = wm.time();

    clear();

    if ( wm.gameMode().type() != GameMode::PlayOn
         && ! wm.gameMode().isPenaltyKickMode() )
    {
        return;
    }

    //
    // check queued action
    //

    if ( M_queued_action_time != wm.time() )
    {
        M_queued_action.reset();
    }
    else
    {
        M_courses.push_back( M_queued_action );
    }

    //
    // updater ball holder
    //
    if ( wm.self().isKickable()
         && ! wm.self().isFrozen() )
    {
        M_first_ball_pos = wm.ball().pos();
        M_first_ball_vel = wm.ball().vel();
    }else if(wm.interceptTable()->selfReachCycle() <=2){
        M_first_ball_pos = wm.ball().inertiaPoint(wm.interceptTable()->selfReachCycle());
        M_first_ball_vel = wm.ball().vel(); // todo RF *decay^2
    }
    // else if ( ! wm.existKickableTeammate()
    //           && ! wm.existKickableOpponent()
    //           && wm.interceptTable()->selfReachCycle() <= 1 )
    // {
    //     M_first_ball_pos = wm.ball().pos() + wm.ball()vel();
    //     M_first_ball_vel = wm.ball().vel() + ServerParam::i().ballDecay();
    // }
    else
    {
        return;
    }

#ifdef DEBUG_PROFILE
    Timer timer;
#endif

    createCourses( wm );

    std::sort( M_courses.begin(), M_courses.end(),
               CooperativeAction::DistCompare( ServerParam::i().theirTeamGoalPos() ) );

#ifdef DEBUG_PROFILE
    dlog.addText( Logger::DRIBBLE,
                  __FILE__": (generate) PROFILE size=%d/%d elapsed %.3f [ms]",
                  (int)M_courses.size(),
                  M_total_count,
                  timer.elapsedReal() );
#endif
}

/*-------------------------------------------------------------------*/
/*!

 */
void
ShortDribbleGenerator::setQueuedAction( const rcsc::WorldModel & wm,
                                        CooperativeAction::Ptr action )
{
    M_queued_action_time = wm.time();
    M_queued_action = action;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
ShortDribbleGenerator::createCourses( const WorldModel & wm )
{
    static const int angle_div = 32;
    static const double angle_step = 360.0 / angle_div;

    const ServerParam & SP = ServerParam::i();

    const PlayerType & ptype = wm.self().playerType();

    const double my_first_speed = wm.self().vel().r();
    for ( int a = -2; a <= 2; ++a )
    {
        AngleDeg dash_angle = wm.self().body() - AngleDeg(180.0) + ( angle_step * a );

        //
        // angle filter
        //

//        if ( dash_angle.abs() > 90.0 )
//        {
//#ifdef DEBUG_PRINT
//            dlog.addText( Logger::DRIBBLE,
//                          __FILE__": (createTargetPoints) canceled(1) dash_angle=%.1f",
//                          dash_angle.degree() );
//#endif
////            continue;
//        }

        if ( M_first_ball_pos.x < -25.0 )
        {
#ifdef DEBUG_PRINT
            dlog.addText( Logger::DRIBBLE,
                          __FILE__": (createTargetPoints) canceled(2) dash_angle=%.1f",
                          dash_angle.degree() );
#endif
            continue;
        }

        int n_turn = 0;

        double my_speed = my_first_speed * ptype.playerDecay(); // first action is kick
        double dir_diff = AngleDeg( angle_step * a ).abs();

        while ( dir_diff > 0.0 )
        {
            dir_diff -= ptype.effectiveTurn( SP.maxMoment(), my_speed );
            if ( dir_diff < 0.0 ) dir_diff = 0.0;
            my_speed *= ptype.playerDecay();
            ++n_turn;
        }

        if ( n_turn >= 2 )
        {
#ifdef DEBUG_PRINT
            dlog.addText( Logger::DRIBBLE,
                          __FILE__": (createTargetPoints) canceled dash_angle=%.1f n_turn=%d",
                          dash_angle.degree(), n_turn );
#endif
            continue;
        }
        dlog.addText(Logger::DRIBBLE," start back for da:%.2f,turn:%d",dash_angle.degree(),n_turn);
        simulateKickTurnBackDashes( wm, dash_angle, n_turn );
    }
    dlog.addText( Logger::DRIBBLE,"start advance sim");
    for ( int a = 0; a < angle_div; a+=1 )
    {
        if (!PlayerAgent::canProcessMore())
            return;
        AngleDeg dash_angle = wm.self().body() + ( angle_step * a );

        //
        // angle filter
        //

        if ( M_first_ball_pos.x < -15.0
             && dash_angle.abs() > 100.0 )
        {
#ifdef DEBUG_PRINT
            dlog.addText( Logger::DRIBBLE,
                          __FILE__": (createTargetPoints) canceled(1) dash_angle=%.1f",
                          dash_angle.degree() );
#endif
            continue;
        }

        if ( wm.self().pos().x < -36.0
             && wm.self().pos().absY() < 20.0
             && dash_angle.abs() > 60.0 )
        {
#ifdef DEBUG_PRINT
            dlog.addText( Logger::DRIBBLE,
                          __FILE__": (createTargetPoints) canceled(2) dash_angle=%.1f",
                          dash_angle.degree() );
#endif
            continue;
        }

        int n_turn = 0;

        double my_speed = my_first_speed * ptype.playerDecay(); // first action is kick
        double dir_diff = AngleDeg( angle_step * a ).abs();

        while ( dir_diff > 10.0 )
        {
            dir_diff -= ptype.effectiveTurn( SP.maxMoment(), my_speed );
            if ( dir_diff < 0.0 ) dir_diff = 0.0;
            my_speed *= ptype.playerDecay();
            ++n_turn;
        }

        if ( n_turn >= 3 )
        {
#ifdef DEBUG_PRINT
            dlog.addText( Logger::DRIBBLE,
                          __FILE__": (createTargetPoints) canceled dash_angle=%.1f n_turn=%d",
                          dash_angle.degree(), n_turn );
#endif
            continue;
        }

        if ( angle_step * a < 180.0 )
        {
            dash_angle -= dir_diff;
        }
        else
        {
            dash_angle += dir_diff;
        }

        simulateKickTurnsDashesAdvance( wm, dash_angle, n_turn, a );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
ShortDribbleGenerator::simulateKickTurnBackDashes(  const WorldModel & wm,
                                                    const AngleDeg & dash_angle,
                                                    const int n_turn )
{
    //static const int max_dash = 5;
    static const int max_dash = 6; // 2009-06-29
    static const int min_dash = 2;
    //static const int min_dash = 1;

    static std::vector< Vector2D > self_cache;

    //
    // create self position cache
    //
    createSelfBackCache( wm, dash_angle, n_turn, max_dash, self_cache );

    const ServerParam & SP = ServerParam::i();
    const PlayerType & ptype = wm.self().playerType();

    const Vector2D trap_rel
            = Vector2D::polar2vector( ptype.playerSize() + ptype.kickableMargin() * 0.1 + SP.ballSize(),
                                      dash_angle );

    const double max_x = ( SP.keepawayMode()
                           ? SP.keepawayLength() * 0.5 - 1.5
                           : SP.pitchHalfLength() - 1.0 );
    const double max_y = ( SP.keepawayMode()
                           ? SP.keepawayWidth() * 0.5 - 1.5
                           : SP.pitchHalfWidth() - 1.0 );

#ifdef DEBUG_PRINT
    dlog.addText( Logger::DRIBBLE,
                  __FILE__": (simulateKickTurnsDashes) dash_angle=%.1f n_turn=%d",
                  dash_angle.degree(), n_turn );
#endif

    for ( int n_dash = max_dash; n_dash >= min_dash; --n_dash )
    {
        const Vector2D ball_trap_pos = self_cache[n_turn + n_dash] + trap_rel;

        ++M_total_count;

#ifdef DEBUG_PRINT
        dlog.addText( Logger::DRIBBLE,
                      "back%d: n_turn=%d n_dash=%d ball_trap=(%.3f %.3f)",
                      M_total_count,
                      n_turn, n_dash,
                      ball_trap_pos.x, ball_trap_pos.y );
#endif
        if ( ball_trap_pos.absX() > max_x
             || ball_trap_pos.absY() > max_y )
        {
#ifdef DEBUG_PRINT_FAILED_COURSE
            dlog.addText( Logger::DRIBBLE,
                          "%d: xxx out of pitch" );
            debug_paint_failed( M_total_count, ball_trap_pos );
#endif
            continue;
        }

        const double term
                = ( 1.0 - std::pow( SP.ballDecay(), 1 + n_turn + n_dash ) )
                / ( 1.0 - SP.ballDecay() );
        const Vector2D first_vel = ( ball_trap_pos - M_first_ball_pos ) / term;
        const Vector2D kick_accel = first_vel - M_first_ball_vel;
        const double kick_power = kick_accel.r() / wm.self().kickRate();

        if ( kick_power > SP.maxPower() + 2
             || kick_accel.r2() > std::pow( SP.ballAccelMax(), 2 )
             || first_vel.r2() > std::pow( SP.ballSpeedMax(), 2 ) )
        {
#ifdef DEBUG_PRINT_FAILED_COURSE
            dlog.addText( Logger::DRIBBLE,
                          "%d: xxx cannot kick. first_vel=(%.1f %.1f, r=%.2f) accel=(%.1f %.1f)r=%.2f power=%.1f",
                          M_total_count,
                          first_vel.x, first_vel.y, first_vel.r(),
                          kick_accel.x, kick_accel.y, kick_accel.r(),
                          kick_power );
            debug_paint_failed( M_total_count, ball_trap_pos );
#endif
            continue;
        }

        if ( ( M_first_ball_pos + first_vel ).dist2( self_cache[0] )
             < std::pow( ptype.playerSize() + SP.ballSize() + (wm.ball().pos().dist(Vector2D(52,0))< 15?0:0.1), 2 ) )
        {
#ifdef DEBUG_PRINT_FAILED_COURSE
            dlog.addText( Logger::DRIBBLE,
                          "%d: xxx collision. first_vel=(%.1f %.1f, r=%.2f) accel=(%.1f %.1f)r=%.2f power=%.1f",
                          M_total_count,
                          first_vel.x, first_vel.y, first_vel.r(),
                          kick_accel.x, kick_accel.y, kick_accel.r(),
                          kick_power );
            debug_paint_failed( M_total_count, ball_trap_pos );
#endif
            continue;
        }

        int opp_min_dif = 3;
        bool safe_with_pos_count = true;
        int danger = 0;
        if( !can_opp_reach(wm,M_first_ball_pos,first_vel,ball_trap_pos,
        1 + n_turn + n_dash,opp_min_dif, safe_with_pos_count, danger))
        {
            safe_with_pos_count = false;
            CooperativeAction::Ptr ptr( new Dribble( wm.self().unum(),
                                                     ball_trap_pos,
                                                     first_vel.r(),
                                                     1, // n_kick
                                                     n_turn,
                                                     n_dash,
                                                     "shortBackDribble",
                                                     opp_min_dif,
                                                     safe_with_pos_count,
                                                     danger) );
            ptr->setIndex( M_total_count );
            M_courses.push_back( ptr );

#ifdef DEBUG_PRINT_SUCCESS_COURSE
            if (safe_with_pos_count){
                dlog.addCircle( Logger::DRIBBLE,
                                ball_trap_pos.x, ball_trap_pos.y, 0.1,
                                "#00ff00" );
                dlog.addText( Logger::DRIBBLE,
                              "%d: ok safe trap_pos=(%.2f %.2f) first_vel=(%.1f %.1f, r=%.2f) n_turn=%d n_dash=%d",
                              M_total_count,
                              ball_trap_pos.x, ball_trap_pos.y,
                              first_vel.x, first_vel.y, first_vel.r(),
                              n_turn, n_dash );
            }
            else{
                dlog.addText( Logger::DRIBBLE,
                              "%d: ok notsafe trap_pos=(%.2f %.2f) first_vel=(%.1f %.1f, r=%.2f) n_turn=%d n_dash=%d danger=%d",
                              M_total_count,
                              ball_trap_pos.x, ball_trap_pos.y,
                              first_vel.x, first_vel.y, first_vel.r(),
                              n_turn, n_dash, danger);
                dlog.addCircle( Logger::DRIBBLE,
                                ball_trap_pos.x, ball_trap_pos.y, 0.1,
                                "#41A2F4" );
            }

            char num[8]; snprintf( num, 8, "%d", M_total_count );
            dlog.addMessage( Logger::DRIBBLE,
                             ball_trap_pos, num );
#endif
        }else{
            debug_paint_failed( M_total_count, ball_trap_pos );
        }
    }
}
#include <rcsc/soccer_math.h>
void
ShortDribbleGenerator::simulateKickTurnsDashesAdvance( const WorldModel & wm,
                                                       const AngleDeg & dash_angle,
                                                       const int n_turn, int a )
{
    int max_dash = 7;
    if(wm.gameMode().type() == GameMode::PenaltyTaken_)
        max_dash = 15;
    int min_dash = 2;
    if( (M_first_ball_pos.x > -10 && M_first_ball_pos.absY() > 20)
            || (M_first_ball_pos.x > wm.offsideLineX() - 10) ){
        if (dash_angle.abs() < 45){
            max_dash = 20;
        }
    }
    if(max_dash == 7){
        if (a%4!=0){
            return;
        }
    }

    //static const int min_dash = 1;

    static std::vector< Vector2D > self_cache;

    //
    // create self position cache
    //
    createSelfCache( wm, dash_angle, n_turn, max_dash, self_cache );

    const ServerParam & SP = ServerParam::i();
    const PlayerType & ptype = wm.self().playerType();

    const double max_x = ( SP.keepawayMode()
                           ? SP.keepawayLength() * 0.5 - 1.5
                           : SP.pitchHalfLength() - 1.0 );
    const double max_y = ( SP.keepawayMode()
                           ? SP.keepawayWidth() * 0.5 - 1.5
                           : SP.pitchHalfWidth() - 1.0 );

    for (double trap_angle = -70; trap_angle <= 70;trap_angle += 20){
        if (trap_angle == -10){
            trap_angle = 0;
        }else{
            if (trap_angle > -40 && trap_angle < 40)
                continue;
        }
        if(trap_angle == 60){
            trap_angle = 70;
        }

        double dist_ball_to_body = 0.35;
        if(trap_angle == 0)
            dist_ball_to_body = 0.2;
        const Vector2D trap_rel
                = Vector2D::polar2vector( ptype.playerSize() + ptype.kickableMargin() * dist_ball_to_body + SP.ballSize(),
                                          dash_angle + AngleDeg(trap_angle));
#ifdef DEBUG_PRINT
        dlog.addText( Logger::DRIBBLE,
                      __FILE__": (simulateKickTurnsDashes) dash_angle=%.1f n_turn=%d",
                      dash_angle.degree(), n_turn );
#endif

        for ( int n_dash = max_dash; n_dash >= min_dash; --n_dash )
        {
            if(n_dash > 7 && trap_angle!= 0)
                continue;
            const Vector2D ball_trap_pos = self_cache[n_turn + n_dash] + trap_rel;

            ++M_total_count;

#ifdef DEBUG_PRINT
            dlog.addText( Logger::DRIBBLE,
                          "%d: n_turn=%d n_dash=%d ball_trap=(%.3f %.3f) body=%.1f",
                          M_total_count,
                          n_turn, n_dash,
                          ball_trap_pos.x, ball_trap_pos.y, dash_angle );
#endif
            if ( ball_trap_pos.absX() > max_x
                 || ball_trap_pos.absY() > max_y )
            {
#ifdef DEBUG_PRINT_FAILED_COURSE
                dlog.addText( Logger::DRIBBLE,
                              "%d: xxx out of pitch" );
                debug_paint_failed( M_total_count, ball_trap_pos );
#endif
                continue;
            }

            const double term
                    = ( 1.0 - std::pow( SP.ballDecay(), 1 + n_turn + n_dash ) )
                    / ( 1.0 - SP.ballDecay() );
            const Vector2D first_vel = ( ball_trap_pos - M_first_ball_pos ) / term;
            const Vector2D kick_accel = first_vel - M_first_ball_vel;
            double kickrate;
            if(wm.self().isKickable()){
                kickrate = wm.self().kickRate();
            }else{
                kickrate
                    = kick_rate( 0.3,
                                 ( (M_first_ball_pos - wm.self().pos()).th() - wm.self().body() ).degree(),
                                 wm.self().playerType().kickPowerRate(), //ServerParam::i().kickPowerRate(),
                                 ServerParam::i().ballSize(),
                                 wm.self().playerType().playerSize(),
                                 wm.self().playerType().kickableMargin() );
            }
            const double kick_power = kick_accel.r() / kickrate; // todo RF only on step?

            if ( kick_power > SP.maxPower()
                 || kick_accel.r2() > std::pow( SP.ballAccelMax(), 2 )
                 || first_vel.r2() > std::pow( SP.ballSpeedMax(), 2 ) )
            {
#ifdef DEBUG_PRINT_FAILED_COURSE
                dlog.addText( Logger::DRIBBLE,
                              "%d: xxx cannot kick. first_vel=(%.1f %.1f, r=%.2f) accel=(%.1f %.1f)r=%.2f power=%.1f",
                              M_total_count,
                              first_vel.x, first_vel.y, first_vel.r(),
                              kick_accel.x, kick_accel.y, kick_accel.r(),
                              kick_power );
                debug_paint_failed( M_total_count, ball_trap_pos );
#endif
                continue;
            }

            if ( ( M_first_ball_pos + first_vel ).dist2( self_cache[0] )
                 < std::pow( ptype.playerSize() + SP.ballSize() + 0.1, 2 ) )
            {
#ifdef DEBUG_PRINT_FAILED_COURSE
                dlog.addText( Logger::DRIBBLE,
                              "%d: xxx collision. first_vel=(%.1f %.1f, r=%.2f) accel=(%.1f %.1f)r=%.2f power=%.1f",
                              M_total_count,
                              first_vel.x, first_vel.y, first_vel.r(),
                              kick_accel.x, kick_accel.y, kick_accel.r(),
                              kick_power );
                debug_paint_failed( M_total_count, ball_trap_pos );
#endif
                continue;
            }

            int opp_min_dif = 3;
            bool safe_with_pos_count = true;
            int danger = 0;
            if( !can_opp_reach(wm,M_first_ball_pos,first_vel,ball_trap_pos,
                     1 + n_turn + n_dash,opp_min_dif, safe_with_pos_count, danger))
            {
                safe_with_pos_count = false;
                CooperativeAction::Ptr ptr( new Dribble( wm.self().unum(),
                                                         ball_trap_pos,
                                                         self_cache[n_turn + n_dash],
                                            first_vel.r(),
                                            1, // n_kick
                                            n_turn,
                                            n_dash,
                                            "shortDribbleAdvance",
                                            dash_angle.degree(),
                                            opp_min_dif,
                                            safe_with_pos_count,
                                            danger) );
                ptr->setIndex( M_total_count );
                M_courses.push_back( ptr );

#ifdef DEBUG_PRINT_SUCCESS_COURSE
                if (safe_with_pos_count){
                    dlog.addCircle( Logger::DRIBBLE,
                                    ball_trap_pos.x, ball_trap_pos.y, 0.1,
                                    "#00ff00" );
                    dlog.addText( Logger::DRIBBLE,
                                  "%d: ok safe trap_pos=(%.2f %.2f) first_vel=(%.1f %.1f, r=%.2f) n_turn=%d n_dash=%d",
                                  M_total_count,
                                  ball_trap_pos.x, ball_trap_pos.y,
                                  first_vel.x, first_vel.y, first_vel.r(),
                                  n_turn, n_dash );
                }
                else{
                    dlog.addText( Logger::DRIBBLE,
                                  "%d: ok nosafe trap_pos=(%.2f %.2f) first_vel=(%.1f %.1f, r=%.2f) n_turn=%d n_dash=%d danger=%d",
                                  M_total_count,
                                  ball_trap_pos.x, ball_trap_pos.y,
                                  first_vel.x, first_vel.y, first_vel.r(),
                                  n_turn, n_dash, danger );
                    dlog.addCircle( Logger::DRIBBLE,
                                    ball_trap_pos.x, ball_trap_pos.y, 0.1,
                                    "#41A2F4" );
                }

                char num[8]; snprintf( num, 8, "%d", M_total_count );
                dlog.addMessage( Logger::DRIBBLE,
                                 ball_trap_pos, num );
#endif
            }else{
                debug_paint_failed( M_total_count, ball_trap_pos );
            }
        }
    }

}

void
ShortDribbleGenerator::createSelfCache( const WorldModel & wm,
                                        const AngleDeg & dash_angle,
                                        const int n_turn,
                                        const int n_dash,
                                        std::vector< Vector2D > & self_cache )
{
    const ServerParam & SP = ServerParam::i();
    const PlayerType & ptype = wm.self().playerType();

    self_cache.clear();

    StaminaModel stamina_model = wm.self().staminaModel();

    Vector2D my_pos = wm.self().pos();
    Vector2D my_vel = wm.self().vel();

    my_pos += my_vel;
    my_vel *= ptype.playerDecay();

    self_cache.push_back( my_pos ); // first element is next cycle just after kick

    for ( int i = 0; i < n_turn; ++i )
    {
        my_pos += my_vel;
        my_vel *= ptype.playerDecay();
        self_cache.push_back( my_pos );
    }

    stamina_model.simulateWaits( ptype, 1 + n_turn );

    const Vector2D unit_vec = Vector2D::polar2vector( 1.0, dash_angle );

    for ( int i = 0; i < n_dash; ++i )
    {
        double available_stamina = std::max( 0.0,
                                             stamina_model.stamina() - SP.recoverDecThrValue() - 300.0 );
        double dash_power = std::min( available_stamina, SP.maxDashPower() );
        Vector2D dash_accel = unit_vec.setLengthVector( dash_power * ptype.dashPowerRate() * stamina_model.effort() );

        my_vel += (dash_accel*0.9);
        my_pos += my_vel;
        my_vel *= ptype.playerDecay();

        stamina_model.simulateDash( ptype, dash_power );

        dlog.addTriangle(Logger::DRIBBLE,my_pos + Vector2D::polar2vector(0.06,-120),my_pos + Vector2D::polar2vector(0.06,0),my_pos + Vector2D::polar2vector(0.06,+120),0,0,0,true);
        self_cache.push_back( my_pos );
    }

}

void
ShortDribbleGenerator::createSelfBackCache( const WorldModel & wm,
                                            const AngleDeg & dash_angle,
                                            const int n_turn,
                                            const int n_dash,
                                            std::vector< Vector2D > & self_cache )
{
    const ServerParam & SP = ServerParam::i();
    const PlayerType & ptype = wm.self().playerType();

    self_cache.clear();

    StaminaModel stamina_model = wm.self().staminaModel();

    Vector2D my_pos = wm.self().pos();
    Vector2D my_vel = wm.self().vel();

    my_pos += my_vel;
    my_vel *= ptype.playerDecay();

    self_cache.push_back( my_pos ); // first element is next cycle just after kick

    for ( int i = 0; i < n_turn; ++i )
    {
        my_pos += my_vel;
        my_vel *= ptype.playerDecay();
        self_cache.push_back( my_pos );
    }

    stamina_model.simulateWaits( ptype, 1 + n_turn );

    const Vector2D unit_vec = Vector2D::polar2vector( 1.0, dash_angle);

    for ( int i = 0; i < n_dash; ++i )
    {
        double available_stamina = std::max( 0.0,
                                             stamina_model.stamina() - SP.recoverDecThrValue() - 300.0 );
        double dash_power = std::min( available_stamina, SP.maxDashPower() );

        Vector2D dash_accel = unit_vec.setLengthVector( SP.maxDashPower() * ptype.effortMax() * ServerParam::i().dashDirRate( 180.0 ) * ptype.dashPowerRate() );
        my_vel += dash_accel;
        my_pos += my_vel;
        my_vel *= ptype.playerDecay();

        stamina_model.simulateDash( ptype, dash_power );

        self_cache.push_back( my_pos );
    }

}

bool ShortDribbleGenerator::can_opp_reach(const WorldModel & wm, const Vector2D start_ball,
                                     const Vector2D kick_vel, const Vector2D ball_trap_pos,
                                      const int action_cycle, int & opp_min_dif, bool & safe_with_pos_count
                                      , int & worst_danger){
    dlog.addText(Logger::DRIBBLE,"--canopp reach b(%.1f,%.1f)-(%.1f,%.1f)->(%.1f,%.1f),c:%d",start_ball.x,start_ball.y,kick_vel.x,kick_vel.y,ball_trap_pos.x,ball_trap_pos.y,action_cycle);
    const ServerParam & sp = ServerParam::i();
    const rcsc::AngleDeg ball_move_angle = ( ball_trap_pos - M_first_ball_pos ).th();

    double control_aria_safe_thr = 0;
    if(  FieldAnalyzer::isMiracle(wm)
          || FieldAnalyzer::isKN2C(wm) ){
        control_aria_safe_thr = 0.2;
    }
    bool use_tackle = false;
    if(  FieldAnalyzer::isMT(wm) ){
        use_tackle = false;
    }

    const PlayerPtrCont::const_iterator o_end = wm.opponentsFromSelf().end();
    for ( PlayerPtrCont::const_iterator o = wm.opponentsFromSelf().begin();
          o != o_end;
          ++o )
    {
        if ( (*o)->distFromSelf() > 45.0 ) break;
        const Vector2D ball_to_opp_rel = ( (*o)->pos() - M_first_ball_pos ).rotatedVector( -ball_move_angle );
        const PlayerType * ptype = (*o)->playerTypePtr();

        Vector2D ball_pos = start_ball;
        Vector2D ball_vel = kick_vel;
        ball_pos += ball_vel;
        ball_vel *= sp.ballDecay();
        Vector2D opp_pos = (*o)->pos();
        Vector2D opp_vel = (*o)->vel();
        opp_pos += Vector2D::polar2vector(ptype->playerSpeedMax() * (opp_vel.r() / 0.4),opp_vel.th());  // todo RF maxSpeed*opp_vel?
        dlog.addText(Logger::DRIBBLE,"--opp%d p(%.1f,%.1f),pp(%.1f,%.1f),v(%.1f,%.1f)",(*o)->unum(),(*o)->pos().x,(*o)->pos().y,opp_pos.x,opp_pos.y,opp_vel.x,opp_vel.y);
        int bonus_step = 0;
        if ( ball_trap_pos.x < 30.0)
        {
            bonus_step += 1;
        }

        if ( ball_trap_pos.x < 0.0 )
        {
            bonus_step += 1;
        }

        if ( (*o)->isTackling() )
        {
            bonus_step = -5;
        }

        int max_pos_count_effect_front = Setting::i()->mChainAction->mDribblePosCountMaxFrontOpp;
        int max_pos_count_effect_behind = Setting::i()->mChainAction->mDribblePosCountMaxBehindOpp;
        double pos_count_effect_factor = Setting::i()->mChainAction->mDribblePosCountZ;
        if ( ball_to_opp_rel.x > 0.5 )
        {
            bonus_step += bound( 0, static_cast<int>(/*std::ceil*/((*o)->posCount() * pos_count_effect_factor)), max_pos_count_effect_front );
        }
        else
        {
            if(wm.ball().pos().x > 15 && wm.ball().pos().x < 45 && wm.ball().pos().x > wm.theirOffenseLineX() - 10){
                bonus_step += bound( 0, static_cast<int>(/*std::ceil*/((*o)->posCount() * pos_count_effect_factor)), max_pos_count_effect_behind / 2 );
            }else{
                bonus_step += bound( 0, static_cast<int>(/*std::ceil*/((*o)->posCount() * pos_count_effect_factor)), max_pos_count_effect_behind );
            }

        }
        for(int c = 1; c <= action_cycle; c++){


            int opp_turn_cycle;
            int opp_dash_cycle;
            int opp_view_cycle;
            int opp_cycle = (*o)->cycles_to_cut_ball_with_safe_thr_dist(wm,
                                                                         ball_pos,
                                                                         c,
                                                                         use_tackle,
                                                                         opp_dash_cycle,
                                                                         opp_turn_cycle,
                                                                         opp_view_cycle,
                                                                         opp_pos,
                                                                         opp_vel,
                                                                         control_aria_safe_thr);

            dlog.addText(Logger::DRIBBLE,"----c:%d,opptc:%d,oppdc:%d,oppc:%d,bonus_step:%d",c,opp_turn_cycle,opp_dash_cycle,opp_cycle,bonus_step);
            if(  FieldAnalyzer::isMiracle(wm)
                  || FieldAnalyzer::isKN2C(wm) ){
//                opp_cycle = opp_turn_cycle + opp_dash_cycle;
            }
            int opp_reach_cycle = opp_cycle - bonus_step;
            int diff = opp_reach_cycle - c;

            // danger
            int danger = 0;
            if  (diff <= 3){
                danger = std::max(3 - diff, 0);
            }

            if (danger > worst_danger){
                worst_danger = danger;
            }

            if (opp_reach_cycle <= c){
                return true;
            }
            opp_min_dif = std::min(opp_min_dif, opp_cycle - bonus_step - c);
            ball_pos += ball_vel;
            ball_vel *= sp.ballDecay();
        }
    }
    return false;
}
