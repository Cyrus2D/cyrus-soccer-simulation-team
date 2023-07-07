// -*-c++-*-

/*!
  \file sample_communication.cpp
  \brief sample communication planner Source File
 */

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

#include "sample_communication.h"

#include "strategy.h"

#include "move_def/bhv_mark_execute.h"
#include "move_def/bhv_mark_decision_greedy.h"

#include <rcsc/formation/formation.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/say_message_builder.h>
// #include <rcsc/player/freeform_parser.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/common/player_param.h>
#include <rcsc/common/audio_memory.h>
#include <rcsc/common/say_message_parser.h>
#include <rcsc/player/debug_client.h>
#include <cmath>

#include <map>
#include <random>

// #define DEBUG_PRINT
// #define DEBUG_PRINT_PLAYER_RECORD

using namespace rcsc;

namespace {

struct ObjectScore {
    const AbstractPlayerObject * player_;
    int number_;
    double score_;

    ObjectScore()
        : player_( static_cast< AbstractPlayerObject * >( 0 ) ),
          number_( -1 ),
          score_( -65535.0 )
    { }

    struct Compare {
        bool operator()( const ObjectScore & lhs,
                         const ObjectScore & rhs ) const
        {
            return lhs.score_ > rhs.score_;
        }
    };

    struct IllegalChecker {
        bool operator()( const ObjectScore & val ) const
        {
            return val.score_ <= 0.1;
        }
    };
};


struct AbstractPlayerSelfDistCmp {
    bool operator()( const AbstractPlayerObject * lhs,
                     const AbstractPlayerObject * rhs ) const
    {
        return lhs->distFromSelf() < rhs->distFromSelf();
    }
};

struct AbstractPlayerBallDistCmp {
    bool operator()( const AbstractPlayerObject * lhs,
                     const AbstractPlayerObject * rhs ) const
    {
        return lhs->distFromBall() < rhs->distFromBall();
    }
};


struct AbstractPlayerBallXDiffCmp {
private:
    AbstractPlayerBallXDiffCmp();
public:
    const double ball_x_;
    AbstractPlayerBallXDiffCmp( const double & ball_x )
        : ball_x_( ball_x )
    { }

    bool operator()( const AbstractPlayerObject * lhs,
                     const AbstractPlayerObject * rhs ) const
    {
        return std::fabs( lhs->pos().x - ball_x_ )
                < std::fabs( rhs->pos().x - ball_x_ );
    }
};

struct AbstractPlayerXCmp {
    bool operator()( const AbstractPlayerObject * lhs,
                     const AbstractPlayerObject * rhs ) const
    {
        return lhs->pos().x < rhs->pos().x;
    }
};

inline
double
distance_rate( const double & val,
               const double & variance )
{
    return std::exp( - std::pow( val, 2 ) / ( 2.0 * std::pow( variance, 2 ) ) );

}

inline
double
distance_from_ball( const AbstractPlayerObject * p,
                    const Vector2D & ball_pos,
                    const double & x_rate,
                    const double & y_rate )
{
    //return p->distFromBall();
    return std::sqrt( std::pow( ( p->pos().x - ball_pos.x ) * x_rate, 2 )
                      + std::pow( ( p->pos().y - ball_pos.y ) * y_rate, 2 ) ); // Magic Number
}

}

/*-------------------------------------------------------------------*/
/*!

 */
SampleCommunication::SampleCommunication()
    : M_current_sender_unum( Unum_Unknown ),
      M_next_sender_unum( Unum_Unknown ),
      M_ball_send_time( 0, 0 )
{
    for ( int i = 0; i < 12; ++i )
    {
        M_teammate_send_time[i].assign( 0, 0 );
        M_opponent_send_time[i].assign( 0, 0 );
    }

}

/*-------------------------------------------------------------------*/
/*!

 */
SampleCommunication::~SampleCommunication()
{

}

/*-------------------------------------------------------------------*/
/*!

 */
bool
SampleCommunication::execute( PlayerAgent * agent )
{
    if ( ! agent->config().useCommunication() )
    {
        return false;
    }



    updateCurrentSender( agent );

    attentiontoSomeone( agent );
    if(currentSenderUnum() != M_current_sender_unum)
        return false;

    const WorldModel & wm = agent->world();
    const bool penalty_shootout = wm.gameMode().isPenaltyKickMode();

    bool say_recovery = false;
    if ( wm.gameMode().type() != GameMode::PlayOn
         && ! penalty_shootout
         && currentSenderUnum() == wm.self().unum()
         && wm.self().recovery() < ServerParam::i().recoverInit() - 0.002 )
    {
        say_recovery = true;
        sayRecovery( agent );
    }

    if ( wm.gameMode().type() == GameMode::BeforeKickOff
         || wm.gameMode().type() == GameMode::AfterGoal_
         || penalty_shootout )
    {
        return say_recovery;
    }
    //	if( wm.interceptTable()->teammateReachCycle() < wm.interceptTable()->opponentReachCycle() && wm.ball().distFromSelf() < 25){
    //		saySelf( agent );
    //		//		sayUnmark(agent);
    //	}
#ifdef DEBUG_PRINT_PLAYER_RECORD
    const AudioMemory::PlayerRecord::const_iterator end = agent->world().audioMemory().playerRecord().end();
    for ( AudioMemory::PlayerRecord::const_iterator p = agent->world().audioMemory().playerRecord().begin();
          p != end;
          ++p )
    {
        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": PlayerRecord: time=[%d,%d] %s %d from %d",
                      p->first.cycle(), p->first.stopped(),
                      p->second.unum_ <= 11 ? "teammate" : "opponent",
                      p->second.unum_ <= 11 ? p->second.unum_ : p->second.unum_ - 11,
                      p->second.sender_ );
    }
#endif

#if 1
    sayBallAndPlayers( agent );
    sayStamina( agent );
#else
    sayBall( agent );
    sayGoalie( agent );
    saySelf( agent );
    sayPlayers( agent );
#endif

    return true;
}

bool SampleCommunication::sayUnmark(PlayerAgent * agent) {

//    return true;
//    if(M_current_sender_unum != agent->world().self().unum())
        return false;
        sayBallAndPlayers(agent);
    const int current_len = agent->effector().getSayMessageLength();
    const int available_len = ServerParam::i().playerSayMsgSize() - current_len;

    const WorldModel & wm = agent->world();
    bool rec_prepass= false;
    if(wm.audioMemory().passTime().cycle() == wm.time().cycle()
            && wm.audioMemory().pass().size() > 0
            && wm.audioMemory().pass().front().receiver_ == wm.self().unum())
            rec_prepass = true;
    if(std::rand()%3 == 0
            && !wm.kickableTeammate()
            && !rec_prepass){
        agent->debugClient().addMessage("dontsayunmark");
        return false;
    }
    agent->debugClient().addMessage("sayunmark");
    map<char,int> say_eval_base;
    say_eval_base['s'] = 32;
    say_eval_base['n'] = 16;
    say_eval_base['p'] = 8;
    say_eval_base['d'] = 4;
    say_eval_base['o'] = 2;

    std::vector<Vector2D> say_player;
    std::vector<int> say_unum;
    std::vector<int> say_count;
    std::vector<double> say_eval;
    std::vector<char> say_mod;

    say_player.push_back(agent->effector().queuedNextMyPos());
    say_unum.push_back(wm.self().unum());
    say_count.push_back(0);
    say_eval.push_back(say_eval_base['s']);
    say_mod.push_back('s');

    int fastest_tm = wm.interceptTable()->fastestTeammate()->unum();
    Vector2D ball_pos = wm.ball().inertiaPoint(wm.interceptTable()->teammateReachCycle());
    Vector2D self_pos = agent->effector().queuedNextMyPos();

    //opp drible
    for(int t=1;t<=11;t++){
        const AbstractPlayerObject * opp = wm.theirPlayer(t);
        if( opp == NULL
                || opp->unum() < 0
                || opp->posCount() > 2
                || opp->pos().dist(ball_pos) > 10
                || opp->pos().dist(self_pos) > 25
                ){
            continue;
        }
        say_player.push_back(opp->inertiaPoint(1));
        say_unum.push_back(opp->unum() + 11);
        say_count.push_back(opp->posCount());
        double eval = say_eval_base['d'] * (opp->pos().dist(ball_pos)/10.0) * 2.0;
        say_eval.push_back(eval);
        say_mod.push_back('d');
        dlog.addText(Logger::COMMUNICATION,"opp, dribble u:%d e:%.1f",opp->unum(), eval);
    }

    //opp near me
    for(int t=1;t<=11;t++){
        const AbstractPlayerObject * opp = wm.theirPlayer(t);
        if( opp == NULL
                || opp->unum() < 0
                || opp->posCount() > 2
                || opp->pos().dist(self_pos) > 10
                ){
            continue;
        }
        say_player.push_back(opp->inertiaPoint(1));
        say_unum.push_back(opp->unum() + 11);
        say_count.push_back(opp->posCount());
        double eval = say_eval_base['n']  * (opp->pos().dist(self_pos)/10.0) * 2.0;
        say_eval.push_back(eval);
        say_mod.push_back('n');
        dlog.addText(Logger::COMMUNICATION,"opp, nearme u:%d e:%.1f",opp->unum(), eval);
    }

    //other tm
    for(int t=1;t<=11;t++){
        const AbstractPlayerObject * tm = wm.ourPlayer(t);
        if( t == wm.self().unum()
                || t == fastest_tm
                || tm == NULL
                || tm->unum() < 0
                || tm->posCount() > 2
                || (tm->pos().dist(ball_pos) > 25
                    && tm->pos().dist(self_pos) > 25)
                ){
            continue;
        }
        say_player.push_back(tm->inertiaPoint(1));
        say_unum.push_back(tm->unum());
        say_count.push_back(tm->posCount());
        double eval = say_eval_base['o']  * (tm->pos().dist(ball_pos)/25.0) * 2.0;
        say_eval.push_back(eval);
        say_mod.push_back('o');
        dlog.addText(Logger::COMMUNICATION,"tm, other u:%d e:%.1f",tm->unum(), eval);
    }

    double pass_angle = (self_pos - ball_pos).th().degree();
    Line2D pass_line(ball_pos,self_pos);
    Sector2D pass_sec(ball_pos,0,ball_pos.dist(self_pos),pass_angle - 15,pass_angle + 15);
    //opp pass
    for(int t=1;t<=11;t++){
        const AbstractPlayerObject * opp = wm.theirPlayer(t);
        if( opp == NULL
                || opp->unum() < 0
                || opp->posCount() > 2
                || !pass_sec.contains(opp->pos())
                ){
            continue;
        }
        say_player.push_back(opp->inertiaPoint(1));
        say_unum.push_back(opp->unum() + 11);
        say_count.push_back(opp->posCount());
        double eval = say_eval_base['p']  * (20 - pass_line.dist(opp->pos())) * 2.0;
        say_eval.push_back(eval);
        say_mod.push_back('p');
        dlog.addText(Logger::COMMUNICATION,"opp, pass u:%d e:%.1f",opp->unum(), eval);
    }

    for (int i = 0; i < say_player.size(); i++) {
        dlog.addText(Logger::COMMUNICATION,
                     __FILE__"say Unmark: index=%d , unum=%d , player=(%.2f,%.2f) mod=%c eval=%.1f", i,
                     say_unum[i], say_player[i].x, say_player[i].y,say_mod[i],say_eval[i]);
    }

    for(int i = 1; i < say_player.size(); i++){
        for(int j = i + 1; j < say_player.size(); j++){
            if(say_eval[i] < say_eval[j]){
                swap(say_player[i],say_player[j]);
                swap(say_unum[i],say_unum[j]);
                swap(say_count[i],say_count[j]);
                swap(say_eval[i],say_eval[j]);
                swap(say_mod[i],say_mod[j]);
            }
        }
    }

    for (int i = 0; i < say_player.size(); i++) {
        dlog.addText(Logger::COMMUNICATION,
                     __FILE__"say Unmark sorted: index=%d , unum=%d , player=(%.2f,%.2f) mod=%c eval=%.1f", i,
                     say_unum[i], say_player[i].x, say_player[i].y,say_mod[i],say_eval[i]);
    }
    std::vector<Vector2D> new_say_player;
    std::vector<int> new_say_unum;
    std::vector<int> new_say_count;

    int number2say = 0;
    if(available_len >= ThreePlayerMessage::slength()){
        number2say = 2;
    }else if(available_len >= TwoPlayerMessage::slength()){
        number2say = 2;
    }else if(available_len >= SelfMessage::slength()){
        number2say = 1;
    }else{
        return false;
    }
    number2say = std::min(number2say,(int)say_player.size());

    dlog.addText(Logger::COMMUNICATION,"num2s %d",number2say);
    int number = 0;
    for(int i = 0; i < say_player.size(); i++){
        bool used = false;
        for(int j = 0; j < new_say_player.size(); j++){
            if(new_say_unum[j] == say_unum[i])
                used = true;
        }
        if(!used){
            new_say_player.push_back(say_player[i]);
            new_say_count.push_back(say_count[i]);
            new_say_unum.push_back(say_unum[i]);
            number += 1;
        }
        if(number2say == number)
            break;
    }
    number2say = std::min(number2say,number);
    dlog.addText(Logger::COMMUNICATION,"num2s %d",number2say);

    for(int i=0;i<new_say_count.size();i++){
        dlog.addText(Logger::COMMUNICATION,"new %d %d",new_say_unum[i],new_say_count[i]);
    }
    if(number2say==0)
        return false;
    for(int i=1;i<new_say_player.size();i++){
        for(int j = i + 1;j<new_say_player.size();j++){
            if(new_say_count[i] > new_say_count[j]){
                swap(new_say_count[i],new_say_count[j]);
                swap(new_say_player[i],new_say_player[j]);
                swap(new_say_unum[i],new_say_unum[j]);
            }
        }
    }

    for(int i=0;i<new_say_count.size();i++){
        dlog.addText(Logger::COMMUNICATION,"newsort %d %d",new_say_unum[i],new_say_count[i]);
    }
    /*if(number2say == 3){
        if(new_say_count[0] == 0 && new_say_count[1] == 0 && new_say_count[2] == 0){
            agent->addSayMessage(new ThreePlayerMessage(new_say_unum[0],new_say_player[0],new_say_unum[1],new_say_player[1],new_say_unum[2],new_say_player[2]));
        }else if(new_say_count[0] == 0 && new_say_count[1] == 0 && new_say_count[2] == 1){
            agent->addSayMessage(new ThreePlayerMessage001(new_say_unum[0],new_say_player[0],new_say_unum[1],new_say_player[1],new_say_unum[2],new_say_player[2]));
        }else if(new_say_count[0] == 0 && new_say_count[1] == 0 && new_say_count[2] == 2){
            agent->addSayMessage(new ThreePlayerMessage002(new_say_unum[0],new_say_player[0],new_say_unum[1],new_say_player[1],new_say_unum[2],new_say_player[2]));
        }else if(new_say_count[0] == 0 && new_say_count[1] == 1 && new_say_count[2] == 1){
            agent->addSayMessage(new ThreePlayerMessage011(new_say_unum[0],new_say_player[0],new_say_unum[1],new_say_player[1],new_say_unum[2],new_say_player[2]));
        }else if(new_say_count[0] == 0 && new_say_count[1] == 1 && new_say_count[2] == 2){
            agent->addSayMessage(new ThreePlayerMessage012(new_say_unum[0],new_say_player[0],new_say_unum[1],new_say_player[1],new_say_unum[2],new_say_player[2]));
        }else if(new_say_count[0] == 0 && new_say_count[1] == 2 && new_say_count[2] == 2){
            agent->addSayMessage(new ThreePlayerMessage022(new_say_unum[0],new_say_player[0],new_say_unum[1],new_say_player[1],new_say_unum[2],new_say_player[2]));
        }
        agent->debugClient().addMessage("say %d %d %d",new_say_unum[0],new_say_unum[1],new_say_unum[2]);
    }else */if(number2say >= 2){
        if(new_say_unum[0] == wm.self().unum()){
            agent->addSayMessage(new SelfMessage(agent->effector().queuedNextSelfPos(),agent->effector().queuedNextMyBody(),wm.self().stamina()));
            if(new_say_count[1] == 0){
                agent->addSayMessage(new OnePlayerMessage(new_say_unum[1],new_say_player[1]));
            }else if(new_say_count[1] == 1){
                agent->addSayMessage(new OnePlayerMessage1(new_say_unum[1],new_say_player[1]));
            }else if(new_say_count[1] == 2){
                agent->addSayMessage(new OnePlayerMessage2(new_say_unum[1],new_say_player[1]));
            }
        }else{
            if(new_say_count[0] == 0 && new_say_count[1] == 0){
                agent->addSayMessage(new TwoPlayerMessage(new_say_unum[0],new_say_player[0],new_say_unum[1],new_say_player[1]));
            }else if(new_say_count[0] == 0 && new_say_count[1] == 1){
                agent->addSayMessage(new TwoPlayerMessage01(new_say_unum[0],new_say_player[0],new_say_unum[1],new_say_player[1]));
            }else if(new_say_count[0] == 0 && new_say_count[1] == 2){
                agent->addSayMessage(new TwoPlayerMessage02(new_say_unum[0],new_say_player[0],new_say_unum[1],new_say_player[1]));
            }
        }

        agent->debugClient().addMessage("say %d %d",new_say_unum[0],new_say_unum[1]);
    }else if(number2say == 1){
        agent->addSayMessage(new SelfMessage(agent->effector().queuedNextSelfPos(),agent->effector().queuedNextMyBody(),wm.self().stamina()));
        agent->debugClient().addMessage("say %d",new_say_unum[0]);
    }


    return true;
}
/*-------------------------------------------------------------------*/
/*!

 */
void
SampleCommunication::updateCurrentSender( const PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();
//    M_current_sender_unum = wm.self().unum();
//    return;

    if ( agent->effector().getSayMessageLength() > 0 )
    {
        M_current_sender_unum = wm.self().unum();
        return;
    }

    int val = ( wm.time().stopped() > 0
                ? wm.time().cycle() + wm.time().stopped()
                : wm.time().cycle() );

    int self_min = wm.interceptTable()->selfReachCycle();
    int mate_min = wm.interceptTable()->teammateReachCycle();
    if ( !Strategy::i().isDefSit(wm,wm.self().unum())){
        Vector2D ball = wm.ball().inertiaPoint(std::min(self_min,mate_min));
        vector<int> unums;
        for(int i=1;i<=11;i++){
            Strategy::PostLine pl = Strategy::i().tm_Line(i);
            if(ball.x > 10){
                if(pl == Strategy::PostLine::golie
                        || pl == Strategy::PostLine::back)
                    continue;
            }else if(ball.x > -25){
                if(pl == Strategy::PostLine::golie)
                    continue;
            }
            unums.push_back(i);
        }
        if(unums.size() > 0){
            int current = val % unums.size();
            int next = ( val + 1 ) % unums.size();

            M_current_sender_unum = unums[current];
            M_next_sender_unum = unums[next];

            return;
        }
    }

    // if ( wm.self().distFromBall() > ServerParam::i().audioCutDist() + 30.0 )
    // {
    //     M_current_sender_unum = wm.self().unum();
    //     return;
    // }

    M_current_sender_unum = Unum_Unknown;
    std::vector< int > candidate_unum;
    //    candidate_unum.reserve( 11 );

    Vector2D ball_pos = wm.ball().pos();
    if(Strategy::i().isDefSit(wm,wm.self().unum())){
        ball_pos = wm.ball().inertiaPoint(wm.interceptTable()->opponentReachCycle());
        //		M_current_sender_unum = bhv_mark_execute().get_mark_sender(wm);
        for ( int unum = 1; unum <= 8; ++unum )
        {
//            Vector2D tm_home_pos = Strategy::i().getPosition(unum);
//
//            if(tm_home_pos.dist(ball_pos) < 35){
                candidate_unum.push_back( unum );
//            }

        }
        if(candidate_unum.size()==0){
            if(wm.self().unum() == 6)
                M_current_sender_unum = 6;
            if(wm.self().unum() == 7)
                M_current_sender_unum = 7;
            return;
        }

        int current = val % candidate_unum.size();
        int next = ( val + 1 ) % candidate_unum.size();

        M_current_sender_unum = candidate_unum[current];
        M_next_sender_unum = candidate_unum[next];

        return;
    }

    if ( wm.ball().pos().x < -10.0
         || wm.gameMode().type() != GameMode::PlayOn )
    {
        //
        // all players
        //
        for ( int unum = 1; unum <= 11; ++unum )
        {
            Vector2D tm_home_pos = Strategy::i().getPosition(unum);

            if(tm_home_pos.dist(ball_pos) < 35){
                candidate_unum.push_back( unum );
            }

        }
    }
    else
    {
        //
        // exclude goalie
        //
        const int goalie_unum = ( wm.ourGoalieUnum() != Unum_Unknown
                ? wm.ourGoalieUnum()
                : Strategy::i().goalieUnum() );

        for ( int unum = 1; unum <= 11; ++unum )
        {
            if ( unum != goalie_unum )
            {
                Vector2D tm_home_pos = Strategy::i().getPosition(unum);

                if(tm_home_pos.dist(ball_pos) < 35){
                    candidate_unum.push_back( unum );
                }
#if 0
                dlog.addText( Logger::COMMUNICATION,
                              __FILE__": (updateCurrentSender) field_player unum=%d",
                              candidate_unum.back() );
#endif
            }
        }
    }

    if(candidate_unum.size()==0){
        if(wm.self().unum()==6)
            M_current_sender_unum = 6;
        if(wm.self().unum()==7)
            M_current_sender_unum = 7;
        return;
    }

    int current = val % candidate_unum.size();
    int next = ( val + 1 ) % candidate_unum.size();

    M_current_sender_unum = candidate_unum[current];
    M_next_sender_unum = candidate_unum[next];

#ifdef DEBUG_PRINT
    dlog.addText( Logger::COMMUNICATION,
                  __FILE__": (updateCurrentSender) time=%d size=%d index=%d current=%d next=%d",
                  wm.time().cycle(),
                  candidate_unum.size(),
                  current,
                  M_current_sender_unum,
                  M_next_sender_unum );
#endif

}

/*-------------------------------------------------------------------*/
/*!

 */
void
SampleCommunication::updatePlayerSendTime( const WorldModel & wm,
                                           const SideID side,
                                           const int unum )
{
    if ( unum < 1 || 11 < unum )
    {
        std::cerr << __FILE__ << ':' << __LINE__
                  << ": Illegal player number. " << unum
                  << std::endl;
        return;
    }

    if ( side == wm.ourSide() )
    {
        M_teammate_send_time[unum] = wm.time();
    }
    else
    {
        M_opponent_send_time[unum] = wm.time();
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
SampleCommunication::shouldSayBall( const PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    if ( wm.ball().seenPosCount() > 0
         || wm.ball().seenVelCount() > 2 )
    {
        return false;
    }

    if ( wm.gameMode().type() != GameMode::PlayOn )
    {
        if ( agent->effector().queuedNextBallVel().r2() < std::pow( 0.5, 2 ) )
        {
            return false;
        }
    }

    if ( wm.kickableTeammate() )
    {
        return false;
    }

    bool ball_vel_changed = false;

    const double cur_ball_speed = wm.ball().vel().r();

    // const BallObject::State * prev_ball_state = wm.ball().getState( 1 );
    if ( wm.prevBall().velValid() )
    {
        const double prev_ball_speed = wm.prevBall().vel().r();

        double angle_diff = ( wm.ball().vel().th() - wm.prevBall().vel().th() ).abs();

        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": (shouldSayBall) check ball vel" );
        dlog.addText( Logger::COMMUNICATION,
                      "prev_vel=(%.2f %.2f) r=%.3f",
                      wm.prevBall().vel().x, wm.prevBall().vel().y,
                      prev_ball_speed );
        dlog.addText( Logger::COMMUNICATION,
                      "cur_vel=(%.2f %.2f) r=%.3f",
                      wm.ball().vel().x, wm.ball().vel().y,
                      cur_ball_speed );

        if ( cur_ball_speed > prev_ball_speed + 0.1
             || ( prev_ball_speed > 0.5
                  && cur_ball_speed < prev_ball_speed * ServerParam::i().ballDecay() * 0.5 )
             || ( prev_ball_speed > 0.5         // Magic Number
                  && angle_diff > 20.0 ) ) // Magic Number
        {
            dlog.addText( Logger::COMMUNICATION,
                          __FILE__": (shouldSayBall) ball vel changed." );
            ball_vel_changed = true;
        }
    }

    if ( ball_vel_changed
         && wm.lastKickerSide() != wm.ourSide()
         && ! wm.kickableOpponent() )
    {
        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": (shouldSayBall) ball vel changed. opponent kicked. no opponent kicker" );
        return true;
    }

    if ( wm.self().isKickable() )
    {
        if ( agent->effector().queuedNextBallKickable() )
        {
            if ( cur_ball_speed < 1.0 )
            {
                return false;
            }
        }

        if ( ball_vel_changed
             && agent->effector().queuedNextBallVel().r() > 1.0 )
        {
            dlog.addText( Logger::COMMUNICATION,
                          __FILE__": (shouldSayBall) kickable. ball vel changed" );
            return true;
        }

        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": (shouldSayBall) kickable, but no say" );
        return false;
    }


    const PlayerObject * ball_nearest_teammate = NULL;
    const PlayerObject * second_ball_nearest_teammate = NULL;

    for ( PlayerObject::Cont::const_iterator t = wm.teammatesFromBall().begin(),
              end = wm.teammatesFromBall().end();
          t != end;
          ++t )
    {
        if ( (*t)->isGhost() || (*t)->posCount() >= 10 ) continue;

        if ( ! ball_nearest_teammate )
        {
            ball_nearest_teammate = *t;
        }
        else if ( ! second_ball_nearest_teammate )
        {
            second_ball_nearest_teammate = *t;
            break;
        }
    }

    int our_min = std::min( wm.interceptTable()->selfReachCycle(),
                            wm.interceptTable()->teammateReachCycle() );
    int opp_min = wm.interceptTable()->opponentReachCycle();

    //
    // I am the nearest player to ball
    //
    if ( ! ball_nearest_teammate
         || ( ball_nearest_teammate->distFromBall() > wm.ball().distFromSelf() - 3.0 )  )
    {
        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": (shouldSayBall) maybe nearest to ball" );
        //return true;

        if ( ball_vel_changed
             || ( opp_min <= 1 && cur_ball_speed > 1.0 ) )
        {
            dlog.addText( Logger::COMMUNICATION,
                          __FILE__": (shouldSayBall) nearest to ball. ball vel changed?" );
            return true;
        }
    }

    if ( ball_nearest_teammate
         && wm.ball().distFromSelf() < 20.0
         && 1.0 < ball_nearest_teammate->distFromBall()
         && ball_nearest_teammate->distFromBall() < 6.0
         && ( opp_min <= our_min + 1
              || ball_vel_changed ) )
    {
        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": (shouldSayBall) support nearest player" );
        return true;
    }

    {
        if (!wm.self().goalie()){
            Vector2D ball_vel = wm.ball().vel();
            Vector2D ball_pos = wm.ball().pos();
            Line2D goal_line = Line2D(Vector2D(-52.5, -10), Vector2D(-52.5, +10));
            Line2D ball_move_line = Line2D(ball_pos, ball_vel.th());
            Vector2D intersection = goal_line.intersection(ball_move_line);
            Vector2D final_ball_pos = wm.ball().inertiaFinalPoint();
            if (ball_vel.r() > 0.3 && final_ball_pos.x < -40 && intersection.isValid() && intersection.absY() < 10.0){
                if (wm.interceptTable()->fastestTeammate() != nullptr && wm.interceptTable()->fastestTeammate()->unum() > 0){
                    if (wm.interceptTable()->fastestTeammate()->goalie()){
                        return true;
                    }
                }
            }
        }
    }
#if 0
    if ( ball_nearest_teammate
         && second_ball_nearest_teammate
         && ( ball_nearest_teammate->distFromBall()  // the ball nearest player
              > ServerParam::i().visibleDistance() - 0.5 ) // over vis dist
         && ( second_ball_nearest_teammate->distFromBall() > wm.ball().distFromSelf() - 5.0 )  // I am second
         )
    {
        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": (shouldSayBall) second nearest to ball" );
        return true;
    }
#endif
    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
SampleCommunication::shouldSayOpponentGoalie( const PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    const AbstractPlayerObject * goalie = wm.getTheirGoalie();

    if ( ! goalie )
    {
        return false;
    }

    if ( goalie->seenPosCount() == 0
         && goalie->bodyCount() == 0
         && goalie->unum() != Unum_Unknown
         && goalie->unumCount() == 0
         && goalie->distFromSelf() < 25.0
         && 53.0 - 16.0 < goalie->pos().x
         && goalie->pos().x < 52.5
         && goalie->pos().absY() < 20.0 )
    {
        Vector2D goal_pos = ServerParam::i().theirTeamGoalPos();
        Vector2D ball_next = wm.ball().pos() + wm.ball().vel();

        if ( ball_next.dist2( goal_pos ) < std::pow( 18.0, 2 ) )
        {
            return true;
        }
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
SampleCommunication::goalieSaySituation( const rcsc::PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    if ( ! wm.self().goalie() )
    {
        return false;
    }

    int player_count = 0;

    for ( AbstractPlayerObject::Cont::const_iterator p = wm.allPlayers().begin(),
              end = wm.allPlayers().end();
          p != end;
          ++p )
    {
        if ( (*p)->unumCount() == 0 )
        {
            ++player_count;
        }
    }

    if ( player_count >= 5 )
    {
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */

class SendPlayer{
public:
    int unum;
    Vector2D pos;
    int pc;
    const AbstractPlayerObject * player;
    SendPlayer(int _unum, Vector2D _pos, const AbstractPlayerObject * _player): player(_player)
    {
        unum = _unum;
        pos = _pos;
        pc = player->posCount();
    }
};

bool
SampleCommunication::sayBallAndPlayers( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    const int current_len = agent->effector().getSayMessageLength();

    const bool should_say_ball = shouldSayBall( agent );
    const bool should_say_goalie = shouldSayOpponentGoalie( agent );
    const bool goalie_say_situation = false; //goalieSaySituation( agent );

    if ( ! should_say_ball
         && ! should_say_goalie
         && ! goalie_say_situation
         && currentSenderUnum() != wm.self().unum()
         && current_len == 0 )
    {
        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": (sayBallAndPlayers) no send situation" );
        return false;
    }

    const int available_len = ServerParam::i().playerSayMsgSize() - current_len;

    const AudioMemory & am = wm.audioMemory();

    //
    // initialize object priority score with fixed value 1000
    //

    std::vector< ObjectScore > objects( 23 ); // 0: ball, 1-11: teammate, 12-23: opponent

    for ( int i = 0; i < 23; ++i )
    {
        objects[i].number_ = i;
        objects[i].score_ = 1000.0;
    }

    //
    // as a base score, set time difference since last hear
    //
    objects[0].score_ = wm.time().cycle() - am.ballTime().cycle();

    {
        const AudioMemory::PlayerRecord::const_iterator end = am.playerRecord().end();
        for ( AudioMemory::PlayerRecord::const_iterator p = am.playerRecord().begin();
              p != end;
              ++p )
        {
            int num = p->second.unum_;
            if ( num < 1 || 22 < num )
            {
                continue;
            }

            objects[num].score_ = wm.time().cycle() -  p->first.cycle();
        }
    }

    //
    // opponent goalie
    //
    if ( 1 <= wm.theirGoalieUnum()
         && wm.theirGoalieUnum() <= 11 )
    {
        int num = wm.theirGoalieUnum() + 11;
        double diff = wm.time().cycle() - am.goalieTime().cycle();
        objects[num].score_ = std::min( objects[num].score_, diff );
    }

#ifdef DEBUG_PRINT
    for ( int i = 0; i < 23; ++i )
    {
        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": base score: %s %d = %f",
                      ( i == 0 ? "ball" : i <= 11 ? "teammate" : "opponent" ),
                      ( i <= 11 ? i : i - 11 ),
                      objects[i].score_ );
    }
#endif

    //
    // ball
    //

    if ( wm.self().isKickable() )
    {
        if ( agent->effector().queuedNextBallKickable() )
        {
            objects[0].score_ = -65535.0;
        }
        else
        {
            objects[0].score_ = 1000.0;
        }
    }
    else if ( wm.ball().seenPosCount() > 0
              || wm.ball().seenVelCount() > 1
              || wm.kickableTeammate() )
    {
        objects[0].score_ = -65535.0;
    }
    else if ( should_say_ball )
    {
        objects[0].score_ = 1000.0;
    }
    else if ( objects[0].score_ > 0.0 )
    {
        if ( wm.prevBall().velValid() )
        {
            double angle_diff = ( wm.ball().vel().th() - wm.prevBall().vel().th() ).abs();
            double prev_speed = wm.prevBall().vel().r();
            double cur_speed = wm.ball().vel().r();

            if ( cur_speed > prev_speed + 0.1
                 || ( prev_speed > 0.5
                      && cur_speed < prev_speed * ServerParam::i().ballDecay() * 0.5 )
                 || ( prev_speed > 0.5         // Magic Number
                      && angle_diff > 20.0 ) ) // Magic Number
            {
                // ball velocity changed.
                objects[0].score_ = 1000.0;
            }
            else
            {
                objects[0].score_ *= 0.5;
            }
        }
    }

    //
    // players:
    //   1. set illegal value if player's uniform number has not been seen directry.
    //   2. reduced the priority based on the distance from ball
    //
    bool use_z = false;
    {
        const double variance = 30.0; // Magic Number
        const double x_rate = 1.0; // Magic Number
        const double y_rate = 0.5; // Magic Number

        const int min_step = std::min( std::min( wm.interceptTable()->opponentReachCycle(),
                                                 wm.interceptTable()->teammateReachCycle() ),
                                       wm.interceptTable()->selfReachCycle() );

        const Vector2D ball_pos = wm.ball().inertiaPoint( min_step );

        for ( int unum = 1; unum <= 11; ++unum )
        {
            {
                const AbstractPlayerObject * t = wm.ourPlayer( unum );
                if ( ! t
                     || t->unumCount() >= 2 || t->posCount() > 3)
                {
                    objects[unum].score_ = -65535.0;
                }
                else
                {
                    double d = distance_from_ball( t, ball_pos, x_rate, y_rate );
                    objects[unum].score_ *= distance_rate( d, variance );
                    objects[unum].score_ *= std::pow( 0.3, t->unumCount() );
//                    if(wm.interceptTable()->opponentReachCycle() == min_step){
//                        if(Strategy::i().tm_Line(unum) == Strategy::PostLine::back || Strategy::i().tm_Line(unum) == Strategy::PostLine::half)
//                            objects[unum].score_ *= 2;
//                    }
//                    if(wm.interceptTable()->teammateReachCycle() == min_step){
//                        if(ball_pos.x > 25 && ball_pos.y > 15){
//                            if(unum == 7 || unum == 11 || unum == 5){
//                                if(t->pos().dist(ball_pos) < 30){
//                                    objects[unum].score_ *= 2;
//                                }
//                            }
//                        }
//                    }
                    if(Strategy::i().isDefSit(wm, wm.self().unum())){
                        if(BhvMarkDecisionGreedy::markDecision(wm) == MarkDec::MidMark){
                                if(Strategy::i().tm_Line(unum)!=Strategy::PostLine::back){
                                    objects[unum].score_ *= 2.0;
                                    use_z = true;
                                }
                        }
                    }
                    else if(wm.ball().pos().x > 0){
                        if(Strategy::i().tm_Line(unum)!=Strategy::PostLine::forward
                            || Strategy::i().tm_Line(unum)!=Strategy::PostLine::half){
                            objects[unum].score_ *= 2.0;
                            use_z = true;
                        }
                    }
                    objects[unum].player_ = t;
                }
            }
            {
                const AbstractPlayerObject * o = wm.theirPlayer( unum );
                if ( ! o
                     || o->unumCount() >= 2 )
                {
                    objects[unum + 11].score_ = -65535.0;
                }
                else
                {
                    double d = distance_from_ball( o, ball_pos, x_rate, y_rate );
                    objects[unum + 11].score_ *= distance_rate( d, variance );
                    objects[unum + 11].score_ *= std::pow( 0.3, o->unumCount() );
                    if(Strategy::i().isDefSit(wm, wm.self().unum())){
                        if(BhvMarkDecisionGreedy::markDecision(wm) == MarkDec::MidMark){
                            auto OffStaticOpp = BhvMarkDecisionGreedy::getOppOffensiveStatic(wm);
                                if(std::find(OffStaticOpp.begin(), OffStaticOpp.end(), unum) == OffStaticOpp.end()){
                                    objects[unum + 11].score_ *= 2.0;
                                    use_z = true;
                                }
                        }
                    }
                    else if(wm.ball().pos().x > 0){
                        if(o->pos().x > wm.ball().pos().x - 20){
                            objects[unum].score_ *= 2.0;
                            use_z = true;
                        }
                    }
//                    if(wm.interceptTable()->opponentReachCycle() == min_step){
//                        if( ball_pos.x > -20){
//                            if(o->pos().x < wm.ourDefenseLineX() + 20)
//                                objects[unum].score_ *= 2;
//                        }else{
//                            if(o->pos().dist(Vector2D(-52,0)) < 25 || o->pos().x < -20)
//                                objects[unum].score_ *= 2;
//                        }
//                    }
//                    if(wm.interceptTable()->teammateReachCycle() == min_step){
//                        if(ball_pos.x > 25 && ball_pos.y > 15){
//                                if(o->pos().dist(ball_pos) < 15){
//                                    objects[unum].score_ *= 2;
//                                }
//                        }
//                    }
//                    if(wm.interceptTable()->fastestOpponent()!=nullptr)
//                        if(wm.interceptTable()->fastestOpponent()->unum() == unum)
//                            if(wm.interceptTable()->teammateReachCycle() == min_step){
//                                objects[unum].score_ *= 2;
//                            }
                    objects[unum + 11].player_ = o;
                }
            }
        }
    }
    if(use_z){
        objects[0].score_ *= 2;
    }
    //
    // erase illegal(unseen) objects
    //

    objects.erase( std::remove_if( objects.begin(),
                                   objects.end(),
                                   ObjectScore::IllegalChecker() ),
                   objects.end() );


    //
    // sorted by priority score
    //
    std::sort( objects.begin(), objects.end(),
               ObjectScore::Compare() );


#ifdef DEBUG_PRINT
    for ( std::vector< ObjectScore >::const_iterator it = objects.begin();
          it != objects.end();
          ++it )
    {
        // if ( it->score_ < 0.0 ) break;

        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": rated score: %s %d = %f",
                      ( it->number_ == 0 ? "ball" : it->number_ <= 11 ? "teammate" : "opponent" ),
                      ( it->number_ <= 11 ? it->number_ : it->number_ - 11 ),
                      it->score_ );
    }
#endif

    bool can_send_ball = false;
    bool send_ball_and_player = false;
    std::vector< ObjectScore > send_players;

    for ( std::vector< ObjectScore >::iterator it = objects.begin();
          it != objects.end();
          ++it )
    {
        if ( it->number_ == 0 )
        {
            can_send_ball = true;
            if ( send_players.size() == 1 )
            {
                send_ball_and_player = true;
                break;
            }
        }
        else
        {
            send_players.push_back( *it );

            if ( can_send_ball )
            {
                send_ball_and_player = true;
                break;
            }

            if ( send_players.size() >= 3 )
            {
                break;
            }
        }
    }

    if ( should_say_ball )
    {
        can_send_ball = true;
    }

    Vector2D ball_vel = agent->effector().queuedNextBallVel();
    if ( wm.self().isKickable()
         && agent->effector().queuedNextBallKickable() )
    {
        // invalidate ball velocity
        ball_vel.assign( 0.0, 0.0 );
        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": (sayBallAndPlayers) next cycle kickable." );
    }

    if ( wm.kickableOpponent()
         || wm.kickableTeammate() )
    {
        ball_vel.assign( 0.0, 0.0 );
    }

    //
    // send ball only
    //
    if ( can_send_ball
         && ! send_ball_and_player
         && available_len >= BallMessage::slength() )
    {
        if ( available_len >= BallPlayerMessage::slength() )
        {
            agent->addSayMessage( new BallPlayerMessage( agent->effector().queuedNextBallPos(),
                                                         ball_vel,
                                                         wm.self().unum(),
                                                         agent->effector().queuedNextSelfPos(),
                                                         agent->effector().queuedNextSelfBody() ) );
            updatePlayerSendTime( wm, wm.ourSide(), wm.self().unum() );
        }
        else
        {
            agent->addSayMessage( new BallMessage( agent->effector().queuedNextBallPos(),
                                                   ball_vel ) );
        }
        M_ball_send_time = wm.time();
        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": (sayBallAndPlayers) only ball" );
        return true;
    }

    //
    // send ball and player
    //
    if ( send_ball_and_player )
    {
        if ( should_say_goalie
             && available_len >= BallGoalieMessage::slength() )
        {
            const AbstractPlayerObject * goalie = wm.getTheirGoalie();
            agent->addSayMessage( new BallGoalieMessage( agent->effector().queuedNextBallPos(),
                                                         ball_vel,
                                                         goalie->pos() + goalie->vel(),
                                                         goalie->body() ) );
            M_ball_send_time = wm.time();
            updatePlayerSendTime( wm, goalie->side(), goalie->unum() );

            dlog.addText( Logger::COMMUNICATION,
                          __FILE__": (sayBallAndPlayers) ball and goalie" );
            return true;
        }

        if ( available_len >= BallPlayerMessage::slength() )
        {
            const AbstractPlayerObject * p0 = send_players[0].player_;
            if ( p0->unum() == wm.self().unum() )
            {
                agent->addSayMessage( new BallPlayerMessage( agent->effector().queuedNextBallPos(),
                                                             ball_vel,
                                                             wm.self().unum(),
                                                             agent->effector().queuedNextSelfPos(),
                                                             agent->effector().queuedNextSelfBody() ) );
            }
            else
            {
                agent->addSayMessage( new BallPlayerMessage( agent->effector().queuedNextBallPos(),
                                                             ball_vel,
                                                             send_players[0].number_,
                                      p0->pos() + p0->vel(),
                                      p0->body() ) );

            }
            M_ball_send_time = wm.time();
            updatePlayerSendTime( wm, p0->side(), p0->unum() );

            dlog.addText( Logger::COMMUNICATION,
                          __FILE__": (sayBallAndPlayers) ball and player %c%d",
                          p0->side() == wm.ourSide() ? 'T' : 'O', p0->unum() );
            return true;
        }
    }

    //
    // send players
    //
    if(!Strategy::i().isDefSit(wm,wm.self().unum())){
//        sayUnmark(agent);
    }
    if ( wm.ball().pos().x > 34.0
         && wm.ball().pos().absY() < 20.0 )
    {
        const AbstractPlayerObject * goalie = wm.getTheirGoalie();
        if ( goalie
             && goalie->seenPosCount() == 0
             && goalie->bodyCount() == 0
             && goalie->pos().x > 53.0 - 16.0
             && goalie->pos().absY() < 20.0
             && goalie->unum() != Unum_Unknown
             && goalie->distFromSelf() < 25.0 )
        {
            if ( available_len >= GoalieAndPlayerMessage::slength() )
            {
                const AbstractPlayerObject * player = static_cast< AbstractPlayerObject * >( 0 );

                for ( std::vector< ObjectScore >::const_iterator it = send_players.begin();
                      it != send_players.end();
                      ++it )
                {
                    if ( it->player_->unum() != goalie->unum()
                         && it->player_->side() != goalie->side()  )
                    {
                        player = it->player_;
                        break;
                    }
                }

                if ( player )
                {
                    Vector2D goalie_pos = goalie->pos() + goalie->vel();
                    goalie_pos.x = bound( 53.0 - 16.0, goalie_pos.x, 52.9 );
                    goalie_pos.y = bound( -19.9, goalie_pos.y, +19.9 );
                    agent->addSayMessage( new GoalieAndPlayerMessage( goalie->unum(),
                                                                      goalie_pos,
                                                                      goalie->body(),
                                                                      ( player->side() == wm.ourSide()
                                                                        ? player->unum()
                                                                        : player->unum() + 11 ),
                                                                      player->pos() + player->vel() ) );

                    updatePlayerSendTime( wm, goalie->side(), goalie->unum() );
                    updatePlayerSendTime( wm, player->side(), player->unum() );

                    dlog.addText( Logger::COMMUNICATION,
                                  __FILE__": say goalie and player: goalie=%d (%.2f %.2f) body=%.1f,"
                                          " player=%s[%d] (%.2f %.2f)",
                                  goalie->unum(),
                                  goalie->pos().x,
                                  goalie->pos().y,
                                  goalie->body().degree(),
                                  ( player->side() == wm.ourSide() ? "teammate" : "opponent" ),
                                  player->unum() );
                    return true;
                }
            }

            if ( available_len >= GoalieMessage::slength() )
            {
                Vector2D goalie_pos = goalie->pos() + goalie->vel();
                goalie_pos.x = bound( 53.0 - 16.0, goalie_pos.x, 52.9 );
                goalie_pos.y = bound( -19.9, goalie_pos.y, +19.9 );
                agent->addSayMessage( new GoalieMessage( goalie->unum(),
                                                         goalie_pos,
                                                         goalie->body() ) );
                M_ball_send_time = wm.time();
                M_opponent_send_time[goalie->unum()] = wm.time();

                dlog.addText( Logger::COMMUNICATION,
                              __FILE__": say goalie info: %d pos=(%.1f %.1f) body=%.1f",
                              goalie->unum(),
                              goalie->pos().x,
                              goalie->pos().y,
                              goalie->body().degree() );
                return true;
            }
        }
    }

    if ( send_players.size() >= 3
         && available_len >= ThreePlayerMessage::slength() )
    {
        vector<SendPlayer> send_players_vec;
        const AbstractPlayerObject * p0 = send_players[0].player_;
        const AbstractPlayerObject * p1 = send_players[1].player_;
        const AbstractPlayerObject * p2 = send_players[2].player_;
        int self_unum = wm.self().unum();
        Vector2D self_pos = agent->effector().queuedNextSelfPos();
        int p0_unum = send_players[0].number_;
        int p1_unum = send_players[1].number_;
        int p2_unum = send_players[2].number_;
        Vector2D p0_pos = p0->pos() + p0->vel();
        Vector2D p1_pos = p1->pos() + p1->vel();
        Vector2D p2_pos = p2->pos() + p2->vel();
        if(p0_unum == self_unum)
            p0_pos = self_pos;
        if(p1_unum == self_unum)
            p1_pos = self_pos;
        if(p2_unum == self_unum)
            p2_pos = self_pos;
        if (p0->posCount() <= 2)
            send_players_vec.push_back(SendPlayer(p0_unum, p0_pos, p0));
        if (p1->posCount() <= 2)
            send_players_vec.push_back(SendPlayer(p1_unum, p1_pos, p1));
        if (p2->posCount() <= 2)
            send_players_vec.push_back(SendPlayer(p2_unum, p2_pos, p2));
        if (send_players_vec.size() < 3){
            bool is_self_exist = false;
            for (auto & s: send_players_vec)
                if (s.unum == self_unum){
                    is_self_exist = true;
                    break;
                }
            if (!is_self_exist){
                send_players_vec.push_back(SendPlayer(self_unum, self_pos, wm.ourPlayer(self_unum)));
            }
            vector<SendPlayer> send_players_vec_tmp;
            if (send_players_vec.size() < 3){
                for (auto & n: send_players){
                    bool exist = false;
                    for (auto & e: send_players_vec){
                        if (n.number_ == e.unum){
                            exist = true;
                            break;
                        }
                    }
                    if (!exist){
                        send_players_vec_tmp.push_back(SendPlayer(n.number_, n.player_->pos(), n.player_));
                    }
                    if (send_players_vec.size() + send_players_vec_tmp.size() >= 3)
                        break;
                }
                if (send_players_vec_tmp.size() > 0)
                    send_players_vec.insert(send_players_vec.end(), send_players_vec_tmp.begin(), send_players_vec_tmp.end());
            }
        }
        std::sort(send_players_vec.begin(), send_players_vec.end(), [](const SendPlayer & a, const SendPlayer & b){return a.player->posCount() < b.player->posCount();});
        if (send_players_vec.size() == 3){
            if (send_players_vec[0].pc == 0 && send_players_vec[1].pc == 0 && send_players_vec[2].pc == 0){
                agent->addSayMessage( new ThreePlayerMessage( send_players_vec[0].unum, send_players_vec[0].pos,  send_players_vec[1].unum, send_players_vec[1].pos,  send_players_vec[2].unum, send_players_vec[2].pos));
            }
            else if (send_players_vec[0].pc == 0 && send_players_vec[1].pc == 0 && send_players_vec[2].pc == 1){
                agent->addSayMessage( new ThreePlayerMessage001( send_players_vec[0].unum, send_players_vec[0].pos,  send_players_vec[1].unum, send_players_vec[1].pos,  send_players_vec[2].unum, send_players_vec[2].pos));
            }
            else if (send_players_vec[0].pc == 0 && send_players_vec[1].pc == 0 && send_players_vec[2].pc > 2){
                agent->addSayMessage( new ThreePlayerMessage002( send_players_vec[0].unum, send_players_vec[0].pos,  send_players_vec[1].unum, send_players_vec[1].pos,  send_players_vec[2].unum, send_players_vec[2].pos));
            }
            else if (send_players_vec[0].pc == 0 && send_players_vec[1].pc == 1 && send_players_vec[2].pc == 1){
                agent->addSayMessage( new ThreePlayerMessage011( send_players_vec[0].unum, send_players_vec[0].pos,  send_players_vec[1].unum, send_players_vec[1].pos,  send_players_vec[2].unum, send_players_vec[2].pos));
            }
            else if (send_players_vec[0].pc == 0 && send_players_vec[1].pc == 1 && send_players_vec[2].pc > 2){
                agent->addSayMessage( new ThreePlayerMessage012( send_players_vec[0].unum, send_players_vec[0].pos,  send_players_vec[1].unum, send_players_vec[1].pos,  send_players_vec[2].unum, send_players_vec[2].pos));
            }
            else if (send_players_vec[0].pc == 0 && send_players_vec[1].pc == 2 && send_players_vec[2].pc > 2){
                agent->addSayMessage( new ThreePlayerMessage022( send_players_vec[0].unum, send_players_vec[0].pos,  send_players_vec[1].unum, send_players_vec[1].pos,  send_players_vec[2].unum, send_players_vec[2].pos));
            }
            else if (send_players_vec[0].pc == 1 && send_players_vec[1].pc == 1 && send_players_vec[2].pc == 1){
                agent->addSayMessage( new ThreePlayerMessage111( send_players_vec[0].unum, send_players_vec[0].pos,  send_players_vec[1].unum, send_players_vec[1].pos,  send_players_vec[2].unum, send_players_vec[2].pos));
            }
            else if (send_players_vec[0].pc == 1 && send_players_vec[1].pc == 1 && send_players_vec[2].pc > 2){
                agent->addSayMessage( new ThreePlayerMessage112( send_players_vec[0].unum, send_players_vec[0].pos,  send_players_vec[1].unum, send_players_vec[1].pos,  send_players_vec[2].unum, send_players_vec[2].pos));
            }
            else if (send_players_vec[0].pc == 1 && send_players_vec[1].pc > 2 && send_players_vec[2].pc > 2){
                agent->addSayMessage( new ThreePlayerMessage122( send_players_vec[0].unum, send_players_vec[0].pos,  send_players_vec[1].unum, send_players_vec[1].pos,  send_players_vec[2].unum, send_players_vec[2].pos));
            }
            else if (send_players_vec[0].pc > 2 && send_players_vec[1].pc > 2 && send_players_vec[2].pc > 2){
                agent->addSayMessage( new ThreePlayerMessage222( send_players_vec[0].unum, send_players_vec[0].pos,  send_players_vec[1].unum, send_players_vec[1].pos,  send_players_vec[2].unum, send_players_vec[2].pos));
            }
        }

//        if (p0->posCount() == 0){
//            if(p1->posCount() == 0){
//                if(p2->posCount() == 0){
//                    agent->addSayMessage( new ThreePlayerMessage( p0_unum,
//                                                                  p0_pos,
//                                                                  p1_unum,
//                                                                  p1_pos,
//                                                                  p2_unum,
//                                                                  p2_pos ) );
//
//                }
//                else if(p2->posCount() == 1){
//                    agent->addSayMessage( new ThreePlayerMessage001( p0_unum,
//                                                                     p0_pos,
//                                                                     p1_unum,
//                                                                     p1_pos,
//                                                                     p2_unum,
//                                                                     p2_pos ) );
//
//                }
//                else if(p2->posCount() == 2){
//                    agent->addSayMessage( new ThreePlayerMessage002( p0_unum,
//                                                                     p0_pos,
//                                                                     p1_unum,
//                                                                     p1_pos,
//                                                                     p2_unum,
//                                                                     p2_pos ) );
//
//                }
//            }
//            else if(p1->posCount() == 1){
//                if(p2->posCount() == 0){
//                    agent->addSayMessage( new ThreePlayerMessage001( p0_unum,
//                                                                     p0_pos,
//                                                                     p2_unum,
//                                                                     p2_pos,
//                                                                     p1_unum,
//                                                                     p1_pos ) );
//
//                }else if(p2->posCount() == 1){
//                    agent->addSayMessage( new ThreePlayerMessage011( p0_unum,
//                                                                     p0_pos,
//                                                                     p1_unum,
//                                                                     p1_pos,
//                                                                     p2_unum,
//                                                                     p2_pos ) );
//
//                }else if(p2->posCount() == 2){
//                    agent->addSayMessage( new ThreePlayerMessage012( p0_unum,
//                                                                     p0_pos,
//                                                                     p1_unum,
//                                                                     p1_pos,
//                                                                     p2_unum,
//                                                                     p2_pos ) );
//
//                }
//            }
//            else if(p1->posCount() == 2){
//                if(p2->posCount() == 0){
//                    agent->addSayMessage( new ThreePlayerMessage002( p0_unum,
//                                                                     p0_pos,
//                                                                     p2_unum,
//                                                                     p2_pos,
//                                                                     p1_unum,
//                                                                     p1_pos ) );
//
//                }else if(p2->posCount() == 1){
//                    agent->addSayMessage( new ThreePlayerMessage012( p0_unum,
//                                                                     p0_pos,
//                                                                     p2_unum,
//                                                                     p2_pos,
//                                                                     p1_unum,
//                                                                     p1_pos ) );
//
//                }else if(p2->posCount() == 2){
//                    agent->addSayMessage( new ThreePlayerMessage022( p0_unum,
//                                                                     p0_pos,
//                                                                     p1_unum,
//                                                                     p1_pos,
//                                                                     p2_unum,
//                                                                     p2_pos ) );
//
//                }
//            }
//        }
//        else if (p0->posCount() == 1){
//            if(p1->posCount() == 0){
//                if(p2->posCount() == 0){
//                    agent->addSayMessage( new ThreePlayerMessage001( p2_unum,
//                                                                     p2_pos,
//                                                                     p1_unum,
//                                                                     p1_pos,
//                                                                     p0_unum,
//                                                                     p0_pos ) );
//
//                }else if(p2->posCount() == 1){
//                    agent->addSayMessage( new ThreePlayerMessage011( p1_unum,
//                                                                     p1_pos,
//                                                                     p0_unum,
//                                                                     p0_pos,
//                                                                     p2_unum,
//                                                                     p2_pos ) );
//
//                }else if(p2->posCount() == 2){
//                    agent->addSayMessage( new ThreePlayerMessage012( p1_unum,
//                                                                     p1_pos,
//                                                                     p0_unum,
//                                                                     p0_pos,
//                                                                     p2_unum,
//                                                                     p2_pos ) );
//
//                }
//            }else if(p1->posCount() == 1){
//                if(p2->posCount() == 0){
//                    agent->addSayMessage( new ThreePlayerMessage011( p2_unum,
//                                                                     p2_pos,
//                                                                     p1_unum,
//                                                                     p1_pos,
//                                                                     p0_unum,
//                                                                     p0_pos ) );
//
//                }else if(p2->posCount() == 1){
//                    agent->addSayMessage( new ThreePlayerMessage011( self_unum,
//                                                                     self_pos,
//                                                                     p1_unum,
//                                                                     p1_pos,
//                                                                     p0_unum,
//                                                                     p0_pos ) );
//
//                }else if(p2->posCount() == 2){
//                    agent->addSayMessage( new ThreePlayerMessage011( self_unum,
//                                                                     self_pos,
//                                                                     p1_unum,
//                                                                     p1_pos,
//                                                                     p0_unum,
//                                                                     p0_pos ) );
//
//                }
//            }else if(p1->posCount() == 2){
//                if(p2->posCount() == 0){
//                    agent->addSayMessage( new ThreePlayerMessage012( p2_unum,
//                                                                     p2_pos,
//                                                                     p0_unum,
//                                                                     p0_pos,
//                                                                     p1_unum,
//                                                                     p1_pos ) );
//
//                }else if(p2->posCount() == 1){
//                    agent->addSayMessage( new ThreePlayerMessage012( self_unum,
//                                                                     self_pos,
//                                                                     p0_unum,
//                                                                     p0_pos,
//                                                                     p1_unum,
//                                                                     p1_pos ) );
//
//                }else if(p2->posCount() == 2){
//                    agent->addSayMessage( new ThreePlayerMessage012( self_unum,
//                                                                     self_pos,
//                                                                     p0_unum,
//                                                                     p0_pos,
//                                                                     p1_unum,
//                                                                     p1_pos ) );
//
//                }
//            }
//
//        }
//        else if(p0->posCount() == 2){
//            if(p1->posCount() == 0){
//                if(p2->posCount() == 0){
//                    agent->addSayMessage( new ThreePlayerMessage002( p2_unum,
//                                                                     p2_pos,
//                                                                     p1_unum,
//                                                                     p1_pos,
//                                                                     p0_unum,
//                                                                     p0_pos ) );
//
//                }else if(p2->posCount() == 1){
//                    agent->addSayMessage( new ThreePlayerMessage012( p1_unum,
//                                                                     p1_pos,
//                                                                     p2_unum,
//                                                                     p2_pos,
//                                                                     p0_unum,
//                                                                     p0_pos ) );
//
//                }else if(p2->posCount() == 2){
//                    agent->addSayMessage( new ThreePlayerMessage022( p1_unum,
//                                                                     p1_pos,
//                                                                     p0_unum,
//                                                                     p0_pos,
//                                                                     p2_unum,
//                                                                     p2_pos ) );
//
//                }
//            }else if(p1->posCount() == 1){
//                if(p2->posCount() == 0){
//                    agent->addSayMessage( new ThreePlayerMessage012( p2_unum,
//                                                                     p2_pos,
//                                                                     p1_unum,
//                                                                     p1_pos,
//                                                                     p0_unum,
//                                                                     p0_pos ) );
//
//                }else if(p2->posCount() == 1){
//                    agent->addSayMessage( new ThreePlayerMessage012( self_unum,
//                                                                     self_pos,
//                                                                     p1_unum,
//                                                                     p1_pos,
//                                                                     p0_unum,
//                                                                     p0_pos ) );
//
//                }else if(p2->posCount() == 2){
//                    agent->addSayMessage( new ThreePlayerMessage012( self_unum,
//                                                                     self_pos,
//                                                                     p1_unum,
//                                                                     p1_pos,
//                                                                     p0_unum,
//                                                                     p0_pos ) );
//
//                }
//            }else if(p1->posCount() == 2){
//                if(p2->posCount() == 0){
//                    agent->addSayMessage( new ThreePlayerMessage022( p2_unum,
//                                                                     p2_pos,
//                                                                     p0_unum,
//                                                                     p0_pos,
//                                                                     p1_unum,
//                                                                     p1_pos ) );
//
//                }else if(p2->posCount() == 1){
//                    agent->addSayMessage( new ThreePlayerMessage022( self_unum,
//                                                                     self_pos,
//                                                                     p0_unum,
//                                                                     p0_pos,
//                                                                     p1_unum,
//                                                                     p1_pos ) );
//
//                }else if(p2->posCount() == 2){
//                    agent->addSayMessage( new ThreePlayerMessage022( self_unum,
//                                                                     self_pos,
//                                                                     p0_unum,
//                                                                     p0_pos,
//                                                                     p1_unum,
//                                                                     p1_pos ) );
//
//                }
//            }
//
//
//        }
//        else{
//            if(p1->posCount() == 0){
//                if(p2->posCount() == 0){
//                    agent->addSayMessage( new ThreePlayerMessage( self_unum,
//                                                                  self_pos,
//                                                                  p1_unum,
//                                                                  p1_pos,
//                                                                  p2_unum,
//                                                                  p2_pos ) );
//
//                }
//                else if(p2->posCount() == 1){
//                    agent->addSayMessage( new ThreePlayerMessage001( self_unum,
//                                                                     self_pos,
//                                                                     p1_unum,
//                                                                     p1_pos,
//                                                                     p2_unum,
//                                                                     p2_pos ) );
//
//                }
//                else if(p2->posCount() == 2){
//                    agent->addSayMessage( new ThreePlayerMessage002( self_unum,
//                                                                     self_pos,
//                                                                     p0_unum,
//                                                                     p0_pos,
//                                                                     p2_unum,
//                                                                     p2_pos ) );
//
//                }
//            }
//            else if(p1->posCount() == 1){
//                if(p2->posCount() == 0){
//                    agent->addSayMessage( new ThreePlayerMessage001( self_unum,
//                                                                     self_pos,
//                                                                     p2_unum,
//                                                                     p2_pos,
//                                                                     p1_unum,
//                                                                     p1_pos ) );
//
//                }else if(p2->posCount() == 1){
//                    agent->addSayMessage( new ThreePlayerMessage011( self_unum,
//                                                                     self_pos,
//                                                                     p1_unum,
//                                                                     p1_pos,
//                                                                     p2_unum,
//                                                                     p2_pos ) );
//
//                }else if(p2->posCount() == 2){
//                    agent->addSayMessage( new ThreePlayerMessage012( self_unum,
//                                                                     self_pos,
//                                                                     p1_unum,
//                                                                     p1_pos,
//                                                                     p2_unum,
//                                                                     p2_pos ) );
//
//                }
//            }
//            else if(p1->posCount() == 2){
//                if(p2->posCount() == 0){
//                    agent->addSayMessage( new ThreePlayerMessage002( self_unum,
//                                                                     self_pos,
//                                                                     p2_unum,
//                                                                     p2_pos,
//                                                                     p1_unum,
//                                                                     p1_pos ) );
//
//                }else if(p2->posCount() == 1){
//                    agent->addSayMessage( new ThreePlayerMessage012( self_unum,
//                                                                     self_pos,
//                                                                     p2_unum,
//                                                                     p2_pos,
//                                                                     p1_unum,
//                                                                     p1_pos ) );
//
//                }else if(p2->posCount() == 2){
//                    agent->addSayMessage( new ThreePlayerMessage022( self_unum,
//                                                                     self_pos,
//                                                                     p2_unum,
//                                                                     p2_pos,
//                                                                     p1_unum,
//                                                                     p1_pos ) );
//
//                }
//            }
//        }

        updatePlayerSendTime( wm, p0->side(), p0->unum() );
        updatePlayerSendTime( wm, p1->side(), p1->unum() );
        updatePlayerSendTime( wm, p2->side(), p2->unum() );

        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": say three players %c%d %c%d %c%d",
                      p0->side() == wm.ourSide() ? 'T' : 'O', p0->unum(),
                      p1->side() == wm.ourSide() ? 'T' : 'O', p1->unum(),
                      p2->side() == wm.ourSide() ? 'T' : 'O', p2->unum() );
        return true;
    }

    if ( send_players.size() >= 2
         && available_len >= TwoPlayerMessage::slength() )
    {
        const AbstractPlayerObject * p0 = send_players[0].player_;
        const AbstractPlayerObject * p1 = send_players[1].player_;

        Vector2D p0_pos = p0->pos() + p0->vel();
        Vector2D p1_pos = p1->pos() + p1->vel();
        if(send_players[0].number_ == wm.self().unum())
            p0_pos = agent->effector().queuedNextMyPos();
        if(send_players[1].number_ == wm.self().unum())
            p1_pos = agent->effector().queuedNextMyPos();
        if (p0->posCount() == 0 && p1->posCount() == 0){
            agent->addSayMessage( new TwoPlayerMessage( send_players[0].number_,
                                                        p0_pos,
                                                        send_players[1].number_,
                                                        p1_pos ) );
        }
        else if (p0->posCount() == 0 && p1->posCount() == 1){
            agent->addSayMessage( new TwoPlayerMessage01( send_players[0].number_,
                                                        p0_pos,
                                                        send_players[1].number_,
                                                        p1_pos ) );
        }
        else if (p0->posCount() == 1 && p1->posCount() == 0){
            agent->addSayMessage( new TwoPlayerMessage01( send_players[1].number_,
                                                          p1_pos,
                                                          send_players[0].number_,
                                                        p0_pos) );
        }
        else if (p0->posCount() == 1 && p1->posCount() == 1){
            agent->addSayMessage( new TwoPlayerMessage11( send_players[0].number_,
                                                        p0_pos,
                                                        send_players[1].number_,
                                                        p1_pos ) );
        }
        else if (p0->posCount() == 0 && p1->posCount() == 2){
            agent->addSayMessage( new TwoPlayerMessage02( send_players[0].number_,
                                                        p0_pos,
                                                        send_players[1].number_,
                                                        p1_pos ) );
        }
        else if (p0->posCount() == 2 && p1->posCount() == 0){
            agent->addSayMessage( new TwoPlayerMessage02( send_players[1].number_,
                                                        p1_pos,
                                                        send_players[0].number_,
                                                        p0_pos ) );
        }
        else if (p0->posCount() == 1 && p1->posCount() == 2){
            agent->addSayMessage( new TwoPlayerMessage12( send_players[0].number_,
                                                        p0_pos,
                                                        send_players[1].number_,
                                                        p1_pos ) );
        }
        else if (p0->posCount() == 2 && p1->posCount() == 1){
            agent->addSayMessage( new TwoPlayerMessage12( send_players[1].number_,
                                                        p1_pos,
                                                        send_players[0].number_,
                                                        p0_pos ) );
        }
        else if (p0->posCount() == 2 && p1->posCount() == 2){
            agent->addSayMessage( new TwoPlayerMessage22( send_players[0].number_,
                                                        p0_pos,
                                                        send_players[1].number_,
                                                        p1_pos ) );
        }
        else{
            agent->addSayMessage( new TwoPlayerMessage22( send_players[0].number_,
                                                        p0_pos,
                                                        send_players[1].number_,
                                                        p1_pos ) );
        }


        updatePlayerSendTime( wm, p0->side(), p0->unum() );
        updatePlayerSendTime( wm, p1->side(), p1->unum() );

        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": say two players %c%d %c%d",
                      p0->side() == wm.ourSide() ? 'T' : 'O', p0->unum(),
                      p1->side() == wm.ourSide() ? 'T' : 'O', p1->unum() );
        return true;
    }

    if ( send_players.size() >= 1
         && available_len >= GoalieMessage::slength() )
    {
        const AbstractPlayerObject * p0 = send_players[0].player_;

        if ( p0->side() == wm.theirSide()
             && p0->goalie()
             && p0->pos().x > 53.0 - 16.0
             && p0->pos().absY() < 20.0
             && p0->distFromSelf() < 25.0 )
        {
            Vector2D goalie_pos = p0->pos() + p0->vel();
            goalie_pos.x = bound( 53.0 - 16.0, goalie_pos.x, 52.9 );
            goalie_pos.y = bound( -19.9, goalie_pos.y, +19.9 );
            agent->addSayMessage( new GoalieMessage( p0->unum(),
                                                     goalie_pos,
                                                     p0->body() ) );
            updatePlayerSendTime( wm, p0->side(), p0->unum() );

            dlog.addText( Logger::COMMUNICATION,
                          __FILE__": (sayBallAndPlayers) ball and goalie %c%d",
                          p0->side() == wm.ourSide() ? 'T' : 'O', p0->unum() );
            return true;
        }
    }

    if ( send_players.size() >= 1
         && available_len >= OnePlayerMessage::slength() )
    {
        const AbstractPlayerObject * p0 = send_players[0].player_;
        Vector2D p0_pos = p0->pos() + p0->vel();
        if(send_players[0].number_ == wm.self().unum()){
            p0_pos = agent->effector().queuedNextSelfPos();
        }
        if(p0->posCount() == 0){
            agent->addSayMessage( new OnePlayerMessage( send_players[0].number_,
                                  p0_pos ) );
        }else if(p0->posCount() == 1){
            agent->addSayMessage( new OnePlayerMessage1( send_players[0].number_,
                                  p0_pos ) );
        }else if(p0->posCount() == 2){
            agent->addSayMessage( new OnePlayerMessage2( send_players[0].number_,
                                  p0_pos ) );
        }


        updatePlayerSendTime( wm, p0->side(), p0->unum() );

        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": say one players %c%d",
                      p0->side() == wm.ourSide() ? 'T' : 'O', p0->unum() );
        return true;
    }


    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
SampleCommunication::sayBall( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    const int current_len = agent->effector().getSayMessageLength();
    const int available_len = ServerParam::i().playerSayMsgSize() - current_len;

    if ( available_len < BallMessage::slength() )
    {
        return false;
    }

    if ( wm.ball().seenPosCount() > 0
         || wm.ball().seenVelCount() > 1 )
    {
        // not seen at current cycle
        return false;
    }

    if ( wm.gameMode().type() != GameMode::PlayOn )
    {
        if ( agent->effector().queuedNextBallVel().r2() < 0.5*0.5 )
        {
            return false;
        }
    }

#if 1
    if ( wm.kickableTeammate()
         //|| wm.kickableOpponent()
         )
    {
        return false;
    }
#endif

    const PlayerObject * ball_nearest_teammate = NULL;
    const PlayerObject * second_ball_nearest_teammate = NULL;

    for ( PlayerObject::Cont::const_iterator t = wm.teammatesFromBall().begin(),
              end = wm.teammatesFromBall().end();
          t != end;
          ++t )
    {
        if ( (*t)->isGhost() || (*t)->posCount() >= 10 ) continue;

        if ( ! ball_nearest_teammate )
        {
            ball_nearest_teammate = *t;
        }
        else if ( ! second_ball_nearest_teammate )
        {
            second_ball_nearest_teammate = *t;
            break;
        }
    }

    int self_min = wm.interceptTable()->selfReachCycle();
    int mate_min = wm.interceptTable()->teammateReachCycle();
    int our_min = std::min( self_min, mate_min );
    int opp_min = wm.interceptTable()->opponentReachCycle();


    bool send_ball = false;
    if ( wm.self().isKickable()  // I am kickable
         || ( ! ball_nearest_teammate
              || ( ball_nearest_teammate->distFromBall()
                   > wm.ball().distFromSelf() - 3.0 ) // I am nearest to ball
              || ( wm.ball().distFromSelf() < 20.0
                   && ball_nearest_teammate->distFromBall() < 6.0
                   && ball_nearest_teammate->distFromBall() > 1.0
                   && opp_min <= mate_min + 1 ) )
         || ( second_ball_nearest_teammate
              && ( ball_nearest_teammate->distFromBall()  // nearest to ball teammate is
                   > ServerParam::i().visibleDistance() - 0.5 ) // over vis dist
              && ( second_ball_nearest_teammate->distFromBall()
                   > wm.ball().distFromSelf() - 5.0 ) ) ) // I am second
    {
        send_ball = true;
    }

    //     const PlayerObject * nearest_opponent = wm.getOpponentNearestToBall( 10 );
    //     if ( nearest_opponent
    //          && ( nearest_opponent->distFromBall() >= 10.0
    //               || nearest_opponent->distFromBall() < nearest_opponent->playerTypePtr()->kickableArea() + 0.1 ) )
    //     {
    //         send_ball = false;
    //         return false;
    //     }

    Vector2D ball_vel = agent->effector().queuedNextBallVel();
    if ( wm.self().isKickable()
         && agent->effector().queuedNextBallKickable() )
    {
        // set ball velocity to 0.
        ball_vel.assign( 0.0, 0.0 );
        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": (sayBall) say ball status. next cycle kickable." );
    }

    Vector2D ball_trap_pos = wm.ball().inertiaPoint( our_min );

    //
    // ball & opponent goalie
    //
    if ( send_ball
         && ( wm.kickableTeammate()
              || our_min <= opp_min + 1 )
         && ball_trap_pos.x > 34.0
         && ball_trap_pos.absY() < 20.0
         && available_len >= BallGoalieMessage::slength() )
    {
        if ( shouldSayOpponentGoalie( agent ) )
        {
            const AbstractPlayerObject * goalie = wm.getTheirGoalie();
            agent->addSayMessage( new BallGoalieMessage( agent->effector().queuedNextBallPos(),
                                                         ball_vel,
                                                         goalie->pos() + goalie->vel(),
                                                         goalie->body() ) );
            M_ball_send_time = wm.time();
            updatePlayerSendTime( wm, goalie->side(), goalie->unum() );

            dlog.addText( Logger::COMMUNICATION,
                          __FILE__": (sayBall) say ball/goalie status" );
            return true;
        }
    }

    //
    // ball & opponent
    //
    if ( send_ball
         // && ball_nearest_teammate
         // && mate_min <= 3
         && available_len >= BallPlayerMessage::slength() )
    {
        // static GameTime s_last_opponent_send_time( 0, 0 );
        // static int s_last_sent_opponent_unum = Unum_Unknown;

        const PlayerObject * opponent = static_cast< const PlayerObject * >( 0 );

        Vector2D opp_trap_pos = wm.ball().inertiaPoint( opp_min );


        for ( PlayerObject::Cont::const_iterator o = wm.opponentsFromBall().begin(),
            end = wm.opponentsFromBall().end();
        o != end;
        ++o )
        {
            if ( (*o)->seenPosCount() > 0 ) continue;
            if ( (*o)->unum() == Unum_Unknown ) continue;
            if ( (*o)->unumCount() > 0 ) continue;
            if ( (*o)->bodyCount() > 0 ) continue;
            if ( (*o)->pos().dist2( opp_trap_pos ) > 15.0*15.0 ) continue;
            // if ( (*o)->distFromBall() > 20.0 ) continue;
            // if ( (*o)->distFromSelf() > 20.0 ) continue;

            // if ( (*o)->unum() == s_last_sent_opponent_unum
            //      && s_last_opponent_send_time.cycle() >= wm.time().cycle() - 1 )
            // {
            //     continue;
            // }

            opponent = *o;
            break;
        }

        if ( opponent )
        {
            // s_last_opponent_send_time = wm.time();
            // s_last_sent_opponent_unum = opponent->unum();

            agent->addSayMessage( new BallPlayerMessage( agent->effector().queuedNextBallPos(),
                                                         ball_vel,
                                                         opponent->unum() + 11,
                                                         opponent->pos(),
                                                         opponent->body() ) );
            M_opponent_send_time[opponent->unum()] = wm.time();

            dlog.addText( Logger::COMMUNICATION,
                          __FILE__": (sayBall) say ball/opponent status. opponent=%d",
                          opponent->unum() );
            return true;
        }
    }

    if ( send_ball )
    {
        agent->addSayMessage( new BallMessage( agent->effector().queuedNextBallPos(),
                                               ball_vel ) );
        M_ball_send_time = wm.time();
        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": (sayBall) say ball only" );
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
SampleCommunication::sayGoalie( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    // goalie info: ball is in chance area
    const int current_len = agent->effector().getSayMessageLength();
    const int available_len = ServerParam::i().playerSayMsgSize() - current_len;

    if ( available_len < GoalieMessage::slength() )
    {
        return false;
    }

    if ( shouldSayOpponentGoalie( agent ) )
    {
        const AbstractPlayerObject * goalie = wm.getTheirGoalie();
        Vector2D goalie_pos = goalie->pos() + goalie->vel();
        goalie_pos.x = bound( 53.0 - 16.0, goalie_pos.x, 52.9 );
        goalie_pos.y = bound( -19.9, goalie_pos.y, +19.9 );
        agent->addSayMessage( new GoalieMessage( goalie->unum(),
                                                 goalie_pos,
                                                 goalie->body() ) );
        M_ball_send_time = wm.time();
        updatePlayerSendTime( wm, goalie->side(), goalie->unum() );

        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": say goalie info: %d pos=(%.1f %.1f) body=%.1f",
                      goalie->unum(),
                      goalie->pos().x,
                      goalie->pos().y,
                      goalie->body().degree() );
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
SampleCommunication::sayIntercept( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    const int current_len = agent->effector().getSayMessageLength();
    const int available_len = ServerParam::i().playerSayMsgSize() - current_len;

    if ( available_len < InterceptMessage::slength() )
    {
        return false;
    }

    if ( wm.gameMode().type() != GameMode::PlayOn )
    {
        return false;
    }

    if ( wm.ball().posCount() > 0
         || wm.ball().velCount() > 0 )
    {
        return false;
    }

    int self_min = wm.interceptTable()->selfReachCycle();
    int mate_min = wm.interceptTable()->teammateReachCycle();
    int opp_min = wm.interceptTable()->opponentReachCycle();

    if ( wm.self().isKickable() )
    {
        double next_dist =  agent->effector().queuedNextMyPos().dist( agent->effector().queuedNextBallPos() );
        if ( next_dist > wm.self().playerType().kickableArea() )
        {
            self_min = 10000;
        }
        else
        {
            self_min = 0;
        }
    }

    if ( self_min <= mate_min
         && self_min <= opp_min
         && self_min <= 10 )
    {
        agent->addSayMessage( new InterceptMessage( true,
                                                    wm.self().unum(),
                                                    self_min ) );
        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": say self intercept info %d",
                      self_min );
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
SampleCommunication::sayOffsideLine( PlayerAgent * agent )
{
    const int current_len = agent->effector().getSayMessageLength();

    if ( current_len == 0 )
    {
        return false;
    }

    const int available_len = ServerParam::i().playerSayMsgSize() - current_len;

    if ( available_len < OffsideLineMessage::slength() )
    {
        return false;
    }

    const WorldModel & wm = agent->world();

    if ( wm.offsideLineX() < 10.0 )
    {
        return false;
    }

    if ( wm.seeTime() != wm.time()
         || wm.offsideLineCount() > 1
         || wm.opponentsFromSelf().size() < 10 )
    {
        return false;
    }

    int s_min = wm.interceptTable()->selfReachCycle();
    int t_min = wm.interceptTable()->teammateReachCycle();
    int o_min = wm.interceptTable()->opponentReachCycle();

    if ( o_min < t_min
         && o_min < s_min )
    {
        return false;
    }

    int min_step = std::min( s_min, t_min );

    Vector2D ball_pos = wm.ball().inertiaPoint( min_step );

    if ( 0.0 < ball_pos.x
         && ball_pos.x < 37.0
         && ball_pos.x > wm.offsideLineX() - 20.0
         && std::fabs( wm.self().pos().x - wm.offsideLineX() ) < 20.0
         )
    {
        agent->addSayMessage( new OffsideLineMessage( wm.offsideLineX() ) );
        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": say offside line %.1f",
                      wm.offsideLineX() );
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
SampleCommunication::sayDefenseLine( PlayerAgent * agent )
{
    const int current_len = agent->effector().getSayMessageLength();
    const int available_len = ServerParam::i().playerSayMsgSize() - current_len;

    if ( available_len < DefenseLineMessage::slength() )
    {
        return false;
    }

    const WorldModel & wm = agent->world();

    if ( std::fabs( wm.self().pos().x - wm.ourDefenseLineX() ) > 40.0 )
    {
        return false;
    }

    if ( wm.seeTime() != wm.time() )
    {
        return false;
    }

    int opp_min = wm.interceptTable()->opponentReachCycle();

    Vector2D opp_trap_pos = wm.ball().inertiaPoint( opp_min );

    if ( wm.self().goalie()
         && wm.gameMode().type() == GameMode::PlayOn
         && -40.0 < opp_trap_pos.x
         && opp_trap_pos.x < -20.0
         && wm.time().cycle() % 3 == 0 )
    {
        agent->addSayMessage( new DefenseLineMessage( wm.ourDefenseLineX() ) );
        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": say defense line %.1f",
                      wm.ourDefenseLineX() );
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
SampleCommunication::sayPlayers( PlayerAgent * agent )
{
    static GameTime s_last_time( -1, 0 );

    const int len = agent->effector().getSayMessageLength();
    if ( len + OnePlayerMessage::slength() > ServerParam::i().playerSayMsgSize() )
    {
        return false;
    }

    const WorldModel & wm = agent->world();

    //     if ( s_last_time.cycle() >= wm.time().cycle() - 3 )
    //     {
    //         return false;
    //     }

    if ( len == 0
         && wm.time().cycle() % 11 != wm.self().unum() - 1 )
    {
        return false;
    }

    bool opponent_attack = false;

    int opp_min = wm.interceptTable()->opponentReachCycle();
    int mate_min = wm.interceptTable()->opponentReachCycle();
    int self_min = wm.interceptTable()->opponentReachCycle();

    Vector2D opp_trap_pos = wm.ball().inertiaPoint( opp_min );

    if ( opp_min <= mate_min
         && opp_min <= self_min )
    {
        if ( opp_trap_pos.x < 10.0 )
        {
            opponent_attack = true;
        }
    }

    AbstractPlayerObject::Cont candidates;
    bool include_self = false;

    // set self as candidate
    if ( M_teammate_send_time[wm.self().unum()] != wm.time()
         && ( ! opponent_attack
              || wm.self().pos().x < opp_trap_pos.x + 10.0 )
         && wm.time().cycle() % 11 == ( wm.self().unum() - 1 ) )
    {
#ifdef DEBUG_PRINT
        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": (sayPlayers) candidate self(1)" );
#endif
        candidates.push_back( &(wm.self()) );
        include_self = true;
    }

    if ( ! opponent_attack )
    {
        // set teammate candidates
        for ( PlayerObject::Cont::const_iterator t = wm.teammatesFromSelf().begin(),
                end = wm.teammatesFromSelf().end();
            t != end;
            ++t )
        {
            dlog.addText( Logger::COMMUNICATION,
                          __FILE__": (sayPlayers) __ check(1) teammate %d(%.1f %.1f)",
                          (*t)->unum(),
                          (*t)->pos().x, (*t)->pos().y );

            if ( (*t)->seenPosCount() > 0 ) continue;
            //if ( (*t)->seenVelCount() > 0 ) continue;
            if ( (*t)->unumCount() > 0 ) continue;
            if ( (*t)->unum() == Unum_Unknown ) continue;

            if ( M_teammate_send_time[(*t)->unum()].cycle() >= wm.time().cycle() - 1 )
            {
                continue;
            }
#ifdef DEBUG_PRINT
            dlog.addText( Logger::COMMUNICATION,
                          __FILE__": (sayPlayers) candidate teammate(1) %d",
                          (*t)->unum() );
#endif
            candidates.push_back( *t );
        }
    }

    // set opponent candidates
    for ( PlayerObject::Cont::const_iterator o = wm.opponentsFromSelf().begin(),
            end = wm.opponentsFromSelf().end();
        o != end;
        ++o )
    {
#ifdef DEBUG_PRINT
        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": (sayPlayers) __ check opponent %d(%.1f %.1f)",
                      (*o)->unum(),
                      (*o)->pos().x, (*o)->pos().y );
#endif
        if ( (*o)->seenPosCount() > 0 ) continue;
        //if ( (*o)->seenVelCount() > 0 ) continue;
        if ( (*o)->unumCount() > 0 ) continue;
        if ( (*o)->unum() == Unum_Unknown ) continue;

        if ( ! opponent_attack )
        {
            if ( M_opponent_send_time[(*o)->unum()].cycle() >= wm.time().cycle() - 1 )
            {
                dlog.addText( Logger::COMMUNICATION,
                              __FILE__": (sayPlayers) __ check opponent %d. recently sent",
                              (*o)->unum() );
                continue;
            }
        }
        else
        {
            if ( (*o)->pos().x > 0.0
                 && (*o)->pos().x > wm.ourDefenseLineX() + 15.0 )
            {
#ifdef DEBUG_PRINT
                dlog.addText( Logger::COMMUNICATION,
                              __FILE__": (sayPlayers) __ check opponent %d. x>0 && x>defense_line+15",
                              (*o)->unum() );
#endif
                continue;
            }
        }
#ifdef DEBUG_PRINT
        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": (sayPlayers) candidate opponent %d",
                      (*o)->unum() );
#endif
        candidates.push_back( *o );
    }

    if ( //opponent_attack &&
         ! candidates.empty()
         && candidates.size() < 3 )
    {
        // set self
        if ( ! include_self
             && M_teammate_send_time[wm.self().unum()] != wm.time()
             && wm.time().cycle() % 11 == ( wm.self().unum() - 1 ) )
        {
            dlog.addText( Logger::COMMUNICATION,
                          __FILE__": (sayPlayers) candidate self(2)" );
            candidates.push_back( &(wm.self()) );
        }

        // set teammate candidates
        for ( PlayerObject::Cont::const_iterator t = wm.teammatesFromSelf().begin(),
                end = wm.teammatesFromSelf().end();
            t != end;
            ++t )
        {
            dlog.addText( Logger::COMMUNICATION,
                          __FILE__": (sayPlayers) __ check(2) teammate %d(%.1f %.1f)",
                          (*t)->unum(),
                          (*t)->pos().x, (*t)->pos().y );

            if ( (*t)->seenPosCount() > 0 ) continue;
            //if ( (*t)->seenVelCount() > 0 ) continue;
            if ( (*t)->unumCount() > 0 ) continue;
            if ( (*t)->unum() == Unum_Unknown ) continue;

            if ( M_teammate_send_time[(*t)->unum()].cycle() >= wm.time().cycle() - 1 )
            {
                continue;
            }

            dlog.addText( Logger::COMMUNICATION,
                          __FILE__": (sayPlayers) candidate teammate(2) %d",
                          (*t)->unum() );
            candidates.push_back( *t );
        }
    }


    if ( candidates.empty() )
    {
        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": (sayPlayers) no candidate" );
        return false;
    }


    if ( opponent_attack )
    {
        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": (sayPlayers) opponent attack. sorted by X." );
        std::sort( candidates.begin(), candidates.end(), AbstractPlayerXCmp() );
    }
    else
    {
        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": (sayPlayers) NO opponent attack. sorted by distance from self." );
        std::sort( candidates.begin(), candidates.end(), AbstractPlayerSelfDistCmp() );
    }

    AbstractPlayerObject::Cont::const_iterator first = candidates.begin();
    int first_unum = ( (*first)->side() == wm.ourSide()
                       ? (*first)->unum()
                       : (*first)->unum() + 11 );

    if ( candidates.size() >= 3
         && len + ThreePlayerMessage::slength() <= ServerParam::i().playerSayMsgSize() )
    {
        AbstractPlayerObject::Cont::const_iterator second = first; ++second;
        int second_unum = ( (*second)->side() == wm.ourSide()
                            ? (*second)->unum()
                            : (*second)->unum() + 11 );
        AbstractPlayerObject::Cont::const_iterator third = second; ++third;
        int third_unum = ( (*third)->side() == wm.ourSide()
                           ? (*third)->unum()
                           : (*third)->unum() + 11 );

        agent->addSayMessage( new ThreePlayerMessage( first_unum,
                                                      (*first)->pos() + (*first)->vel(),
                                                      second_unum,
                                                      (*second)->pos() + (*second)->vel(),
                                                      third_unum,
                                                      (*third)->pos() + (*third)->vel() ) );
        updatePlayerSendTime( wm, (*first)->side(), (*first)->unum() );
        updatePlayerSendTime( wm, (*second)->side(), (*second)->unum() );
        updatePlayerSendTime( wm, (*third)->side(), (*third)->unum() );

        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": say three players %c%d %c%d %c%d",
                      (*first)->side() == wm.ourSide() ? 'T' : 'O', (*first)->unum(),
                      (*second)->side() == wm.ourSide() ? 'T' : 'O', (*second)->unum(),
                      (*third)->side() == wm.ourSide() ? 'T' : 'O', (*third)->unum() );
    }
    else if ( candidates.size() >= 2
              && len + TwoPlayerMessage::slength() <= ServerParam::i().playerSayMsgSize() )
    {
        AbstractPlayerObject::Cont::const_iterator second = first; ++second;
        int second_unum = ( (*second)->side() == wm.ourSide()
                            ? (*second)->unum()
                            : (*second)->unum() + 11 );
        agent->addSayMessage( new TwoPlayerMessage( first_unum,
                                                    (*first)->pos() + (*first)->vel(),
                                                    second_unum,
                                                    (*second)->pos() + (*second)->vel() ) );
        updatePlayerSendTime( wm, (*first)->side(), (*first)->unum() );
        updatePlayerSendTime( wm, (*second)->side(), (*second)->unum() );

        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": say two players %c%d %c%d",
                      (*first)->side() == wm.ourSide() ? 'T' : 'O', (*first)->unum(),
                      (*second)->side() == wm.ourSide() ? 'T' : 'O', (*second)->unum() );
    }
    else if ( len + OnePlayerMessage::slength() <= ServerParam::i().playerSayMsgSize() )
    {
        agent->addSayMessage( new OnePlayerMessage( first_unum,
                                                    (*first)->pos() + (*first)->vel() ) );
        updatePlayerSendTime( wm, (*first)->side(), (*first)->unum() );

        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": say one players %c%d",
                      (*first)->side() == wm.ourSide() ? 'T' : 'O', (*first)->unum() );
    }

    s_last_time = wm.time();
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
SampleCommunication::sayOpponents( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    if ( sayThreeOpponents( agent ) )
    {
        return true;
    }

    if ( sayTwoOpponents( agent ) )
    {
        return true;
    }

    const int current_len = agent->effector().getSayMessageLength();
    const int available_len = ServerParam::i().playerSayMsgSize() - current_len;
    if ( available_len < OpponentMessage::slength() )
    {
        return false;
    }

    int self_min = wm.interceptTable()->selfReachCycle();
    int mate_min = wm.interceptTable()->teammateReachCycle();
    int opp_min = wm.interceptTable()->opponentReachCycle();

    if ( opp_min > self_min + 10
         && opp_min > mate_min + 10 )
    {
        return false;
    }

    const PlayerObject * fastest_opponent = wm.interceptTable()->fastestOpponent();
    if ( fastest_opponent
         && fastest_opponent->unum() != Unum_Unknown
         && fastest_opponent->unumCount() == 0
         && fastest_opponent->seenPosCount() == 0
         && fastest_opponent->bodyCount() == 0
         && 10.0 < fastest_opponent->distFromSelf()
         && fastest_opponent->distFromSelf() < 30.0 )
    {
        agent->addSayMessage( new OpponentMessage( fastest_opponent->unum(),
                                                   fastest_opponent->pos(),
                                                   fastest_opponent->body() ) );
        M_opponent_send_time[fastest_opponent->unum()] = wm.time();

        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": say opponent status" );
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
SampleCommunication::sayTwoOpponents( PlayerAgent * agent )
{
    const int current_len = agent->effector().getSayMessageLength();
    const int available_len = ServerParam::i().playerSayMsgSize() - current_len;
    if ( available_len < TwoPlayerMessage::slength() )
    {
        return false;
    }

    const WorldModel & wm = agent->world();

    std::vector< const PlayerObject * > candidates;

    for ( PlayerObject::Cont::const_iterator o = wm.opponentsFromSelf().begin(),
            end = wm.opponentsFromSelf().end();
        o != end;
        ++o )
    {
        if ( (*o)->seenPosCount() > 0 ) continue;
        if ( (*o)->unumCount() > 0 ) continue;

        if ( M_opponent_send_time[(*o)->unum()].cycle() >= wm.time().cycle() - 1 )
        {
            continue;
        }

        candidates.push_back( *o );
    }

    if ( candidates.size() < 2 )
    {
        return false;
    }

    std::vector< const PlayerObject * >::const_iterator first = candidates.begin();
    std::vector< const PlayerObject * >::const_iterator second = first; ++second;

    agent->addSayMessage( new TwoPlayerMessage( (*first)->unum() + 11,
                                                (*first)->pos() + (*first)->vel(),
                                                (*second)->unum() + 11,
                                                (*second)->pos() + (*second)->vel() ) );
    M_opponent_send_time[(*first)->unum()] = wm.time();
    M_opponent_send_time[(*second)->unum()] = wm.time();


    dlog.addText( Logger::COMMUNICATION,
                  __FILE__": say two oppoinents %d %d",
                  (*first)->unum(),
                  (*second)->unum() );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
SampleCommunication::sayThreeOpponents( PlayerAgent * agent )
{
    const int current_len = agent->effector().getSayMessageLength();
    const int available_len = ServerParam::i().playerSayMsgSize() - current_len;
    if ( available_len < ThreePlayerMessage::slength() )
    {
        return false;
    }

    const WorldModel & wm = agent->world();

    std::vector< const PlayerObject * > candidates;

    for ( PlayerObject::Cont::const_iterator o = wm.opponentsFromSelf().begin(),
            end = wm.opponentsFromSelf().end();
        o != end;
        ++o )
    {
        if ( (*o)->seenPosCount() > 0 ) continue;
        if ( (*o)->unumCount() > 0 ) continue;

        if ( M_opponent_send_time[(*o)->unum()].cycle() >= wm.time().cycle() - 1 )
        {
            continue;
        }

        candidates.push_back( *o );
    }

    if ( candidates.size() < 3 )
    {
        return false;
    }

    std::vector< const PlayerObject * >::const_iterator first = candidates.begin();
    std::vector< const PlayerObject * >::const_iterator second = first; ++second;
    std::vector< const PlayerObject * >::const_iterator third = second; ++third;

    agent->addSayMessage( new ThreePlayerMessage( (*first)->unum() + 11,
                                                  (*first)->pos() + (*first)->vel(),
                                                  (*second)->unum() + 11,
                                                  (*second)->pos() + (*second)->vel(),
                                                  (*third)->unum() + 11,
                                                  (*third)->pos() + (*third)->vel() ) );
    M_opponent_send_time[(*first)->unum()] = wm.time();
    M_opponent_send_time[(*second)->unum()] = wm.time();
    M_opponent_send_time[(*third)->unum()] = wm.time();

    dlog.addText( Logger::COMMUNICATION,
                  __FILE__": say three oppoinents %d %d %d",
                  (*first)->unum(),
                  (*second)->unum(),
                  (*third)->unum() );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
SampleCommunication::saySelf( PlayerAgent * agent )
{
    const int current_len = agent->effector().getSayMessageLength();
    const int available_len = ServerParam::i().playerSayMsgSize() - current_len;
    if ( available_len < SelfMessage::slength() )
    {
        return false;
    }

    const WorldModel & wm = agent->world();

    // if ( std::fabs( wm.self().pos().x - wm.defenseLineX() ) > 40.0 )
    // {
    //     return false;
    // }

    if ( std::fabs( wm.self().distFromBall() ) > 35.0 )
    {
        return false;
    }

    if ( M_teammate_send_time[wm.self().unum()].cycle() >= wm.time().cycle() )
    {
        return false;
    }

    int self_min = wm.interceptTable()->selfReachCycle();
    int mate_min = wm.interceptTable()->teammateReachCycle();
    int opp_min = wm.interceptTable()->opponentReachCycle();

    if ( opp_min < self_min
         && opp_min < mate_min )
    {
        return false;
    }

    bool send_self = false;

    if ( current_len > 0 )
    {
        // if another type message is already registered, self info is appended to the message.
        send_self = true;
    }

    if ( ! send_self
         && wm.time().cycle() % 11 == ( wm.self().unum() - 1 ) )
    {
        const PlayerObject * ball_nearest_teammate = NULL;
        const PlayerObject * second_ball_nearest_teammate = NULL;

        for ( PlayerObject::Cont::const_iterator t = wm.teammatesFromBall().begin(),
              end = wm.teammatesFromBall().end();
          t != end;
          ++t )
        {
            if ( (*t)->isGhost() || (*t)->posCount() >= 10 ) continue;

            if ( ! ball_nearest_teammate )
            {
                ball_nearest_teammate = *t;
#ifdef DEBUG_PRINT
                dlog.addText( Logger::COMMUNICATION,
                              __FILE__": (saySelf) ball_nearest_teammate %d (%.1f %.1f)",
                              (*t)->unum(),
                              (*t)->pos().x,  (*t)->pos().y );
#endif
            }
            else if ( ! second_ball_nearest_teammate )
            {
                second_ball_nearest_teammate = *t;
#ifdef DEBUG_PRINT
                dlog.addText( Logger::COMMUNICATION,
                              __FILE__": (saySelf) second_ball_nearest_teammate %d (%.1f %.1f)",
                              (*t)->unum(),
                              (*t)->pos().x,  (*t)->pos().y );
#endif
                break;
            }
        }

        if ( ball_nearest_teammate
             && ball_nearest_teammate->distFromBall() < wm.ball().distFromSelf()
             && ( ! second_ball_nearest_teammate
                  || second_ball_nearest_teammate->distFromBall() > wm.ball().distFromSelf() )
             )
        {
            send_self = true;
        }
    }

    if ( send_self )
    {
        agent->addSayMessage( new SelfMessage( agent->effector().queuedNextMyPos(),
                                               agent->effector().queuedNextMyBody(),
                                               wm.self().stamina() ) );
        M_teammate_send_time[wm.self().unum()] = wm.time();

        dlog.addText( Logger::COMMUNICATION,
                      __FILE__": say self status" );
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
SampleCommunication::sayStamina( PlayerAgent * agent )
{
    const int current_len = agent->effector().getSayMessageLength();
    if ( current_len == 0 )
    {
        return false;
    }

    const int available_len = ServerParam::i().playerSayMsgSize() - current_len;
    if ( available_len < StaminaMessage::slength() )
    {
        return false;
    }

    agent->addSayMessage( new StaminaMessage( agent->world().self().stamina() ) );

    dlog.addText( Logger::COMMUNICATION,
                  __FILE__": say self stamina" );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
SampleCommunication::sayRecovery( PlayerAgent * agent )
{
    const int current_len = agent->effector().getSayMessageLength();
    const int available_len = ServerParam::i().playerSayMsgSize() - current_len;
    if ( available_len < RecoveryMessage::slength() )
    {
        return false;
    }

    agent->addSayMessage( new RecoveryMessage( agent->world().self().recovery() ) );

    dlog.addText( Logger::COMMUNICATION,
                  __FILE__": say self recovery" );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
#include "chain_action/action_chain_graph.h"
#include "chain_action/action_chain_holder.h"
void
SampleCommunication::attentiontoSomeone( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    static int last = 0;
    if(wm.time().cycle() > last){
        last = wm.time().cycle();
    }else{
        return;
    }
    if (attentiontoPasser(agent))
        return;
    if (attentiontoReceiver(agent))
        return;
    if (attentiontoOffMove(agent))
        return;
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();


    const PlayerObject * fastest_teammate = wm.interceptTable()->fastestTeammate();
    if ( wm.self().pos().x > wm.offsideLineX() - 15.0
         && wm.interceptTable()->selfReachCycle() <= 3 )
    {
        if ( currentSenderUnum() != wm.self().unum()
             && currentSenderUnum() != Unum_Unknown )
        {
            agent->doAttentionto( wm.ourSide(), currentSenderUnum() );
            agent->debugClient().addMessage( "AttCurSender%d", currentSenderUnum() );
        }
        else
        {
            std::vector< const PlayerObject * > candidates;
            for ( PlayerObject::Cont::const_iterator p = wm.teammatesFromSelf().begin(),
              end = wm.teammatesFromSelf().end();
             p != end;
                ++p )
            {
                if ( (*p)->goalie() ) continue;
                if ( (*p)->unum() == Unum_Unknown ) continue;
                if ( (*p)->pos().x > wm.offsideLineX() + 1.0 ) continue;

                if ( (*p)->distFromSelf() > 20.0 ) break;

                candidates.push_back( *p );
            }

            const Vector2D self_next = agent->effector().queuedNextSelfPos();

            const PlayerObject * target_teammate = NULL;
            double max_x = -10000000.0;
            for ( std::vector< const PlayerObject * >::const_iterator p = candidates.begin();
                  p != candidates.end();
                  ++p )
            {
                Vector2D diff = (*p)->pos() + (*p)->vel() - self_next;

                double x = diff.x * ( 1.0 - diff.absY() / 40.0 ) ;

                if ( x > max_x )
                {
                    max_x = x;
                    target_teammate = *p;
                }
            }

            if ( target_teammate )
            {
                dlog.addText( Logger::COMMUNICATION,
                              __FILE__": attentionto most front teammate",
                              wm.offsideLineX() );
                agent->debugClient().addMessage( "AttFrontMate%d", target_teammate->unum() );
                agent->doAttentionto( wm.ourSide(), target_teammate->unum() );
                return;
            }

            // maybe ball owner
            if ( wm.self().attentiontoUnum() > 0 )
            {
                dlog.addText( Logger::COMMUNICATION,
                              __FILE__": attentionto off. maybe ball owner",
                              wm.offsideLineX() );
                agent->debugClient().addMessage( "AttOffBOwner" );
                agent->doAttentiontoOff();
            }
        }

        return;
    }

    if ( fastest_teammate
         && fastest_teammate->unum() != Unum_Unknown
         && mate_min <= 1
         && mate_min < self_min
         && mate_min <= opp_min + 1
         && mate_min <= 5 + std::min( 4, fastest_teammate->posCount() )
         && wm.ball().inertiaPoint( mate_min ).dist2( agent->effector().queuedNextSelfPos() )
         < std::pow( 35.0, 2 ) )
    {
        // set attention to ball nearest teammate
        agent->debugClient().addMessage( "AttBallOwner%d", fastest_teammate->unum() );
        agent->doAttentionto( wm.ourSide(), fastest_teammate->unum() );
        return;
    }

    const PlayerObject * nearest_teammate = wm.getTeammateNearestToBall( 5 );

    if ( nearest_teammate
         && nearest_teammate->unum() != Unum_Unknown
         && opp_min <= 3
         && opp_min <= mate_min
         && opp_min <= self_min
         && nearest_teammate->distFromSelf() < 45.0
         && nearest_teammate->distFromBall() < 20.0 )
    {
        agent->debugClient().addMessage( "AttBallNearest(1)%d", nearest_teammate->unum() );
        agent->doAttentionto( wm.ourSide(), nearest_teammate->unum() );
        return;
    }

    if ( nearest_teammate
         && nearest_teammate->unum() != Unum_Unknown
         && wm.ball().posCount() >= 3
         && nearest_teammate->distFromBall() < 20.0 )
    {
        agent->debugClient().addMessage( "AttBallNearest(2)%d", nearest_teammate->unum() );
        agent->doAttentionto( wm.ourSide(), nearest_teammate->unum() );
        return;
    }

    if ( nearest_teammate
         && nearest_teammate->unum() != Unum_Unknown
         && nearest_teammate->distFromSelf() < 45.0
         && nearest_teammate->distFromBall() < 3.5 )
    {
        agent->debugClient().addMessage( "AttBallNearest(3)%d", nearest_teammate->unum() );
        agent->doAttentionto( wm.ourSide(), nearest_teammate->unum() );
        return;
    }


    // if ( ( ! wm.self().goalie()
    //        || wm.self().pos().x > ServerParam::i().ourPenaltyAreaLineX() - 1.0 )
    //      && currentSenderUnum() != wm.self().unum()
    //      && currentSenderUnum() != Unum_Unknown )
    // {
    //     agent->doAttentionto( wm.ourSide(), currentSenderUnum() );
    //     agent->debugClient().addMessage( "AttCurrentSender%d", currentSenderUnum() );
    //     return;
    // }

    if ( currentSenderUnum() != wm.self().unum()
         && currentSenderUnum() != Unum_Unknown )
    {
        agent->doAttentionto( wm.ourSide(), currentSenderUnum() );
        agent->debugClient().addMessage( "AttCurSender%d", currentSenderUnum() );
    }
    else
    {
        agent->debugClient().addMessage( "AttOff" );
        agent->doAttentiontoOff();
    }
}

bool
SampleCommunication::attentiontoPasser( PlayerAgent * agent ){
    const WorldModel & wm = agent->world();
    static int last_time_inten[12]={0};
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();
    const PlayerObject * fastest_teammate = wm.interceptTable()->fastestTeammate();
    int unum = 0;
    static int passer = 0;
    static int passer_cycle = 0;
    if(wm.audioMemory().pass().size() > 0
            && wm.audioMemory().passTime().cycle() == wm.time().cycle()/*
            && wm.audioMemory().pass().front().receiver_ == wm.self().unum()*/){
        passer = wm.audioMemory().pass().front().sender_;
        passer_cycle = wm.time().cycle();
    }
    if(passer_cycle == wm.time().cycle()){
        unum = passer;
    }else if(fastest_teammate)
        unum = fastest_teammate->unum();
    static int shood = 0;
    if(unum != 0 /*&& unum != wm.self ().unum ()*/){
        if(mate_min <=2){
            shood = 1;
            agent->doAttentionto(wm.ourSide(),unum);
            agent->debugClient().addMessage("wantAtToRecPass %d",unum);
            return true;
        }else if(shood > 0 && shood <= 2){
            shood -= 1;
            agent->doAttentionto(wm.ourSide(),unum);
            agent->debugClient().addMessage("wantAtToRecPass %d",unum);
            return true;
        }
    }
    shood = 0;
    return false;
}
bool
SampleCommunication::attentiontoReceiver( PlayerAgent * agent ){
    const WorldModel & wm = agent->world();

    static int last_time_inten[12]={0};
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();

    if(self_min <= opp_min && self_min <= mate_min){
        if(wm.ball().pos().x > 20 && wm.teammatesFromBall().size() > 0 && wm.teammatesFromBall().front()->distFromBall() < 3)
            agent->doAttentionto(wm.ourSide(),wm.teammatesFromBall().front()->unum());
        return true;
    }
    if(self_min <= opp_min && self_min <= mate_min){
        agent->debugClient().addMessage("attOff");
        if(self_min < 3){
            const ActionChainGraph & chain_graph = ActionChainHolder::i().graph();
            const CooperativeAction & first_action = chain_graph.getFirstAction();
            int unum = 0;
            switch (first_action.category()) {
            case CooperativeAction::Pass: {
                unum = first_action.targetPlayerUnum();
                break;
            }
            }
            if(wm.opponentsFromBall().size() > 0){
                if(wm.opponentsFromBall().front()->distFromBall() < 3){
                    if(wm.teammatesFromBall().size() > 0){
                        if(wm.time().cycle() % 2 == 0){
                            agent->doAttentionto(wm.ourSide(),wm.teammatesFromBall().front()->unum());
                            agent->debugClient().addMessage("wantAtToForSendPassDanger %d",wm.teammatesFromBall().front()->unum());
                            return true;
                        }
                    }
                }
            }
            if(unum != 0){
                agent->doAttentionto(wm.ourSide(),unum);
                agent->debugClient().addMessage("wantAtToForSendPass %d",unum);
                return true;
            }
        }
        int eval[12] = {1000};
        Vector2D ball_pos = wm.ball().pos();
        for(int t = 1;t <= 11;t+=1){
            const AbstractPlayerObject * tm = wm.ourPlayer(t);
            if(t == wm.self().unum())
                continue;
            bool use_pos = true;
            if(tm == NULL || tm->unum() <= 0){
                use_pos = false;
            }

            if(ball_pos.x > 0){
                if(ball_pos.y > 20){
                    if(ball_pos.x > 20){
                        if(Strategy::i().tm_Line(t) == Strategy::PostLine::golie
                                ||Strategy::i().tm_Line(t) == Strategy::PostLine::back
                                || Strategy::i().tm_Post(t) == Strategy::pp_lh)
                            continue;
                    }else{
                        if(Strategy::i().tm_Line(t) == Strategy::PostLine::golie
                                ||Strategy::i().tm_Post(t) == Strategy::pp_lb
                                || Strategy::i().tm_Post(t) == Strategy::pp_lh)
                            continue;
                    }
                }else if(ball_pos.y < -20){
                    if(ball_pos.x > 20){
                        if(Strategy::i().tm_Line(t) == Strategy::PostLine::golie
                                ||Strategy::i().tm_Line(t) == Strategy::PostLine::back
                                || Strategy::i().tm_Post(t) == Strategy::pp_rh)
                            continue;
                    }else{
                        if(Strategy::i().tm_Line(t) == Strategy::PostLine::golie
                                ||Strategy::i().tm_Post(t) == Strategy::pp_rb
                                || Strategy::i().tm_Post(t) == Strategy::pp_rh)
                            continue;
                    }
                }else{
                    if(Strategy::i().tm_Line(t) == Strategy::PostLine::golie
                            ||Strategy::i().tm_Line(t) == Strategy::PostLine::back)
                        continue;
                }
            }else if(ball_pos.x < -25){
                if(Strategy::i().tm_Line(t) == Strategy::PostLine::forward)
                    continue;
            }else{

            }
            if(use_pos)
                if(tm->pos().dist(ball_pos) > 30)
                    continue;
            eval[t] = wm.time().cycle() - last_time_inten[t];
        }
        int best_eval = -1;
        int best_tm = 0;
        for(int t = 1;t<=11;t++){
            if(eval[t] > best_eval){
                best_eval = eval[t];
                best_tm = t;
            }
        }
        if(best_tm > 0){
            agent->doAttentionto(wm.ourSide(),best_tm);
            last_time_inten[best_tm] = wm.time().cycle();
            agent->debugClient().addMessage("wantAtToForPrepareChain %d",best_tm);
            return true;
        }
    }
    return false;
}
bool
SampleCommunication::attentiontoOffMove( PlayerAgent * agent ){
    const WorldModel & wm = agent->world();

    static int last_time_inten[12]={0};
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();


    if(!Strategy::i().isDefSit(wm,wm.self().unum())){
        agent->debugClient().addMessage("attOffmove");
        int eval[12] = {1000};
        Vector2D ball_pos = wm.ball().pos();
        for(int t = 1;t <= 11;t+=1){
            const AbstractPlayerObject * tm = wm.ourPlayer(t);
            if(t == wm.self().unum())
                continue;
            bool use_pos = true;
            if(tm == NULL || tm->unum() <= 0){
                use_pos = false;
            }
            if(ball_pos.x > 0){
                if(ball_pos.y > 20){
                    if(ball_pos.x > 20){
                        if(Strategy::i().tm_Line(t) == Strategy::PostLine::golie
                                ||Strategy::i().tm_Line(t) == Strategy::PostLine::back
                                || Strategy::i().tm_Post(t) == Strategy::pp_lh)
                            continue;
                    }else{
                        if(Strategy::i().tm_Line(t) == Strategy::PostLine::golie
                                ||Strategy::i().tm_Post(t) == Strategy::pp_lb
                                || Strategy::i().tm_Post(t) == Strategy::pp_lh)
                            continue;
                    }
                }else if(ball_pos.y < -20){
                    if(ball_pos.x > 20){
                        if(Strategy::i().tm_Line(t) == Strategy::PostLine::golie
                                ||Strategy::i().tm_Line(t) == Strategy::PostLine::back
                                || Strategy::i().tm_Post(t) == Strategy::pp_rh)
                            continue;
                    }else{
                        if(Strategy::i().tm_Line(t) == Strategy::PostLine::golie
                                ||Strategy::i().tm_Post(t) == Strategy::pp_rb
                                || Strategy::i().tm_Post(t) == Strategy::pp_rh)
                            continue;
                    }
                }else{
                    if(Strategy::i().tm_Line(t) == Strategy::PostLine::golie
                            ||Strategy::i().tm_Line(t) == Strategy::PostLine::back)
                        continue;
                }
            }else if(ball_pos.x < -25){
                if(Strategy::i().tm_Line(t) == Strategy::PostLine::forward)
                    continue;
            }else{

            }
            if(use_pos)
                if(tm->pos().dist(ball_pos) > 30)
                    continue;
            eval[t] = wm.time().cycle() - last_time_inten[t];
        }
        int best_eval = -1;
        int best_tm = 0;
        for(int t = 1;t<=11;t++){
            if(eval[t] > best_eval){
                best_eval = eval[t];
                best_tm = t;
            }
        }
        if(best_tm > 0){
            agent->doAttentionto(wm.ourSide(),best_tm);
            last_time_inten[best_tm] = wm.time().cycle();
            agent->debugClient().addMessage("wantAtToForPrepareChain %d",best_tm);
            return true;
        }
    }
    return false;
}

void
SampleCommunication::attentiontoOther( PlayerAgent * agent ){

}
