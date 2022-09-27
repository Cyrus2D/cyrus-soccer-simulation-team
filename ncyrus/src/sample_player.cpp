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

#include "sample_player.h"
#include "bhv_basic_offensive_kick.h"
#include "strategy.h"
#include "chain_action/field_analyzer.h"
#include "move_def/bhv_basic_tackle.h"
#include "chain_action/action_chain_holder.h"
#include "sample_field_evaluator.h"
#include "chain_action/bhv_pass_kick_find_receiver.h"
#include "data_extractor/offensive_data_extractor.h"

#include "roles/soccer_role.h"

#include "sample_communication.h"

#include "setplay/bhv_penalty_kick.h"
#include "setplay/bhv_set_play.h"
#include "setplay/bhv_set_play_kick_in.h"
#include "setplay/bhv_set_play_indirect_free_kick.h"

#include "setplay/bhv_custom_before_kick_off.h"
#include "bhv_strict_check_shoot.h"

#include "neck/view_tactical.h"

#include "intention_receive.h"
#include "bhv_basic_move.h"
#include <rcsc/action/basic_actions.h>
#include <rcsc/action/bhv_emergency.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/body_intercept.h>
#include <rcsc/action/body_kick_one_step.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/view_synch.h>

#include <rcsc/formation/formation.h>
#include <rcsc/action/kick_table.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/say_message_builder.h>
#include <rcsc/player/audio_sensor.h>
// #include <rcsc/player/freeform_parser.h>
#include <rcsc/player/stamina_with_pointto.h>

#include <rcsc/common/basic_client.h>
#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/common/player_param.h>
#include <rcsc/common/audio_memory.h>
#include <rcsc/common/say_message_parser.h>
#include "move_off/bhv_unmark.h"
// #include <rcsc/common/free_message_parser.h>

#include "setting.h"
Setting * Setting::instance = nullptr;

#include <rcsc/param/param_map.h>
#include <rcsc/param/cmd_line_parser.h>
#include "neck/neck_decision.h"

#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>

using namespace rcsc;

/*-------------------------------------------------------------------*/
/*!

 */
SamplePlayer::SamplePlayer()
    : PlayerAgent(),
      M_communication(),
      M_field_evaluator( createFieldEvaluator() ),
      M_action_generator( createActionGenerator() )
{
    boost::shared_ptr< AudioMemory > audio_memory( new AudioMemory );

    M_worldmodel.setAudioMemory( audio_memory );

    //
    // set communication message parser
    //
    addSayMessageParser( SayMessageParser::Ptr( new MarkMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new BallMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new PassMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new PrePassMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new PreCrossMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new InterceptMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new GoalieMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new GoalieAndPlayerMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new OffsideLineMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new DefenseLineMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new WaitRequestMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new PassRequestMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new DribbleMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new BallGoalieMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new OnePlayerMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new OnePlayerMessageParser1( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new OnePlayerMessageParser2( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new TwoPlayerMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new TwoPlayerMessageParser01( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new TwoPlayerMessageParser02( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new ThreePlayerMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new ThreePlayerMessageParser001( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new ThreePlayerMessageParser002( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new ThreePlayerMessageParser011( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new ThreePlayerMessageParser012( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new ThreePlayerMessageParser022( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new SelfMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new TeammateMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new OpponentMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new BallPlayerMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new StaminaMessageParser( audio_memory ) ) );
    addSayMessageParser( SayMessageParser::Ptr( new RecoveryMessageParser( audio_memory ) ) );

    // addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 9 >( audio_memory ) ) );
    // addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 8 >( audio_memory ) ) );
    // addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 7 >( audio_memory ) ) );
    // addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 6 >( audio_memory ) ) );
    // addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 5 >( audio_memory ) ) );
    // addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 4 >( audio_memory ) ) );
    // addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 3 >( audio_memory ) ) );
    // addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 2 >( audio_memory ) ) );
    // addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 1 >( audio_memory ) ) );

    //
    // set freeform message parser
    //
    setFreeformParser( FreeformParser::Ptr( new FreeformParser( M_worldmodel ) ) );

    //
    // set action generators
    //
    // M_action_generators.push_back( ActionGenerator::Ptr( new PassGenerator() ) );

    //
    // set communication planner
    //
    M_communication = Communication::Ptr( new SampleCommunication() );
}

/*-------------------------------------------------------------------*/
/*!

 */
SamplePlayer::~SamplePlayer()
{

}

/*-------------------------------------------------------------------*/
/*!

 */
int SamplePlayer::player_port = 0;
bool
SamplePlayer::initImpl( CmdLineParser & cmd_parser )
{
    bool result = PlayerAgent::initImpl( cmd_parser );

    // read additional options
    result &= Strategy::instance().init( cmd_parser );

    rcsc::ParamMap my_params( "Additional options" );
#if 0
    std::string param_file_path = "params";
    param_map.add()
            ( "param-file", "", &param_file_path, "specified parameter file" );
#endif

    cmd_parser.parse( my_params );

    if ( cmd_parser.count( "help" ) > 0 )
    {
        my_params.printHelp( std::cout );
        return false;
    }

    if ( cmd_parser.failed() )
    {
        std::cerr << "player: ***WARNING*** detected unsuppprted options: ";
        cmd_parser.print( std::cerr );
        std::cerr << std::endl;
    }

    if ( ! result )
    {
        return false;
    }

    if ( ! Strategy::instance().read( config().configDir() ) )
    {
        std::cerr << "***ERROR*** Failed to read team strategy." << std::endl;
        return false;
    }

    if ( KickTable::instance().read( config().configDir() + "/kick-table" ) )
    {
        std::cerr << "Loaded the kick table: ["
                  << config().configDir() << "/kick-table]"
                  << std::endl;
    }

    OffensiveDataExtractor::active = config().dataExtract();
    //bhv_unmarkes::load_dnn();

    return true;
}

/*-------------------------------------------------------------------*/
/*!
  main decision
  virtual method in super class
 */
#include "move_def/bhv_basic_tackle.h"
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds
#include "calculate_offensive_opponents.h"
CalculateOffensiveOpponents* CalculateOffensiveOpponents::instance= nullptr;
void
SamplePlayer::actionImpl()
{
    SamplePlayer::player_port = this->config().port();
    Setting::i();
    Setting::i()->SetTeamName(this->world().theirTeamName());
    //
    // update strategy and analyzer
    //
    Strategy::instance().update( world() );
    FieldAnalyzer::instance().update( world() );
    if(FieldAnalyzer::isOxsy(world()))
        CalculateOffensiveOpponents::getInstance()->updatePlayers(world());

    //
    // prepare action chain
    //
    M_field_evaluator = createFieldEvaluator();
    M_action_generator = createActionGenerator();

//    OffensiveDataExtractor::i().update_history(this);
    ActionChainHolder::instance().setFieldEvaluator( M_field_evaluator );
    ActionChainHolder::instance().setActionGenerator( M_action_generator );

    if (!this->config().useSyncMode() && ServerParam::i().synchMode()){
        std::cout<<"defense training mode"<<std::endl;
        std::this_thread::sleep_for (std::chrono::seconds(1));
    }

    //
    // special situations (tackle, objects accuracy, intention...)
    //
    if ( doPreprocess() )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": preprocess done" );
        if ( M_communication )
        {
            M_communication->execute( this );
        }
        return;
    }
    //
    // update action chain
    //
    ActionChainHolder::instance().update( this );
    const ActionChainGraph &chain_graph = ActionChainHolder::i().graph();
    ActionChainGraph::debug_send_chain( this, chain_graph.getAllChain() );

    for (int i = 1; i<=11; i++){
        const AbstractPlayerObject * tm = world().ourPlayer(i);
        if(tm == nullptr || tm->unum() < 1)
            continue;
        dlog.addText(Logger::TEAM, "%d %d %.1f", i, tm->seenStaminaCount(),tm->seenStamina());
    }
    //
    // create current role
    //
    SoccerRole::Ptr role_ptr;
    {
        role_ptr = Strategy::i().createRole( world().self().unum(), world() );
        if ( ! role_ptr )
        {
            std::cerr << config().teamName() << ": "
                      << world().self().unum()
                      << " Error. Role is not registerd.\nExit ..."
                      << std::endl;
            M_client->setServerAlive( false );
            return;
        }
    }
    //
    // override execute if role accept
    //
    if ( role_ptr->acceptExecution( world() ) )
    {
        role_ptr->execute( this );
        StaminaWithPointto::doPointtoForStamina(this);

        return;
    }

    //
    // play_on mode
    //
    if ( world().gameMode().type() == GameMode::PlayOn )
    {
        role_ptr->execute( this );
        StaminaWithPointto::doPointtoForStamina(this);
        if ( M_communication )
        {
            M_communication->execute( this );
        }
        return;
    }


    //
    // penalty kick mode
    //
    if ( world().gameMode().isPenaltyKickMode() )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": penalty kick" );
        Bhv_PenaltyKick().execute( this );
        return;
    }

    //
    // other set play mode
    //
    Bhv_SetPlay().execute( this );
    StaminaWithPointto::doPointtoForStamina(this);
    if ( M_communication )
    {
        M_communication->execute( this );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
SamplePlayer::handleActionStart()
{

}

/*-------------------------------------------------------------------*/
/*!

 */
void
SamplePlayer::handleActionEnd()
{
    if ( world().self().posValid() )
    {
#if 0
        const ServerParam & SP = ServerParam::i();
        //
        // inside of pitch
        //

        // top,lower
        debugClient().addLine( Vector2D( world().ourOffenseLineX(),
                                         -SP.pitchHalfWidth() ),
                               Vector2D( world().ourOffenseLineX(),
                                         -SP.pitchHalfWidth() + 3.0 ) );
        // top,lower
        debugClient().addLine( Vector2D( world().ourDefenseLineX(),
                                         -SP.pitchHalfWidth() ),
                               Vector2D( world().ourDefenseLineX(),
                                         -SP.pitchHalfWidth() + 3.0 ) );

        // bottom,upper
        debugClient().addLine( Vector2D( world().theirOffenseLineX(),
                                         +SP.pitchHalfWidth() - 3.0 ),
                               Vector2D( world().theirOffenseLineX(),
                                         +SP.pitchHalfWidth() ) );
        //
        debugClient().addLine( Vector2D( world().offsideLineX(),
                                         world().self().pos().y - 15.0 ),
                               Vector2D( world().offsideLineX(),
                                         world().self().pos().y + 15.0 ) );

        // outside of pitch

        // top,upper
        debugClient().addLine( Vector2D( world().ourOffensePlayerLineX(),
                                         -SP.pitchHalfWidth() - 3.0 ),
                               Vector2D( world().ourOffensePlayerLineX(),
                                         -SP.pitchHalfWidth() ) );
        // top,upper
        debugClient().addLine( Vector2D( world().ourDefensePlayerLineX(),
                                         -SP.pitchHalfWidth() - 3.0 ),
                               Vector2D( world().ourDefensePlayerLineX(),
                                         -SP.pitchHalfWidth() ) );
        // bottom,lower
        debugClient().addLine( Vector2D( world().theirOffensePlayerLineX(),
                                         +SP.pitchHalfWidth() ),
                               Vector2D( world().theirOffensePlayerLineX(),
                                         +SP.pitchHalfWidth() + 3.0 ) );
        // bottom,lower
        debugClient().addLine( Vector2D( world().theirDefensePlayerLineX(),
                                         +SP.pitchHalfWidth() ),
                               Vector2D( world().theirDefensePlayerLineX(),
                                         +SP.pitchHalfWidth() + 3.0 ) );
#else
        // top,lower
        debugClient().addLine( Vector2D( world().ourDefenseLineX(),
                                         world().self().pos().y - 2.0 ),
                               Vector2D( world().ourDefenseLineX(),
                                         world().self().pos().y + 2.0 ) );

        //
        debugClient().addLine( Vector2D( world().offsideLineX(),
                                         world().self().pos().y - 15.0 ),
                               Vector2D( world().offsideLineX(),
                                         world().self().pos().y + 15.0 ) );
#endif
    }

    //
    // ball position & velocity
    //
    dlog.addText( Logger::WORLD,
                  "WM: BALL pos=(%lf, %lf), vel=(%lf, %lf, r=%lf, ang=%lf)",
                  world().ball().pos().x,
                  world().ball().pos().y,
                  world().ball().vel().x,
                  world().ball().vel().y,
                  world().ball().vel().r(),
                  world().ball().vel().th().degree() );


    dlog.addText( Logger::WORLD,
                  "WM: SELF move=(%lf, %lf, r=%lf, th=%lf)",
                  world().self().lastMove().x,
                  world().self().lastMove().y,
                  world().self().lastMove().r(),
                  world().self().lastMove().th().degree() );

    Vector2D diff = world().ball().rpos() - world().prevBall().rpos();
    dlog.addText( Logger::WORLD,
                  "WM: BALL rpos=(%lf %lf) prev_rpos=(%lf %lf) diff=(%lf %lf)",
                  world().ball().rpos().x,
                  world().ball().rpos().y,
                  world().prevBall().rpos().x,
                  world().prevBall().rpos().y,
                  diff.x,
                  diff.y );

    Vector2D ball_move = diff + world().self().lastMove();
    Vector2D diff_vel = ball_move * ServerParam::i().ballDecay();
    dlog.addText( Logger::WORLD,
                  "---> ball_move=(%lf %lf) vel=(%lf, %lf, r=%lf, th=%lf)",
                  ball_move.x,
                  ball_move.y,
                  diff_vel.x,
                  diff_vel.y,
                  diff_vel.r(),
                  diff_vel.th().degree() );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
SamplePlayer::handleServerParam()
{
    if ( KickTable::instance().createTables() )
    {
        std::cerr << world().teamName() << ' '
                  << world().self().unum() << ": "
                  << " KickTable created."
                  << std::endl;
    }
    else
    {
        std::cerr << world().teamName() << ' '
                  << world().self().unum() << ": "
                  << " KickTable failed..."
                  << std::endl;
        M_client->setServerAlive( false );
    }

}

/*-------------------------------------------------------------------*/
/*!

 */
void
SamplePlayer::handlePlayerParam()
{

}

/*-------------------------------------------------------------------*/
/*!

 */
void
SamplePlayer::handlePlayerType()
{

}

/*-------------------------------------------------------------------*/
/*!
  communication decision.
  virtual method in super class
 */
void
SamplePlayer::communicationImpl()
{
    if ( M_communication )
    {
        M_communication->execute( this );
    }
}

/*-------------------------------------------------------------------*/
/*!
 */
bool
SamplePlayer::doPreprocess()
{
    // check tackle expires
    // check self position accuracy
    // ball search
    // check queued intention
    // check simultaneous kick

    const WorldModel & wm = this->world();

    dlog.addText( Logger::TEAM,
                  __FILE__": (doPreProcess)" );

    //
    // freezed by tackle effect
    //
    if ( wm.self().isFrozen() )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": tackle wait. expires= %d",
                      wm.self().tackleExpires() );
        // face neck to ball
        this->setViewAction( new View_Tactical() );
        this->setNeckAction( new Neck_TurnToBallOrScan() );
        return true;
    }

    //
    // BeforeKickOff or AfterGoal. jump to the initial position
    //
    if ( wm.gameMode().type() == GameMode::BeforeKickOff
         || wm.gameMode().type() == GameMode::AfterGoal_ )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": before_kick_off" );
        Vector2D move_point =  Strategy::i().getPosition( wm.self().unum() );
        Bhv_CustomBeforeKickOff( move_point ).execute( this );
        this->setViewAction( new View_Tactical() );
        return true;
    }

    //
    // self localization error
    //
    if ( ! wm.self().posValid() )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": invalid my pos" );
        Bhv_Emergency().execute( this ); // includes change view
        return true;
    }

    //
    // ball localization error
    //
    const int count_thr = ( wm.self().goalie()
                            ? 10
                            : 5 );
    if ( wm.ball().posCount() > count_thr
         || ( wm.gameMode().type() != GameMode::PlayOn
              && wm.ball().seenPosCount() > count_thr + 10 ) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": search ball" );
        this->setViewAction( new View_Tactical() );
        Bhv_NeckBodyToBall().execute( this );
        return true;
    }

    //
    // set default change view
    //
    this->setViewAction( new View_Tactical() );

    //
    // check shoot chance
    //
    if ( doShoot() )
    {
        OffensiveDataExtractor::i().update_for_shoot(this, Bhv_StrictCheckShoot::target, Bhv_StrictCheckShoot::speed);
        return true;
    }

    //
    // check queued action
    //
    if ( this->doIntention() )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": do queued intention" );
        return true;
    }

    //
    // check simultaneous kick
    //
    if ( doForceKick() )
    {
        return true;
    }

    //
    // check pass message
    //
    if ( doHeardPassReceive() )
    {
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
SamplePlayer::doShoot()
{
    //	if(Bhv_BasicOffensiveKick().cr_shoot(this)){
    //		return true;
    //	}
    //	return false;
    const WorldModel & wm = this->world();

    if ( wm.gameMode().type() != GameMode::IndFreeKick_
         && wm.time().stopped() == 0
         && wm.self().isKickable()
         && Bhv_StrictCheckShoot(0.7).execute( this ) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": shooted" );

        // reset intention
        this->setIntention( static_cast< SoccerIntention * >( 0 ) );
        return true;
    }

    if ( wm.gameMode().type() != GameMode::IndFreeKick_
         && wm.time().stopped() == 0
         && wm.self().isKickable()
         && Bhv_StrictCheckShoot(0.4).execute( this ) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": shooted" );

        // reset intention
        this->setIntention( static_cast< SoccerIntention * >( 0 ) );
        return true;
    }

    if ( wm.gameMode().type() != GameMode::IndFreeKick_
         && wm.time().stopped() == 0
         && wm.self().isKickable()
         && Bhv_StrictCheckShoot(0.2).execute( this ) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": shooted" );

        // reset intention
        this->setIntention( static_cast< SoccerIntention * >( 0 ) );
        return true;
    }
    if ( wm.gameMode().type() != GameMode::IndFreeKick_
         && wm.time().stopped() == 0
         && wm.self().isKickable()
         && Bhv_StrictCheckShoot(0.0).execute( this ) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": shooted" );

        // reset intention
        this->setIntention( static_cast< SoccerIntention * >( 0 ) );
        return true;
    }
    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
SamplePlayer::doForceKick()
{
    const WorldModel & wm = this->world();

    if ( wm.gameMode().type() == GameMode::PlayOn
         && ! wm.self().goalie()
         && wm.self().isKickable()
         && wm.maybeKickableOpponent() )
    {

        if(Bhv_BasicTackle(0.7).execute(this)){
            return true;
        }
        dlog.addText( Logger::TEAM,
                      __FILE__": simultaneous kick" );
        this->debugClient().addMessage( "SimultaneousKick" );
        Vector2D goal_pos( ServerParam::i().pitchHalfLength(), 0.0 );

        if ( wm.self().pos().x > 36.0
             && wm.self().pos().absY() > 10.0 )
        {
            goal_pos.x = 50.0;
            dlog.addText( Logger::TEAM,
                          __FILE__": simultaneous kick cross type" );
        }else if(wm.self().pos().x > 36.0){
            if(wm.self().pos().y > 0)
                goal_pos.y = 4;
            else
                goal_pos.y = -4;
        }else if(wm.self().pos().x > 0){
            if(wm.teammatesFromSelf().size()>0){
                goal_pos=wm.teammatesFromSelf().front()->pos();
            }
        }else{
            if(wm.ourPlayer(9) != NULL && wm.ourPlayer(9)->unum() == 9)
                goal_pos = wm.ourPlayer(9)->pos();
        }
        Body_KickOneStep( goal_pos,
                          ServerParam::i().ballSpeedMax(), true
                          ).execute( this );
        this->setNeckAction( new Neck_ScanField() );
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
SamplePlayer::doHeardPassReceive()
{
    const WorldModel & wm = this->world();

    if ( wm.audioMemory().passTime() != wm.time()
         || wm.audioMemory().pass().empty()
         || wm.audioMemory().pass().front().receiver_ != wm.self().unum() )
    {
        return false;
    }

    if(wm.self().isKickable())
        return false;

    int self_min = wm.interceptTable()->selfReachCycle();
    int opp_min = wm.interceptTable()->opponentReachCycle();
    Vector2D first_ball = wm.ball().pos();
    Vector2D intercept_pos = wm.ball().inertiaPoint( self_min );
    Vector2D heard_pos = wm.audioMemory().pass().front().receive_pos_;
    int heard_cycle = wm.self().playerTypePtr()->cyclesToReachDistance(heard_pos.dist(wm.self().pos()));
    bool prepass_received = wm.audioMemory().pass().front().prepass;
    bool cross_pass = wm.audioMemory().pass().front().cross;
    int sender = wm.audioMemory().pass().front().sender_;
    int fastest_tm = wm.interceptTable()->fastestTeammate()->unum();
    Vector2D self_pos = wm.self().inertiaPoint(1);
    dlog.addText( Logger::INTERCEPT,
                  __FILE__":  (doHeardPassReceive) heard_pos(%.2f %.2f) intercept_pos(%.2f %.2f)",
                  heard_pos.x, heard_pos.y,
                  intercept_pos.x, intercept_pos.y );

    Line2D ballmove = Line2D(wm.ball().pos(),heard_pos);

    this->debugClient().addCircle(heard_pos,0.1);
    double min_opp_dist2target = 1000;
    if(wm.opponentsFromSelf().size() > 0){
        min_opp_dist2target = wm.getDistOpponentNearestTo(heard_pos,5);
    }
    if(prepass_received && wm.self().pos().x > wm.offsideLineX() - 0.5){
        return false;
    }
    if(prepass_received && cross_pass && min_opp_dist2target <= 3.0){
        this->debugClient().addMessage("hear PreCrossMSG");
        if(ballmove.dist(self_pos) < 1){
            IntentionReceive::gotoIntercept(this,heard_pos);
//            heard_pos = wm.ball().pos();
        }else {
            IntentionReceive::gotoIntercept(this,heard_pos);
        }

    }else if(prepass_received && cross_pass && min_opp_dist2target > 3.0){
        this->debugClient().addMessage("hear PreCrossMSG");
        IntentionReceive::gotoIntercept(this,heard_pos);
        heard_pos = heard_pos;
    }else if(prepass_received){
        this->debugClient().addMessage("hear PrePassMSG");
        IntentionReceive::gotoIntercept(this,heard_pos,true);
        SampleCommunication().sayUnmark(this);
    }else if( wm.maybeKickableTeammate() ){
        this->debugClient().addMessage("hear kickableTm:goto heard pos");
        this->debugClient().addCircle(heard_pos,0.1);
        IntentionReceive::gotoIntercept(this,heard_pos);
        this->setNeckAction( new Neck_TurnToBall() );
    }else if (Body_Intercept2022(heard_pos,2,first_ball).execute( this)){
        this->debugClient().addMessage("hear intercept2.0");
    }else if (Body_Intercept2022(heard_pos,4,first_ball).execute( this)){
        this->debugClient().addMessage("hear intercept4.0");
    }else if (Body_Intercept2022(heard_pos,6,first_ball).execute( this)){
        this->debugClient().addMessage("hear intercept6.0");
    }else if (self_min > 5
              && heard_cycle > 5){
        this->debugClient().addMessage("s,h > 5,gotoIntercept");
        IntentionReceive::gotoIntercept(this,heard_pos);
    }else if (Body_Intercept2022(false,first_ball).execute( this)){
        this->debugClient().addMessage("hear intercept");
    }else{
        this->debugClient().addMessage("hear gotoIntercept");
        IntentionReceive::gotoIntercept(this,heard_pos);
    }

    this->setIntention( new IntentionReceive( heard_pos,
                                              ServerParam::i().maxDashPower(),
                                              0.9,
                                              std::min(10,heard_cycle),
                                              wm.time(),
                                              wm.audioMemory().pass().front().sender_,
                                              prepass_received) );
    if(prepass_received)
        SampleCommunication().saySelf(this);
    NeckDecisionWithBall().setNeck(this, NeckDecisionType::intercept);

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
FieldEvaluator::ConstPtr
SamplePlayer::getFieldEvaluator() const
{
    return M_field_evaluator;
}

/*-------------------------------------------------------------------*/
/*!

 */
FieldEvaluator::ConstPtr
SamplePlayer::createFieldEvaluator() const
{
    return FieldEvaluator::ConstPtr( new SampleFieldEvaluator );
}


/*-------------------------------------------------------------------*/
/*!
 */
#include "actgen_cross.h"
#include "actgen_direct_pass.h"
#include "actgen_self_pass.h"
#include "actgen_strict_check_pass.h"
#include "actgen_short_dribble.h"
#include "actgen_simple_dribble.h"
#include "actgen_shoot.h"
#include "actgen_action_chain_length_filter.h"

ActionGenerator::ConstPtr
SamplePlayer::createActionGenerator() const
{
    CompositeActionGenerator * g = new CompositeActionGenerator();

    //
    // shoot
    //
    g->addGenerator( new ActGen_RangeActionChainLengthFilter
                     ( new ActGen_Shoot(),
                       2, ActGen_RangeActionChainLengthFilter::MAX ) );
    //
    //	//
    //	// strict check pass
    //	//
    g->addGenerator( new ActGen_MaxActionChainLengthFilter
                     ( new ActGen_StrictCheckPass(), 1 ) );
    //
    //	//
    //	// cross
    //	//
    g->addGenerator( new ActGen_MaxActionChainLengthFilter
                     ( new ActGen_Cross(), 1 ) );
    //
    //	//
    //	// direct pass
    //	//
    g->addGenerator( new ActGen_RangeActionChainLengthFilter
                     ( new ActGen_DirectPass(),
                       2, ActGen_RangeActionChainLengthFilter::MAX ) );

    //
    // short dribble
    //
    g->addGenerator( new ActGen_MaxActionChainLengthFilter
                     ( new ActGen_ShortDribble(), 1 ) );

    //	//
    //	// self pass (long dribble)
    //	//
    //    g->addGenerator( new ActGen_MaxActionChainLengthFilter
    //            ( new ActGen_SelfPass(), 1 ) );
    //
    //	//
    //	// simple dribble
    //	//
    g->addGenerator( new ActGen_RangeActionChainLengthFilter
                     ( new ActGen_SimpleDribble(),
                       2, ActGen_RangeActionChainLengthFilter::MAX ) );

    return ActionGenerator::ConstPtr( g );
}
