// -*-c++-*-

/*!
  \file shoot_generator.cpp
  \brief shoot course generator class Source File
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

#include "shoot_generator.h"

#include "field_analyzer.h"
#include "../setting.h"
#include <rcsc/action/kick_table.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/math_util.h>
#include <rcsc/timer.h>

//#define SEARCH_UNTIL_MAX_SPEED_AT_SAME_POINT

//#define DEBUG_PROFILE
// #define DEBUG_PRINT

// #define DEBUG_PRINT_SUCCESS_COURSE
// #define DEBUG_PRINT_FAILED_COURSE

// #define DEBUG_PRINT_EVALUATE

using namespace rcsc;

/*-------------------------------------------------------------------*/
/*!

 */
ShootGenerator::ShootGenerator()
{
    M_courses.reserve( 32 );

    clear();
}

/*-------------------------------------------------------------------*/
/*!

 */
ShootGenerator &
ShootGenerator::instance()
{
    static ShootGenerator s_instance;
    return s_instance;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
ShootGenerator::clear()
{
    M_total_count = 0;
    M_courses.clear();
}

/*-------------------------------------------------------------------*/
/*!

 */
void
ShootGenerator::generate( const WorldModel & wm , double opp_dist_thr)
{
    static GameTime s_update_time( 0, 0 );

    if ( s_update_time == wm.time() )
    {
//        return;
    }
    s_update_time = wm.time();

    clear();

    if ( ! wm.self().isKickable()
         && wm.interceptTable()->selfReachCycle() > 2 )
    {
        return;
    }

    if ( wm.time().stopped() > 0
         || wm.gameMode().type() == GameMode::KickOff_
         // || wm.gameMode().type() == GameMode::KickIn_
         || wm.gameMode().type() == GameMode::IndFreeKick_ )
    {
        return;
    }

    const ServerParam & SP = ServerParam::i();

    if ( wm.self().pos().dist2( SP.theirTeamGoalPos() ) > std::pow( 30.0, 2 ) )
    {
#ifdef DEBUG_PRINT
        dlog.addText( Logger::SHOOT,
                      __FILE__": over shootable distance" );
#endif
        return;
    }

    int self_min = wm.interceptTable()->selfReachCycle();
    M_first_ball_pos = ( wm.self().isKickable()
                         ? wm.ball().pos()
                         : (self_min == 1?wm.ball().pos() + wm.ball().vel():wm.ball().inertiaPoint(2)) );

#ifdef DEBUG_PROFILE
    MSecTimer timer;
#endif

    Vector2D goal_l( SP.pitchHalfLength(), -SP.goalHalfWidth() );
    Vector2D goal_r( SP.pitchHalfLength(), +SP.goalHalfWidth() );

    goal_l.y += std::min( 1.5,
                          0.6 + goal_l.dist( M_first_ball_pos ) * 0.042 );
    goal_r.y -= std::min( 1.5,
                          0.6 + goal_r.dist( M_first_ball_pos ) * 0.042 );

    if ( wm.self().pos().x > SP.pitchHalfLength() - 1.0
         && wm.self().pos().absY() < SP.goalHalfWidth() )
    {
        goal_l.x = wm.self().pos().x + 1.5;
        goal_r.x = wm.self().pos().x + 1.5;
    }

    if(ShootGenerator::shouldShootSafe(wm))
    {
        goal_l.y += 0.3;
        goal_r.y -= 0.3;
    }

    const int DIST_DIVS = 25;
    const double dist_step = std::fabs( goal_l.y - goal_r.y ) / ( DIST_DIVS - 1 );

#ifdef DEBUG_PRINT
    dlog.addText( Logger::SHOOT,
                  __FILE__": ===== Shoot search range=(%.1f %.1f)-(%.1f %.1f) dist_step=%.1f =====",
                  goal_l.x, goal_l.y, goal_r.x, goal_r.y, dist_step );
#endif

    for ( int i = 0; i < DIST_DIVS; ++i )
    {
        ++M_total_count;

        Vector2D target_point = goal_l;
        target_point.y += dist_step * i;

#ifdef DEBUG_PRINT
        dlog.addText( Logger::SHOOT,
                      "%d: ===== shoot target(%.2f %.2f) ===== ",
                      M_total_count,
                      target_point.x, target_point.y );
#endif
        createShoot( wm, target_point,opp_dist_thr );
    }


    evaluateCourses( wm );


#ifdef DEBUG_PROFILE
    dlog.addText( Logger::SHOOT,
                  __FILE__": PROFILE %d/%d. elapsed=%.3f [ms]",
                  (int)M_courses.size(),
                  DIST_DIVS,
                  timer.elapsedReal() );
#endif

}

/*-------------------------------------------------------------------*/
/*!

 */
void
ShootGenerator::createShoot( const WorldModel & wm,
                             const Vector2D & target_point,
                             const double & opp_dist_thr)
{
    const AngleDeg ball_move_angle = ( target_point - M_first_ball_pos ).th();

    const PlayerObject * goalie = wm.getTheirGoalie();
    if ( goalie
         && 5 < goalie->posCount()
         && goalie->posCount() < 30
         && wm.dirCount( ball_move_angle ) > 3 )
    {
#ifdef DEBUG_PRINT
        dlog.addText( Logger::SHOOT,
                      "%d: __ xxx goalie_count=%d, low dir accuracy",
                      M_total_count,
                      goalie->posCount() );
#endif
        return;
    }

    const ServerParam & SP = ServerParam::i();

    const double ball_speed_max = ( wm.gameMode().type() == GameMode::PlayOn
                                    || wm.gameMode().isPenaltyKickMode()
                                    ? SP.ballSpeedMax()
                                    : wm.self().kickRate() * SP.maxPower() );

    const double ball_move_dist = M_first_ball_pos.dist( target_point );

    const Vector2D max_one_step_vel
        = ( wm.self().isKickable()
            ? KickTable::calc_max_velocity( ball_move_angle,
                                            wm.self().kickRate(),
                                            wm.ball().vel() )
            : ( target_point - M_first_ball_pos ).setLengthVector( 0.1 ) );
    const double max_one_step_speed = max_one_step_vel.r();

    double first_ball_speed
        = std::max( ( ball_move_dist + 5.0 ) * ( 1.0 - SP.ballDecay() ),
                    std::max( max_one_step_speed,
                              1.5 ) );

    bool over_max = false;
#ifdef DEBUG_PRINT_FAILED_COURSE
    bool success = false;
#endif
    while ( ! over_max )
    {
        if ( first_ball_speed > ball_speed_max - 0.001 )
        {
            over_max = true;
            first_ball_speed = ball_speed_max;
        }

        if ( createShoot( wm,
                          target_point,
                          first_ball_speed,
                          ball_move_angle,
                          ball_move_dist ,
                          opp_dist_thr) )
        {
            Course & course = M_courses.back();

            if ( first_ball_speed <= max_one_step_speed + 0.001 )
            {
                course.kick_step_ = 1;
            }

#ifdef DEBUG_PRINT_SUCCESS_COURSE
            dlog.addText( Logger::SHOOT,
                          "%d: ok shoot target=(%.2f %.2f)"
                          " speed=%.3f angle=%.1f",
                          M_total_count,
                          target_point.x, target_point.y,
                          first_ball_speed,
                          ball_move_angle.degree() );
            dlog.addRect( Logger::SHOOT,
                          target_point.x - 0.1, target_point.y - 0.1,
                          0.2, 0.2,
                          "#00ff00" );
            char num[8];
            snprintf( num, 8, "%d", M_total_count );
            dlog.addMessage( Logger::SHOOT,
                             target_point, num, "#ffffff" );
#endif
#ifdef DEBUG_PRINT_FAILED_COURSE
            success = true;
#endif
#ifdef SEARCH_UNTIL_MAX_SPEED_AT_SAME_POINT
            if ( course.goalie_never_reach_
                 && course.opponent_never_reach_ )
            {
                return;
            }
            ++M_total_count;
#else
//            return;
#endif
        }

        first_ball_speed += 0.3;
    }

#ifdef DEBUG_PRINT_FAILED_COURSE
    if ( success )
    {
        return;
    }

    dlog.addText( Logger::SHOOT,
                  "%d: xxx shoot target=(%.2f %.2f)"
                  " speed=%.3f angle=%.1f",
                  M_total_count,
                  target_point.x, target_point.y,
                  first_ball_speed,
                  ball_move_angle.degree() );
    dlog.addRect( Logger::SHOOT,
                  target_point.x - 0.1, target_point.y - 0.1,
                  0.2, 0.2,
                  "#ff0000" );
    char num[8];
    snprintf( num, 8, "%d", M_total_count );
    dlog.addMessage( Logger::SHOOT,
                     target_point, num, "#ffffff" );
#endif
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
ShootGenerator::createShoot( const WorldModel & wm,
                             const Vector2D & target_point,
                             const double & first_ball_speed,
                             const rcsc::AngleDeg & ball_move_angle,
                             const double & ball_move_dist ,
                             const double & opp_dist_thr)
{
    const ServerParam & SP = ServerParam::i();

    const int ball_reach_step
        = static_cast< int >( std::ceil( calc_length_geom_series( first_ball_speed,
                                                                   ball_move_dist + 0.1,
                                                                   SP.ballDecay() ) ) );
#ifdef DEBUG_PRINT
    dlog.addText( Logger::SHOOT,
                  "%d: target=(%.2f %.2f) speed=%.3f angle=%.1f"
                  " ball_reach_step=%d",
                  M_total_count,
                  target_point.x, target_point.y,
                  first_ball_speed,
                  ball_move_angle.degree(),
                  ball_reach_step );
#endif

    Course course( M_total_count,
                   target_point,
                   first_ball_speed,
                   ball_move_angle,
                   ball_move_dist,
                   ball_reach_step );

    if ( ball_reach_step <= 1 )
    {
        course.ball_reach_step_ = 1;
        M_courses.push_back( course );
#ifdef DEBUG_PRINT
        dlog.addText( Logger::SHOOT,
                      "%d: one step to the goal" );
#endif
        return true;
    }

    // estimate opponent interception

    const double opponent_x_thr = SP.theirPenaltyAreaLineX() - 30.0;
    const double opponent_y_thr = SP.penaltyAreaHalfWidth();

    int nearest_step_diff = 5;
    for ( PlayerObject::Cont::const_iterator o = wm.opponentsFromSelf().begin(),
            end = wm.opponentsFromSelf().end();
        o != end;
        ++o )
    {
        if ( (*o)->isTackling() ) continue;
        if ( (*o)->pos().x < opponent_x_thr ) continue;
        if ( (*o)->pos().absY() > opponent_y_thr ) continue;

        // behind of shoot course
        if ( ( ball_move_angle - (*o)->angleFromSelf() ).abs() > 90.0 )
        {
            continue;
        }

        //
        // check field player
        //

        if ( (*o)->posCount() > 10 && !(*o)->goalie()) continue;
        if ( (*o)->isGhost() && (*o)->posCount() > 5 && !(*o)->goalie()) continue;

        if ( opponentCanReach( *o, course,wm ,opp_dist_thr,nearest_step_diff) )
        {
#ifdef DEBUG_PRINT
                dlog.addText( Logger::SHOOT,
                              "%d: maybe opponent", M_total_count );
#endif
            return false;
        }
    }
    course.min_dif = nearest_step_diff;
    M_courses.push_back( course );
    return true;

}

/*-------------------------------------------------------------------*/
/*!

 */
bool
ShootGenerator::opponentCanReach( const PlayerObject * opponent,
                                  Course & course,
                                  const WorldModel & wm,
                                  const double & opp_dist_thr,
                                  int & nearest_step_diff)
{
    static const Rect2D penalty_area( Vector2D( ServerParam::i().theirPenaltyAreaLineX(),
                                                -ServerParam::i().penaltyAreaHalfWidth() ),
                                      Size2D( ServerParam::i().penaltyAreaLength(),
                                              ServerParam::i().penaltyAreaWidth() ) );
    const ServerParam & SP = ServerParam::i();

    const PlayerType * ptype = opponent->playerTypePtr();
    double control_area = ptype->kickableArea();

    int min_cycle = FieldAnalyzer::estimate_min_reach_cycle( opponent->pos(),
                                                                   ptype->realSpeedMax(),
                                                                   M_first_ball_pos,
                                                                   course.ball_move_angle_ );

    if ( min_cycle < 0 )
    {
// #ifdef DEBUG_PRINT
//         dlog.addText( Logger::SHOOT,
//                       "%d: (opponent) [%d](%.2f %.2f) never reach",
//                       M_total_count,
//                       opponent->unum(),
//                       opponent->pos().x, opponent->pos().y );
// #endif
        return false;
    }

    min_cycle -= opponent->posCount();
    if(min_cycle < 0)
        min_cycle = 0;
    const double opponent_speed = opponent->vel().r();
    const int max_cycle = course.ball_reach_step_;

    bool maybe_reach = false;
    bool maybe_reach_tackle = false;
#ifdef DEBUG_PRINT
    int nearest_cycle = 1000;
#endif


    Vector2D opp_pos = opponent->pos();
    Vector2D opp_vel = opponent->vel();
    opp_pos += Vector2D::polar2vector(ptype->playerSpeedMax() * (opp_vel.r() / 0.4),opp_vel.th());

    for ( int cycle = min_cycle; cycle < max_cycle; ++cycle )
    {
        Vector2D ball_pos = inertia_n_step_point( M_first_ball_pos,
                                                  course.first_ball_vel_,
                                                  cycle,
                                                  SP.ballDecay() );

        Vector2D inertia_pos = opp_pos;
        double target_dist = inertia_pos.dist( ball_pos );

        if((opponent->goalie() && penalty_area.contains(ball_pos)) || wm.gameMode().type() == GameMode::PenaltyTaken_)
        {
            control_area = ServerParam::i().catchableArea();
            if(ShootGenerator::shouldShootSafe(wm))
            {
                control_area += 0.2;
            }
        }
        else
        {
            control_area = ptype->kickableArea();
        }
        control_area += opp_dist_thr;

        if ( target_dist - control_area < 0.001 )
        {
#ifdef DEBUG_PRINT
            dlog.addText( Logger::SHOOT,
                          "%d: (opponent) [%d] inertiaPos=(%.2f %.2f) can kick without dash",
                          M_total_count,
                          opponent->unum(),
                          inertia_pos.x, inertia_pos.y );
#endif
            return true;
        }

        double dash_dist = target_dist;

        int n_turn;
        int n_dash;
        int n_view;

        int opp_cycle = opponent->cycles_to_cut_ball(wm,
                                                     ball_pos,
                                                     cycle,
                                                     (wm.gameMode().type() == GameMode::PenaltyTaken_? true : false),
                                                     n_dash,
                                                     n_turn,
                                                     n_view,
                                                     opp_pos,
                                                     opp_vel);

        int bonus_step = bound( 0, opponent->posCount(), 1 );
        int penalty_step = 0; //-3;

        if ( opponent->isTackling() )
        {
            penalty_step -= 5;
        }

        if ( opp_cycle - bonus_step - penalty_step <= cycle )
        {
#ifdef DEBUG_PRINT
            dlog.addText( Logger::SHOOT,
                          "%d: xxx (opponent)%d can reach. cycle=%d ball_pos(%.1f %.1f)"
                          " oppStep=%d(t:%d,d%d) bonus=%d",
                          M_total_count,opponent->unum(),
                          cycle,
                          ball_pos.x, ball_pos.y,
                          n_step, n_turn, n_dash, bonus_step );
#endif
            return true;
        }

        int diff = (opp_cycle - bonus_step - penalty_step) - cycle;
        if ( diff < nearest_step_diff )
        {
            nearest_step_diff = diff;
        }

        if ( opp_cycle <= cycle + opponent->posCount() +1 )
        {
            maybe_reach = true;
        }
        if(opponent->goalie())
        {
            int n_turn;
            int n_dash;
            int n_view;
            int opp_cycle = opponent->cycles_to_cut_ball(wm,
                                                         ball_pos,
                                                         cycle,
                                                         true,
                                                         n_dash,
                                                         n_turn,
                                                         n_view,
                                                         opp_pos,
                                                         opp_vel);

            int bonus_step = bound( 0, opponent->posCount(), 2 );
            int penalty_step = 0; //-3;

            if ( opponent->isTackling() )
            {
                penalty_step -= 5;
            }

            if ( opp_cycle <= cycle + bonus_step + penalty_step )
            {
                maybe_reach_tackle = true;
            }
        }
    }

    if ( maybe_reach )
    {
#ifdef DEBUG_PRINT
        dlog.addText( Logger::SHOOT,
                      "%d: (opponent) maybe reach. nearest_step=%d diff=%d",
                      M_total_count,
                      nearest_cycle, nearest_step_diff );
#endif
        course.opponent_never_reach_ = false;
    }
    if( maybe_reach_tackle ){
        course.goalie_never_reach_ = false;
    }


    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
ShootGenerator::evaluateCourses( const WorldModel & wm )
{
    const double y_dist_thr2 = std::pow( 8.0, 2 );

    const ServerParam & SP = ServerParam::i();
    const PlayerObject * goalie = wm.getTheirGoalie();
    const AngleDeg goalie_angle = ( goalie
                                    ? ( goalie->pos() - M_first_ball_pos ).th()
                                    : 180.0 );

    AngleDeg amax = (Vector2D(52.5, +7) - wm.ball().pos()).th() - AngleDeg(3);
    AngleDeg amin = (Vector2D(52.5, -7) - wm.ball().pos()).th() + AngleDeg(3);
    const Container::iterator end = M_courses.end();
    for ( Container::iterator it = M_courses.begin();
          it != end;
          ++it )
    {
        dlog.addText(Logger::SHOOT,"shoot targ(%.1f,%.1f",it->target_point_.x,it->target_point_.y);
        double score = 1.0;

        if ( it->kick_step_ == 1 )
        {
            dlog.addText(Logger::SHOOT,"kickstep=1");
            score += 50.0;
            if(wm.maybeKickableOpponent()){
                score += 1000;
            }
        }else{
            if(wm.maybeKickableOpponent()){
                score = -1000000;
                        it->score_ = score;
                continue;
            }
        }

        if ( it->goalie_never_reach_ )
        {
            dlog.addText(Logger::SHOOT,"never reach tackle");
            score += 100.0;
        }

        if ( it->opponent_never_reach_ )
        {
            dlog.addText(Logger::SHOOT,"never reach");
            score += 100.0;
        }

        double y_rate = 1.0;
        if ( it->target_point_.dist2( M_first_ball_pos ) > y_dist_thr2 )
        {
            double besty = 1000;
            if(goalie){
                if(goalie->pos().dist(Vector2D(52,-6.5)) < goalie->pos().dist(Vector2D(52,6.5))){
                    besty = 6.5;
                }else{
                    besty = -6.5;
                }

            }
            double y_dist = std::max( 0.0, it->target_point_.absY() - 6.0 );
            if(FieldAnalyzer::isHFUT(wm) && wm.ball().pos().y < -5 && wm.ball().pos().x > 40){
                y_dist = std::max( 0.0, it->target_point_.absY() - 3.0 );
            }
//            if(goalie)
                y_dist = std::fabs( it->target_point_.y - besty );
//            y_rate = std::exp( - std::pow( y_dist, 2.0 )
//                               / ( 2.0 * std::pow( SP.goalHalfWidth() - 1.5, 2 ) ) );
                y_rate = 7 - y_dist;
                if(y_rate < 0)
                    y_rate = 0.1;
        }
        score *= y_rate;
        score *= it->min_dif;
        if(wm.gameMode().type() != GameMode::PenaltyTaken_)
            if( !( it->target_point_ - wm.ball().pos()).th().isWithin(amin,amax)){
                score /= 2.0;
            }
        dlog.addText(Logger::SHOOT,"yrate:%.1f,mindif:%.1f,score",y_rate,it->min_dif,score);
        if(wm.opponentsFromBall().size() > 0
                && wm.opponentsFromBall().front()->distFromBall() < 2.5)
            if (it->kick_step_ > 1)
                score *= 0.3;
        it->score_ = score;
#ifdef DEBUG_PRINT_EVALUATE
        dlog.addText( Logger::SHOOT,
                      "(shoot eval) %d: score(%f) pos(%.2f %.2f) speed=%.3f mindif=%f y_rate=%f",
                      it->index_, score,
                      it->target_point_.x, it->target_point_.y,
                      it->first_ball_speed_,
                      it->min_dif,
                      y_rate );
#endif
    }
}

bool ShootGenerator::shouldShootSafe(const WorldModel &wm)
{
    if ( !Setting::i()->mChainAction->mUseShootSafe )
        return false;
    if ( !wm.self().isKickable() )
        return false;
    Vector2D ballPos = wm.ball().pos();
    AngleDeg tir1 = (Vector2D(52.5,7.0) - ballPos).th();
    AngleDeg tir2 = (Vector2D(52.5,-7.0) - ballPos).th();
    bool existOpp = false;
    for(int o = 1; o <= 11; o++)
    {
        const AbstractPlayerObject * opp = wm.theirPlayer(o);
        if(opp == nullptr || opp->unum() != o)
            continue;
        if(opp->goalie())
            continue;
        AngleDeg oppAngle = (opp->pos() - ballPos).th();
        if(oppAngle.isWithin(tir2, tir1))
            existOpp = true;
    }
    if(existOpp)
        return false;
    if(wm.getDistOpponentNearestToBall(10, true) < 3)
        return false;
    dlog.addText(Logger::SHOOT, "Safe Shoot!!");
    return true;
}
