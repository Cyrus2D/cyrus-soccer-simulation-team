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

#include "bhv_set_play_kick_in.h"

#include "../strategy.h"
#include "setting.h"
#include "bhv_set_play.h"
#include "bhv_go_to_static_ball.h"
#include "bhv_chain_action.h"

#include "intention_wait_after_set_play_kick.h"

#include "basic_actions/basic_actions.h"
#include "basic_actions/body_go_to_point.h"
#include "basic_actions/body_kick_one_step.h"
#include "basic_actions/body_advance_ball.h"
#include "basic_actions/body_pass.h"
#include "basic_actions/neck_scan_field.h"
#include "basic_actions/neck_turn_to_ball_or_scan.h"
#include <lib/cyrus_say_message_builder.h>
#include <lib/cyrus_audio_memory.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/player/say_message_builder.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>

#include <rcsc/math_util.h>

#include <algorithm>
#include <limits>

using namespace rcsc;
bool Bhv_SetPlayKickIn::kick_waiting_finish_check;
int Bhv_SetPlayKickIn::kick_waiting_finish_cycle;
int Bhv_SetPlayKickIn::waiting_before_kick_cycle;

/*!
  execute action
*/
bool
Bhv_SetPlayKickIn::execute( PlayerAgent * agent )
{
    const WorldModel &wm = agent->world();

    dlog.addText( Logger::TEAM,
                  __FILE__": Bhv_SetPlayKickIn" );

    if (Setting::i()->mStrategySetting->mMoveBeforeSetPlay) {
        dlog.addText(Logger::TEAM,
                     __FILE__": Bhv_SetPlayKickIn cycle %d lastsetplay %d", wm.time().cycle(),
                     wm.lastSetPlayStartTime().cycle());

        if (wm.time().cycle() == wm.lastSetPlayStartTime().cycle() + 1) {
            kick_waiting_finish_check = false;
            kick_waiting_finish_cycle = wm.time().cycle();
            waiting_before_kick_cycle = 5;
        }

        const int real_set_play_count
                = static_cast< int >( wm.time().cycle() - wm.lastSetPlayStartTime().cycle());

        if (real_set_play_count >= ServerParam::i().dropBallTime() - waiting_before_kick_cycle) {
            waiting_before_kick_cycle = ServerParam::i().dropBallTime() - real_set_play_count - 1;
            dlog.addText(Logger::ROLE,
                         __FILE__"can not wait more than %d cycle",
                         waiting_before_kick_cycle);
        }
    }

    if ( Bhv_SetPlay::is_kicker( agent ) )
    {
        doKick( agent );
    }
    else
    {
        doMove( agent );
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Bhv_SetPlayKickIn::doKick( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    //
    // go to the kick position
    //
    AngleDeg ball_place_angle = ( wm.ball().pos().y > 0.0
                                  ? -90.0
                                  : 90.0 );
    if ( Bhv_GoToStaticBall( ball_place_angle ).execute( agent ) )
    {
        return;
    }

    //
    // wait
    //
    if (Setting::i()->mStrategySetting->mMoveBeforeSetPlay) {
        if(!kick_waiting_finish_check)
        {
            if ( doKickWait( agent ) )
            {
                return;
            }
            else if (wm.ball().pos().x > 25)
            {
                kick_waiting_finish_check = true;
                kick_waiting_finish_cycle = wm.time().cycle();
                agent->addSayMessage(new StartSetPlayKickMessage());
            }
        }

        if(kick_waiting_finish_check && wm.time().cycle() - kick_waiting_finish_cycle < waiting_before_kick_cycle)
        {
            const Vector2D face_point( 40.0, 0.0 );
            const AngleDeg face_angle = ( face_point - wm.self().pos() ).th();

            Body_TurnToPoint( face_point ).execute( agent );
            dlog.addText( Logger::ROLE,
                          __FILE__"Waiting for %d from %d cycle before kick " , wm.time().cycle() - kick_waiting_finish_cycle , waiting_before_kick_cycle);
            agent->setNeckAction( new Neck_ScanField() );
            return;

        }
    }
    else
    {
        if ( doKickWait( agent ) )
        {
            return;
        }
    }

    //
    // kick
    //

    const double max_ball_speed = wm.self().kickRate() * ServerParam::i().maxPower();

    //
    // pass
    //
    if ( Bhv_ChainAction().execute( agent ) )
    {
        agent->setIntention( new IntentionWaitAfterSetPlayKick() );
        agent->debugClient().addMessage( "KickIn:Chain" );
        return;
    }

    //
    // kick to the nearest teammate
    //
    {
        const PlayerObject * receiver = wm.getTeammateNearestToBall( 10 );
        if ( receiver
             && receiver->distFromBall() < 10.0
             && receiver->pos().absX() < ServerParam::i().pitchHalfLength()
             && receiver->pos().absY() < ServerParam::i().pitchHalfWidth() )
        {
            Vector2D target_point = receiver->inertiaFinalPoint();
            target_point.x += 0.5;

            double ball_move_dist = wm.ball().pos().dist( target_point );
            int ball_reach_step
                    = static_cast< int >( std::ceil( calc_length_geom_series( max_ball_speed,
                                                                              ball_move_dist,
                                                                              ServerParam::i().ballDecay() ) ) );
            double ball_speed = 0.0;
            if ( ball_reach_step > 3 )
            {
                ball_speed = calc_first_term_geom_series( ball_move_dist,
                                                          ServerParam::i().ballDecay(),
                                                          ball_reach_step );
            }
            else
            {
                ball_speed = calc_first_term_geom_series_last( 1.4,
                                                               ball_move_dist,
                                                               ServerParam::i().ballDecay() );
                ball_reach_step
                        = static_cast< int >( std::ceil( calc_length_geom_series( ball_speed,
                                                                                  ball_move_dist,
                                                                                  ServerParam::i().ballDecay() ) ) );
            }

            ball_speed = std::min( ball_speed, max_ball_speed );

            agent->debugClient().addMessage( "KickIn:ForcePass%.3f", ball_speed );
            agent->debugClient().setTarget( target_point );

            dlog.addText( Logger::TEAM,
                          __FILE__":  kick to nearest teammate (%.1f %.1f) speed=%.2f",
                          target_point.x, target_point.y,
                          ball_speed );
            Body_KickOneStep( target_point,
                              ball_speed
            ).execute( agent );
            agent->setNeckAction( new Neck_ScanField() );
            return;
        }
    }

    //
    // clear
    //

    //
    // turn to ball
    //

    if ( ( wm.ball().angleFromSelf() - wm.self().body() ).abs() > 1.5 )
    {
        agent->debugClient().addMessage( "KickIn:Advance:TurnToBall" );
        dlog.addText( Logger::TEAM,
                      __FILE__":  clear. turn to ball" );

        Body_TurnToBall().execute( agent );
        agent->setNeckAction( new Neck_ScanField() );
        return;
    }

    //
    // advance ball
    //

    if ( wm.self().pos().x < 20.0 )
    {
        agent->debugClient().addMessage( "KickIn:Advance" );

        dlog.addText( Logger::TEAM,
                      __FILE__": advance(1)" );
        Body_AdvanceBall().execute( agent );
        agent->setNeckAction( new Neck_ScanField() );
        return;
    }

    //
    // kick to the opponent side corner
    //
    {
        agent->debugClient().addMessage( "KickIn:ForceAdvance" );

        Vector2D target_point( ServerParam::i().pitchHalfLength() - 2.0,
                               ( ServerParam::i().pitchHalfWidth() - 5.0 )
                               * ( 1.0 - ( wm.self().pos().x
                                           / ServerParam::i().pitchHalfLength() ) ) );
        if ( wm.self().pos().y < 0.0 )
        {
            target_point.y *= -1.0;
        }
        // enforce one step kick
        dlog.addText( Logger::TEAM,
                      __FILE__": advance(2) to (%.1f, %.1f)",
                      target_point.x, target_point.y );
        Body_KickOneStep( target_point,
                          ServerParam::i().ballSpeedMax()
        ).execute( agent );
        agent->setNeckAction( new Neck_ScanField() );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_SetPlayKickIn::doKickWait( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    int our_score = ( wm.ourSide() == LEFT
                      ? wm.gameMode().scoreLeft()
                      : wm.gameMode().scoreRight() );
    int opp_score = ( wm.ourSide() == LEFT
                      ? wm.gameMode().scoreRight()
                      : wm.gameMode().scoreLeft() );


    bool wait_is_beter = true;

    if ( our_score <= opp_score )
        if(wm.time().cycle() < 5000)
            wait_is_beter = true;

    const int real_set_play_count
            = static_cast< int >( wm.time().cycle() - wm.lastSetPlayStartTime().cycle() );

    if ( real_set_play_count >= ServerParam::i().dropBallTime() - 5 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (dontKickWait) real set play count = %d > drop_time-10, force kick mode",
                      real_set_play_count );
        return false;
    }

    if ( Bhv_SetPlay::is_delaying_tactics_situation( agent ) )
    {
        agent->debugClient().addMessage( "KickIn:Delaying" );
        dlog.addText( Logger::TEAM,
                      __FILE__": (doKickWait) delaying" );

        Body_TurnToPoint( Vector2D( 0.0, 0.0 ) ).execute( agent );
        agent->setNeckAction( new Neck_ScanField() );
        return true;
    }

    if ( wm.teammatesFromBall().empty() )
    {
        agent->debugClient().addMessage( "KickIn:NoTeammate" );
        dlog.addText( Logger::TEAM,
                      __FILE__": (doKickWait) no teammate" );

        Body_TurnToPoint( Vector2D( 0.0, 0.0 ) ).execute( agent );
        agent->setNeckAction( new Neck_ScanField() );
        return true;
    }

    if ( wm.getSetPlayCount() <= (wait_is_beter ? 6 : 4) )
    {
        agent->debugClient().addMessage( "KickIn:Wait%d", wm.getSetPlayCount() );
        dlog.addText( Logger::TEAM,
                      __FILE__": (doKickWait) wait teammates" );

        Body_TurnToBall().execute( agent );
        agent->setNeckAction( new Neck_ScanField() );
        return true;
    }

    if ( wm.getSetPlayCount() >= 15
         && wm.seeTime() == wm.time()
         && wm.self().stamina() > ServerParam::i().staminaMax() * (wait_is_beter ? 0.9 : 0.7) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (dontKickWait) set play count = %d, force kick mode",
                      wm.getSetPlayCount() );
        return false;
    }

    if ( wm.seeTime() != wm.time()
         || wm.self().stamina() < ServerParam::i().staminaMax() * 0.9 )
    {
        Body_TurnToBall().execute( agent );
        agent->setNeckAction( new Neck_ScanField() );

        agent->debugClient().addMessage( "KickIn:Wait%d", wm.getSetPlayCount() );
        dlog.addText( Logger::TEAM,
                      __FILE__": (doKickWait) no see or recover" );
        return true;
    }
    dlog.addText( Logger::TEAM,
                  __FILE__": (dontKickWait) end" );
    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Bhv_SetPlayKickIn::doMove( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    Vector2D target_point = Strategy::i().getPosition( wm.self().unum() );

    const int current_len = agent->effector().getSayMessageLength();
    const int available_len = ServerParam::i().playerSayMsgSize() - current_len;
    if ( available_len >= WaitRequestMessage::slength() )
    {
        if(wm.self().stamina() < ServerParam::i().staminaMax() * 0.9)
            agent->addSayMessage( new WaitRequestMessage() );
    }

    if (Setting::i()->mStrategySetting->mMoveBeforeSetPlay){
        bool can_shoot_pass = false;

        if ( wm.ball().pos().x > 40 && wm.ball().pos().absY() < 11){
            Sector2D shoot_pass_sector = Sector2D(wm.ball().pos(),
                                                  1,
                                                  15,
                                                  (Vector2D(52,-9) - wm.ball().pos()).th(),
                                                  (Vector2D(52,+9) - wm.ball().pos()).th());
            if(wm.existTeammateIn(shoot_pass_sector,2,false)){
                can_shoot_pass = true;
            }
        }

        auto cyrus_memory = std::static_pointer_cast< CyrusAudioMemory >( agent->world().audioMemoryPtr() );

        if(cyrus_memory->startSetPlayKickTime().cycle() == wm.time().cycle())
        {
            kick_waiting_finish_check = true;
        }

        if (!kick_waiting_finish_check && !can_shoot_pass && wm.ball().pos().x > 25)
        {
            dlog.addText(Logger::ROLE , "kick waiting check = %d / --- / can shoot pass = %d / --- / waiting_cycle = %d / --- / recieved message time = %d"
                    ,kick_waiting_finish_check,can_shoot_pass,waiting_before_kick_cycle,cyrus_memory->startSetPlayKickTime().cycle());

            dlog.addCircle(Logger::ROLE , target_point ,1,0,0,0,true);
            agent->debugClient().addCircle(Circle2D(target_point, 0.5));
            auto old_target_point = target_point;
            target_point = Strategy::i().getPreSetPlayPosition(wm , waiting_before_kick_cycle);
            agent->debugClient().addCircle(Circle2D(target_point, 0.5));
            agent->debugClient().addLine(old_target_point, target_point);

            double dash_power
                    = Bhv_SetPlay::get_set_play_dash_power( agent );
            double dist_thr = wm.ball().distFromSelf() * 0.07;
            if ( dist_thr < 1.0 ) dist_thr = 1.0;

            if ( ! Body_GoToPoint( target_point,
                                   dist_thr,
                                   dash_power
            ).execute( agent ) )
            {
                // already there
                Body_TurnToAngle((old_target_point - target_point).th()).execute(agent);
//        Body_TurnToBall().execute( agent );
            }
            return;
        }

        dlog.addText(Logger::ROLE , "kick waiting check = %d / --- / can shoot pass = %d / --- / waiting_cycle = %d / --- / recieved message time = %d"
                ,kick_waiting_finish_check,can_shoot_pass,waiting_before_kick_cycle,cyrus_memory->startSetPlayKickTime().cycle());

        dlog.addCircle(Logger::ROLE , target_point ,1,0,0,0,true);
    }

    bool avoid_opponent = false;
    if ( wm.self().stamina() > ServerParam::i().staminaMax() * 0.9 )
    {
        const PlayerObject * nearest_opp = wm.getOpponentNearestToSelf( 5 );

        if ( nearest_opp
             && nearest_opp->pos().dist( target_point ) < 3.0 )
        {
            Vector2D add_vec = ( wm.ball().pos() - target_point );
            add_vec.setLength( 3.0 );

            long time_val = wm.time().cycle() % 60;
            if ( time_val < 20 )
            {

            }
            else if ( time_val < 40 )
            {
                target_point += add_vec.rotatedVector( 90.0 );
            }
            else
            {
                target_point += add_vec.rotatedVector( -90.0 );
            }

            target_point.x = min_max( - ServerParam::i().pitchHalfLength(),
                                      target_point.x,
                                      + ServerParam::i().pitchHalfLength() );
            target_point.y = min_max( - ServerParam::i().pitchHalfWidth(),
                                      target_point.y,
                                      + ServerParam::i().pitchHalfWidth() );
            avoid_opponent = true;
        }
    }

    double dash_power = Bhv_SetPlay::get_set_play_dash_power( agent );
    double dist_thr = wm.ball().distFromSelf() * 0.07;
    if ( dist_thr < 1.0 ) dist_thr = 1.0;

    agent->debugClient().addMessage( "KickInMove" );
    agent->debugClient().setTarget( target_point );

    double kicker_ball_dist = ( ! wm.teammatesFromBall().empty()
                                ? wm.teammatesFromBall().front()->distFromBall()
                                : 1000.0 );


    if ( ! Body_GoToPoint( target_point,
                           dist_thr,
                           dash_power
    ).execute( agent ) )
    {
        // already there
        if ( kicker_ball_dist > 1.0 )
        {
            agent->doTurn( 120.0 );
        }
        else
        {
            bool turn_back = false;
            if(wm.ball().pos().x > 45){
                if(wm.ball().pos().y > 0){
                    if(Strategy::i().tmPost(wm.self().unum()) == pp_rf){
                        turn_back = true;
                    }
                }else{
                    if(Strategy::i().tmPost(wm.self().unum()) == pp_lf){
                        turn_back = true;
                    }
                }
            }
            if(turn_back){
                Body_TurnToAngle(-180).execute(agent);
            }else{
                AngleDeg a1 = (wm.ball().pos() - wm.self().pos()).th() + 90;
                AngleDeg a2 = (wm.ball().pos() - wm.self().pos()).th() - 90;
                AngleDeg c = (ServerParam::i().theirTeamGoalPos() - wm.self().pos()).th();
                if((a1 - c).abs() < (a2 - c).abs()){
                    Body_TurnToAngle(a1).execute(agent);
                }else{
                    Body_TurnToAngle(a2).execute(agent);
                }

//            Body_TurnToBall().execute( agent );
        }
    }
    }

    Vector2D my_inertia = wm.self().inertiaFinalPoint();
    double wait_dist_buf = ( avoid_opponent
                             ? 10.0
                             : wm.ball().pos().dist( target_point ) * 0.2 + 6.0 );

    if ( my_inertia.dist( target_point ) > wait_dist_buf
         || wm.self().stamina() < rcsc::ServerParam::i().staminaMax() * 0.9 )
    {
        if ( ! wm.self().staminaModel().capacityIsEmpty() )
        {
            agent->debugClient().addMessage( "Sayw" );
            agent->addSayMessage( new WaitRequestMessage() );
        }
    }

    if ( kicker_ball_dist > 3.0 )
    {
        agent->setViewAction( new View_Wide() );
        agent->setNeckAction( new Neck_ScanField() );
    }
    else if ( wm.ball().distFromSelf() > 10.0
              || kicker_ball_dist > 1.0 )
    {
        agent->setNeckAction( new Neck_TurnToBallOrScan( 0 ) );
    }
    else
    {
        agent->setNeckAction( new Neck_TurnToBall() );
    }
}
