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

#include "intention_receive.h"

#include "chain_action/bhv_pass_kick_find_receiver.h"
#include <rcsc/action/body_intercept.h>

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/action/neck_scan_field.h>
#include "chain_action/bhv_strict_check_shoot.h"
#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/debug_client.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/common/audio_memory.h>
#include <rcsc/common/say_message_parser.h>

#include "neck/neck_default_intercept_neck.h"
#include "neck/neck_offensive_intercept_neck.h"
#include "sample_communication.h"
#include "bhv_basic_move.h"
#include "move_def/bhv_tackle_intercept.h"
#include "neck/next_pass_predictor.h"

using namespace rcsc;

/*-------------------------------------------------------------------*/
/*!

 */
IntentionReceive::IntentionReceive( const Vector2D & target_point,
                                    const double & dash_power,
                                    const double & buf,
                                    const int max_step,
                                    const GameTime & start_time,
                                    const int passer,
                                    const bool prepass)
    : M_target_point( target_point )
    , M_dash_power( dash_power )
    , M_buffer( buf )
    , M_step( max_step )
    , M_last_execute_time( start_time )
    , M_passer(passer)
    , M_prepass(prepass)
    , M_ex_step(1)
{

}

/*-------------------------------------------------------------------*/
/*!

 */
bool
IntentionReceive::finished(  PlayerAgent * agent )
{
    if ( M_step <= 0 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": finished. time 0" );
        return true;
    }

    if(M_prepass && M_ex_step==2){
        dlog.addText( Logger::TEAM,
                      __FILE__": prepass just 2 step in max" );
        return true;
    }

    if ( agent->world().self().isKickable() )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": already kickable" );
        return true;
    }

    if ( agent->world().existKickableTeammate() && M_ex_step >=2 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": exist kickable teammate" );
        return true;
    }

    if ( agent->world().ball().distFromSelf() < 3.0 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": finished. ball very near" );
        return true;
    }

    if ( M_last_execute_time.cycle() < agent->world().time().cycle() - 1 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": finished. strange time." );
        return true;
    }

    if ( agent->world().self().pos().dist( M_target_point ) < M_buffer )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": finished. already there." );
        return true;
    }
    if ( agent->world().audioMemory().passTime() == agent->world().time())
    {
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
IntentionReceive::execute( PlayerAgent * agent )
{
    if ( M_step <= 0 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": execute. empty intention." );
        return false;
    }

    const WorldModel & wm = agent->world();

    M_step -= 1;
    M_ex_step += 1;
    M_last_execute_time = wm.time();

    int self_min = wm.interceptTable()->selfReachCycle();
    int opp_min = wm.interceptTable()->opponentReachCycle();
    Vector2D intercept_pos = wm.ball().inertiaPoint( self_min );
    Vector2D first_ball = wm.ball().pos();
    //    Vector2D heard_pos = wm.audioMemory().pass().front().receive_pos_;
    int heard_cycle = wm.self().playerTypePtr()->cyclesToReachDistance(M_target_point.dist(wm.self().pos()));
    bool prepass_received = M_prepass;
    int sender = M_passer;
    int fastest_tm = wm.interceptTable()->fastestTeammate()->unum();
    dlog.addText( Logger::INTERCEPT,
                  __FILE__":  (intentionReceive)");

    Vector2D chain_target = Vector2D::INVALIDATED;
    if(self_min < 3){
        bool shoot_is_best = false;
        if(wm.ball().pos().dist(Vector2D(52,0)) < 20){
            Vector2D shoot_tar = Bhv_StrictCheckShoot(0).get_best_shoot(wm);
            if(shoot_tar.isValid()){
                chain_target = shoot_tar;
                shoot_is_best = true;
            }
        }
        if(!shoot_is_best){
            ActionChainHolder::instance().update( agent );
            const ActionChainGraph & chain_graph = ActionChainHolder::i().graph();
            const CooperativeAction & first_action = chain_graph.getFirstAction();
            switch (first_action.category()) {
            case CooperativeAction::Shoot: {
                chain_target = first_action.targetPoint();
                break;
            }
            case CooperativeAction::Dribble: {
                chain_target = first_action.targetPoint();
                break;
            }
            case CooperativeAction::Hold: {
                break;
            }
            case CooperativeAction::Pass: {
                chain_target = first_action.targetPoint();
                if (self_min <= 2) {
                    Bhv_PassKickFindReceiver(chain_graph).doSayPrePass(agent,first_action);
                }
                break;
            }
            }
        }
    }

    if(chain_target.isValid()){
        AngleDeg ball_move_angle = (M_target_point - first_ball).th();
        AngleDeg next_action_angle = (chain_target - first_ball).th();
        AngleDeg dif = (ball_move_angle - next_action_angle).abs();
        if(dif.degree() > 60 && dif.degree() < 130){
            chain_target = (first_ball + chain_target)/2.0;
        }else if(dif.degree() > 130){
            chain_target = first_ball;
        }else{
            chain_target = (first_ball + chain_target)/2.0;
        }
    }else{
        chain_target = first_ball;
    }


    agent->debugClient().addCircle(M_target_point,0.1);

    if(prepass_received){
        agent->debugClient().addMessage("hear PrePassMSG");
        IntentionReceive::gotoIntercept(agent,M_target_point,true);
    }else if(self_min > opp_min){
        auto self_tackle_cycle = bhv_tackle_intercept::intercept_cycle(wm);
        Vector2D ball_after_tackle_inter = wm.ball().inertiaPoint(self_tackle_cycle.first);
        if(ball_after_tackle_inter.dist(Vector2D(52.0,0)) < 20 && self_tackle_cycle.first <= opp_min && bhv_tackle_intercept().execute(agent)){
            agent->debugClient().addMessage("hear PassMSG OppCanInter->tackle");
        }else
        {
            agent->debugClient().addMessage("hear PassMSG OppCanInter");
            if(M_ex_step < 5 || M_step > 7){
                if (Body_Intercept2009(M_target_point,2,chain_target).execute( agent)){
                    agent->debugClient().addMessage("hear intercept2.0");
                }else if (Body_Intercept2009(M_target_point,4,chain_target).execute( agent)){
                    agent->debugClient().addMessage("hear intercept4.0");
                }else if (Body_Intercept2009(M_target_point,6,chain_target).execute( agent)){
                    agent->debugClient().addMessage("hear intercept6.0");
                }else{
                    IntentionReceive::gotoIntercept(agent,wm.ball().inertiaPoint(opp_min));
                }
            }
            else
                IntentionReceive::gotoIntercept(agent,wm.ball().inertiaPoint(opp_min));
        }
    }
    else if( wm.existKickableTeammate() ){
        agent->debugClient().addMessage("hear kickableTm:goto heard pos");
        agent->debugClient().addCircle(M_target_point,0.1);
        IntentionReceive::gotoIntercept(agent,M_target_point);
        agent->setNeckAction( new Neck_TurnToBall() );
    }else if (Body_Intercept2009(M_target_point,2,chain_target).execute( agent)){
        agent->debugClient().addMessage("hear intercept2.0");
    }else if (Body_Intercept2009(M_target_point,4,chain_target).execute( agent)){
        agent->debugClient().addMessage("hear intercept4.0");
    }else if (Body_Intercept2009(M_target_point,6,chain_target).execute( agent)){
        agent->debugClient().addMessage("hear intercept6.0");
    }else if (self_min > 5
              && heard_cycle > 5){
        agent->debugClient().addMessage("s,h > 5,gotoIntercept");
        IntentionReceive::gotoIntercept(agent,M_target_point);
    }else if (Body_Intercept2009(false,chain_target).execute( agent)){
        agent->debugClient().addMessage("hear intercept");
    }else{
        agent->debugClient().addMessage("hear gotoIntercept");
        IntentionReceive::gotoIntercept(agent,M_target_point);
    }


    Bhv_BasicMove().set_off_neck_with_ball(agent);
    if(prepass_received)
        SampleCommunication().saySelf(agent);

    return true;
}

bool IntentionReceive::gotoIntercept( rcsc::PlayerAgent * agent, rcsc::Vector2D target, bool prepass){
    const WorldModel & wm = agent->world();
    Vector2D selfpos = wm.self().inertiaPoint(wm.self().pos().dist(target));
    Vector2D self2ball = target - selfpos;
    AngleDeg body = wm.self().body();
    self2ball.rotate(-wm.self().body());
    bool back = false;
    if(self2ball.r() < 3 && self2ball.x < 0 && selfpos.dist(wm.ball().pos()) < 5){
        back = true;
    }
    if(back){
        if(self2ball.absY() < wm.self().playerTypePtr()->kickableArea()){
            if(selfpos.dist(target) < 2){
                agent->debugClient().addMessage("giback1");
                agent->doDash(100,(target - selfpos).th() - wm.self().body());
            }else{
                agent->debugClient().addMessage("giback2");
                agent->doDash(100,180);
            }

            return true;
        }else{
            agent->debugClient().addMessage("giback3");
            Body_TurnToAngle((target - selfpos).th() + 180).execute(agent);
            //            agent->setNeckAction( new Neck_TurnToBallOrScan() );
            //            return true;
        }
    }else{
        if(self2ball.x > 0 && self2ball.absY() < wm.self().playerTypePtr()->kickableArea()){
            if(selfpos.dist(target) < 2){
                agent->debugClient().addMessage("gi1");
                agent->doDash(100,(target - selfpos).th() - wm.self().body());
            }else{
                agent->debugClient().addMessage("gi2");
                agent->doDash(100,0);
            }
            //            agent->setNeckAction( new Neck_TurnToBallOrScan() );
            //            return true;
        }else{
            if(!Body_GoToPoint(target,0.5,100).execute(agent)){
                agent->debugClient().addMessage("gi3");
                Body_TurnToAngle((target - selfpos).th()).execute(agent);
            }else{
                agent->debugClient().addMessage("gi4");
            }

            //            agent->setNeckAction( new Neck_TurnToBallOrScan() );
            //            return true;
        }
    }
    if(agent->effector().queuedNextMyPos().x > wm.offsideLineX() && prepass){
        Body_TurnToPoint(target).execute(agent);
    }
    agent->setNeckAction( new Neck_TurnToBallOrScan() );
    return true;
}
