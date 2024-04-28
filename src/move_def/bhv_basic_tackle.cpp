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

#include "bhv_basic_tackle.h"

#include "../chain_action/tackle_generator.h"

#include "basic_actions/neck_turn_to_ball_or_scan.h"
#include "basic_actions/neck_turn_to_point.h"

#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/player/intercept_table.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/ray_2d.h>

//#define DEBUG_PRINT

using namespace rcsc;

/*-------------------------------------------------------------------*/
/*!

 */

double Bhv_BasicTackle::calc_takle_prob(const rcsc::WorldModel & wm, Vector2D bp){
    const ServerParam & SP = ServerParam::i();

    const int self_min = wm.interceptTable().selfStep();
    const int mate_min = wm.interceptTable().teammateStep();
    const int opp_min = wm.interceptTable().opponentStep();
    const int our_min = std::min(mate_min,self_min);

    const Vector2D end_ball = wm.ball().inertiaPoint(std::min(self_min,std::min(opp_min,mate_min)));
    Vector2D ballpos = wm.ball().pos();

    bool ball_in_our_goal = false;
    if(end_ball.x < -52){
        if(end_ball.dist(ballpos) > 0.1){
            Line2D ball_move = Line2D(ballpos,end_ball);
            Line2D goal_line = Line2D(Vector2D(-52.5,0),90);
            Vector2D intersect = ball_move.intersection(goal_line);
            if(intersect.isValid()){
                if(intersect.absY() < 7.5){
                    ball_in_our_goal = true;
                }
            }
        }
    }
    if(ball_in_our_goal)
        return 0.1;
    if(end_ball.x < -47 && end_ball.absY() < 8){
        if(opp_min <= 1)
            return 0.1;
        return 0.3;
    }
    if(bp.isValid())
        ballpos = bp;
    double prob = 0.9;
    if(wm.ball().vel().r()>0.05 || wm.ball().posCount() < 4){
        if(end_ball.absX() > 52.5 || end_ball.absY() > 34.0){
            if(end_ball.x < -52.5){
                const Ray2D ball_ray( ballpos, wm.ball().vel().th() );
                const Line2D goal_line( Vector2D( -SP.pitchHalfLength(), 10.0 ),
                                        Vector2D( -SP.pitchHalfLength(), -10.0 ) );
                const Vector2D intersect = ball_ray.intersection( goal_line );
                if ( intersect.isValid()
                     && intersect.absY() < SP.goalHalfWidth() + 1.0
                     && wm.ball().inertiaPoint(mate_min).x < -52)
                {
                    prob = 0.4;
                }
            }else if(end_ball.x > 52.5){
                const Ray2D ball_ray( ballpos, wm.ball().vel().th() );
                const Line2D goal_line( Vector2D( SP.pitchHalfLength(), 10.0 ),
                                        Vector2D( SP.pitchHalfLength(), -10.0 ) );
                const Vector2D intersect = ball_ray.intersection( goal_line );
                if ( intersect.isValid()
                     && intersect.absY() < SP.goalHalfWidth() + 1.0 )
                {
                    prob = 0.3;
                }
            }else{
                if(wm.lastKickerSide() == wm.ourSide()){
                    prob = 0.4;
                }else{
                    prob = 0.8;
                }
            }
        }else{
            if(ballpos.x > 35 && ballpos.absY() < 10 && ((ballpos - wm.self().pos()).th() - (Vector2D(52,0) - wm.self().pos()).th()).abs() < 30){
                prob = 0.5;
            }else if(ballpos.x > 35 && ballpos.absY() < 10){
                prob = 0.7;
            }
            else if(ballpos.x > wm.ourDefenseLineX() + 15){
                if (opp_min < our_min && wm.self().pos().dist(end_ball) > 3.0){
                    prob = 0.5;
                }else{
                    prob = 0.8;
                }
            }else{
                if (opp_min < our_min && wm.self().pos().dist(end_ball) > 3.0){
                    prob = 0.4;
                }else{
                    prob = 0.9;
                }
            }
        }
    }else{
        prob = 0.9;
    }
    return prob;
}

bool Bhv_BasicTackle::can_tackle( const rcsc::WorldModel & wm){
    const ServerParam & SP = ServerParam::i();
    const int self_min = wm.interceptTable().selfStep();
    const int mate_min = wm.interceptTable().teammateStep();
    const int opp_min = wm.interceptTable().opponentStep();

    const Vector2D self_reach_point = wm.ball().inertiaPoint( self_min );
    const Vector2D our_reach_point = wm.ball().inertiaPoint( std::min(self_min,mate_min));
    Vector2D ballpos = wm.ball().pos();
    bool ball_will_be_in_our_goal = false;

    if ( our_reach_point.x < -SP.pitchHalfLength() )
    {
        const Ray2D ball_ray( wm.ball().pos(), wm.ball().vel().th() );
        const Line2D goal_line( Vector2D( -SP.pitchHalfLength(), 10.0 ),
                                Vector2D( -SP.pitchHalfLength(), -10.0 ) );

        const Vector2D intersect = ball_ray.intersection( goal_line );
        if ( intersect.isValid()
             && intersect.absY() < SP.goalHalfWidth() + 1.0 )
        {
            ball_will_be_in_our_goal = true;

            dlog.addText( Logger::TEAM,
                          __FILE__": ball will be in our goal. intersect=(%.2f %.2f)",
                          intersect.x, intersect.y );
        }
    }


    if(wm.kickableOpponent() && (ballpos.x < 30 || self_min >= 3) ){
        return true;
    }
    if(ball_will_be_in_our_goal){
        return true;
    }
    if(opp_min < self_min - 3 && opp_min < mate_min - 3){
        return true;
    }
    if(self_min >= 5
            && wm.ball().pos().dist2( SP.theirTeamGoalPos() ) < std::pow( 10.0, 2 )
            && ( ( SP.theirTeamGoalPos() - wm.self().pos() ).th() - wm.self().body() ).abs() < 45.0
            && mate_min > opp_min + 1){
        return true;
    }
    if( wm.ball().pos().dist( SP.theirTeamGoalPos() ) < 15
            && self_min > opp_min && mate_min > opp_min
            && wm.self().body().degree() < 70){
        return true;
    }
    if(wm.ball().pos().dist( SP.theirTeamGoalPos() ) < 15
            && self_min > opp_min && mate_min > opp_min
            && wm.self().body().degree() < 70){
        return true;
    }
    if(wm.lastKickerSide() == wm.ourSide()){
        if(our_reach_point.x > 52.6
                && Line2D(ballpos,wm.ball().vel()).intersection(Line2D(Vector2D(52.5,0),90)).isValid()
                && Line2D(ballpos,wm.ball().vel()).intersection(Line2D(Vector2D(52.5,0),90)).absY() > 7.5 ){
            return true;
        }
        if(our_reach_point.absY() > 34.2){
            return true;
        }
    }
    if(ballpos.x > 35 && ballpos.absY() < 12 && opp_min < mate_min && opp_min < self_min){
        return true;
    }
    if(ballpos.x > 40 && ballpos.absY() < 8 && our_reach_point.absY() > 15){
        if( ((Vector2D(52,0) - wm.self().pos()).th() - wm.self().body()).abs() < 40)
            return true;
    }
    if(wm.ball().inertiaPoint(opp_min).x < -35 && opp_min < mate_min && opp_min < self_min && wm.ball().inertiaPoint(opp_min).absY() < 12){
        return true;
    }

    TackleGenerator::instance().generate(wm);
    const TackleGenerator::TackleResult & result
            = TackleGenerator::instance().bestResult( wm );
    if(result.score_ > 500 && self_min > 1)
        return true;
    return false;
}

bool
Bhv_BasicTackle::execute( PlayerAgent * agent )
{
    const ServerParam & SP = ServerParam::i();
    const WorldModel & wm = agent->world();

    bool use_foul = false;
    double tackle_prob = wm.self().tackleProbability();

    if ( agent->config().version() >= 14.0
         && wm.self().card() == NO_CARD
         && ( wm.ball().pos().x > SP.ourPenaltyAreaLineX() + 0.5
              || wm.ball().pos().absY() > SP.penaltyAreaHalfWidth() + 0.5 )
         && tackle_prob < wm.self().foulProbability() && ! wm.gameMode().isPenaltyKickMode() )
    {
        tackle_prob = wm.self().foulProbability();
        use_foul = true;
    }

    if ( tackle_prob < M_min_probability )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": failed. low tackle_prob=%.2f < %.2f",
                      wm.self().tackleProbability(),
                      M_min_probability );
        return false;
    }

    if ( can_tackle(wm))
    {
        // try tackle
        return executeV14( agent, use_foul );
    }
    else
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": Bhv_BasicTackle. not necessary" );
                return false;
    }

    // v14+

}

/*-------------------------------------------------------------------*/
/*!

 */
#include <rcsc/player/say_message_builder.h>
bool
Bhv_BasicTackle::executeV14( PlayerAgent * agent,
                             const bool use_foul )
{
    const WorldModel & wm = agent->world();

    TackleGenerator::instance().generate(wm);
    const TackleGenerator::TackleResult & result
            = TackleGenerator::instance().bestResult( wm );

    if(result.score_ == -1000)
        return false;
    Vector2D ball_next = wm.ball().pos() + result.ball_vel_;

    dlog.addText( Logger::TEAM,
                  __FILE__": (executeV14) %s angle=%.0f resultVel=(%.2f %.2f) ballNext=(%.2f %.2f)",
                  use_foul ? "foul" : "tackle",
                  result.tackle_angle_.degree(),
                  result.ball_vel_.x, result.ball_vel_.y,
                  ball_next.x, ball_next.y );
    agent->debugClient().addMessage( "Basic%s%.0f",
                                     use_foul ? "Foul" : "Tackle",
                                     result.tackle_angle_.degree() );

    double tackle_dir = ( result.tackle_angle_ - wm.self().body() ).degree();

    agent->doTackle( tackle_dir, use_foul );
    if(result.type == TackleGenerator::tackle_pass){
        agent->addSayMessage( new PassMessage( result.pass_unum, result.pass_target, agent->effector().queuedNextBallPos(),
                                               agent->effector().queuedNextBallVel() ) );
    }
    agent->setNeckAction( new Neck_TurnToPoint( ball_next ) );

    return true;
}
