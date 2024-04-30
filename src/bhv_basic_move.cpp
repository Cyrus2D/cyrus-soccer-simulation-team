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

#include "strategy.h"

#include "move_def/bhv_basic_tackle.h"
#include "move_off/bhv_unmark.h"
#include "move_off/bhv_scape.h"
#include "move_def/bhv_block.h"
#include "move_def/bhv_mark_execute.h"
#include "move_def/bhv_tackle_intercept.h"
#include "chain_action/field_analyzer.h"

#include "move_def/cyrus_interceptable.h"
#include "chain_action/bhv_pass_kick_find_receiver.h"
#include "chain_action/action_chain_holder.h"
#include "basic_actions/basic_actions.h"
#include "basic_actions/body_go_to_point.h"
#include "basic_actions/body_intercept2009.h"
#include "basic_actions/neck_turn_to_ball_or_scan.h"
#include "basic_actions/neck_turn_to_low_conf_teammate.h"
#include "chain_action/shoot_generator.h"
#include "chain_action/bhv_strict_check_shoot.h"
#include <thread>
#include <chrono>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/player/intercept_table.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/common/audio_memory.h>

#include <rcsc/player/abstract_player_object.h>
#include <rcsc/player/cut_ball_calculator.h>
#include "neck/neck_offensive_intercept_neck.h"
#include "move_off/bhv_offensive_move.h"
#include "neck/next_pass_predictor.h"
#include "neck/neck_decision.h"
#include "setting.h"

#define DEBUG_PRINT
// #define DEBUG_PRINT_INTERCEPT_LIST

#define USE_GOALIE_MODE

using namespace rcsc;

//bool
//Body_Intercept2022::execute( PlayerAgent * agent )
//{
//    dlog.addText( Logger::TEAM,
//                  __FILE__": Body_Intercept2022" );
//
//    const WorldModel & wm = agent->world();
//
//    /////////////////////////////////////////////
//    if ( doKickableOpponentCheck( agent ) )
//    {
//        return true;;
//    }
//
//    const InterceptTable * table = wm.interceptTable();
//
//    /////////////////////////////////////////////
//    if ( table.selfStep() > 100 )
//    {
//        Vector2D final_point = wm.ball().inertiaFinalPoint();
//        agent->debugClient().setTarget( final_point );
//
//        dlog.addText( Logger::INTERCEPT,
//                      __FILE__": no solution... Just go to ball end point (%.2f %.2f)",
//                      final_point.x, final_point.y );
//        agent->debugClient().addMessage( "InterceptNoSolution" );
//        Body_GoToPoint( final_point,
//                        2.0,
//                        ServerParam::i().maxDashPower()
//                        ).execute( agent );
//        return true;
//    }
//
//    /////////////////////////////////////////////
//    int ignore_intercept = 0;
//    InterceptInfo best_intercept = getBestIntercept( wm, table,ignore_intercept );
//    if(ignore_intercept == table->selfCache().size())
//        return false;
//    //InterceptInfo best_intercept_test = getBestIntercept( wm, table );
//
//    dlog.addText( Logger::INTERCEPT,
//                  __FILE__": solution size= %d. selected best cycle is %d"
//                  " (turn:%d + dash:%d) power=%.1f dir=%.1f",
//                  table->selfCache().size(),
//                  best_intercept.reachCycle(),
//                  best_intercept.turnCycle(), best_intercept.dashCycle(),
//                  best_intercept.dashPower(),
//                  best_intercept.dashDir());
//
//    Vector2D target_point = wm.ball().inertiaPoint( best_intercept.reachCycle() );
//    agent->debugClient().setTarget( target_point );
//
//    if ( best_intercept.dashCycle() == 0 )
//    {
//        dlog.addText( Logger::INTERCEPT,
//                      __FILE__": can get the ball only by inertia move. Turn!" );
//        const bool goalie_mode
//                = ( wm.self().goalie()
//                    && wm.lastKickerSide() != wm.ourSide()
//                    && best_intercept.selfPos().x < ServerParam::i().ourPenaltyAreaLineX()
//                    && best_intercept.selfPos().absY() < ServerParam::i().penaltyAreaHalfWidth()
//                    && ((wm.ball().inertiaPoint(1) - wm.self().inertiaPoint(best_intercept.turnCycle())).th() - wm.self().body()).abs() < 90.0 + wm.self().playerType().effectiveTurn( ServerParam::i().maxMoment(), wm.self().vel().r() )
//                );
//        Vector2D face_point = M_face_point;
//        if ( ! face_point.isValid() )
//        {
//            face_point.assign( 50.5, wm.self().pos().y * 0.75 );
//        }
//        if (goalie_mode){
//            face_point = wm.ball().inertiaPoint(1);
//        }
//        agent->debugClient().addMessage( "InterceptTurnOnly" );
//        Body_TurnToPoint( face_point,
//                          best_intercept.reachCycle() ).execute( agent );
//        return true;
//    }
//
//    /////////////////////////////////////////////
//    if ( best_intercept.turnCycle() > 0 )
//    {
//        Vector2D my_inertia = wm.self().inertiaPoint( best_intercept.reachCycle() );
//        AngleDeg target_angle = ( target_point - my_inertia ).th();
//        if ( std::abs(best_intercept.dashDir()) > 179 )
//        {
//            // back dash
//            target_angle -= 180.0;
//        }
//
//        dlog.addText( Logger::INTERCEPT,
//                      __FILE__": turn.first.%s target_body_angle = %.1f",
//                      ( best_intercept.dashPower() < 0.0 ? "BackMode" : "" ),
//                      target_angle.degree() );
//        agent->debugClient().addMessage( "InterceptTurn%d(%d/%d)",
//                                         best_intercept.reachCycle(),
//                                         best_intercept.turnCycle(),
//                                         best_intercept.dashCycle() );
//
//        return agent->doTurn( target_angle - wm.self().body() );
//    }
//
//    /////////////////////////////////////////////
//    dlog.addText( Logger::INTERCEPT,
//                  __FILE__": try dash. power=%.1f  target_point=(%.2f, %.2f)",
//                  best_intercept.dashPower(),
//                  target_point.x, target_point.y );
//
//    if ( doWaitTurn( agent, target_point, best_intercept ) )
//    {
//        return true;
//    }
//
//    if ( M_save_recovery
//         && ! wm.self().staminaModel().capacityIsEmpty() )
//    {
//        double consumed_stamina = best_intercept.dashPower();
//        if ( best_intercept.dashPower() < 0.0 ) consumed_stamina *= -2.0;
//
//        if ( wm.self().stamina() - consumed_stamina
//             < ServerParam::i().recoverDecThrValue() + 1.0 )
//        {
//            dlog.addText( Logger::INTERCEPT,
//                          __FILE__": insufficient stamina" );
//            agent->debugClient().addMessage( "InterceptRecover" );
//            agent->doTurn( 0.0 );
//            return false;
//        }
//
//    }
//
//    return doInertiaDash( agent,
//                          target_point,
//                          best_intercept );
//}
//
//bool
//Body_Intercept2022::executeTackle( PlayerAgent * agent )
//{
//    dlog.addText( Logger::TEAM,
//                  __FILE__": Body_Intercept2022 tackle" );
//
//    const WorldModel & wm = agent->world();
//
//    /////////////////////////////////////////////
//    if ( doKickableOpponentCheck( agent ) )
//    {
//        return true;;
//    }
//
//    const InterceptTable * table = wm.interceptTable();
//
//    /////////////////////////////////////////////
//     if ( table->selfReachCycleTackle() > 100 )
//     {
//         Vector2D final_point = wm.ball().inertiaFinalPoint();
//         agent->debugClient().setTarget( final_point );
//
//         dlog.addText( Logger::INTERCEPT,
//                       __FILE__": no solution... Just go to ball end point (%.2f %.2f)",
//                       final_point.x, final_point.y );
//         agent->debugClient().addMessage( "InterceptNoSolution" );
//         Body_GoToPoint( final_point,
//                         2.0,
//                         ServerParam::i().maxDashPower()
//                         ).execute( agent );
//         return true;
//     }
//
//    /////////////////////////////////////////////
//    int ignore_intercept = 0;
//    InterceptInfo best_intercept = getBestInterceptTackle( wm, table,ignore_intercept );
//    if(ignore_intercept == table->selfCache().size())
//        return false;
//    //InterceptInfo best_intercept_test = getBestIntercept( wm, table );
//
//    dlog.addText( Logger::INTERCEPT,
//                  __FILE__": solution size= %d. selected best cycle is %d"
//                  " (turn:%d + dash:%d) power=%.1f dir=%.1f",
//                   table->selfCacheTackle().size(),
//                  best_intercept.reachCycle(),
//                  best_intercept.turnCycle(), best_intercept.dashCycle(),
//                  best_intercept.dashPower(),
//                  std::abs(best_intercept.dashDir()));
//
//    Vector2D target_point = wm.ball().inertiaPoint( best_intercept.reachCycle() );
//    agent->debugClient().setTarget( target_point );
//
//    if ( best_intercept.dashCycle() == 0 )
//    {
//        dlog.addText( Logger::INTERCEPT,
//                      __FILE__": can get the ball only by inertia move. Turn!" );
//
//        Vector2D face_point = M_face_point;
//        if ( ! face_point.isValid() )
//        {
//            face_point.assign( 50.5, wm.self().pos().y * 0.75 );
//        }
//
//        agent->debugClient().addMessage( "InterceptTurnOnly" );
//        Body_TurnToPoint( wm.ball().inertiaPoint(best_intercept.reachCycle()),
//                          best_intercept.reachCycle() ).execute( agent );
//        return true;
//    }
//
//    /////////////////////////////////////////////
//    if ( best_intercept.turnCycle() > 0 )
//    {
//        Vector2D my_inertia = wm.self().inertiaPoint( best_intercept.reachCycle() );
//        AngleDeg target_angle = ( target_point - my_inertia ).th();
//        if ( best_intercept.dashPower() < 0.0 )
//        {
//            // back dash
//            target_angle -= 180.0;
//        }
//
//        dlog.addText( Logger::INTERCEPT,
//                      __FILE__": turn.first.%s target_body_angle = %.1f",
//                      ( best_intercept.dashPower() < 0.0 ? "BackMode" : "" ),
//                      target_angle.degree() );
//        agent->debugClient().addMessage( "InterceptTurn%d(%d/%d)",
//                                         best_intercept.reachCycle(),
//                                         best_intercept.turnCycle(),
//                                         best_intercept.dashCycle() );
//
//        return agent->doTurn( target_angle - wm.self().body() );
//    }
//
//    /////////////////////////////////////////////
//    dlog.addText( Logger::INTERCEPT,
//                  __FILE__": try dash. power=%.1f  target_point=(%.2f, %.2f)",
//                  best_intercept.dashPower(),
//                  target_point.x, target_point.y );
//
//    if ( doWaitTurn( agent, target_point, best_intercept ) )
//    {
//        return true;
//    }
//
//    if ( M_save_recovery
//         && ! wm.self().staminaModel().capacityIsEmpty() )
//    {
//        double consumed_stamina = best_intercept.dashPower();
//        if ( best_intercept.dashPower() < 0.0 ) consumed_stamina *= -2.0;
//
//        if ( wm.self().stamina() - consumed_stamina
//             < ServerParam::i().recoverDecThrValue() + 1.0 )
//        {
//            dlog.addText( Logger::INTERCEPT,
//                          __FILE__": insufficient stamina" );
//            agent->debugClient().addMessage( "InterceptRecover" );
//            agent->doTurn( 0.0 );
//            return false;
//        }
//
//    }
//
//    return doInertiaDash( agent,
//                          target_point,
//                          best_intercept );
//}
///*-------------------------------------------------------------------*/
///*!
//
//*/
//bool
//Body_Intercept2022::doKickableOpponentCheck( PlayerAgent * agent )
//{
//    const WorldModel & wm = agent->world();
//    if ( wm.ball().distFromSelf() < 2.0
//         && wm.kickableOpponent() )
//    {
//        const PlayerObject * opp = wm.opponentsFromBall().front();
//        if ( opp )
//        {
//            Vector2D goal_pos( -ServerParam::i().pitchHalfLength(), 0.0 );
//            Vector2D my_next = wm.self().pos() + wm.self().vel();
//            Vector2D attack_pos = opp->pos() + opp->vel();
//
//            if ( attack_pos.dist2( goal_pos ) > my_next.dist2( goal_pos ) )
//            {
//                dlog.addText( Logger::INTERCEPT,
//                              __FILE__": attack to opponent" );
//
//                Body_GoToPoint( attack_pos,
//                                0.1,
//                                ServerParam::i().maxDashPower(),
//                                -1.0, // dash speed
//                                1, // cycle
//                                true, // save recovery
//                                15.0  // dir thr
//                                ).execute( agent );
//                return true;
//            }
//        }
//    }
//    return false;
//}
//
///*-------------------------------------------------------------------*/
///*!
//
//*/
//InterceptInfo
//Body_Intercept2022::getBestIntercept( const WorldModel & wm,
//                                      const InterceptTable * table,
//                                      int &ignore_intercept) const
//{
//    const ServerParam & SP = ServerParam::i();
//    const std::vector< InterceptInfo > & cache = table->selfCache();
//
//    if ( cache.empty() )
//    {
//        return InterceptInfo();
//    }
//
//#ifdef DEBUG_PRINT
//    dlog.addText( Logger::INTERCEPT,
//                  "==================== getBestIntercept ====================");
//#endif
//
//    const Vector2D goal_pos( 65.0, 0.0 );
//    const Vector2D our_goal_pos( -SP.pitchHalfLength(), 0.0 );
//    const double max_pitch_x = ( SP.keepawayMode()
//                                 ? SP.keepawayLength() * 0.5 - 1.0
//                                 : SP.pitchHalfLength() - 1.0 );
//    const double max_pitch_y = ( SP.keepawayMode()
//                                 ? SP.keepawayWidth() * 0.5 - 1.0
//                                 : SP.pitchHalfWidth() - 1.0 );
//    const double penalty_x = SP.ourPenaltyAreaLineX();
//    const double penalty_y = SP.penaltyAreaHalfWidth();
//    const double speed_max = wm.self().playerType().realSpeedMax() * 0.9;
//    const int opp_min = table.opponentStep();
//    const int mate_min = table.teammateStep();
//    //const PlayerObject * fastest_opponent = table.firstOpponent();
//
//    const InterceptInfo * attacker_best = static_cast< InterceptInfo * >( 0 );
//    double attacker_score = 0.0;
//
//    const InterceptInfo * forward_best = static_cast< InterceptInfo * >( 0 );
//    double forward_score = 0.0;
//
//    const InterceptInfo * noturn_best = static_cast< InterceptInfo * >( 0 );
//    double noturn_score = 0.0;
//
//    const InterceptInfo * nearest_best = static_cast< InterceptInfo * >( 0 );
//    double nearest_score = 10000.0;
//
//#ifdef USE_GOALIE_MODE
//    const InterceptInfo * goalie_best = static_cast< InterceptInfo * >( 0 );
//    double goalie_score = -10000.0;
//
//    const InterceptInfo * goalie_aggressive_best = static_cast< InterceptInfo * >( 0 );
//    double goalie_aggressive_score = -10000.0;
//#endif
//
//    const std::size_t MAX = cache.size();
//    for ( std::size_t i = 0; i < MAX; ++i )
//    {
//        if ( M_origin_target.isValid() && wm.ball().inertiaPoint(cache[i].reachCycle()).dist(M_origin_target) > M_origin_dist){
//            ignore_intercept += 1;
//            continue;
//        }
//        if ( M_save_recovery )
//             && cache[i].mode() != InterceptInfo::NORMAL )
//        {
//            continue;
//        }
//
//        const int cycle = cache[i].reachCycle();
//        const Vector2D self_pos = wm.self().inertiaPoint( cycle );
//        const Vector2D ball_pos = wm.ball().inertiaPoint( cycle );
//        const Vector2D ball_vel = wm.ball().vel() * std::pow( SP.ballDecay(), cycle );
//
//#ifdef DEBUG_PRINT_INTERCEPT_LIST
//        dlog.addText( Logger::INTERCEPT,
//                      "intercept %d: cycle=%d t=%d d=%d pos=(%.2f %.2f) vel=(%.2f %.1f) trap_ball_dist=%f",
//                      i,  cycle, cache[i].turnCycle(), cache[i].dashCycle(),
//                      ball_pos.x, ball_pos.y,
//                      ball_vel.x, ball_vel.y,
//                      cache[i].ballDist() );
//#endif
//
//        if ( ball_pos.absX() > max_pitch_x
//             || ball_pos.absY() > max_pitch_y )
//        {
//            continue;
//        }
//
//#ifdef USE_GOALIE_MODE
//        if ( wm.self().goalie()
//             && wm.lastKickerSide() != wm.ourSide()
//             && ball_pos.x < penalty_x - 1.0
//             && ball_pos.absY() < penalty_y - 1.0
//             && cycle < opp_min - 1 )
//        {
//            bool can_catch = false;
//            // if (cache[i].dashAngle().abs() < 1.0) CYRUS_LIB
//            //     can_catch = true;
//            // else if (cache[i].dashAngle().abs() < 179.0) CYRUS_LIB
//            if (((ball_pos - cache[i].selfPos()).th() - wm.self().body()).abs() < 90.0)
//                can_catch = true;
//            if ( can_catch
//                 || cache[i].ballDist() < 0.01 )
//            {
//                double d = ball_pos.dist2( our_goal_pos );
//                if (cache[i].turnCycle() == 0)
//                    d = d * 2.0 + 5.0;
////                AngleDeg target_angle = ( ball_pos - self_pos ).th();
////                if ( cache[i].dashPower() < 0.0 )
////                {
////                    // back dash
////                    target_angle -= 180.0;
////                }
////                if((target_angle - (wm.ball().pos() - wm.self().pos()).th()).abs() > 120)
////                    d /= 2;
//
//                if ( d> goalie_score )
//                {
//                    goalie_score = d;
//                    goalie_best = &cache[i];
//#ifdef DEBUG_PRINT
//                    dlog.addText( Logger::INTERCEPT,
//                                  "___ %d updated goalie_best score=%f  trap_ball_dist=%f",
//                                  i, goalie_score, cache[i].ballDist() );
//#endif
//                }
//            }
//        }
//
//        if ( wm.self().goalie()
//             && wm.lastKickerSide() != wm.ourSide()
//             && cycle < mate_min - 3
//             && cycle < opp_min - 5
//             && ( ball_pos.x > penalty_x - 1.0
//                  || ball_pos.absY() > penalty_y - 1.0 ) )
//        {
//            if ( ( cache[i].turnCycle() == 0
//                   && cache[i].ballDist() < wm.self().playerType().kickableArea() * 0.5 )
//                 || cache[i].ballDist() < 0.01 )
//            {
//                if ( ball_pos.x > goalie_aggressive_score )
//                {
//                    goalie_aggressive_score = ball_pos.x;
//                    goalie_aggressive_best = &cache[i];
//#ifdef DEBUG_PRINT
//                    dlog.addText( Logger::INTERCEPT,
//                                  "___ %d updated goalie_aggressive_best score=%f  trap_ball_dist=%f",
//                                  i, goalie_aggressive_score, cache[i].ballDist() );
//#endif
//                }
//            }
//        }
//#endif
//
//        bool attacker = false;
//        if ( ball_vel.x > 0.5
//             && ball_vel.r2() > std::pow( speed_max, 2 )
//             && cache[i].dashPower() >= 0.0
//             && ball_pos.x < 47.0
//             //&& std::fabs( ball_pos.y - wm.self().pos().y ) < 10.0
//             && ( ball_pos.x > 35.0
//                  || ball_pos.x > wm.offsideLineX() - 2.)
//             )
//        {
//#ifdef DEBUG_PRINT
//            dlog.addText( Logger::INTERCEPT,
//                          "___ %d attacker", i );
//#endif
//            attacker = true;
//        }
//
//        const double opp_rate = ( attacker ? 0.95 : 0.7 );
//#if 0
//        if ( attacker
//             && opp_min <= cycle - 5
//             && ball_vel.r2() > std::pow( 1.2, 2 ) )
//        {
//#ifdef DEBUG_PRINT
//            dlog.addText( Logger::INTERCEPT,
//                          "___ %d attacker ignores opponent cycle=%d: ball_vel=(%.1f %.1f)",
//                          i,
//                          opp_min,
//                          ball_vel.x, ball_vel.y );
//#endif
//        }
//        else
//#endif
//            if ( cycle >= opp_min * opp_rate )
//        {
//#ifdef DEBUG_PRINT
//            dlog.addText( Logger::INTERCEPT,
//                          "___ %d failed: cycle=%d pos=(%.1f %.1f) turn=%d dash=%d  opp_min=%d rated=%.2f",
//                          i, cycle,
//                          ball_pos.x, ball_pos.y,
//                          cache[i].turnCycle(), cache[i].dashCycle(),
//                          opp_min, opp_min * opp_rate );
//#endif
//            continue;
//        }
//
//        // attacker type
//
//        if ( attacker )
//        {
//            double goal_dist = 100.0 - std::min( 100.0, ball_pos.dist( goal_pos ) );
//            double x_diff = 47.0 - ball_pos.x;
//
//            double score
//                = ( goal_dist / 100.0 )
//                * std::exp( - ( x_diff * x_diff ) / ( 2.0 * 100.0 ) );
//#ifdef DEBUG_PRINT
//            dlog.addText( Logger::INTERCEPT,
//                          "___ %d attacker cycle=%d pos=(%.1f %.1f) turn=%d dash=%d score=%f",
//                          i, cycle,
//                          ball_pos.x, ball_pos.y,
//                          cache[i].turnCycle(), cache[i].dashCycle(),
//                          score );
//#endif
//            if ( score > attacker_score )
//            {
//                attacker_best = &cache[i];
//                attacker_score = score;
//#ifdef DEBUG_PRINT
//                dlog.addText( Logger::INTERCEPT,
//                              "___ %d updated attacker_best score=%f",
//                              i, score );
//#endif
//            }
//
//            continue;
//        }
//
//        // no turn type
//
//        if ( cache[i].turnCycle() == 0 )
//        {
//            int diff = std::min(0, 4 - (opp_min - cache[i].dashCycle()));
//            auto opp = table.firstOpponent();
//            if (opp){
//                diff += opp->posCount();
//            }
//            //double score = ball_pos.x;
//            //double score = wm.self().pos().dist2( ball_pos );
//            double score = cycle;
//            score -= diff;
//            //if ( ball_vel.x > 0.0 )
//            //{
//            //    score *= std::exp( - std::pow( ball_vel.r() - 1.0, 2.0 )
//            //                       / ( 2.0 * 1.0 ) );
//            //}
//#ifdef DEBUG_PRINT
//            dlog.addText( Logger::INTERCEPT,
//                          "___ %d noturn cycle=%d pos=(%.1f %.1f) turn=%d dash=%d score=%f",
//                          i, cycle,
//                          ball_pos.x, ball_pos.y,
//                          cache[i].turnCycle(), cache[i].dashCycle(),
//                          score );
//#endif
//            if ( score > noturn_score )
//            {
//                noturn_best = &cache[i];
//                noturn_score = score;
//#ifdef DEBUG_PRINT
//                dlog.addText( Logger::INTERCEPT,
//                              "___ %d updated noturn_best score=%f",
//                              i, score );
//#endif
//            }
//
//            continue;
//        }
//
//        // forward type
//
////         if ( ball_vel.x > 0.5
////              && ball_pos.x > wm.offsideLineX() - 15.0
////              && ball_vel.r() > speed_max * 0.98
////              && cycle <= opp_min - 5 )
//        if ( ball_vel.x > 0.1
//             && cycle <= opp_min - 5
//             && ball_vel.r2() > std::pow( 0.6, 2 ) )
//        {
//            double score
//                = ( 100.0 * 100.0 )
//                - std::min( 100.0 * 100.0, ball_pos.dist2( goal_pos ) );
//#ifdef DEBUG_PRINT
//            dlog.addText( Logger::INTERCEPT,
//                          "___ %d forward cycle=%d pos=(%.1f %.1f) turn=%d dash=%d score=%f",
//                          i, cycle,
//                          ball_pos.x, ball_pos.y,
//                          cache[i].turnCycle(), cache[i].dashCycle(),
//                          score );
//#endif
//            if ( score > forward_score )
//            {
//                forward_best = &cache[i];
//                forward_score = score;
//#ifdef DEBUG_PRINT
//                dlog.addText( Logger::INTERCEPT,
//                              "___ %d updated forward_best score=%f",
//                              i, score );
//#endif
//            }
//
//            continue;
//        }
//
//        // other: select nearest one
//
//        {
//            //double d = wm.self().pos().dist2( ball_pos );
//            double d = self_pos.dist2( ball_pos );
//#ifdef DEBUG_PRINT
//            dlog.addText( Logger::INTERCEPT,
//                          "___ %d other cycle=%d pos=(%.1f %.1f) turn=%d dash=%d dist2=%.2f",
//                          i, cycle,
//                          ball_pos.x, ball_pos.y,
//                          cache[i].turnCycle(), cache[i].dashCycle(),
//                          d );
//#endif
//            if ( d < nearest_score )
//            {
//                nearest_best = &cache[i];
//                nearest_score = d;
//#ifdef DEBUG_PRINT
//                dlog.addText( Logger::INTERCEPT,
//                              "___ %d updated nearest_best score=%f",
//                              i, nearest_score );
//#endif
//            }
//        }
//
//    }
//
//#ifdef USE_GOALIE_MODE
//    if ( goalie_aggressive_best )
//    {
//        dlog.addText( Logger::INTERCEPT,
//                      "<--- goalie aggressive_best: cycle=%d(t=%d,d=%d) ball_dist=%.3f score=%f",
//                      goalie_aggressive_best->reachCycle(),
//                      goalie_aggressive_best->turnCycle(), goalie_aggressive_best->dashCycle(),
//                      goalie_aggressive_best->ballDist(),
//                      goalie_aggressive_score );
//        return *goalie_aggressive_best;
//    }
//
//    if ( goalie_best )
//    {
//        dlog.addText( Logger::INTERCEPT,
//                      "<--- goalie best: cycle=%d(t=%d,d=%d) ball_dist=%.3f score=%f",
//                      goalie_best->reachCycle(),
//                      goalie_best->turnCycle(), goalie_best->dashCycle(),
//                      goalie_best->ballDist(),
//                      goalie_score );
//        return *goalie_best;
//    }
//#endif
//    if ( attacker_best && noturn_best)
//    {
//        if(wm.ball().inertiaPoint((*attacker_best).reachCycle()).dist(wm.ball().inertiaPoint((*noturn_best).reachCycle())) < 4
//            && wm.offsideLineX() - wm.ball().inertiaPoint((*attacker_best).reachCycle()).x > 5.
//            && std::fabs(wm.ball().vel().th().degree()) > 60.){
//            dlog.addText( Logger::INTERCEPT,
//                              "<--- noturn best(0): cycle=%d(t=%d,d=%d) score=%f",
//                          noturn_best->reachCycle(),
//                          noturn_best->turnCycle(), noturn_best->dashCycle(),
//                          noturn_score );
//            return *noturn_best;
//        }
//        dlog.addText( Logger::INTERCEPT,
//                      "<--- attacker best: cycle=%d(t=%d,d=%d) score=%f",
//                      attacker_best->reachCycle(),
//                      attacker_best->turnCycle(), attacker_best->dashCycle(),
//                      attacker_score );
//
//        return *attacker_best;
//    }
//
//    if ( attacker_best )
//    {
//        dlog.addText( Logger::INTERCEPT,
//                      "<--- attacker best: cycle=%d(t=%d,d=%d) score=%f",
//                      attacker_best->reachCycle(),
//                      attacker_best->turnCycle(), attacker_best->dashCycle(),
//                      attacker_score );
//
//        return *attacker_best;
//    }
//
//    if ( noturn_best && forward_best )
//    {
//        //const Vector2D forward_ball_pos = wm.ball().inertiaPoint( forward_best->reachCycle() );
//        //const Vector2D forward_ball_vel
//        //    = wm.ball().vel()
//        //    * std::pow( SP.ballDecay(), forward_best->reachCycle() );
//
//        if ( forward_best->reachCycle() >= 5 )
//        {
//            dlog.addText( Logger::INTERCEPT,
//                          "<--- forward best(1): cycle=%d(t=%d,d=%d) score=%f",
//                          forward_best->reachCycle(),
//                          forward_best->turnCycle(), forward_best->dashCycle(),
//                          forward_score );
//        }
//
//        const Vector2D noturn_ball_vel
//            = wm.ball().vel()
//            * std::pow( SP.ballDecay(), noturn_best->reachCycle() );
//        const double noturn_ball_speed = noturn_ball_vel.r();
//        if ( noturn_ball_vel.x > 0.1
//             && ( noturn_ball_speed > speed_max
//                  || noturn_best->reachCycle() <= forward_best->reachCycle() + 2 )
//             )
//        {
//            dlog.addText( Logger::INTERCEPT,
//                              "<--- noturn best(1): cycle=%d(t=%d,d=%d) score=%f",
//                          noturn_best->reachCycle(),
//                          noturn_best->turnCycle(), noturn_best->dashCycle(),
//                          noturn_score );
//            return *noturn_best;
//        }
//    }
//
//    if ( forward_best )
//    {
//        dlog.addText( Logger::INTERCEPT,
//                      "<--- forward best(2): cycle=%d(t=%d,d=%d) score=%f",
//                      forward_best->reachCycle(),
//                      forward_best->turnCycle(), forward_best->dashCycle(),
//                      forward_score );
//
//        return *forward_best;
//    }
//
//    const Vector2D fastest_pos = wm.ball().inertiaPoint( cache[0].reachCycle() );
//    const Vector2D fastest_vel = wm.ball().vel() * std::pow( SP.ballDecay(),
//                                                             cache[0].reachCycle() );
//    if ( ( fastest_pos.x > -33.0
//           || fastest_pos.absY() > 20.0 )
//         && ( cache[0].reachCycle() >= 10
//             //|| wm.ball().vel().r() < 1.5 ) )
//             || fastest_vel.r() < 1.2 ) )
//    {
//        dlog.addText( Logger::INTERCEPT,
//                      "<--- fastest best: cycle=%d(t=%d,d=%d)",
//                      cache[0].reachCycle(),
//                      cache[0].turnCycle(), cache[0].dashCycle() );
//        return cache[0];
//    }
//
//    if ( noturn_best && nearest_best )
//    {
//        const Vector2D noturn_self_pos = wm.self().inertiaPoint( noturn_best->reachCycle() );
//        const Vector2D noturn_ball_pos = wm.ball().inertiaPoint( noturn_best->reachCycle() );
//        const Vector2D nearest_self_pos = wm.self().inertiaPoint( nearest_best->reachCycle() );
//        const Vector2D nearest_ball_pos = wm.ball().inertiaPoint( nearest_best->reachCycle() );
//
////         if ( wm.self().pos().dist2( noturn_ball_pos )
////              < wm.self().pos().dist2( nearest_ball_pos ) )
//        if ( noturn_self_pos.dist2( noturn_ball_pos )
//             < nearest_self_pos.dist2( nearest_ball_pos ) )
//        {
//            dlog.addText( Logger::INTERCEPT,
//                          "<--- noturn best(2): cycle=%d(t=%d,d=%d) score=%f",
//                          noturn_best->reachCycle(),
//                          noturn_best->turnCycle(), noturn_best->dashCycle(),
//                          noturn_score );
//
//            return *noturn_best;
//        }
//
//        if ( nearest_best->reachCycle() <= noturn_best->reachCycle() + 2 )
//        {
//            const Vector2D nearest_ball_vel
//                = wm.ball().vel()
//                * std::pow( SP.ballDecay(), nearest_best->reachCycle() );
//            const double nearest_ball_speed = nearest_ball_vel.r();
//            if ( nearest_ball_speed < 0.7 )
//            {
//                dlog.addText( Logger::INTERCEPT,
//                              "<--- nearest best(2): cycle=%d(t=%d,d=%d) score=%f",
//                              nearest_best->reachCycle(),
//                              nearest_best->turnCycle(), nearest_best->dashCycle(),
//                              nearest_score );
//                return *nearest_best;
//            }
//
//            const Vector2D noturn_ball_vel
//                = wm.ball().vel()
//                * std::pow( SP.ballDecay(), noturn_best->reachCycle() );
//
//            if ( nearest_best->ballDist() < wm.self().playerType().kickableArea() - 0.4
//                 && nearest_best->ballDist() < noturn_best->ballDist()
//                 && noturn_ball_vel.x < 0.5
//                 && noturn_ball_vel.r2() > std::pow( 1.0, 2 )
//                 && noturn_ball_pos.x > nearest_ball_pos.x )
//            {
//                dlog.addText( Logger::INTERCEPT,
//                              "<--- nearest best(3): cycle=%d(t=%d,d=%d) score=%f",
//                              nearest_best->reachCycle(),
//                              nearest_best->turnCycle(), nearest_best->dashCycle(),
//                              nearest_score );
//                return *nearest_best;
//            }
//
//            Vector2D nearest_self_pos = wm.self().inertiaPoint( nearest_best->reachCycle() );
//            if ( nearest_ball_speed > 0.7
//                //&& wm.self().pos().dist( nearest_ball_pos ) < wm.self().playerType().kickableArea() )
//                 && nearest_self_pos.dist( nearest_ball_pos ) < wm.self().playerType().kickableArea() )
//            {
//                dlog.addText( Logger::INTERCEPT,
//                              "<--- nearest best(4): cycle=%d(t=%d,d=%d) score=%f",
//                              nearest_best->reachCycle(),
//                              nearest_best->turnCycle(), nearest_best->dashCycle(),
//                              nearest_score );
//                return *nearest_best;
//            }
//        }
//
//        dlog.addText( Logger::INTERCEPT,
//                          "<--- noturn best(3): cycle=%d(t=%d,d=%d) score=%f",
//                      noturn_best->reachCycle(),
//                      noturn_best->turnCycle(), noturn_best->dashCycle(),
//                      noturn_score );
//
//        return *noturn_best;
//    }
//
//    if ( noturn_best )
//    {
//        dlog.addText( Logger::INTERCEPT,
//                      "<--- noturn best only: cycle=%d(t=%d,d=%d) score=%f",
//                      noturn_best->reachCycle(),
//                      noturn_best->turnCycle(), noturn_best->dashCycle(),
//                      noturn_score );
//
//        return *noturn_best;
//    }
//
//    if ( nearest_best )
//    {
//        dlog.addText( Logger::INTERCEPT,
//                      "<--- nearest best only: cycle=%d(t=%d,d=%d) score=%f",
//                      nearest_best->reachCycle(),
//                      nearest_best->turnCycle(), nearest_best->dashCycle(),
//                      nearest_score );
//
//        return *nearest_best;
//    }
//
//
//
//    if ( wm.self().pos().x > 40.0
//         && wm.ball().vel().r() > 1.8
//         && wm.ball().vel().th().abs() < 100.0
//         && cache[0].reachCycle() > 1 )
//    {
//        const InterceptInfo * chance_best = static_cast< InterceptInfo * >( 0 );
//        for ( std::size_t i = 0; i < MAX; ++i )
//        {
//            if ( cache[i].reachCycle() <= cache[0].reachCycle() + 3
//                 && cache[i].reachCycle() <= opp_min - 2 )
//            {
//                chance_best = &cache[i];
//            }
//        }
//
//        if ( chance_best )
//        {
//            dlog.addText( Logger::INTERCEPT,
//                          "<--- chance best only: cycle=%d(t=%d,d=%d)",
//                          chance_best->reachCycle(),
//                          chance_best->turnCycle(), chance_best->dashCycle() );
//            return *chance_best;
//        }
//    }
//
//    return cache[0];
//
//}
//
//InterceptInfo
//Body_Intercept2022::getBestInterceptTackle( const WorldModel & wm,
//                                      const InterceptTable * table,
//                                      int &ignore_intercept) const
//{
//    const ServerParam & SP = ServerParam::i();
//    const std::vector< InterceptInfo > & cache = table-> selfCache();// selfCacheTackle(); // CYRUS_LIB
//
//    if ( cache.empty() )
//    {
//        return InterceptInfo();
//    }
//
//#ifdef DEBUG_PRINT
//    dlog.addText( Logger::INTERCEPT,
//                  "===== getBestIntercept =====");
//#endif
//
//    const Vector2D goal_pos( 65.0, 0.0 );
//    const Vector2D our_goal_pos( -SP.pitchHalfLength(), 0.0 );
//    const double max_pitch_x = ( SP.keepawayMode()
//                                 ? SP.keepawayLength() * 0.5 - 1.0
//                                 : SP.pitchHalfLength() );
//    const double max_pitch_y = ( SP.keepawayMode()
//                                 ? SP.keepawayWidth() * 0.5 - 1.0
//                                 : SP.pitchHalfWidth() );
//    const double penalty_x = SP.ourPenaltyAreaLineX();
//    const double penalty_y = SP.penaltyAreaHalfWidth();
//    const double speed_max = wm.self().playerType().realSpeedMax() * 0.9;
//    const int opp_min = table.opponentStep();
//    const int mate_min = table.teammateStep();
//    //const PlayerObject * fastest_opponent = table.firstOpponent();
//
//    const InterceptInfo * noturn_best = static_cast< InterceptInfo * >( 0 );
//    double noturn_score = 10000.0;
//
//    const InterceptInfo * nearest_best = static_cast< InterceptInfo * >( 0 );
//    double nearest_score = 10000.0;
//
//    const std::size_t MAX = cache.size();
//    for ( std::size_t i = 0; i < MAX; i++ )
//    {
//        const int cycle = cache[i].reachCycle();
//        const Vector2D self_pos = wm.self().inertiaPoint( cycle );
//        const Vector2D ball_pos = wm.ball().inertiaPoint( cycle );
//        const Vector2D ball_vel = wm.ball().vel() * std::pow( SP.ballDecay(), cycle );
//
//#ifdef DEBUG_PRINT
//        dlog.addText( Logger::INTERCEPT,
//                      "intercept %d: cycle=%d t=%d d=%d pos=(%.2f %.2f) vel=(%.2f %.1f) trap_ball_dist=%f",
//                      i,  cycle, cache[i].turnCycle(), cache[i].dashCycle(),
//                      ball_pos.x, ball_pos.y,
//                      ball_vel.x, ball_vel.y,
//                      cache[i].ballDist() );
//#endif
//        if ( M_save_recovery )
//            //  && cache[i].mode() != InterceptInfo::NORMAL ) CYRUS_LIB
//        {
//#ifdef DEBUG_PRINT
//        dlog.addText( Logger::INTERCEPT,
//                      "continue is not normal");
//#endif
//            continue;
//        }
//
//
//
//
//        if ( ball_pos.absX() > max_pitch_x
//             || ball_pos.absY() > max_pitch_y )
//        {
//#ifdef DEBUG_PRINT
//        dlog.addText( Logger::INTERCEPT,
//                      "continue is out");
//#endif
//            continue;
//        }
//
//        // no turn type
//
//        if ( cache[i].turnCycle() == 0 )
//        {
//            double score = Line2D(self_pos, wm.self().body()).dist(ball_pos);
//            score += (cache[i].dashCycle() + cache[i].turnCycle());
//            if ( ball_pos.absX() > max_pitch_x -1
//                 || ball_pos.absY() > max_pitch_y - 1 )
//            {
//                score += 5;
//            }
//#ifdef DEBUG_PRINT
//            dlog.addText( Logger::INTERCEPT,
//                          "___ %d noturn cycle=%d pos=(%.1f %.1f) turn=%d dash=%d score=%f",
//                          i, cycle,
//                          ball_pos.x, ball_pos.y,
//                          cache[i].turnCycle(), cache[i].dashCycle(),
//                          score );
//#endif
//            if ( score < noturn_score )
//            {
//                noturn_best = &cache[i];
//                noturn_score = score;
//#ifdef DEBUG_PRINT
//                dlog.addText( Logger::INTERCEPT,
//                              "___ %d updated noturn_best score=%f",
//                              i, score );
//#endif
//            }
//        }
//        else
//        {
//            double d = cache[i].selfPos().dist(ball_pos);
//            d += (cache[i].dashCycle() + cache[i].turnCycle());
//            if ( ball_pos.absX() > max_pitch_x -1
//                 || ball_pos.absY() > max_pitch_y - 1 )
//            {
//                d += 5;
//            }
//#ifdef DEBUG_PRINT
//            dlog.addText( Logger::INTERCEPT,
//                          "___ %d other cycle=%d pos=(%.1f %.1f) turn=%d dash=%d dist2=%.2f",
//                          i, cycle,
//                          ball_pos.x, ball_pos.y,
//                          cache[i].turnCycle(), cache[i].dashCycle(),
//                          d );
//#endif
//            if ( d < nearest_score )
//            {
//                nearest_best = &cache[i];
//                nearest_score = d;
//#ifdef DEBUG_PRINT
//                dlog.addText( Logger::INTERCEPT,
//                              "___ %d updated nearest_best score=%f",
//                              i, nearest_score );
//#endif
//            }
//        }
//    }
//
//    if ( noturn_best )
//    {
//        const Vector2D noturn_ball_vel
//            = wm.ball().vel()
//            * std::pow( SP.ballDecay(), noturn_best->reachCycle() );
//        const double noturn_ball_speed = noturn_ball_vel.r();
//        if ( noturn_ball_vel.x > 0.1
//             && ( noturn_ball_speed > speed_max )
//             )
//        {
//            dlog.addText( Logger::INTERCEPT,
//                              "<--- noturn best(1): cycle=%d(t=%d,d=%d) score=%f",
//                          noturn_best->reachCycle(),
//                          noturn_best->turnCycle(), noturn_best->dashCycle(),
//                          noturn_score );
//            return *noturn_best;
//        }
//    }
//
//    const Vector2D fastest_pos = wm.ball().inertiaPoint( cache[0].reachCycle() );
//    const Vector2D fastest_vel = wm.ball().vel() * std::pow( SP.ballDecay(),
//                                                             cache[0].reachCycle() );
//    if ( ( fastest_pos.x > -33.0
//           || fastest_pos.absY() > 20.0 )
//         && ( cache[0].reachCycle() >= 10
//             //|| wm.ball().vel().r() < 1.5 ) )
//             || fastest_vel.r() < 1.2 ) )
//    {
//        dlog.addText( Logger::INTERCEPT,
//                      "<--- fastest best: cycle=%d(t=%d,d=%d)",
//                      cache[0].reachCycle(),
//                      cache[0].turnCycle(), cache[0].dashCycle() );
//        return cache[0];
//    }
//
//    if ( noturn_best && nearest_best )
//    {
//        const Vector2D noturn_self_pos = wm.self().inertiaPoint( noturn_best->reachCycle() );
//        const Vector2D noturn_ball_pos = wm.ball().inertiaPoint( noturn_best->reachCycle() );
//        const Vector2D nearest_self_pos = wm.self().inertiaPoint( nearest_best->reachCycle() );
//        const Vector2D nearest_ball_pos = wm.ball().inertiaPoint( nearest_best->reachCycle() );
//
//        if ( noturn_self_pos.dist2( noturn_ball_pos )
//             < nearest_self_pos.dist2( nearest_ball_pos ) )
//        {
//            dlog.addText( Logger::INTERCEPT,
//                          "<--- noturn best(2): cycle=%d(t=%d,d=%d) score=%f",
//                          noturn_best->reachCycle(),
//                          noturn_best->turnCycle(), noturn_best->dashCycle(),
//                          noturn_score );
//
//            return *noturn_best;
//        }
//
//        if ( nearest_best->reachCycle() <= noturn_best->reachCycle() + 2 )
//        {
//            const Vector2D nearest_ball_vel
//                = wm.ball().vel()
//                * std::pow( SP.ballDecay(), nearest_best->reachCycle() );
//            const double nearest_ball_speed = nearest_ball_vel.r();
//            if ( nearest_ball_speed < 0.7 )
//            {
//                dlog.addText( Logger::INTERCEPT,
//                              "<--- nearest best(2): cycle=%d(t=%d,d=%d) score=%f",
//                              nearest_best->reachCycle(),
//                              nearest_best->turnCycle(), nearest_best->dashCycle(),
//                              nearest_score );
//                return *nearest_best;
//            }
//
//            const Vector2D noturn_ball_vel
//                = wm.ball().vel()
//                * std::pow( SP.ballDecay(), noturn_best->reachCycle() );
//
//            if ( nearest_best->ballDist() < wm.self().playerType().kickableArea() - 0.4
//                 && nearest_best->ballDist() < noturn_best->ballDist()
//                 && noturn_ball_vel.x < 0.5
//                 && noturn_ball_vel.r2() > std::pow( 1.0, 2 )
//                 && noturn_ball_pos.x > nearest_ball_pos.x )
//            {
//                dlog.addText( Logger::INTERCEPT,
//                              "<--- nearest best(3): cycle=%d(t=%d,d=%d) score=%f",
//                              nearest_best->reachCycle(),
//                              nearest_best->turnCycle(), nearest_best->dashCycle(),
//                              nearest_score );
//                return *nearest_best;
//            }
//
//            Vector2D nearest_self_pos = wm.self().inertiaPoint( nearest_best->reachCycle() );
//            if ( nearest_ball_speed > 0.7
//                //&& wm.self().pos().dist( nearest_ball_pos ) < wm.self().playerType().kickableArea() )
//                 && nearest_self_pos.dist( nearest_ball_pos ) < wm.self().playerType().kickableArea() )
//            {
//                dlog.addText( Logger::INTERCEPT,
//                              "<--- nearest best(4): cycle=%d(t=%d,d=%d) score=%f",
//                              nearest_best->reachCycle(),
//                              nearest_best->turnCycle(), nearest_best->dashCycle(),
//                              nearest_score );
//                return *nearest_best;
//            }
//        }
//
//        dlog.addText( Logger::INTERCEPT,
//                          "<--- noturn best(3): cycle=%d(t=%d,d=%d) score=%f",
//                      noturn_best->reachCycle(),
//                      noturn_best->turnCycle(), noturn_best->dashCycle(),
//                      noturn_score );
//
//        return *noturn_best;
//    }
//
//    if ( noturn_best )
//    {
//        dlog.addText( Logger::INTERCEPT,
//                      "<--- noturn best only: cycle=%d(t=%d,d=%d) score=%f",
//                      noturn_best->reachCycle(),
//                      noturn_best->turnCycle(), noturn_best->dashCycle(),
//                      noturn_score );
//
//        return *noturn_best;
//    }
//
//    if ( nearest_best )
//    {
//        dlog.addText( Logger::INTERCEPT,
//                      "<--- nearest best only: cycle=%d(t=%d,d=%d) score=%f",
//                      nearest_best->reachCycle(),
//                      nearest_best->turnCycle(), nearest_best->dashCycle(),
//                      nearest_score );
//
//        return *nearest_best;
//    }
//
//
//
//    if ( wm.self().pos().x > 40.0
//         && wm.ball().vel().r() > 1.8
//         && wm.ball().vel().th().abs() < 100.0
//         && cache[0].reachCycle() > 1 )
//    {
//        const InterceptInfo * chance_best = static_cast< InterceptInfo * >( 0 );
//        for ( std::size_t i = 0; i < MAX; ++i )
//        {
//            if ( cache[i].reachCycle() <= cache[0].reachCycle() + 3
//                 && cache[i].reachCycle() <= opp_min - 2 )
//            {
//                chance_best = &cache[i];
//            }
//        }
//
//        if ( chance_best )
//        {
//            dlog.addText( Logger::INTERCEPT,
//                          "<--- chance best only: cycle=%d(t=%d,d=%d)",
//                          chance_best->reachCycle(),
//                          chance_best->turnCycle(), chance_best->dashCycle() );
//            return *chance_best;
//        }
//    }
//
//    return cache[0];
//
//}
///*-------------------------------------------------------------------*/
///*!
//
//*/
//InterceptInfo
//Body_Intercept2022::getBestIntercept_Test( const WorldModel & /*wm*/,
//                                           const InterceptTable * /*table*/ ) const
//{
//#if 0
//    const std::vector< InterceptInfo > & cache = table->selfCache();
//
//    if ( cache.empty() )
//    {
//        return InterceptInfo();
//    }
//
//    const ServerParam & SP = ServerParam::i();
//    const int our_min = table.teammateStep();
//    const int opp_min = table.opponentStep();
//    const PlayerObject * opponent = table.firstOpponent();
//
//    const std::size_t MAX = 0 // cache.size(); CYRUS_LIB
//
//    for ( std::size_t i = 0; i < MAX; ++i )
//    {
//        const InterceptInfo & info = cache[i];
//
//        if ( M_save_recovery
//             && info.mode() != InterceptInfo::NORMAL )
//        {
//            continue;
//        }
//
//        const int reach_cycle = info.reachCycle();
//        const Vector2D ball_pos = wm.ball().inertiaPoint( reach_cycle );
//        const Vector2D ball_vel = wm.ball().vel() * std::pow( SP.ballDecay(), reach_cycle );
//
//
//
//    }
//
//
//#if 0
//    dlog.addText( Logger::INTERCEPT,
//                  "____ test best cycle=%d"
//                  " (turn:%d + dash:%d) power=%.1f dir=%.1f pos=(%.1f %.1f) stamina=%.1f %.1f",
//                  table->selfCache().size(),
//                  best_intercept_test.reachCycle(),
//                  best_intercept_test.turnCycle(), best_intercept_test.dashCycle(),
//                  best_intercept_test.dashPower(), best_intercept_test.dashAngle().degree(),
//                  best_intercept_test.selfPos().x, best_intercept_test.selfPos().y,
//                  best_intercept_test.stamina() );
//#endif
//#endif
//    return InterceptInfo();
//}
//
///*-------------------------------------------------------------------*/
///*!
//
//*/
//bool
//Body_Intercept2022::doWaitTurn( PlayerAgent * agent,
//                                const Vector2D & target_point,
//                                const InterceptInfo & info )
//{
//    const WorldModel & wm = agent->world();
//
//    {
//        const PlayerObject * opp = wm.getOpponentNearestToSelf( 5 );
//        if ( opp && opp->distFromSelf() < 3.0 )
//        {
//            dlog.addText( Logger::INTERCEPT,
//                          __FILE__": doWaitTurn. exist near opponent, cancel." );
//            return false;
//        }
//
//        int opp_min = wm.interceptTable().opponentStep();
//        if ( info.reachCycle() > opp_min - 5 )
//        {
//            dlog.addText( Logger::INTERCEPT,
//                          __FILE__": doWaitTurn. exist opponent intercepter, cancel." );
//            return false;
//        }
//    }
//
//    const Vector2D my_inertia = wm.self().inertiaPoint( info.reachCycle() );
//    const Vector2D target_rel = ( target_point - my_inertia ).rotatedVector( - wm.self().body() );
//    const double target_dist = target_rel.r();
//
//    const double ball_travel
//        = inertia_n_step_distance( wm.ball().vel().r(),
//                                   info.reachCycle(),
//                                   ServerParam::i().ballDecay() );
//    const double ball_noise = ball_travel * ServerParam::i().ballRand();
//
//    if ( info.reachCycle() == 1
//         && info.turnCycle() == 1 )
//    {
//        Vector2D face_point = M_face_point;
//        if ( ! face_point.isValid() )
//        {
//            face_point.assign( 50.5, wm.self().pos().y * 0.9 );
//        }
//        Body_TurnToPoint( face_point ).execute( agent );
//        dlog.addText( Logger::INTERCEPT,
//                      __FILE__": doWaitTurn. 1 step inertia_ball_dist=%.2f",
//                      target_dist  );
//        agent->debugClient().addMessage( "WaitTurn1" );
//        return true;
//    }
//
//    double extra_buf = 0.1 * bound( 0, info.reachCycle() - 1, 4 );
//    {
//        double angle_diff = ( wm.ball().vel().th() - wm.self().body() ).abs();
//        if ( angle_diff < 10.0
//             || 170.0 < angle_diff )
//        {
//            extra_buf = 0.0;
//        }
//    }
//
//    double dist_buf = wm.self().playerType().kickableArea() - 0.3 + extra_buf;
//    dist_buf -= 0.1 * wm.ball().seenPosCount();
//
//    dlog.addText( Logger::INTERCEPT,
//                  __FILE__": doWaitTurn. inertia_ball_dist=%.2f buf=%.2f extra=%.2f ball_noise=%.3f",
//                  target_dist,
//                  dist_buf, extra_buf, ball_noise );
//
//    if ( target_dist > dist_buf )
//    {
//        return false;
//    }
//
//    Vector2D face_point = M_face_point;
//    if ( info.reachCycle() > 2 )
//    {
//        face_point = my_inertia
//            + ( wm.ball().pos() - my_inertia ).rotatedVector( 90.0 );
//        if ( face_point.x < my_inertia.x )
//        {
//            face_point = my_inertia
//                + ( wm.ball().pos() - my_inertia ).rotatedVector( -90.0 );
//        }
//    }
//
//    if ( ! face_point.isValid() )
//    {
//        face_point.assign( 50.5, wm.self().pos().y * 0.9 );
//    }
//
//    Vector2D face_rel = face_point - my_inertia;
//    AngleDeg face_angle = face_rel.th();
//
//    Vector2D faced_rel = target_point - my_inertia;
//    faced_rel.rotate( face_angle );
//    if ( faced_rel.absY() > wm.self().playerType().kickableArea() - ball_noise - 0.2 )
//    {
//        dlog.addText( Logger::INTERCEPT,
//                      __FILE__": doWaitTurn. inertia_y_diff %.2f  ball_noise=%.2f",
//                      faced_rel.y, ball_noise );
//        return false;
//    }
//
//    Body_TurnToPoint( face_point ).execute( agent );
//    agent->debugClient().addMessage( "WaitTurn%d", info.reachCycle() );
//
//    return true;
//}
//
///*-------------------------------------------------------------------*/
///*!
//
//*/
//bool
//Body_Intercept2022::doInertiaDash( PlayerAgent * agent,
//                                   const Vector2D & target_point,
//                                   const InterceptInfo & info )
//{
//    const WorldModel & wm = agent->world();
//    const PlayerType & ptype = wm.self().playerType();
//
//    // if ( info.reachCycle() == 1 ) CYRUS_LIB
//    // {
//    //     // agent->debugClient().addMessage( "Intercept1Dash%.0f|%.0f",
//    //     //                                  info.dashPower(), info.dashAngle().degree() );
//    //     agent->doDash( info.dashPower(), info.dashAngle() );
//    //     return true;
//    // }
//
//    Vector2D target_rel = target_point - wm.self().pos();
//    target_rel.rotate( - wm.self().body() );
//
//    AngleDeg accel_angle = wm.self().body();
//    // if ( info.dashAngle().abs() > 179.0 ) accel_angle += 180.0;
//
//    Vector2D ball_vel = wm.ball().vel() * std::pow( ServerParam::i().ballDecay(),
//                                                    info.reachCycle() );
//
//    if ( ( ! wm.self().goalie()
//           || wm.lastKickerSide() == wm.ourSide() )
//         && wm.self().body().abs() < 50.0 )
//    {
//        double buf = 0.3;
//        if ( info.reachCycle() >= 8 )
//        {
//            buf = 0.0;
//        }
//        else if ( target_rel.absY() > wm.self().playerType().kickableArea() - 0.25 )
//        {
//            buf = 0.0;
//        }
//        else if ( target_rel.x < 0.0 )
//        {
//            if ( info.reachCycle() >= 3 ) buf = 0.5;
//        }
//        else if ( target_rel.x < 0.3 )
//        {
//            if ( info.reachCycle() >= 3 ) buf = 0.5;
//        }
//        else if ( target_rel.absY() < 0.5 )
//        {
//            if ( info.reachCycle() >= 3 ) buf = 0.5;
//            if ( info.reachCycle() == 2 ) buf = std::min( target_rel.x, 0.5 );
//        }
//        else if ( ball_vel.r() < 1.6 )
//        {
//            buf = 0.4;
//        }
//        else
//        {
//            if ( info.reachCycle() >= 4 ) buf = 0.3;
//            else if ( info.reachCycle() == 3 ) buf = 0.3;
//            else if ( info.reachCycle() == 2 ) buf = std::min( target_rel.x, 0.3 );
//        }
//
////        target_rel.x -= buf;
//
//        dlog.addText( Logger::INTERCEPT,
//                      __FILE__": doInertiaDash. slightly back to wait. buf=%.3f",
//                      buf );
//    }
//
//    double used_power = info.dashPower();
//
//    if ( wm.ball().seenPosCount() <= 2
//         && wm.ball().vel().r() * std::pow( ServerParam::i().ballDecay(), info.reachCycle() ) < ptype.kickableArea() * 1.5
//        //  && info.dashAngle().abs() < 5.0 CYRUS_LIB
//         && target_rel.absX() < ( ptype.kickableArea()
//                                  + ptype.dashRate( wm.self().effort() )
//                                  * ServerParam::i().maxDashPower()
//                                  * 0.8 ) )
//    {
//        double first_speed
//            = calc_first_term_geom_series( target_rel.x,
//                                           wm.self().playerType().playerDecay(),
//                                           info.reachCycle() );
//
//        first_speed = min_max( - wm.self().playerType().playerSpeedMax(),
//                               first_speed,
//                               wm.self().playerType().playerSpeedMax() );
//        Vector2D rel_vel = wm.self().vel().rotatedVector( - wm.self().body() );
//        double required_accel = first_speed - rel_vel.x;
//        used_power = required_accel / wm.self().dashRate();
//        // used_power /= ServerParam::i().dashDirRate( info.dashAngle().degree() ); CYRUS_LIB
//
//        //if ( info.dashPower() < 0.0 ) used_power = -used_power;
//
//        used_power = ServerParam::i().normalizeDashPower( used_power );
//        if ( M_save_recovery )
//        {
//            used_power = wm.self().getSafetyDashPower( used_power );
//        }
//
//        // agent->debugClient().addMessage( "InterceptInertiaDash%d:%.0f|%.0f",
//        //                                  info.reachCycle(), used_power, info.dashAngle().degree() ); CYRUS_LIB
//        dlog.addText( Logger::INTERCEPT,
//                      __FILE__": doInertiaDash. x_diff=%.2f first_speed=%.2f"
//                      " accel=%.2f power=%.1f",
//                      target_rel.x, first_speed, required_accel, used_power );
//
//    }
//    else
//    {
//        // agent->debugClient().addMessage( "InterceptDash%d:%.0f|%.0f",
//        //                                  info.reachCycle(), used_power, info.dashAngle().degree() ); CYRUS_LIB
//        dlog.addText( Logger::INTERCEPT,
//                      __FILE__": doInertiaDash. normal dash. x_diff=%.2f ",
//                      target_rel.x );
//    }
//
//
//    if ( info.reachCycle() >= 4
//         && ( target_rel.absX() < 0.5
//              || std::fabs( used_power ) < 5.0 )
//         )
//    {
//        agent->debugClient().addMessage( "LookBall" );
//
//        Vector2D my_inertia = wm.self().inertiaPoint( info.reachCycle() );
//        Vector2D face_point = M_face_point;
//        if ( ! M_face_point.isValid() )
//        {
//            face_point.assign( 50.5, wm.self().pos().y * 0.75 );
//        }
//        AngleDeg face_angle = ( face_point - my_inertia ).th();
//
//        Vector2D ball_next = wm.ball().pos() + wm.ball().vel();
//        AngleDeg ball_angle = ( ball_next - my_inertia ).th();
//        double normal_half_width = ViewWidth::width( ViewWidth::NORMAL );
//
//        if ( ( ball_angle - face_angle ).abs()
//             > ( ServerParam::i().maxNeckAngle()
//                 + normal_half_width
//                 - 10.0 )
//             )
//        {
//            face_point.x = my_inertia.x;
//            if ( ball_next.y > my_inertia.y + 1.0 ) face_point.y = 50.0;
//            else if ( ball_next.y < my_inertia.y - 1.0 ) face_point.y = -50.0;
//            else  face_point = ball_next;
//            dlog.addText( Logger::INTERCEPT,
//                          __FILE__": doInertiaDash. check ball with turn."
//                          " face to (%.1f %.1f)",
//                          face_point.x, face_point.y );
//        }
//        else
//        {
//            dlog.addText( Logger::INTERCEPT,
//                          __FILE__": doInertiaDash. can check ball without turn"
//                          " face to (%.1f %.1f)",
//                          face_point.x, face_point.y );
//        }
//        Body_TurnToPoint( face_point ).execute( agent );
//        return true;
//    }
//
//    agent->doDash( used_power , info.turnAngle()); // info.dashAngle() ); CYRUS_LIB
//    return true;
//}


bool Bhv_BasicMove::set_def_neck_with_ball(PlayerAgent *agent, Vector2D targetPoint, const AbstractPlayerObject *opp,
                                           int blocker) {
    const WorldModel &wm = agent->world();
    Vector2D next_target = Vector2D::INVALIDATED;
    int self_min = wm.interceptTable().selfStep();
    int ball_pos_count = wm.ball().posCount();
    int ball_pos_count_s = wm.ball().seenPosCount();
    int ball_vel_count = wm.ball().velCount();
    int ball_vel_count_s = wm.ball().seenVelCount();
    Vector2D ball_pos = wm.ball().pos();
    Vector2D self_pos = wm.self().pos();
    AngleDeg self_body = agent->effector().queuedNextMyBody();
    int min_ball_count = std::min(std::min(ball_pos_count, ball_pos_count_s),
                                  std::min(ball_vel_count, ball_vel_count_s));
    AngleDeg ball_angle = (ball_pos - self_pos).th();
    Vector2D ball_iner = wm.ball().inertiaPoint(self_min);
    const double next_view_width = agent->effector().queuedNextViewWidth().width();
    bool should_see_ball = false;
    if (min_ball_count >= std::min(wm.interceptTable().opponentStep(), 5))
        should_see_ball = true;
    bool can_see_ball = false;
    if ((self_body - ball_angle).abs() < next_view_width * 0.5 + 85)
        can_see_ball = true;
    if (wm.self().isKickable())
        should_see_ball = false;
    std::vector<std::pair<Vector2D, double>> targs;
    for (int i = 0; i < wm.opponentsFromSelf().size() && i <= 3; i++) {
        if (wm.opponentsFromSelf().at(i) != NULL && wm.opponentsFromSelf().at(i)->posCount() > 0)
            targs.emplace_back(wm.opponentsFromSelf().at(i)->pos(), 0.9);
    }
    //
    //    for (int i = 0; i < wm.opponentsFromBall().size() && i <= 3; i++) {
    //        if (wm.opponentsFromBall().at(i)->posCount() > 0)
    //            targs.push_back(make_pair(wm.opponentsFromBall().at(i)->pos(), 0.9));
    //    }

    for (int i = 0; i < wm.teammatesFromSelf().size() && i <= 2; i++) {
        if (wm.teammatesFromSelf().at(i)->posCount() > 0)
            targs.emplace_back(make_pair(wm.teammatesFromSelf().at(i)->pos(), 0.8));
    }
    if (opp->posCount() > 0)
        targs.emplace_back(make_pair(opp->pos(), 1.3));
    //    targs.emplace_back(make_pair(targetPoint, 0.7));
    //    targs.push_back(ball_iner);
    if (wm.interceptTable().firstOpponent() != NULL
            && wm.interceptTable().firstOpponent()->posCount() > 0) {
        targs.emplace_back(make_pair(wm.interceptTable().firstOpponent()->pos(), 1.1));
    }
    if (blocker != 0 && blocker != wm.self().unum() && wm.ourPlayer(blocker) != NULL &&
            wm.ourPlayer(blocker)->posCount() > 0)
        targs.emplace_back(make_pair(wm.ourPlayer(blocker)->pos(), 0.9));
    if (wm.ball().posCount() > 0)
        targs.emplace_back(make_pair(wm.ball().pos(), 1.2));

    vector<const AbstractPlayerObject *> myLine = Strategy::i().myLineTmms(wm, Strategy::i().tm_Line(wm.self().unum()));
    for (auto &i : myLine) {
        if (i->unum() != wm.self().unum() && i->posCount() > 2)
            targs.emplace_back(make_pair(i->pos(), 0.7));
    }
    if (ball_iner.x < -35 && wm.getOurGoalie() != NULL && wm.getOurGoalie()->posCount() > 2) {
        targs.emplace_back(make_pair(wm.getOurGoalie()->pos(), 0.6));
    }


    for (auto &targ : targs) {
        dlog.addText(Logger::TEAM,
                     __FILE__": def neck target=(%.1f %.1f) eval=%.2f", targ.first.x,
                     targ.first.y, targ.second);
    }


    double best_neck = -1000;
    double best_eval = 0;
    for (double neck = self_body.degree() - 90; neck <= self_body.degree() + 90; neck += 10) {
        AngleDeg min_see(neck - next_view_width / 2.0 + 10);
        AngleDeg max_see = (neck + next_view_width / 2.0 - 10);
        double eval = 0;
        if (should_see_ball) {
            if (can_see_ball) {
                if (ball_angle.isWithin(min_see, max_see)) {
                    for (auto &targ : targs) {
                        AngleDeg targ_angle = (targ.first - self_pos).th();
                        if (targ_angle.isWithin(min_see, max_see)) {
                            eval += (wm.dirCount(targ_angle) * targ.second);
                        }
                    }
                }
            } else {
                for (auto &targ : targs) {
                    AngleDeg targ_angle = (targ.first - self_pos).th();
                    if (targ_angle.isWithin(min_see, max_see)) {
                        eval += (wm.dirCount(targ_angle) * targ.second);
                    }
                }
            }
        } else {
            for (auto &targ : targs) {
                AngleDeg targ_angle = (targ.first - self_pos).th();
                if (targ_angle.isWithin(min_see, max_see)) {
                    eval += (wm.dirCount(targ_angle) * targ.second);
                }
            }
        }
        if (eval > best_eval) {
            best_eval = eval;
            best_neck = neck;
        }
    }
    dlog.addText(Logger::TEAM,
                 __FILE__": def neck %.1f eval=%.2f", best_neck, best_eval);
    if (best_eval != 0) {
        agent->setNeckAction(new Neck_TurnToPoint(self_pos + Vector2D::polar2vector(10, best_neck)));
    } else if (should_see_ball && can_see_ball) {
        agent->setNeckAction(new Neck_TurnToBall());
    } else {
        agent->setNeckAction(new Neck_TurnToBallOrScan(1));
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

bool
Bhv_BasicMove::execute(PlayerAgent *agent) {

    //    if(ServerParam::i().synchMode() == true){
    //        std::this_thread::sleep_for(std::chrono::seconds(1));
    //        std::cout<<"Deep Learning for Move is Already RUN"<<std::endl;
    //    }
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
    Vector2D ball_iner = wm.ball().inertiaPoint(std::min(std::min(opp_min, mate_min), self_min));
    int unum = wm.self().unum();
    int stamina = wm.self().stamina();
    Vector2D target_point = Strategy::i().getPosition(wm.self().unum());
    double dash_power = Strategy::get_normal_dash_power(wm);
    Vector2D self_pos = wm.self().pos();


    if (intercept_plan(agent)) {
        return true;
    }
    if (TurnToTackle(agent)) {
        return true;
    }

    bool can_5_go_forward = true;
    if(wm.self().unum() == 5){
        if (!Strategy::i().isDefSit(wm, wm.self().unum())){
            if(ball_iner.x > 0 && self_pos.x < 0){
                if(stamina < 5500){
                    can_5_go_forward = false;
                    target_point.x = std::min(target_point.x, 0.0);
                }
            }
        }
    }
    if(wm.self().unum() < 5){
        if (!Strategy::i().isDefSit(wm, wm.self().unum())){
            if(ball_iner.x > 0 && self_pos.x < 0){
                if(stamina < 6000){
                    target_point.x = std::min(target_point.x, -1.0);
                }
            }
        }
    }
    // todo check formation
    if (Strategy::i().isDefSit(wm, wm.self().unum())){
        double xdist_to_home5 = 1000;
        if(wm.ourPlayer(5)!= nullptr && wm.ourPlayer(5)->unum() > 0){
            xdist_to_home5 = wm.ourPlayer(5)->pos().x - Strategy::i().getPosition(5).x;
        }
        double xdist_to_home6 = 1000;
        if(wm.ourPlayer(6)!= nullptr && wm.ourPlayer(6)->unum() > 0){
            xdist_to_home6 = wm.ourPlayer(6)->pos().x - Strategy::i().getPosition(6).x;
        }
        bool is_5_far = xdist_to_home5 > 10;
        bool is_6_far = xdist_to_home6 > 10;
        Vector2D home2 = Strategy::i().getPosition(2);
        Vector2D home3 = Strategy::i().getPosition(3);
        Vector2D home4 = Strategy::i().getPosition(4);
        Vector2D home5 = Strategy::i().getPosition(5);
        Vector2D home6 = Strategy::i().getPosition(6);
        if (ball_iner.x > -30){
            if (is_5_far && is_6_far){
                Vector2D new_home2 = home2 - Vector2D(4, 0);
                Vector2D new_home3 = (home3 + home5) / 2.0 - Vector2D(4, 0);
                Vector2D new_home4 = (home4 + home6) / 2.0 - Vector2D(4, 0);
                Vector2D new_home5 = home5 - Vector2D(4, 0);
                Vector2D new_home6 = home6 - Vector2D(4, 0);
                Strategy::i().set_position(2, new_home2);
                Strategy::i().set_position(3, new_home3);
                Strategy::i().set_position(4, new_home4);
                Strategy::i().set_position(5, new_home5);
                Strategy::i().set_position(6, new_home6);
            }
            else if (is_5_far){
                Vector2D new_home2 = (home2 * 2.0 + home5) / 3.0 - Vector2D(3, 0);
                Vector2D new_home3 = (home3 + home5) / 2.0 - Vector2D(3, 0);
                Vector2D new_home4 = (home4 * 2.0 + home6) / 3.0 - Vector2D(3, 0);
                Vector2D new_home5 = home5 - Vector2D(3, 0);
                Vector2D new_home6 = (home2 + home6 * 2.0) / 3.0 - Vector2D(3, 0);
                Strategy::i().set_position(2, new_home2);
                Strategy::i().set_position(3, new_home3);
                Strategy::i().set_position(4, new_home4);
                Strategy::i().set_position(5, new_home5);
                Strategy::i().set_position(6, new_home6);
            }
            else if (is_6_far){
                Vector2D new_home2 = (home2 * 2.0 + home6) / 3.0 - Vector2D(3, 0);
                Vector2D new_home3 = (home3 + home5 * 2.0) / 3.0 - Vector2D(3, 0);
                Vector2D new_home4 = (home4 + home6) / 2.0 - Vector2D(3, 0);
                Vector2D new_home5 = (home3 + home5 * 2.0) / 3.0 - Vector2D(3, 0);
                Vector2D new_home6 = home6 - Vector2D(3, 0);
                Strategy::i().set_position(2, new_home2);
                Strategy::i().set_position(3, new_home3);
                Strategy::i().set_position(4, new_home4);
                Strategy::i().set_position(5, new_home5);
                Strategy::i().set_position(6, new_home6);
            }
        }
    }
    else{
        if(wm.self().unum() == 6){
            if(wm.ourPlayer(5)!= nullptr && wm.ourPlayer(5)->unum() > 0){
                if(Strategy::i().getPosition(5).dist(wm.ourPlayer(5)->pos())>10){
                    if(ball_iner.x > 20){
                        Strategy::i().set_position(6, (Strategy::i().getPosition(5) + target_point)/ 2.0);
                        target_point = Strategy::i().getPosition(wm.self().unum());
                    }
                }
            }
        }
    }
    if (Strategy::i().isDefSit(wm, wm.self().unum()) ||
            (Strategy::i().tm_Line(wm.self().unum()) == Strategy::PostLine::back && wm.ball().inertiaPoint(opp_min).x > 30)) {
        if (DefSitPlan(agent))
            return true;
    } else {
        if(can_5_go_forward)
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

bool can_opp_shoot_to_goal(const WorldModel &wm) {
    int opp_min = wm.interceptTable().opponentStep();
    Vector2D ballpos = wm.ball().inertiaPoint(opp_min);
    if (!ServerParam::i().ourPenaltyArea().contains(ballpos))
        return false;
    bool twostepshoot = false;
    if (opp_min > 0 && wm.ball().vel().r() * std::pow(ServerParam::i().ballDecay(), opp_min) > 0.5) {
        AngleDeg opp2goal = (ballpos - Vector2D(-52, 0)).th();
        AngleDeg opp2ball = (ballpos - wm.ball().pos()).th();
        if ((opp2goal - opp2ball).abs() < 9)
            twostepshoot = true;
    }
    bool can_goal = false;
    for (double y = -6.5; y < 6.6; y += 0.5) {
        Vector2D target(-52.5, y);
        Vector2D ball_vel_sim = Vector2D::polar2vector(2.9, (target - ballpos).th());
        Vector2D ball_pos_sim = ballpos;
        bool can_intercept = false;
        for (int c = 1; c <= 30; c++) {
            ball_pos_sim += ball_vel_sim;
            if (ball_pos_sim.x < -52.5)
                can_goal = true;
            for (int t = 1; t <= 5; t++) {
                const AbstractPlayerObject *tm = wm.ourPlayer(t);
                if (tm == NULL || tm->unum() != t)
                    continue;
                if (tm->pos().dist(ball_pos_sim) / tm->playerTypePtr()->realSpeedMax() > c + 2)
                    continue;
                int dc;
                int dt;
                int dv;
                int intercept_cycle = CutBallCalculator().cycles_to_cut_ball(tm,  ball_pos_sim, c, true, dc, dt, dv);
                if (intercept_cycle <= c) {
                    can_intercept = true;
                    break;
                }
            }
            if (can_intercept)
                break;
            ball_vel_sim *= ServerParam::i().ballDecay();
        }
        if (can_intercept) {
            dlog.addCircle(Logger::SHOOT, target, 0.2, 0, 255, 0, true);
        } else {
            dlog.addCircle(Logger::SHOOT, target, 0.2, 255, 0, 0, true);
        }
    }
    return can_goal;
}


vector<int> Bhv_BasicMove::who_goto_goal(PlayerAgent *agent) {
    const WorldModel &wm = agent->world();
    vector<int> results;
    int opp_min = wm.interceptTable().opponentStep();
    Vector2D ballpos = wm.ball().inertiaPoint(opp_min);
    if (can_opp_shoot_to_goal(wm)) {
        double min_dist = 1000;
        Vector2D tar;
        for (int i = 2; i <= 5; i++) {
            if (ballpos.y > 5) {
                tar = Vector2D(-52, -4);
            } else if (ballpos.y < -5) {
                tar = Vector2D(-52, +4);
            } else {
                if (Strategy::i().getPosition(i).y > 0) {
                    tar = Vector2D(-52, +5);
                } else {
                    tar = Vector2D(-52, -5);
                }
            }
            int min_unum = 0;
            for (int t = 2; t <= 5; t++) {
                const AbstractPlayerObject *tm = wm.ourPlayer(t);
                if (tm == NULL || tm->unum() != t)
                    continue;
                if (tm->pos().dist(wm.ball().inertiaPoint(wm.interceptTable().opponentStep())) < 3)
                    continue;
                Vector2D pos = tm->pos();
                double dist = pos.dist(tar);
                if (dist < min_dist) {
                    min_dist = dist;
                    min_unum = t;
                }
            }
            if (min_unum == i)
                results.push_back(i);
        }
    }
    return results;
}


bool go_to_goal(PlayerAgent *agent) {
    const WorldModel &wm = agent->world();
    int opp_min = wm.interceptTable().opponentStep();
    Vector2D ballpos = wm.ball().inertiaPoint(opp_min);
    if (wm.self().pos().dist(wm.ball().inertiaPoint(wm.interceptTable().opponentStep())) < 3)
        return false;
    if (can_opp_shoot_to_goal(wm)) {
        double min_dist = 1000;
        int min_unum = 0;
        Vector2D tar;
        if (ballpos.y > 5) {
            tar = Vector2D(-52, -4);
        } else if (ballpos.y < -5) {
            tar = Vector2D(-52, +4);
        } else {
            if (Strategy::i().getPosition(wm.self().unum()).y > 0) {
                tar = Vector2D(-52, +5);
            } else {
                tar = Vector2D(-52, -5);
            }
        }
        for (int t = 2; t <= 5; t++) {
            const AbstractPlayerObject *tm = wm.ourPlayer(t);
            if (tm == NULL || tm->unum() != t)
                continue;
            Vector2D pos = tm->pos();
            double dist = pos.dist(tar);
            if (dist < min_dist) {
                min_dist = dist;
                min_unum = t;
            }
        }

        if (min_unum == wm.self().unum()) {
            Vector2D self_pos = wm.self().pos();

            if (ballpos.y > 5) {
                if (ballpos.x < -42) {
                    tar = Vector2D(-51, 0);
                } else if (ballpos.x < -39) {
                    tar = Vector2D(-51, +4);
                }
            } else if (ballpos.y < -5) {
                if (ballpos.x < -42) {
                    tar = Vector2D(-51, 0);
                } else if (ballpos.x < -39) {
                    tar = Vector2D(-52, -4);
                }
            } else {
                if (ballpos.x < -42) {
                    if (tar.y < 0)
                        tar = Vector2D(-51, -4);
                    else
                        tar = Vector2D(-51, +4);
                } else if (ballpos.x < -39) {
                    if (tar.y < 0)
                        tar = Vector2D(-51, -5);
                    else
                        tar = Vector2D(-51, +5);
                }
            }

            if (!Body_GoToPoint(tar, 1.5, 100, 2, 1, false, 20).execute(agent)) {
                if (self_pos.dist(tar) < 1.0 && wm.interceptTable().opponentStep() <= 1 &&
                        wm.self().body().abs() > 80 && wm.self().body().abs() < 100) {
                    agent->doDash(100, (tar - wm.self().pos()).th() - wm.self().body());
                } else {
                    if (ballpos.y > wm.self().pos().y) {
                        Body_TurnToAngle(90).execute(agent);
                    } else {
                        Body_TurnToAngle(-90).execute(agent);
                    }
                }
            }
            agent->debugClient().addMessage("goto goal");
            if (wm.kickableOpponent() && wm.ball().distFromSelf() < 18.0 || opp_min <= 2) {
                agent->setNeckAction(new Neck_TurnToBall());
            } else {
                agent->setNeckAction(new Neck_TurnToBallOrScan(0));
            }
            return true;
        }
    }
    return false;
}

bool Bhv_BasicMove::DefSitPlan(rcsc::PlayerAgent *agent) {

    const WorldModel &wm = agent->world();
    /*--------------------------------------------------------*/
    // chase ball
    const int self_min = wm.interceptTable().selfStep();
    const int mate_min = wm.interceptTable().teammateStep();
    const int opp_min = wm.interceptTable().opponentStep();

    //	cout<<"*****"<<wm.time().cycle()<<endl;
    if (wm.interceptTable().firstOpponent() == NULL || wm.interceptTable().firstOpponent()->unum() < 1)
        return false;

    bool mark_or_block = true;
    if (Strategy::i().self_Line() == Strategy::PostLine::forward) {

        if (wm.ball().pos().x < -20) {
            if (wm.self().stamina() < 4000) {
                mark_or_block = false;
            }
        } else if (wm.self().stamina() < 5000) {
            mark_or_block = false;
        }
    }
    if(Setting::i()->mStrategySetting->mIsGoalForward && wm.self().unum()==2)
        mark_or_block = false;
//    if (Strategy::i().self_Line() == Strategy::PostLine::back) {
//        if (go_to_goal(agent)){
//            agent->debugClient().addMessage("GoToGoal");
//            return true;
//        }
//    }
    if (mark_or_block) {
        dlog.addText(Logger::BLOCK, "block or mark");
        if (bhv_mark_execute().execute(agent)){
            agent->debugClient().addMessage("MarkEx");
            return true;
        }
    }

    double min_x_hpos = 1000;
    for (int i = 2; i <= 11; i++) {
        if (Setting::i()->mStrategySetting->mIsGoalForward && i==2)
            continue;
        double hposx = Strategy::i().getPosition(i).x;
        if (hposx < min_x_hpos)
            min_x_hpos = hposx;
    }

    Vector2D inertia_ball_pos = wm.ball().inertiaPoint(
                wm.interceptTable().opponentStep());
    double mark_x_line = std::max(wm.ourDefenseLineX(), min_x_hpos);

    Vector2D target_point = Strategy::i().getPosition(wm.self().unum());

    if (wm.audioMemory().waitRequest().size() > 0) {
        if (wm.audioMemory().waitRequestTime().cycle() > wm.time().cycle() - 4) {
            if (Strategy::i().tm_Line(wm.self().unum()) == Strategy::PostLine::back) {
                if (wm.audioMemory().waitRequest().front().sender_ != wm.self().unum()) {
                    const AbstractPlayerObject *tm = wm.ourPlayer(wm.audioMemory().waitRequest().front().sender_);
                    if (tm != NULL && tm->unum() > 1) {
                        target_point.x = tm->pos().x;
                    }
                }
            }
        }
    }
    //    if (wm.theirTeamName().compare("KN2C") == 0) {
    //        int our_score = (wm.ourSide() == LEFT
    //                         ? wm.gameMode().scoreLeft()
    //                         : wm.gameMode().scoreRight());
    //        int opp_score = (wm.ourSide() == LEFT
    //                         ? wm.gameMode().scoreRight()
    //                         : wm.gameMode().scoreLeft());
    //        if (opp_score > our_score) {
    //            vector<int> unums;
    //            unums.push_back(0);
    //            unums.push_back(0);
    //            unums.push_back(11);
    //            unums.push_back(10);
    //            unums.push_back(9);
    //            unums.push_back(8);
    //            unums.push_back(7);
    //            unums.push_back(5);
    //            unums.push_back(4);
    //            unums.push_back(3);
    //            unums.push_back(2);
    //            unums.push_back(6);
    //            int u = unums.at(wm.self().unum());
    //            if (wm.theirPlayer(u) != NULL && wm.theirPlayer(u)->unum() == u) {
    //                target_point = wm.theirPlayer(u)->pos() - Vector2D(1, 0);
    //            }
    //        }
    //    }

    double min_x_strategy = 100;
    for (int i = 2; i <= 11; i++) {
        if (Setting::i()->mStrategySetting->mIsGoalForward && i==2)
            continue;
        double x = Strategy::i().getPosition(i).x;
        if (x < min_x_strategy)
            min_x_strategy = x;
    }

    double dash_power = Strategy::get_normal_dash_power(wm);
    if (wm.ball().pos().x < wm.self().pos().x
            && Strategy::i().self_Line() == Strategy::PostLine::back) {
        //		target_point.x = min(-42.0, inertia_ball_pos.x);
        dash_power = 100;
    }
    /*if (wm.ourDefenseLineX() < target_point.x && wm.self().unum() < 6) {
     target_point.x = wm.ourDefenseLineX();
     dash_power = 100;
     }*/
    if (wm.self().pos().x < wm.ourDefenseLineX()
            && wm.ball().pos().x > wm.ourDefenseLineX()) {
        //		target_point.x = (wm.ourDefenseLineX() - 1);
    }
    if (Strategy::i().self_Line() == Strategy::PostLine::back || Strategy::i().self_Line() == Strategy::PostLine::half) {
        if (wm.ball().inertiaPoint(opp_min).x < -20
                && (wm.self().pos().dist(target_point) > 4
                    || wm.self().pos().x > target_point.x + 2)) {
            dash_power = 100;
        }
    }

    if (wm.self().pos().x < min_x_strategy)
        dash_power = 100;
    if (Strategy::i().self_line == Strategy::PostLine::back && abs(inertia_ball_pos.x - wm.ourDefenseLineX()) < 20) {
        dash_power = 100;
    }
    double dist_thr = wm.ball().distFromSelf() * 0.1;
    if (dist_thr < 1.0)
        dist_thr = 1.0;

    if(wm.self().unum() < 5){
        if(inertia_ball_pos.x > 0 && wm.self().pos().x < 0){
            if(wm.self().stamina() < 6000){
                target_point.x = std::min(target_point.x, -1.0);
            }
        }
    }
    dlog.addText(Logger::TEAM,
                 __FILE__": Bhv_BasicMove target=(%.1f %.1f) dist_thr=%.2f", target_point.x,
                 target_point.y, dist_thr);

    agent->debugClient().addMessage("BasicMoveDef%.0f", dash_power);
    agent->debugClient().setTarget(target_point);
    agent->debugClient().addCircle(target_point, dist_thr);
    bool isGoalieForward=Setting::i()->mStrategySetting->mIsGoalForward;
    double base_def_pos_x = isGoalieForward?Strategy::i().getPosition(3).x:Strategy::i().getPosition(2).x;
    if (wm.self().stamina() < 4500 && wm.self().pos().x < base_def_pos_x) {
        agent->addSayMessage(new WaitRequestMessage());
    }
    if (!Body_GoToPoint(target_point, dist_thr, dash_power).execute(agent)) {
        Body_TurnToPoint(target_point).execute(agent);
    }
    const AbstractPlayerObject *nearest_opp = wm.opponentsFromSelf().at(0);
    if (nearest_opp != nullptr)
        set_def_neck_with_ball(agent, wm.ball().pos(), nearest_opp, 0);
    else {
        if (wm.kickableOpponent() && wm.ball().distFromSelf() < 18.0) {
            agent->setNeckAction(new Neck_TurnToBall());
        } else {
            agent->setNeckAction(new Neck_TurnToBallOrScan(0));
        }
    }
    return true;
}


#include "sample_communication.h"
