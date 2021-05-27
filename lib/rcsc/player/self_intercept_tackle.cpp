// -*-c++-*-

/*!
  \file self_intercept_v13.cpp
  \brief self intercept predictor for rcssserver v13+ Source File
*/

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 3 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "self_intercept_tackle.h"

#include "world_model.h"
#include "intercept_table.h"
#include "self_object.h"
#include "ball_object.h"

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/common/player_type.h>
#include <rcsc/geom/segment_2d.h>
#include <rcsc/geom/vector_2d.h>
#include <rcsc/soccer_math.h>
#include <rcsc/math_util.h>
#include <rcsc/timer.h>

#define DEBUG_PROFILE

#define DEBUG_PRINT

#define DEBUG_PRINT_ONE_STEP
// #define DEBUG_PRINT_SHORT_STEP

// #define DEBUG_PRINT_LONG_STEP
// #define DEBUG_PRINT_LONG_STEP_LEVEL_1
// #define DEBUG_PRINT_LONG_STEP_LEVEL_2
// #define DEBUG_PRINT_LONG_STEP_LEVEL_3


namespace rcsc {

namespace {
//const double control_area_buf = 0.1;
const double control_area_buf = 0.15; // 2009-07-03
//const double control_area_buf = 0.2; // 2009-07-04
}


const int SelfInterceptTackle::MAX_SHORT_STEP = 7;
const double SelfInterceptTackle::MIN_TURN_THR = 12.5;
//const double SelfInterceptTackle::MIN_TURN_THR = 15.0;
const double SelfInterceptTackle::BACK_DASH_THR_ANGLE = 100.0;

/*-------------------------------------------------------------------*/
/*!

 */
void
SelfInterceptTackle::predict( const int max_cycle,
                              std::vector< InterceptInfo > & self_cache ) const
{
#ifdef DEBUG_PROFILE
    rcsc::Timer timer;
#endif

     #ifdef DEBUG_PRINT
         dlog.addText( Logger::INTERCEPT,
                       __FILE__": ------------- predict self intercept tackle ---------------" );
     #endif

    if ( M_ball_pos_cache.size() < 2 )
    {
        dlog.addText( Logger::INTERCEPT,
                      __FILE__": no ball position cache." );
        std::cerr << M_world.self().unum() << ' '
                  << M_world.time()
                  << " no ball position cache." << std::endl;
        return;
    }

    const bool save_recovery = ( M_world.self().staminaModel().capacity() == 0.0
                                 ? false
                                 : true );

    size_t find_intercept = 0;
    predictOneStep( self_cache );
    find_intercept = self_cache.size();
#ifdef DEBUG_PROFILE
    dlog.addText( Logger::INTERCEPT,
                  __FILE__" >>>>>>>>predict one step size %d",
                  find_intercept );
#endif

    predictShortStep( max_cycle, save_recovery, self_cache );
#ifdef DEBUG_PROFILE
    dlog.addText( Logger::INTERCEPT,
                  __FILE__" >>>>>>>>predict short step size %d",
                  self_cache.size() - find_intercept );
#endif
    find_intercept = self_cache.size();

    predictLongStep( max_cycle, save_recovery, self_cache );
#ifdef DEBUG_PROFILE
    dlog.addText( Logger::INTERCEPT,
                  __FILE__" >>>>>>>>predict long step size %d",
                  self_cache.size() - find_intercept );
#endif
    find_intercept = self_cache.size();

#ifdef SELF_INTERCEPT_USE_NO_SAVE_RECEVERY
    predictLongStep( max_cycle, false, self_cache );
#endif

    std::sort( self_cache.begin(), self_cache.end(), InterceptInfo::Cmp() );

#ifdef DEBUG_PROFILE
    dlog.addText( Logger::INTERCEPT,
                  __FILE__" (predict) elapsed %f [ms]",
                  timer.elapsedReal() );
#endif


#ifdef DEBUG_PRINT
    dlog.addText( Logger::INTERCEPT,
                  __FILE__"(SelfIntercept) solution size = %d",
                  self_cache.size() );
    const std::vector< InterceptInfo >::iterator end = self_cache.end();
    for ( std::vector< InterceptInfo >::iterator it = self_cache.begin();
          it != end;
          ++it )
    {
        Vector2D bpos = M_world.ball().inertiaPoint( it->reachCycle() );
        dlog.addText( Logger::INTERCEPT,
                      __FILE__"(SelfIntercept) type=%d cycle=%d (turn=%d dash=%d)"
                      " power=%.2f angle=%.1f"
                      " self_pos=(%.2f %.2f) bdist=%.3f stamina=%.1f",
                      it->mode(),
                      it->reachCycle(),
                      it->turnCycle(),
                      it->dashCycle(),
                      it->dashPower(),
                      it->dashAngle().degree(),
                      it->selfPos().x, it->selfPos().y,
                      it->ballDist(),
                      it->stamina() );
    }
#endif
}

/*-------------------------------------------------------------------*/
/*!

 */
void
SelfInterceptTackle::predictOneStep( std::vector< InterceptInfo > & self_cache ) const
{
    const double control_area = ServerParam::i().tackleDist();
    ///////////////////////////////////////////////////////////
    // current distance is too far. never reach by one dash
    if ( M_world.ball().distFromSelf()
         > ( ServerParam::i().ballSpeedMax()
             + M_world.self().playerType().realSpeedMax()
             + control_area ) )
    {
#ifdef DEBUG_PRINT_ONE_STEP
        dlog.addText( Logger::INTERCEPT,
                      "__1 dash: too far. never reach" );
#endif
        return;
    }

    predictOneDash( self_cache );
    predictNoDash( self_cache );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
SelfInterceptTackle::predictNoDash( std::vector< InterceptInfo > & self_cache ) const
{
    const ServerParam & SP = ServerParam::i();
    const SelfObject & self = M_world.self();

    const Vector2D my_next = self.pos() + self.vel();
    const Vector2D ball_next = M_world.ball().pos() + M_world.ball().vel();
    const double control_area = ServerParam::i().tackleDist();
    double max_turn = self.playerType().effectiveTurn( SP.maxMoment(), self.vel().r() );
    std::vector<double> good_angle;
    for(double t = -max_turn; t <= max_turn; t += 10){
        AngleDeg new_body(self.body().degree() + t);
        const Vector2D next_ball_rel = ( ball_next - my_next ).rotatedVector( -new_body );
        const double ball_noise = M_world.ball().vel().r() * ServerParam::i().ballRand();
        const double next_ball_dist = next_ball_rel.r();
        if ( next_ball_rel.x > control_area - 0.15 - ball_noise
             || next_ball_rel.absY() > 0.8
             || next_ball_rel.x < 0 )
        {
    #ifdef DEBUG_PRINT_ONE_STEP
            dlog.addText( Logger::INTERCEPT,
                          "____No dash, out of control area with t=%0.1f. area=%.3f  ball_dist=%.3f  noise=%.3f",
                          t,
                          control_area,
                          next_ball_dist,
                          ball_noise );
    #endif
            continue;
        }
    #ifdef DEBUG_PRINT_ONE_STEP
        dlog.addText( Logger::INTERCEPT,
                      "____No dash, in control area with t=%0.1f. area=%.3f  ball_dist=%.3f  noise=%.3f ballrelx=%.1f ballrely=%.1f",
                      t,
                      control_area,
                      next_ball_dist,
                      ball_noise,
                      next_ball_rel.x,
                      next_ball_rel.y);
    #endif
        good_angle.push_back(t);
    }

    if( good_angle.size() == 0 )
    {
        dlog.addText(Logger::INTERCEPT, "cant find intercept tackle no dash");
        return false;
    }

    //
    // at least, player can stop the ball
    //

    double best_angle = 1000;
    double min_dist = 1000;
    for(auto& t : good_angle)
    {
        double dist = Line2D(self.inertiaPoint(1), t).dist(ball_next);
        if(dist < min_dist)
        {
            min_dist = dist;
            best_angle = t;
        }
    }
    StaminaModel stamina_model = self.staminaModel();
    stamina_model.simulateWait( self.playerType() );
    InterceptInfo tmp ( InterceptInfo::NORMAL,
                         1, 0, // 1 turn
                         0.0, 0.0,
                         my_next,
                         self.inertiaPoint(1).dist(ball_next),
                         stamina_model.stamina() );
    tmp.setTackle();
    self_cache.push_back( tmp );
#ifdef DEBUG_PRINT_ONE_STEP
    dlog.addText( Logger::INTERCEPT,
                  "-->Sucess! No dash, next_dist=%.3f",
                  self.inertiaPoint(1).dist(ball_next) );
#endif
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
SelfInterceptTackle::predictOneDash( std::vector< InterceptInfo > & self_cache ) const
{
    static std::vector< InterceptInfo > tmp_cache;

    const ServerParam & SP = ServerParam::i();
    const BallObject & ball = M_world.ball();
    const SelfObject & self = M_world.self();
    const PlayerType & ptype = self.playerType();

    const Vector2D ball_next = ball.pos() + ball.vel();

    const double control_area = SP.tackleDist();
    const double dash_angle_step = std::max( 5.0, SP.dashAngleStep() );
    const double min_dash_angle = ( -180.0 < SP.minDashAngle() && SP.maxDashAngle() < 180.0
                                    ? SP.minDashAngle()
                                    : dash_angle_step * static_cast< int >( -180.0 / dash_angle_step ) );
    const double max_dash_angle = ( -180.0 < SP.minDashAngle() && SP.maxDashAngle() < 180.0
                                    ? SP.maxDashAngle() + dash_angle_step * 0.5
                                    : dash_angle_step * static_cast< int >( 180.0 / dash_angle_step ) - 1.0 );

#ifdef DEBUG_PRINT_ONE_STEP
    dlog.addText( Logger::INTERCEPT,
                  "(predictOneDash) min_angle=%.1f max_angle=%.1f, step_angle=%.1f",
                  min_dash_angle, max_dash_angle, dash_angle_step );
#endif

    tmp_cache.clear();

    for ( double dir = min_dash_angle;
          dir < max_dash_angle;
          dir += dash_angle_step )
    {
        const AngleDeg dash_angle = self.body() + SP.discretizeDashAngle( SP.normalizeDashAngle( dir ) );
        const double dash_rate = self.dashRate() * SP.dashDirRate( dir );

#ifdef DEBUG_PRINT_ONE_STEP
        dlog.addText( Logger::INTERCEPT,
                      "(predictOneDash) dir=%.1f angle=%.1f dash_rate=%f",
                      dir, dash_angle.degree(), dash_rate );
#endif

        //
        // check receovery save dash
        //

        Vector2D max_forward_accel
                = Vector2D::polar2vector( SP.maxDashPower() * dash_rate,
                                          dash_angle );
        Vector2D max_back_accel
                = Vector2D::polar2vector( SP.minDashPower() * dash_rate,
                                          dash_angle );
        ptype.normalizeAccel( self.vel(), &max_forward_accel );
        ptype.normalizeAccel( self.vel(), &max_back_accel );

        {
            InterceptInfo info;
            if ( predictOneDashAdjust( dash_angle,
                                       max_forward_accel,
                                       max_back_accel,
                                       control_area,
                                       &info ) )
            {
#ifdef DEBUG_PRINT_ONE_STEP
                dlog.addText( Logger::INTERCEPT,
                              "****>Register 1 dash intercept(2) mode=%d power=%.1f dir=%.1f pos=(%.1f %.1f) stamina=%.1f",
                              info.mode(),
                              info.dashPower(),
                              info.dashAngle().degree(),
                              info.selfPos().x, info.selfPos().y,
                              info.stamina() );
#endif
                tmp_cache.push_back( info );
                continue;
            }
        }
#ifdef DEBUG_PRINT_ONE_STEP
        dlog.addText( Logger::INTERCEPT,
                      "____(predictOneDash) failed. dash_angle=%.1f",
                      dash_angle.degree() );
#endif
    }

    if ( tmp_cache.empty() )
    {
        return false;
    }

    const double safety_ball_dist = std::max( control_area - 0.2 - ball.vel().r() * SP.ballRand(),
                                              ptype.playerSize() + SP.ballSize() + ptype.kickableMargin() * 0.4 );
#ifdef DEBUG_PRINT_ONE_STEP
    dlog.addText( Logger::INTERCEPT,
                  "decide best 1 step interception. size=%d safety_ball_dist=%.3f",
                  tmp_cache.size(), safety_ball_dist );
#endif

    const InterceptInfo * best = &(tmp_cache.front());
#ifdef DEBUG_PRINT_ONE_STEP
    dlog.addText( Logger::INTERCEPT,
                  "____ turn=%d dash=%d power=%.1f dir=%.1f ball_dist=%.3f stamina=%.1f",
                  best->turnCycle(), best->dashCycle(),
                  best->dashPower(), best->dashAngle().degree(),
                  best->ballDist(), best->stamina() );
#endif

    double best_dist = 1000;
    const std::vector< InterceptInfo >::iterator end = tmp_cache.end();
    std::vector< InterceptInfo >::iterator it = tmp_cache.begin();
    ++it;
    for ( ; it != end; ++it )
    {
#ifdef DEBUG_PRINT_ONE_STEP
        dlog.addText( Logger::INTERCEPT,
                      "____ turn=%d dash=%d power=%.1f dir=%.1f ball_dist=%.3f stamina=%.1f",
                      it->turnCycle(), it->dashCycle(),
                      it->dashPower(), it->dashAngle().degree(),
                      it->ballDist(), it->stamina() );
#endif
        if ( it->ballDist() < best->ballDist()){
            best = &(*it);
        }
    }

#ifdef DEBUG_PRINT_ONE_STEP
    dlog.addText( Logger::INTERCEPT,
                  "<<<<< Register best cycle=%d(t=%d d=%d) my_pos=(%.2f %.2f) ball_dist=%.3f stamina=%.1f",
                  best->reachCycle(), best->turnCycle(), best->dashCycle(),
                  best->ballDist(),
                  best->selfPos().x, best->selfPos().y,
                  best->stamina() );
#endif

    self_cache.push_back( *best );
    return true;
}


/*-------------------------------------------------------------------*/
/*!

 */
bool
SelfInterceptTackle::predictOneDashAdjust( const AngleDeg & dash_angle,
                                           const Vector2D & max_forward_accel,
                                           const Vector2D & max_back_accel,
                                           const double & control_area,
                                           InterceptInfo * info ) const
{
    const ServerParam & SP = ServerParam::i();
    const SelfObject & self = M_world.self();

    const double control_buf = control_area - 0.075;

    const AngleDeg dash_dir = dash_angle - M_world.self().body();
    const Vector2D ball_next = M_world.ball().pos() + M_world.ball().vel();
    const Vector2D self_next = self.pos() + self.vel();

    const Vector2D ball_rel = ( ball_next - self_next ).rotatedVector( -dash_angle );
    const Vector2D ball_relbody = ( ball_next - self_next ).rotatedVector( -self.body() );
    const Vector2D forward_accel_rel = max_forward_accel.rotatedVector( -dash_angle );
    const Vector2D forward_accel_relbody = max_forward_accel.rotatedVector( -self.body() );
    const double dash_rate = self.dashRate() * SP.dashDirRate( dash_dir.degree() );

#ifdef DEBUG_PRINT_ONE_STEP
    dlog.addText( Logger::INTERCEPT,
                  "(predictOneDashAdjust) dir=%.1f angle=%.1f ball_rel=(%.3f %.3f)",
                  dash_dir.degree(),
                  dash_angle.degree(),
                  ball_rel.x, ball_rel.y );
    dlog.addText( Logger::INTERCEPT,
                  "_____ max_forward_accel=(%.3f %.3f) rel=(%.3f %.3f)",
                  max_forward_accel.x, max_forward_accel.y,
                  forward_accel_rel.x, forward_accel_rel.y );
#endif

    if ( ball_relbody.absY() > 0.8
         || ball_relbody.x > forward_accel_relbody.x + control_area
         || ball_relbody.x < 0)
    {
#ifdef DEBUG_PRINT_ONE_STEP
        dlog.addText( Logger::INTERCEPT,
                      "__(predictOneDashAdjust) out of control area=%.3f"
                      " ball_absY=%.3f forward_dist=%.3f",
                      control_buf, ball_rel.absY(),
                      ball_rel.dist( forward_accel_rel ));
#endif
        return false;
    }

    double dash_power = forward_accel_rel.x / dash_rate;

    InterceptInfo::Mode mode = InterceptInfo::NORMAL;

    Vector2D accel = Vector2D::polar2vector( dash_power * dash_rate, dash_angle );
    Vector2D my_vel = self.vel() + accel;
    Vector2D my_pos = self.pos() + my_vel;

    StaminaModel stamina_model = self.staminaModel();
    stamina_model.simulateDash( self.playerType(), dash_power );

    if ( stamina_model.stamina() < SP.recoverDecThrValue()
         && ! stamina_model.capacityIsEmpty() )
    {
        mode = InterceptInfo::EXHAUST;
    }

    *info = InterceptInfo( mode, 0, 1, dash_power, dash_dir,
                           my_pos,
                           my_pos.dist( ball_next ),
                           stamina_model.stamina() );
    (*info).setTackle();
#ifdef DEBUG_PRINT_ONE_STEP
    dlog.addText( Logger::INTERCEPT,
                  "__*** (predictOneDashAdjust) --->Success! power=%.3f rel_dir=%.1f angle=%.1f"
                  " my_pos=(%.2f %.2f) ball_dist=%.3f stamina=%.1f",
                  info->dashPower(),
                  info->dashAngle().degree(),
                  dash_angle.degree(),
                  my_pos.x, my_pos.y,
                  info->ballDist(),
                  stamina_model.stamina() );
#endif
    return true;
}

/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*!

 */
void
SelfInterceptTackle::predictShortStep( const int max_cycle,
                                       const bool save_recovery,
                                       std::vector< InterceptInfo > & self_cache ) const
{
    static std::vector< InterceptInfo > tmp_cache;

    const int max_loop = std::min( MAX_SHORT_STEP, max_cycle );

    const ServerParam & SP = ServerParam::i();
    const BallObject & ball = M_world.ball();
    const SelfObject & self = M_world.self();
    const PlayerType & ptype = self.playerType();

    const double pen_area_x = SP.ourPenaltyAreaLineX() - 0.5;
    const double pen_area_y = SP.penaltyAreaHalfWidth() - 0.5;

    // calc Y distance from ball line
    const Vector2D ball_to_self = ( self.pos() - ball.pos() ).rotatedVector( - ball.vel().th() );
    int min_cycle
            = static_cast< int >( std::ceil( ( ball_to_self.absY() - SP.tackleDist() )
                                             / ptype.realSpeedMax() ) );
    if ( min_cycle >= max_loop ) return;
    if ( min_cycle < 2 ) min_cycle = 2;


    Vector2D ball_pos = ball.inertiaPoint( min_cycle - 1 );
    Vector2D ball_vel = ball.vel() * std::pow( SP.ballDecay(), min_cycle - 1 );

    for ( int cycle = min_cycle; cycle <= max_loop; ++cycle )
    {
        tmp_cache.clear();

        ball_pos += ball_vel;
        ball_vel *= SP.ballDecay();

#ifdef DEBUG_PRINT_SHORT_STEP
        dlog.addText( Logger::INTERCEPT,
                      "--------- cycle %d  -----------", cycle );
        dlog.addText( Logger::INTERCEPT,
                      "(predictShortStep) cycle %d: bpos(%.3f, %.3f) bvel(%.3f, %.3f)",
                      cycle,
                      ball_pos.x, ball_pos.y,
                      ball_vel.x, ball_vel.y );

#endif
        const double control_area = SP.tackleDist();
        if ( std::pow( control_area + ptype.realSpeedMax() * cycle, 2 )
             < self.pos().dist2( ball_pos ) )
        {
#ifdef DEBUG_PRINT_SHORT_STEP
            dlog.addText( Logger::INTERCEPT,
                          "(predictShortStep) too far." );
#endif
            continue;
        }

#ifdef DEBUG_PRINT_SHORT_STEP
        dlog.addText( Logger::INTERCEPT,
                      ">>>>>>>> turn dash forward, turn_margin_dist=%f",
                      control_area - 0.4 );
#endif
        predictTurnDashShort( cycle, ball_pos, control_area, save_recovery, false, // forward dash
                              std::max( 0.1, control_area - 0.4 ),
                              tmp_cache );

#ifdef DEBUG_PRINT_SHORT_STEP
        dlog.addText( Logger::INTERCEPT,
                      ">>>>>>>> turn dash forward, turn_margin_dist=%f",
                      control_area - 0.1 );
#endif
        predictTurnDashShort( cycle, ball_pos, control_area, save_recovery, false, // forward dash
                              std::max( 0.1, control_area - control_area_buf ),
                              tmp_cache );

#ifdef DEBUG_PRINT_SHORT_STEP
        dlog.addText( Logger::INTERCEPT,
                      ">>>>>>>> turn dash back, turn_margin_dist=%f",
                      control_area - 0.4 );
#endif


#ifdef DEBUG_PRINT_SHORT_STEP
        dlog.addText( Logger::INTERCEPT,
                      ">>>>>>>> omni dash forward" );
#endif
        if ( cycle <= 5 )
        {
            predictOmniDashShort( cycle, ball_pos, control_area, save_recovery, false, // forward dash
                                  tmp_cache );
        }

        //
        // register best interception
        //

        if ( tmp_cache.empty() )
        {
            continue;
        }

        //const double danger_ball_dist = control_area - 0.2;
        const double safety_ball_dist = std::max( control_area - 0.2 - ball.pos().dist( ball_pos ) * SP.ballRand(),
                                                  ptype.playerSize() + SP.ballSize() + ptype.kickableMargin() * 0.4 );
#ifdef DEBUG_PRINT_SHORT_STEP
        dlog.addText( Logger::INTERCEPT,
                      "decide best interception. size=%d safety_ball_dist=%.3f",
                      tmp_cache.size(), safety_ball_dist );
#endif

        const InterceptInfo * best = &(tmp_cache.front());
#ifdef DEBUG_PRINT_SHORT_STEP
        dlog.addText( Logger::INTERCEPT,
                      "____ turn=%d dash=%d power=%.1f dir=%.1f ball_dist=%.3f stamina=%.1f",
                      best->turnCycle(), best->dashCycle(),
                      best->dashPower(), best->dashAngle().degree(),
                      best->ballDist(), best->stamina() );
#endif

        const std::vector< InterceptInfo >::iterator end = tmp_cache.end();
        std::vector< InterceptInfo >::iterator it = tmp_cache.begin();
        ++it;
        for ( ; it != end; ++it )
        {
#ifdef DEBUG_PRINT_SHORT_STEP
            dlog.addText( Logger::INTERCEPT,
                          "____ turn=%d dash=%d power=%.1f dir=%.1f ball_dist=%.3f stamina=%.1f",
                          it->turnCycle(), it->dashCycle(),
                          it->dashPower(), it->dashAngle().degree(),
                          it->ballDist(), it->stamina() );
#endif
            if ( best->ballDist() < safety_ball_dist
                 && it->ballDist() < safety_ball_dist )
            {
                if ( best->turnCycle() > it->turnCycle() )
                {
                    best = &(*it);
#ifdef DEBUG_PRINT_SHORT_STEP
                    dlog.addText( Logger::INTERCEPT,
                                  "--> updated(1)" );
#endif
                }
                else if ( best->turnCycle() == it->turnCycle()
                          && best->stamina() < it->stamina() )
                {
                    best = &(*it);
#ifdef DEBUG_PRINT_SHORT_STEP
                    dlog.addText( Logger::INTERCEPT,
                                  "--> updated(2)" );
#endif
                }
            }
            else
            {
                //if ( ( best->ballDist() > danger_ball_dist
                //     || ( best->turnCycle() > 0
                //          && best->turnCycle() >= it->turnCycle() ) )
                if ( best->turnCycle() >= it->turnCycle()
                     && ( best->ballDist() > it->ballDist()
                          || ( std::fabs( best->ballDist() - it->ballDist() ) < 0.001
                               && best->stamina() < it->stamina() ) ) )
                {
                    best = &(*it);
#ifdef DEBUG_PRINT_SHORT_STEP
                    dlog.addText( Logger::INTERCEPT,
                                  "--> updated(3)" );
#endif
                }
            }
        }

#ifdef DEBUG_PRINT_SHORT_STEP
        dlog.addText( Logger::INTERCEPT,
                      "<<<<< Register best cycle=%d(t=%d d=%d) my_pos=(%.2f %.2f) ball_dist=%.3f stamina=%.1f",
                      best->reachCycle(), best->turnCycle(), best->dashCycle(),
                      best->selfPos().x, best->selfPos().y,
                      best->ballDist(),
                      best->stamina() );
#endif

        //self_cache.insert( self_cache.end(), tmp_cache.begin(), tmp_cache.end() );
        self_cache.push_back( *best );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
SelfInterceptTackle::predictTurnDashShort( const int cycle,
                                           const Vector2D & ball_pos,
                                           const double & control_area,
                                           const bool save_recovery,
                                           const bool back_dash,
                                           const double & turn_margin_control_area,
                                           std::vector< InterceptInfo > & self_cache ) const
{
    AngleDeg dash_angle = M_world.self().body();
    int n_turn = predictTurnCycleLong(cycle, ball_pos, control_area, back_dash,
                         &dash_angle);
    if ( n_turn > cycle )
    {
#ifdef DEBUG_PRINT_SHORT_STEP
        dlog.addText( Logger::INTERCEPT,
                      "(predictShortStep_cycle=%d) turn=%d over",
                      cycle, n_turn );
#endif
        return;
    }

    predictDashCycleShort( cycle, n_turn, ball_pos, dash_angle, control_area, save_recovery, back_dash,
                           self_cache );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
SelfInterceptTackle::predictDashCycleShort( const int cycle,
                                            const int n_turn,
                                            const Vector2D & ball_pos,
                                            const AngleDeg & dash_angle,
                                            const double & control_area,
                                            const bool save_recovery,
                                            const bool back_dash,
                                            std::vector< InterceptInfo > & self_cache ) const
{
    const ServerParam & SP = ServerParam::i();
    const SelfObject & self = M_world.self();
    const PlayerType & ptype = self.playerType();

    const double recover_dec_thr = SP.recoverDecThrValue() + 1.0;
    const int max_dash = cycle - n_turn;

    const Vector2D my_inertia = self.inertiaPoint( cycle );

    Vector2D my_pos = self.inertiaPoint( n_turn );
    Vector2D my_vel = self.vel() * std::pow( ptype.playerDecay(), n_turn );

    StaminaModel stamina_model = self.staminaModel();
    stamina_model.simulateWaits( ptype, n_turn );

    if ( my_inertia.dist2( ball_pos ) < std::pow( control_area - control_area_buf, 2 ) )
    {
        Vector2D my_final_pos = my_inertia;

        StaminaModel tmp_stamina = stamina_model;
        tmp_stamina.simulateWaits( ptype, cycle - n_turn );
#ifdef DEBUG_PRINT_SHORT_STEP
        dlog.addText( Logger::INTERCEPT,
                      "%d  **OK** (predictDashCycleShort) can reach. turn=%d dash=0.",
                      cycle, n_turn );
        dlog.addText( Logger::INTERCEPT,
                      "%d _____________________"
                      " bpos(%.1f %.1f) my_inertia=(%.1f %.1f) dist=%.3f stamina=%.1f",
                      cycle,
                      ball_pos.x, ball_pos.y,
                      my_inertia.x, my_inertia.y,
                      my_final_pos.dist( ball_pos ),
                      tmp_stamina.stamina() );
#endif
        InterceptInfo tmp( InterceptInfo::NORMAL,
                         n_turn, cycle - n_turn,
                         0.0, 0.0,
                         my_final_pos,
                         my_final_pos.dist( ball_pos ),
                         tmp_stamina.stamina() );
        tmp.setTackle();
        self_cache.push_back( tmp );
    }

    const AngleDeg target_angle = ( ball_pos - my_inertia ).th();
    if ( ( target_angle - dash_angle ).abs() > 90.0 )
    {
#ifdef DEBUG_PRINT_SHORT_STEP
        dlog.addText( Logger::INTERCEPT,
                      "%d XXX (predictDashCycleShort) turn=%d.",
                      cycle, n_turn );
        dlog.addText( Logger::INTERCEPT,
                      "%d ____________________"
                      " (target_angle(%.1f) - dash_angle(%.1f)) > 90",
                      cycle,
                      target_angle.degree(), dash_angle.degree() );
#endif
        return;
    }


    const Vector2D accel_unit = Vector2D::polar2vector( 1.0, dash_angle );
    double first_dash_power = 0.0;

    for ( int n_dash = 1; n_dash <= max_dash; ++n_dash )
    {
#if 0
        dlog.addText( Logger::INTERCEPT,
                      "__ dash %d: max_dash=%d", n_dash, max_dash );
#endif
        Vector2D ball_rel = ( ball_pos - my_pos ).rotatedVector( -dash_angle );
        double first_speed = calc_first_term_geom_series( ball_rel.x,
                                                          ptype.playerDecay(),
                                                          max_dash - n_dash + 1 );
        Vector2D rel_vel = my_vel.rotatedVector( -dash_angle );
        double required_accel = first_speed - rel_vel.x;
        double dash_power = required_accel / ( ptype.dashRate( stamina_model.effort() ) );

        double available_stamina = ( save_recovery
                                     ? std::max( 0.0, stamina_model.stamina() - recover_dec_thr )
                                     : stamina_model.stamina() + ptype.extraStamina() );
        dash_power = bound( 0.0, dash_power, SP.maxDashPower() );
        dash_power = std::min( available_stamina, dash_power );

        if ( n_dash == 1 )
        {
            first_dash_power = dash_power;
        }

        double accel_mag = std::fabs( dash_power * ptype.dashRate( stamina_model.effort() ) );
        Vector2D accel = accel_unit * accel_mag;

        my_vel += accel;
        my_pos += my_vel;
        my_vel *= ptype.playerDecay();

        stamina_model.simulateDash( ptype, dash_power );

#if 0
        dlog.addText( Logger::INTERCEPT,
                      "____ pos=(%.2f %.2f) vel=(%.2f %.2f)r=%.3f th=%.1f",
                      my_pos.x, my_pos.y,
                      my_vel.x, my_vel.y,
                      my_vel.r(), my_vel.th().degree() );
        dlog.addText( Logger::INTERCEPT,
                      "____ required first_speed=%.3f accel=%.3f dash_power=%.1f",
                      first_speed, required_accel, dash_power );
#endif
    }


    if ( my_pos.dist2( ball_pos ) < std::pow( control_area - control_area_buf, 2 )
         || self.pos().dist2( my_pos ) > self.pos().dist2( ball_pos ) )
    {
        InterceptInfo::Mode mode = ( stamina_model.stamina() < SP.recoverDecThrValue()
                                     && ! stamina_model.capacityIsEmpty()
                                     ? InterceptInfo::EXHAUST
                                     : InterceptInfo::NORMAL );
#ifdef DEBUG_PRINT_SHORT_STEP
        dlog.addText( Logger::INTERCEPT,
                      "%d **OK** (predictDashCycleShort) controllable turn=%d dash=%d",
                      cycle, n_turn, cycle - n_turn );
        dlog.addText( Logger::INTERCEPT,
                      "%d __"
                      " bpos(%.1f %.1f) my_pos=(%.1f %.1f) ball_dist=%.3f"
                      " first_dash_power=%.1f stamina=%.1f",
                      cycle,
                      ball_pos.x, ball_pos.y,
                      my_pos.x, my_pos.y,
                      my_pos.dist( ball_pos ),
                      first_dash_power, stamina_model.stamina() );
        dlog.addText( Logger::INTERCEPT,
                      "%d __"
                      " first_dash_power=%.1f stamina=%.1f",
                      cycle,
                      first_dash_power, stamina_model.stamina() );
#endif
        InterceptInfo tmp( mode, n_turn, cycle - n_turn,
                         first_dash_power, 0.0,
                         my_pos,
                         my_pos.dist( ball_pos ),
                         stamina_model.stamina() );
        tmp.setTackle();
        self_cache.push_back( tmp );
        return;
    }

#ifdef DEBUG_PRINT_SHORT_STEP
    dlog.addText( Logger::INTERCEPT,
                  "%d XXX (predictDashCycleShort) turn=%d dash=%d.",
                  cycle, n_turn, max_dash );
    dlog.addText( Logger::INTERCEPT,
                  "%d __ bpos(%.2f %.2f) mypos=(%.2f %.2f)",
                  cycle,
                  ball_pos.x, ball_pos.y,
                  my_pos.x, my_pos.y );
    dlog.addText( Logger::INTERCEPT,
                  "%d __ ball_dist=%.3f control_area=%.3f(real:%.3f buf=%.3f",
                  cycle,
                  my_pos.dist( ball_pos ),
                  control_area - control_area_buf,
                  control_area,
                  control_area_buf );
    dlog.addText( Logger::INTERCEPT,
                  "%d __ my_dash_move=%.3f first_ball_dist=%.3f",
                  cycle,
                  self.pos().dist( my_pos ),
                  self.pos().dist( ball_pos ) );
#endif
}

/*-------------------------------------------------------------------*/
/*!

 */
void
SelfInterceptTackle::predictOmniDashShort( const int cycle,
                                           const Vector2D & ball_pos,
                                           const double & control_area,
                                           const bool save_recovery,
                                           const bool back_dash,
                                           std::vector< InterceptInfo > & self_cache ) const
{
    const ServerParam & SP = ServerParam::i();
    const SelfObject & self = M_world.self();
    const PlayerType & ptype = self.playerType();

    const AngleDeg body_angle = self.body();
    const Vector2D my_inertia = self.inertiaPoint( cycle );
    const Line2D target_line( ball_pos, body_angle );

    if ( target_line.dist( my_inertia ) < 0.6 )
    {
#ifdef DEBUG_PRINT_SHORT_STEP
        dlog.addText( Logger::INTERCEPT,
                      "%d (predictOmniDashShort) already on line. no need to omnidash. target_line_dist=%.3f",
                      cycle, target_line.dist( my_inertia ) );
#endif
        return;
    }

    const double recover_dec_thr = SP.recoverDecThrValue() + 1.0;

    const double dash_angle_step = std::max( 15.0, SP.dashAngleStep() );
    const double min_dash_angle = ( -180.0 < SP.minDashAngle() && SP.maxDashAngle() < 180.0
                                    ? SP.minDashAngle()
                                    : dash_angle_step * static_cast< int >( -180.0 / dash_angle_step ) );
    const double max_dash_angle = ( -180.0 < SP.minDashAngle() && SP.maxDashAngle() < 180.0
                                    ? SP.maxDashAngle() + dash_angle_step * 0.5
                                    : dash_angle_step * static_cast< int >( 180.0 / dash_angle_step ) - 1.0 );
#ifdef DEBUG_PRINT_SHORT_STEP
    dlog.addText( Logger::INTERCEPT,
                  "%d (predictOmniDashShort) min_angle=%.1f max_angle=%.1f",
                  cycle, min_dash_angle, max_dash_angle );
#endif

    const AngleDeg target_angle = ( ball_pos - my_inertia ).th();

    for ( double dir = min_dash_angle;
          dir < max_dash_angle;
          dir += dash_angle_step )
    {
        if ( std::fabs( dir ) < 1.0 ) continue;

#ifdef DEBUG_PRINT_SHORT_STEP
        dlog.addText( Logger::INTERCEPT,
                      "%d ===== (predictOmniDashShort) dir=%.1f",
                      cycle, dir );
#endif

        const AngleDeg dash_angle = body_angle + SP.discretizeDashAngle( SP.normalizeDashAngle( dir ) );

        if ( ( dash_angle - target_angle ).abs() > 91.0 )
        {
#ifdef DEBUG_PRINT_SHORT_STEP
            dlog.addText( Logger::INTERCEPT,
                          "%d XXX angle over. target_angle=%.1f dash_angle=%.1f",
                          cycle,
                          target_angle.degree(), dash_angle.degree() );
#endif
            continue;
        }

        double first_dash_power = 0.0;

        Vector2D my_pos = self.pos();
        Vector2D my_vel = self.vel();

        StaminaModel stamina_model = self.staminaModel();

        int n_omni_dash = predictAdjustOmniDash( cycle,
                                                 ball_pos,
                                                 control_area,
                                                 save_recovery,
                                                 back_dash,
                                                 dir,
                                                 &my_pos,
                                                 &my_vel,
                                                 &stamina_model,
                                                 &first_dash_power );

        if ( n_omni_dash < 0 )
        {
#ifdef DEBUG_PRINT_SHORT_STEP
            dlog.addText( Logger::INTERCEPT,
                          "%d XXX no adjustable",
                          cycle );
#endif
            continue;
        }

        if ( n_omni_dash == 0 )
        {
#ifdef DEBUG_PRINT_SHORT_STEP
            dlog.addText( Logger::INTERCEPT,
                          "%d XXX not need to adjust",
                          cycle );
#endif
            continue;
        }

        //
        // check target point direction
        //
        {
            Vector2D inertia_pos = ptype.inertiaPoint( my_pos, my_vel, cycle - n_omni_dash );
            Vector2D target_rel = ( ball_pos - inertia_pos ).rotatedVector( -body_angle );

#ifdef DEBUG_PRINT_SHORT_STEP
            dlog.addText( Logger::INTERCEPT,
                          "%d ____after omni dash. inertia_pos=(%.3f %.3f) ball_pos=(%.3f %.3f) body_angle=%.1f target_rel=(%.3f %.3f)",
                          cycle,
                          inertia_pos.x, inertia_pos.y,
                          ball_pos.x, ball_pos.y,
                          body_angle.degree(),
                          target_rel.x, target_rel.y );
#endif

            if ( ( back_dash && target_rel.x > 0.0 )
                 || ( ! back_dash && target_rel.x < 0.0 ) )
            {
#ifdef DEBUG_PRINT_SHORT_STEP
                dlog.addText( Logger::INTERCEPT,
                              "%d XXX invalid dash direction. dash=%d target_rel.x=%.3f",
                              cycle, n_omni_dash,
                              target_rel.x );
#endif
                continue;
            }
        }

        //
        // dash to the body direction
        //

        const Vector2D body_accel_unit = Vector2D::polar2vector( 1.0, body_angle );

        for ( int n_dash = n_omni_dash + 1 ; n_dash <= cycle; ++n_dash )
        {
            double first_speed = calc_first_term_geom_series( ( ball_pos - my_pos ).rotatedVector( -body_angle ).x,
                                                              ptype.playerDecay(),
                                                              cycle - n_dash + 1 );
            Vector2D rel_vel = my_vel.rotatedVector( -body_angle );
            double required_accel = first_speed - rel_vel.x;
            double dash_power = required_accel / ( ptype.dashRate( stamina_model.effort() ) );
            if ( back_dash ) dash_power = -dash_power;

            double available_stamina = ( save_recovery
                                         ? std::max( 0.0, stamina_model.stamina() - recover_dec_thr )
                                         : stamina_model.stamina() + ptype.extraStamina() );

            dash_power = bound( 0.0, dash_power, SP.maxDashPower() );
            dash_power = std::min( available_stamina, dash_power );

            double accel_mag = std::fabs( dash_power ) * ptype.dashRate( stamina_model.effort() );
            Vector2D accel = body_accel_unit * accel_mag;

            my_vel += accel;
            my_pos += my_vel;
#ifdef DEBUG_PRINT_SHORT_STEP
            dlog.addText( Logger::INTERCEPT,
                          "%d __ body_dash=%d pos=(%.2f %.2f) vel=(%.2f %.2f)r=%.3f",
                          cycle, n_dash - n_omni_dash,
                          my_pos.x, my_pos.y,
                          my_vel.x, my_vel.y, my_vel.r() );
#endif
            my_vel *= ptype.playerDecay();

            stamina_model.simulateDash( ptype, dash_power );
        }

        const Vector2D my_move = my_pos - self.pos();

        if ( my_pos.dist2( ball_pos ) < std::pow( control_area - control_area_buf, 2 )
             || my_move.r() > ( ball_pos - self.pos() ).rotatedVector( - my_move.th() ).absX() )
        {
            InterceptInfo::Mode mode = ( stamina_model.recovery() < M_world.self().recovery()
                                         && ! stamina_model.capacityIsEmpty()
                                         ? InterceptInfo::EXHAUST
                                         : InterceptInfo::NORMAL );
#ifdef DEBUG_PRINT_SHORT_STEP
            dlog.addText( Logger::INTERCEPT,
                          "%d **OK** can reach, after body dir dash.", cycle );
            dlog.addText( Logger::INTERCEPT,
                          "%d ____ omni_dash=%d body_dash=%d",
                          cycle,
                          n_omni_dash, cycle - n_omni_dash );
            dlog.addText( Logger::INTERCEPT,
                          "%d ____ final_pos=(%.1f %.1f) ball_dist=%.3f ctrl_area=%.3f",
                          cycle,
                          my_pos.x, my_pos.y,
                          my_pos.dist( ball_pos ), control_area );
            dlog.addText( Logger::INTERCEPT,
                          "%d ____ my_move_dist=%.3f ball_rel_x=%.3f",
                          cycle,
                          my_move.r(),
                          ( ball_pos - self.pos() ).rotatedVector( - my_move.th() ).x );
            dlog.addText( Logger::INTERCEPT,
                          "%d ____ 1st_dash_power=%.1f stamina=%.1f",
                          cycle,
                          first_dash_power, stamina_model.stamina() );
#endif
            InterceptInfo tmp( mode,
                             0, cycle,
                             first_dash_power, dir,
                             my_pos,
                             my_pos.dist( ball_pos ),
                             stamina_model.stamina() );
            tmp.setTackle();
            self_cache.push_back( tmp );
        }
    }

}

/*-------------------------------------------------------------------*/
/*!

 */
int
SelfInterceptTackle::predictAdjustOmniDash( const int cycle,
                                            const Vector2D & ball_pos,
                                            const double & control_area,
                                            const bool save_recovery,
                                            const bool back_dash,
                                            const double & dash_rel_dir,
                                            Vector2D * my_pos,
                                            Vector2D * my_vel,
                                            StaminaModel * stamina_model,
                                            double * first_dash_power ) const
{
    const ServerParam & SP = ServerParam::i();
    const SelfObject & self = M_world.self();
    const PlayerType & ptype = self.playerType();

    const double recover_dec_thr = SP.recoverDecThrValue() + 1.0;
    const int max_omni_dash = std::min( 2, cycle );

    const AngleDeg body_angle = self.body();
    const Line2D target_line( ball_pos, body_angle );
    const Vector2D my_inertia = self.inertiaPoint( cycle );

    if ( target_line.dist( my_inertia ) < 0.6 )
    {
#ifdef DEBUG_PRINT_SHORT_STEP
        dlog.addText( Logger::INTERCEPT,
                      "%d (predictAdjustOmniDash) no dash required. line_dist=%.3f < control_buf=%.3f(%.3f)",
                      cycle, target_line.dist( my_inertia ), control_area - 0.4, control_area );
#endif
        *first_dash_power = 0.0;
        return 0;
    }

    const AngleDeg dash_angle = body_angle
            + SP.discretizeDashAngle( SP.normalizeDashAngle( dash_rel_dir ) );

    const Vector2D accel_unit = Vector2D::polar2vector( 1.0, dash_angle );
    const double dash_dir_rate = SP.dashDirRate( dash_rel_dir );

    //
    // dash simulation
    //
#ifdef DEBUG_PRINT_SHORT_STEP
    dlog.addText( Logger::INTERCEPT,
                  "%d (predictAdjustOmniDash) dir=%.1f angle=%.1f",
                  cycle, dash_rel_dir, dash_angle.degree() );
#endif

    for ( int n_omni_dash = 1; n_omni_dash <= max_omni_dash; ++n_omni_dash )
    {
        double first_speed = calc_first_term_geom_series( std::max( 0.0, target_line.dist( *my_pos ) ),
                                                          ptype.playerDecay(),
                                                          cycle - n_omni_dash + 1 );
        Vector2D rel_vel = my_vel->rotatedVector( -dash_angle );
        double required_accel = first_speed - rel_vel.x;

        if ( std::fabs( required_accel ) < 0.01 )
        {
#ifdef DEBUG_PRINT_SHORT_STEP
            dlog.addText( Logger::INTERCEPT,
                          "%d *** adjustable without dash. omni_dash_loop=%d",
                          cycle, n_omni_dash );
            dlog.addText( Logger::INTERCEPT,
                          "%d __ first_speed=%.3f rel_vel=(%.3f %.3f) required_accel=%.3f",
                          cycle,
                          rel_vel.x, rel_vel.y,
                          first_speed, required_accel );
#endif
            return n_omni_dash - 1;
        }

        double dash_power = required_accel
                / ( ptype.dashRate( stamina_model->effort() ) * dash_dir_rate );
        double available_stamina = ( save_recovery
                                     ? std::max( 0.0, stamina_model->stamina() - recover_dec_thr )
                                     : stamina_model->stamina() + ptype.extraStamina() );

        dash_power = bound( 0.0, dash_power, SP.maxDashPower() );
        dash_power = std::min( available_stamina, dash_power );

        if ( n_omni_dash == 1 )
        {
            *first_dash_power = dash_power;
        }

        double accel_mag
                = std::fabs( dash_power )
                * ptype.dashRate( stamina_model->effort() )
                * dash_dir_rate;
        Vector2D accel = accel_unit * accel_mag;

        *my_vel += accel;
        *my_pos += *my_vel;
        *my_vel *= ptype.playerDecay();

        stamina_model->simulateDash( ptype, dash_power );

        Vector2D inertia_pos = ptype.inertiaPoint( *my_pos, *my_vel, cycle - n_omni_dash );

#ifdef DEBUG_PRINT_SHORT_STEP
        dlog.addText( Logger::INTERCEPT,
                      "%d ____ omni_dash=%d"
                      " accel=(%.3f %.3f)r=%.3f inertia_line_dist=%.3f",
                      cycle, n_omni_dash,
                      accel.x, accel.y, accel.r(),
                      target_line.dist( inertia_pos ) );
#endif

        if ( target_line.dist( inertia_pos ) < control_area - control_area_buf
             && Line2D(inertia_pos, body_angle).dist(ball_pos) < 0.8)
        {
#ifdef DEBUG_PRINT_SHORT_STEP
            dlog.addText( Logger::INTERCEPT,
                          "%d *** adjustable. omni_dash=%d first_dash_power=%.1f line_dist=%.3f ctrl_dist=%.3f",
                          cycle, n_omni_dash,
                          *first_dash_power,
                          target_line.dist( inertia_pos ),
                          control_area );
#endif
            return n_omni_dash;
        }
    }

    return -1;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
SelfInterceptTackle::predictLongStep( const int max_cycle,
                                      const bool save_recovery,
                                      std::vector< InterceptInfo > & self_cache ) const
{
    static std::vector< InterceptInfo > tmp_cache;

    const ServerParam & SP = ServerParam::i();
    const BallObject & ball = M_world.ball();
    const SelfObject & self = M_world.self();
    const PlayerType & ptype = self.playerType();

    // calc Y distance from ball line
    Vector2D ball_to_self = self.pos() - ball.pos();
    ball_to_self.rotate( - ball.vel().th() );
    int start_cycle = static_cast< int >( std::ceil( ( ball_to_self.absY()
                                                       - ptype.kickableArea()
                                                       - 0.2 )
                                                     / ptype.realSpeedMax() ) );

    if ( start_cycle <= MAX_SHORT_STEP )
    {
        start_cycle = MAX_SHORT_STEP + 1;
    }

#ifdef DEBUG_PRINT_LONG_STEP_LEVEL_1
    dlog.addText( Logger::INTERCEPT,
                  "(predictLongStep) start_cycle=%d max_cycle=%d",
                  start_cycle, max_cycle );
#endif

#ifdef DEBUG_PRINT_LONG_STEP_LEVEL_1
    if ( max_cycle <= start_cycle )
    {
        dlog.addText( Logger::INTERCEPT,
                      "(predictLongStep) Too big Y difference = %f."
                      "  start_cycle = %d.  max_cycle = %d",
                      ball_to_self.y, start_cycle, max_cycle );
    }
#endif

    Vector2D ball_pos = ball.inertiaPoint( start_cycle - 1 );
    Vector2D ball_vel = ball.vel() * std::pow( SP.ballDecay(), start_cycle - 1 );
    bool found = false;

    int max_loop = max_cycle;

    tmp_cache.clear();
    for ( int cycle = start_cycle; cycle < max_loop; ++cycle )
    {
        ball_pos += ball_vel;
        ball_vel *= SP.ballDecay();

#ifdef DEBUG_PRINT_LONG_STEP_LEVEL_1
        dlog.addText( Logger::INTERCEPT,
                      "---------- cycle %d ----------", cycle );
        dlog.addText( Logger::INTERCEPT,
                      "bpos(%.3f, %.3f) bvel(%.3f, %.3f)",
                      ball_pos.x, ball_pos.y,
                      ball_vel.x, ball_vel.y );

#endif
        //         // ball is stopped
        //         if ( ball_vel.r2() < 0.0001 )
        //         {
        // #ifdef DEBUG_PRINT
        //             dlog.addText( Logger::INTERCEPT,
        //                           "____ball speed reach ZERO" );
        // #endif
        //             break;
        //         }

        if ( ball_pos.absX() > SP.pitchHalfLength() + 10.0
             || ball_pos.absY() > SP.pitchHalfWidth() + 10.0
             )
        {
#ifdef DEBUG_PRINT_LONG_STEP_LEVEL_2
            dlog.addText( Logger::INTERCEPT,
                          "%d ____ball is out of pitch", cycle );
#endif
            break;
        }

        const double control_area = SP.tackleDist();

        ///////////////////////////////////////////////////////////
        // reach point is too far. never reach
        if ( control_area + ( ptype.realSpeedMax() * cycle )
             < self.pos().dist( ball_pos ) )
        {
#ifdef DEBUG_PRINT_LONG_STEP_LEVEL_2
            dlog.addText( Logger::INTERCEPT,
                          "%d ____ball is too far. never reach", cycle );
#endif
            continue;
        }

        ///////////////////////////////////////////////////////////
        bool back_dash = false;
        int n_turn = 0;
        double result_recovery = 0.0;
        if ( canReachAfterTurnDash( cycle,
                                    ball_pos,
                                    control_area,
                                    save_recovery, // save recovery
                                    &n_turn, &back_dash, &result_recovery,
                                    self_cache ) )
        {
#ifdef DEBUG_PRINT_LONG_STEP_LEVEL_2
            dlog.addText( Logger::INTERCEPT,
                          "%d --> can reach. cycle=%d, turn=%d, %s recovery=%f",
                          cycle, cycle, n_turn,
                          ( back_dash ? "back" : "forward" ),
                          result_recovery );
#endif
            if ( ! found )
            {
                max_loop = std::min( max_cycle, cycle + 10 );
            }
            found = true;
        }
        ///////////////////////////////////////////////////////////
#if 0

#ifdef DEBUG_PRINT_LONG_STEP
        dlog.addText( Logger::INTERCEPT,
                      ">>>>>>>> turn dash long forward <<<<<<<<<<" );
#endif
        predictTurnDashLong( cycle, ball_pos, control_area, save_recovery, false, // forward dash
                             tmp_cache );
#ifdef DEBUG_PRINT_LONG_STEP
        dlog.addText( Logger::INTERCEPT,
                      ">>>>>>>> turn dash long back <<<<<<<<<<" );
#endif
        predictTurnDashLong( cycle, ball_pos, control_area, save_recovery, true, // back dash
                             tmp_cache );
#endif
    }

    // not registered any interception
    if ( ! found
         && save_recovery )
    {
#ifdef DEBUG_PRINT_LONG_STEP_LEVEL_1
        dlog.addText( Logger::INTERCEPT,
                      __FILE__": SelfInterceptTackle. failed to predict? register ball final point" );
#endif
        predictFinal( max_cycle, self_cache );
    }

    if ( self_cache.empty() )
    {
#ifdef DEBUG_PRINT_LONG_STEP_LEVEL_1
        dlog.addText( Logger::INTERCEPT,
                      __FILE__": SelfInterceptTackle. not found. retry predictFinal()" );
#endif
        predictFinal( max_cycle, self_cache );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
SelfInterceptTackle::predictFinal( const int max_cycle,
                                   std::vector< InterceptInfo > & self_cache ) const

{
    const SelfObject & self = M_world.self();
    const PlayerType & ptype = self.playerType();

    const Vector2D my_final_pos = self.inertiaPoint( 100 );
    const Vector2D ball_final_pos = M_world.ball().inertiaPoint( 100 );
    const bool goalie_mode =
            ( self.goalie()
              && M_world.lastKickerSide() != M_world.ourSide()
            && ball_final_pos.x < ServerParam::i().ourPenaltyAreaLineX()
            && ball_final_pos.absY() < ServerParam::i().penaltyAreaHalfWidth()
            );
    const double control_area = ( goalie_mode
                                  ? ServerParam::i().catchableArea() - 0.15
                                  : ptype.kickableArea() );

    AngleDeg dash_angle = self.body();
    bool back_dash = false; // dummy
    int n_turn = predictTurnCycle( 100,
                                   ball_final_pos,
                                   control_area,
                                   &dash_angle, &back_dash );
    double dash_dist = my_final_pos.dist( ball_final_pos );
    dash_dist -= control_area;
    int n_dash = ptype.cyclesToReachDistance( dash_dist );

#ifdef DEBUG_PRINT_LONG_STEP_LEVEL_1
    dlog.addText( Logger::INTERCEPT,
                  "(predictFinal) register ball final point. max_cycle=%d, turn=%d, dash=%d",
                  max_cycle,
                  n_turn, n_dash );
#endif
    if ( max_cycle > n_turn + n_dash )
    {
        n_dash = max_cycle - n_turn;
#ifdef DEBUG_PRINT_LONG_STEP_LEVEL_1
        dlog.addText( Logger::INTERCEPT,
                      "__Final(2) dash step is changed by max_cycle. max=%d turn=%d dash=%d",
                      max_cycle,
                      n_turn, n_dash );
#endif
    }

    StaminaModel stamina_model = self.staminaModel();

    stamina_model.simulateWaits( ptype, n_turn );
    stamina_model.simulateDashes( ptype, n_dash, ServerParam::i().maxDashPower() );

    self_cache.push_back( InterceptInfo( InterceptInfo::NORMAL,
                                         n_turn, n_dash,
                                         ServerParam::i().maxDashPower(), 0.0,
                                         ball_final_pos,
                                         0.0,
                                         stamina_model.stamina() ) );
}

/*-------------------------------------------------------------------*/
/*!
  \param ball_pos ball position after 'cycle'.
*/
bool
SelfInterceptTackle::canReachAfterTurnDash( const int cycle,
                                            const Vector2D & ball_pos,
                                            const double & control_area,
                                            const bool save_recovery,
                                            int * n_turn,
                                            bool * back_dash,
                                            double * result_recovery,
                                            std::vector< InterceptInfo > & self_cache ) const
{
    AngleDeg dash_angle = M_world.self().body();

    *n_turn = predictTurnCycle( cycle,
                                ball_pos,
                                control_area,
                                &dash_angle, back_dash );
    if ( *n_turn > cycle )
    {
        return false;
    }

    return canReachAfterDash( *n_turn, std::max( 0, cycle - (*n_turn) ),
                              ball_pos, control_area,
                              save_recovery,
                              dash_angle, *back_dash,
                              result_recovery,
                              self_cache );
}

/*-------------------------------------------------------------------*/
/*!
  predict & get teammate's ball gettable cycle
*/
int
SelfInterceptTackle::predictTurnCycle( const int cycle,
                                       const Vector2D & ball_pos,
                                       const double & control_area,
                                       AngleDeg * dash_angle,
                                       bool * back_dash ) const
{
    const PlayerType & ptype = M_world.self().playerType();

    int n_turn = 0;

    ///////////////////////////////////////////////////
    // prepare variables
    const Vector2D inertia_pos = M_world.self().inertiaPoint( cycle );

    const Vector2D target_rel = ball_pos - inertia_pos;
    const AngleDeg target_angle = target_rel.th();

    double angle_diff = ( target_angle - (*dash_angle) ).degree();
    const bool diff_is_positive = ( angle_diff > 0.0 ? true : false );
    angle_diff = std::fabs( angle_diff );

    ///////////////////////////////////////////////////
    // predict turn cycles
    const double max_moment
            = ServerParam::i().maxMoment()
            * ( 1.0 - ServerParam::i().playerRand() );
    double player_speed = M_world.self().vel().r();
    double body_angle = (*dash_angle).degree();
    double player_line_dist_to_ball = Line2D(inertia_pos, body_angle).dist(ball_pos);
    while ( angle_diff > 90 ||  player_line_dist_to_ball > 1)
    {
        double max_turnable
                = ptype.effectiveTurn( max_moment, player_speed );
        angle_diff -= max_turnable;
        player_speed *= ptype.playerDecay();

        if(angle_diff < 0)
            body_angle = target_angle.degree();
        else if(diff_is_positive)
            body_angle += max_turnable;
        else
            body_angle -= max_turnable;
        player_line_dist_to_ball = Line2D(inertia_pos, body_angle).dist(ball_pos);
        ++n_turn;
    }

    ///////////////////////////////////////////////////
    // update dash angle
    if ( n_turn > 0 )
    {
        angle_diff = std::max( 0.0, angle_diff );
        *dash_angle = target_angle + ( diff_is_positive
                                       ? + angle_diff
                                       : - angle_diff );
    }

    return n_turn;
}

/*-------------------------------------------------------------------*/
/*!
  predict & get teammate's ball gettable cycle
*/
bool
SelfInterceptTackle::canBackDashChase( const int cycle,
                                       const double & /*target_dist*/,
                                       const double & angle_diff ) const
{
    ///////////////////////////////////////////////
    // check angle threshold
    if ( angle_diff < BACK_DASH_THR_ANGLE )
    {
        return false;
    }

    if ( ( ! M_world.self().goalie()
           || M_world.lastKickerSide() == M_world.ourSide() )
         && cycle >= 5 )
    {
        return false;
    }

    if ( M_world.self().goalie()
         && M_world.lastKickerSide() != M_world.ourSide()
         && cycle >= 5 )
    {
        if ( cycle >= 15 )
        {
            return false;
        }

        Vector2D goal( - ServerParam::i().pitchHalfLength(), 0.0 );
        Vector2D bpos = M_world.ball().inertiaPoint( cycle );
        if ( goal.dist( bpos ) > 21.0 )
        {
            return false;
        }
    }

    ///////////////////////////////////////////////
    // check stamina threshold
    // consumed stamina by one step
    double total_consume = - ServerParam::i().minDashPower() * 2.0 * cycle;
    double total_recover
            = M_world.self().playerType().staminaIncMax() * M_world.self().recovery()
            * ( cycle - 1 );
    double result_stamina
            = M_world.self().stamina()
            - total_consume
            + total_recover;

    if ( result_stamina < ServerParam::i().recoverDecThrValue() + 205.0 )
    {
#ifdef DEBUG_PRINT_LONG_STEP_LEVEL_2
        dlog.addText( Logger::INTERCEPT,
                      "%d ______ goalie no stamina. no back. stamina=%f",
                      cycle, result_stamina );
#endif
        return false;
    }

#ifdef DEBUG_PRINT_LONG_STEP_LEVEL_2
    dlog.addText( Logger::INTERCEPT,
                  "%d ______try back dash. result stamina=%.1f",
                  cycle, result_stamina );
#endif

    return true;
}

/*-------------------------------------------------------------------*/
/*!
  assume that players's dash angle is directed to ball reach point
  \param ball_pos ball position after 'cycle'.
*/
bool
SelfInterceptTackle::canReachAfterDash( const int n_turn,
                                        const int n_dash,
                                        const Vector2D & ball_pos,
                                        const double & control_area,
                                        const bool save_recovery,
                                        const AngleDeg & dash_angle,
                                        const bool back_dash,
                                        double * result_recovery,
                                        std::vector< InterceptInfo > & self_cache ) const
{
    static const double PLAYER_NOISE_RATE
            //= ( 1.0 - ServerParam::i().playerRand() * 0.25 );
            = ( 1.0 - ServerParam::i().playerRand() * 0.01 );
    static const double MAX_POWER = ServerParam::i().maxDashPower();

    const ServerParam & SP = ServerParam::i();
    const PlayerType & ptype = M_world.self().playerType();

    const Vector2D my_inertia = M_world.self().inertiaPoint( n_turn + n_dash );

    const double recover_dec_thr = SP.recoverDecThr() * SP.staminaMax();
    //////////////////////////////////////////////////////////////
    const AngleDeg dash_angle_minus = -dash_angle;
    const Vector2D ball_rel
            = ( ball_pos - M_world.self().pos() ).rotatedVector( dash_angle_minus );
    const double ball_noise
            = M_world.ball().pos().dist( ball_pos )
            * SP.ballRand()
            * 0.5;
    const double noised_ball_x = ball_rel.x + ball_noise;


    //////////////////////////////////////////////////////////////
    // prepare loop variables
    // ORIGIN: first player pos.
    // X-axis: dash angle
    //Vector2D tmp_pos = ptype.inertiaPoint( M_world.self().pos(), M_world.self().vel(), n_turn );
    //tmp_pos -= M_world.self().pos();
    Vector2D tmp_pos = ptype.inertiaTravel( M_world.self().vel(), n_turn );
    tmp_pos.rotate( dash_angle_minus );

    Vector2D tmp_vel = M_world.self().vel();
    tmp_vel *= std::pow( ptype.playerDecay(), n_turn );
    tmp_vel.rotate( dash_angle_minus );

    StaminaModel stamina_model = M_world.self().staminaModel();
    stamina_model.simulateWaits( ptype, n_turn );

    double prev_effort = stamina_model.effort();
    double dash_power_abs = MAX_POWER;
    // only consider about x of dash accel vector,
    // because current orientation is player's dash angle (included back dash case)
    // NOTE: dash_accel_x must be positive value.
    double dash_accel_x = dash_power_abs * ptype.dashRate( stamina_model.effort() );

#ifdef DEBUG_PRINT_LONG_STEP_LEVEL_2
    dlog.addText( Logger::INTERCEPT,
                  "%d ______Try %d turn: %d dash:"
                  " angle=%.1f first_accel=%.2f first_vel=(%.2f %.2f)",
                  n_turn + n_dash,
                  n_turn, n_dash,
                  dash_angle.degree(), dash_accel_x, tmp_vel.x, tmp_vel.y );
#endif

    //////////////////////////////////////////////////////////////
    bool can_over_speed_max = ptype.canOverSpeedMax( dash_power_abs,
                                                     stamina_model.effort() );
    double first_dash_power = dash_power_abs * ( back_dash ? -1.0 : 1.0 );

    for ( int i = 0; i < n_dash; ++i )
    {
        /////////////////////////////////////////////////////////
        // update dash power & accel
        double available_power = ( save_recovery
                                   ? std::max( 0.0, stamina_model.stamina() - recover_dec_thr )
                                   : stamina_model.stamina() + ptype.extraStamina() );
        available_power = min_max( 0.0, available_power, MAX_POWER );

        bool must_update_power = false;
        if ( available_power < dash_power_abs  // no enough stamina
             || stamina_model.effort() < prev_effort       // effort decreased.
             || ( ! can_over_speed_max         // can inclease dash power
                  && dash_power_abs < available_power )
             )
        {

            must_update_power = true;
#ifdef DEBUG_PRINT_LONG_STEP_LEVEL_3
            dlog.addText( Logger::INTERCEPT,
                          "%d ________dash %d/%d: no enough? stamina=%.1f extra=%.1f"
                          " cur_pow=%.1f  available_pow=%.1f",
                          n_turn + n_dash,
                          i, n_dash,
                          stamina_model.stamina(), ptype.extraStamina(),
                          dash_power_abs, available_power );
            dlog.addText( Logger::INTERCEPT,
                          "%d ________dash %d/%d: effort decayed? %f -> %f",
                          n_turn + n_dash,
                          i, n_dash, prev_effort, stamina_model.effort() );
            dlog.addText( Logger::INTERCEPT,
                          "%d ________dash %d/%d: reset max power?. curr_pow=%.1f"
                          "  available=%.1f",
                          n_turn + n_dash,
                          i, n_dash, dash_power_abs, available_power );
#endif
        }

        if ( must_update_power )
        {
            dash_power_abs = available_power;
            dash_accel_x = dash_power_abs * ptype.dashRate( stamina_model.effort() );
            can_over_speed_max = ptype.canOverSpeedMax( dash_power_abs,
                                                        stamina_model.effort() );
            if ( i == 0 )
            {
                first_dash_power = dash_power_abs * ( back_dash ? -1.0 : 1.0 );
            }
#ifdef DEBUG_PRINT_LONG_STEP_LEVEL_3
            dlog.addText( Logger::INTERCEPT,
                          "%d ________dash %d/%d: update dash_power_abs=%.1f accel_x=%f",
                          n_turn + n_dash,
                          i, n_dash, dash_power_abs, dash_accel_x );
#endif
        }

        /////////////////////////////////////////////////////////
        // update vel
        tmp_vel.x += dash_accel_x;
        // power conservation, update accel magnitude & dash_power
        if ( can_over_speed_max
             && tmp_vel.r2() > ptype.playerSpeedMax2() )
        {
            tmp_vel.x -= dash_accel_x;
            // conserve power & reduce accel
            // sqr(rel_vel.y) + sqr(max_dash_x) == sqr(max_speed);
            // accel_mag = dash_x - rel_vel.x;
            double max_dash_x = std::sqrt( ptype.playerSpeedMax2()
                                           - ( tmp_vel.y * tmp_vel.y ) );
            dash_accel_x = max_dash_x - tmp_vel.x;
            dash_power_abs = std::fabs( dash_accel_x / ptype.dashRate( stamina_model.effort() ) );
            // re-update vel
            tmp_vel.x += dash_accel_x;
            can_over_speed_max = ptype.canOverSpeedMax( dash_power_abs,
                                                        stamina_model.effort() );
#ifdef DEBUG_PRINT_LONG_STEP_LEVEL_3
            dlog.addText( Logger::INTERCEPT,
                          "%d ________dash %d/%d: power conserve. power=%.1f accel_x=%f",
                          n_turn + n_dash,
                          i, n_dash, dash_power_abs, dash_accel_x );
#endif
        }

        /////////////////////////////////////////////////////////
        // update pos & vel
        tmp_pos += tmp_vel;
        tmp_vel *= ptype.playerDecay();
        /////////////////////////////////////////////////////////
        // update stamina

        stamina_model.simulateDash( ptype, dash_power_abs * ( back_dash ? -1.0 : 1.0 ) );

        /////////////////////////////////////////////////////////
        // check run over
        // it is not necessary to consider about Y difference,
        // because dash_angle is corrected for ball_reach_point
        //if ( tmp_pos.x * PLAYER_NOISE_RATE > noised_ball_x )
        if ( tmp_pos.x * PLAYER_NOISE_RATE + 0.1 > noised_ball_x )
        {
#ifdef DEBUG_PRINT_LONG_STEP_LEVEL_1
            dlog.addText( Logger::INTERCEPT,
                          "%d ____dash %d/%d: can run over. rel_move_pos=(%.2f, %.2f)"
                          " ball_x=%.3f over=%.3f y_diff=%.3f",
                          n_turn + n_dash,
                          i, n_dash,
                          tmp_pos.x, tmp_pos.y,
                          noised_ball_x,
                          //tmp_pos.x * PLAYER_NOISE_RATE - noised_ball_x,
                          tmp_pos.x * PLAYER_NOISE_RATE - 0.1 - noised_ball_x,
                          std::fabs( tmp_pos.y - ball_rel.y ) );
#endif
            *result_recovery = stamina_model.recovery();

            Vector2D inertia_pos = ptype.inertiaPoint( tmp_pos, tmp_vel, n_dash - ( i + 1 ) );

            Vector2D my_final_pos = M_world.self().pos() + tmp_pos.rotate( dash_angle );
            if ( my_inertia.dist2( my_final_pos ) > 0.01 )
            {
                my_final_pos = Line2D( my_inertia, my_final_pos ).projection( ball_pos );
            }

            stamina_model.simulateWaits( ptype, n_dash - ( i + 1 ) );

            InterceptInfo::Mode mode = ( stamina_model.recovery() < M_world.self().recovery()
                                         && ! stamina_model.capacityIsEmpty()
                                         ? InterceptInfo::EXHAUST
                                         : InterceptInfo::NORMAL );
            InterceptInfo tmp( mode,
                               n_turn, n_dash,
                               first_dash_power, 0.0,
                               my_final_pos,
                               my_final_pos.dist( ball_pos ),
                               stamina_model.stamina() );
            tmp.setTackle();
            self_cache.push_back( tmp );
            return true;
        }
    }

#ifdef DEBUG_PRINT_LONG_STEP_LEVEL_2
    dlog.addText( Logger::INTERCEPT,
                  "%d ______dash %d: no run over. pmove=(%.2f(noised=%.2f), %.2f) ball_x=%.3f"
                  " x_diff=%.3f y_diff=%.3f",
                  n_turn + n_dash,
                  n_dash,
                  tmp_pos.x, tmp_pos.x * PLAYER_NOISE_RATE, tmp_pos.y,
                  noised_ball_x,
                  tmp_pos.x * PLAYER_NOISE_RATE - noised_ball_x,
                  std::fabs( tmp_pos.y - ball_rel.y ) );
#endif

    //////////////////////////////////////////////////////////
    //     // when cycle is small, do strict check
    //     if ( n_turn + n_dash <= 6 )
    {
        // tmp_pos is relative to playerPos() --> tmp_pos.r() == player_travel
        double player_travel = tmp_pos.r();
        double player_noise = player_travel * SP.playerRand() * 0.5;
        double last_ball_dist = ball_rel.dist( tmp_pos );
        double buf = 0.2; // 0.1
        //if ( n_turn > 0 ) buf += 0.3;
        //if ( n_turn + n_dash >= 4 )
        {
            buf += player_noise;
            buf += ball_noise;
        }

        if ( last_ball_dist < std::max( control_area - 0.225, control_area - buf ) )
            //         if ( std::fabs( tmp_pos.x - ball_rel.x ) < control_area - 0.15
            //              && std::fabs( tmp_pos.y - ball_rel.y ) < control_area - buf
            //              && last_ball_dist < control_area )
        {
            Vector2D my_final_pos = M_world.self().pos() + tmp_pos.rotate( dash_angle );
#ifdef DEBUG_PRINT_LONG_STEP_LEVEL_1
            dlog.addText( Logger::INTERCEPT,
                          "%d ____dash %d: can reach.last ball dist=%.3f. noised_ctrl_area=%.3f/%.3f",
                          n_turn + n_dash,
                          n_dash,
                          last_ball_dist,
                          std::max( control_area - 0.225, control_area - buf ),
                          control_area );
            dlog.addText( Logger::INTERCEPT,
                          "%d ____ player_noise=%f ball_noise=%f buf=%f",
                          n_turn + n_dash,
                          player_noise, ball_noise, buf );
#endif
            *result_recovery = stamina_model.recovery();
            InterceptInfo::Mode mode = ( stamina_model.recovery() < M_world.self().recovery()
                                         && ! stamina_model.capacityIsEmpty()
                                         ? InterceptInfo::EXHAUST
                                         : InterceptInfo::NORMAL );
            InterceptInfo tmp( mode,
                             n_turn, n_dash,
                             first_dash_power, 0.0,
                             my_final_pos,
                             my_final_pos.dist( ball_pos ),
                             stamina_model.stamina() );
            tmp.setTackle();
            self_cache.push_back( tmp);
            return true;
        }

#ifdef DEBUG_PRINT_LONG_STEP_LEVEL_2
        dlog.addText( Logger::INTERCEPT,
                      "%d ______dash %d: failed. last_ball_dist=%.3f noised_ctrl_area=%.3f/%.3f",
                      n_turn + n_dash,
                      n_dash,
                      last_ball_dist,
                      std::max( control_area - 0.225, control_area - buf ),
                      control_area );
        dlog.addText( Logger::INTERCEPT,
                      "%d ______player_rel=(%.3f %.3f) ball_rel=(%.3f %.3f)  p_noise=%.3f b_noise=%.3f",
                      n_turn + n_dash,
                      tmp_pos.x, tmp_pos.y,
                      ball_rel.x, ball_rel.y,
                      player_noise,
                      ball_noise );
        dlog.addText( Logger::INTERCEPT,
                      "%d ______noised_ball_x=%.3f  bnoise=%.3f  pnoise=%.3f",
                      n_turn + n_dash,
                      noised_ball_x,
                      ball_noise, player_noise );
#endif
        return false;
    }

#ifdef DEBUG_PRINT_LONG_STEP_LEVEL_2
    dlog.addText( Logger::INTERCEPT,
                  "%d ____dash %d: cannot reach. player_rel=(%f %f)  ball_rel=(%f %f)"
                  "  noise_ball_x=%f",
                  n_turn + n_dash,
                  n_dash,
                  tmp_pos.x, tmp_pos.y,
                  ball_rel.x, ball_rel.y,
                  noised_ball_x );
#endif
    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
SelfInterceptTackle::predictTurnDashLong( const int cycle,
                                          const Vector2D & ball_pos,
                                          const double & control_area,
                                          const bool save_recovery,
                                          const bool back_dash,
                                          std::vector< InterceptInfo > & self_cache ) const
{
    AngleDeg dash_angle = M_world.self().body();
    int n_turn = predictTurnCycleLong( cycle, ball_pos, control_area, back_dash, &dash_angle );
    if ( n_turn > cycle )
    {
#ifdef DEBUG_PRINT_LONG_STEP
        dlog.addText( Logger::INTERCEPT,
                      "(predictShortStep_cycle=%d) turn=%d over",
                      cycle, n_turn );
#endif
        return;
    }

    predictDashCycleLong( cycle, n_turn, ball_pos, dash_angle, control_area, save_recovery, back_dash,
                          self_cache );
}

/*-------------------------------------------------------------------*/
/*!

*/
int
SelfInterceptTackle::predictTurnCycleLong( const int cycle,
                                           const Vector2D & ball_pos,
                                           const double & control_area,
                                           const bool back_dash,
                                           AngleDeg * result_dash_angle ) const
{
    const ServerParam & SP = ServerParam::i();
    const double max_moment = SP.maxMoment();

    const SelfObject & self = M_world.self();
    const PlayerType & ptype = self.playerType();

    const double dist_thr = control_area - 0.1;

    const Vector2D inertia_pos = self.inertiaPoint( cycle );
    const double target_dist = ( ball_pos - inertia_pos ).r();
    const AngleDeg target_angle = ( ball_pos - inertia_pos ).th();

    int n_turn = 0;

    const AngleDeg body_angle = ( back_dash
                                  ? self.body() + 180.0
                                  : self.body() );
    double angle_diff = ( target_angle - body_angle ).abs();

    double turn_margin = 180.0;
    if ( dist_thr < target_dist )
    {
        turn_margin = std::max( MIN_TURN_THR,
                                AngleDeg::asin_deg( dist_thr / target_dist ) );
    }

    double my_speed = self.vel().r();
    while ( angle_diff > turn_margin )
    {
        angle_diff -= ptype.effectiveTurn( max_moment, my_speed );
        my_speed *= ptype.playerDecay();
        ++n_turn;
    }

    *result_dash_angle = body_angle;
    if ( n_turn > 0 )
    {
        angle_diff = std::max( 0.0, angle_diff );
        if ( ( target_angle - body_angle ).degree() > 0.0 )
        {
            *result_dash_angle = target_angle - angle_diff;
        }
        else
        {
            *result_dash_angle = target_angle + angle_diff;
        }
    }

#ifdef DEBUG_PRINT_LONG_STEP
    dlog.addText( Logger::INTERCEPT,
                  "(predictTurnCycleLong) cycle=%d turn=%d"
                  " turn_margin=%.1f"
                  " turn_moment=%.1f"
                  " first_angle_diff=%.1f"
                  " final_angle_diff=%.1f"
                  " dash_angle=%.1f",
                  cycle, n_turn,
                  turn_margin,
                  ( *result_dash_angle - body_angle ).degree(),
                  ( target_angle - body_angle ).degree(),
                  angle_diff,
                  result_dash_angle->degree() );
#endif

    return n_turn;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
SelfInterceptTackle::predictDashCycleLong( const int cycle,
                                           const int n_turn,
                                           const Vector2D & ball_pos,
                                           const AngleDeg & dash_angle,
                                           const double & control_area,
                                           const bool save_recovery,
                                           const bool back_dash,
                                           std::vector< InterceptInfo > & self_cache ) const
{
    const ServerParam & SP = ServerParam::i();
    const SelfObject & self = M_world.self();
    const PlayerType & ptype = self.playerType();

    const double recover_dec_thr = SP.recoverDecThrValue() + 1.0;
    const int max_dash = cycle - n_turn;

    const Vector2D my_inertia = self.inertiaPoint( cycle );

    Vector2D my_pos = self.inertiaPoint( n_turn );
    Vector2D my_vel = self.vel() * std::pow( ptype.playerDecay(), n_turn );

    StaminaModel stamina_model = self.staminaModel();
    stamina_model.simulateWaits( ptype, n_turn );

    if ( my_inertia.dist2( ball_pos ) < std::pow( control_area - 0.1, 2 ) )
    {
        Vector2D my_final_pos = my_inertia;
        //         if ( my_inertia.dist2( my_pos ) > 0.01 )
        //         {
        //             my_final_pos = Line2D( my_inertia, my_pos ).projection( ball_pos );
        //         }

        StaminaModel tmp_stamina = stamina_model;
        tmp_stamina.simulateWaits( ptype, cycle - n_turn );
#ifdef DEBUG_PRINT_LONG_STEP
        dlog.addText( Logger::INTERCEPT,
                      "(predictDashCycleLong) **OK** can reach. cycle=%d turn=%d dash=0."
                      " bpos(%.1f %.1f) my_inertia=(%.1f %.1f) dist=%.3f stamina=%.1f",
                      cycle, n_turn,
                      ball_pos.x, ball_pos.y,
                      my_inertia.x, my_inertia.y,
                      my_final_pos.dist( ball_pos ),
                      tmp_stamina.stamina() );
#endif
        self_cache.push_back( InterceptInfo( InterceptInfo::NORMAL,
                                             n_turn, cycle - n_turn,
                                             0.0, 0.0,
                                             my_final_pos,
                                             my_final_pos.dist( ball_pos ),
                                             tmp_stamina.stamina() ) );
    }

    const AngleDeg target_angle = ( ball_pos - my_inertia ).th();
    if ( ( target_angle - dash_angle ).abs() > 90.0 )
    {
#ifdef DEBUG_PRINT_LONG_STEP
        dlog.addText( Logger::INTERCEPT,
                      "(predictDashCycleLong) XXX cycle=%d turn=%d."
                      " (target_angle(%.1f) - dash_angle(%.1f)) > 90",
                      target_angle.degree(), dash_angle.degree() );
#endif
        return;
    }

    const Vector2D accel_unit = Vector2D::polar2vector( 1.0, dash_angle );
    double first_dash_power = 0.0;

    for ( int n_dash = 1; n_dash <= max_dash; ++n_dash )
    {
        double available_stamina = ( save_recovery
                                     ? std::max( 0.0, stamina_model.stamina() - recover_dec_thr )
                                     : stamina_model.stamina() + ptype.extraStamina() );
        double dash_power = 0.0;
        if ( back_dash )
        {
            available_stamina *= 0.5;
            dash_power = bound( SP.minDashPower(), -available_stamina, 0.0 );
        }
        else
        {
            dash_power = bound( 0.0, available_stamina, SP.maxDashPower() );
        }

        if ( n_dash == 1 )
        {
            first_dash_power = dash_power;
        }

        double accel_mag = std::fabs( dash_power * ptype.dashRate( stamina_model.effort() ) );
        Vector2D accel = accel_unit * accel_mag;

        my_vel += accel;
        my_pos += my_vel;
        my_vel *= ptype.playerDecay();

        stamina_model.simulateDash( ptype, dash_power );

        Vector2D inertia_pos = ptype.inertiaPoint( my_pos, my_vel, cycle - n_turn - n_dash );
        AngleDeg my_move_angle = ( inertia_pos - self.pos() ).th();
        Vector2D target_rel = ( ball_pos - self.pos() ).rotatedVector( -my_move_angle );
        if ( std::pow( target_rel.x, 2 ) < ( inertia_pos - self.pos() ).r2() )
        {
            InterceptInfo::Mode mode = ( stamina_model.stamina() < SP.recoverDecThrValue()
                                         && ! stamina_model.capacityIsEmpty()
                                         ? InterceptInfo::EXHAUST
                                         : InterceptInfo::NORMAL );
            Vector2D my_final_pos = inertia_pos;
            if ( inertia_pos.dist2( my_inertia ) > 0.01 )
            {
                my_final_pos = Line2D( inertia_pos, my_inertia ).projection( ball_pos );
            }
            stamina_model.simulateWaits( ptype, cycle - n_turn - n_dash );
#ifdef DEBUG_PRINT_LONG_STEP
            dlog.addText( Logger::INTERCEPT,
                          "(predictDashCycleLong) **OK** can run over."
                          " cycle=%d turn=%d dash=%d"
                          " bpos(%.1f %.1f) inertia_pos=(%.1f %.1f) final_pos=(%.1f %.1f)"
                          " target_rel.x=%.3f my_move=%.3f"
                          " ball_dist=%.3f"
                          " first_dash_power=%.1f stamina=%.1f",
                          cycle, n_turn, n_dash,
                          ball_pos.x, ball_pos.y,
                          inertia_pos.x, inertia_pos.y,
                          my_final_pos.x, my_final_pos.y,
                          target_rel.x, ( inertia_pos - self.pos() ).r(),
                          my_final_pos.dist( ball_pos ),
                          first_dash_power, stamina_model.stamina() );
#endif
            self_cache.push_back( InterceptInfo( mode, n_turn, cycle - n_turn,
                                                 first_dash_power, 0.0,
                                                 my_final_pos,
                                                 my_final_pos.dist( ball_pos ),
                                                 stamina_model.stamina() ) );
            return;
        }
    }

    if ( my_pos.dist2( ball_pos ) < std::pow( control_area - 0.1, 2 ) )
    {
        InterceptInfo::Mode mode = ( stamina_model.stamina() < SP.recoverDecThrValue()
                                     && ! stamina_model.capacityIsEmpty()
                                     ? InterceptInfo::EXHAUST
                                     : InterceptInfo::NORMAL );
#ifdef DEBUG_PRINT_LONG_STEP
        dlog.addText( Logger::INTERCEPT,
                      "(predictDashCycleLong) **OK** controllable cycle=%d turn=%d dash=%d."
                      " bpos(%.1f %.1f) my_pos=(%.1f %.1f) ball_dist=%.3f"
                      " first_dash_power=%.1f stamina=%.1f",
                      cycle, n_turn, cycle - n_turn,
                      ball_pos.x, ball_pos.y,
                      my_pos.x, my_pos.y,
                      my_pos.dist( ball_pos ),
                      first_dash_power, stamina_model.stamina() );
#endif
        self_cache.push_back( InterceptInfo( mode, n_turn, cycle - n_turn,
                                             first_dash_power, 0.0,
                                             my_pos,
                                             my_pos.dist( ball_pos ),
                                             stamina_model.stamina() ) );
        return;
    }

#ifdef DEBUG_PRINT_LONG_STEP
    dlog.addText( Logger::INTERCEPT,
                  "(predictDashCycleLong) XXX cycle=%d turn=%d dash=%d."
                  " bpos(%.1f %.1f) mypos=(%.1f %.1f) ball_dist=%.3f my_dash_move=%.3f",
                  cycle, n_turn, max_dash,
                  ball_pos.x, ball_pos.y,
                  my_pos.x, my_pos.y,
                  my_pos.dist( ball_pos ),
                  my_inertia.dist( my_pos ) );
#endif
}

}
