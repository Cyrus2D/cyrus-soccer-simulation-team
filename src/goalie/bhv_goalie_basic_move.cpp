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

#include "bhv_goalie_basic_move.h"
#include "../strategy.h"
#include "../move_def/bhv_basic_tackle.h"
#include "neck_goalie_turn_neck.h"
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>

#include <rcsc/action/body_intercept.h>
#include <rcsc/action/basic_actions.h>
// #include <rcsc/action/body_goalie_go_to_point.h> CYRUS_LIB
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/body_stop_dash.h>
#include <rcsc/action/bhv_go_to_point_look_ball.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/debug_client.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/line_2d.h>
#include <rcsc/soccer_math.h>
#include <CppDNN/DeepNueralNetwork.h>

#include "../chain_action/field_analyzer.h"
using namespace rcsc;

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_GoalieBasicMove::execute( PlayerAgent * agent )
{

    agent->debugClient().addMessage("ex");
    //////////////////////////////////////////////////////////////////
    // tackle
    if ( Bhv_BasicTackle( 0.8 ).execute( agent ) )
    {
        agent->debugClient().addMessage("tackle");
        dlog.addText( Logger::TEAM,
                      __FILE__": tackle" );
        return true;
    }

    /////////////////////////////////////////////////////////////
    // intercepts out of penalty area
    if( doOffensiveIntercept( agent ) )
    {
        agent->debugClient().addMessage("offInter");
        dlog.addText(Logger::PLAN, "do off inter");
        // run run run !!
        return true;
    }
    if( doHeliosGolie(agent)){
        agent->debugClient().addMessage("HelGol");
        return true;
    }
    /////////////////////////////////////////////////////////////
    // offensive movements out of penalty area
    if( doOffensivePositioning( agent ) )
    {
        agent->debugClient().addMessage("offPos");
        // no comment :D just do it!
        dlog.addText(Logger::PLAN, "do off pos");
        return true;
    }
    /////////////////////////////////////////////////////////////
    Vector2D move_point = getTargetPoint( agent );

    dlog.addText( Logger::TEAM,
                  __FILE__": Bhv_GoalieBasicMove. move_point(%.2f %.2f)",
                  move_point.x, move_point.y );

    //----------------------------------------------------------
    if ( doPrepareDeepCross( agent, move_point ) )
    {
        agent->debugClient().addMessage("DeepCross");
        // face to opponent side to wait the opponent last cross pass
        return true;
    }
    //----------------------------------------------------------
    // check distance to the move target point
    // if already there, try to stop
    if ( doStopAtMovePoint( agent, move_point ) ) /// checked! ok
    {
        agent->debugClient().addMessage("StopMove");
        // execute stop action
        return true;
    }
    //----------------------------------------------------------
    // check whether ball is in very dangerous state
    /*    if ( doMoveForDangerousState( agent, move_point ) ) /// checked! seems unneccessary. disabled!
    {
        // execute emergency action
        return true;
    }*/
    //----------------------------------------------------------
    // check & correct X difference
    if ( doCorrectX( agent, move_point ) ) /// checked! ok
    {
        agent->debugClient().addMessage("correctX");
        // execute x-pos adjustment action
        return true;
    }

    //----------------------------------------------------------
    if ( doCorrectBodyDir( agent, move_point, true ) ) /// checked! zaheran ok
        // consider opp
    {
        agent->debugClient().addMessage("correctBody");
        // exeucte turn
        return true;
    }

    //----------------------------------------------------------
    if ( doGoToMovePoint( agent, move_point ) )
    {
        agent->debugClient().addMessage("GoMove");
        // mainly execute Y-adjustment if body direction is OK. -> only dash
        // if body direction is not good, nomal go to action is done.
        return true;
    }

    //----------------------------------------------------------
    // change my body angle to desired angle
    if ( doCorrectBodyDir( agent, move_point, false ) ) // not consider opp
    {
        agent->debugClient().addMessage("correctBody");
        return true;
    }

    dlog.addText( Logger::TEAM,
                  __FILE__": only look ball" );
    agent->debugClient().addMessage( "OnlyTurnNeck" );

    agent->doTurn( 0.0 );
    agent->setNeckAction( new Neck_GoalieTurnNeck() );

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_GoalieBasicMove::doOffensivePositioning( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();
    Vector2D ball = wm.ball().pos();
    Vector2D me = wm.self().pos();
    Vector2D homePos = getTargetPoint( agent );
    float dashPower;
    double dist_thr = wm.ball().distFromSelf() * 0.175;
    if( dist_thr < 1.5 )
        dist_thr = 1.5;

    getOffenseivePos( agent, homePos );
    dashPower = getOffensiveMoveDashPower( agent, homePos );

    int opp_min = wm.interceptTable()->opponentReachCycle();
    int mate_min = wm.interceptTable()->teammateReachCycle();

    rcsc::Vector2D ballFinal = rcsc::Vector2D( -30.0,0 );

    if( opp_min <= mate_min )
        ballFinal = wm.ball().inertiaPoint(opp_min);
    else
        ballFinal = wm.ball().inertiaPoint(mate_min);


    if( ballFinal.x > -18 )
    {

        /*
           Body_GoalieGoToPoint( M_target_point, dist_thr,
                           ServerParam::i().maxPower(),
                           100, // cycle
                           false, // back
                           true, // stamina save
                           20.0 // angle threshold
                           ).execute( agent )
         */

        doGoToPointLookBall( agent,
                             homePos,
                             wm.ball().angleFromSelf(),
                             dist_thr,
                             dashPower );
        //        if ( !Body_GoalieGoToPoint( homePos, dist_thr, dashPower ).execute( agent ) )
        //            Body_TurnToBall().execute( agent );

        agent->setNeckAction( new Neck_TurnToBall() );

        return true;
    }
    else if( ball.x < -20 && ball.x > -40 && (me.x > -49.5 || me.absY() > 11) )
    {
        /*    std::cout<<"\n^&%&%^&$^$%^#$!@ Cycle: "<<wm.time().cycle()<<" GOALIEEE HARKATE MOZAKHRAAAF! \n";*/
        doGoToPointLookBall( agent,
                             homePos,
                             wm.ball().angleFromSelf(),
                             dist_thr,
                             dashPower );
        //        Body_GoalieGoToPoint( Vector2D(-50.5,getTargetPoint(agent).y),
        //                              0.8, dashPower ).execute( agent );
        agent->setNeckAction( new Neck_TurnToBallOrScan(0) );

        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_GoalieBasicMove::doOffensiveIntercept( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();
    Vector2D ball = wm.ball().pos();
    Vector2D me = wm.self().pos();

    int myCycles = wm.interceptTable()->selfReachCycle();
    int tmmCycles = wm.interceptTable()->teammateReachCycle();
    int oppCycles = wm.interceptTable()->opponentReachCycle();

    if( myCycles <= oppCycles && myCycles < tmmCycles &&
            !wm.kickableTeammate() && !wm.kickableOpponent() &&
            wm.gameMode().type() == GameMode::PlayOn )
    {
        //       Body_Intercept().execute( agent );
        Body_Intercept( false ).execute( agent );

        if ( wm.ball().distFromSelf() < ServerParam::i().visibleDistance() )
            agent->setNeckAction( new Neck_TurnToLowConfTeammate() );
        else
            agent->setNeckAction( new Neck_TurnToBallOrScan(0) );

        return true;
    }

    return false;
}


/*-------------------------------------------------------------------*/
/*!

 */
void
Bhv_GoalieBasicMove::getOffenseivePos( PlayerAgent * agent,
                                       Vector2D &move_point )
{
    const WorldModel & wm = agent->world();
    Vector2D ball = wm.ball().pos();
    Vector2D me = wm.self().pos();

    int tmmCycles = wm.interceptTable()->teammateReachCycle();
    int oppCycles = wm.interceptTable()->opponentReachCycle();

    if( tmmCycles < oppCycles )
        ball = wm.ball().inertiaPoint( tmmCycles );
    else
        ball = wm.ball().inertiaPoint( oppCycles );

    if( ball.x > -22.0 )
    {
        if( ball.x > 25.0 )
            move_point = Vector2D( -15.0,0.0 );
        else if( ball.x > 15.0 )
            move_point = Vector2D( -18.0,0.0 );
        else if( ball.x > 5.0 )
            move_point = Vector2D( -20.0,0.0 );
        else if( ball.x > -5.0 )
            move_point = Vector2D( -27.0,0.0 );
        else if( ball.x > -15.0 )
            move_point = Vector2D( -30.0,0.0 );
        else if( ball.x > -22.0 )
            move_point = Vector2D( -43.0,0.0 );

        //TODO should be checked
//        if (ball.x < wm.ourDefensePlayerLineX() + 5){
//            move_point.x -= 10;
//        }
        if( ball.absY() > 11.0 )
            move_point.y = sign(ball.y) * 4.0;
        if( ball.absY() > 18.0 )
            move_point.y = sign(ball.y) * 6.5;
        if( ball.absY() > 23.0 )
            move_point.y = sign(ball.y) * 9.0;

        if( ball.x < -15.0 )
        {
            if( move_point.y > 5.5 )  move_point.y = 5.5;
            if( move_point.y < -5.5 ) move_point.y = -5.5;
        }

        if( wm.ourDefenseLineX() - 20.0 < move_point.x )
            move_point.x = wm.ourDefenseLineX() - 20.0;

        if( move_point.x < -48.0 )
            move_point.x = -48.0;
    }
    if(wm.interceptTable()->teammateReachCycle() < wm.interceptTable()->opponentReachCycle()){
        move_point.x +=5;
    }

}


/*-------------------------------------------------------------------*/
/*!

 */
double
Bhv_GoalieBasicMove::getOffensiveMoveDashPower( PlayerAgent * agent,
                                                const Vector2D & homePos )
{
    const WorldModel & wm = agent->world();
    const PlayerType & mytype = wm.self().playerType();
    Vector2D ball = wm.ball().pos();
    Vector2D me = wm.self().pos();
    const double my_inc = mytype.staminaIncMax() * wm.self().recovery();


    if ( std::fabs( me.x - homePos.x ) > 2.0 )
        return ServerParam::i().maxPower();

    if ( wm.gameMode().type() != GameMode::PlayOn && me.dist(homePos) < 3 )
        return my_inc * 0.5;

    if ( ball.x > 0.0 )
    {
        if ( wm.self().stamina() < ServerParam::i().staminaMax() * 0.9 )
            return my_inc * 0.5;
        return my_inc;
    }
    else if ( ball.x > -30.0 )
    {

        if ( wm.ball().vel().x > 1.0 )
            return my_inc * 0.5;

        int opp_min = wm.interceptTable()->opponentReachCycle();
        if ( opp_min <= 5 )
            return ServerParam::i().maxPower();

        if ( wm.self().stamina() < ServerParam::i().staminaMax() * 0.7 )
            return my_inc * 0.7;

        return ServerParam::i().maxPower() * 0.8;
    }
    else
        return ServerParam::i().maxPower();

}



/*-------------------------------------------------------------------*/
/*!

 */
Vector2D
Bhv_GoalieBasicMove::getTargetPoint( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();
    double base_move_x = -50.3;

    //    if( wm.ball().pos().dist(rcsc::Vector2D(-52.5,0)) > 15.0 && wm.ball().pos().absY() < 12.0 )
    //        base_move_x = -48.5;

    const double danger_move_x = -51.5;

    int ball_reach_step = 0;
    if ( ! wm.kickableTeammate()
         && ! wm.kickableOpponent() )
    {
        ball_reach_step
                = std::min( wm.interceptTable()->teammateReachCycle(),
                            wm.interceptTable()->opponentReachCycle() );
    }
    const Vector2D base_pos = wm.ball().inertiaPoint( ball_reach_step );


    //---------------------------------------------------------//
    // angle is very dangerous
    if ( base_pos.y > ServerParam::i().goalHalfWidth() + 3.0 )
    {
        Vector2D right_pole( - ServerParam::i().pitchHalfLength(),
                             ServerParam::i().goalHalfWidth() );
        AngleDeg angle_to_pole = ( right_pole - base_pos ).th();

        if ( -140.0 < angle_to_pole.degree()
             && angle_to_pole.degree() < -90.0 )
        {
            agent->debugClient().addMessage( "RPole" );
            return Vector2D( danger_move_x, ServerParam::i().goalHalfWidth() + 0.001 );
        }
    }
    else if ( base_pos.y < -ServerParam::i().goalHalfWidth() - 3.0 )
    {
        Vector2D left_pole( - ServerParam::i().pitchHalfLength(),
                            - ServerParam::i().goalHalfWidth() );
        AngleDeg angle_to_pole = ( left_pole - base_pos ).th();

        if ( 90.0 < angle_to_pole.degree()
             && angle_to_pole.degree() < 140.0 )
        {
            agent->debugClient().addMessage( "LPole" );
            return Vector2D( danger_move_x, - ServerParam::i().goalHalfWidth() - 0.001 );
        }
    }

    //---------------------------------------------------------//
    // ball is close to goal line
    if ( base_pos.x < -ServerParam::i().pitchHalfLength() + 8.0
         && base_pos.absY() > ServerParam::i().goalHalfWidth() + 2.0 )
    {
        Vector2D target_point( base_move_x, ServerParam::i().goalHalfWidth() - 0.1 );
        if ( base_pos.y < 0.0 )
        {
            target_point.y *= -1.0;
        }

        dlog.addText( Logger::TEAM,
                      __FILE__": getTarget. target is goal pole" );
        agent->debugClient().addMessage( "Pos(1)" );

        return target_point;
    }

    //---------------------------------------------------------//
    {
        const double x_back = 7.0; // tune this!!
        int ball_pred_cycle = 5; // tune this!!
        const double y_buf = 0.5; // tune this!!
        const Vector2D base_point( - ServerParam::i().pitchHalfLength() - x_back,
                                   0.0 );
        Vector2D ball_point;
        if ( wm.kickableOpponent() )
        {
            ball_point = base_pos;
            agent->debugClient().addMessage( "Pos(2)" );
        }
        else
        {
            int opp_min = wm.interceptTable()->opponentReachCycle();
            if ( opp_min < ball_pred_cycle )
            {
                ball_pred_cycle = opp_min;
                dlog.addText( Logger::TEAM,
                              __FILE__": opp may reach near future. cycle = %d",
                              opp_min );
            }

            ball_point
                    = inertia_n_step_point( base_pos,
                                            wm.ball().vel(),
                                            ball_pred_cycle,
                                            ServerParam::i().ballDecay() );
            agent->debugClient().addMessage( "Pos(3)" );
        }

        if ( ball_point.x < base_point.x + 0.1 )
        {
            ball_point.x = base_point.x + 0.1;
        }

        Line2D ball_line( ball_point, base_point );
        double move_y = ball_line.getY( base_move_x );

        if ( move_y > ServerParam::i().goalHalfWidth() - y_buf )
        {
            move_y = ServerParam::i().goalHalfWidth() - y_buf;
        }
        if ( move_y < - ServerParam::i().goalHalfWidth() + y_buf )
        {
            move_y = - ServerParam::i().goalHalfWidth() + y_buf;
        }

        if( wm.ball().pos().dist(rcsc::Vector2D(-49,rcsc::sign(wm.ball().pos().y)*2.5 )) > 12.0 )
            move_y *= 0.85;
        else
            move_y *= 0.94;

        /// MarliK Goalie Positioning 2011 ///

        float fixedX = -50.3;
        rcsc::Vector2D ball = wm.ball().pos();

        rcsc::Line2D xLine = rcsc::Line2D( rcsc::Vector2D(fixedX,30), rcsc::Vector2D(fixedX,-30) );

        rcsc::Line2D ballToUpTir = rcsc::Line2D( base_pos, rcsc::Vector2D(-52.5,-7.0) );
        rcsc::Line2D ballToDownTir = rcsc::Line2D( base_pos, rcsc::Vector2D(-52.5,7.0) );

        rcsc::Vector2D upIntersection = xLine.intersection(ballToUpTir);
        rcsc::Vector2D downIntersection = xLine.intersection(ballToDownTir);

        base_move_x = fixedX;
        move_y = upIntersection.y + downIntersection.y;

        if(wm.teammatesFromSelf().size() > 0)
            if( wm.teammatesFromSelf().front()->pos().x < -49.0 &&  wm.teammatesFromSelf().front()->pos().absY() < 7.0 )
                move_y *= 1.3;

        if( move_y < -6.0 ) move_y = -6.1;
        if( move_y >  6.0 ) move_y =  6.1;

        return Vector2D( base_move_x, move_y );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
double
Bhv_GoalieBasicMove::getBasicDashPower( PlayerAgent * agent,
                                        const Vector2D & move_point )
{
    const WorldModel & wm = agent->world();
    const PlayerType & mytype = wm.self().playerType();

    const double my_inc = mytype.staminaIncMax() * wm.self().recovery();

    if ( fabs( wm.self().pos().x - move_point.x ) > 3.0 || fabs( wm.self().pos().y - move_point.y ) > 2.0 )
    {
        return ServerParam::i().maxDashPower();
    }

    if ( wm.ball().pos().x > -30.0 )
    {
        if ( wm.self().stamina() < ServerParam::i().staminaMax() * 0.9 )
        {
            return my_inc * 0.5;
        }
        agent->debugClient().addMessage( "P1" );
        return my_inc;
    }
    else if ( wm.ball().pos().x > ServerParam::i().ourPenaltyAreaLineX() )
    {
        if ( wm.ball().pos().absY() > 20.0 )
        {
            // penalty area
            agent->debugClient().addMessage( "P2" );
            return my_inc;
        }
        if ( wm.ball().vel().x > 1.0 )
        {
            // ball is moving to opponent side
            agent->debugClient().addMessage( "P2.5" );
            return my_inc * 0.5;
        }

        int opp_min = wm.interceptTable()->opponentReachCycle();
        if ( opp_min <= 3 )
        {
            agent->debugClient().addMessage( "P2.3" );
            return ServerParam::i().maxDashPower();
        }

        if ( wm.self().stamina() < ServerParam::i().staminaMax() * 0.7 )
        {
            agent->debugClient().addMessage( "P2.6" );
            return my_inc * 0.7;
        }
        agent->debugClient().addMessage( "P3" );
        return ServerParam::i().maxDashPower() * 0.6;
    }
    else
    {
        if ( wm.ball().pos().absY() < 15.0
             || wm.ball().pos().y * wm.self().pos().y < 0.0 ) // opposite side
        {
            agent->debugClient().addMessage( "P4" );
            return ServerParam::i().maxDashPower();
        }
        else
        {
            agent->debugClient().addMessage( "P5" );
            return my_inc;
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_GoalieBasicMove::doPrepareDeepCross( PlayerAgent * agent,
                                         Vector2D & move_point )
{
    if ( move_point.absY() < ServerParam::i().goalHalfWidth() - 0.8 )
    {
        // consider only very deep cross
        dlog.addText( Logger::TEAM,
                      __FILE__": doPrepareDeepCross no deep cross" );
        return false;
    }

    const WorldModel & wm = agent->world();

    if( wm.gameMode().type() != GameMode::PlayOn  )
    {
        return false;
    }


    const Vector2D goal_c( - ServerParam::i().pitchHalfLength(), 0.0 );

    Vector2D me = wm.self().pos();
    Vector2D ball = wm.ball().pos();

    move_point = Vector2D(-55.0,0.0) + Vector2D::polar2vector(8.8,(ball-goal_c).dir().degree());

    move_point.y = std::max(-6.8,move_point.y);
    move_point.y = std::min(6.8,move_point.y);

    if( move_point.x < -51.7 ) // 2013 AmirZ -51.3 bud >> bug dasht
        move_point.x = -51.7;    // 2013 AmirZ -51.3 bud >> bug dasht

    if( move_point.x > -49.5 ) // > -49.5 AmirZ RC2012 Round2
        move_point.x = -49.5;


    Vector2D goal_to_ball = wm.ball().pos() - goal_c;

    if ( goal_to_ball.th().abs() < 60.0 ) // AmirZ 2013: < 50.0 bud!
    {
        // ball is not in side cross area
        dlog.addText( Logger::TEAM,
                      __FILE__": doPrepareDeepCross.ball is not in side cross area" );
        return false;
    }

    Vector2D my_inertia = wm.self().inertiaFinalPoint();
    double dist_thr = wm.ball().distFromSelf() * 0.05;
    if ( dist_thr < 0.3 ) dist_thr = 0.3;
    //double dist_thr = 0.5;

    if ( my_inertia.dist( move_point ) > dist_thr )
    {
        // needed to go to move target point
        double dash_power = getBasicDashPower( agent, move_point );
        dlog.addText( Logger::TEAM,
                      __FILE__": doPrepareDeepCross. need to move. power=%.1f",
                      dash_power );
        agent->debugClient().addMessage( "DeepCrossMove%.0f", dash_power );
        agent->debugClient().setTarget( move_point );
        agent->debugClient().addCircle( move_point, dist_thr );

        doGoToPointLookBall( agent,
                             move_point,
                             wm.ball().angleFromSelf(),
                             dist_thr,
                             dash_power );
        return true;
    }

    AngleDeg body_angle = ( wm.ball().pos().y < 0.0
                            ? 25.0 // IO2013 12 o -12 bud AmirZ!
                            : -25.0 );
    agent->debugClient().addMessage( "PrepareCross" );
    dlog.addText( Logger::TEAM,
                  __FILE__": doPrepareDeepCross  body angle = %.1f  move_point(%.1f %.1f)",
                  body_angle.degree(),
                  move_point.x, move_point.y );
    agent->debugClient().setTarget( move_point );

    Body_TurnToAngle( body_angle ).execute( agent );
    agent->setNeckAction( new Neck_GoalieTurnNeck() );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_GoalieBasicMove::doStopAtMovePoint( PlayerAgent * agent,
                                        const Vector2D & move_point )
{
    //----------------------------------------------------------
    // already exist at target point
    // but inertia movement is big
    // stop dash

    const WorldModel & wm = agent->world();
    double dist_thr = wm.ball().distFromSelf() * 0.04;
    if ( dist_thr < 0.3 ) dist_thr = 0.3;

    // now, in the target area
    if ( wm.self().pos().dist( move_point ) < dist_thr )
    {
        const Vector2D my_final
                = inertia_final_point( wm.self().pos(),
                                       wm.self().vel(),
                                       wm.self().playerType().playerDecay() );
        // after inertia move, can stay in the target area
        if ( my_final.dist( move_point ) < dist_thr )
        {
            agent->debugClient().addMessage( "InertiaStay" );
            dlog.addText( Logger::TEAM,
                          __FILE__": doStopAtMovePoint. inertia stay" );
            return false;
        }

        // try to stop at the current point
        dlog.addText( Logger::TEAM,
                      __FILE__": doStopAtMovePoint. stop dash" );
        agent->debugClient().addMessage( "Stop" );
        agent->debugClient().setTarget( move_point );

        Body_StopDash( true ).execute( agent ); // save recovery
        agent->setNeckAction( new Neck_GoalieTurnNeck() );
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_GoalieBasicMove::doMoveForDangerousState( PlayerAgent * agent,
                                              const Vector2D & move_point )
{
    const WorldModel& wm = agent->world();

    const double x_buf = 0.4;

    const Vector2D ball_next = wm.ball().pos() + wm.ball().vel();

    dlog.addText( Logger::TEAM,
                  __FILE__": doMoveForDangerousState" );

    if ( std::fabs( move_point.x - wm.self().pos().x ) > x_buf
         && ball_next.x < -ServerParam::i().pitchHalfLength() + 11.0
         && ball_next.absY() < ServerParam::i().goalHalfWidth() + 1.0 )
    {
        // x difference to the move point is over threshold
        // but ball is in very dangerous area (just front of our goal)

        // and, exist opponent close to ball
        if ( ! wm.opponentsFromBall().empty()
             && wm.opponentsFromBall().front()->distFromBall() < 2.0 )
        {
            Vector2D block_point
                    = wm.opponentsFromBall().front()->pos();
            block_point.x -= 2.5;
            block_point.y = move_point.y;

            if ( wm.self().pos().x < block_point.x )
            {
                block_point.x = wm.self().pos().x;
            }

            dlog.addText( Logger::TEAM,
                          __FILE__": block opponent kickaer" );
            agent->debugClient().addMessage( "BlockOpp" );

            if ( doGoToMovePoint( agent, block_point ) )
            {
                return true;
            }

            double dist_thr = wm.ball().distFromSelf() * 0.04;
            if ( dist_thr < 0.4 ) dist_thr = 0.4;

            agent->debugClient().setTarget( block_point );
            agent->debugClient().addCircle( block_point, dist_thr );

            doGoToPointLookBall( agent,
                                 move_point,
                                 wm.ball().angleFromSelf(),
                                 dist_thr,
                                 ServerParam::i().maxDashPower() );
            return true;
        }
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_GoalieBasicMove::doCorrectX( PlayerAgent * agent,
                                 const Vector2D & move_point )
{
    const WorldModel & wm = agent->world();

    const double x_buf = 0.4;

    dlog.addText( Logger::TEAM,
                  __FILE__": doCorrectX" );
    if ( std::fabs( move_point.x - wm.self().pos().x ) < x_buf )
    {
        // x difference is already small.
        dlog.addText( Logger::TEAM,
                      __FILE__": doCorrectX. x diff is small" );
        return false;
    }

    int opp_min_cyc = wm.interceptTable()->opponentReachCycle();
    if ( ( ! wm.kickableOpponent() && opp_min_cyc >= 4 )
         || wm.ball().distFromSelf() > 18.0 )
    {
        double dash_power = getBasicDashPower( agent, move_point );

        dlog.addText( Logger::TEAM,
                      __FILE__": doCorrectX. power=%.1f",
                      dash_power );
        agent->debugClient().addMessage( "CorrectX%.0f", dash_power );
        agent->debugClient().setTarget( move_point );
        agent->debugClient().addCircle( move_point, x_buf );

        if ( ! wm.kickableOpponent()
             && wm.ball().distFromSelf() > 30.0 )
        {
            // if ( ! Body_GoalieGoToPoint( move_point, x_buf, dash_power
            //                     ).execute( agent ) ) CYRUS_LIB
            if ( ! Body_GoToPoint( move_point, x_buf, dash_power
                                         ).execute( agent ) )
                                         
            {
                AngleDeg body_angle = ( wm.self().body().degree() > 0.0
                                        ? 90.0
                                        : -90.0 );
                Body_TurnToAngle( body_angle ).execute( agent );

            }
            agent->setNeckAction( new Neck_TurnToBall() );
            return true;
        }

        doGoToPointLookBall( agent,
                             move_point,
                             wm.ball().angleFromSelf(),
                             x_buf,
                             dash_power );
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_GoalieBasicMove::doCorrectBodyDir( PlayerAgent * agent,
                                       const Vector2D & move_point,
                                       const bool consider_opp )
{
    // adjust only body direction

    const WorldModel & wm = agent->world();

    const Vector2D ball_next = wm.ball().pos() + wm.ball().vel();

    const AngleDeg target_angle = ( ball_next.y < 0.0 ? -90.0 : 90.0 );
    const double angle_diff = ( wm.self().body() - target_angle ).abs();

    dlog.addText( Logger::TEAM,
                  __FILE__": doCorrectBodyDir" );

    if ( angle_diff < 7.0 ) // < 5 bud 2011
    {
        return false;
    }

#if 1
    {
        const Vector2D goal_c( - ServerParam::i().pitchHalfLength(), 0.0 );
        Vector2D goal_to_ball = wm.ball().pos() - goal_c;
        if ( goal_to_ball.th().abs() >= 60.0 )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": doCorrectBodyDir. danger area" );
            return false;
        }
    }
#else
    if ( wm.ball().pos().x < -36.0
         && wm.ball().pos().absY() < 15.0
         && wm.self().pos().dist( move_point ) > 1.5 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": doCorrectBodyDir. danger area" );
        return false;
    }
#endif

    double opp_ball_dist
            = ( wm.opponentsFromBall().empty()
                ? 100.0
                : wm.opponentsFromBall().front()->distFromBall() );
    if ( ! consider_opp
         || opp_ball_dist > 7.0
         || wm.ball().distFromSelf() > 20.0
         || ( std::fabs( move_point.y - wm.self().pos().y ) < 1.0 // y diff
              && ! wm.kickableOpponent() ) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": body face to %.1f.  angle_diff=%.1f %s",
                      target_angle.degree(), angle_diff,
                      consider_opp ? "consider_opp" : "" );
        agent->debugClient().addMessage( "CorrectBody%s",
                                         consider_opp ? "WithOpp" : "" );
        Body_TurnToAngle( target_angle ).execute( agent );
        agent->setNeckAction( new Neck_GoalieTurnNeck() );
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_GoalieBasicMove::doGoToMovePoint( PlayerAgent * agent,
                                      const Vector2D & move_point )
{
    // move to target point
    // check Y coordinate difference

    const WorldModel & wm = agent->world();

    double dist_thr = wm.ball().distFromSelf() * 0.0375;
    if ( dist_thr < 0.3 ) dist_thr = 0.03;

    const double y_diff = std::fabs( move_point.y - wm.self().pos().y );
    if ( y_diff < dist_thr )
    {
        // already there
        dlog.addText( Logger::TEAM,
                      __FILE__": doGoToMovePoint. y_diff=%.2f < thr=%.2f",
                      y_diff, dist_thr );
        return false;
    }

    //----------------------------------------------------------//
    // dash to body direction

    double dash_power = getBasicDashPower( agent, move_point );

    // body direction is OK
    if ( std::fabs( wm.self().body().abs() - 90.0 ) < 8.0 )
    {
        // calc dash power only to reach the target point
        double required_power = y_diff / wm.self().dashRate();
        if ( dash_power > required_power )
        {
            dash_power = required_power;
        }

        if ( move_point.y > wm.self().pos().y )
        {
            if ( wm.self().body().degree() < 0.0 )
            {
                dash_power *= -1.0;
            }
        }
        else
        {
            if ( wm.self().body().degree() > 0.0 )
            {
                dash_power *= -1.0;
            }
        }

        dash_power = ServerParam::i().normalizeDashPower( dash_power );

        dlog.addText( Logger::TEAM,
                      __FILE__": doGoToMovePoint. CorrectY(1) power= %.1f",
                      dash_power );
        agent->debugClient().addMessage( "CorrectY(1)%.0f", dash_power );
        agent->debugClient().setTarget( move_point );

        agent->doDash( dash_power );
        agent->setNeckAction( new Neck_GoalieTurnNeck() );
    }
    else
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": doGoToMovePoint. CorrectPos power= %.1f",
                      dash_power );
        agent->debugClient().addMessage( "CorrectPos%.0f", dash_power );
        agent->debugClient().setTarget( move_point );
        agent->debugClient().addCircle( move_point, dist_thr );

        doGoToPointLookBall( agent,
                             move_point,
                             wm.ball().angleFromSelf(),
                             dist_thr,
                             dash_power );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Bhv_GoalieBasicMove::doGoToPointLookBall( PlayerAgent * agent,
                                          const Vector2D & target_point,
                                          const AngleDeg & body_angle,
                                          const double & dist_thr,
                                          const double & dash_power,
                                          const double & back_power_rate )
{
    const WorldModel & wm = agent->world();
    agent->debugClient().setTarget(target_point);
    if ( wm.gameMode().type() == GameMode::PlayOn
         || wm.gameMode().type() == GameMode::PenaltyTaken_ )
    {
        agent->debugClient().addMessage( "Goalie:GoToLook" );
        dlog.addText( Logger::TEAM,
                      __FILE__": doGoToPointLookBall. use GoToPointLookBall to (%.1f, %.1f)", target_point.x, target_point.y );
//        if (!Bhv_GoToPointLookBall( target_point,
//                               dist_thr,
//                               dash_power,
//                               back_power_rate
//                               ).execute( agent )){
        dlog.addText( Logger::TEAM,
                      __FILE__": doGoToPointLookBall. use GoToPointLookBall execute body go to point" );
        Body_GoToPoint(target_point,
                       dist_thr,
                       dash_power,
                       -1.0,
                       1,
                       false,
                       20.0,
                       3.0,
                       false ).execute(agent);
        Vector2D self_pos = wm.self().pos();
        Vector2D ball_pos = wm.ball().inertiaPoint(1);
        double dif_angle = ((target_point - self_pos).th() - (ball_pos - self_pos).th()).abs();
        if ( dif_angle < ServerParam::i().maxNeckAngle() - 5.0 + wm.self().viewWidth() / 2.0) {
            //ok
        }else{
            if (dif_angle < 140.0){
                agent->doChangeView( ViewWidth::NORMAL );
            }
            else{
                agent->doChangeView( ViewWidth::WIDE );
            }
        }
        agent->setNeckAction( new Neck_TurnToBall() );
//        }
    }
    else
    {
        agent->debugClient().addMessage( "Goalie:GoTo" );
        dlog.addText( Logger::TEAM,
                      __FILE__": doGoToPointLookBall. use GoToPoint" );
        // if ( ! Body_GoalieGoToPoint( target_point, dist_thr, dash_power
        //                     ).execute( agent ) ) CYRUS_LIB
        if ( Body_GoToPoint( target_point, dist_thr, dash_power
        ).execute( agent ) )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": doGoToPointLookBall. go" );
        }
        else
        {
            Body_TurnToAngle( body_angle ).execute( agent );
            dlog.addText( Logger::TEAM,
                          __FILE__": doGoToPointLookBall. turn to %.1f",
                          body_angle.degree() );
        }
        agent->setNeckAction( new Neck_TurnToBall() );
    }
}

bool Bhv_GoalieBasicMove::doHeliosGolie(PlayerAgent *agent)
{
    dlog.addText(Logger::PLAN, "start helios");
    const WorldModel & wm = agent->world();
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();
    Vector2D ball_pos = wm.ball().inertiaPoint(opp_min);

    if (opp_min > std::min(self_min, mate_min) + 5){
        dlog.addText(Logger::PLAN, "cant for opp min");
        return false;
    }

    if(ball_pos.x > -25){
        dlog.addText(Logger::PLAN, "cant for ball x");
        return false;
    }
    static DeepNueralNetwork * golie_dnn;
    static bool load_dnn = false;
    if(!load_dnn){
        load_dnn = true;
        golie_dnn = new DeepNueralNetwork();
        golie_dnn->ReadFromKeras("./data/deep/helios_golie_origin");
    }
    MatrixXd input(2,1);
    input(0,0) = (ball_pos.x + 52.5) / 105.0;
    input(1,0) = (ball_pos.y + 34.0) / 68.0;
    golie_dnn->Calculate(input);

    Vector2D target(golie_dnn->mOutput(0) * 105.0 - 52.5, golie_dnn->mOutput(1) * 68.0 - 34.0);
    Vector2D self_pos = wm.self().pos();
    AngleDeg self_body = wm.self().body();
    AngleDeg ball_to_up = (ball_pos - Vector2D(-52.5, -7)).th();
    AngleDeg ball_to_down = (ball_pos - Vector2D(-52.5, +7)).th();
    AngleDeg center_angle = AngleDeg((ball_to_up + ball_to_down).degree() / 2);
    Line2D center_line(ball_pos, center_angle);
    AngleDeg target_angle = (target - self_pos).th();
    Vector2D new_target = center_line.projection(target);

    agent->debugClient().setTarget(target);
    agent->debugClient().addTriangle(new_target + Vector2D(0.5,0),new_target + Vector2D(-0.35,0.35),new_target + Vector2D(-0.35,-0.35));
    if(ball_pos.absY() < 20)
        target = new_target;
    if(target.x < -52.0){
        target.x = -52.0;
    }
    Vector2D ball_opp_pos = ball_pos;
    if(wm.interceptTable()->fastestOpponent() != nullptr){
        if(wm.interceptTable()->fastestOpponent()->unum() > 0){
            if(wm.interceptTable()->fastestOpponent()->isKickable()){
                ball_opp_pos = wm.interceptTable()->fastestOpponent()->pos();
            }
        }
    }
    Vector2D base_target = target;
    if(target.dist(ball_opp_pos) < 5 && target.dist(ball_opp_pos) > 2){
        Vector2D new_target = target - ball_opp_pos;
        new_target.setLength(2.7);
        new_target = ball_opp_pos + new_target;
        if(new_target.x > -52){
            target = new_target;
            agent->debugClient().addRectangle(Rect2D(target,target + Vector2D(1,1)));
        }
    }

    AngleDeg a1((ball_pos - target).th().degree() + 90);
    AngleDeg a2((ball_pos - target).th().degree() - 90);
    AngleDeg body_target;
    if(ball_pos.y > 0){
        body_target = a2;
    }else{
        body_target = a1;
    }
    Line2D self_body_target_line(target, body_target);
//    Vector2D body_line_intersect_goal = Segment2D(Vector2D(-52.0, 7), Vector2D(-52.0, -7)).intersection(self_body_target_line);
//    if(body_line_intersect_goal.isValid()){
//        agent->debugClient().addLine(target, target + Vector2D::polar2vector(10, body_target));
//        Vector2D near_tir(-52.5, -7);
//        if(body_line_intersect_goal.dist(near_tir) > body_line_intersect_goal.dist(Vector2D(-52.5, +7))){
//            near_tir = Vector2D(-52.5, +7);
//        }
//        body_target = (target - near_tir).th();
//        agent->debugClient().addLine(target, target + Vector2D::polar2vector(20, body_target));
//    }

    if((body_target - AngleDeg(90)).abs() < 10){
        body_target = AngleDeg(90);
    }else if((body_target - AngleDeg(-90)).abs() < 10){
        body_target = AngleDeg(-90);
    }

    target -= Vector2D::polar2vector(0.5, body_target);
    double body_diff = (body_target - self_body).degree();
    double body_diff_abs = (body_target - self_body).abs();

    int cycle_dash = wm.self().playerTypePtr()-> cyclesToReachDistance(target.dist(self_pos));
    int cycle_dash_back_dash = wm.self().playerTypePtr()->cyclesToReachDistanceOnDashDir(target.dist(self_pos), 180.0);
    int cycle_turn = FieldAnalyzer::predict_player_turn_cycle(wm.self().playerTypePtr(),
                                                              wm.self().body(),
                                                              wm.self().vel().r(),
                                                              target.dist(self_pos),
                                                              (target - self_pos).th(),
                                                              0.0,
                                                              false);
    int cycle_turn_backmove = FieldAnalyzer::predict_player_turn_cycle(wm.self().playerTypePtr(),
                                                                       -wm.self().body(),
                                                                       wm.self().vel().r(),
                                                                       target.dist(self_pos),
                                                                       (target - self_pos).th(),
                                                                       0.0,
                                                                       false);

    int cycle_to_target = cycle_dash + cycle_turn;
    int cycle_to_target_backmove = cycle_dash_back_dash + cycle_turn_backmove;

    int cycle_turn_angle = FieldAnalyzer::predict_player_turn_cycle(wm.self().playerTypePtr(),
                                                                    (target - self_pos).th(),
                                                                    wm.self().playerTypePtr()->realSpeedMax(),
                                                                    0,
                                                                    target_angle,
                                                                    0.5,
                                                                    false);
    int cycle_turn_angle_backmove = FieldAnalyzer::predict_player_turn_cycle(wm.self().playerTypePtr(),
                                                                             -(target - self_pos).th(),
                                                                             wm.self().playerTypePtr()->realSpeedMax(),
                                                                             0,
                                                                             target_angle,
                                                                             0.5,
                                                                             false);
    int cycle_to_targetbody = cycle_to_target + cycle_turn_angle;
    int cycle_to_targetbody_backmove = cycle_to_target_backmove + cycle_turn_angle_backmove;

    if (target.dist(self_pos) > 5.0 // Magic Number
         && ( target_angle - wm.self().body() ).abs() < 90.0)
        cycle_to_targetbody_backmove = 1000;

    agent->debugClient().addLine(target, target + Vector2D::polar2vector(10, body_target));
    if(self_pos.dist(target) < 1
            &&((body_target - self_body).abs() < 20)
            && opp_min <= 2)
    {//target is near and body is ok
        dlog.addText(Logger::TEAM,"target is near and body is ok");
        if(self_pos.dist(target) < 0.5 && ball_pos.dist(target) > 20)
        {
            Body_TurnToAngle(body_target).execute(agent);
        }
        else if(self_pos.dist(target) < 0.3)
        {
            Body_TurnToAngle(body_target).execute(agent);
        }
        else if(self_pos.dist(target) < 0.5)
        {
            agent->doDash(50, (target - self_pos).th() - wm.self().body());
        }else
        {
            agent->doDash(100, (target - self_pos).th() - wm.self().body());
        }
    }
    else if(self_pos.dist(target) < 0.1)
    {//target is very near
        dlog.addText(Logger::TEAM,"target is very near");
        Body_TurnToAngle(body_target).execute(agent);
    }
    else if(self_pos.dist(target) < 0.5)
    {//target is near
        dlog.addText(Logger::TEAM,"target is near");
        double angle_diff = (self_body - body_target).abs();
        if(angle_diff > 15){
            dlog.addText(Logger::TEAM,__FILE__":Turn to targetbody");
            Body_TurnToAngle(body_target).execute(agent);
        }else{
            dlog.addText(Logger::TEAM,__FILE__":Do dash 50");
            agent->doDash(50, (target - self_pos).th() - wm.self().body());
        }
    }
    else if(ball_pos.dist(Vector2D(-52,0)) > 25)
    {//opp is'nt danger
        dlog.addText(Logger::TEAM,__FILE__":opp is'nt danger");
        if(!Body_GoToPoint(target, 0.5, Strategy::get_normal_dash_power(wm),1.5,1,false,20).execute(agent)){
            Body_TurnToAngle(body_target).execute(agent);
        }
    }
    else if(self_pos.dist(target) < 2.5
            && ((body_target - self_body).abs() < 20))
    {//Omni Dash to new target
        dlog.addText(Logger::TEAM,__FILE__":Omni Dash to new target");
        if(target.dist(self_pos) > 0.5)
            agent->doDash(100, (target - self_pos).th() - wm.self().body());
        else
            agent->doDash(50, (target - self_pos).th() - wm.self().body());
    }
    else
    {//opp is danger
        dlog.addText(Logger::TEAM,"");
        if(cycle_to_targetbody < opp_min + 2)
        {//can receive normaly
            dlog.addText(Logger::TEAM,__FILE__":can receive normaly");
            if(!Body_GoToPoint(target, 0.5, 100,1.5,1,false,20).execute(agent))
            {
                Body_TurnToAngle(body_target).execute(agent);
            }
        }
        else if(cycle_to_targetbody_backmove < opp_min + 2)
        {//can receive with backdash
            dlog.addText(Logger::TEAM,__FILE__":can receive with backdash");
            double angle_diff = (-self_body - (target - self_pos).th()).abs();
            if(angle_diff < 15)
            {
                dlog.addText(Logger::TEAM,"BackDash to target");
                agent->doDash(100, -180);
            }else
            {
                dlog.addText(Logger::TEAM,"BackTurn to target");
                Body_TurnToAngle(-(target - self_pos).th()).execute(agent);
            }
        }
        else
        {//can't receive normaly or with backdash
            dlog.addText(Logger::TEAM, "can't receive normaly or with backdash");
            if(target.dist(self_pos) < 2)
            {
                dlog.addText(Logger::TEAM, "target is near");
                if((body_target - self_body).abs() < 20){
                    dlog.addText(Logger::TEAM, "angle is ok");
                    if(self_pos.dist(target) < 1){
                        agent->doDash(50, (target - self_pos).th() - wm.self().body());
                    }else{
                        agent->doDash(100, (target - self_pos).th() - wm.self().body());
                    }
                }
                else
                {
                    dlog.addText(Logger::TEAM, "angle is'nt ok");
                    doGoToPointLookBall(agent, target, body_target, 0.3, 100);
                }
            }
            else
            {
                dlog.addText(Logger::TEAM, "target is far");
                doGoToPointLookBall(agent, target, body_target, 0.3, 100);
            }
        }
    }

    if(opp_min > 2 && wm.ball().velCount() < 2){
        agent->setNeckAction(new Neck_TurnToBallOrScan(1));
    }else{
        agent->setNeckAction( new Neck_TurnToBall() );
    }

    agent->debugClient().addCircle(target, 1);
    agent->debugClient().addMessage("hel(%.1f,%.1f) %.1f", target.x, target.y, body_target.degree());
    return true;
}
