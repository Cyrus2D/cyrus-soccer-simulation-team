// -*-c++-*-

/*!
 \file strict_check_pass_generator.cpp
 \brief strict checked pass course generator Source File
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

#include "strict_check_pass_generator.h"

#include "pass.h"
#include "field_analyzer.h"
#include "sample_player.h"

#include <rcsc/player/world_model.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/common/audio_memory.h>
#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/common/player_type.h>
#include <rcsc/math_util.h>
#include <rcsc/timer.h>
#include "../strategy.h"
#include <rcsc/player/player_agent.h>

#include <algorithm>
#include <limits>
#include <sstream>
#include <cmath>
#include "../debugs.h"
#include "../setting.h"

// #define CREATE_SEVERAL_CANDIDATES_ON_SAME_POINT

using namespace rcsc;

namespace {

    inline void
    debug_paint_failed_pass(const int count,
                            const Vector2D &receive_point,
                            std::int32_t logger)
    {
        dlog.addRect(Logger::PASS,
                    receive_point.x - 0.1, receive_point.y - 0.1,
                    0.2, 0.2,
                    "#ff0000");
        if (count >= 0)
        {
            char num[8];
            snprintf(num, 8, "%d", count);
            dlog.addMessage(logger, receive_point, num);
        }
}
}


/*-------------------------------------------------------------------*/
/*!

 */
std::vector<bool> StrictCheckPassGenerator::Receivers_unum;
StrictCheckPassGenerator::Receiver::Receiver(const AbstractPlayerObject * p,
                                             const Vector2D & first_ball_pos) :
    player_(p), pos_(p->seenPosCount() <= p->posCount() ? p->seenPos() : p->pos()),
    vel_(p->seenVelCount() <= p->velCount() ? p->seenVel() : p->vel()),
    inertia_pos_(p->playerTypePtr()->inertiaFinalPoint(pos_, vel_)),
    speed_(vel_.r()),
    penalty_distance_(FieldAnalyzer::estimate_virtual_dash_distance(p)),
    penalty_step_(p->playerTypePtr()->cyclesToReachDistance(penalty_distance_)),
    angle_from_ball_((p->pos() - first_ball_pos).th()) {

}

/*-------------------------------------------------------------------*/
/*!

 */
StrictCheckPassGenerator::Opponent::Opponent(const AbstractPlayerObject * p) :
    player_(p), pos_(
                    p->seenPosCount() <= p->posCount() ? p->seenPos() : p->pos()), vel_(
                                                                                       p->seenVelCount() <= p->velCount() ? p->seenVel() : p->vel()), speed_(
                                                                                                                                                          vel_.r()), bonus_distance_(
                                                                                                                                                                         FieldAnalyzer::estimate_virtual_dash_distance(p)) {

}

/*-------------------------------------------------------------------*/
/*!

 */
StrictCheckPassGenerator::StrictCheckPassGenerator() :
    M_update_time(-1, 0), M_total_count(0), M_pass_type('-'), M_passer(
                                                                  static_cast<AbstractPlayerObject *>(0)), M_start_time(-1, 0) {
    M_receiver_candidates.reserve(11);
    M_opponents.reserve(16);
    M_courses.reserve(1024);

    clear();
}

/*-------------------------------------------------------------------*/
/*!

 */
StrictCheckPassGenerator &
StrictCheckPassGenerator::instance() {
    static StrictCheckPassGenerator s_instance;
    return s_instance;
}

/*-------------------------------------------------------------------*/
/*!

 */
void StrictCheckPassGenerator::clear() {
    M_total_count = 0;
    M_pass_type = '-';
    M_passer = static_cast<AbstractPlayerObject *>(0);
    M_start_time.assign(-1, 0);
    M_first_point.invalidate();
    M_receiver_candidates.clear();
    M_opponents.clear();
    M_direct_size = M_leading_size = M_through_size = 0;
    M_courses.clear();
}

/*-------------------------------------------------------------------*/
/*!

 */
void StrictCheckPassGenerator::generate(const WorldModel & wm) {
    if (M_update_time == wm.time()) {
        return;
    }
    M_update_time = wm.time();

    clear();

    if (wm.time().stopped() > 0 || wm.gameMode().isPenaltyKickMode()) {
        return;
    }

    #ifdef DEBUG_PASS_TIME_CALC
    Timer timer;
    #endif
    updatePasser(wm);

    if (!M_passer || !M_first_point.isValid()) {
        #ifdef DEBUG_PASS
        dlog.addText(Logger::PASS," (generate) passer not found.");
        #endif
        return;
    }else{
        #ifdef DEBUG_PASS
        dlog.addText(Logger::PASS, " passer is %",M_passer->unum());
        #endif
    }
    updateReceivers(wm);

    if (M_receiver_candidates.empty()) {
        #ifdef DEBUG_PASS
        dlog.addText(Logger::PASS, " (generate) no receiver.");
        #endif
        return;
    }

    updateOpponents(wm);
    createCourses(wm);
    std::sort(M_courses.begin(), M_courses.end(),
              CooperativeAction::DistCompare(
                  ServerParam::i().theirTeamGoalPos()));

    #ifdef DEBUG_PASS_TIME_CALC
    if (M_passer->unum() == wm.self().unum()) {
        dlog.addText(Logger::PASS,
                     " (generate) PROFILE passer=self size=%d/%d D=%d L=%d T=%d elapsed %f [ms]",
                     (int) M_courses.size(), M_total_count, M_direct_size,
                     M_leading_size, M_through_size, timer.elapsedReal());
    } else {
        dlog.addText(Logger::PASS,
                     " (update) PROFILE passer=%d size=%d/%d D=%d L=%d T=%d elapsed %f [ms]",
                     M_passer->unum(), (int) M_courses.size(), M_total_count,
                     M_direct_size, M_leading_size, M_through_size,
                     timer.elapsedReal());
    }
    #endif
}

/*-------------------------------------------------------------------*/
/*!

 */
void StrictCheckPassGenerator::updatePasser(const WorldModel & wm) {
    #ifdef DEBUG_PASS_UPDATE_PASSER
    dlog.addText( Logger::PASS, " ================ Update Passer ===================" );
    #endif
    if (wm.self().isKickable() && !wm.self().isFrozen()) {
        M_passer = &wm.self();
        M_start_time = wm.time();
        M_first_point = wm.ball().pos();
        #ifdef DEBUG_PASS_UPDATE_PASSER
        dlog.addText( Logger::PASS, " (updatePasser) self kickable." );
        #endif
        return;
    }

    int s_min = wm.interceptTable()->selfReachCycle();
    int t_min = wm.interceptTable()->teammateReachCycle();
    int o_min = wm.interceptTable()->opponentReachCycle();

    int our_min = std::min(s_min, t_min);
    if (o_min < std::min(our_min - 4, (int) rint(our_min * 0.9))) {
        #ifdef DEBUG_PASS_UPDATE_PASSER
        dlog.addText(Logger::PASS, " (updatePasser) opponent ball.");
        #endif
        return;
    }

    if (s_min <= t_min) {
        if (s_min <= 2) {
            M_passer = &wm.self();
            M_first_point = wm.ball().inertiaPoint(s_min);
        }
    } else {
        if (t_min <= 2) {
            M_passer = wm.interceptTable()->fastestTeammate();
            M_first_point = wm.ball().inertiaPoint(t_min);
        }
    }

    if (!M_passer) {
        #ifdef DEBUG_PASS_UPDATE_PASSER
        dlog.addText(Logger::PASS, " (updatePasser) no passer.");
        #endif
        return;
    }

    M_start_time = wm.time();
    if (!wm.gameMode().isServerCycleStoppedMode()) {
        M_start_time.addCycle(t_min);
    }

    if (M_passer->unum() != wm.self().unum()) {
        if (M_first_point.dist2(wm.self().pos()) > std::pow(30.0, 2)) {
            M_passer = static_cast<const AbstractPlayerObject *>(0);
            #ifdef DEBUG_PASS_UPDATE_PASSER
            dlog.addText(Logger::PASS, " (updatePasser) passer is too far.");
            #endif
            return;
        }
    }

    #ifdef DEBUG_PASS_UPDATE_PASSER
    dlog.addText( Logger::PASS, " (updatePasser) passer=%d(%.1f %.1f) reachStep=%d startPos=(%.1f %.1f)",
                  M_passer->unum(),
                  M_passer->pos().x, M_passer->pos().y,
                  t_min,
                  M_first_point.x, M_first_point.y );
    #endif
}

/*-------------------------------------------------------------------*/
/*!

 */
struct ReceiverAngleCompare {

    bool operator()(const StrictCheckPassGenerator::Receiver & lhs,
                    const StrictCheckPassGenerator::Receiver & rhs) const {
        return lhs.angle_from_ball_.degree() < rhs.angle_from_ball_.degree();
    }
};

/*-------------------------------------------------------------------*/
/*!

 */
namespace {
struct ReceiverDistCompare {

    const Vector2D pos_;

    ReceiverDistCompare(const Vector2D & pos) :
        pos_(pos) {
    }

    bool operator()(const StrictCheckPassGenerator::Receiver & lhs,
                    const StrictCheckPassGenerator::Receiver & rhs) const {
        return lhs.pos_.dist2(pos_) < rhs.pos_.dist2(pos_);
    }
};
}

/*-------------------------------------------------------------------*/
/*!

 */
void StrictCheckPassGenerator::updateReceivers(const WorldModel & wm) {
    #ifdef DEBUG_PASS_UPDATE_RECEIVERS
    dlog.addText(Logger::PASS, "=============== Update Receiver =================");
    #endif
    const ServerParam & SP = ServerParam::i();
    const double max_dist2 = std::pow(40.0, 2); // Magic Number

    const bool is_self_passer = (M_passer->unum() == wm.self().unum());

    // for (AbstractPlayerCont::const_iterator p = wm.ourPlayers().begin(), end =
    //      wm.ourPlayers().end(); p != end; ++p) {
    for ( AbstractPlayerObject::Cont::const_iterator p = wm.ourPlayers().begin(),
              end = wm.ourPlayers().end();
          p != end;
          ++p )
    {
        if (*p == M_passer)
            continue;

        if (is_self_passer) {
            // if ( (*p)->isGhost() ) continue;
            if ((*p)->unum() == Unum_Unknown){
                continue;
            }
            if ((*p)->posCount() > (wm.self().isKickable()?10:20)){
                #ifdef DEBUG_PASS_UPDATE_RECEIVERS
                dlog.addText(Logger::PASS,
                             "(updateReceiver) unum=%d > poscount=%d",
                             (*p)->unum(), (*p)->posCount());
                #endif
                continue;
            }
            if ((*p)->isTackling()){
                #ifdef DEBUG_PASS_UPDATE_RECEIVERS
                dlog.addText(Logger::PASS,
                             "(updateReceiver) unum=%d > tackling",
                             (*p)->unum());
                #endif
                continue;
            }
            if ((*p)->pos().x > wm.offsideLineX() + (wm.self().isKickable()?0:5)) {
                #ifdef DEBUG_PASS_UPDATE_RECEIVERS
                dlog.addText(Logger::PASS,
                             "(updateReceiver) unum=%d (%.2f %.2f) > offside=%.2f",
                             (*p)->unum(), (*p)->pos().x, (*p)->pos().y,
                             wm.offsideLineX());
                #endif
                continue;
            }
            if ((*p)->goalie()
                    && (*p)->pos().x < SP.ourPenaltyAreaLineX() + 15.0 && wm.interceptTable()->opponentReachCycle() > 3) {
                #ifdef DEBUG_PASS_UPDATE_RECEIVERS
                dlog.addText(Logger::PASS,
                             "(updateReceiver) unum=%d > goalie near goal and reach cycle more than 3",
                             (*p)->unum());
                #endif
                continue;
            }
            if ((*p)->goalie()
                    && (*p)->pos().x < SP.ourPenaltyAreaLineX()) {
                #ifdef DEBUG_PASS_UPDATE_RECEIVERS
                dlog.addText(Logger::PASS,
                             "(updateReceiver) unum=%d > goalie danger area",
                             (*p)->unum());
                #endif
                continue;
            }
        } else {
            // ignore other players
            if ((*p)->unum() != wm.self().unum()) {
                #ifdef DEBUG_PASS_UPDATE_RECEIVERS
                dlog.addText(Logger::PASS,
                             "(updateReceiver) unum=%d > ignore other player",
                             (*p)->unum());
                #endif
                continue;
            }
        }

        if ((*p)->pos().dist2(M_first_point) > max_dist2){
            #ifdef DEBUG_PASS_UPDATE_RECEIVERS
            dlog.addText(Logger::PASS,
                             "(updateReceiver) unum=%d > dist=%.1f more than max",
                             (*p)->unum(), (*p)->pos().dist(M_first_point));
            #endif
            continue;
        }

        M_receiver_candidates.push_back(Receiver(*p, M_first_point));
    }

    std::sort(M_receiver_candidates.begin(), M_receiver_candidates.end(),
              ReceiverDistCompare(SP.theirTeamGoalPos()));

    // std::sort( M_receiver_candidates.begin(),
    //            M_receiver_candidates.end(),
    //            ReceiverAngleCompare() );

    #ifdef DEBUG_PASS_UPDATE_RECEIVERS
    for ( ReceiverCont::const_iterator p = M_receiver_candidates.begin();
          p != M_receiver_candidates.end();
          ++p )
    {
        dlog.addText( Logger::PASS,
                      "StrictPass receiver %d pos(%.1f %.1f) inertia=(%.1f %.1f) vel(%.2f %.2f)"
                      " posCount %d velCount %d bodyCount %d penalty_dist=%.3f penalty_step=%d"
                      " angle=%.1f",
                      p->player_->unum(),
                      p->pos_.x, p->pos_.y,
                      p->inertia_pos_.x, p->inertia_pos_.y,
                      p->player_->posCount(), p->player_->velCount(), p->player_->bodyCount(),
                      p->vel_.x, p->vel_.y,
                      p->penalty_distance_,
                      p->penalty_step_,
                      p->angle_from_ball_.degree() );
    }
    #endif
}

/*-------------------------------------------------------------------*/
/*!

 */
void StrictCheckPassGenerator::updateOpponents(const WorldModel & wm) {
    #ifdef DEBUG_PASS_UPDATE_OPPONENT
    const Opponent & o = M_opponents.back();
    dlog.addText( Logger::PASS, "====================== Update Opponents ===================");
    #endif
    // for (AbstractPlayerCont::const_iterator p = wm.theirPlayers().begin(), end =
    //      wm.theirPlayers().end(); p != end; ++p) {
    for ( AbstractPlayerObject::Cont::const_iterator p = wm.theirPlayers().begin(),
              end = wm.theirPlayers().end();
          p != end;
          ++p )
    {
        M_opponents.push_back(Opponent(*p));
        #ifdef DEBUG_PASS_UPDATE_OPPONENT
        const Opponent & o = M_opponents.back();
        dlog.addText( Logger::PASS,
                      "StrictPass opp %d pos(%.1f %.1f) vel(%.2f %.2f) bonus_dist=%.3f",
                      o.player_->unum(),
                      o.pos_.x, o.pos_.y,
                      o.vel_.x, o.vel_.y,
                      o.bonus_distance_ );
        #endif
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void StrictCheckPassGenerator::createCourses(const WorldModel & wm) {
    #ifdef DEBUG_PASS
        dlog.addMessage(Logger::PASS, Vector2D(-52, -33), "HELP");
        dlog.addMessage(Logger::PASS, Vector2D(-52, -32), "RED shows failed");
        dlog.addMessage(Logger::PASS, Vector2D(-52, -31), "BLACK shows no safe with noise");
        dlog.addMessage(Logger::PASS, Vector2D(-52, -30), "Blue shows danger");
        dlog.addMessage(Logger::PASS, Vector2D(-52, -29), "Green shows succeed");
    #endif
    const ReceiverCont::iterator end = M_receiver_candidates.end();
    M_pass_type = 'D';
    M_pass_logger = Logger::D_PASS;
    M_pass_logger = Logger::PASS;
    for (ReceiverCont::iterator p = M_receiver_candidates.begin(); p != end;
         ++p) {
        createDirectPass(wm, *p);
    }
    M_pass_type = 'L';
    M_pass_logger = Logger::L_PASS;
    M_pass_logger = Logger::PASS;
    for (ReceiverCont::iterator p = M_receiver_candidates.begin(); p != end;
         ++p) {
        createLeadingPass(wm, *p);
    }
    M_pass_type = 'T';
    M_pass_logger = Logger::TH_PASS;
    M_pass_logger = Logger::PASS;

    for (ReceiverCont::iterator p = M_receiver_candidates.begin(); p != end;
         ++p) {
        createThroughPass(wm, *p);
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void StrictCheckPassGenerator::createDirectPass(const WorldModel & wm,
                                                const Receiver & receiver) {
    static const int MIN_RECEIVE_STEP = 3;
    #ifdef CREATE_SEVERAL_CANDIDATES_ON_SAME_POINT
    static const int MAX_RECEIVE_STEP = 15; // Magic Number
    #endif

    static const double MIN_DIRECT_PASS_DIST =
            ServerParam::i().defaultKickableArea() * 2.2;
    static const double MAX_DIRECT_PASS_DIST = 0.8
            * inertia_final_distance(ServerParam::i().ballSpeedMax(),
                                     ServerParam::i().ballDecay());
    static const double MAX_RECEIVE_BALL_SPEED = ServerParam::i().ballSpeedMax()
            * std::pow(ServerParam::i().ballDecay(), MIN_RECEIVE_STEP);

    const ServerParam & SP = ServerParam::i();

    //
    // check receivable area
    //
    if (receiver.pos_.x > SP.pitchHalfLength() - 1.5) {
        if (M_first_point.x > receiver.pos_.x
            || receiver.pos_.absY() < 7){

        }
        else{
            #ifdef DEBUG_PASS
            M_total_count ++;
            dlog.addText( M_pass_logger,
                          "%d: xxx (direct) unum=%d outOfBounds pos=(%.2f %.2f)",
                          M_total_count, receiver.player_->unum(),
                          receiver.pos_.x, receiver.pos_.y );
            debug_paint_failed_pass( M_total_count, receiver.pos_, M_pass_logger );
            #endif
            return;
        }

    }
    if (receiver.pos_.x < -SP.pitchHalfLength() + 5.0) {
        if (M_first_point.x + 1.0 < receiver.pos_.x
            && receiver.pos_.absY() > 8){

        }
        else{
            #ifdef DEBUG_PASS
            M_total_count ++;
            dlog.addText( M_pass_logger,
                          "%d: xxx (direct) unum=%d outOfBounds pos=(%.2f %.2f)",
                          M_total_count, receiver.player_->unum(),
                          receiver.pos_.x, receiver.pos_.y );
            debug_paint_failed_pass( M_total_count, receiver.pos_, M_pass_logger );
            #endif
            return;
        }
    }
    if (receiver.pos_.absY() > SP.pitchHalfWidth() - 1.5) {
        if (M_first_point.absY() > receiver.pos_.absY()){

        }
        else{
            #ifdef DEBUG_PASS
            M_total_count ++;
            dlog.addText( M_pass_logger,
                          "%d: xxx (direct) unum=%d outOfBounds pos=(%.2f %.2f)",
                          M_total_count, receiver.player_->unum(),
                          receiver.pos_.x, receiver.pos_.y );
            debug_paint_failed_pass( M_total_count, receiver.pos_, M_pass_logger );
            #endif
        }
        //TODO
        return;
    }

    //
    // avoid dangerous area
    //
    if (receiver.pos_.x < M_first_point.x + 1.0
            && receiver.pos_.dist2(SP.ourTeamGoalPos()) < std::pow(18.0, 2)) {
        #ifdef DEBUG_PASS
        M_total_count ++;
        dlog.addText( M_pass_logger,
                      "%d: xxx (direct) unum=%d dangerous pos=(%.2f %.2f)",
                      M_total_count, receiver.player_->unum(),
                      receiver.pos_.x, receiver.pos_.y );
        debug_paint_failed_pass( M_total_count, receiver.pos_, M_pass_logger );
        #endif
        return;
    }

    int max_pos_count = 1;
    if(wm.opponentsFromBall().size() > 0
        && wm.opponentsFromBall().at(0) != nullptr
        && wm.opponentsFromBall().at(0)->unum() > 0
        && wm.opponentsFromBall().at(0)->distFromBall() < 4){
        max_pos_count = 3;
    }
    if (receiver.player_->posCount() > max_pos_count){
        #ifdef DEBUG_PASS
        M_total_count ++;
        dlog.addText( M_pass_logger,
                      "%d: xxx (direct) unum=%d pos count %d",
                      M_total_count, receiver.player_->unum(),
                      receiver.player_->posCount());
        debug_paint_failed_pass( M_total_count, receiver.pos_, M_pass_logger );
        #endif
        return;
    }

    const PlayerType * ptype = receiver.player_->playerTypePtr();

    const double max_ball_speed = (
                wm.gameMode().type() == GameMode::PlayOn ? SP.ballSpeedMax() :
                                                           wm.self().isKickable() ?
                                                               wm.self().kickRate() * SP.maxPower() :
                                                               SP.kickPowerRate() * SP.maxPower());
    const double min_ball_speed = SP.defaultRealSpeedMax();

    const Vector2D receive_point = receiver.inertia_pos_;

    const double ball_move_dist = M_first_point.dist(receive_point);

    if (ball_move_dist < MIN_DIRECT_PASS_DIST
            || MAX_DIRECT_PASS_DIST < ball_move_dist) {
        #ifdef DEBUG_PASS
        M_total_count ++;
        dlog.addText( M_pass_logger,
                      "%d: xxx (direct) unum=%d overBallMoveDist=%.3f minDist=%.3f maxDist=%.3f",
                      M_total_count, receiver.player_->unum(),
                      ball_move_dist,
                      MIN_DIRECT_PASS_DIST, MAX_DIRECT_PASS_DIST );
        debug_paint_failed_pass( M_total_count, receiver.pos_, M_pass_logger );
        #endif
        return;
    }

    if (wm.gameMode().type() == GameMode::GoalKick_
            && receive_point.x < SP.ourPenaltyAreaLineX() + 1.0
            && receive_point.absY() < SP.penaltyAreaHalfWidth() + 1.0) {
        #ifdef DEBUG_PASS
        M_total_count ++;
        dlog.addText( M_pass_logger,
                      "%d: xxx (direct) unum=%d, goal_kick",
                      M_total_count, receiver.player_->unum() );
        debug_paint_failed_pass( M_total_count, receiver.pos_, M_pass_logger );
        #endif
        return;
    }

    //
    // decide ball speed range
    //

    const double max_receive_ball_speed = std::min(MAX_RECEIVE_BALL_SPEED,
                                                   ptype->kickableArea()
                                                   + (SP.maxDashPower() * ptype->dashPowerRate()
                                                      * ptype->effortMax()) * 1.8);
    const double min_receive_ball_speed = ptype->realSpeedMax();

    const AngleDeg ball_move_angle = (receive_point - M_first_point).th();

    const int min_ball_step = SP.ballMoveStep(SP.ballSpeedMax(),
                                              ball_move_dist);

    const int start_step = std::max(std::max(MIN_RECEIVE_STEP, min_ball_step),
                                    receiver.penalty_step_);
    #ifdef CREATE_SEVERAL_CANDIDATES_ON_SAME_POINT
    const int max_step = std::max( MAX_RECEIVE_STEP, start_step + 2 );
    #else
    const int max_step = start_step + 2;
    #endif

    #ifdef DEBUG_PASS
    dlog.addText( M_pass_logger, "--------------------------------------");
    dlog.addText( M_pass_logger,
                  "(direct) unum=%d stepRange=[%d, %d]",
                  receiver.player_->unum(),
                  start_step, max_step );
    #endif

    createPassCommon(wm, receiver, receive_point, start_step, max_step,
                     min_ball_speed, max_ball_speed, min_receive_ball_speed,
                     max_receive_ball_speed, ball_move_dist, ball_move_angle,0,
                     "strictDirect");
    #ifdef DEBUG_PASS
    dlog.addText( M_pass_logger,
                  "--------------------------------------");
    #endif
}

/*-------------------------------------------------------------------*/
/*!

 */
void StrictCheckPassGenerator::createLeadingPass(const WorldModel & wm,
                                                 const Receiver & receiver) {
    static const double OUR_GOAL_DIST_THR2 = std::pow(16.0, 2);

    static const int MIN_RECEIVE_STEP = 4;
    #ifdef CREATE_SEVERAL_CANDIDATES_ON_SAME_POINT
    static const int MAX_RECEIVE_STEP = 20;
    #endif

    static const double MIN_LEADING_PASS_DIST = 3.0;
    static const double MAX_LEADING_PASS_DIST = 0.8
            * inertia_final_distance(ServerParam::i().ballSpeedMax(),
                                     ServerParam::i().ballDecay());
    static const double MAX_RECEIVE_BALL_SPEED = ServerParam::i().ballSpeedMax()
            * std::pow(ServerParam::i().ballDecay(), MIN_RECEIVE_STEP);

    static const int ANGLE_DIVS = 24;//24;
    static const double ANGLE_STEP = 360.0 / ANGLE_DIVS;
    double DIST_DIVS = 7;
    static const double DIST_STEP = 1.1;
    if(receiver.player_!= nullptr && receiver.player_->unum() > 0){
     if(receiver.player_->seenStaminaCount() < 15 && receiver.player_->seenStamina() < 3500)
         DIST_DIVS = 1;
    }
    const ServerParam & SP = ServerParam::i();
    const PlayerType * ptype = receiver.player_->playerTypePtr();

    const double max_ball_speed = (
                wm.gameMode().type() == GameMode::PlayOn ? SP.ballSpeedMax() :
                                                           wm.self().isKickable() ?
                                                               wm.self().kickRate() * SP.maxPower() :
                                                               SP.kickPowerRate() * SP.maxPower());
    const double min_ball_speed = SP.defaultRealSpeedMax();

    const double max_receive_ball_speed = std::min(MAX_RECEIVE_BALL_SPEED,
                                                   ptype->kickableArea()
                                                   + (SP.maxDashPower() * ptype->dashPowerRate()
                                                      * ptype->effortMax()) * 1.5);
    const double min_receive_ball_speed = 0.001;

    const Vector2D our_goal = SP.ourTeamGoalPos();

    //
    // distance loop
    //
    for (int d = 1; d <= DIST_DIVS; ++d) {
        if ( !SamplePlayer::canProcessMore() )
             return;
        double player_move_dist = DIST_STEP * d;
        int a_step = (
                    player_move_dist * 2.0 * M_PI / ANGLE_DIVS < 0.6 ? 2 : 1);

        if(d > 4){
            player_move_dist = DIST_STEP * d * 2.0;
            a_step = 4;
        }
        // const int move_dist_penalty_step
        //     = static_cast< int >( std::floor( player_move_dist * 0.3 ) );

        //
        // angle loop
        //
        for (int a = 0; a < ANGLE_DIVS; a += a_step) {
            const AngleDeg angle = receiver.angle_from_ball_ + ANGLE_STEP * a;
            const Vector2D receive_point = receiver.inertia_pos_
                    + Vector2D::from_polar(player_move_dist, angle);

            int move_dist_penalty_step = 0;
            {
                Line2D ball_move_line(M_first_point, receive_point);
                double player_line_dist = ball_move_line.dist(receiver.pos_);

                move_dist_penalty_step = static_cast<int>(std::floor(
                                                              player_line_dist * 0.3));
            }

            #ifdef DEBUG_PASS
            dlog.addText( M_pass_logger,
                          "--------------------------------");
            dlog.addText( M_pass_logger,
                          "(lead) unum=%d receivePoint=(%.1f %.1f)",
                          receiver.player_->unum(),
                          receive_point.x, receive_point.y );
            #endif

            if (receive_point.x > SP.pitchHalfLength() - 1.0
                    || receive_point.x < -SP.pitchHalfLength() + 3.0
                    || receive_point.absY() > SP.pitchHalfWidth() - 1.0) {
                #ifdef DEBUG_PASS
                M_total_count ++;
                dlog.addText( M_pass_logger,
                              "%d: xxx (lead) unum=%d outOfBounds pos=(%.2f %.2f)",
                              M_total_count, receiver.player_->unum(),
                              receive_point.x, receive_point.y );
                debug_paint_failed_pass( M_total_count, receive_point, M_pass_logger );
                #endif
                continue;
            }

            if (receive_point.absY() > SP.pitchHalfWidth() - 2.5) {
                if(receive_point.absY() > M_first_point.absY()){
                    #ifdef DEBUG_PASS
                    M_total_count ++;
                    dlog.addText( M_pass_logger,
                                  "%d: xxx (lead) unum=%d outOfBounds pos=(%.2f %.2f)",
                                  M_total_count, receiver.player_->unum(),
                                  receive_point.x, receive_point.y );
                    debug_paint_failed_pass( M_total_count, receive_point, M_pass_logger );
                    #endif
                    continue;
                }
            }
            if (receive_point.x > SP.pitchHalfLength() - 3.0) {
                if(receive_point.x > M_first_point.x && receive_point.absY() > 7.0){
                    #ifdef DEBUG_PASS
                    M_total_count ++;
                    dlog.addText( M_pass_logger,
                                  "%d: xxx (lead) unum=%d outOfBounds pos=(%.2f %.2f)",
                                  M_total_count, receiver.player_->unum(),
                                  receive_point.x, receive_point.y );
                    debug_paint_failed_pass( M_total_count, receive_point, M_pass_logger );
                    #endif
                    continue;
                }
            }

            if (receive_point.x < M_first_point.x
                    && receive_point.dist2(our_goal) < OUR_GOAL_DIST_THR2) {
                #ifdef DEBUG_PASS
                M_total_count ++;
                dlog.addText( M_pass_logger,
                              "%d: xxx (lead) unum=%d our goal is near pos=(%.2f %.2f)",
                              M_total_count, receiver.player_->unum(),
                              receive_point.x, receive_point.y );
                debug_paint_failed_pass( M_total_count, receive_point, M_pass_logger );
                #endif
                continue;
            }

            if (wm.gameMode().type() == GameMode::GoalKick_
                    && receive_point.x < SP.ourPenaltyAreaLineX() + 1.0
                    && receive_point.absY() < SP.penaltyAreaHalfWidth() + 1.0) {
                #ifdef DEBUG_PASS
                M_total_count ++;
                dlog.addText( M_pass_logger,
                              "%d: xxx (lead) unum=%d, goal_kick",
                              M_total_count, receiver.player_->unum() );
                debug_paint_failed_pass( M_total_count, receive_point, M_pass_logger );
                #endif
                return;
            }

            const double ball_move_dist = M_first_point.dist(receive_point);

            if (ball_move_dist < MIN_LEADING_PASS_DIST
                    || MAX_LEADING_PASS_DIST < ball_move_dist) {
                #ifdef DEBUG_PASS
                M_total_count ++;
                dlog.addText( M_pass_logger,
                              "%d: xxx (lead) unum=%d overBallMoveDist=%.3f minDist=%.3f maxDist=%.3f",
                              M_total_count, receiver.player_->unum(),
                              ball_move_dist,
                              MIN_LEADING_PASS_DIST, MAX_LEADING_PASS_DIST );
                debug_paint_failed_pass( M_total_count, receive_point, M_pass_logger );
                #endif
                continue;
            }

            {
                int nearest_receiver_unum = getNearestReceiverUnum(
                            receive_point);
                if (nearest_receiver_unum != receiver.player_->unum()) {
                    #ifdef DEBUG_PASS
                    M_total_count ++;
                    dlog.addText( M_pass_logger,
                                  "%d: xxx (lead) unum=%d otherReceiver=%d pos=(%.2f %.2f)",
                                  M_total_count, receiver.player_->unum(),
                                  nearest_receiver_unum,
                                  receive_point.x, receive_point.y );
                    debug_paint_failed_pass( M_total_count, receive_point, M_pass_logger );
                    #endif
                    break;
                }
            }
            bool used_penalty = true;
            if(!wm.self().isKickable())
                used_penalty = false;
            const int receiver_step = predictReceiverReachStep(receiver,
                                                               receive_point, used_penalty) + (used_penalty?move_dist_penalty_step:0);
            const AngleDeg ball_move_angle =
                    (receive_point - M_first_point).th();

            const int min_ball_step = SP.ballMoveStep(SP.ballSpeedMax(),
                                                      ball_move_dist);

            const int start_step = std::max(
                        std::max(MIN_RECEIVE_STEP, min_ball_step), receiver_step);

            #ifdef CREATE_SEVERAL_CANDIDATES_ON_SAME_POINT
            const int max_step = std::max( MAX_RECEIVE_STEP, start_step + 3 );
            #else
            const int max_step = start_step + 3;
            #endif

            #ifdef DEBUG_PASS
            dlog.addText( Logger::PASS,
                          "| ballMove=%.3f moveAngle=%.1f",
                          ball_move_dist, ball_move_angle.degree() );
            dlog.addText( Logger::PASS,
                          "| stepRange=[%d, %d] receiverStep=%d(penalty=%d)",
                          start_step, max_step, receiver_step, move_dist_penalty_step );
            #endif
            bool high_dist = false;
            if(d>6)
                high_dist = true;
            createPassCommon(wm, receiver, receive_point, start_step, max_step,
                             min_ball_speed, max_ball_speed, min_receive_ball_speed,
                             max_receive_ball_speed, ball_move_dist, ball_move_angle,receiver_step,
                             "strictLead",high_dist);
            #ifdef DEBUG_PASS
            dlog.addText( M_pass_logger,
                          "--------------------------------------");
            #endif
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void StrictCheckPassGenerator::createThroughPass(const WorldModel & wm,
                                                 const Receiver & receiver) {
    static const int MIN_RECEIVE_STEP = 6;
    #ifdef CREATE_SEVERAL_CANDIDATES_ON_SAME_POINT
    static const int MAX_RECEIVE_STEP = 35;
    #endif

    static const double MIN_THROUGH_PASS_DIST = 5.0;
    static const double MAX_THROUGH_PASS_DIST = 0.9
            * inertia_final_distance(ServerParam::i().ballSpeedMax(),
                                     ServerParam::i().ballDecay());
    static const double MAX_RECEIVE_BALL_SPEED = ServerParam::i().ballSpeedMax()
            * std::pow(ServerParam::i().ballDecay(), MIN_RECEIVE_STEP);

    static const int ANGLE_DIVS = 15;//14
    static const double MIN_ANGLE = -70.0;
    static const double MAX_ANGLE = +70.0;
    static const double ANGLE_STEP = (MAX_ANGLE - MIN_ANGLE) / ANGLE_DIVS;

    double MIN_MOVE_DIST = 6.0;
    double MAX_MOVE_DIST = 30.0 + 0.001;
    static const double MOVE_DIST_STEP = 2.0;//2

    if(receiver.player_!= nullptr && receiver.player_->unum() > 0)
        if(receiver.player_->seenStaminaCount() < 15 && receiver.player_->seenStamina() < 3500){
         return;
        }
    const ServerParam & SP = ServerParam::i();
    const PlayerType * ptype = receiver.player_->playerTypePtr();
    const AngleDeg receiver_vel_angle = receiver.vel_.th();

    const double min_receive_x = std::min(
                std::min(std::max(10.0, M_first_point.x + 10.0),
                         wm.offsideLineX() - 10.0),
                SP.theirPenaltyAreaLineX() - 5.0);

    if(FieldAnalyzer::i().isFRA(wm)){
        if(M_first_point.x < -30)
            return;
        if(receiver.pos_.x < -15)
            return;
        if(M_first_point.x < receiver.pos_.x - 30)
            return;
    }else{
        if (receiver.pos_.x < min_receive_x - MAX_MOVE_DIST
                || receiver.pos_.x < 1.0) {
            #ifdef DEBUG_PASS
            dlog.addText( M_pass_logger,
                          "%d: xxx (through) unum=%d too back.",
                          M_total_count, receiver.player_->unum() );
            #endif
            return;
        }
    }


    //
    // initialize ball speed range
    //

    const double max_ball_speed = (
                wm.gameMode().type() == GameMode::PlayOn ? SP.ballSpeedMax() :
                                                           wm.self().isKickable() ?
                                                               wm.self().kickRate() * SP.maxPower() :
                                                               SP.kickPowerRate() * SP.maxPower());
    const double min_ball_speed = 1.4; //SP.defaultPlayerSpeedMax();

    const double max_receive_ball_speed = std::min(MAX_RECEIVE_BALL_SPEED,
                                                   ptype->kickableArea()
                                                   + (SP.maxDashPower() * ptype->dashPowerRate()
                                                      * ptype->effortMax()) * 1.5);
    const double min_receive_ball_speed = 0.001;

    //
    // check communication
    //

    bool pass_requested = false;
    AngleDeg requested_move_angle = 0.0;
    if (wm.audioMemory().passRequestTime().cycle() > wm.time().cycle() - 10) // Magic Number
    {
        for (std::vector<AudioMemory::PassRequest>::const_iterator it =
             wm.audioMemory().passRequest().begin();
             it != wm.audioMemory().passRequest().end(); ++it) {
            if (it->sender_ == receiver.player_->unum()) {
                pass_requested = true;
                requested_move_angle = (it->pos_ - receiver.inertia_pos_).th();
                #ifdef DEBUG_PASS
                dlog.addText( M_pass_logger,
                              "%d: (through) receiver=%d pass requested",
                              M_total_count, receiver.player_->unum() );
                #endif
                break;
            }
        }
    }

    //
    // angle loop
    //
    for (int a = 0; a <= ANGLE_DIVS; ++a) {
        if ( !SamplePlayer::canProcessMore() )
             return;
        const AngleDeg angle = MIN_ANGLE + (ANGLE_STEP * a);
        const Vector2D unit_rvec = Vector2D::from_polar(1.0, angle);

        //
        // distance loop
        //
        for (double move_dist = MIN_MOVE_DIST; move_dist < MAX_MOVE_DIST;
             move_dist += MOVE_DIST_STEP) {
            ++M_total_count;

            const Vector2D receive_point = receiver.inertia_pos_
                    + unit_rvec * move_dist;

            #ifdef DEBUG_PASS
            dlog.addText( M_pass_logger,
                          "--------------------------------");
            dlog.addText( M_pass_logger,
                          "(through) receiver=%d receivePoint=(%.1f %.1f)",
                          receiver.player_->unum(),
                          receive_point.x, receive_point.y );
            #endif

            if (receive_point.x < min_receive_x) {
                #ifdef DEBUG_PASS
                dlog.addText( M_pass_logger,
                              "%d: xxx (through) unum=%d tooSmallX pos=(%.2f %.2f)",
                              M_total_count, receiver.player_->unum(),
                              receive_point.x, receive_point.y );
                debug_paint_failed_pass( M_total_count, receive_point, M_pass_logger );
                #endif
                continue;
            }

            if (receive_point.x > SP.pitchHalfLength() - 1.5
                    || receive_point.absY() > SP.pitchHalfWidth() - 1.5) {
                #ifdef DEBUG_PASS
                dlog.addText( M_pass_logger,
                              "%d: xxx (through) unum=%d outOfBounds pos=(%.2f %.2f)",
                              M_total_count, receiver.player_->unum(),
                              receive_point.x, receive_point.y );
                debug_paint_failed_pass( M_total_count, receive_point, M_pass_logger );
                #endif
                break;
            }

            const double ball_move_dist = M_first_point.dist(receive_point);

            if (ball_move_dist < MIN_THROUGH_PASS_DIST
                    || MAX_THROUGH_PASS_DIST < ball_move_dist) {
                #ifdef DEBUG_PASS
                dlog.addText( M_pass_logger,
                              "%d: xxx (through) unum=%d overBallMoveDist=%.3f minDist=%.3f maxDist=%.3f",
                              M_total_count, receiver.player_->unum(),
                              ball_move_dist,
                              MIN_THROUGH_PASS_DIST, MAX_THROUGH_PASS_DIST );
                debug_paint_failed_pass( M_total_count, receive_point, M_pass_logger );
                #endif
                continue;
            }

            {
                int nearest_receiver_unum = getNearestReceiverUnum(
                            receive_point);
                if (nearest_receiver_unum != receiver.player_->unum()) {
                    #ifdef DEBUG_PASS
                    dlog.addText( M_pass_logger,
                                  "%d: xxx (through) unum=%d otherReceiver=%d pos=(%.2f %.2f)",
                                  M_total_count, receiver.player_->unum(),
                                  nearest_receiver_unum,
                                  receive_point.x, receive_point.y );
                    debug_paint_failed_pass( M_total_count, receive_point, M_pass_logger);
                    #endif
                    break;
                }
            }

            const int receiver_step = predictReceiverReachStep(receiver,
                                                               receive_point, false);
            const AngleDeg ball_move_angle =
                    (receive_point - M_first_point).th();

            int start_step = receiver_step;
            if (pass_requested && (requested_move_angle - angle).abs() < 20.0) {
                #ifdef DEBUG_PASS
                dlog.addText( M_pass_logger,
                              "%d: matched with requested pass. angle=%.1f",
                              M_total_count, angle.degree() );
                #endif
            }
            // if ( receive_point.x > wm.offsideLineX() + 5.0
            //      || ball_move_angle.abs() < 15.0 )
            else if (receiver.speed_ > 0.2
                     && (receiver_vel_angle - angle).abs() < 15.0) {
                #ifdef DEBUG_PASS
                dlog.addText( M_pass_logger,
                              "%d: matched with receiver velocity. angle=%.1f",
                              M_total_count, angle.degree() );
                #endif
            } else {
                #ifdef DEBUG_PASS
                dlog.addText( M_pass_logger,
                              "%d: receiver step. one step penalty",
                              M_total_count );
                #endif
                start_step += 1;
                if ((receive_point.x > SP.pitchHalfLength() - 5.0
                     || receive_point.absY() > SP.pitchHalfWidth() - 5.0)
                        && ball_move_angle.abs() > 30.0 && start_step >= 10) {
                    start_step += 1;
                }
            }

            const int min_ball_step = SP.ballMoveStep(SP.ballSpeedMax(),
                                                      ball_move_dist);

            start_step = std::max(std::max(MIN_RECEIVE_STEP, min_ball_step),
                                  start_step);

            #ifdef CREATE_SEVERAL_CANDIDATES_ON_SAME_POINT
            const int max_step = std::max( MAX_RECEIVE_STEP, start_step + 3 );
            #else
            const int max_step = start_step + 3;
            #endif

            #ifdef DEBUG_PASS
            dlog.addText( M_pass_logger,
                          "(through) receiver=%d"
                          " ballPos=(%.1f %.1f) receivePos=(%.1f %.1f)",
                          receiver.player_->unum(),
                          M_first_point.x, M_first_point.y,
                          receive_point.x, receive_point.y );
            dlog.addText( M_pass_logger,
                          "| ballMove=%.3f moveAngle=%.1f",
                          ball_move_dist, ball_move_angle.degree() );
            dlog.addText( M_pass_logger,
                          "| stepRange=[%d, %d] receiverMove=%.3f receiverStep=%d",
                          start_step, max_step,
                          receiver.inertia_pos_.dist( receive_point ), receiver_step );
            #endif

            createPassCommon(wm, receiver, receive_point, start_step, max_step,
                             min_ball_speed, max_ball_speed, min_receive_ball_speed,
                             max_receive_ball_speed, ball_move_dist, ball_move_angle,receiver_step,
                             "strictThrough");
            #ifdef DEBUG_PASS
            dlog.addText( M_pass_logger,
                          "--------------------------------------");
            #endif
        }

    }

}

/*-------------------------------------------------------------------*/
/*!

 */
void StrictCheckPassGenerator::createPassCommon(const WorldModel & wm,
                                                const Receiver & receiver, const Vector2D & receive_point,
                                                const int min_step, const int max_step,
                                                const double & min_first_ball_speed,
                                                const double & max_first_ball_speed,
                                                const double & min_receive_ball_speed,
                                                const double & max_receive_ball_speed, const double & ball_move_dist,
                                                const AngleDeg & ball_move_angle, int receiver_step, const char * description,bool high_dist) {
    const ServerParam & SP = ServerParam::i();

    int success_count = 0;
    #ifdef DEBUG_PASS
    std::vector< int > success_counts;
    success_counts.reserve( max_step - min_step + 1 );
    std::vector< int > failed_counts;
    failed_counts.reserve( max_step - min_step + 1 );
    #endif
    int max_pass_number = 1;
    bool should_find_one_kick = false;
    if(wm.opponentsFromSelf().size() > 0)
        if(wm.opponentsFromSelf().front()->distFromSelf() < 2.5){
            max_pass_number = 2;
            should_find_one_kick = true;
        }
    if (!Setting::i()->mChainAction->mTryFindOneKickPassOppClose)
        should_find_one_kick = false;
    if (Setting::i()->mChainAction->mSlowPass)
        max_pass_number = 3;
    int pass_number = 0;
    bool safe_with_pos_count = true;
    int danger = 0;
    for (int step = min_step; step <= max_step; ++step) {
        ++M_total_count;

        double first_ball_speed = calc_first_term_geom_series(ball_move_dist,
                                                              SP.ballDecay(), step);
        #ifdef DEBUG_PASS
        dlog.addText( M_pass_logger,
                      "|  %d: type=%c unum=%d recvPos=(%.2f %.2f) step=%d ballMoveDist=%.2f speed=%.3f",
                      M_total_count, M_pass_type,
                      receiver.player_->unum(),
                      receive_point.x, receive_point.y,
                      step,
                      ball_move_dist,
                      first_ball_speed );
        #endif
        danger = 0;
        safe_with_pos_count = true;
        if (receiver.player_->posCount() > 5){
            #ifdef DEBUG_PASS
            dlog.addText(M_pass_logger, "|  pass is danger for receiver pos count > 5");
            #endif
            danger += ((receiver.player_->posCount() - 5)/5 + 1);
        }

        if (first_ball_speed < min_first_ball_speed) {
            #ifdef DEBUG_PASS
            dlog.addText( M_pass_logger,
                          "|  %d: xxx type=%c unum=%d (%.1f %.1f) step=%d firstSpeed=%.3f < min=%.3f",
                          M_total_count, M_pass_type,
                          receiver.player_->unum(),
                          receive_point.x, receive_point.y,
                          step,
                          first_ball_speed, min_first_ball_speed );
            failed_counts.push_back(M_total_count);
            #endif
            break;
        }

        if (max_first_ball_speed < first_ball_speed) {
            #ifdef DEBUG_PASS
            dlog.addText( M_pass_logger,
                          "|  %d: xxx type=%c unum=%d (%.1f %.1f) step=%d firstSpeed=%.3f > max=%.3f",
                          M_total_count, M_pass_type,
                          receiver.player_->unum(),
                          receive_point.x, receive_point.y,
                          step,
                          first_ball_speed, max_first_ball_speed );
            failed_counts.push_back(M_total_count);
            #endif
            continue;
        }

        double receive_ball_speed = first_ball_speed
                * std::pow(SP.ballDecay(), step);
        if (receive_ball_speed < min_receive_ball_speed) {
            #ifdef DEBUG_PASS
            dlog.addText( M_pass_logger,
                          "|  %d: xxx type=%c unum=%d (%.1f %.1f) step=%d recvSpeed=%.3f < min=%.3f",
                          M_total_count, M_pass_type,
                          receiver.player_->unum(),
                          receive_point.x, receive_point.y,
                          step,
                          receive_ball_speed, min_receive_ball_speed );
            failed_counts.push_back(M_total_count);
            #endif
            break;
        }

        if (max_receive_ball_speed < receive_ball_speed) {
            #ifdef DEBUG_PASS
            dlog.addText( M_pass_logger,
                          "|  %d: xxx type=%c unum=%d (%.1f %.1f) step=%d recvSpeed=%.3f > max=%.3f",
                          M_total_count, M_pass_type,
                          receiver.player_->unum(),
                          receive_point.x, receive_point.y,
                          step,
                          receive_ball_speed, max_receive_ball_speed );
            failed_counts.push_back(M_total_count);
            #endif
            continue;
        }

        int kick_count = FieldAnalyzer::predict_kick_count(wm, M_passer,
                                                           first_ball_speed, ball_move_angle);
        if (!Setting::i()->mChainAction->mSlowPass)
            if(kick_count == 1)
                max_pass_number = 1;
        int opp_near_cycle = 100;
        if(wm.opponentsFromSelf().size() > 0){
            if(wm.opponentsFromSelf().front()->distFromSelf() < 4){
                const AbstractPlayerObject * opp_near_me = wm.theirPlayer(wm.opponentsFromSelf().front()->unum());
                if(opp_near_me != NULL && opp_near_me->unum() > 0){
                    int dc,tc,vc;
                    opp_near_cycle = opp_near_me->cycles_to_cut_ball(wm,M_first_point,4,true,dc,tc,vc,opp_near_me->pos() + opp_near_me->vel(),opp_near_me->vel(),opp_near_me->body().degree());
                    if(kick_count > opp_near_cycle){
                        #ifdef DEBUG_PASS
                        dlog.addText(M_pass_logger,"|  cont for kick count and opp near kick:%d oppcycle:%d", kick_count, opp_near_cycle);
                        failed_counts.push_back(M_total_count);
                        #endif
                        continue;
                    }
                }
            }
        }
        int opp_cycle_to_first_ball = wm.interceptTable()->opponentReachCycle();
        if(opp_cycle_to_first_ball<=kick_count
                && M_pass_type != 'T'){
            #ifdef DEBUG_PASS
            dlog.addText(M_pass_logger, "|  pass is danger for kick count and opp near");
            #endif
            danger += (kick_count - opp_cycle_to_first_ball + 1);
        }
        const AbstractPlayerObject * opponent =
                static_cast<const AbstractPlayerObject *>(0);
        int tm_dif_cycle = step - receiver_step;
        int opp_dif_cycle = 5;

        int o_step = predictOpponentsReachStep(wm, M_first_point,
                                               first_ball_speed, ball_move_angle, receive_point,
                                               step + 1/*(kick_count - 1) + 5*/, &opponent, opp_dif_cycle,safe_with_pos_count);

        bool failed = false;
        if (M_pass_type == 'T') {
            if (o_step <= step) {
                #ifdef DEBUG_PASS
                dlog.addText( M_pass_logger,
                              "|  %d: ThroughPass failed???",
                              M_total_count );
                #endif
                failed = true;
            }

            if (receive_point.x > 30.0 && step >= 15
                    && (!opponent || !opponent->goalie()) && o_step >= step) // Magic Number
            {
                AngleDeg receive_move_angle =
                        (receive_point - receiver.pos_).th();
                if ((receiver.player_->body() - receive_move_angle).abs()
                        < 15.0) {
                    #ifdef DEBUG_PASS
                    dlog.addText( M_pass_logger,
                                  "|  %d: ********** ThroughPass reset failed flag",
                                  M_total_count );
                    #endif
                    failed = false;
                }
            }
        } else {
            if (o_step <= step/* + (kick_count - 1)*/) {
                failed = true;
            }
        }

        if (failed) {
            // opponent can reach the ball faster than the receiver.
            // then, break the loop, because ball speed is decreasing in the loop.
            #ifdef DEBUG_PASS
            dlog.addText( M_pass_logger,
                          "|  %d: xxx type=%c unum=%d (%.1f %.1f) step=%d >= opp[%d]Step=%d,"
                          " firstSpeed=%.3f recvSpeed=%.3f nKick=%d",
                          M_total_count, M_pass_type,
                          receiver.player_->unum(),
                          receive_point.x, receive_point.y,
                          step,
                          ( opponent ? opponent->unum() : 0 ), o_step,
                          first_ball_speed, receive_ball_speed,
                          kick_count );
            failed_counts.push_back(M_total_count);
            #endif
            break;
        }

        if(high_dist){
            danger++;
            #ifdef DEBUG_PASS
            dlog.addText(M_pass_logger, "|  pass is danger for high dist");
            #endif
        }
        //		if(wm.dirCount((receive_point - M_first_point).th()) > 1)
        //			danger++;
        CooperativeAction::Ptr pass(
                    new Pass(M_passer->unum(), receiver.player_->unum(),
                             receive_point, first_ball_speed, step + kick_count,
                             kick_count, FieldAnalyzer::to_be_final_action(wm),
                             description,safe_with_pos_count,tm_dif_cycle,opp_dif_cycle,danger));
        pass->setIndex(M_total_count);

        switch (M_pass_type) {
        case 'D':
            M_direct_size += 1;
            break;
        case 'L':
            M_leading_size += 1;
            break;
        case 'T':
            M_through_size += 1;
        default:
            break;
        }
        // if ( M_pass_type == 'L'
        //      && success_count > 0 )
        // {
        //     M_courses.pop_back();
        // }

        M_courses.push_back(pass);

        Receivers_unum[receiver.player_->unum()] = true;
        #ifdef DEBUG_PASS
        dlog.addText( M_pass_logger,
                      "|  %d: ok type=%c unum=%d step=%d  opp[%d]Step=%d"
                      " nKick=%d ball=(%.1f %.1f) recv=(%.1f %.1f) "
                      " speed=%.3f->%.3f dir=%.1f, danger=%d, minOD=%d, minTD=%d , safe= %s",
                      M_total_count, M_pass_type,
                      receiver.player_->unum(),
                      step,
                      ( opponent ? opponent->unum() : 0 ),
                      o_step,
                      kick_count,
                      M_first_point.x, M_first_point.y,
                      receive_point.x, receive_point.y,
                      first_ball_speed,
                      receive_ball_speed,
                      ball_move_angle.degree(),
                      danger,
                      opp_dif_cycle,
                      tm_dif_cycle,
                      (safe_with_pos_count?std::string("safenoise").c_str():std::string("nonsafenoise").c_str()));
        success_counts.push_back( M_total_count );
        #endif

        pass_number++;
        if (should_find_one_kick && kick_count > 1)
        {

        }
        else{
            if(pass_number >= max_pass_number){
                dlog.addText( M_pass_logger,
                              "|  pass_number(=%d) >= max_pass_number(=%d) break...",
                              pass_number, max_pass_number );
                break;
            }
            int d = 3;
            if (Setting::i()->mChainAction->mSlowPass){
                if (kick_count > 1 && kick_count >= opp_near_cycle - 1){
                    d = 2;
                }
                else{
                    if (opp_dif_cycle <= 2)
                        break;
                    if (tm_dif_cycle > 1)
                        break;
                }
            }



            //#ifndef CREATE_SEVERAL_CANDIDATES_ON_SAME_POINT
            //        break;
            //#endif

            if (o_step <= step + d) {
                #ifdef DEBUG_PASS
                dlog.addText( M_pass_logger,
                              "|  o_step(=%d) <= step+3(=%d) break...",
                              o_step, step + d );
                #endif
                break;
            }

            if (min_step + 3 <= step) {
                #ifdef DEBUG_PASS
                dlog.addText( M_pass_logger,
                              "|  step=%d >= min_step+?(=%d) break...",
                              step, min_step + 3 );
                #endif
                break;
            }
        }


        if (M_passer->unum() != wm.self().unum()) {
            break;
        }

        ++success_count;
    }

    #ifdef DEBUG_PASS
    int ppp = 0;
    std::ostringstream ostr;
    if ( ! failed_counts.empty() )
    {
        ppp ++;
        dlog.addRect( M_pass_logger,
                      receive_point.x - 0.2, receive_point.y - 0.2,
                      0.2, 0.2,
                      255, 0, 0 );
        std::vector< int >::const_iterator it = failed_counts.begin();

        ostr << *it; ++it;
        for (; it != failed_counts.end(); ++it )
        {
            ostr << ',' << *it;
        }
    }
    if ( ! success_counts.empty() )
    {
        for(size_t i = M_courses.size() - success_counts.size(); i < M_courses.size(); i++){

            double pp = M_courses.size() - i - 1 + ppp;
            if(!M_courses[i]->safeWithNoise()){
                dlog.addRect( M_pass_logger,
                              receive_point.x - 0.2 + pp * 0.1, receive_point.y - 0.2 + pp * 0.1,
                              0.2, 0.2,
                              0,0,0 );
            }else if( M_courses[i]->danger() > 0){
                dlog.addRect( M_pass_logger,
                              receive_point.x - 0.2 + pp * 0.1, receive_point.y - 0.2 + pp * 0.1,
                              0.2, 0.2,
                              0,0,255 );
            }else{
                dlog.addRect( M_pass_logger,
                              receive_point.x - 0.2 + pp * 0.1, receive_point.y - 0.2 + pp * 0.1,
                              0.2, 0.2,
                              0, 250, 30 );
            }
        }
        std::vector< int >::const_iterator it = success_counts.begin();

        if (ppp == 1)
            ostr << ',';
        ostr << *it;
        ++it;
        for (; it != success_counts.end(); ++it )
        {
            ostr << ',' << *it;
        }

    }
    if ( !failed_counts.empty() || ! success_counts.empty()){
        dlog.addMessage( M_pass_logger,
                         receive_point,
                         ostr.str().c_str() );
    }
    #endif

}

/*-------------------------------------------------------------------*/
/*!

 */
int StrictCheckPassGenerator::getNearestReceiverUnum(const Vector2D & pos) {
    int unum = Unum_Unknown;
    double min_dist2 = std::numeric_limits<double>::max();

    for (ReceiverCont::iterator p = M_receiver_candidates.begin();
         p != M_receiver_candidates.end(); ++p) {
        double d2 = p->pos_.dist2(pos);
        if (d2 < min_dist2) {
            min_dist2 = d2;
            unum = p->player_->unum();
        }
    }

    return unum;
}

/*-------------------------------------------------------------------*/
/*!

 */
int StrictCheckPassGenerator::predictReceiverReachStep(
        const Receiver & receiver, const Vector2D & pos,
        const bool use_penalty) {
    const PlayerType * ptype = receiver.player_->playerTypePtr();
    double target_dist = receiver.inertia_pos_.dist(pos);

    int n_turn_original = FieldAnalyzer::predict_player_turn_cycle(ptype,
                                             receiver.player_->body(), receiver.speed_,
                                             target_dist, (pos - receiver.inertia_pos_).th(),
                                             ptype->kickableArea(), false);
    int body_count_effect = bound(0,receiver.player_->bodyCount(),2);
    int n_turn = 0;
    if (body_count_effect < n_turn_original)
        n_turn = n_turn_original;
    else
        n_turn = body_count_effect;

    int effective_pos_count_in_body_count = bound(0,body_count_effect - n_turn,2);
    int pos_count_effect = bound(0, receiver.player_->posCount() - effective_pos_count_in_body_count, receiver.player_->posCount());
    double dash_dist = target_dist;

    // if ( receiver.pos_.x < pos.x )
    // {
    //     dash_dist -= ptype->kickableArea() * 0.5;
    // }

    if (use_penalty) {
//        dash_dist += receiver.penalty_distance_;
       dash_dist += FieldAnalyzer::estimate_virtual_dash_distance(receiver.player_, pos_count_effect);

    }

    // if ( M_pass_type == 'T' )
    // {
    //     dash_dist -= ptype->kickableArea() * 0.5;
    // }

    if (M_pass_type == 'L') {
        // if ( pos.x > -20.0
        //      && dash_dist < ptype->kickableArea() * 1.5 )
        // {
        //     dash_dist -= ptype->kickableArea() * 0.5;
        // }

        // if ( pos.x < 30.0 )
        // {
        //     dash_dist += 0.3;
        // }

        dash_dist *= 1.05;

        AngleDeg dash_angle = (pos - receiver.pos_).th();

        if (dash_angle.abs() > 90.0 || receiver.player_->bodyCount() > 1
                || (dash_angle - receiver.player_->body()).abs() > 30.0) {
            n_turn += 1;
        }
    }

    int n_dash = ptype->cyclesToReachDistance(dash_dist);

    #ifdef DEBUG_PASS
    dlog.addText( M_pass_logger,
                  "|   receiver=%d receivePos=(%.1f %.1f) dist=%.2f dash=%.2f penalty=%.2f turn=%d dash=%d",
                  receiver.player_->unum(),
                  pos.x, pos.y,
                  target_dist, dash_dist, receiver.penalty_distance_,
                  n_turn, n_dash );
    #endif
    if(Strategy::i().my_team_tactic == Strategy::TeamTactic::AllDef)
        n_turn ++;

    return (n_turn == 0 ? n_turn + n_dash : n_turn + n_dash + 1); // 1 step penalty for observation delay.
    // if ( ! use_penalty )
    // {
    //     return n_turn + n_dash;
    // }
    // return n_turn + n_dash + 1; // 1 step penalty for observation delay.
}

/*-------------------------------------------------------------------*/
/*!

 */
int StrictCheckPassGenerator::predictOpponentsReachStep(const WorldModel & wm,
                                                        const Vector2D & first_ball_pos, const double & first_ball_speed,
                                                        const AngleDeg & ball_move_angle, const Vector2D & receive_point,
                                                        const int max_cycle, const AbstractPlayerObject ** opponent,int & opp_dif_cycle,bool & safe_with_pos_count) {
    const Vector2D first_ball_vel = Vector2D::polar2vector(first_ball_speed,
                                                           ball_move_angle);

    double bonus_dist = -10000.0;
    int min_step = 1000;
    const AbstractPlayerObject * fastest_opponent =
            static_cast<AbstractPlayerObject *>(0);

    if(M_opponents.size() == 0)
        return min_step;
    for (OpponentCont::const_iterator o = M_opponents.begin();
         o != M_opponents.end(); ++o) {
        int step = predictOpponentReachStep(wm, *o, first_ball_pos,
                                            first_ball_vel, ball_move_angle, receive_point,
                                            std::min(max_cycle, min_step),opp_dif_cycle,safe_with_pos_count);
        if (step < min_step
                || (step == min_step && o->bonus_distance_ > bonus_dist)) {
            bonus_dist = o->bonus_distance_;
            min_step = step;
            fastest_opponent = o->player_;
        }
    }

    #ifdef DEBUG_PASS_OPPONENT_REACH_STEP
    dlog.addText( M_pass_logger,
                  "|   opponent=%d(%.1f %.1f) step=%d",
                  fastest_opponent->unum(),
                  fastest_opponent->pos().x, fastest_opponent->pos().y,
                  min_step );
    #endif

    if (opponent) {
        *opponent = fastest_opponent;
    }
    return min_step;
}

/*-------------------------------------------------------------------*/
/*!

 */
int StrictCheckPassGenerator::predictOpponentReachStep(const WorldModel & wm,
                                                       const Opponent & opponent, const Vector2D & first_ball_pos,
                                                       const Vector2D & first_ball_vel, const AngleDeg & ball_move_angle,
                                                       const Vector2D & receive_point, const int max_cycle,
                                                       int & opp_dif_cycle,bool & safe_with_pos_count) {
    int our_score = ( wm.ourSide() == LEFT
                      ? wm.gameMode().scoreLeft()
                      : wm.gameMode().scoreRight() );
    int opp_score = ( wm.ourSide() == LEFT
                      ? wm.gameMode().scoreRight()
                      : wm.gameMode().scoreLeft() );
    static const Rect2D penalty_area(
                Vector2D(ServerParam::i().theirPenaltyAreaLineX(),
                         -ServerParam::i().penaltyAreaHalfWidth()),
                Size2D(ServerParam::i().penaltyAreaLength(),
                       ServerParam::i().penaltyAreaWidth()));
    static const double CONTROL_AREA_BUF = 0.15;

    const ServerParam & SP = ServerParam::i();

    const PlayerType * ptype = opponent.player_->playerTypePtr();
    int min_cycle = FieldAnalyzer::estimate_min_reach_cycle(opponent.pos_,
                                                            ptype->realSpeedMax(), first_ball_pos, ball_move_angle) - opponent.player_->posCount() - 1;
    if(min_cycle < 0)
        min_cycle = 0;
    #ifdef DEBUG_PASS_OPPONENT_REACH_STEP
    dlog.addText( M_pass_logger,
                  "|    opponent=%d(%.1f %.1f) posCount %d velCount %d bodyCount %d",
                  opponent.player_->unum(),
                  opponent.pos_.x, opponent.pos_.y,
                  opponent.player_->posCount(), opponent.player_->velCount(), opponent.player_->bodyCount());
    #endif

    if (min_cycle < 0) {
    #ifdef DEBUG_PASS_OPPONENT_REACH_STEP
        dlog.addText( M_pass_logger,
                      "|     never reach(1)",
                      opponent.player_->unum(),
                      opponent.player_->pos().x,
                      opponent.player_->pos().y );
    #endif
        return 1000;
    }

    Vector2D opp_pos = opponent.player_->pos();
    Vector2D opp_vel = opponent.player_->vel();
    opp_pos += Vector2D::polar2vector(ptype->playerSpeedMax() * (opp_vel.r() / 0.4),opp_vel.th());

    double dist_opp_pass = Line2D(first_ball_pos, receive_point).dist(opp_pos);
    if(dist_opp_pass < opponent.player_->playerTypePtr()->kickableArea() / 2)
        safe_with_pos_count = false;
    for (int cycle = std::max(1, min_cycle); cycle <= max_cycle; ++cycle) {
        const Vector2D ball_pos = inertia_n_step_point(first_ball_pos,
                                                       first_ball_vel, cycle, SP.ballDecay());

        //        if(cycle < 5){
        //            if(opp_pos.dist(ball_pos) < opponent.player_->playerTypePtr()->kickableArea() + 0.2)
        //                safe_with_pos_count = false;
        //        }
        bool check_tackle = false;
        if (M_pass_type == 'T' || receive_point.x > wm.offsideLineX() - 2)
            check_tackle = false;

        double control_area = (
                    opponent.player_->goalie() && penalty_area.contains(ball_pos) ?
                        SP.catchableArea() : ptype->kickableArea());

        Vector2D inertia_pos = opp_pos;
        const double target_dist = inertia_pos.dist(ball_pos);
        double dash_dist = target_dist;

        if (M_pass_type == 'T' && first_ball_vel.x > 2.0
                && (receive_point.x > wm.offsideLineX()
                    || receive_point.x > 30.0)) {
        } else {
            //			dash_dist -= opponent.bonus_distance_;
        }

        if (dash_dist - control_area - CONTROL_AREA_BUF < 0.001) {
            return cycle;
        }
        if(wm.getDistOpponentNearestToSelf(10,true) > 4){
            if (M_pass_type == 'T')
                control_area += 0.1;
            else
                control_area += 0.2;
        }
        {
            if (M_pass_type == 'T' && first_ball_vel.x > 2.0
                    && (receive_point.x > wm.offsideLineX()
                        || receive_point.x > 30.0)) {
                dash_dist -= control_area;
            } else {
                if (receive_point.x < 25.0) {
                    dash_dist -= control_area;
                } else {
                    dash_dist -= control_area;
                }
            }
        }

        if (dash_dist
                > ptype->realSpeedMax()
                * (cycle + std::min(opponent.player_->posCount(), 5))) {
            continue;
        }


        int turn_step;
        int dash_step;
        int view_step;

        int c_step = 0;
        double safe_dist_thr = 0;
        if(M_pass_type != 'T'){
            //            if(wm.opponentsFromBall().size() > 0
            //                    && wm.opponentsFromBall().front()->distFromBall() > (M_first_point.x < 25 ? 2 : 4 ))
            //                safe_dist_thr = 0.2;
        }
        if(Strategy::i().my_team_tactic == Strategy::TeamTactic::AllDef){
            safe_dist_thr += 0.7;
            if(wm.getDistOpponentNearestToBall(5,false) > 3)
                safe_dist_thr += 0.5;
        }

//        if(FieldAnalyzer::isMT(wm) || FieldAnalyzer::isIT(wm) || FieldAnalyzer::isFRA(wm) || FieldAnalyzer::isHFUT(wm) || FieldAnalyzer::isHelius(wm) || FieldAnalyzer::isOxsy(wm)
//                || FieldAnalyzer::isCYRUS(wm) || FieldAnalyzer::isKN2C(wm))
        if(M_pass_type != 'T')
            if(M_first_point.x < 30 || our_score < opp_score)
                safe_dist_thr += 0.2;

        c_step = opponent.player_->cycles_to_cut_ball_with_safe_thr_dist(wm,
                                                                         ball_pos,
                                                                         cycle,
                                                                         false,
                                                                         dash_step,
                                                                         turn_step,
                                                                         view_step,
                                                                         opp_pos,
                                                                         opp_vel,
                                                                         safe_dist_thr, true);
        int bonus_step = 0;
        if (opponent.player_->isTackling()) {
            bonus_step = -5; // Magic Number
        }
        c_step -= bonus_step;
        int body_count = opponent.player_->bodyCount();
        int pos_count = opponent.player_->posCount();
        int body_count_effect = body_count - pos_count;
        int pos_count_effect = std::min(8, pos_count);
        if (FieldAnalyzer::isYushan(wm))
            pos_count_effect = std::min(5, pos_count);
        if (body_count_effect > 2)
            body_count_effect = 2;


        double count_effect = 0.8;
        if (FieldAnalyzer::isHelius(wm)
            || FieldAnalyzer::isMT(wm)
            || FieldAnalyzer::isYushan(wm)
            || FieldAnalyzer::isOxsy(wm)
            || FieldAnalyzer::isFRA(wm))
            count_effect = 0.9;
        double c_count_effect = 0.5;
        if ((M_pass_type == 'T' && receive_point.x > wm.offsideLineX() - 5) || receive_point.x > wm.offsideLineX() + 10){
            if (FieldAnalyzer::isHelius(wm)
                || FieldAnalyzer::isMT(wm)
                || FieldAnalyzer::isOxsy(wm) || FieldAnalyzer::isYushan(wm)
                || FieldAnalyzer::isFRA(wm))
            {
                if(receive_point.x > wm.offsideLineX() + 10){
                    count_effect = 0.7;
                    c_count_effect = 0.4;
                }
                else{
                    count_effect = 0.8;
                    c_count_effect = 0.5;
                }
            }
            else
            {
                if(receive_point.x > wm.offsideLineX() + 10){
                    count_effect = 0.4;
                    c_count_effect = 0.2;
                }
                else{
                    count_effect = 0.5;
                    c_count_effect = 0.3;
                }
            }
        }
        if(!wm.self().isKickable()){
            body_count_effect = 0;
            pos_count_effect = 0;
            count_effect = 0;
            c_count_effect = 0;
        }
        int c_body_count_effect = body_count_effect;
        int c_pos_count_effect = pos_count_effect;
        c_body_count_effect = (int)((double)c_body_count_effect * c_count_effect);
        c_pos_count_effect = (int)((double)c_pos_count_effect * c_count_effect);
        if(body_count == 1){
            body_count_effect = 1;
        }else{
            body_count_effect = (int)((double)body_count_effect * count_effect);
        }
        if(pos_count_effect <= 2){
            pos_count_effect = pos_count;
        }else{
            pos_count_effect = (int)((double)pos_count_effect * count_effect);
        }

        int c_turn_step = turn_step - c_body_count_effect;
        int c_dash_step = dash_step - c_pos_count_effect;

        turn_step -= body_count_effect;
        dash_step -= pos_count_effect;
        if (turn_step < 0)
            turn_step = 0;
        if (dash_step < 0)
            dash_step = 0;

        int n_step = c_step;

//        if(FieldAnalyzer::isFRA(wm) || FieldAnalyzer::isMT(wm) || FieldAnalyzer::isHFUT(wm) || FieldAnalyzer::isYushan(wm) || FieldAnalyzer::isHelius(wm) || FieldAnalyzer::isCYRUS(wm)
//                ||Strategy::i().my_team_tactic == Strategy::TeamTactic::AllDef || FieldAnalyzer::isOxsy(wm) || FieldAnalyzer::isIT(wm) || our_score < opp_score
//                || FieldAnalyzer::isKN2C(wm)){
        view_step = 0;
//        }
        c_step = c_turn_step + c_dash_step + view_step;
        n_step = turn_step + dash_step + view_step;

        if ( c_step <= cycle){
            #ifdef DEBUG_PASS_OPPONENT_REACH_STEP
            dlog.addText( M_pass_logger,
                          "|      can reach in bs:%d(%.2f,%.2f) c_step=%d, t_step=%d, d_step=%d, "
                          "n_step=%d, bce=%d, pce=%d, scs=%d, odc:%d, pc:%d, bc:%d",
                          cycle,ball_pos.x,ball_pos.y,c_step,turn_step,dash_step,n_step,
                          body_count_effect,pos_count_effect,view_step,opp_dif_cycle ,pos_count,body_count);
            #endif
            return cycle;
        }else{
            if(wm.self().isKickable()){
                opp_dif_cycle = std::min(opp_dif_cycle,n_step - cycle);

            }

            if(cycle < n_step){
                #ifdef DEBUG_PASS_OPPONENT_REACH_STEP
                dlog.addText( M_pass_logger,
                              "|      cant reach in bs:%d(%.2f,%.2f) c_step=%d, t_step=%d, d_step=%d, n_step=%d, bce=%d, pce=%d, scs=%d, odc:%d, pc:%d, bc:%d",cycle,ball_pos.x,ball_pos.y,c_step,turn_step,dash_step,n_step,body_count_effect,pos_count_effect,view_step,opp_dif_cycle ,pos_count,body_count);
                #endif
            }else{
                #ifdef DEBUG_PASS_OPPONENT_REACH_STEP
                dlog.addText( M_pass_logger,
                              "|      cant reach in bs:%d(%.2f,%.2f) with out noise c_step=%d, t_step=%d, d_step=%d, n_step=%d, bce=%d, pce=%d, scs=%d, odc:%d, pc:%d, bc:%d",cycle,ball_pos.x,ball_pos.y,c_step,turn_step,dash_step,n_step,body_count_effect,pos_count_effect,view_step,opp_dif_cycle,pos_count,body_count );
                #endif
                if(wm.self().isKickable())
                    safe_with_pos_count = false;
            }
        }


        //        else if(cycle < max_cycle - 4){
        //#ifdef DEBUG_PASS_OPPONENT_REACH_STEP
        //            dlog.addText( M_pass_logger,
        //                          "__ cant reach in bs:%d(%.2f,%.2f) with out noise c_step=%d, t_step=%d, d_step=%d, n_step=%d, bce=%d, pce=%d, scs=%d, odc:%d, pc:%d, bc:%d",cycle,ball_pos.x,ball_pos.y,c_step,turn_step,dash_step,n_step,body_count_effect,pos_count_effect,should_see_step,opp_dif_cycle,pos_count,body_count );
        //#endif
        //            safe_with_pos_count = false;
        //        }else{
        //#ifdef DEBUG_PASS_OPPONENT_REACH_STEP
        //            dlog.addText( M_pass_logger,
        //                          "__ cant reach in bs:%d(%.2f,%.2f) with out noise afterM c_step=%d, t_step=%d, d_step=%d, n_step=%d, bce=%d, pce=%d, scs=%d, odc:%d, pc:%d, bc:%d",cycle,ball_pos.x,ball_pos.y,c_step,turn_step,dash_step,n_step,body_count_effect,pos_count_effect,should_see_step,opp_dif_cycle,pos_count,body_count );
        //#endif
        //        }
    }

    #ifdef DEBUG_PASS_OPPONENT_REACH_STEP
    dlog.addText( M_pass_logger,
                  "|      never reach(2) %s",(safe_with_pos_count?"safe":"nosafe") );
    #endif

    return 1000;
}


std::vector<SimplePredictPlayer> StrictCheckPassGenerator::OppPredictGenerator(const WorldModel &wm, int opp_unum, Vector2D target)
{

    Vector2D ball_pos = wm.ball().pos();

    std::vector<SimplePredictPlayer> ret;
    const AbstractPlayerObject * opp = wm.theirPlayer(opp_unum);
    if(opp == nullptr || opp->unum() < 1)
        return ret;


    Vector2D opp_pos = opp->pos();
    Vector2D opp_vel = (opp->vel().isValid()?opp->vel():Vector2D(0,0));
    AngleDeg opp_body = opp->body();

    SimplePredictPlayer tmp(opp_pos, opp_vel, opp_body, 1);
    ret.push_back(tmp);
    if(opp->pos().dist(ball_pos) < 10){
        if( ((ball_pos - opp_pos).th() - opp_body).abs() < 10){
            double speed = opp_vel.r() * 0.4;
            speed = std::max(speed + 0.6, opp->playerTypePtr()->realSpeedMax());
            SimplePredictPlayer tmp(opp_pos + Vector2D::polar2vector(speed, opp_body), Vector2D::polar2vector(speed, opp_body), opp_body,1);
            ret.push_back(tmp);
        }else{
            SimplePredictPlayer tmp(opp_pos + opp_vel, opp_vel * 0.4, (ball_pos - opp_pos).th(),1);
            ret.push_back(tmp);
        }
        if(target.isValid()){
            SimplePredictPlayer tmp(opp_pos + opp_vel, opp_vel * 0.4, (target - ball_pos).th() + 90,1);
            ret.push_back(tmp);
        }
        if(target.isValid()){
            SimplePredictPlayer tmp(opp_pos + opp_vel, opp_vel * 0.4, (target - ball_pos).th() - 90,1);
            ret.push_back(tmp);
        }
    }else{
        double speed = opp_vel.r() * 0.4;
        speed = std::max(speed + 0.6, opp->playerTypePtr()->realSpeedMax());
        SimplePredictPlayer tmp(opp_pos + Vector2D::polar2vector(speed, opp_body), Vector2D::polar2vector(speed, opp_body), opp_body,1);
        ret.push_back(tmp);
    }
    return ret;

    //poses.push_back(opp_pos);
    //veles.push_back(opp_vel);
    //bodys.push_back(opp_body);
    //if(opp->pos().dist(ball_pos) < 10){
    //    if( ((ball_pos - opp_pos).th() - opp_body).abs() < 10){
    //        double speed = opp_vel.r() * 0.4;
    //        speed = std::max(speed + 0.6, opp->playerTypePtr()->realSpeedMax());
    //        poses.push_back(opp_pos + Vector2D::polar2vector(speed, opp_body));
    //        veles.push_back(Vector2D::polar2vector(speed, opp_body));
    //        bodys.push_back(opp_body);
    //    }else{
    //        poses.push_back(opp_pos + opp_vel);
    //        veles.push_back(opp_vel * 0.4);
    //        bodys.push_back((ball_pos - opp_pos).th());
    //    }
    //}else{
    //    double speed = opp_vel.r() * 0.4;
    //    speed = std::max(speed + 0.6, opp->playerTypePtr()->realSpeedMax());
    //    poses.push_back(opp_pos + Vector2D::polar2vector(speed, opp_body));
    //    veles.push_back(Vector2D::polar2vector(speed, opp_body));
    //    bodys.push_back(opp_body);
    //}

}
