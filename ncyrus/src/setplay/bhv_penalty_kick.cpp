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

#include "bhv_penalty_kick.h"
#include "../move_def/bhv_basic_tackle.h"
#include "strategy.h"

#include "goalie/bhv_goalie_chase_ball.h"
#include "goalie/bhv_goalie_basic_move.h"
#include "bhv_go_to_static_ball.h"
#include "chain_action/body_force_shoot.h"
#include "chain_action/bhv_strict_check_shoot.h"
#include "chain_action/bhv_chain_action.h"

#include <rcsc/action/body_clear_ball.h>
#include <rcsc/action/body_dribble2008.h>
#include <rcsc/action/body_dribble.h>
#include <rcsc/action/body_advance_ball.h>
#include <rcsc/action/body_intercept.h>
#include <rcsc/action/body_smart_kick.h>
#include <rcsc/action/arm_point_to_point.h>
#include <rcsc/action/basic_actions.h>
#include <rcsc/action/bhv_go_to_point_look_ball.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/body_kick_one_step.h>
#include <rcsc/action/body_stop_dash.h>
#include <rcsc/action/body_stop_ball.h>
#include <rcsc/action/body_hold_ball.h>

#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_ball_and_player.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/penalty_kick_state.h>

#include <rcsc/player/player_object.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/rect_2d.h>
#include <rcsc/geom/ray_2d.h>
#include <rcsc/soccer_math.h>
#include <rcsc/math_util.h>

#include <cmath>

#include <rcsc/action/view_wide.h>
#include <rcsc/action/view_normal.h>
#include <rcsc/action/view_change_width.h>
#include <rcsc/action/view_synch.h>

#include "../neck/neck_default_intercept_neck.h"


#include <sys/stat.h>
#include <sys/types.h>
#include <iostream>
#include <string>
#include <fstream>
#include <cstdlib>
#include <cstring>
using namespace std;
#define log 1


using namespace rcsc;

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::execute( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();
    const PenaltyKickState * state = wm.penaltyKickState();

//	const ServerParam & SP = ServerParam::i();


    int num = wm.self().unum();
    switch ( wm.gameMode().type() ) {
    case GameMode::PenaltySetup_:
        if ( state->currentTakerSide() == wm.ourSide() )
        {
            if ( state->isKickTaker( wm.ourSide(), num ) )
            {
                return doKickerSetup( agent );
            }
        }
        // their kick phase
        else
        {
            if ( wm.self().goalie() )
            {
                return doGoalieSetup( agent );
            }
        }
        break;
    case GameMode::PenaltyReady_:
        if ( state->currentTakerSide() == wm.ourSide() )
        {
            if ( state->isKickTaker( wm.ourSide(), wm.self().unum() ) )
            {
                return doKickerReady( agent );
            }
        }
        // their kick phase
        else
        {
            if ( wm.self().goalie() )
            {
                return doGoalieSetup( agent );
            }
        }
        break;
    case GameMode::PenaltyTaken_:
        if ( state->currentTakerSide() == agent->world().ourSide() )
        {
            if ( state->isKickTaker( wm.ourSide(), wm.self().unum() ) )
            {
                return doKicker( agent );
            }
        }
        // their kick phase
        else
        {
            if ( wm.self().goalie() )
            {
                return doGoalie( agent );
            }
        }
        break;
    case GameMode::PenaltyScore_:
    case GameMode::PenaltyMiss_:
        if ( state->currentTakerSide() == wm.ourSide() )
        {
            if ( wm.self().goalie() )
            {
                return doGoalieSetup( agent );
            }
        }
        break;
    case GameMode::PenaltyOnfield_:
    case GameMode::PenaltyFoul_:
        break;
    default:
        // nothing to do.
        std::cerr << "Current playmode is NOT a Penalty Shootout???" << std::endl;
        return false;
    }


    if ( wm.self().goalie() )
    {
        return doGoalieWait( agent );
    }
    else
    {
        return doKickerWait( agent );
    }

    // never reach here
    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::doKickerWait( PlayerAgent * agent )
{
#if 1


  //  double dist_step = ( 9.0 + 9.0 ) / 12;

    const rcsc::WorldModel & wm = agent->world();
    rcsc::Vector2D ball = wm.ball().pos();


//    int num = wm.self().unum();

//   Vector2D wait_pos( sign(ball.x)*7.5, (-9.8 + dist_step * gent->world().self().unum())*0.4 );
//    Vector2D wait_pos = Vector2D(sign(ball.x)*wm.self().unum()*.8,sign(wm.self().pos().y));
double angle = wm.self().unum()*15 ;
if(ball.x > 0 )angle -= 90;
else angle +=90;
    Vector2D wait_pos = Vector2D::polar2vector(8,angle);
   // if(ball.x)
//
//    if( num >= 9 || num == 11 )
//           wait_pos.x *= 0.7;
//    else if( num == 3 || num == 10 )
//           wait_pos.x *= 0.7;
//    else if( num == 4 || num == 9 )
//           wait_pos.x *= 0.8;
//    else if( num == 5 || num == 8 )
//           wait_pos.x *= 0.9;


    if(wm.self().pos().dist(Vector2D(0.0,0.0))>9.5)
    {

    	{
          if(  Body_GoToPoint( Vector2D(sign(wm.self().pos().x)*10,0.4),//Vector2D(wm.ball().pos().x,wm.ball().pos().y+(sign(wm.self().pos().y)*0.3)
                            0.3,
                            ServerParam::i().maxDashPower()
                            ).execute( agent ))
                            {
        	  cout<<"yes in "<<wm.time().cycle()<<endl;
            agent->setNeckAction( new Neck_TurnToRelative( 0.0 ) );
            return true;
                            }
    	}
    }
    // already there
     if ( agent->world().self().pos().dist( wait_pos ) < 0.7 )
    {
        Bhv_NeckBodyToBall().execute( agent );
    }
    else
    {
        // no dodge
        Body_GoToPoint( wait_pos,
                        0.3,
                        ServerParam::i().maxDashPower()
                        ).execute( agent );
        agent->setNeckAction( new Neck_TurnToRelative( 0.0 ) );
    }
#else
    const WorldModel & wm = agent->world();
    const PenaltyKickState * state = wm.penaltyKickState();

    const int taker_unum = 11 - ( ( state->ourTakerCounter() - 1 ) % 11 );
    const double circle_r = ServerParam::i().centerCircleR() - 1.0;
    const double dir_step = 360.0 / 9.0;
    //const AngleDeg base_angle = ( wm.time().cycle() % 360 ) * 4;
    const AngleDeg base_angle = wm.time().cycle() % 90;

    AngleDeg wait_angle;
    Vector2D wait_pos( 0.0, 0.0 );

    int i = 0;
    for ( int unum = 2; unum <= 11; ++unum )
    {
        if ( taker_unum == unum )
        {
            continue;
        }

        // TODO: goalie check

        if ( i >= 9 )
        {
            wait_pos.assign( 0.0, 0.0 );
            break;
        }

        if ( wm.self().unum() == unum )
        {
            wait_angle = base_angle + dir_step * i;
            if ( wm.ourSide() == RIGHT )
            {
                wait_angle += dir_step;
            }
            wait_pos = Vector2D::from_polar( circle_r, wait_angle );
            break;
        }

        ++i;
    }

    dlog.addText( Logger::TEAM,
                  __FILE__": penalty wait. count=%d pos=(%.1f %.1f) angle=%.1f",
                  i,
                  wait_pos.x, wait_pos.y,
                  wait_angle.degree() );

    Body_GoToPoint( wait_pos,
                    0.5,
                    ServerParam::i().maxDashPower()
                    ).execute( agent );
    agent->setNeckAction( new Neck_TurnToRelative( 0.0 ) );
#endif
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::doKickerSetup( PlayerAgent * agent )
{
    const Vector2D goal_c = ServerParam::i().theirTeamGoalPos();
    const PlayerObject * opp_goalie = agent->world().getTheirGoalie();
    AngleDeg place_angle = 0.0;

    // ball is close enoughly.
    if ( ! Bhv_GoToStaticBall( place_angle ).execute( agent ) )
    {
        Body_TurnToPoint( goal_c ).execute( agent );
        if ( opp_goalie )
        {
            agent->setNeckAction( new Neck_TurnToPoint( opp_goalie->pos() ) );
        }
        else
        {
            agent->setNeckAction( new Neck_TurnToPoint( goal_c ) );
        }
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::doKickerReady( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();
    const PenaltyKickState * state = wm.penaltyKickState();

    // stamina recovering...
    if ( wm.self().stamina() < ServerParam::i().staminaMax() - 10.0
         && ( wm.time().cycle() - state->time().cycle() > ServerParam::i().penReadyWait() - 3 ) )
    {
        return doKickerSetup( agent );
    }

    if ( ! wm.self().isKickable() )
    {
        return doKickerSetup( agent );
    }

    return doKicker( agent );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::doKicker( PlayerAgent * agent )
{
    //
    // server does NOT allow multiple kicks
    //

    const WorldModel & wm = agent->world();
    if ( wm.gameMode().type() != GameMode::IndFreeKick_
         && wm.time().stopped() == 0
         && wm.self().isKickable()
         && Bhv_StrictCheckShoot(0.5).execute( agent) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": shooted" );

        // reset intention
        agent->setIntention( static_cast< SoccerIntention * >( 0 ) );
        return true;
    }
    if ( wm.gameMode().type() != GameMode::IndFreeKick_
         && wm.time().stopped() == 0
         && wm.self().isKickable()
         && Bhv_StrictCheckShoot(0.2).execute( agent) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": shooted" );

        // reset intention
        agent->setIntention( static_cast< SoccerIntention * >( 0 ) );
        return true;
    }
    if ( ! ServerParam::i().penAllowMultKicks() )
    {
        return doOneKickShoot( agent );
    }

    //
    // server allows multiple kicks
    //


    rcsc::Vector2D me = wm.self().pos();
    rcsc::Vector2D ball=wm.ball().pos();
    rcsc::Vector2D Next_ball=ball+wm.ball().vel();

    /*
     * opp goalie data
     */
        const PlayerObject * opp_goalie = wm.getTheirGoalie();

        const double goalie_dist_me = ( opp_goalie
                                          ? ( opp_goalie->distFromSelf() )
                                          : 200.0 );
        const double goalie_dist_next_ball = ( opp_goalie
                                                  ? Next_ball.dist(( opp_goalie->pos()+opp_goalie->vel()) )
                                                  : 200.0 );


    int side = sign(ball.x);

    // get ball
    if ( ! wm.self().isKickable() )
    {
        if ( ! Body_Intercept().execute( agent ) )
        {
            Body_GoToPoint( wm.ball().pos(),
                            0.4,
                            ServerParam::i().maxDashPower()
                            ).execute( agent );
        }

        if ( wm.ball().posCount() > 0 )
        {
            agent->setNeckAction( new Neck_TurnToBall() );
        }
        else
        {
            const PlayerObject * opp_goalie = agent->world().getTheirGoalie();
            if ( opp_goalie )
            {
                agent->setNeckAction( new Neck_TurnToPoint( opp_goalie->pos() ) );
            }
            else
            {
                agent->setNeckAction( new Neck_TurnToPoint( ServerParam::i().theirTeamGoalPos() )  );
            }
        }
        return true;
    }

    if( wm.ball().pos().absX() < 14.0 && wm.self().unum() % 3 == 2 )
    {
    	doDribbleWhitball(agent);
    	return true;
    }
    else if( wm.ball().pos().absX() < 14.0 && wm.self().unum() % 3 == 0 )
    {

    	doDribbleWhitball(agent);
    	return true;
    }
    else if( wm.ball().pos().absX() < 14.0 && wm.self().unum() % 3 == 1 )
    {
		doDribbleWhitball(agent);
		return true;
    }
/*
 * do tackle
 */

//    if(Bhv_Shootak().PenaltiShoot(agent))
//    {
//    	cout<<" shootakkkkkkkkkkkk shoooooooooooooot in "<<wm.time().cycle()<<endl;
//    	return true;
//    }
	if(goalie_dist_next_ball < wm.getTheirGoalie()->playerTypePtr()->kickableArea()+0.3)
	{
		if(Body_TackleToPoint(ServerParam::i().theirTeamGoalPos()).execute(agent))
		{
			std::cout<<"\n Cycle: "<<wm.time().cycle()<<" do tackl to point"<<endl;
							 return true;
		}
//		if(Bhv_BasicTackle(0.8,60).execute(agent))
//		{
//			std::cout<<"\n Cycle: "<<wm.time().cycle()<<" Bhv_BasicTackle"<<endl;
//							 return true;
//		}
	}
/*
 *
 * force move
 */

    static int count=0;
    static rcsc::Vector2D S_me  = wm.self().pos();
    if(me.dist(S_me) > 1.0){
    	S_me = me = wm.self().pos();
    	count=0;
    }
    else  count++;
    if(count > 5)
    {
    	doDribble(agent);
    }
 /*
 * do force shoot
 */
    if(doforceshoot(agent))
    {
    	std::cout<<"doforceshoot in "<<wm.time().cycle()<<endl;
    	return true;
    }
    if(doShoot(agent))
    {
    	std::cout<<"do shoot in "<<wm.time().cycle()<<endl;
    	return true;
    }
	if ( opp_goalie )
	{
	      agent->setNeckAction( new Neck_TurnToPoint( opp_goalie->pos() ) );
	}
	else
	{
		agent->setNeckAction( new Neck_ScanField() );
	}
/*
 * find Y for face point to turn
 */
    rcsc::AngleDeg opp_angle=0.0;

    static double y=30;
    y=(wm.time().cycle()%5==0)?y*-1:y;
    if(opp_goalie)
    {
    	opp_angle = opp_goalie->body();
    	y=(opp_angle.degree() >= 0.0)?-34:34;
    }
/*
 * manage time to do hold ball
 */
    static int timeholde=wm.time().cycle();
    static int flag=0;
    static int cycleallow=6;

    if(flag==0)timeholde=wm.time().cycle();
    int allowtime=wm.time().cycle()-timeholde;

    Vector2D Hold_pos(side*me.x,+34.0);//wm.self().pos());
	static int switch_track=1;
	Hold_pos=getHoldPos(agent);
	rcsc::Vector2D pos1=me+
			rcsc::Vector2D::polar2vector(2.5,(me - rcsc::Vector2D(-side*52.0, me.y + switch_track*3.5)).th());

    	if(goalie_dist_me<=6.5  &&
    			goalie_dist_me>=3  &&
    			(((allowtime< cycleallow && allowtime > 1)|| goalie_dist_me < 4)
    					&& allowtime < cycleallow + 3 ))
    	{

    		//TODO Easy JAMP
    		if(allowtime > cycleallow-2 && allowtime < cycleallow )Hold_pos=rcsc::Vector2D::INVALIDATED;
    		flag=(flag==0)?1:flag;
			timeholde= (flag==0)?wm.time().cycle():timeholde;
			if(Body_HoldBall(true,pos1,Hold_pos).execute(agent))
			{
				return true;
			}
			return doDribbleWhitball(agent);
    	}
    	else
    	{
    		flag=0;
    		timeholde=wm.time().cycle();
    		return doDribbleWhitball(agent);
    	}
    return true;
}

/*
 *
 */

rcsc::Vector2D
Bhv_PenaltyKick::getHoldPos( rcsc::PlayerAgent * agent )
{
	const WorldModel &wm = agent->world();
    const ServerParam &SP = ServerParam::i();
    rcsc::Vector2D me = wm.self().pos();

    rcsc::Vector2D ball = wm.ball().pos();
    rcsc::Vector2D ball_next = ball+wm.ball().vel();
    rcsc::Vector2D goal(sign(ball.x)*SP.pitchHalfLength(),0.0);

	static rcsc::Vector2D Hold_pos(Vector2D::INVALIDATED);
	int side = sign(ball.x);

	const PlayerObject * opp_goalie = wm.getTheirGoalie();

	double goalie_dis_ball = (opp_goalie)
							 ?(opp_goalie->pos()+opp_goalie->vel()).dist(ball_next)
							:200;
	if(wm.time().cycle()%2==0)Hold_pos = rcsc::Vector2D(side*me.x,sign(Hold_pos.y)* -30.0);
	if(wm.time().cycle()%3==0)Hold_pos = rcsc::Vector2D(0.0,me.y);
	if(goalie_dis_ball < 3)
	{
		if( me.absX()-ball.absX()  > 0.2
				&& fabs(me.y - ball.y) < 0.5)
			Hold_pos = rcsc::Vector2D(side*me.x,sign(Hold_pos.y)* -30.0);
		else Hold_pos = rcsc::Vector2D(0.0,me.y);
	}
	return Hold_pos;
}
/*-------------------------------------------------------------------*/
/*!

 */

bool
Bhv_PenaltyKick::doDribbleWhitball(PlayerAgent* agent)
{
    dlog.addText(Logger::TEAM, "doDribbleWhitball");
    if(Bhv_ChainAction().execute(agent)){
        return true;
    }
    dlog.addText(Logger::TEAM, "doDribbleWhitball continue");

    const WorldModel & wm = agent->world();
    rcsc::Vector2D me = wm.self().pos();
    rcsc::Vector2D ball = wm.ball().pos();
    rcsc::Vector2D goal = ServerParam::i().theirTeamGoalPos();
    const PlayerObject * opp_goalie = wm.getTheirGoalie();
    static Vector2D myTarget = Vector2D(sign(ball.x)*50.0, 0.0);

    static int time = wm.time().cycle();
    static int flag=0;
    	rcsc::Vector2D goalie_pos = (opp_goalie)
    								?opp_goalie->pos()+opp_goalie->vel()
    								:goal;

    	double Y_dif = fabs(me.y - goalie_pos.y);
	if(agent->world().ball().pos().absX() > 15)//return doDribble(agent);
	{
	    const PlayerObject * opp_goalie = wm.getTheirGoalie();
	    /////////////////////////////////////////////////
	    flag = 0;
	    /*
	     *
	     */
	    if(opp_goalie )
	    {
	    	if(wm.self().pos().dist(opp_goalie->pos()) > 18 )
	    	{
	    		AngleDeg Target_angl=(Vector2D(sign(me.x)*52,sign(me.y)*18)-me).th();
	    		myTarget = me+Vector2D::polar2vector(10,Target_angl);//myTarget = goal;
	    		cout<<"0=>"<<myTarget<<"in "<<wm.time().cycle()<<endl;
	    	}
	    	else if(wm.self().pos().dist(opp_goalie->pos()) > 12 )
	    	{
	    		AngleDeg Target_angl=(Vector2D(sign(me.x)*52,0.0)-me).th();
	    		myTarget = me+Vector2D::polar2vector(10,Target_angl);//myTarget = goal;
	    		cout<<"0=>"<<myTarget<<"in "<<wm.time().cycle()<<endl;
	    	}
	    	else
	    	{
	    		AngleDeg Target_angl=(Vector2D(sign(me.x)*52,sign(me.y)*18)-me).th();

	    		if(ball.x > 0.0)
	    		{
	    			/*
	    			 * opp is +90 me must -90
	    			 * me.y < goal.y
	    			 */
					if(fabs(opp_goalie->body().degree() - 90.0) < 30
							&& (Y_dif < 2 || me.y < goalie_pos.y))
					{
						Target_angl = -80.0;
						if(Y_dif > 3 && me.y > goalie_pos.y)Target_angl = -45.0;
						if(me.y < goalie_pos.y)Target_angl = 90.0;
							myTarget = me+Vector2D::polar2vector(10,Target_angl);
						cout<<"a=>"<<Target_angl.degree()<<" in "<<wm.time().cycle()<<endl;
					}
					/*
					 * opp is -90 me must +90
					 */
					if(fabs(opp_goalie->body().degree() + 90.0) < 30
						&& (Y_dif < 1 || me.y > goalie_pos.y))
					{
						Target_angl = 80 ;
						//myTarget = ball+Vector2D::polar2vector(10,-80.0);
						if(Y_dif > 2 )Target_angl = 45.0 ;
							myTarget = me+Vector2D::polar2vector(10,Target_angl);
						cout<<"b=>"<<Target_angl.degree()<<" in "<<wm.time().cycle()<<endl;
					}
	    		}
	    		else
	    		{
					if(fabs(opp_goalie->body().degree() - 90.0) < 30
							&& (Y_dif < 1 || me.y < goalie_pos.y))
					{
						myTarget = me+Vector2D::polar2vector(10,-100.0);
						if(Y_dif > 2 )myTarget =  me+Vector2D::polar2vector(10,-135.0);
						cout<<"c=>"<<myTarget<<"in "<<wm.time().cycle()<<endl;
					}
					if(fabs(opp_goalie->body().degree() + 90.0) < 30
							&& (Y_dif < 1 || me.y > goalie_pos.y))
					{
						myTarget = me+Vector2D::polar2vector(10,100.0);
						if(Y_dif > 2 )myTarget = me+Vector2D::polar2vector(10,135.0);
						cout<<"d=>"<<myTarget<<"in "<<wm.time().cycle()<<endl;
					}
	    		}
	    		myTarget = me+Vector2D::polar2vector(10,Target_angl);
	    	}
	    }
	   // static time
	    static Vector2D static_pos(sign(ball.x)*52,0.0);
	    if(wm.self().pos().absY() > 15 && flag ==0 && wm.self().pos().absX()<34)
	    {
	    	time = wm.time().cycle();
	    	flag = 1;
	    	if(me.absX() < 36)
	    		myTarget = Vector2D(sign(ball.x)*52.0, -sign(wm.self().pos().y)*5);
	    	else myTarget = Vector2D(sign(ball.x)*36.0, -sign(wm.self().pos().y)*5);
	    	static_pos = myTarget;
	    	 cout<<"e=>"<<myTarget<<"in "<<wm.time().cycle()<<endl;
	    }
	    if(flag==1)
	    {
	        if(wm.time().cycle()-time < 15)
	        	myTarget = static_pos;
	        else flag = 0;
	    }
	    if(flag == 0 )time = wm.time().cycle();

//	    if(fabs(me.y - goalie_pos.y) > 4 && fabs(me.x - goalie_pos.x) > 4)
//	    {
//	    	myTarget = Vector2D(sign(me.x)*52,sign(me.y)*7.0);
//	    }
	    if(goalie_pos.absX() >36.0 && me.dist(goalie_pos) > 12)
	    {
	    	myTarget = goal;
	    	myTarget.y = sign(me.y)*12;
	    }
	    Triangle2D t1(me,Vector2D(sign(me.x)*8,10),Vector2D(sign(me.x)*8,10));
	    if((!t1.contains(goalie_pos) && goalie_pos.absY() > me.absY())
	    		||goalie_pos.dist(goal) > me.dist(goal))
	    {
	    	myTarget = goal;
	    }
	    if(opp_goalie->posCount() > 3
	    		|| Y_dif > 4.8)
	    {
	    	myTarget = goal;
	    }
	    static double y=30;
	    y=(wm.time().cycle()%2==0)?y*-1:y;
	    if(wm.getTheirGoalie())
	    {
	    	if(wm.ball().pos().x > 0.0)
	    	   y=(wm.getTheirGoalie()->body().degree()> 30)?-30:30;
	    	if(wm.ball().pos().x < 0.0)
	    	   y=(wm.getTheirGoalie()->body().degree()< -30)?30:-30;
	    }
	//    	if(myTarget.dist(me) > 12)



	//    myTarget =goalie_pos;// Vector2D(sign(wm.ball().pos().x)*52,0);


//			if(Body_Dribble( myTarget,
//						  1,
//						  ServerParam::i().maxDashPower(),
//						  4,
//						  true
//						  ).execute( agent )
//			)

        if ( Bhv_ChainAction().execute( agent ) )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": (execute) do chain action" );
            agent->debugClient().addMessage( "ChainAction" );
            return true;
        }


	}

	  const rcsc::Vector2D Target =getDriblePos(agent);
	    Vector2D ball_move = Target - wm.ball().pos();
	    Vector2D kick_accel = ball_move - wm.ball().vel();
	    double kick_power = kick_accel.r() / wm.self().kickRate();
	//    double diftime = wm.time().cycle() - wm.penaltyKickState()->time().cycle();
		if(kick_power > 0.8)kick_power=0.8;
		if(kick_power < 0.4)
			kick_power = 0.4;
		if(Target.dist(me) > 10 && Target.dist(goalie_pos) > 5)
			kick_power = 0.9;
	    if(wm.getTheirGoalie())
	    {
	    	if(wm.self().pos().dist(wm.getTheirGoalie()->pos())> 22)
	    	{
	    		kick_power = 1.14;
	    	}
	    	if(wm.self().pos().dist(wm.getTheirGoalie()->pos())< 10)
	    	{
	    		if(kick_power > 0.65)kick_power=0.65;
	    	}
	    	if(wm.self().pos().dist(wm.getTheirGoalie()->pos())< 7)
	    	{
	    		if(kick_power > 0.55)kick_power=0.55;
	    	}
	    }
	    cout<<"kick power in "<<wm.time().cycle()<<" is "<<kick_power<<endl;
	    agent->setNeckAction( new  Neck_TurnToPoint( goalie_pos ));
	    if(!wm.ball().vel().isValid())
	    {

	    	if( Body_StopBall().execute(agent))

	    		return true;
	    }
		 if (rcsc::Body_KickOneStep( Target,kick_power ).execute( agent ))
		 return true;
		 return false;
}

/*
 *
 */
rcsc::Vector2D
Bhv_PenaltyKick::getDriblePos( rcsc::PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();
    const ServerParam &SP = ServerParam::i();
    rcsc::Vector2D me = wm.self().pos();
    rcsc::Vector2D me_next = me+wm.self().vel();
    rcsc::Vector2D ball = wm.ball().pos();
    int side = sign(ball.x);
    rcsc::Vector2D goal(side*SP.pitchHalfLength(),0.0);

	rcsc::Vector2D Target = goal;
    if(ball.absX() < 20)
    {
    	if(wm.self().unum()%3==2)
    		return Vector2D(sign(ball.x)*20,6);//6 bud
    	else  if(wm.self().unum()%3==0)
     		return Vector2D(sign(ball.x)*20,-6);
    	else return Vector2D(sign(ball.x)*20,5.0);

    }

    const PlayerObject * opp_goalie = wm.getTheirGoalie();

	double goalie_dis_me = (opp_goalie)
							?(opp_goalie->pos()+opp_goalie->vel()).dist(me_next)
							:200;
	rcsc::AngleDeg goalie_body = (opp_goalie)
								 ?opp_goalie->body()
								:wm.self().body();
	rcsc::Vector2D goalie_pos = (opp_goalie)
								?opp_goalie->pos()+opp_goalie->vel()
								:goal;
	double Y_dif = (me.y - goalie_pos.y);
	rcsc::AngleDeg Target_angel=(goal - me).th();
	/*
	 * if Im near to opp
	 */

	if(fabs(Y_dif) < 2 && goalie_dis_me < 8)
	if(fabs(goalie_body.degree() - wm.self().body().degree() ) > 45)
	{
		Target_angel = (wm.self().body().degree() > 0.0)?90:-90;
	}
	if(fabs(Y_dif) >= 2 && goalie_dis_me < 8)
	{
		Target_angel = (Y_dif < 0.0)?-90:90;
		//if(ball.x<0.0)Target_angel += -90;
	}

	Target = me + rcsc::Vector2D::polar2vector(3.0,Target_angel);
	return Target;
}
/*
 * force shoot
 */
bool
Bhv_PenaltyKick::doforceshoot(PlayerAgent * agent)
{
    const WorldModel & wm = agent->world();
    const ServerParam &SP = ServerParam::i();
    rcsc::Vector2D me = wm.self().pos();
    rcsc::Vector2D me_next = me+wm.self().vel();
    rcsc::Vector2D ball = wm.ball().pos();
    rcsc::Vector2D ball_next = ball+wm.ball().vel();
    rcsc::Vector2D goal(sign(ball.x)*SP.pitchHalfLength(),0.0);
    if(!wm.self().isKickable())
    	return false;


    /*
     * end time
     */
	if ( wm.time().cycle() - wm.penaltyKickState()->time().cycle()
			> SP.penTakenWait() - 30)
	{
	        std::cout<<"\n Cycle: "<<wm.time().cycle()<<" force doOneKickShoot"<<endl;
	        return doOneKickShoot( agent );
	}

	const PlayerObject * opp_goalie  = wm.getTheirGoalie();
	double goalie_dis_me = (opp_goalie)
							?(opp_goalie->pos()+opp_goalie->vel()).dist(me_next)
							:200;
	double goalie_dis_ball = (opp_goalie)
							 ?(opp_goalie->pos()+opp_goalie->vel()).dist(ball_next)
							:200;
	double goalie_dis_goal = (opp_goalie)
							 ?(opp_goalie->pos()+opp_goalie->vel()).dist(goal)
							:200;
	rcsc::AngleDeg goalie_body = (opp_goalie)
								 ?opp_goalie->body()
								:wm.self().body();
	rcsc::Vector2D goalie_pos = (opp_goalie)
								?opp_goalie->pos()+opp_goalie->vel()
								:goal;


    Triangle2D t3 = Triangle2D(me,goal,Vector2D(sign(me.x)*52,8));
    Triangle2D t2 = Triangle2D(me,goal,Vector2D(sign(me.x)*52,-8));
    if((me.absY() < goalie_pos.absY()
    		&& fabs(me.absY() - goalie_pos.absY())>=1.2
    		&&me.absX() > 34 ) || ball.dist(goalie_pos) < 1.6)
    {
    	rcsc::Vector2D target=goal;
		if(!t3.contains(goalie_pos))
		{
			target = Vector2D(sign(me.x)*52,5.5);
		}
		else if(!t2.contains(goalie_pos))
		{
			target = Vector2D(sign(me.x)*52,-5.5);
		}
    	rcsc::Vector2D vel= KickTable::calc_max_velocity( ( target- wm.ball().pos() ).th(),
    	                                        wm.self().kickRate(),
    	                                        wm.ball().vel() );
    	if(vel.r() > 1.8)
		if(rcsc::Body_KickOneStep(target ,vel.r()+0.5).execute( agent ))
		{
			std::cout<<"\n Cycle: "<<wm.time().cycle()<<" triangle"<<endl;
				 return true;
		}
    	if(ball.dist(goalie_pos) < 1.6)
    	{
    		if(Body_TackleToPoint(target).execute(agent))
    		{
    			std::cout<<"\n Cycle: "<<wm.time().cycle()<<" do tackl to point"<<endl;
    							 return true;
    		}
            if(Bhv_BasicTackle(0.75).execute(agent))
    		{
    			std::cout<<"\n Cycle: "<<wm.time().cycle()<<" Bhv_BasicTackle"<<endl;
    							 return true;
    		}

//    		if(rcsc::Body_KickOneStep(target ,3.0).execute( agent ))
//    		{
//    			std::cout<<"\n Cycle: "<<wm.time().cycle()<<" triangle"<<endl;
//    							 return true;
//    		}
    	}
    }


    /*
     * shoot 2013
     */

	double min_dist = SP.catchableArea()+1.0;
	double Y_dif = fabs(me.y - goalie_pos.y);
	double X_dif = fabs(me.x - goalie_pos.x);
	/*
	 * opp goalie is very near
	 */
	if(goalie_dis_ball < min_dist)
	{
    	if(Body_SmartKick( goal,
                    SP.ballSpeedMax(),
                    SP.ballSpeedMax() * 0.94,
                    1 ).execute( agent ))
    	{
    		agent->setNeckAction( new Neck_TurnToPoint( goal ) );
    		std::cout<<"\n Cycle: "<<wm.time().cycle()<<" force Body_SmartKick"<<endl;
    		return true;
    	}
  }

	if(wm.self().pos().absX() < 20)return false;
	rcsc::Vector2D goal1(sign(ball.x),-10);
	rcsc::Vector2D goal2(sign(ball.x),10);
	Triangle2D t1(me,goal1,goal2);

	int score=0;
	if(goalie_dis_goal < me.dist(goal)
			&& goalie_pos.x - me.x >10)
		score = -90;
	if(me.y>goalie_pos.y && goalie_body.degree()<=180 )
	{
		if(goalie_body.degree() < -30 && goalie_body.degree() > -150)
		{
			score += 10;
		}
	}
	if(me.y < goalie_pos.y && goalie_body.degree()<=180 )
	{
		if(goalie_body.degree() > 30 && goalie_body.degree() < 150)
		{
			score += 10;
		}
	}
	if(!t1.contains(goalie_pos))
		score+=20;
	if(me.dist(goal) < goalie_dis_goal && me.dist(goal) < 35)
	{
		score+=100;
	}
	if(opp_goalie->isTackling())
		score+=10;

/*
 * pos
 */
	if(goalie_dis_me < 6 &&  (Y_dif - X_dif) > X_dif / 2)
		score+=15;
	if(goalie_dis_me < 6 &&  (Y_dif > 4) > X_dif / 2)
		score+=15;
/*
 * check fase
 */
	if(opp_goalie)
	{
		if(opp_goalie->faceCount()==0)
		{
			//Line2D faceline
			if(opp_goalie->face().degree() > -45.0
					&& opp_goalie->face().degree() < 45.0
					&& ball.x > 0.0)
			{
				score+=20;
			}
			if(opp_goalie->face().degree() < -135.0
					&& opp_goalie->face().degree() < 135
					&& ball.x < 0.0)
			{
				score+=20;
			}

		}

	}
	rcsc::Vector2D Target(sign(ball.x)*52,0.0);
	rcsc::Vector2D vel= KickTable::calc_max_velocity( ( Target- wm.ball().pos() ).th(),
	                                        wm.self().kickRate(),
	                                        wm.ball().vel() );
	if(score >35 && vel.r() >2.0)
	{
		if(me.y>goalie_pos.y)Target.y=6;
		else Target.y = -6.0;

		if(rcsc::Body_KickOneStep( Target,2.8).execute( agent ))
		{
			std::cout<<"\n Cycle: "<<wm.time().cycle()<<" scor kick["<< score<<"]"<<endl;
			return true;
		}
		if(Body_SmartKick( Target,
		                    SP.ballSpeedMax(),
		                    SP.ballSpeedMax() * 0.90,
		                    1 ).execute( agent ))
		 {
		    		agent->setNeckAction( new Neck_TurnToPoint( goal ) );
		    		std::cout<<"\n Cycle: "<<wm.time().cycle()<<" score Body_SmartKick["<<score<<"]"<<endl;
		    		return true;
		 }


	}
	return false;
}
/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::doOneKickShoot( PlayerAgent* agent )
{
    const double ball_speed = agent->world().ball().vel().r();
    // ball is moveng --> kick has taken.
    if ( ! ServerParam::i().penAllowMultKicks()
         && ball_speed > 0.3 )
    {
        return false;
    }

    // go to the ball side
    if ( ! agent->world().self().isKickable() )
    {
        Body_GoToPoint( agent->world().ball().pos(),
                        0.4,
                        ServerParam::i().maxDashPower()
                        ).execute( agent );
        agent->setNeckAction( new Neck_TurnToBall() );
        return true;
    }

    // turn to the ball to get the maximal kick rate.
    if ( ( agent->world().ball().angleFromSelf() - agent->world().self().body() ).abs()
         > 3.0 )
    {
        Body_TurnToBall().execute( agent );
        const PlayerObject * opp_goalie = agent->world().getTheirGoalie();
        if ( opp_goalie )
        {
            agent->setNeckAction( new Neck_TurnToPoint( opp_goalie->pos() ) );
        }
        else
        {
            Vector2D goal_c = ServerParam::i().theirTeamGoalPos();
            agent->setNeckAction( new Neck_TurnToPoint( goal_c ) );
        }
        return true;
    }

    // decide shot target point
    Vector2D shoot_point = ServerParam::i().theirTeamGoalPos();

    const PlayerObject * opp_goalie = agent->world().getTheirGoalie();
    if ( opp_goalie )
    {
        shoot_point.y = ServerParam::i().goalHalfWidth() - 1.0;
        if ( opp_goalie->pos().absY() > 0.5 )
        {
            if ( opp_goalie->pos().y > 0.0 )
            {
                shoot_point.y *= -1.0;
            }
        }
        else if ( opp_goalie->bodyCount() < 2 )
        {
            if ( opp_goalie->body().degree() > 0.0 )
            {
                shoot_point.y *= -1.0;
            }
        }
    }

    // enforce one step kick
    Body_KickOneStep( shoot_point,
                      ServerParam::i().ballSpeedMax()
                      ).execute( agent );

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::doShoot( PlayerAgent * agent )
{
//	return false;
    const WorldModel & wm = agent->world();
    const ServerParam & SP = ServerParam::i();
    Vector2D me = wm.self().pos();
    Vector2D ball = wm.ball().pos();
    Vector2D goal(rcsc::sign(ball.x)*52.5,0.0);
    const PlayerObject * opp_goalie = wm.getTheirGoalie();
    Vector2D goalie = opp_goalie?
			opp_goalie->pos()+opp_goalie->vel():
			Vector2D(sign(ball.x)*52,0.0);



    rcsc::Vector2D Target = goal;
    Target.y=(me.y > goalie.y )?-4.5:4.5;

    if(me.dist(goalie)<3.8)
    {
    	if(Body_SmartKick( Target,
                    SP.ballSpeedMax(),
                    SP.ballSpeedMax() * 0.90,
                    2 ).execute( agent ))
    	{
    		agent->setNeckAction( new Neck_TurnToPoint( goal ) );
    		std::cout<<"\n Cycle: "<<wm.time().cycle()<<" shoot avali"<<endl;
    		return true;
    	}
    }
    Triangle2D t1(me,Vector2D(sign(ball.x)*52,9),Vector2D(sign(ball.x)*52,-9));

      if(me.dist(goalie) >4 )
      {
        return false;
      }
      if(me.dist(goal)>35)return false;

    Vector2D shot_point;
    double shot_speed;

    if ( getShootTarget( agent, &shot_point, &shot_speed ) )
    {
     if( wm.getTheirGoalie()->posCount() < 3 )
     {
        dlog.addText( Logger::TEAM,
                      __FILE__" (doShoot) shoot to (%.1f %.1f) speed=%f",
                      shot_point.x, shot_point.y,
                      shot_speed );

        if(Body_SmartKick( shot_point,
                        shot_speed,
                        shot_speed * 0.96,
                        2 ).execute( agent ))
        {
        	std::cout<<"\n Cycle: "<<wm.time().cycle()<<" shoot dovomi"<<endl;
        	agent->setNeckAction( new Neck_TurnToPoint( shot_point ) );
        	return true;
        }
    }

    }
    return false;
}
/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::getShootTarget( const PlayerAgent * agent,
                                 Vector2D * point,
                                 double * first_speed )
{
    const WorldModel & wm = agent->world();
    const ServerParam & SP = ServerParam::i();

    if ( SP.theirTeamGoalPos().dist2( wm.ball().pos() ) > std::pow( 35.0, 2 ) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__" (getShootTarget) too far" );
        return false;
    }

    const PlayerObject * opp_goalie = wm.getTheirGoalie();

    // goalie is not found.
    if ( ! opp_goalie )
    {
        Vector2D shot_c = SP.theirTeamGoalPos();
        if ( point ) *point = shot_c;
        if ( first_speed ) *first_speed = SP.ballSpeedMax();

        dlog.addText( Logger::TEAM,
                      __FILE__" (getShootTarget) no goalie" );
        return true;
    }

    int best_l_or_r = 0;
    double best_speed = SP.ballSpeedMax() + 1.0;

    double post_buf = 1.0
        + std::min( 2.0,
                    ( SP.pitchHalfLength() - wm.self().pos().absX() ) * 0.1 );

    // consder only 2 angle
    Vector2D shot_l( SP.pitchHalfLength(), -SP.goalHalfWidth() + post_buf );
    Vector2D shot_r( SP.pitchHalfLength(), +SP.goalHalfWidth() - post_buf );

    const AngleDeg angle_l = ( shot_l - wm.ball().pos() ).th();
    const AngleDeg angle_r = ( shot_r - wm.ball().pos() ).th();

    // !!! Magic Number !!!
    const double goalie_max_speed = 1.0;
    // default player speed max * conf decay
    const double goalie_dist_buf
        = goalie_max_speed * std::min( 5, opp_goalie->posCount() )
        + SP.catchAreaLength()
        + 0.2;

    const Vector2D goalie_next_pos = opp_goalie->pos() + opp_goalie->vel();

    for ( int i = 0; i < 2; i++ )
    {
        const Vector2D target = ( i == 0 ? shot_l : shot_r );
        const AngleDeg angle = ( i == 0 ? angle_l : angle_r );

        double dist2goal = wm.ball().pos().dist( target );

        // set minimum speed to reach the goal line
        double tmp_first_speed =  ( dist2goal + 5.0 ) * ( 1.0 - SP.ballDecay() );
        tmp_first_speed = std::max( 1.2, tmp_first_speed );

        bool over_max = false;
        while ( ! over_max )
        {
            if ( tmp_first_speed > SP.ballSpeedMax() )
            {
                over_max = true;
                tmp_first_speed = SP.ballSpeedMax();
            }

            Vector2D ball_pos = wm.ball().pos();
            Vector2D ball_vel = Vector2D::polar2vector( tmp_first_speed, angle );
            ball_pos += ball_vel;
            ball_vel *= SP.ballDecay();

            bool goalie_can_reach = false;

            // goalie move at first step is ignored (cycle is set to ZERO),
            // because goalie must look the ball velocity before chasing action.
            double cycle = 0.0;
            while ( ball_pos.absX() < SP.pitchHalfLength() )
            {
                if ( goalie_next_pos.dist( ball_pos )
                     < goalie_max_speed * cycle + goalie_dist_buf )
                {
                    dlog.addText( Logger::TEAM,
                                  __FILE__" (getShootTarget) goalie can reach. cycle=%.0f"
                                  " target=(%.1f, %.1f) speed=%.1f",
                                  cycle + 1.0, target.x, target.y, tmp_first_speed );
                    goalie_can_reach = true;
                    break;
                }

                ball_pos += ball_vel;
                ball_vel *= SP.ballDecay();
                cycle += 1.0;
            }

            if ( ! goalie_can_reach )
            {
                dlog.addText( Logger::TEAM,
                              __FILE__" (getShootTarget) goalie never reach. target=(%.1f, %.1f) speed=%.1f",
                              target.x, target.y,
                              tmp_first_speed );
                if ( tmp_first_speed < best_speed )
                {
                    best_l_or_r = i;
                    best_speed = tmp_first_speed;
                }
                break; // end of this angle
            }
            tmp_first_speed += 0.4;
        }
    }


    if ( best_speed <= SP.ballSpeedMax() )
    {
        if ( point )
        {
            *point = ( best_l_or_r == 0 ? shot_l : shot_r );
        }
        if ( first_speed )
        {
            *first_speed = best_speed;
        }

        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!
  dribble to the shootable point
*/
bool
Bhv_PenaltyKick::doDribble( PlayerAgent * agent )
{

    const WorldModel & wm = agent->world();

    const PlayerObject * opp_goalie = wm.getTheirGoalie();


    /////////////////////////////////////////////////
    // dribble parametors

    static Vector2D myTarget = Vector2D(sign(wm.ball().pos().x)*50.0, sign(wm.self().pos().x)*10);
    if(wm.self().pos().x > 20)
    	 myTarget = Vector2D(sign(wm.ball().pos().x)*52.0,sign(wm.self().pos().x)*30);
    if(wm.self().pos().dist(Vector2D(sign(wm.ball().pos().x)*36.0, 0.0))<7)
    	myTarget = Vector2D(sign(wm.ball().pos().x)*50.0,sign(wm.self().pos().x)*30);


    static double y=30;
    y=(wm.time().cycle()%2==0)?y*-1:y;
    if(wm.getTheirGoalie())
    {
    	if(wm.ball().pos().x > 0.0)
    	y=(wm.getTheirGoalie()->body().degree()> 30)?-30:30;
    	if(wm.ball().pos().x < 0.0)
    	   y=(wm.getTheirGoalie()->body().degree()< -30)?30:-30;
    }

    Body_Dribble( myTarget,
                  1.0,
                  ServerParam::i().maxDashPower(),
                  4,
                  true
                  ).execute( agent );

    if ( opp_goalie )
    {
        agent->setNeckAction( new Neck_TurnToPoint( opp_goalie->pos() ) );
    }
    else
    {
        agent->setNeckAction( new Neck_ScanField() );
    }
    return true;

}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::doGoalieWait( PlayerAgent* agent )
{
#if 0
    Vector2D wait_pos( - ServerParam::i().pitchHalfLength() - 2.0, -25.0 );

    if ( agent->world().self().pos().absX()
         > ServerParam::i().pitchHalfLength()
         && wait_pos.y * agent->world().self().pos().y < 0.0 )
    {
        wait_pos.y *= -1.0;
    }

    double dash_power = ServerParam::i().maxDashPower();

    if ( agent->world().self().stamina()
         < ServerParam::i().staminaMax() * 0.8 )
    {
        dash_power *= 0.2;
    }
    else
    {
        dash_power *= ( 0.2 + ( ( agent->world().self().stamina()
                                  / ServerParam::i().staminaMax() ) - 0.8 ) / 0.2 * 0.8 );
    }

    Vector2D face_point( wait_pos.x * 0.5, 0.0 );
    if ( agent->world().self().pos().dist2( wait_pos ) < 1.0 )
    {
        Body_TurnToPoint( face_point ).execute( agent );
        agent->setNeckAction( new Neck_TurnToBall() );
    }
    else
    {
        Body_GoToPoint( wait_pos,
                        0.5,
                        dash_power
                        ).execute( agent );
        agent->setNeckAction( new Neck_TurnToPoint( face_point ) );
    }
#else
    Body_TurnToBall().execute( agent );
    agent->setNeckAction( new Neck_TurnToBall() );
#endif
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::doGoalieSetup( PlayerAgent * agent )
{
    Vector2D move_point( ServerParam::i().ourTeamGoalLineX() + ServerParam::i().penMaxGoalieDistX() - 0.1,
                         0.0 );

    if ( Body_GoToPoint( move_point,
                         0.5,
                         ServerParam::i().maxDashPower()
                         ).execute( agent ) )
    {
        agent->setNeckAction( new Neck_TurnToBall() );
        return true;
    }

    // already there
    if ( std::fabs( agent->world().self().body().abs() ) > 2.0 )
    {
        Vector2D face_point( 0.0, 0.0 );
        Body_TurnToPoint( face_point ).execute( agent );
    }

    agent->setNeckAction( new Neck_TurnToBall() );

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */

bool
Bhv_PenaltyKick::doGoalie( PlayerAgent* agent )
{

	char filenam[512];
	static int m=  mkdir("..//..//log", 0777);
	if(m)m=0;
	sprintf(filenam,"..//..//log//Penalti.txt");
	ofstream log_writetofile;//(filenam,ios::app);
    const rcsc::WorldModel & wm = agent->world();



    const ServerParam & SP = ServerParam::i();
    rcsc::Vector2D ball = wm.ball().pos();
    rcsc::Vector2D Next_ball = wm.ball().pos()+wm.ball().vel();
    rcsc::Vector2D me = wm.self().pos();
    rcsc::Vector2D me_next = wm.self().pos()+wm.self().vel();
    rcsc::Vector2D goalcenter(sign(ball.x)*52.0,0.0);

   // Arm_PointToPoint(me + Vector2D::polar2vector(.8,(goalcenter-me).th())).execute(agent);
//    agent->setNeckAction( new rcsc::Neck_TurnToBall() );
//    return doMove(agent,Vector2D(sign(ball.x)*52,-15),0.5);
//    static int count = 1;
//    if(wm.time().cycle() - wm.penaltyKickState()->time().cycle() <=1)count =1;
//	if(rcsc::Body_TurnToAngle(150).execute( agent ) && count ==1)
//				{
//		count =0;
//					agent->setNeckAction( new rcsc::Neck_TurnToBall() );
//					std::cout<<"turn body to "<<150<< " angle in pos "<<wm.time().cycle()<<endl;
//					return true;
//				}

//   return doMove(agent,Vector2D(sign(wm.ball().pos().x)*50,15),0.5);
//	if( doSidedash(agent,Vector2D(sign(wm.ball().pos().x)*52.0,10),0.5))
//	{
//		agent->setNeckAction( new rcsc::Neck_TurnToBall());
//		std::cout<<"doMove In "<<wm.time().cycle()<<endl;
//		return true;
//	}
//return false;
/////////////////////////////////////////////////////////////////////////////


    const PlayerPtrCont & opps = wm.opponentsFromSelf();
    const PlayerObject * nearest_opp
        = ( opps.empty()
            ? static_cast< PlayerObject * >( 0 )
            : opps.front() );
    const double nearest_opp_dist = ( nearest_opp
                                      ? nearest_opp->distFromSelf()
                                      : 1000.0 );
//////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////
#if log
	log_writetofile.open(filenam,ios::app);
	log_writetofile<<endl<<endl<<"cycle = "<<wm.time().cycle();
	log_writetofile<<" time>>>"<< wm.penaltyKickState()->time()<<endl;
#endif
    /////////////////////////////////////////////////////////////////
    // check if catchabale
    Rect2D our_penalty( Vector2D( -SP.pitchHalfLength(),
                                  -SP.penaltyAreaHalfWidth() + 1.0 ),
                        Size2D( SP.penaltyAreaLength() - 1.0,
                                SP.penaltyAreaWidth() - 2.0 ) );

    if ( wm.ball().distFromSelf() < SP.catchableArea() - 0.05
         && our_penalty.contains( wm.ball().pos() ) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": goalie try to catch" );
#if log
        log_writetofile<<"docatch"<<endl;
        std::cout<<"catch"<<wm.time().cycle()<<endl;
#endif

        return agent->doCatch();
    }

    if ( wm.self().isKickable() )
    {
        Body_ClearBall().execute( agent );
        agent->setNeckAction( new Neck_TurnToBall() );
#if log
        log_writetofile<<"clear ball"<<endl;
        std::cout<<"clear "<<wm.time().cycle()<<endl;
#endif

        return true;
    }
    if ( Bhv_BasicTackle( 0.8 ).execute( agent ) )
    {
#if log
    	std::cout<<" Tackle "<<wm.time().cycle()<<endl;
        log_writetofile<<"tackle"<<endl;
#endif
        return true;
    }
    if( Bhv_BasicTackle( 0.6 ).execute( agent ) )
    {
    	std::cout<<" tackle 2 in "<<wm.time().cycle()<<endl;
        return true;
    }

    if(me_next.dist(Next_ball) < wm.self().playerType().kickableArea()
    		|| Next_ball.dist(me)<wm.self().playerType().kickableArea()+0.8)
    {
    	agent->setNeckAction( new rcsc::Neck_TurnToBall());

    	if(fabs((me_next - me).th().degree()-wm.self().body().degree())<15)
    	{
    		agent->doDash(SP.maxDashPower());
    		return true;
    	}
    	else if(fabs((me_next - me).th().degree()-wm.self().body().degree())>165)
    	{
    		agent->doDash(-SP.maxDashPower());
    		return true;
    	}
    }
    int myCycles = wm.interceptTable()->selfReachCycle();
    int oppCycles = wm.interceptTable()->opponentReachCycle();

    if( wm.ball().vel().r() > 2.0 && (ball+wm.ball().vel()).dist(me+wm.self().vel()) < 1.9 )
     { // for tackle
     		agent->setNeckAction( new rcsc::Neck_TurnToBall());
               rcsc::Body_TurnToPoint(ball+wm.ball().vel()).execute( agent );
               {
             	  std::cout<<"turn for tackl In "<<wm.time().cycle()<<endl;
             	  return true;
               }
      }
       else if( wm.ball().vel().r() > 1.8 && oppCycles > 15 )
      {
     	  agent->setNeckAction( new rcsc::Neck_TurnToBall());
            rcsc::Body_Intercept().execute( agent );
            std::cout<<"shoot intercept "<<wm.time().cycle()<<endl;
            return true;
      }


    if( wm.ball().posCount() > 3)
    {
//    		if( doMove(agent,Vector2D(sign(ball.x)*52.0,0.0),1))
//    		{
//    			agent->setNeckAction( new rcsc::Neck_TurnToBall());
//    			std::cout<<"doMove In "<<wm.time().cycle()<<endl;
//    			return true;
//    		}

		if(rcsc::Body_TurnToBall().execute( agent ))
		{
			agent->setNeckAction( new rcsc::Neck_TurnToBall() );
			std::cout<<"turn body to ball "<<endl;//<<target_angle.degree()<< " angle in pos "<<wm.time().cycle()<<endl;
			return true;
		}
     }


        if(( myCycles  <= oppCycles && wm.ball().posCount()<=1)||wm.ball().vel().r()>2.1)
        {
           rcsc::Body_Intercept().execute( agent );
           agent->setNeckAction( new rcsc::Neck_TurnToBall() );
     //      std::cout<<"\nCYCLE: "<<wm.time().cycle()<<" Intercept Avali\n";
    #if log
           std::cout<<"Intercept 1 in"<<wm.time().cycle()<<endl;
            log_writetofile<<"Intercept Avali"<<endl;
    #endif
           return true;
        }



         bool shoott = wm.ball().vel().r()> 2.0;
     static bool dointercept=false;
    if(wm.existKickableOpponent())
    {
    	dointercept=false;
    	shoott=false;
    }
    if(shoott || dointercept ||me.dist(ball)<2.5)
    {
    	dointercept = true;
    	agent->setNeckAction( new rcsc::Neck_TurnToBall() );
        rcsc::Body_Intercept(false,wm.ball().pos()+wm.ball().vel()).execute( agent );

 #if log
        std::cout<<"Intercept 2 In "<<wm.time().cycle()<<endl;
         log_writetofile<<"Intercept Dovomi"<<endl;
 #endif
        return true;
    }

    if(!wm.existKickableOpponent() && ball.dist(goalcenter) > Next_ball.dist(goalcenter)
    		&& Next_ball.dist(goalcenter) < me.dist(goalcenter))
    {
    	dointercept = true;
       	agent->setNeckAction( new rcsc::Neck_TurnToBall() );
        rcsc::Body_Intercept(false).execute( agent );

     #if log
            std::cout<<"Intercept 3 In "<<wm.time().cycle()<<endl;
             log_writetofile<<"Intercept Dovomi"<<endl;
     #endif
    }



    const PlayerObject * opp1 = wm.interceptTable()->fastestOpponent();

//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
    //neck action
    agent->setNeckAction( new rcsc::Neck_TurnToBall() );
    ViewAction * view = static_cast< ViewAction * >( 0 );
	static Vector2D static_ball_pos = wm.ball().pos()+wm.ball().vel();
    if(wm.ball().posCount()<=1)
    {
    	if(opp1)
    		static_ball_pos=wm.ball().pos()+opp1->vel();
    	else static_ball_pos=wm.ball().pos()+wm.ball().vel();
    }
    if ( opp1 )
    {
    	if(wm.time().cycle()-wm.penaltyKickState()->time().cycle() < 5)
    	{
    	  	view= new View_Synch();
    	  	agent->setViewAction(view);
    	   	if(! Neck_TurnToPoint(Vector2D(0.0,0.0) ).execute(agent))
    	   	{
    	   		agent->setNeckAction( new rcsc::Neck_TurnToPoint(Vector2D(0.0,0.0)) );
    	  	}
    	}
        if(wm.ball().posCount()>=4)
        {
        	view = new View_Wide();
        	 if(wm.ball().posCount()==2)agent->setNeckAction( new rcsc::Neck_TurnToPoint(static_ball_pos) );
        	 else agent->setNeckAction(new rcsc::Neck_ScanField());
        	agent->setViewAction(view);
        }
        else if(wm.ball().posCount()>=2)
        {
           	view = new View_Normal();
           	agent->setNeckAction( new rcsc::Neck_TurnToPoint(static_ball_pos) );
           	agent->setViewAction(view);
        }
        else
        {
        	view= new View_Synch();
        	agent->setViewAction(view);
        	if(! Neck_TurnToBallAndPlayer( opp1 ).execute(agent) || wm.ball().posCount()>1)
        	{
        		agent->setNeckAction( new rcsc::Neck_TurnToPoint(static_ball_pos) );
        	}
        }
     }
/////////////end neck action/////////////////////////////////////////////////////
/*****************************************************************************/

    static int flag = 0;
    if(wm.time().cycle() - wm.penaltyKickState()->time().cycle() < 5)
    	flag = 0;
    if(me.absX() - 30.0 <= 0.6 && nearest_opp_dist > 15 &&
    		wm.time().cycle() - wm.penaltyKickState()->time().cycle() < 22
    		&& flag == 0)
    {
    	flag = 1;
    	//AngleDeg angl = wm.ball().pos().y > 0.0?90:-90;
//    	if(Body_TurnToAngle(angl).execute(agent))
//    	{
//    		std::cout<<"turn body to angle In "<<wm.time().cycle()<<endl;
//    		return true;
//    	}
    }
#if log
       if(opp1) log_writetofile<<"opp "<<opp1->pos()<<" self"<<wm.self().pos()<<endl;
#endif

    if((wm.ball().pos().x > -48 || wm.ball().pos().absY() > 10 ) )
    {
    	const ServerParam & SP = ServerParam::i();
    	const  rcsc::AbstractPlayerObject * o_ob;
    	AngleDeg target_angle = (ball-me).th();

      	o_ob=wm.opponentsFromBall().front();
      	if(o_ob!=NULL)
      	{

      		target_angle = wm.self().body();
//      		if(wm.ball().pos().x > 0.0)
//      		{
//				if(o_ob->body().degree() > 0.1)
//				{
//					target_angle + 90;
//				}
//				else
//				{
//					target_angle - 90;
//				}
//      		}

      	}

      	if(ball.absY() < 10)
      	{
      		if(target_angle.abs()>130
      				|| target_angle.abs()<50)target_angle= ( ball.y > 0  ? 90.0 : -90.0 );
      	}

 		Line2D l(me,target_angle);

 		double y=l.getY(sign(wm.self().pos().x)*52.5);
  		if(fabs(y)<15)
  			target_angle= ( ball.y > 0  ? 90.0 : -90.0 );
  		if(me.x< 0 && me.y <-15)
  			if(target_angle.degree() >  91 && target_angle.degree() <180)
  				target_angle=wm.self().body();
  		if(me.x< 0 && me.y > 15)
  		  			if(target_angle.degree() <  -91 && target_angle.degree() > -180)
  		  				target_angle=wm.self().body();

  		if(me.x> 0 && me.y >15)
  			if(target_angle.degree() > -89 && target_angle.degree() <0.0)
  				target_angle=wm.self().body();
  		if(me.x> 0 && me.y < -15)
  		  			if(target_angle.degree() >  0.0 && target_angle.degree() < 89)
  		  				target_angle=wm.self().body();
      	const Vector2D move_pos = Bhv_PenaltyKick::getPoint(agent);

        dlog.addLine(Logger::TEAM, wm.self().pos(), Vector2D::polar2vector(1, target_angle) + wm.self().pos());



//      	if(me.dist(goalcenter) > move_pos.dist(goalcenter))
//    		if( doMove(agent,move_pos,0.4))
//    		{
//    			agent->setNeckAction( new rcsc::Neck_TurnToBall());
//    			std::cout<<"doMove In "<<wm.time().cycle()<<endl;
//    			return true;
//    		}
////////////////////////////////////////////////////////////////

          if((wm.self().body() - target_angle).abs() >90){
              if(rcsc::Body_TurnToAngle(target_angle).execute( agent )){
                    agent->setNeckAction( new rcsc::Neck_TurnToBall() );
                    return true;
            }

          }

          if (wm.self().pos().dist(goalcenter) > move_pos.dist(goalcenter)){
              if((wm.self().body() - (move_pos - wm.self().pos()).th()).abs() > 100){
                agent->doDash(-SP.maxDashPower());
                agent->setNeckAction( new rcsc::Neck_TurnToBall() );
                return true;
            }
          }

      	  if(!Body_GoToPoint( move_pos,0.4,SP.maxDashPower(),-1,100,false,45).execute( agent ))
      	  {
      		agent->setNeckAction( new rcsc::Neck_TurnToBall() );
#if log
       log_writetofile<<"Body_TurnToAngle "<<target_angle.degree()<<endl;
#endif
   			if(true)//false )//wm.self().body().degree() - target_angle.degree() < 6)
   			{
   			//	if(o_ob->distFromSelf()<6)
				if(rcsc::Body_TurnToAngle(target_angle).execute( agent ))
				{
					agent->setNeckAction( new rcsc::Neck_TurnToBall() );
					std::cout<<"turn body to "<<target_angle.degree()<< " angle in pos "<<wm.time().cycle()<<endl;
					return true;
				}
   			}
      	  }
#if log
        log_writetofile<<"Body_GoToPoint "<<move_pos<<endl;
#endif
        cout<<"gotopoint in "<<wm.time().cycle()<<"cycle to move_pos"<<move_pos<<endl;
      	  return true;
     }

#if log
        log_writetofile<<"intercept"<<endl;
#endif
    	agent->setNeckAction( new rcsc::Neck_TurnToBall() );
        rcsc::Body_Intercept(false,wm.ball().pos()+wm.ball().vel()).execute( agent );

        return true;

}


bool
Bhv_PenaltyKick::doMove(rcsc::PlayerAgent * agent,const rcsc::Vector2D &point,double thr)
{
	const rcsc::WorldModel & wm = agent->world();

	if((wm.self().pos() + wm.self().vel() ).dist(point) <= thr )
	{
	    return Body_StopDash(false).execute(agent);
	}
	if(point.dist(wm.self().pos()) <= thr )
	{
		return false;
	}
	 rcsc::Vector2D GoalCenter(sign(wm.ball().pos().x)*52.0,0.0);

	 AngleDeg body = wm.self().body();
//	 AngleDeg pointangl =(point - wm.self().pos()).th();
//	 rcsc::Vector2D target =  rcsc::Vector2D::polar2vector(5,pointangl);
//	 rcsc::Vector2D imagpos =  rcsc::Vector2D::polar2vector(5,body);
	 //double z = std::fabs( (*t)->body().degree() -  (target - tPos).dir().degree() );
	 //ekhtelafe badane girande nesbat be noghte PassTarget
//     AngleDeg point_360 = pointangl.degree()>0.0?pointangl:360+pointangl;
//     AngleDeg body_360 =body.degree()>0.0?body:360+body;
//wm.self().playerType().kickableArea()	 AngleDeg targetangl1 = fabs( body.degree() -  (point - wm.self().pos()).dir().degree()) ;
	 AngleDeg targetangl =  body.degree() -  (point - wm.self().pos()).th().degree() ;

//	 double a1 =sqrt(imagpos.x*imagpos.x+imagpos.y*imagpos.y);
//	 double a2 = sqrt(target.x*target.x+target.y*target.y);
//	 AngleDeg targetangl =  AngleDeg::acos_deg((imagpos.x*target.x + target.y*imagpos.y)/(a1*a2) );


//	 AngleDeg targetangl_360 =  body_360 - point_360;

//	 if()
//	 {
//
//	 }

	  if ( targetangl.degree() < -360.0 || 360.0 < targetangl.degree() )
	   {
		  targetangl = std::fmod( targetangl.degree(), 360.0 );
	   }

     if ( targetangl.degree() < -180.0 )
     {
    	 targetangl += 360.0;
     }

     if ( targetangl.degree() > 180.0 )
     {
    	 targetangl -= 360.0;
     }
     //cout<<" targetangl "<<targetangl.degree()<<" a1=>"<<a1<<" a2=>"<<a2<< " "<<wm.time().cycle()<<endl;

	 /*
	  * do - dash
	  */
	 return  agent->doDash( ServerParam::i().maxDashPower(), targetangl );
}


rcsc::Vector2D Bhv_PenaltyKick::getPoint( rcsc::PlayerAgent * agent )
{
  const rcsc::WorldModel & wm = agent->world();

  rcsc::Vector2D ball = wm.ball().pos();
  rcsc::Vector2D me = wm.self().pos();
  rcsc::Vector2D GoalCenter(sign(ball.x)*53.0,0.0);
  rcsc::Line2D lball(ball,wm.ball().vel().th());
		  rcsc::Line2D(wm.opponentsFromBall().front()->pos(),wm.opponentsFromBall().front()->body());

  int oppCycles = wm.interceptTable()->opponentReachCycle();
  Vector2D ball_pos;
  if (wm.existKickableOpponent() )
  {
      ball_pos = wm.ball().pos();
      ball_pos += wm.opponentsFromBall().front()->vel();
    	if(wm.ball().posCount()>=1)
    	{
    		//ball_pos += wm.opponentsFromBall().front()->vel();
    	      ball_pos = inertia_n_step_point( wm.ball().pos(),
    	    		  	  	  	  	  	  	  wm.opponentsFromBall().front()->vel(),
    	                                       wm.ball().posCount(),
    	                                       0.0 );
    	}
  }
  else
  {
	  int cycle = oppCycles;//std::max(oppCycles,3);
	  if(wm.ball().posCount()>1)cycle += min (3,wm.ball().posCount());
      ball_pos = inertia_n_step_point( wm.ball().pos(),
                                       wm.ball().vel(),
                                       cycle,
                                       ServerParam::i().ballDecay() );
  }
  	static int flag = 0;
  	static rcsc::Vector2D last_move(0,0);
	static double disthr=0.83;
	double Dis=disthr * ball_pos.dist(GoalCenter);
	// Dis = min(Dis,23.9);
	AngleDeg goalfromball= (   ball_pos-GoalCenter ).th();
	Vector2D move_pos= GoalCenter +  rcsc::Vector2D::polar2vector(Dis ,goalfromball);
	Line2D TArget_line = Line2D(GoalCenter,move_pos);
    move_pos.x = min(move_pos.x, -30.);

//	if(ball_pos.dist(GoalCenter) > 24)
//	{
//		move_pos = GoalCenter +  rcsc::Vector2D::polar2vector(22 ,goalfromball);
//	}
//	bool Stime = (wm.time().cycle()-wm.penaltyKickState()->time().cycle() )< 30;
//	if(Stime && move_pos.absX() < 30.0 && move_pos.absY() < 7)
//	{
//		move_pos = TArget_line.intersection(Line2D(Vector2D(sign(ball.x)*30,0),90));
//	}
	//else
	bool move = (oppCycles > 1 ||me.dist(ball) > 10)  && me.dist(move_pos) > 7;
	rcsc::Vector2D my = Vector2D(me.x,TArget_line.getY(me.x));
	if(me.dist(move_pos) > 6 && move_pos.absX() < me.absX()
			&& fabs(my.y - me.y)> 0.70
			&& (flag == 0 || ! move))
	{
		/*
		 * correct soon my pos
		 */


		double mag = 2.0;//me.dist(ball_pos)*0.2;

		my+=Vector2D::polar2vector(mag,goalfromball);
		move_pos = my;
		last_move = move_pos;
		flag =1;
	}
//

	if(flag == 1 && move)
	{
		move_pos = last_move ;
		flag=0;
	}
	else if(flag == 1 )flag = 0;


//	if(((move_pos.dist(GoalCenter) < me.dist(GoalCenter) && me.dist(move_pos)>1.5)
//			||(me.dist(wm.ball().pos())<5 ))&& me.absY()< 10 && me.absX() < 34)
//	{
//		Line2D my_line (me,90.0);
//		move_pos= TArget_line.intersection(my_line);
//	}
	Arm_PointToPoint(move_pos).execute(agent);
    dlog.addLine(Logger::TEAM, move_pos, wm.self().pos(), "#000000");
	return move_pos;
}



/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::doGoalieBasicMove( PlayerAgent * agent )
{
    const ServerParam & SP = ServerParam::i();
    const WorldModel & wm = agent->world();

    Rect2D our_penalty( Vector2D( -SP.pitchHalfLength(),
                                  -SP.penaltyAreaHalfWidth() + 1.0 ),
                        Size2D( SP.penaltyAreaLength() - 1.0,
                                SP.penaltyAreaWidth() - 2.0 ) );

    dlog.addText( Logger::TEAM,
                  __FILE__": goalieBasicMove. " );

    ////////////////////////////////////////////////////////////////////////
    // get active interception catch point
    const int self_min = wm.interceptTable()->selfReachCycle();
    Vector2D move_pos = wm.ball().inertiaPoint( self_min );

    if ( our_penalty.contains( move_pos ) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": goalieBasicMove. exist intercept point " );
        agent->debugClient().addMessage( "ExistIntPoint" );
        if ( wm.interceptTable()->opponentReachCycle() < wm.interceptTable()-> selfReachCycle()
             || wm.interceptTable()-> selfReachCycle() <=4 )
        {
            if ( Body_Intercept( false ).execute( agent ) )
            {
                agent->debugClient().addMessage( "Intercept" );
                dlog.addText( Logger::TEAM,
                              __FILE__": goalieBasicMove. do intercept " );
                agent->setNeckAction( new Neck_TurnToBall() );
                return true;
            }
        }
    }


    Vector2D my_pos = wm.self().pos();
    Vector2D ball_pos;
    if ( wm.existKickableOpponent() )
    {
        ball_pos = wm.opponentsFromBall().front()->pos();
        ball_pos += wm.opponentsFromBall().front()->vel();
    }
    else
    {
        ball_pos = inertia_n_step_point( wm.ball().pos(),
                                         wm.ball().vel(),
                                         3,
                                         SP.ballDecay() );
    }

    move_pos = getGoalieMovePos( ball_pos, my_pos );

    dlog.addText( Logger::TEAM,
                  __FILE__": goalie basic move to (%.1f, %.1f)",
                  move_pos.x, move_pos.y );
    agent->debugClient().setTarget( move_pos );
    agent->debugClient().addMessage( "BasicMove" );

    if ( ! Body_GoToPoint( move_pos,
                           0.5,
                           SP.maxDashPower()
                           ).execute( agent ) )
    {
        // already there
        AngleDeg face_angle = wm.ball().angleFromSelf();
        if ( wm.ball().angleFromSelf().isLeftOf( wm.self().body() ) )
        {
            face_angle += 90.0;
        }
        else
        {
            face_angle -= 90.0;
        }
        Body_TurnToAngle( face_angle ).execute( agent );
    }
    agent->setNeckAction( new Neck_TurnToBall() );

    return true;
}

/*-------------------------------------------------------------------*/
/*!
  ball_pos & my_pos is set to self localization oriented.
  if ( onfiled_side != our_side ), these coordinates must be reversed.
*/
Vector2D
Bhv_PenaltyKick::getGoalieMovePos( const Vector2D & ball_pos,
                                   const Vector2D & my_pos )
{
    const ServerParam & SP = ServerParam::i();
    const double min_x = -SP.pitchHalfLength() + SP.catchAreaLength()*0.9;

    if ( ball_pos.x < -49.0 )
    {
        if ( ball_pos.absY() < SP.goalHalfWidth() )
        {
            return Vector2D( min_x, ball_pos.y );
        }
        else
        {
            return Vector2D( min_x,
                             sign( ball_pos.y ) * SP.goalHalfWidth() );
        }
    }

    Vector2D goal_l( -SP.pitchHalfLength(), -SP.goalHalfWidth() );
    Vector2D goal_r( -SP.pitchHalfLength(), +SP.goalHalfWidth() );

    AngleDeg ball2post_angle_l = ( goal_l - ball_pos ).th();
    AngleDeg ball2post_angle_r = ( goal_r - ball_pos ).th();

    // NOTE: post_angle_r < post_angle_l
    AngleDeg line_dir = AngleDeg::bisect( ball2post_angle_r,
                                          ball2post_angle_l );

    Line2D line_mid( ball_pos, line_dir );
    Line2D goal_line( goal_l, goal_r );

    Vector2D intersection = goal_line.intersection( line_mid );
    if ( intersection.isValid() )
    {
        Line2D line_l( ball_pos, goal_l );
        Line2D line_r( ball_pos, goal_r );

        AngleDeg alpha = AngleDeg::atan2_deg( SP.goalHalfWidth(),
                                              SP.penaltyAreaLength() - 2.5 );
        double dist_from_goal
            = ( ( line_l.dist( intersection ) + line_r.dist( intersection ) ) * 0.5 )
            / alpha.sin();

        dlog.addText( Logger::TEAM,
                      __FILE__": goalie move. intersection=(%.1f, %.1f) dist_from_goal=%.1f",
                      intersection.x, intersection.y, dist_from_goal );
        if ( dist_from_goal <= SP.goalHalfWidth() )
        {
            dist_from_goal = SP.goalHalfWidth();
            dlog.addText( Logger::TEAM,
                          __FILE__": goalie move. outer of goal. dist_from_goal=%.1f",
                          dist_from_goal );
        }

        if ( ( ball_pos - intersection ).r() + 1.5 < dist_from_goal )
        {
            dist_from_goal = ( ball_pos - intersection ).r() + 1.5;
            dlog.addText( Logger::TEAM,
                          __FILE__": goalie move. near than ball. dist_from_goal=%.1f",
                          dist_from_goal );
        }

        AngleDeg position_error = line_dir - ( intersection - my_pos ).th();

        const double danger_angle = 21.0;
        dlog.addText( Logger::TEAM,
                      __FILE__": goalie move position_error_angle=%.1f",
                      position_error.degree() );
        if ( position_error.abs() > danger_angle )
        {
            dist_from_goal *= ( ( 1.0 - ((position_error.abs() - danger_angle)
                                         / (180.0 - danger_angle)) )
                                * 0.5 );
            dlog.addText( Logger::TEAM,
                          __FILE__": goalie move. error is big. dist_from_goal=%.1f",
                          dist_from_goal );
        }

        Vector2D result = intersection;
        Vector2D add_vec = ball_pos - intersection;
        add_vec.setLength( dist_from_goal );
        dlog.addText( Logger::TEAM,
                      __FILE__": goalie move. intersection=(%.1f, %.1f) add_vec=(%.1f, %.1f)%.2f",
                      intersection.x, intersection.y,
                      add_vec.x, add_vec.y,
                      add_vec.r() );
        result += add_vec;
        if ( result.x < min_x )
        {
            result.x = min_x;
        }
        return result;
    }
    else
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": goalie move. shot line has no intersection with goal line" );

        if ( ball_pos.x > 0.0 )
        {
            return Vector2D(min_x , goal_l.y);
        }
        else if ( ball_pos.x < 0.0 )
        {
            return Vector2D(min_x , goal_r.y);
        }
        else
        {
            return Vector2D(min_x , 0.0);
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::doGoalieSlideChase( PlayerAgent* agent )
{
    const WorldModel & wm = agent->world();

    if ( std::fabs( 90.0 - wm.self().body().abs() ) > 2.0 )
    {
        Vector2D face_point( wm.self().pos().x, 100.0);
        if ( wm.self().body().degree() < 0.0 )
        {
            face_point.y = -100.0;
        }
        Body_TurnToPoint( face_point ).execute( agent );
        agent->setNeckAction( new Neck_TurnToBall() );
        return true;
    }

    Ray2D ball_ray( wm.ball().pos(), wm.ball().vel().th() );
    Line2D ball_line( ball_ray.origin(), ball_ray.dir() );
    Line2D my_line( wm.self().pos(), wm.self().body() );

    Vector2D intersection = my_line.intersection( ball_line );
    if ( ! intersection.isValid()
         || ! ball_ray.inRightDir( intersection ) )
    {
        Body_Intercept( false ).execute( agent ); // goalie mode
        agent->setNeckAction( new Neck_TurnToBall() );
        return true;
    }

    if ( wm.self().pos().dist( intersection )
         < ServerParam::i().catchAreaLength() * 0.7 )
    {
        Body_StopDash( false ).execute( agent ); // not save recovery
        agent->setNeckAction( new Neck_TurnToBall() );
        return true;
    }

    AngleDeg angle = ( intersection - wm.self().pos() ).th();
    double dash_power = ServerParam::i().maxDashPower();
    if ( ( angle - wm.self().body() ).abs() > 90.0 )
    {
        dash_power = ServerParam::i().minDashPower();
    }
    agent->doDash( dash_power );
    agent->setNeckAction( new Neck_TurnToBall() );
    return true;
}
