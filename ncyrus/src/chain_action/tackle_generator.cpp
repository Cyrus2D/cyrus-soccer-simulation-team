// -*-c++-*-

/*!
  \file tackle_generator.cpp
  \brief tackle/foul generator class Source File
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

#include "tackle_generator.h"

#include "field_analyzer.h"

#include <rcsc/player/player_agent.h>
#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/ray_2d.h>
#include <rcsc/geom/rect_2d.h>
#include <rcsc/math_util.h>
#include <rcsc/timer.h>

#include <limits>

#define ASUUME_OPPONENT_KICK

#define DEBUG_PROFILE
// #define DEBUG_PRINT
// #define DEBUG_PRINT_EVALUATE

// #define DEBUG_PREDICT_OPPONENT_REACH_STEP
// #define DEBUG_PREDICT_OPPONENT_REACH_STEP_LEVEL2
#define MY_DEBUG

using namespace rcsc;

namespace {

const int ANGLE_DIVS = 40;

/*-------------------------------------------------------------------*/
/*!

 */
struct DeflectingEvaluator {
    constexpr static const double not_shoot_ball_eval = 10000;

    double operator()( const WorldModel & wm,
                       TackleGenerator::TackleResult & result ) const
    {
        double eval = 0.0;

        if ( ! FieldAnalyzer::is_ball_moving_to_our_goal( wm.ball().pos(),
                                                          result.ball_vel_,
                                                          2.0 ) )
        {
            eval += not_shoot_ball_eval;
        }

        Vector2D final_ball_pos = inertia_final_point( wm.ball().pos(),
                                                       result.ball_vel_,
                                                       ServerParam::i().ballDecay() );

        if ( result.ball_vel_.x < 0.0
             && wm.ball().pos().x > ServerParam::i().ourTeamGoalLineX() + 0.5 )
        {
            //const double goal_half_width = ServerParam::i().goalHalfWidth();
            const double pitch_half_width = ServerParam::i().pitchHalfWidth();
            const double goal_line_x = ServerParam::i().ourTeamGoalLineX();
            const Vector2D corner_plus_post( goal_line_x, +pitch_half_width );
            const Vector2D corner_minus_post( goal_line_x, -pitch_half_width );

            const Line2D goal_line( corner_plus_post, corner_minus_post );

            const Segment2D ball_segment( wm.ball().pos(), final_ball_pos );
            Vector2D cross_point = ball_segment.intersection( goal_line );
            if ( cross_point.isValid() )
            {
                eval += 1000.0;

                double c = std::min( std::fabs( cross_point.y ),
                                     ServerParam::i().pitchHalfWidth() );
                //if ( c > goal_half_width
                //     && ( cross_point.y * wm.self().pos().y >= 0.0
                //          && c > wm.self().pos().absY() ) )
                {
                    eval += c;
                }
            }
        }
        else
        {
            if ( final_ball_pos.x > ServerParam::i().ourTeamGoalLineX() + 5.0 )
            {
                eval += 1000.0;
            }

            eval += sign( wm.ball().pos().y ) * result.ball_vel_.y;
        }

        return eval;
    }
};

}



/*-------------------------------------------------------------------*/
/*!

 */
TackleGenerator::TackleResult::TackleResult()
{
    clear();
}

/*-------------------------------------------------------------------*/
/*!

 */
TackleGenerator::TackleResult::TackleResult( const rcsc::AngleDeg & angle,
                                             const rcsc::Vector2D & vel )
    : tackle_angle_( angle ),
      ball_vel_( vel ),
      ball_speed_( vel.r() ),
      ball_move_angle_( vel.th() ),
      score_( 0.0 )
{

}

/*-------------------------------------------------------------------*/
/*!

 */
void
TackleGenerator::TackleResult::clear()
{
    tackle_angle_ = 0.0;
    ball_vel_.assign( 0.0, 0.0 );
    ball_speed_ = 0.0;
    ball_move_angle_ = 0.0;
    score_ = -std::numeric_limits< double >::max();
}

/*-------------------------------------------------------------------*/
/*!

 */
TackleGenerator::TackleGenerator()
{
    M_candidates.reserve( ANGLE_DIVS );
    clear();
}

/*-------------------------------------------------------------------*/
/*!

 */
TackleGenerator &
TackleGenerator::instance()
{
    static TackleGenerator s_instance;
    return s_instance;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
TackleGenerator::clear()
{
    M_best_result = TackleResult();
    M_candidates.clear();
}

/*-------------------------------------------------------------------*/
/*!

 */
void
TackleGenerator::generate( const WorldModel & wm )
{
    static GameTime s_update_time( 0, 0 );

    dlog.addText(Logger::CLEAR,"in gennnnnnnnnnnn");
    if ( s_update_time == wm.time() )
    {
        // dlog.addText( Logger::CLEAR,
        //               __FILE__": already updated" );
        return;
    }
    s_update_time = wm.time();

    clear();

    if ( wm.self().isKickable() )
    {
        //         dlog.addText( Logger::CLEAR,
        //                       __FILE__": kickable" );
                return;
    }

    if ( wm.self().tackleProbability() < 0.001
         && wm.self().foulProbability() < 0.001 )
    {
        // dlog.addText( Logger::CLEAR,
        //               __FILE__": never tacklable" );
        return;
    }

    if ( wm.time().stopped() > 0 )
    {
        // dlog.addText( Logger::CLEAR,
        //               __FILE__": time stopped" );
        return;
    }

    if ( wm.gameMode().type() != GameMode::PlayOn
         && ! wm.gameMode().isPenaltyKickMode() )
    {
        // dlog.addText( Logger::CLEAR,
        //               __FILE__": illegal playmode " );
                return;
    }


#ifdef DEBUG_PROFILE
    MSecTimer timer;
#endif
    calculate( wm );

#ifdef DEBUG_PROFILE
    dlog.addText( Logger::CLEAR,
                  __FILE__": PROFILE. elapsed=%.3f [ms]",
                  timer.elapsedReal() );
#endif

}

/*-------------------------------------------------------------------*/
/*!

 */
static int can_num = 0;
void
TackleGenerator::calculate( const WorldModel & wm )
{
    const ServerParam & SP = ServerParam::i();

    const double min_angle = SP.minMoment();
    const double max_angle = SP.maxMoment();
    const double angle_step = std::fabs( max_angle - min_angle ) / ANGLE_DIVS;

#ifdef ASSUME_OPPONENT_KICK
    const Vector2D goal_pos = SP.ourTeamGoalPos();
    const bool shootable_area = ( wm.ball().pos().dist2( goal_pos ) < std::pow( 18.0, 2 ) );
    const Vector2D shoot_accel = ( goal_pos - wm.ball().pos() ).setLengthVector( 2.0 );
#endif

    const AngleDeg ball_rel_angle = wm.ball().angleFromSelf() - wm.self().body();
    const double tackle_rate
            = SP.tacklePowerRate()
            * ( 1.0 - 0.5 * ball_rel_angle.abs() / 180.0 );

#ifdef DEBUG_PRINT
    dlog.addText( Logger::CLEAR,
                  __FILE__": min_angle=%.1f max_angle=%.1f angle_step=%.1f",
                  min_angle, max_angle, angle_step );
    dlog.addText( Logger::CLEAR,
                  __FILE__": ball_rel_angle=%.1f tackle_rate=%.1f",
                  ball_rel_angle.degree(), tackle_rate );
#endif

    for ( int a = 0; a < ANGLE_DIVS; ++a )
    {
        const AngleDeg dir = min_angle + angle_step * a;

        double eff_power= ( SP.maxBackTacklePower()
                            + ( SP.maxTacklePower() - SP.maxBackTacklePower() )
                            * ( 1.0 - ( dir.abs() / 180.0 ) ) );
        eff_power *= tackle_rate;

        AngleDeg angle = wm.self().body() + dir;
        Vector2D accel = Vector2D::from_polar( eff_power, angle );

#ifdef ASSUME_OPPONENT_KICK
        if ( shootable_area
             && wm.maybeKickableOpponent() )
        {
            accel += shoot_accel;
            double d = accel.r();
            if ( d > SP.ballAccelMax() )
            {
                accel *= ( SP.ballAccelMax() / d );
            }
        }
#endif

        Vector2D vel = wm.ball().vel() + accel;

        double speed = vel.r();
        if ( speed > SP.ballSpeedMax() )
        {
            vel *= ( SP.ballSpeedMax() / speed );
        }

        M_candidates.push_back( TackleResult( angle, vel ) );
        const TackleResult & result = M_candidates.back();
#ifdef MY_DEBUG
        dlog.addText( Logger::CLEAR,
                      "%d: angle=%.1f(dir=%.1f), result: vel(%.2f %.2f ) speed=%.2f move_angle=%.1f",
                      a,
                      result.tackle_angle_.degree(), dir.degree(),
                      result.ball_vel_.x, result.ball_vel_.y,
                      result.ball_speed_, result.ball_move_angle_.degree() );

#endif
    }


    M_best_result.clear();

    can_num = 0;
    const Container::iterator end = M_candidates.end();
    for ( Container::iterator it = M_candidates.begin();
          it != end;
          ++it )
    {
        Vector2D ball_end_point = inertia_final_point( wm.ball().pos(),
                                                       it->ball_vel_,
                                                       SP.ballDecay() );

        it->score_ = evaluate( wm, *it );
        can_num++;

#ifdef DEBUG_PRINT
        Vector2D ball_end_point = inertia_final_point( wm.ball().pos(),
                                                       it->ball_vel_,
                                                       SP.ballDecay() );
        dlog.addLine( Logger::CLEAR,
                      wm.ball().pos(),
                      ball_end_point,
                      "#0000ff" );
        char buf[16];
        snprintf( buf, 16, "%.3f", it->score_ );
        dlog.addMessage( Logger::CLEAR,
                         ball_end_point, buf, "#ffffff" );
#endif

        if ( it->score_ > M_best_result.score_ )
        {
#ifdef DEBUG_PRINT
            dlog.addText( Logger::CLEAR,
                          ">>>> updated" );
#endif
            M_best_result = *it;
        }
    }


#ifdef DEBUG_PRINT
    dlog.addLine( Logger::CLEAR,
                  wm.ball().pos(),
                  inertia_final_point( wm.ball().pos(),
                                       M_best_result.ball_vel_,
                                       SP.ballDecay() ),
                  "#ff0000" );
    dlog.addText( Logger::CLEAR,
                  "==== best_angle=%.1f score=%f speed=%.3f move_angle=%.1f",
                  M_best_result.tackle_angle_.degree(),
                  M_best_result.score_,
                  M_best_result.ball_speed_,
                  M_best_result.ball_move_angle_.degree() );
#endif
}

/*-------------------------------------------------------------------*/
/*!

 */

double
TackleGenerator::evaluate( const WorldModel & wm,
                           TackleResult & result )
{
    const ServerParam & SP = ServerParam::i();

    const Vector2D ball_end_point = inertia_final_point( wm.ball().pos(),
                                                         result.ball_vel_,
                                                         SP.ballDecay() );
    const Segment2D ball_line( wm.ball().pos(), ball_end_point );
    const double ball_speed = result.ball_speed_;
    const AngleDeg ball_move_angle = result.ball_move_angle_;

#ifdef DEBUG_PRINT
    dlog.addText( Logger::CLEAR,
                  "(evaluate) angle=%.1f speed=%.2f move_angle=%.1f end_point=(%.2f %.2f)",
                  result.tackle_angle_.degree(),
                  ball_speed,
                  ball_move_angle.degree(),
                  ball_end_point.x, ball_end_point.y );
#endif



    //
    // normal evaluation
    //

    int tm = 0,opp = 0;
    int tm_dif = 5,opp_dif = 5;
    bool use_dif = false;
    int opponent_reach_step = predictOpponentsReachStep( wm,
                                                         wm.ball().pos(),
                                                         result.ball_vel_,
                                                         ball_move_angle,
                                                         opp,
                                                         opp_dif);
    int teammate_reach_step = predictTeammatesReachStep( wm,
                                                         wm.ball().pos(),
                                                         result.ball_vel_,
                                                         ball_move_angle,
                                                         tm,
                                                         tm_dif);
    int self_reach_step = predictSelfReachStep( wm,
                                                wm.ball().pos(),
                                                result.ball_vel_,
                                                ball_move_angle);

    if( teammate_reach_step > self_reach_step){
        teammate_reach_step = self_reach_step;
        tm = wm.self().unum();
        tm_dif = 0;
    }

    /*
     * ourgoal = -1000
     * theirgoal = 1000
     * out = x [0,100]
     * team = (x + 40-dist2goal)/max*100 + 100 ->[100,200]
     * opp = x [-200,-100]
     *
     * */
    if(opponent_reach_step == 1000 && teammate_reach_step == 1000){
        //
        // moving to their goal
        //
        if ( ball_end_point.x > SP.pitchHalfLength()
             && wm.ball().pos().dist2( SP.theirTeamGoalPos() ) < std::pow( 20.0, 2 ) )
        {
            const Line2D goal_line( Vector2D( SP.pitchHalfLength(), 10.0 ),
                                    Vector2D( SP.pitchHalfLength(), -10.0 ) );
            const Vector2D intersect = ball_line.intersection( goal_line );
            if ( intersect.isValid()
                 && intersect.absY() < SP.goalHalfWidth() )
            {
                double shoot_score = 1000.0;
                double y_rate = 6.0 - std::abs(intersect.absY() - 6.0);
                shoot_score *= y_rate;
                if(use_dif)
                    shoot_score *= opp_dif;
#ifdef MY_DEBUG
                dlog.addLine(Logger::CLEAR,wm.ball().pos(),ball_end_point,0,255,0);
                char str[16];
                snprintf( str, 16, "%d", can_num );
                dlog.addMessage(Logger::CLEAR,ball_end_point.x,ball_end_point.y,str);
                dlog.addText(Logger::CLEAR,"t%d: OppGoal:bmangle:%.1f, tangle:%.1f, score:%.1f, (%.1f,%.1f)",can_num,result.ball_move_angle_.degree(),result.tackle_angle_.degree(),shoot_score,ball_end_point.x,ball_end_point.y);
#endif
                return shoot_score;
            }
        }
        //
        // moving to our goal
        //
        else if ( ball_end_point.x < -SP.pitchHalfLength() - 2)
        {
            const Line2D goal_line( Vector2D( -SP.pitchHalfLength(), 10.0 ),
                                    Vector2D( -SP.pitchHalfLength(), -10.0 ) );
            const Vector2D intersect = ball_line.intersection( goal_line );
            if ( intersect.isValid()
                 && intersect.absY() < SP.goalHalfWidth() - 1.0 )
            {
                double shoot_score = -1000.0;
#ifdef MY_DEBUG
                dlog.addLine(Logger::CLEAR,wm.ball().pos(),ball_end_point,255,0,0);
                char str[16];
                snprintf( str, 16, "%d", can_num );
                dlog.addMessage(Logger::CLEAR,ball_end_point.x,ball_end_point.y,str);
                dlog.addText(Logger::CLEAR,"t%d: Ourgoal:bmangle:%.1f, tangle:%.1f, score:%.1f, (%.1f,%.1f)",can_num,result.ball_move_angle_.degree(),result.tackle_angle_.degree(),shoot_score,ball_end_point.x,ball_end_point.y);
#endif
                return shoot_score;
            }
            if ( intersect.isValid()
                 && intersect.absY() < SP.goalHalfWidth() + 5.0 )
            {
                double shoot_score = -500.0;
#ifdef MY_DEBUG
                dlog.addLine(Logger::CLEAR,wm.ball().pos(),ball_end_point,255,30,0);
                char str[16];
                snprintf( str, 16, "%d", can_num );
                dlog.addMessage(Logger::CLEAR,ball_end_point.x,ball_end_point.y,str);
                dlog.addText(Logger::CLEAR,"t%d: Ourgoal:bmangle:%.1f, tangle:%.1f, score:%.1f, (%.1f,%.1f)",can_num,result.ball_move_angle_.degree(),result.tackle_angle_.degree(),shoot_score,ball_end_point.x,ball_end_point.y);
#endif
                return shoot_score;
            }
        }
        if ( ball_end_point.x < -SP.pitchHalfLength() - 1)
        {
            const Line2D goal_line( Vector2D( -SP.pitchHalfLength(), 10.0 ),
                                    Vector2D( -SP.pitchHalfLength(), -10.0 ) );
            const Vector2D intersect = ball_line.intersection( goal_line );
            if ( intersect.isValid()
                 && intersect.absY() < SP.goalHalfWidth() - 2.0 )
            {
                double shoot_score = -1000.0;
#ifdef MY_DEBUG
                dlog.addLine(Logger::CLEAR,wm.ball().pos(),ball_end_point,255,0,0);
                char str[16];
                snprintf( str, 16, "%d", can_num );
                dlog.addMessage(Logger::CLEAR,ball_end_point.x,ball_end_point.y,str);
                dlog.addText(Logger::CLEAR,"t%d: Ourgoal:bmangle:%.1f, tangle:%.1f, score:%.1f, (%.1f,%.1f)",can_num,result.ball_move_angle_.degree(),result.tackle_angle_.degree(),shoot_score,ball_end_point.x,ball_end_point.y);
#endif
                return shoot_score;
            }
            if ( intersect.isValid()
                 && intersect.absY() < SP.goalHalfWidth() + 5.0 )
            {
                double shoot_score = -450.0;
#ifdef MY_DEBUG
                dlog.addLine(Logger::CLEAR,wm.ball().pos(),ball_end_point,255,30,0);
                char str[16];
                snprintf( str, 16, "%d", can_num );
                dlog.addMessage(Logger::CLEAR,ball_end_point.x,ball_end_point.y,str);
                dlog.addText(Logger::CLEAR,"t%d: Ourgoal:bmangle:%.1f, tangle:%.1f, score:%.1f, (%.1f,%.1f)",can_num,result.ball_move_angle_.degree(),result.tackle_angle_.degree(),shoot_score,ball_end_point.x,ball_end_point.y);
#endif
                return shoot_score;
            }
        }
        if ( ball_end_point.x < -SP.pitchHalfLength())
        {
            const Line2D goal_line( Vector2D( -SP.pitchHalfLength(), 10.0 ),
                                    Vector2D( -SP.pitchHalfLength(), -10.0 ) );
            const Vector2D intersect = ball_line.intersection( goal_line );
            if ( intersect.isValid()
                 && intersect.absY() < SP.goalHalfWidth() - 2.0 )
            {
                double shoot_score = -1000.0;
#ifdef MY_DEBUG
                dlog.addLine(Logger::CLEAR,wm.ball().pos(),ball_end_point,255,0,0);
                char str[16];
                snprintf( str, 16, "%d", can_num );
                dlog.addMessage(Logger::CLEAR,ball_end_point.x,ball_end_point.y,str);
                dlog.addText(Logger::CLEAR,"t%d: Ourgoal:bmangle:%.1f, tangle:%.1f, score:%.1f, (%.1f,%.1f)",can_num,result.ball_move_angle_.degree(),result.tackle_angle_.degree(),shoot_score,ball_end_point.x,ball_end_point.y);
#endif
                return shoot_score;
            }
            if ( intersect.isValid()
                 && intersect.absY() < SP.goalHalfWidth() + 5.0 )
            {
                double shoot_score = -410.0;
#ifdef MY_DEBUG
                dlog.addLine(Logger::CLEAR,wm.ball().pos(),ball_end_point,255,30,0);
                char str[16];
                snprintf( str, 16, "%d", can_num );
                dlog.addMessage(Logger::CLEAR,ball_end_point.x,ball_end_point.y,str);
                dlog.addText(Logger::CLEAR,"t%d: Ourgoal:bmangle:%.1f, tangle:%.1f, score:%.1f, (%.1f,%.1f)",can_num,result.ball_move_angle_.degree(),result.tackle_angle_.degree(),shoot_score,ball_end_point.x,ball_end_point.y);
#endif
                return shoot_score;
            }
        }
        {
            const Vector2D ball_end_point = inertia_final_point( wm.ball().pos(),
                                                                 result.ball_vel_,
                                                                 SP.ballDecay() );
            if ( ball_end_point.absX() > SP.pitchHalfLength()
                 || ball_end_point.absY() > SP.pitchHalfWidth() )
            {
                Rect2D pitch = Rect2D::from_center( 0.0, 0.0, SP.pitchLength(), SP.pitchWidth() );
                Ray2D ball_ray( wm.ball().pos(), ball_move_angle );
                Vector2D sol1, sol2;
                int n_sol = pitch.intersection( ball_ray, &sol1, &sol2 );
                if ( n_sol == 1 )
                {
                    double score = sol1.x;
                    score += 52.5;
                    score *= (100.0 / 105.0);
#ifdef DEBUG_PRINT
                    dlog.addText( Logger::CLEAR,
                                  "(predictOpponent) ball will be out. step=%d reach_point=(%.2f %.2f)",
                                  first_min_step,
                                  sol1.x, sol1.y );
#endif
#ifdef MY_DEBUG
                    dlog.addLine(Logger::CLEAR,wm.ball().pos(),ball_end_point,255,200,0);
                    char str[16];
                    snprintf( str, 16, "%d", can_num );
                    dlog.addMessage(Logger::CLEAR,ball_end_point.x,ball_end_point.y,str);
                    dlog.addText(Logger::CLEAR,"t%d: Out:bmangle:%.1f, tangle:%.1f, score:%.1f, (%.1f,%.1f)",can_num,result.ball_move_angle_.degree(),result.tackle_angle_.degree(),score,ball_end_point.x,ball_end_point.y);
#endif
                    return score;
                }
            }
        }
    }
    else if (opponent_reach_step <= teammate_reach_step){
        Vector2D final_point = inertia_n_step_point( wm.ball().pos(),
                                                     result.ball_vel_,
                                                     opponent_reach_step,
                                                     SP.ballDecay() );
        double score = final_point.x;
        score += 52.5;
        score *= (100.0 / 105.5);
        score -= 200.0;
#ifdef MY_DEBUG
        dlog.addLine(Logger::CLEAR,wm.ball().pos(),final_point,0,0,0);
        char str[16];
        snprintf( str, 16, "%d", can_num );
        dlog.addMessage(Logger::CLEAR,final_point.x,final_point.y,str);
        dlog.addText(Logger::CLEAR,"t%d: Opp %d:bmangle:%.1f, tangle:%.1f, score:%.1f, (%.1f,%.1f)",can_num,opp,result.ball_move_angle_.degree(),result.tackle_angle_.degree(),score,final_point.x,final_point.y);
#endif
        return score;
    }else{
        Vector2D final_point = inertia_n_step_point( wm.ball().pos(),
                                                     result.ball_vel_,
                                                     teammate_reach_step,
                                                     SP.ballDecay() );
        double score = final_point.x + std::min(0.0,40.0 - final_point.dist(Vector2D(52,0)));
        score += 52.5;
        score *= (100.0 / 105.5);
        score += 100.0;
        if(use_dif)
            score += (10.0 * tm_dif);
        if(use_dif)
            score += (10.0 * opp_dif);
        if(final_point.x < -35 && final_point.absY() < 15)
            score -= 100;

#ifdef MY_DEBUG
        dlog.addLine(Logger::CLEAR,wm.ball().pos(),final_point,255,255,255);
        char str[16];
        snprintf( str, 16, "%d", can_num );
        dlog.addMessage(Logger::CLEAR,final_point.x,final_point.y,str);
        dlog.addText(Logger::CLEAR,"t%d: Pass to %d:bmangle:%.1f, tangle:%.1f, score:%.1f, (%.1f,%.1f)",can_num,tm,result.ball_move_angle_.degree(),result.tackle_angle_.degree(),score,final_point.x,final_point.y);
#endif
        if(tm!=0){
            result.type = tackle_pass;
            result.pass_unum = tm;
            result.pass_target = final_point;
        }
        return score;
    }
    //TODO need test and check, why 0?
    return 0;
}


/*-------------------------------------------------------------------*/
/*!

 */
int
TackleGenerator::predictOpponentsReachStep( const WorldModel & wm,
                                            const Vector2D & first_ball_pos,
                                            const Vector2D & first_ball_vel,
                                            const AngleDeg & ball_move_angle,
                                            int & opp,
                                            int & opp_dif)
{
    int first_min_step = 50;

#if 1
    const ServerParam & SP = ServerParam::i();
    const Vector2D ball_end_point = inertia_final_point( first_ball_pos,
                                                         first_ball_vel,
                                                         SP.ballDecay() );
    if ( ball_end_point.absX() > SP.pitchHalfLength()
         || ball_end_point.absY() > SP.pitchHalfWidth() )
    {
        Rect2D pitch = Rect2D::from_center( 0.0, 0.0, SP.pitchLength(), SP.pitchWidth() );
        Ray2D ball_ray( first_ball_pos, ball_move_angle );
        Vector2D sol1, sol2;
        int n_sol = pitch.intersection( ball_ray, &sol1, &sol2 );
        if ( n_sol == 1 )
        {
            first_min_step = SP.ballMoveStep( first_ball_vel.r(), first_ball_pos.dist( sol1 ) );
#ifdef DEBUG_PRINT
            dlog.addText( Logger::CLEAR,
                          "(predictOpponent) ball will be out. step=%d reach_point=(%.2f %.2f)",
                          first_min_step,
                          sol1.x, sol1.y );
#endif
        }
    }
#endif

    int min_step = first_min_step;
    for ( AbstractPlayerCont::const_iterator
          o = wm.theirPlayers().begin(),
          end = wm.theirPlayers().end();
          o != end;
          ++o )
    {
        int step = predictOpponentReachStep( *o,
                                             first_ball_pos,
                                             first_ball_vel,
                                             ball_move_angle,
                                             min_step,
                                             opp_dif);
        if ( step < min_step )
        {
            min_step = step;
            if((*o)->unum() > 0)
                opp = (*o)->unum();
        }
    }

    return ( min_step == first_min_step
             ? 1000
             : min_step );
}


/*-------------------------------------------------------------------*/
/*!

 */
int
TackleGenerator::predictOpponentReachStep( const AbstractPlayerObject * opponent,
                                           const Vector2D & first_ball_pos,
                                           const Vector2D & first_ball_vel,
                                           const AngleDeg & ball_move_angle,
                                           const int max_cycle,
                                           int & opp_dif )
{
    const ServerParam & SP = ServerParam::i();

    const PlayerType * ptype = opponent->playerTypePtr();
    const double opponent_speed = opponent->vel().r();

    int min_cycle = FieldAnalyzer::estimate_min_reach_cycle( opponent->pos(),
                                                             ptype->realSpeedMax(),
                                                             first_ball_pos,
                                                             ball_move_angle );
    if ( min_cycle < 0 )
    {
        min_cycle = 10;
    }

    for ( int cycle = min_cycle; cycle < max_cycle; ++cycle )
    {
        Vector2D ball_pos = inertia_n_step_point( first_ball_pos,
                                                  first_ball_vel,
                                                  cycle,
                                                  SP.ballDecay() );

        if ( ball_pos.absX() > SP.pitchHalfLength()
             || ball_pos.absY() > SP.pitchHalfWidth() )
        {
#ifdef DEBUG_PREDICT_OPPONENT_REACH_STEP
            dlog.addText( Logger::CLEAR,
                          "__ opponent=%d(%.1f %.1f) step=%d ball is out of pitch. ",
                          opponent->unum(),
                          opponent->pos().x, opponent->pos().y,
                          cycle  );
#endif
            return 1000;
        }

        Vector2D inertia_pos = opponent->inertiaPoint( cycle );
        double target_dist = inertia_pos.dist( ball_pos );

        if ( target_dist - ptype->kickableArea() < 0.001 )
        {
#ifdef DEBUG_PREDICT_OPPONENT_REACH_STEP
            dlog.addText( Logger::CLEAR,
                          "____ opponent=%d(%.1f %.1f) step=%d already there. dist=%.1f",
                          opponent->unum(),
                          opponent->pos().x, opponent->pos().y,
                          cycle,
                          target_dist );
#endif
            opp_dif = 0;
            return cycle;
        }

        double dash_dist = target_dist;
        if ( cycle > 1 )
        {
            dash_dist -= ptype->kickableArea();
            dash_dist -= 0.5; // special bonus
        }

        if ( dash_dist > ptype->realSpeedMax() * cycle )
        {
#ifdef DEBUG_PREDICT_OPPONENT_REACH_STEP_LEVEL2
            dlog.addText( Logger::CLEAR,
                          "______ opponent=%d(%.1f %.1f) cycle=%d dash_dist=%.1f reachable=%.1f",
                          opponent->unum(),
                          opponent->pos().x, opponent->pos().y,
                          cycle, dash_dist, ptype->realSpeedMax()*cycle );
#endif
            continue;
        }

        //
        // dash
        //

        int n_dash = ptype->cyclesToReachDistance( dash_dist );

        if ( n_dash > cycle )
        {
#ifdef DEBUG_PREDICT_OPPONENT_REACH_STEP_LEVEL2
            dlog.addText( Logger::CLEAR,
                          "______ opponent=%d(%.1f %.1f) cycle=%d dash_dist=%.1f n_dash=%d",
                          opponent->unum(),
                          opponent->pos().x, opponent->pos().y,
                          cycle, dash_dist, n_dash );
#endif
            opp_dif = std::min(opp_dif,n_dash+ 2 - cycle);
            continue;
        }

        //
        // turn
        //
        int n_turn = ( opponent->bodyCount() > 1
                       ? 0
                       : FieldAnalyzer::predict_player_turn_cycle( ptype,
                                                                   opponent->body(),
                                                                   opponent_speed,
                                                                   target_dist,
                                                                   ( ball_pos - inertia_pos ).th(),
                                                                   ptype->kickableArea(),
                                                                   true ) );

        int n_step = ( n_turn == 0
                       ? n_turn + n_dash
                       : n_turn + n_dash + 1 ); // 1 step penalty for observation delay
        if ( opponent->isTackling() )
        {
            n_step += 5; // Magic Number
        }

        n_step -= std::min( 3, opponent->posCount() );
        opp_dif = std::min(opp_dif,n_step - cycle);
        if ( n_step <= cycle )
        {
#ifdef DEBUG_PREDICT_OPPONENT_REACH_STEP
            dlog.addText( Logger::CLEAR,
                          "____ opponent=%d(%.1f %.1f) step=%d(t:%d,d:%d) bpos=(%.2f %.2f) dist=%.2f dash_dist=%.2f",
                          opponent->unum(),
                          opponent->pos().x, opponent->pos().y,
                          cycle, n_turn, n_dash,
                          ball_pos.x, ball_pos.y,
                          target_dist,
                          dash_dist );
#endif
            return cycle;
        }

    }

    return 1000;
}

int
TackleGenerator::predictTeammatesReachStep( const WorldModel & wm,
                                            const Vector2D & first_ball_pos,
                                            const Vector2D & first_ball_vel,
                                            const AngleDeg & ball_move_angle,
                                            int & tm,
                                            int & tm_dif)
{
    int first_min_step = 50;

#if 1
    const ServerParam & SP = ServerParam::i();
    const Vector2D ball_end_point = inertia_final_point( first_ball_pos,
                                                         first_ball_vel,
                                                         SP.ballDecay() );
    if ( ball_end_point.absX() > SP.pitchHalfLength()
         || ball_end_point.absY() > SP.pitchHalfWidth() )
    {
        Rect2D pitch = Rect2D::from_center( 0.0, 0.0, SP.pitchLength(), SP.pitchWidth() );
        Ray2D ball_ray( first_ball_pos, ball_move_angle );
        Vector2D sol1, sol2;
        int n_sol = pitch.intersection( ball_ray, &sol1, &sol2 );
        if ( n_sol == 1 )
        {
            first_min_step = SP.ballMoveStep( first_ball_vel.r(), first_ball_pos.dist( sol1 ) );
#ifdef DEBUG_PRINT
            dlog.addText( Logger::CLEAR,
                          "(predictOpponent) ball will be out. step=%d reach_point=(%.2f %.2f)",
                          first_min_step,
                          sol1.x, sol1.y );
#endif
        }
    }
#endif

    int min_step = first_min_step;
    for ( AbstractPlayerCont::const_iterator
          o = wm.ourPlayers().begin(),
          end = wm.ourPlayers().end();
          o != end;
          ++o )
    {
        if((*o)->unum() == wm.self().unum())
            continue;
        if((*o)->goalie())
            continue;
        int step = predictTeammateReachStep( *o,
                                             first_ball_pos,
                                             first_ball_vel,
                                             ball_move_angle,
                                             min_step,
                                             tm_dif );
        if ( step < min_step )
        {
            min_step = step;
            if((*o)->unum() > 0)
                tm = (*o)->unum();
        }
    }

    return ( min_step == first_min_step
             ? 1000
             : min_step );
}


/*-------------------------------------------------------------------*/
/*!

 */
int
TackleGenerator::predictTeammateReachStep( const AbstractPlayerObject * teammate,
                                           const Vector2D & first_ball_pos,
                                           const Vector2D & first_ball_vel,
                                           const AngleDeg & ball_move_angle,
                                           const int max_cycle,
                                           int & tm_dif )
{
    const ServerParam & SP = ServerParam::i();

    const PlayerType * ptype = teammate->playerTypePtr();
    const double teammate_speed = teammate->vel().r();

    int min_cycle = FieldAnalyzer::estimate_min_reach_cycle( teammate->pos(),
                                                             ptype->realSpeedMax(),
                                                             first_ball_pos,
                                                             ball_move_angle );
    if ( min_cycle < 0 )
    {
        min_cycle = 5;
    }

    for ( int cycle = min_cycle; cycle < max_cycle; ++cycle )
    {
        Vector2D ball_pos = inertia_n_step_point( first_ball_pos,
                                                  first_ball_vel,
                                                  cycle,
                                                  SP.ballDecay() );

        if ( ball_pos.absX() > SP.pitchHalfLength()
             || ball_pos.absY() > SP.pitchHalfWidth() )
        {
#ifdef DEBUG_PREDICT_OPPONENT_REACH_STEP
            dlog.addText( Logger::CLEAR,
                          "__ opponent=%d(%.1f %.1f) step=%d ball is out of pitch. ",
                          opponent->unum(),
                          opponent->pos().x, opponent->pos().y,
                          cycle  );
#endif
            return 1000;
        }

        Vector2D inertia_pos = teammate->inertiaPoint( cycle );
        double target_dist = inertia_pos.dist( ball_pos );

        if ( target_dist - ptype->kickableArea() < 0.001 )
        {
#ifdef DEBUG_PREDICT_OPPONENT_REACH_STEP
            dlog.addText( Logger::CLEAR,
                          "____ opponent=%d(%.1f %.1f) step=%d already there. dist=%.1f",
                          opponent->unum(),
                          opponent->pos().x, opponent->pos().y,
                          cycle,
                          target_dist );
#endif
            tm_dif = 0;
            return cycle;
        }

        double dash_dist = target_dist;
        if ( cycle > 1 )
        {
            dash_dist -= ptype->kickableArea();
            dash_dist += 0.5; // special bonus
        }

        if ( dash_dist > ptype->realSpeedMax() * cycle )
        {
#ifdef DEBUG_PREDICT_OPPONENT_REACH_STEP_LEVEL2
            dlog.addText( Logger::CLEAR,
                          "______ opponent=%d(%.1f %.1f) cycle=%d dash_dist=%.1f reachable=%.1f",
                          opponent->unum(),
                          opponent->pos().x, opponent->pos().y,
                          cycle, dash_dist, ptype->realSpeedMax()*cycle );
#endif
            continue;
        }

        //
        // dash
        //

        int n_dash = ptype->cyclesToReachDistance( dash_dist );

        if ( n_dash > cycle )
        {
            continue;
        }

        //
        // turn
        //
        int n_turn = ( teammate->bodyCount() > 1
                       ? 0
                       : FieldAnalyzer::predict_player_turn_cycle( ptype,
                                                                   teammate->body(),
                                                                   teammate_speed,
                                                                   target_dist,
                                                                   ( ball_pos - inertia_pos ).th(),
                                                                   ptype->kickableArea(),
                                                                   true ) );

        int n_step = ( n_turn == 0
                       ? n_turn + n_dash
                       : n_turn + n_dash + 1 ); // 1 step penalty for observation delay
        if ( teammate->isTackling() )
        {
            n_step += 7; // Magic Number
        }

        n_step += std::min( 2, teammate->posCount() );

        tm_dif = std::min(tm_dif, cycle - n_step);
        if ( n_step <= cycle )
        {
#ifdef DEBUG_PREDICT_OPPONENT_REACH_STEP
            dlog.addText( Logger::CLEAR,
                          "____ opponent=%d(%.1f %.1f) step=%d(t:%d,d:%d) bpos=(%.2f %.2f) dist=%.2f dash_dist=%.2f",
                          opponent->unum(),
                          opponent->pos().x, opponent->pos().y,
                          cycle, n_turn, n_dash,
                          ball_pos.x, ball_pos.y,
                          target_dist,
                          dash_dist );
#endif
            return cycle;
        }

    }

    return 1000;
}
int
TackleGenerator::predictSelfReachStep( const WorldModel & wm,
                                       const Vector2D & first_ball_pos,
                                       const Vector2D & first_ball_vel,
                                       const AngleDeg & ball_move_angle)
{
    const ServerParam & SP = ServerParam::i();
    const Vector2D ball_end_point = inertia_final_point( first_ball_pos,
                                                         first_ball_vel,
                                                         SP.ballDecay() );

    const PlayerType * ptype = wm.self().playerTypePtr();
    const double teammate_speed = wm.self().vel().r();

    int min_cycle = FieldAnalyzer::estimate_min_reach_cycle( wm.self().pos(),
                                                             ptype->realSpeedMax(),
                                                             first_ball_pos,
                                                             ball_move_angle );
    if ( min_cycle < 0 )
    {
        min_cycle = 5;
    }

    for ( int cycle = min_cycle; cycle < 50; ++cycle )
    {
        Vector2D ball_pos = inertia_n_step_point( first_ball_pos,
                                                  first_ball_vel,
                                                  cycle,
                                                  SP.ballDecay() );

        if ( ball_pos.absX() > SP.pitchHalfLength()
             || ball_pos.absY() > SP.pitchHalfWidth() )
        {
#ifdef DEBUG_PREDICT_OPPONENT_REACH_STEP
            dlog.addText( Logger::CLEAR,
                          "__ opponent=%d(%.1f %.1f) step=%d ball is out of pitch. ",
                          opponent->unum(),
                          opponent->pos().x, opponent->pos().y,
                          cycle  );
#endif
            return 1000;
        }

        Vector2D inertia_pos = wm.self().inertiaPoint( cycle );
        double target_dist = inertia_pos.dist( ball_pos );

        if ( target_dist - ptype->kickableArea() < 0.001 )
        {
            return cycle;
        }

        double dash_dist = target_dist;
        if ( cycle > 1 )
        {
            dash_dist -= ptype->kickableArea();
            dash_dist += 0.5; // special bonus
        }

        if ( dash_dist > ptype->realSpeedMax() * cycle )
        {
            continue;
        }

        //
        // dash
        //

        int n_dash = ptype->cyclesToReachDistance( dash_dist );

        if ( n_dash > cycle )
        {
            continue;
        }

        //
        // turn
        //
        int n_turn = ( wm.self().bodyCount() > 1
                       ? 0
                       : FieldAnalyzer::predict_player_turn_cycle( ptype,
                                                                   wm.self().body(),
                                                                   teammate_speed,
                                                                   target_dist,
                                                                   ( ball_pos - inertia_pos ).th(),
                                                                   ptype->kickableArea(),
                                                                   true ) );
        int n_step = ( n_turn == 0
                       ? n_turn + n_dash
                       : n_turn + n_dash + 1 ); // 1 step penalty for observation delay
        n_step += 10; // Magic Number

        if ( n_step <= cycle )
        {
#ifdef DEBUG_PREDICT_OPPONENT_REACH_STEP
            dlog.addText( Logger::CLEAR,
                          "____ opponent=%d(%.1f %.1f) step=%d(t:%d,d:%d) bpos=(%.2f %.2f) dist=%.2f dash_dist=%.2f",
                          opponent->unum(),
                          opponent->pos().x, opponent->pos().y,
                          cycle, n_turn, n_dash,
                          ball_pos.x, ball_pos.y,
                          target_dist,
                          dash_dist );
#endif
            return cycle;
        }

    }

    return 1000;
}
