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

#include "bhv_basic_offensive_kick.h"

#include "basic_actions/body_advance_ball.h"
#include "basic_actions/body_dribble.h"
#include "basic_actions/body_hold_ball.h"
#include "basic_actions/body_pass.h"
#include "basic_actions/neck_scan_field.h"
#include "basic_actions/neck_turn_to_low_conf_teammate.h"
#include "basic_actions/body_smart_kick.h"
#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/sector_2d.h>

#include <vector>
using namespace rcsc;
using namespace std;
/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_BasicOffensiveKick::execute( PlayerAgent * agent )
{
    dlog.addText( Logger::TEAM,
                  __FILE__": Bhv_BasicOffensiveKick" );

    const WorldModel & wm = agent->world();

    const PlayerObject::Cont & opps = wm.opponentsFromSelf();
    const PlayerObject * nearest_opp
        = ( opps.empty()
            ? static_cast< PlayerObject * >( 0 )
            : opps.front() );
    const double nearest_opp_dist = ( nearest_opp
                                      ? nearest_opp->distFromSelf()
                                      : 1000.0 );
    const Vector2D nearest_opp_pos = ( nearest_opp
                                       ? nearest_opp->pos()
                                       : Vector2D( -1000.0, 0.0 ) );

    Vector2D pass_point;
    if ( Body_Pass::get_best_pass( wm, &pass_point, NULL, NULL ) )
    {
        if ( pass_point.x > wm.self().pos().x - 1.0 )
        {
            bool safety = true;
            for ( PlayerObject::Cont::const_iterator it = opps.begin(),
                      end = opps.end();
                  it != end;
                  ++it )
            {
                if ( (*it)->pos().dist( pass_point ) < 4.0 )
                {
                    safety = false;
                }
            }

            if ( safety )
            {
                dlog.addText( Logger::TEAM,
                              __FILE__": (execute) do best pass" );
                agent->debugClient().addMessage( "OffKickPass(1)" );
                Body_Pass().execute( agent );
                agent->setNeckAction( new Neck_TurnToLowConfTeammate() );
                return true;
            }
        }
    }

    if ( nearest_opp_dist < 7.0 )
    {
        if ( Body_Pass().execute( agent ) )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": (execute) do best pass" );
            agent->debugClient().addMessage( "OffKickPass(2)" );
            agent->setNeckAction( new Neck_TurnToLowConfTeammate() );
            return true;
        }
    }

    // dribble to my body dir
    if ( nearest_opp_dist < 5.0
         && nearest_opp_dist > ( ServerParam::i().tackleDist()
                                 + ServerParam::i().defaultPlayerSpeedMax() * 1.5 )
         && wm.self().body().abs() < 70.0 )
    {
        const Vector2D body_dir_drib_target
            = wm.self().pos()
            + Vector2D::polar2vector(5.0, wm.self().body());

        int max_dir_count = 0;
        wm.dirRangeCount( wm.self().body(), 20.0, &max_dir_count, NULL, NULL );

        if ( body_dir_drib_target.x < ServerParam::i().pitchHalfLength() - 1.0
             && body_dir_drib_target.absY() < ServerParam::i().pitchHalfWidth() - 1.0
             && max_dir_count < 3
             )
        {
            // check opponents
            // 10m, +-30 degree
            const Sector2D sector( wm.self().pos(),
                                   0.5, 10.0,
                                   wm.self().body() - 30.0,
                                   wm.self().body() + 30.0 );
            // opponent check with goalie
            if ( ! wm.existOpponentIn( sector, 10, true ) )
            {
                dlog.addText( Logger::TEAM,
                              __FILE__": (execute) dribble to my body dir" );
                agent->debugClient().addMessage( "OffKickDrib(1)" );
                Body_Dribble( body_dir_drib_target,
                              1.0,
                              ServerParam::i().maxDashPower(),
                              2
                              ).execute( agent );
                agent->setNeckAction( new Neck_TurnToLowConfTeammate() );
                return true;
            }
        }
    }

    Vector2D drib_target( 50.0, wm.self().pos().absY() );
    if ( drib_target.y < 20.0 ) drib_target.y = 20.0;
    if ( drib_target.y > 29.0 ) drib_target.y = 27.0;
    if ( wm.self().pos().y < 0.0 ) drib_target.y *= -1.0;
    const AngleDeg drib_angle = ( drib_target - wm.self().pos() ).th();

    // opponent is behind of me
    if ( nearest_opp_pos.x < wm.self().pos().x + 1.0 )
    {
        // check opponents
        // 15m, +-30 degree
        const Sector2D sector( wm.self().pos(),
                               0.5, 15.0,
                               drib_angle - 30.0,
                               drib_angle + 30.0 );
        // opponent check with goalie
        if ( ! wm.existOpponentIn( sector, 10, true ) )
        {
            const int max_dash_step
                = wm.self().playerType()
                .cyclesToReachDistance( wm.self().pos().dist( drib_target ) );
            if ( wm.self().pos().x > 35.0 )
            {
                drib_target.y *= ( 10.0 / drib_target.absY() );
            }

            dlog.addText( Logger::TEAM,
                          __FILE__": (execute) fast dribble to (%.1f, %.1f) max_step=%d",
                          drib_target.x, drib_target.y,
                          max_dash_step );
            agent->debugClient().addMessage( "OffKickDrib(2)" );
            Body_Dribble( drib_target,
                          1.0,
                          ServerParam::i().maxDashPower(),
                          std::min( 5, max_dash_step )
                          ).execute( agent );
        }
        else
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": (execute) slow dribble to (%.1f, %.1f)",
                          drib_target.x, drib_target.y );
            agent->debugClient().addMessage( "OffKickDrib(3)" );
            Body_Dribble( drib_target,
                          1.0,
                          ServerParam::i().maxDashPower(),
                          2
                          ).execute( agent );

        }
        agent->setNeckAction( new Neck_TurnToLowConfTeammate() );
        return true;
    }

    // opp is far from me
    if ( nearest_opp_dist > 5.0 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": opp far. dribble(%.1f, %.1f)",
                      drib_target.x, drib_target.y );
        agent->debugClient().addMessage( "OffKickDrib(4)" );
        Body_Dribble( drib_target,
                      1.0,
                      ServerParam::i().maxDashPower() * 0.4,
                      1
                      ).execute( agent );
        agent->setNeckAction( new Neck_TurnToLowConfTeammate() );
        return true;
    }

    // opp is near

    // can pass
    if ( Body_Pass().execute( agent ) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (execute) pass",
                      __LINE__ );
        agent->debugClient().addMessage( "OffKickPass(3)" );
        agent->setNeckAction( new Neck_TurnToLowConfTeammate() );
        return true;
    }

    // opp is far from me
    if ( nearest_opp_dist > 3.0 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (execute) opp far. dribble(%f, %f)",
                      drib_target.x, drib_target.y );
        agent->debugClient().addMessage( "OffKickDrib(5)" );
        Body_Dribble( drib_target,
                      1.0,
                      ServerParam::i().maxDashPower() * 0.2,
                      1
                      ).execute( agent );
        agent->setNeckAction( new Neck_TurnToLowConfTeammate() );
        return true;
    }

    if ( nearest_opp_dist > 2.5 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": hold" );
        agent->debugClient().addMessage( "OffKickHold" );
        Body_HoldBall().execute( agent );
        agent->setNeckAction( new Neck_TurnToLowConfTeammate() );
        return true;
    }

    {
        dlog.addText( Logger::TEAM,
                      __FILE__": clear" );
        agent->debugClient().addMessage( "OffKickAdvance" );
        Body_AdvanceBall().execute( agent );
        agent->setNeckAction( new Neck_ScanField() );
    }

    return true;

}
bool
Bhv_BasicOffensiveKick::cr_shoot( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();
    if(!wm.self().isKickable())
    	return false;
    Vector2D ball = wm.ball().pos();
    vector<Vector2D> targets;
    vector<double> dist_angle;
    for(double d=-6;d<=6;d+=0.5){
    	targets.push_back(Vector2D(52.5,d));
    	dist_angle.push_back(1000);
    }
    for(int i=0;i<25;i++){
    	Vector2D t = targets[i];
    	if(t.dist(ball) > 30)
    		continue;
    	for(int p=1;p<=11;p++){
    		const AbstractPlayerObject * opp = wm.theirPlayer(p);
    		if(opp==NULL || opp->unum()<1)
    			continue;
    		Vector2D pos = opp->pos();
    		if(pos.x<ball.x)
    			continue;
    		double shoot_angle = (t-ball).th().degree();
    		double opp_angle = (pos-ball).th().degree();
    		double dist_a = abs(shoot_angle - opp_angle);
    		if(dist_a>180)
    			dist_a = 360 - dist_a;
    		if(dist_a<dist_angle[i])
    			dist_angle[i] = dist_a;
    	}
    }
    int best_target = 0;
    double best_eval = 0;
    for(int i=0;i<25;i++){
    	if(dist_angle[i]>best_eval){
    		best_eval = dist_angle[i];
    		best_target = i;
    	}
    }
    if(best_eval>15){
    	Body_SmartKick(targets[best_target],3.0,2.7,3).execute(agent);
    	agent->setNeckAction( new Neck_ScanField() );
    	return true;
    }
    return false;
}
