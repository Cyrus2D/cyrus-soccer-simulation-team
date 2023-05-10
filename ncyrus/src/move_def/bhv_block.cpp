/*
 * bhv_block.cpp
 *
 *  Created on: Jun 25, 2017
 *      Author: nader
 */
#include "bhv_block.h"
#include "../strategy.h"
#include "bhv_basic_tackle.h"
#include "bhv_mark_decision_greedy.h"
#include "../bhv_basic_move.h"
#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/body_turn_to_point.h>
#include <rcsc/action/body_intercept.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/player/world_model.h>
#include "../setting.h"
#include <rcsc/common/logger.h>
#include "../debugs.h"

//tackle block
bool bhv_block::do_tackle_block(PlayerAgent *agent) {
    const WorldModel &wm = agent->world();
    Vector2D my_inertia = wm.self().pos() + wm.self().vel();
    Vector2D ballPos = wm.ball().pos();
    Vector2D opponentPos = wm.ball().inertiaPoint(wm.interceptTable()->opponentReachCycle());
    const PlayerObject *fastestOpp = wm.interceptTable()->fastestOpponent();
    double dribleDir = get_dribble_angle(wm, opponentPos);
    if (my_inertia.dist(opponentPos) > 3.0) {
        return false;
    }
    if (fastestOpp->vel().r() > 0.15) {
        return false;
    }
    bool turnNeed = false;
    AngleDeg idealDir = (opponentPos - my_inertia).dir();
    if ((idealDir - wm.self().body()).abs() > 30.0) {
        turnNeed = true;
    }
    AngleDeg myDir = wm.self().body();
    if (turnNeed) {
        double effectiveTurnMoment = wm.self().playerType().effectiveTurn((idealDir - wm.self().body()).abs(),
                                                                          wm.self().vel().r());
        if (effectiveTurnMoment - (idealDir - wm.self().body()).abs() < -15.0) {
        }
        if (idealDir.isRightEqualOf(myDir)) {
            myDir -= effectiveTurnMoment;
        } else {
            myDir += effectiveTurnMoment;
        }
        const double tackleProb = calc_tackle_prob(opponentPos, my_inertia, myDir);
        if (tackleProb < 0.7) {
            return false;
        }
        if (((dribleDir + 180.0) - idealDir).abs() > 30.0 and my_inertia.dist(opponentPos) > 1.6) {
            return false;
        }
        return Body_TurnToPoint(opponentPos).execute(agent);
    }
    my_inertia += Vector2D::polar2vector(wm.self().playerType().dashDistanceTable()[0][1], myDir);
    const double tackleProb = calc_tackle_prob(opponentPos, my_inertia, myDir);
    if (tackleProb < 0.7) {
        #ifdef DEBUG_BLOCK
        dlog.addText(Logger::BLOCK, "low tackle prob! %.2f", tackleProb);
        #endif
        return false;
    }
    if (((dribleDir + 180.0) - idealDir).abs() > 30.0 and my_inertia.dist(opponentPos) > 2.0) {
        #ifdef DEBUG_BLOCK
        dlog.addText(Logger::BLOCK, "not safe to dash!");
        #endif
        return false;
    }
    #ifdef DEBUG_BLOCK
    dlog.addText(Logger::BLOCK, "decided to go for tackle! Dash => tp = %.2f", tackleProb);
    #endif
    Bhv_BasicMove::set_def_neck_with_ball(agent, opponentPos, wm.interceptTable()->fastestOpponent(), wm.self().unum());
    return agent->doDash(wm.self().getSafetyDashPower(100.0));
}

double bhv_block::calc_tackle_prob(Vector2D ballPos, Vector2D selfPos, AngleDeg selfBody) {
    const Vector2D player2ball = (ballPos - selfPos).rotatedVector(-selfBody);
    double tackle_dist;
    if (player2ball.x > 0.0) {
        tackle_dist = ServerParam::i().tackleDist();
    } else {
        tackle_dist = ServerParam::i().tackleBackDist();
    }
    double tackle_fail_prob = 1.0;
    double tackleProb = 0.0;
    if (tackle_dist > 1.0e-5) {
        tackle_fail_prob = (std::pow(player2ball.absX() / tackle_dist,
                                     ServerParam::i().tackleExponent())
                            + std::pow(player2ball.absY() / ServerParam::i().tackleWidth(),
                                       ServerParam::i().tackleExponent()));
    }
    if (tackle_fail_prob < 1.0) {
        tackleProb = 1.0 - tackle_fail_prob;
    }
    return tackleProb;
}

//tackle block end
double min_dist_tm(const WorldModel &wm, Vector2D pos) {
    double min_dist = 1000;
    for (int i = 1; i <= 11; i++) {
        if (i == wm.self().unum())
            continue;
        const AbstractPlayerObject *opp = wm.ourPlayer(i);
        if (opp != NULL && opp->unum() > 0) {
            if (opp->pos().dist(pos) < min_dist)
                min_dist = opp->pos().dist(pos);
        }
    }
    return min_dist;
}

double min_dist_opp(const WorldModel &wm, Vector2D pos) {
    double min_dist = 1000;
    for (int i = 1; i <= 11; i++) {
        const AbstractPlayerObject *opp = wm.theirPlayer(i);
        if (opp != NULL && opp->unum() > 0) {
            if (opp->pos().dist(pos) < min_dist)
                min_dist = opp->pos().dist(pos);
        }
    }
    return min_dist;
}

double min_dist_opp_unum(const WorldModel &wm, Vector2D pos) {
    double min_dist = 1000;
    int min_unum = 0;
    for (int i = 1; i <= 11; i++) {
        const AbstractPlayerObject *opp = wm.theirPlayer(i);
        if (opp != NULL && opp->unum() > 0) {
            if (opp->pos().dist(pos) < min_dist) {
                min_dist = opp->pos().dist(pos);
                min_unum = i;
            }
        }
    }
    return min_dist;
}

rcsc::Vector2D bhv_block::get_block_center_pos(const WorldModel &wm) {
    const int opp_min = wm.interceptTable()->opponentReachCycle();
    Vector2D opp_trap_pos = wm.ball().inertiaPoint(opp_min);
    const PlayerObject *opponent = wm.interceptTable()->fastestOpponent();
    if (opponent
        && opponent->bodyCount() <= 1
        && opponent->isKickable()
        && -44.0 < opp_trap_pos.x
        && opp_trap_pos.x < -25.0) {
        const Segment2D goal_segment(Vector2D(-ServerParam::i().pitchHalfLength(),
                                              -ServerParam::i().goalHalfWidth()),
                                     Vector2D(-ServerParam::i().pitchHalfLength(),
                                              +ServerParam::i().goalHalfWidth()));
        const Segment2D opponent_move(opp_trap_pos,
                                      opp_trap_pos + Vector2D::from_polar(500.0, opponent->body()));
        Vector2D intersection = goal_segment.intersection(opponent_move, false);
        if (intersection.isValid()) {
            return intersection;
        }
    }
    //
    // searth the best point
    //
    Vector2D center_pos(-44.0, 0.0);
    if (opp_trap_pos.x < -38.0
        && opp_trap_pos.absY() < 7.0) {
        center_pos.x = -52.5;
        // 2009-06-17
        center_pos.y = -2.0 * sign(wm.self().pos().y);
    } else if (opp_trap_pos.x < -38.0
               && opp_trap_pos.absY() < 9.0) {
        center_pos.x = std::min(-49.0, opp_trap_pos.x - 0.2);
        //center_pos.y = opp_trap_pos.y * 0.6;
        Vector2D goal_pos(-ServerParam::i().pitchHalfLength(),
                          ServerParam::i().goalHalfWidth() * 0.5);
        if (opp_trap_pos.y > 0.0) {
            goal_pos.y *= -1.0;
        }
        Line2D opp_line(opp_trap_pos, goal_pos);
        center_pos.y = opp_line.getY(center_pos.x);
        if (center_pos.y == Line2D::ERROR_VALUE) {
            center_pos.y = opp_trap_pos.y * 0.6;
        }
    } else if (opp_trap_pos.x < -38.0
               && opp_trap_pos.absY() < 12.0) {
        //center_pos.x = -50.0;
        center_pos.x = std::min(-46.5, opp_trap_pos.x - 0.2);
        //center_pos.y = 2.5 * sign( opp_trap_pos.y );
        //center_pos.y = 6.5 * sign( opp_trap_pos.y );
        //center_pos.y = opp_trap_pos.y * 0.8;
        center_pos.y = 0.0;
    } else if (opp_trap_pos.x < -30.0
               && 2.0 < opp_trap_pos.absY()
               && opp_trap_pos.absY() < 8.0) {
        center_pos.x = -50.0;
        center_pos.y = opp_trap_pos.y * 0.9;
    } else if (opp_trap_pos.absY() > 25.0) {
        center_pos.x = -44.0;
        if (opp_trap_pos.x < -36.0) {
            center_pos.y = 5.0 * sign(opp_trap_pos.y);
        } else {
            center_pos.y = 20.0 * sign(opp_trap_pos.y);
        }
    } else if (opp_trap_pos.absY() > 20.0) {
        center_pos.x = -44.0;
        if (opp_trap_pos.x > -18.0) {
            center_pos.y = 10.0 * sign(opp_trap_pos.y);
        } else {
            center_pos.y = 5.0 * sign(opp_trap_pos.y);
        }
    } else if (opp_trap_pos.absY() > 15.0) {
        center_pos.x = -44.0;
        if (opp_trap_pos.x > -18.0) {
            center_pos.y = 10.0 * sign(opp_trap_pos.y);
        } else {
            center_pos.y = 5.0 * sign(opp_trap_pos.y);
        }
    }
    return center_pos;
}

double bhv_block::get_dribble_angle(const WorldModel &wm, Vector2D start_dribble) {
    Vector2D best_target = Vector2D::INVALIDATED;
    if (start_dribble.x > -20)
        return (get_block_center_pos(wm) - start_dribble).th().degree();
    double best_eval = -100;
    for (double angle = -180; angle < 180; angle += 10) {
        Vector2D tmpball = start_dribble + Vector2D::polar2vector(10, angle);
        if (tmpball.absY() > 34 || tmpball.absX() > 52)
            continue;
        double eval = -tmpball.x + std::max(40 - tmpball.dist(Vector2D(-52, 0)), 0.0);
        if (eval > best_eval) {
            best_target = tmpball;
            best_eval = eval;
        }
    }
    return (best_target - start_dribble).th().degree();
}

vector<double> bhv_block::blocker_eval(const WorldModel &wm) {
    static int last_time_execute = 0;
    static vector<double> block_eval(12, 1000);
    if (wm.time().cycle() <= last_time_execute) {
        return block_eval;
    }
    last_time_execute = wm.time().cycle();
    block_eval = vector<double>(12, 1000);
    vector<double> block_cycle;
    vector<double> block_zarib;
    for (int i = 0; i <= 11; i++) {
        block_zarib.push_back(1.0);
        block_cycle.push_back(1000);
    }
    const int opp_min = wm.interceptTable()->opponentReachCycle();
    Vector2D start_drible = wm.ball().inertiaPoint(opp_min);
    if (start_drible.x < wm.ourDefensePlayerLineX() + 10)
        start_drible = start_drible + Vector2D::polar2vector(4, (Vector2D(-52, 0) - start_drible).th());
    double my_hdef_x = 100;
    for (int t = 2; t <= 11; t++) {
        if(Setting::i()->mStrategySetting->mIsGoalForward && t==2 )
            continue;
        Vector2D hpos = Strategy::i().getPosition(t);

        if (hpos.x < my_hdef_x)
            my_hdef_x = hpos.x;
    }
    if (start_drible.x > -20 && start_drible.x > my_hdef_x + 5) {
        for (int t = 2; t <= 11; t++) {
            const AbstractPlayerObject *tm = wm.ourPlayer(t);
            if (tm == NULL || tm->unum() < 1)
                continue;
            if (Strategy::i().tm_Line(t) != Strategy::i().PostLine::back)
                continue;
            if (min_dist_opp(wm, tm->pos()) < 15 &&
                min_dist_opp_unum(wm, tm->pos()) != wm.interceptTable()->fastestOpponent()->unum()) {
                block_zarib[t] *= 1.5;
            }
        }
    }
    if (start_drible.x > -20 && start_drible.x > my_hdef_x + 5) {
        for (int t = 2; t <= 11; t++) {
            const AbstractPlayerObject *tm = wm.ourPlayer(t);
            if (tm == NULL || tm->unum() < 1)
                continue;
            if (Strategy::i().tm_Line(t) != Strategy::i().PostLine::back)
                continue;
            if (min_dist_opp(wm, tm->pos()) < 5 &&
                min_dist_opp_unum(wm, tm->pos()) != wm.interceptTable()->fastestOpponent()->unum()) {
                block_zarib[t] *= 1.5;
            }
        }
    }
    for (int t = 2; t <= 11; t++) {
        const AbstractPlayerObject *tm = wm.ourPlayer(t);
        if (tm == NULL || tm->unum() < 1)
            continue;
        Vector2D tm_pos = tm->pos();
        int cycle_reach = tm->playerTypePtr()->cyclesToReachDistance(tm_pos.dist(start_drible));
        Vector2D target;
        bhv_block::block_cycle(wm, t, cycle_reach, target);
        if (tm->isTackling())
            cycle_reach += 9;
        cycle_reach *= block_zarib[t];
        block_cycle[t] = cycle_reach;
    }
    for (int t = 2; t <= 11; t++) {
        block_eval[t] = block_cycle[t];
    }
    return block_eval;
}

std::pair<vector<double>, vector<Vector2D> > bhv_block::blocker_eval_mark_decision(const WorldModel &wm) {
    static int last_time_execute = 0;
    static vector<double> block_eval(12, 1000);
    static vector<Vector2D> block_target(12, Vector2D::INVALIDATED);
    if (wm.time().cycle() <= last_time_execute) {
        return make_pair(block_eval, block_target);
    }
    last_time_execute = wm.time().cycle();
    block_eval = vector<double>(12, 1000);
    vector<double> block_cycle;
    vector<double> block_zarib;
    for (int i = 0; i <= 11; i++) {
        block_zarib.push_back(1.0);
        block_cycle.push_back(1000);
    }
    const int opp_min = wm.interceptTable()->opponentReachCycle();
    Vector2D start_drible = wm.ball().inertiaPoint(opp_min);
    if (start_drible.x < wm.ourDefensePlayerLineX() + 10)
        start_drible = start_drible + Vector2D::polar2vector(4, (Vector2D(-52, 0) - start_drible).th());
    double my_hdef_x = 100;
    for (int t = 2; t <= 11; t++) {
        if(Setting::i()->mStrategySetting->mIsGoalForward && t==2 )
            continue;
        Vector2D hpos = Strategy::i().getPosition(t);
        if (hpos.x < my_hdef_x)
            my_hdef_x = hpos.x;
    }
//    if (start_drible.x > -20 && start_drible.x > my_hdef_x + 5) {
//        for (int t = 2; t <= 11; t++) {
//            const AbstractPlayerObject *tm = wm.ourPlayer(t);
//            if (tm == NULL || tm->unum() < 1)
//                continue;
//            if (Strategy::i().tm_Line(t) != Strategy::i().PostLine::back)
//                continue;
//            if (min_dist_opp(wm, tm->pos()) < 15 &&
//                min_dist_opp_unum(wm, tm->pos()) != wm.interceptTable()->fastestOpponent()->unum()) {
//                block_zarib[t] *= 1.5;
//            }
//        }
//    }
//    if (start_drible.x > -20 && start_drible.x > my_hdef_x + 5) {
//        for (int t = 2; t <= 11; t++) {
//            const AbstractPlayerObject *tm = wm.ourPlayer(t);
//            if (tm == NULL || tm->unum() < 1)
//                continue;
//            if (Strategy::i().tm_Line(t) != Strategy::i().PostLine::back)
//                continue;
//            if (min_dist_opp(wm, tm->pos()) < 5 &&
//                min_dist_opp_unum(wm, tm->pos()) != wm.interceptTable()->fastestOpponent()->unum()) {
//                block_zarib[t] *= 1.5;
//            }
//        }
//    }
    for (int t = 2; t <= 11; t++) {
        const AbstractPlayerObject *tm = wm.ourPlayer(t);
        if (tm == NULL || tm->unum() < 1)
            continue;
        Vector2D tm_pos = tm->pos();
        int cycle_reach = tm->playerTypePtr()->cyclesToReachDistance(tm_pos.dist(start_drible));
	Vector2D target;
	bhv_block::block_cycle(wm, t, cycle_reach, target);
        if (tm->isTackling())
            cycle_reach += 9;
        if (Strategy::i().tm_Post(t) == Strategy::player_post::pp_cb){
            if (target.isValid() && target.absY() > 15){
                block_zarib[t] = Setting::i()->mDefenseMove->mBlockZ_CB_Next;
            }else if (target.isValid() && target.x > my_hdef_x + 10){
                block_zarib[t] = Setting::i()->mDefenseMove->mBlockZ_CB_Forward;
            }
        }
        if (Strategy::i().tm_Post(t) == Strategy::player_post::pp_rb || Strategy::i().tm_Post(t) == Strategy::player_post::pp_lb){
            if (target.isValid() && target.x > my_hdef_x + 10){
                block_zarib[t] = Setting::i()->mDefenseMove->mBlockZ_LB_RB_Forward;
            }
        }
        cycle_reach *= block_zarib[t];
        block_cycle[t] = cycle_reach;
        block_target[t] = target;
    }
    for (int t = 2; t <= 11; t++) {
        block_eval[t] = block_cycle[t];
    }
    return make_pair(block_eval, block_target);
}

int bhv_block::who_is_blocker(const WorldModel &wm, vector<int> marker) {
    vector<double> block_eval = blocker_eval(wm);
    vector<int> blockers;
    for (int i = 2; i <= 11; i++) {
        if (std::find(marker.begin(), marker.end(), i) == marker.end())
            blockers.push_back(i);
    }
    int min_cycle = 1000;
    int min_unum = 0;
    for (int t = 0; t < blockers.size(); t++) {
        if (block_eval[blockers[t]] < min_cycle) {
            min_cycle = block_eval[blockers[t]];
            min_unum = blockers[t];
        }
    }
    return min_unum;
}

void bhv_block::get_start_dribble(const WorldModel &wm, Vector2D &start_dribble, int &cycle) {

    Vector2D inertia_ball_pos = wm.ball().inertiaPoint(wm.interceptTable()->opponentReachCycle());
    int oppCycles = wm.interceptTable()->opponentReachCycle();
    vector<Vector2D> ball_cache = CyrusPlayerIntercept::createBallCache(wm);

    CyrusPlayerIntercept ourpredictor(wm, ball_cache);
    const PlayerObject *it = wm.interceptTable()->fastestOpponent();
    if (it != nullptr && (*it).unum() >= 2) {
        const PlayerType *player_type = (*it).playerTypePtr();
        vector<CyrusOppInterceptTable> pred = ourpredictor.predict(*it, *player_type, 1000);
        CyrusOppInterceptTable tmp = CyrusPlayerIntercept::getBestIntercept(wm, pred);
        oppCycles = tmp.cycle;
        inertia_ball_pos = tmp.current_position;
        if (oppCycles > 100 || !inertia_ball_pos.isValid()) {
            oppCycles = wm.interceptTable()->opponentReachCycle();
            inertia_ball_pos = wm.ball().inertiaPoint(oppCycles);
        }
    }
    start_dribble = inertia_ball_pos;
    #ifdef DEBUG_BLOCK
    dlog.addCircle(Logger::BLOCK, start_dribble, 0.5, 255,0,0,true);
    #endif
    cycle = oppCycles;
    return;

    Vector2D ball_pos = wm.ball().pos();
    Vector2D opp_pos = wm.interceptTable()->fastestOpponent()->pos();
    int tm_min = wm.interceptTable()->teammateReachCycle();
    int opp_min = wm.interceptTable()->opponentReachCycle();
    int self_min = wm.interceptTable()->selfReachCycle();
    tm_min = std::min(tm_min,self_min);

    // double ball2opp = wm.ball().calc_travel_step(ball_pos.dist(opp_pos), wm.ball().vel().r()); CYURS_LIB fixed but check
    double ball2opp = calc_length_geom_series( wm.ball().vel().r(),
                                    ball_pos.dist(opp_pos),
                                    ServerParam::i().ballDecay());

    start_dribble = wm.ball().inertiaPoint(opp_min);
    if(start_dribble.absY() > 15 && start_dribble.x > -20){
        if(ball2opp < tm_min && opp_min > 3){
            if(Line2D(ball_pos, wm.ball().vel().th()).dist(opp_pos) < 2){
                cycle = static_cast<int>(ball2opp);
                start_dribble = opp_pos;
                return;
            }
        }
    }
    cycle = opp_min;
}

void bhv_block::block_cycle(const WorldModel &wm, int unum, int &cycle, Vector2D &target, bool debug) {
    Vector2D start_drible;
    int start_drible_time;
    get_start_dribble(wm, start_drible, start_drible_time);
    double best_angle = get_dribble_angle(wm, start_drible);
    double dribble_speed = 0.8;
    Sector2D dribble_sector(start_drible, 0, 20, best_angle - 20, best_angle + 20);
    #ifdef DEBUG_BLOCK
    dlog.addLine(Logger::BLOCK, start_drible, start_drible + Vector2D::polar2vector(20, best_angle - 20));
    dlog.addLine(Logger::BLOCK, start_drible, start_drible + Vector2D::polar2vector(20, best_angle + 20));
    #endif
    bool is_tm_in_dribble_sector = false;
    for (int i = 2; i <= 11; i++){
        const AbstractPlayerObject * tm = wm.ourPlayer(i);
        if (tm == nullptr || tm->unum() != i)
            continue;
        if (dribble_sector.contains(tm->pos())){
            is_tm_in_dribble_sector = true;
            break;
        }
    }
    if (is_tm_in_dribble_sector){
        dribble_speed = 0.6;
    }
    Vector2D vel = Vector2D::polar2vector(dribble_speed, best_angle);
    if (wm.ourPlayer(unum) == NULL || wm.ourPlayer(unum)->unum() < 1) {
        target = Vector2D::INVALIDATED;
        cycle = 1000;
        return;
    }
    Vector2D ball = start_drible;
    const AbstractPlayerObject *tm = wm.ourPlayer(unum);
    for (int i = 1; i < 40; i++) {
        ball += vel;
        if (ball.absX() > 52 || ball.absY() > 34)
            break;
        int drible_step = i + start_drible_time;
        Vector2D tm_pos = tm->pos();
        Vector2D tm_vel = tm->vel();
        int dc,tc,vc;
        wm.ourPlayer(wm.self().unum())->cycles_to_cut_ball(wm, ball, drible_step, false, dc, tc, vc, tm_pos, tm_vel);
        int dash_step = dc + tc;
        if (dash_step <= drible_step) {
            target = ball;
            cycle = drible_step;
            #ifdef DEBUG_BLOCK
                dlog.addCircle(Logger::BLOCK, ball, 0.2, 0, 0, 255);
                char num[8];
                snprintf(num, 8, "%d", drible_step);
                char num2[8];
                snprintf(num2, 8, "%d", dash_step);
                dlog.addMessage(Logger::BLOCK, ball - Vector2D(0, 1), num);
                dlog.addMessage(Logger::BLOCK, ball - Vector2D(0, 2), num2);
            #endif
            return;
        } else {
            #ifdef DEBUG_BLOCK
                dlog.addCircle(Logger::BLOCK, ball, 0.2, 255, 0, 0);
                char num[8];
                snprintf(num, 8, "%d", drible_step);
                char num2[8];
                snprintf(num2, 8, "%d", dash_step);
                dlog.addMessage(Logger::BLOCK, ball - Vector2D(0, 1), num);
                dlog.addMessage(Logger::BLOCK, ball - Vector2D(0, 2), num2);
            #endif
        }
    }
    cycle = 1000;
    target = Vector2D::INVALIDATED;
}

bool bhv_block::do_block_pass(PlayerAgent *agent)
{
    const WorldModel &wm = agent->world();
    Vector2D start_drible;
    int start_drible_time;
    get_start_dribble(wm, start_drible, start_drible_time);

    if(start_drible.x > -25 || start_drible.absY() < 15)
    {
        #ifdef DEBUG_BLOCK
        dlog.addText(Logger::BLOCK, "block pass: return false: out of area");
        #endif
        return false;
    }
    bool opp_near_goal = false;
    vector<Vector2D> opps_pos;
    for(int o = 1; o <= 11; o++){
        const AbstractPlayerObject * opp = wm.theirPlayer(o);
        if(opp == nullptr
                || opp->unum() < 1)
            continue;
        if(opp->pos().dist(Vector2D(-52,0)) < 30
                || opp->unum() != wm.interceptTable()->fastestOpponent()->unum()){
            opps_pos.push_back(opp->pos());
            opp_near_goal = true;
        }
    }
    if(!opp_near_goal)
    {
        #ifdef DEBUG_BLOCK
        dlog.addText(Logger::BLOCK, "block pass: return false: opponent not found near goal");
        #endif
        return false;
    }

    bool tm_near_goal = false;
    for(int t = 1; t <= 11; t++){
        const AbstractPlayerObject * tm = wm.theirPlayer(t);
        if(tm == nullptr
                || tm->unum() < 1
                || tm->goalie()
                || t == wm.self().unum())
            continue;
        if(tm->pos().dist(Vector2D(-52,0)) < 20){
            tm_near_goal = true;
        }
    }
    if(tm_near_goal)
    {
        #ifdef DEBUG_BLOCK
        dlog.addText(Logger::BLOCK, "block pass: return false: teammate found near goal");
        #endif
        return false;
    }
    Vector2D pass_tar(-43.0, 0.0);
    AngleDeg pass_ang = (pass_tar - start_drible).th();
    Line2D pass_line(pass_tar, start_drible);
    AngleDeg sector_angle1 = pass_ang - 20;
    AngleDeg sector_angle2 = pass_ang + 20;
    #ifdef DEBUG_BLOCK
    dlog.addText(Logger::BLOCK, "sec1:%.1f sec2:%.1f", sector_angle1.degree(), sector_angle2.degree());
    #endif
    if(sector_angle1.degree() > sector_angle2.degree())
        swap(sector_angle1, sector_angle2);
    Sector2D pass_sector(start_drible, 0, start_drible.dist(pass_tar), sector_angle1, sector_angle2);
    #ifdef DEBUG_BLOCK
    dlog.addSector(Logger::BLOCK, pass_sector, 0,0,255);
    #endif
    if(pass_sector.contains(wm.self().pos()))
    {
        #ifdef DEBUG_BLOCK
        dlog.addText(Logger::BLOCK, "block pass: return false: I'm in pass sector");
        #endif
        return false;
    }
    if(start_drible.dist(wm.self().pos()) < 3)
    {
        #ifdef DEBUG_BLOCK
        dlog.addText(Logger::BLOCK, "block pass: return false: I'm near start drible");
        #endif
        return false;
    }
    if(wm.self().pos().x < start_drible.x)
    {
        #ifdef DEBUG_BLOCK
        dlog.addText(Logger::BLOCK, "block pass: return false: I'm back of start drible");
        #endif
        return false;
    }
    double block_pos_y = wm.self().pos().y;
    if(start_drible.y > 0){
        if(block_pos_y > start_drible.y){
            block_pos_y = start_drible.y - 2;
        }
    }else{
        if(block_pos_y < start_drible.y){
            block_pos_y = start_drible.y + 2;
        }
    }
    Vector2D block_pos(pass_line.getX(block_pos_y),block_pos_y);
    Body_GoToPoint(block_pos, 0.5, 100, 2, 1, false, 15).execute(agent);
    Bhv_BasicMove::set_def_neck_with_ball(agent, start_drible, wm.interceptTable()->fastestOpponent(), wm.self().unum());

    agent->debugClient().addMessage("block pass");
    agent->debugClient().setTarget(block_pos);
    #ifdef DEBUG_BLOCK
    dlog.addText(Logger::BLOCK, "block pass: execute: (%.1f, %.1f)", block_pos.x, block_pos.y);
    dlog.addCircle(Logger::BLOCK, block_pos,0.5,255,0,0,true);
    dlog.addText(Logger::ACTION, "PassBlock Executed");
    #endif
    return true;
}
bool bhv_block::execute(rcsc::PlayerAgent *agent) {
//    if(do_tackle_block(agent))
//        return true;
    const WorldModel &wm = agent->world();
    Vector2D self_pos = wm.self().pos();
    AngleDeg self_body = wm.self().body();
    double kickable_area = wm.self().playerType().kickableArea();
    if (wm.interceptTable()->fastestOpponent() == NULL || wm.interceptTable()->fastestOpponent()->unum() < 1)
        return false;
    if (wm.gameMode().type() != wm.gameMode().PlayOn)
        return false;
    Vector2D target;
    int cycle;
    if(Setting::i()->mDefenseMove->mUsePassBlock
            && do_block_pass(agent)){
        return true;
    }
    block_cycle(wm, wm.self().unum(), cycle, target, true);
    auto fastest_opp = wm.interceptTable()->fastestOpponent();
    bool go_to_opp = false;
    if (Setting::i()->mDefenseMove->mBlockGoToOppPos){
        if (target.isValid()){
            #ifdef DEBUG_BLOCK
            dlog.addText(Logger::BLOCK, "is valid k%d c%d d%.2f", fastest_opp->isKickable(),cycle,target.dist(fastest_opp->pos()));
            #endif
            if (fastest_opp->isKickable() && cycle <= 2 && target.dist(fastest_opp->pos()) < 1.2){
                // AngleDeg dir = AngleDeg::INVALIDATED; CLIB F0
                // AngleDeg dir = AngleDeg(numeric_limits<double>::max()); CLIB 1
                AngleDeg dir = AngleDeg();
                if (target.x > -10){
                    dir = -180.0;
                }
                else{
                    Vector2D last_tar = Vector2D::INVALIDATED;
                    if (target.x < -35.0){
                        if (target.absY() > 20.0){
                            if (target.x > -43){
                                last_tar = Vector2D(-47, target.y > 0 ? 9 : -9);
                            }
                            else{
                                last_tar = Vector2D(target.x, 0);
                            }
                        }
                        else if(target.absY() > 9.0){
                            if (target.x > -43){
                                last_tar = Vector2D(-47, target.y > 0 ? 6 : -6);
                            }
                            else{
                                last_tar = Vector2D(-52.5, target.y > 0 ? 3 : -3);
                            }
                        }
                        else {
                            last_tar = Vector2D(-52.5, target.y / 9.0 * 5.0);
                        }
                    }
                    else {
                        if (target.absY() < 20.0){
                            last_tar = Vector2D(-45.0, target.y / 20.0 * 13.0);
                        }
                        else{
                            last_tar = Vector2D(-45.0, (target.absY() - 20.0) / 14.0 * 7.0 - 3.5);
                            if (target.y < 0){
                                last_tar.y *= -1.0;
                            }
                        }
                    }
                    dir = (last_tar - target).th();
                    #ifdef DEBUG_BLOCK
                    dlog.addCircle(Logger::BLOCK, last_tar, 0.5, 255, 192, 203);
                    #endif
                }
                Vector2D opp_pos = fastest_opp->pos();
                Vector2D sec_center = opp_pos - Vector2D::polar2vector(1.0, dir);
                #ifdef DEBUG_BLOCK
                dlog.addLine(Logger::BLOCK, sec_center, sec_center + Vector2D::polar2vector(10, dir), 255, 192, 203);
                dlog.addLine(Logger::BLOCK, sec_center, sec_center + Vector2D::polar2vector(10, dir + 15.0), 255, 192, 203);
                dlog.addLine(Logger::BLOCK, sec_center, sec_center + Vector2D::polar2vector(10, dir - 15.0), 255, 192, 203);
                dlog.addCircle(Logger::BLOCK, sec_center, 0.5, 255, 192, 203);
                dlog.addCircle(Logger::BLOCK, Vector2D::polar2vector(1.0, dir), 0.5, 255, 192, 203);
                #endif

                Sector2D tar_sec = Sector2D(sec_center, 1.0, 5.0, dir - 15.0, dir + 15.0);
                if (tar_sec.contains(self_pos)){
                    target = opp_pos;
                    go_to_opp = true;
                    #ifdef DEBUG_BLOCK
                    agent->debugClient().addMessage("change block to opp");
                    #endif
                }
            }
        }
    }

    if (target.isValid() && target.absY() < 35 && target.absX() < 53) {
//        agent->doPointto(target.x, target.y);
        agent->debugClient().addMessage("blockk");
        bool move = false;
        double dash_power = 100;
        if(Strategy::i().self_Line() == Strategy::PostLine::forward && target.x > 10)
            dash_power = Strategy::i().get_normal_dash_power(wm);
        double body_diff_degree = ((target - wm.self().pos()).th() - wm.self().body()).abs();
        Vector2D self_inertia = wm.self().inertiaPoint(1);
        Line2D direct_dash_line(self_inertia, self_body);
        if (!move){
            double max_dist_line = kickable_area - 0.1;
            if (go_to_opp){
                max_dist_line = 0.2;
            }
            if (body_diff_degree < 90 && direct_dash_line.dist(target) < max_dist_line && self_pos.dist(target) > 1){
                agent->doDash(dash_power, 0);
                agent->debugClient().addMessage("direct dash");
                move = true;
            }
        }
        if (!move){
            if ( cycle == 1 ){
                AngleDeg dash_dir = ((target - self_inertia).th() - wm.self().body());
                agent->doDash(100, dash_dir);
                agent->debugClient().addMessage("close omni dash");
                move = true;
            }
            else if (cycle <= 5){
                AngleDeg dash_dir = ((target - wm.self().inertiaPoint(cycle)).th() - wm.self().body());
                if (body_diff_degree < 15){
                    agent->doDash(dash_power, dash_dir);
                    agent->debugClient().addMessage("far omni dash");
                    move = true;
                }
            }
        }
        if (!move){
            if (Body_GoToPoint(
                    target,
                    0.5,
                    dash_power,
                    -1.0,
                    cycle = 0,
                    true,
                    20.0,
                    1.0,
                    false ).execute(agent)){
                agent->debugClient().addMessage("BGP A");
            }else{
                Body_TurnToBall().execute(agent);
                agent->debugClient().addMessage("Turn A");
            }
        }

        Bhv_BasicMove::set_def_neck_with_ball(agent, target, wm.interceptTable()->fastestOpponent(), wm.self().unum());
//        Vector2D start_drible;
//        int start_drible_time;
//        get_start_dribble(wm, start_drible, start_drible_time);
        /*agent->setIntention(new IntentionBlock(wm.interceptTable()->fastestOpponent()->unum(),
                                               wm.interceptTable()->fastestOpponent()->pos(),
                                               target,
                                               wm.ball().pos(),
                                               start_drible,
                                               cycle - 2,
                                               wm.time()));*/
        return true;
    }else{
//        std::cout<<"B9"<<std::endl;
        return Bhv_BasicMove().intercept_plan(agent, true);
    }
    return false;
}

