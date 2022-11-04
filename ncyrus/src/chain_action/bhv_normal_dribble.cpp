// -*-c++-*-

/*!
  \file bhv_normal_dribble.cpp
  \brief normal dribble action that uses DribbleTable
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
#include <string>
#include "bhv_normal_dribble.h"

#include "action_chain_holder.h"
#include "action_chain_graph.h"
#include "cooperative_action.h"
#include "../data_extractor/offensive_data_extractor.h"

#include "dribble.h"
#include "short_dribble_generator.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_turn_to_goalie_or_scan.h>
#include <rcsc/action/view_synch.h>

#include <rcsc/player/intercept_table.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/soccer_intention.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/common/logger.h>
#include <rcsc/common/audio_memory.h>
#include <rcsc/common/server_param.h>
#include <rcsc/soccer_math.h>
#include <rcsc/math_util.h>
#include <rcsc/player/say_message_builder.h>
#include "../bhv_basic_move.h"
#include "bhv_pass_kick_find_receiver.h"
#include "../neck/next_pass_predictor.h"
#include "../neck/neck_decision.h"

//#define DEBUG_PRINT

using namespace rcsc;

class IntentionNormalDribble
        : public SoccerIntention {
private:
    Vector2D M_target_point; //!< trapped ball position

    int M_turn_step; //!< remained turn step
    int M_dash_step; //!< remained dash step

    std::string M_desc;
    GameTime M_last_execute_time; //!< last executed time

    NeckAction::Ptr M_neck_action;
    ViewAction::Ptr M_view_action;
    double M_dash_angle;
    Vector2D M_player_target_point;
public:

    IntentionNormalDribble( const Vector2D & target_point,
                            const int n_turn,
                            const int n_dash,
                            const GameTime & start_time ,
                            NeckAction::Ptr neck = NeckAction::Ptr(),
                            ViewAction::Ptr view = ViewAction::Ptr() )
        : M_target_point( target_point ),
          M_turn_step( n_turn ),
          M_dash_step( n_dash ),
          M_last_execute_time( start_time ),
          M_neck_action( neck ),
          M_view_action( view )
    { }

    IntentionNormalDribble( const Vector2D & target_point,
                            const int n_turn,
                            const int n_dash,
                            const GameTime & start_time ,
                            const char * desc
                            )
        : M_target_point( target_point ),
          M_turn_step( n_turn ),
          M_dash_step( n_dash ),
          M_last_execute_time( start_time ),
          M_desc(desc)
    {
        //        M_neck_action = ViewAction::Ptr();
        //        M_view_action = NeckAction::Ptr();
    }
    IntentionNormalDribble( const Vector2D & target_point,
                            const int n_turn,
                            const int n_dash,
                            const GameTime & start_time ,
                            const char * desc,
                            const double dash_angle,
                            const Vector2D & player_target_point
                            )
        : M_target_point( target_point ),
          M_turn_step( n_turn ),
          M_dash_step( n_dash ),
          M_last_execute_time( start_time ),
          M_desc(desc),
          M_dash_angle(dash_angle),
          M_player_target_point(player_target_point)
    {
        //        M_neck_action = ViewAction::Ptr();
        //        M_view_action = NeckAction::Ptr();
    }
    bool finished( const PlayerAgent * agent );

    bool execute( PlayerAgent * agent );

private:
    /*!
      \brief clear the action queue
     */
    void clear()
    {
        M_turn_step = M_dash_step = 0;
    }

    bool checkOpponent( const WorldModel & wm );
    bool doTurn( PlayerAgent * agent );
    bool doDash( PlayerAgent * agent );
};

/*-------------------------------------------------------------------*/
/*!

*/
bool
IntentionNormalDribble::finished( const PlayerAgent * agent )
{
    if ( M_turn_step + M_dash_step == 0 )
    {
        dlog.addText( Logger::DRIBBLE,
                      __FILE__": (finished) check finished. empty queue" );
        return true;
    }

    const WorldModel &wm = OffensiveDataExtractor::i().option.output_worldMode == FULLSTATE ? agent->fullstateWorld() : agent->world();

    if ( M_last_execute_time.cycle() + 1 != wm.time().cycle() )
    {
        dlog.addText( Logger::DRIBBLE,
                      __FILE__": finished(). last execute time is not match" );
        return true;
    }

    if ( wm.kickableTeammate()
         || wm.kickableOpponent() )
    {
        dlog.addText( Logger::DRIBBLE,
                      __FILE__": (finished). exist other kickable player" );
        return true;
    }

    if ( wm.ball().pos().dist2( M_target_point ) < std::pow( 1.0, 2 )
         && wm.self().pos().dist2( M_target_point ) < std::pow( 1.0, 2 ) )
    {
        dlog.addText( Logger::DRIBBLE,
                      __FILE__": (finished). reached target point" );
        return true;
    }

    const Vector2D ball_next = wm.ball().pos() + wm.ball().vel();
    if ( ball_next.absX() > ServerParam::i().pitchHalfLength() - 0.5
         || ball_next.absY() > ServerParam::i().pitchHalfWidth() - 0.5 )
    {
        dlog.addText( Logger::DRIBBLE,
                      __FILE__": (finished) ball will be out of pitch. stop intention." );
        return true;
    }

    if ( wm.audioMemory().passRequestTime() == agent->world().time() )
    {
        dlog.addText( Logger::DRIBBLE,
                      __FILE__": (finished). heard pass request." );
        return true;
    }

    // playmode is checked in PlayerAgent::parse()
    // and intention queue is totally managed at there.

    dlog.addText( Logger::DRIBBLE,
                  __FILE__": (finished). not finished yet." );

    return false;
}


/*-------------------------------------------------------------------*/
/*!

*/
bool
IntentionNormalDribble::execute( PlayerAgent * agent )
{
    if ( M_turn_step + M_dash_step == 0 )
    {
        dlog.addText( Logger::DRIBBLE,
                      __FILE__": (intention:execute) empty queue." );
        return false;
    }

    dlog.addText( Logger::DRIBBLE,
                  __FILE__": (intention:execute) turn=%d dash=%d",
                  M_turn_step, M_dash_step );

    const WorldModel &wm = OffensiveDataExtractor::i().option.output_worldMode == FULLSTATE ? agent->fullstateWorld() : agent->world();

//    if (wm.ball().posCount() == 0 && wm.ball().velCount() == 0){
//        Vector2D end_ball = wm.ball().inertiaPoint(M_turn_step + M_dash_step);
//        if(end_ball.dist(M_target_point) > 0.5){
//            M_target_point = end_ball;
//        }
//    }
    //
    // compare the current queue with other chain action candidates
    //

    if ( wm.self().isKickable()
         && M_turn_step <= 0 )
    {
        CooperativeAction::Ptr current_action( new Dribble( wm.self().unum(),
                                                            M_target_point,
                                                            wm.ball().vel().r(),
                                                            0,
                                                            M_turn_step,
                                                            M_dash_step,
                                                            "queuedDribble" ) );
        current_action->setIndex( 0 );
        current_action->setFirstDashPower( ServerParam::i().maxDashPower() );

//        ShortDribbleGenerator::instance().setQueuedAction( wm, current_action );

        ActionChainHolder::instance().update( agent );
        const ActionChainGraph & search_result = ActionChainHolder::i().graph();
        const CooperativeAction & first_action = search_result.getFirstAction();

        if ( first_action.category() != CooperativeAction::Dribble
             || ! first_action.targetPoint().equals( current_action->targetPoint() ) )
        {
            agent->debugClient().addMessage( "CancelDribbleQ" );
            dlog.addText( Logger::DRIBBLE,
                          __FILE__": (intention:execute) cancel. select other action." );
            return false;
        }
    }

    //
    //
    //

    if ( checkOpponent( wm ) )
    {
        dlog.addText( Logger::DRIBBLE,
                      __FILE__": (intention:execute). but exist opponent. cancel intention." );
        return false;
    }

    //
    // execute action
    //

    if ( M_turn_step > 0 )
    {
        if ( ! doTurn( agent ) )
        {
            dlog.addText( Logger::DRIBBLE,
                          __FILE__": (intention:exuecute) failed to turn. clear intention" );
            if( M_dash_step > 0 ){
                if ( ! doDash( agent ) )
                {
                    dlog.addText( Logger::DRIBBLE,
                                  __FILE__": (intention:execute) failed to dashA.  clear intention" );
                    this->clear();
                    return false;
                }
            }else{
                this->clear();
                return false;
            }
        }
    }
    else if ( M_dash_step > 0 )
    {
        if ( ! doDash( agent ) )
        {
            dlog.addText( Logger::DRIBBLE,
                          __FILE__": (intention:execute) failed to dashB.  clear intention" );
            this->clear();
            return false;
        }
    }
    else
    {
        dlog.addText( Logger::DRIBBLE,
                      __FILE__": (intention:execute). No command queue" );
        this->clear();
        return false;
    }

    dlog.addText( Logger::DRIBBLE,
                  __FILE__": (intention:execute). done" );
    agent->debugClient().addMessage( "NormalDribbleQ%d:%d", M_turn_step, M_dash_step );
    agent->debugClient().setTarget( M_target_point );


    if ( wm.gameMode().type() == GameMode::PenaltyTaken_){
        agent->setNeckAction(new Neck_TurnToGoalieOrScan(0));
        return true;
    }

    if ( ! M_view_action )
    {
        if ( wm.gameMode().type() != GameMode::PlayOn
             || M_turn_step + M_dash_step <= 1 )
        {
            dlog.addText( Logger::DRIBBLE,
                          __FILE__": (intention:execute) default view synch" );
            agent->debugClient().addMessage( "ViewSynch" );
            agent->setViewAction( new View_Synch() );
        }
        else
        {
            dlog.addText( Logger::DRIBBLE,
                          __FILE__": (intention:execute) default view normal" );
            agent->debugClient().addMessage( "ViewNormal" );
            agent->setViewAction( new View_Normal() );
        }
    }
    else
    {
        dlog.addText( Logger::DRIBBLE,
                      __FILE__": (intention:execute) registered view" );
        agent->debugClient().addMessage( "ViewRegisterd" );
        agent->setViewAction( M_view_action->clone() );
    }

    if ( ! M_neck_action )
    {
        dlog.addText( Logger::DRIBBLE,
                      __FILE__": (intention:execute) default turn_neck scan field" );
        agent->debugClient().addMessage( "NeckScan" );
        NeckDecisionWithBall().setNeck(agent, NeckDecisionType::dribbling);
    }
    else
    {
        dlog.addText( Logger::DRIBBLE,
                      __FILE__": (intention:execute) registered turn_neck" );
        agent->debugClient().addMessage( "NeckRegistered" );
        agent->setNeckAction( M_neck_action->clone() );
    }

    M_last_execute_time = wm.time();

    if(M_dash_step <= 2){
        Vector2D chain_target = Vector2D::INVALIDATED;
        int current_len = agent->effector().getSayMessageLength();
        int available_len = ServerParam::i().playerSayMsgSize() - current_len;

        if(wm.interceptTable()->selfReachCycle() < 3 ){ // && PrePassMessage::slength() <= available_len){ CYRUS_LIB
            const ActionChainGraph & chain_graph = ActionChainHolder::i().graph();
            const CooperativeAction & first_action = chain_graph.getFirstAction();
            switch (first_action.category()) {
            case CooperativeAction::Pass: {
                chain_target = first_action.targetPoint();
                if (wm.interceptTable()->selfReachCycle() <= 2) {
                    Bhv_PassKickFindReceiver(chain_graph).doSayPrePass(agent,first_action);
                }
                break;
            }
            }
        }
        current_len = agent->effector().getSayMessageLength();
        available_len = ServerParam::i().playerSayMsgSize() - current_len;
        if(available_len <= DribbleMessage::slength()){
            agent->addSayMessage(new DribbleMessage(M_target_point,M_turn_step + M_dash_step));
        }
    }else{
    agent->addSayMessage(new DribbleMessage(M_target_point,M_turn_step + M_dash_step));
    }



    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
IntentionNormalDribble::checkOpponent( const WorldModel & wm )
{
    const Vector2D ball_next = wm.ball().pos() + wm.ball().vel();

    /*--------------------------------------------------------*/
    // exist near opponent goalie in NEXT cycle
    if ( ball_next.x > ServerParam::i().theirPenaltyAreaLineX()
         && ball_next.absY() < ServerParam::i().penaltyAreaHalfWidth() )
    {
        const AbstractPlayerObject * opp_goalie = wm.getTheirGoalie();
        if ( opp_goalie
             && opp_goalie->distFromBall() < ( ServerParam::i().catchableArea()
                                               + ServerParam::i().defaultPlayerSpeedMax() )
             )
        {
            dlog.addText( Logger::DRIBBLE,
                          __FILE__": existOpponent(). but exist near opponent goalie" );
            this->clear();
            return true;
        }
    }

    const PlayerObject * nearest_opp = wm.getOpponentNearestToSelf( 5 );

    if ( ! nearest_opp )
    {
        dlog.addText( Logger::DRIBBLE,
                      __FILE__": existOppnent(). No opponent" );
        return false;
    }

    /*--------------------------------------------------------*/
    // exist very close opponent at CURRENT cycle
    if (  nearest_opp->distFromBall() < ServerParam::i().defaultKickableArea() + 0.2 )
    {
        dlog.addText( Logger::DRIBBLE,
                      __FILE__": existOpponent(). but exist kickable opp" );
        this->clear();
        return true;
    }

    /*--------------------------------------------------------*/
    // exist near opponent at NEXT cycle
    if ( nearest_opp->pos().dist( ball_next )
         < ServerParam::i().defaultPlayerSpeedMax() + ServerParam::i().defaultKickableArea() + 0.3 )
    {
        const Vector2D opp_next = nearest_opp->pos() + nearest_opp->vel();
        // oppopnent angle is known
        if ( nearest_opp->bodyCount() == 0
             || nearest_opp->vel().r() > 0.2 )
        {
            Line2D opp_line( opp_next,
                             ( nearest_opp->bodyCount() == 0
                               ? nearest_opp->body()
                               : nearest_opp->vel().th() ) );
            if ( opp_line.dist( ball_next ) > 1.2 )
            {
                // never reach
                dlog.addText( Logger::DRIBBLE,
                              __FILE__": existOpponent(). opp never reach." );
            }
            else if ( opp_next.dist( ball_next ) < 0.6 + 1.2 )
            {
                dlog.addText( Logger::DRIBBLE,
                              __FILE__": existOpponent(). but opp may reachable(1)." );
                this->clear();
                return true;
            }

            dlog.addText( Logger::DRIBBLE,
                          __FILE__": existOpponent(). opp angle is known. opp may not be reached." );
        }
        // opponent angle is not known
        else
        {
            if ( opp_next.dist( ball_next ) < 1.2 + 1.2 ) //0.6 + 1.2 )
            {
                dlog.addText( Logger::DRIBBLE,
                              __FILE__": existOpponent(). but opp may reachable(2)." );
                this->clear();
                return true;
            }
        }

        dlog.addText( Logger::DRIBBLE,
                      __FILE__": existOpponent(). exist near opp. but avoidable?" );
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
IntentionNormalDribble::doTurn( PlayerAgent * agent )
{
    if ( M_turn_step <= 0 )
    {
        return false;
    }

    const double default_dist_thr = 0.5;

    const WorldModel &wm = OffensiveDataExtractor::i().option.output_worldMode == FULLSTATE ? agent->fullstateWorld() : agent->world();

    --M_turn_step;

    Vector2D my_inertia = wm.self().inertiaPoint( M_turn_step + M_dash_step );
    AngleDeg target_angle = ( M_target_point - my_inertia ).th();
    if(M_desc.compare("shortBackDribble") == 0){
        target_angle += AngleDeg(180.0);
        target_angle = AngleDeg(target_angle);
//        if(target_angle.degree() > 360.0){
//            target_angle = target_angle - AngleDeg(360.0);
//        }
    }else if(M_desc.compare("shortDribbleAdvance") == 0){
        target_angle = M_dash_angle;
    }
    AngleDeg angle_diff = target_angle - wm.self().body();

    double dist_target2dashline = Line2D(wm.self().pos(),wm.self().body()).dist(M_target_point);

    if (dist_target2dashline < wm.self().playerType().kickableArea() - 0.3){
        dlog.addText( Logger::DRIBBLE,
                      __FILE__": (doTurn)  but already facing(dash line).");
        //        this->clear();
        return false;
    }

    dlog.addText( Logger::DRIBBLE,
                  __FILE__": (doTurn) turn to (%.2f, %.2f) moment=%f",
                  M_target_point.x, M_target_point.y, angle_diff.degree() );
    double max_trun_angle = wm.self().playerTypePtr()->effectiveTurn(ServerParam::i().maxMoment(), wm.self().vel().r());

    double tmp_angle_dif = angle_diff.abs();
    if(tmp_angle_dif > 180)
        tmp_angle_dif = 360 - tmp_angle_dif;
    if(tmp_angle_dif < -180)
        tmp_angle_dif = 360 + tmp_angle_dif;

    if(max_trun_angle > abs(tmp_angle_dif) && M_desc.compare("shortDribbleAdvance")==0){
        angle_diff = ( M_target_point - my_inertia ).th() - wm.self().body();
        target_angle = (M_target_point - my_inertia).th();
        dlog.addText(Logger::DRIBBLE," want to complite turn to target,max:%.1f,tmp:%.1f,ad:%.1f",max_trun_angle,tmp_angle_dif,angle_diff.degree());
        //////////////////////// baraye advanse bayad fekri beshe, vaghti mitoone ta toop becharkhe khob becharkhe ke noise toop tasiresh kam beshe
        /// hamintor lazem nist be andaze target turn bezane
    }

    Body_TurnToAngle(target_angle).execute(agent);
//    agent->doTurn( angle_diff );

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
IntentionNormalDribble::doDash( PlayerAgent * agent )
{
    if ( M_dash_step <= 0 )
    {
        return false;
    }

    const WorldModel &wm = OffensiveDataExtractor::i().option.output_worldMode == FULLSTATE ? agent->fullstateWorld() : agent->world();

    --M_dash_step;

    double dash_power
            = wm.self().getSafetyDashPower( ServerParam::i().maxDashPower() );
    if(M_dash_step > 0)
        dash_power = 100;
    double accel_mag = dash_power * wm.self().dashRate();

    Vector2D dash_accel = Vector2D::polar2vector( accel_mag, wm.self().body() );
    if(M_desc.compare("shortBackDribble")==0){
        accel_mag = ServerParam::i().maxDashPower() * wm.self().playerType().effortMax() * ServerParam::i().dashDirRate( (M_target_point - wm.self().pos()).th().degree() ) * wm.self().playerType().dashPowerRate();
        dash_accel = Vector2D::polar2vector( accel_mag, AngleDeg((M_target_point - wm.self().pos()).th() ));
        dlog.addText(Logger::DRIBBLE,"change dash accel for back,r:%.1f,d:%.1f",dash_accel.r(),dash_accel.th().degree());
    }

    Vector2D my_next = wm.self().pos() + wm.self().vel() + dash_accel;
    Vector2D ball_next = wm.ball().pos() + wm.ball().vel();
    Vector2D ball_next_rel = ( ball_next - my_next ).rotatedVector( - wm.self().body() );
    if(M_desc.compare("shortBackDribble")==0){
        ball_next_rel = ( ball_next - my_next ).rotatedVector( wm.self().body() );
    }
    double ball_next_dist = ball_next_rel.r();

    if ( ball_next_dist < ( wm.self().playerType().playerSize()
                            + ServerParam::i().ballSize() )
         )
    {
        dlog.addText( Logger::DRIBBLE,
                      __FILE__": (doDash) collision may occur. ball_dist = %f",
                      ball_next_dist );
        this->clear();
        return false;
    }

    if(M_desc.compare("shortDribbleAdvance") == 0){
        Vector2D end_ball = wm.ball().inertiaPoint(M_dash_step + 1);
        dlog.addCircle(Logger::DRIBBLE,end_ball,0.2,200,0,0,false);
        dlog.addCircle(Logger::DRIBBLE,M_target_point,0.2,0,200,0,false);
        dlog.addCircle(Logger::DRIBBLE,M_player_target_point,0.2,200,200,200,false);
        dlog.addText(Logger::DRIBBLE,"sda,dist to line:%.2f , %.2f",Line2D(wm.self().inertiaPoint(1),wm.self().body()).dist(end_ball), wm.self().playerType().kickableArea() - 0.1);
        if ( Line2D(wm.self().inertiaPoint(1),wm.self().body()).dist(end_ball) > wm.self().playerType().kickableArea() - 0.1 ){
            Vector2D change = end_ball - M_target_point;
            Vector2D new_player_target = M_player_target_point + change;
            AngleDeg dash_angle = (new_player_target - wm.self().pos()).th() - wm.self().body();
            agent->doDash( dash_power, dash_angle);
//            if(dash_angle.degree() > 0)
//                agent->doDash( dash_power, dash_angle + AngleDeg(10));
//            else
//                agent->doDash( dash_power, dash_angle + AngleDeg(-10));
            dlog.addText(Logger::DRIBBLE, "oriv dash for sDA,endB:%.1f,%.1f,mtar:%.1f,%.1f,ch:%.1f,%.1f,mptp:%.1f,%.1f,npt:%.1f,%.1f,A:%.1f",
                         end_ball.x,end_ball.y,M_target_point.x,M_target_point.y,change.x,change.y,M_player_target_point.x,M_player_target_point.y,new_player_target.x,new_player_target.y,dash_angle.degree());
            dlog.addCircle(Logger::DRIBBLE,new_player_target,0.1,100,0,100);
            return true;
        }
    }
    if ( ball_next_rel.absY() > wm.self().playerType().kickableArea() - 0.1 )
    {
        dlog.addText( Logger::DRIBBLE,
                      __FILE__": (doDash) next Y difference is over. y_diff = %f ballnex=(%.2f,%.2f) body=%.2f selfnext(%.1f,%.1f)",
                      ball_next_rel.absY(),ball_next.x,ball_next.y,wm.self().body().degree(),my_next.x,my_next.y );
        if(M_desc.compare("shortBackDribble")==0){
            agent->doDash( dash_power,(M_target_point - wm.self().pos()).th() - wm.self().body() );
            return true;
            this->clear();
            return false;
        }
        else{
            //            Vector2D player_tar = ;
            agent->doDash( dash_power,(M_target_point - wm.self().pos()).th() - wm.self().body() );
            return true;
        }

    }

    // this dash is the last of queue
    // but at next cycle, ball is NOT kickable
    if ( M_dash_step <= 0 )
    {
        if ( ball_next_dist > wm.self().playerType().kickableArea() - 0.15 )
        {
            dlog.addText( Logger::DRIBBLE,
                          __FILE__": (doDash) last dash. but not kickable at next. ball_dist=%f",
                          ball_next_dist );
            this->clear();
            return false;
        }
    }

    if ( M_dash_step > 0 )
    {
        // remain dash step. but next dash cause over run.
        if(M_desc.compare("shortBackDribble")==0){
        }else{

            AngleDeg ball_next_angle = ( ball_next - my_next ).th();
            if ( ( wm.self().body() - ball_next_angle ).abs() > 90.0
                 && ball_next_dist > wm.self().playerType().kickableArea() - 0.2 )
            {
                dlog.addText( Logger::DRIBBLE,
                              __FILE__": (doDash) dash. but run over. ball_dist=%f",
                              ball_next_dist );
                this->clear();
                return false;
            }
        }
    }

    dlog.addText( Logger::DRIBBLE,
                  __FILE__": (doDash) power=%.1f  accel_mag=%.2f",
                  dash_power, accel_mag );
    if(M_desc.compare("shortBackDribble")==0){
        agent->doDash( dash_power, (M_target_point - wm.self().pos()).th() - wm.self().body());
    }
    else{
        agent->doDash( dash_power );
    }

    return true;
}





/*-------------------------------------------------------------------*/
/*-------------------------------------------------------------------*/
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*!

*/
#include <rcsc/action/body_smart_kick.h>
#include <rcsc/action/body_kick_one_step.h>
Bhv_NormalDribble::Bhv_NormalDribble( const CooperativeAction & action,
                                      NeckAction::Ptr neck,
                                      ViewAction::Ptr view )
    : M_target_point( action.targetPoint() ),
      M_intermediate_pos( action.intermediatePoint()),
      M_first_ball_speed( action.firstBallSpeed() ),
      M_first_turn_moment( action.firstTurnMoment() ),
      M_first_dash_power( action.firstDashPower() ),
      M_first_dash_angle( action.firstDashAngle() ),
      M_total_step( action.durationStep() ),
      M_kick_step( action.kickCount() ),
      M_turn_step( action.turnCount() ),
      M_dash_step( action.dashCount() ),
      M_neck_action( neck ),
      M_view_action( view ),
      M_dec(action.description()),
      M_dash_angle(action.dribble_dash_angle()),
      M_player_target_point(action.getPlayerTargetPoint())
{
    if ( action.category() != CooperativeAction::Dribble )
    {
        M_target_point = Vector2D::INVALIDATED;
        M_total_step = 0;
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_NormalDribble::execute( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    if ( ! wm.self().isKickable() )
    {
        dlog.addText( Logger::DRIBBLE,
                      __FILE__": (execute). no kickable..." );
        return false;
    }

    if ( ! M_target_point.isValid()
         || M_total_step <= 0 )
    {
        dlog.addText( Logger::DRIBBLE,
                      __FILE__": (execute). illegal target point or illegal total step" );
#ifdef DEBUG_PRINT
        std::cerr << wm.self().unum() << ' ' << wm.time()
                  << " Bhv_NormalDribble::execute() illegal target point or illegal total step"
                  << std::endl;
#endif
        return false;
    }

    std::string desc = M_dec;
    bool is_2kick = false;
    if(desc.compare("shortDribbleAdvance2Kick")==0){
        is_2kick = true;
        ShortDribbleGenerator::last_cycle_double_dirbble = wm.time().cycle();
    }

    const ServerParam & SP = ServerParam::i();

    if ( M_kick_step > 0 )
    {
        Vector2D tar;
        if (is_2kick)
            tar = M_intermediate_pos;
        else
            tar = M_target_point;
        Vector2D first_vel = tar - wm.ball().pos();
        first_vel.setLength( M_first_ball_speed );
        bool can_kick = false;
        if(!Body_KickOneStep(tar,M_first_ball_speed,false).execute(agent)){
            dlog.addText(Logger::DRIBBLE, "cant one kick step");
            if(!Body_SmartKick(tar,M_first_ball_speed,M_first_ball_speed - 0.1,1).execute(agent)){
                dlog.addText(Logger::DRIBBLE, "cant smart kick");
                return false;
            }else{
                can_kick = true;
                dlog.addText(Logger::DRIBBLE, "can smart kick");
            }
        }else{
            can_kick = true;
            dlog.addText(Logger::DRIBBLE, "can one kick step");
        }
        const Vector2D kick_accel = first_vel - wm.ball().vel();
        const double kick_power = kick_accel.r() / wm.self().kickRate();
        const AngleDeg kick_angle = kick_accel.th() - wm.self().body();

        dlog.addText( Logger::DRIBBLE,
                      __FILE__": (execute). first kick: power=%f angle=%f, n_turn=%d n_dash=%d, fist_vel=(%.2f,%.2f)",
                      kick_power, kick_angle.degree(),
                      M_turn_step, M_dash_step,(wm.ball().pos() +first_vel).x,(wm.ball().pos()+ first_vel).y );

        if ( kick_power > SP.maxPower() + 1.0e-5 )
        {
            dlog.addText( Logger::DRIBBLE,
                          __FILE__": (execute) over the max power" );
#ifdef DEBUG_PRINT
            std::cerr << __FILE__ << ':' << __LINE__ << ':'
                      << wm.self().unum() << ' ' << wm.time()
                      << " over the max power " << kick_power << std::endl;
#endif
        }
        if(!can_kick)
            agent->doKick( kick_power, kick_angle );

        dlog.addCircle( Logger::DRIBBLE,
                        wm.ball().pos() + first_vel,
                        0.1,
                        "#0000ff" );
    }
    else if ( M_turn_step > 0 )
    {
        dlog.addText( Logger::DRIBBLE,
                      __FILE__": (execute). first turn: moment=%.2f, n_turn=%d n_dash=%d",
                      M_first_turn_moment,
                      M_turn_step, M_dash_step );

        agent->doTurn( M_first_turn_moment );

    }
    else if ( M_dash_step > 0 )
    {
        dlog.addText( Logger::DRIBBLE,
                      __FILE__": (execute). first dash: dash power=%.1f dir=%.1f",
                      M_first_dash_power, M_first_dash_angle.degree() );
        agent->doDash( M_first_dash_power, M_first_dash_angle );
    }
    else
    {
        dlog.addText( Logger::DRIBBLE,
                      __FILE__": (execute). no action steps" );
        std::cerr << __FILE__ << ':' << __LINE__ << ':'
                  << wm.self().unum() << ' ' << wm.time()
                  << " no action step." << std::endl;
        return false;
    }

    if(desc.compare("shortDribbleAdvance")==0){
        agent->setIntention( new IntentionNormalDribble( M_target_point,
                                                         M_turn_step,
                                                         M_total_step - M_turn_step - 1, // n_dash
                                                         wm.time(),M_dec,M_dash_angle,M_player_target_point) );

    }
    else if(desc.compare("shortBackDribble")==0){
        agent->setIntention( new IntentionNormalDribble( M_target_point,
                                                         M_turn_step,
                                                         M_total_step - M_turn_step - 1, // n_dash
                                                         wm.time(),M_dec) );

    }
    else if (is_2kick){
        // do nothing
    }
    else{
        agent->setIntention( new IntentionNormalDribble( M_target_point,
                                                         M_turn_step,
                                                         M_total_step - M_turn_step - 1, // n_dash
                                                         wm.time() ) );
    }

    agent->addSayMessage(new DribbleMessage(M_target_point,M_total_step));
    if ( ! M_view_action )
    {
        if ( M_turn_step + M_dash_step >= 3 )
        {
            agent->setViewAction( new View_Normal() );
        }
    }
    else
    {
        agent->setViewAction( M_view_action->clone() );
    }
    if ( ! M_neck_action )
    {
        NeckDecisionWithBall().setNeck(agent, NeckDecisionType::dribbling);
    }
    else
    {
        agent->setNeckAction( M_neck_action->clone() );
    }

    return true;
}
