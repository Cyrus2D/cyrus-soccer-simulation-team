/*
 * bhv_mark_execute.cpp
 *
 *  Created on: Apr 12, 2017
 *      Author: nader
 */
#include "bhv_mark_execute.h"
#include "bhv_mark_decision_greedy.h"
#include "bhv_mark_intention.h"
#include "bhv_block.h"
#include "../strategy.h"
#include "bhv_mark_execute.h"
#include "bhv_basic_tackle.h"
#include "bhv_basic_move.h"
#include <rcsc/geom.h>
#include "move_def/mark_position_finder.h"
#include <rcsc/action/body_turn_to_ball.h>
#include <rcsc/action/neck_turn_to_ball.h>
#include <ctime>
#include "../debugs.h"
#include "mark_position_finder.h"
#include "../setting.h"

using namespace std;
using namespace rcsc;


bool bhv_mark_execute::execute(PlayerAgent *agent) {
    const WorldModel &wm = agent->world();
    if (do_tackle(agent)) {
        return true;
    }
//    if (defenseGoBack(agent)){
//        return true;
//    }
    if (wm.interceptTable()->fastestOpponent() == NULL || wm.interceptTable()->fastestOpponent()->unum() < 1) {
        #ifdef DEBUG_MARK_EXECUTE
        dlog.addText(Logger::MARK, "Cant Mark fastest opp not found");
        #endif
        return false;
    }

    int mark_unum = 0;
    bool blocked = false;
    int opp_cycle = wm.interceptTable()->opponentReachCycle();
    Vector2D ball_inertia = wm.ball().inertiaPoint(opp_cycle);
    MarkType mark_type;

    vector<MarkType> global_how_mark;
    vector<size_t> global_tm_mark_target;
    vector<size_t> global_opp_marker;
    for (int i = 0; i <= 11; i++) {
        global_tm_mark_target.push_back(0);
        global_opp_marker.push_back(0);
        global_how_mark.push_back(MarkType::NoType);
    }

    BhvMarkDecisionGreedy().getMarkTargets(agent, mark_type, mark_unum, blocked,
                                           global_how_mark, global_tm_mark_target, global_opp_marker);

    if (mark_unum > 0 || mark_type == MarkType::Goal_keep) {
        if (run_mark(agent, mark_unum, mark_type)) {
            agent->debugClient().addMessage("RunMark");
            return true;
        }
    }
    else {
        #ifdef DEBUG_MARK_EXECUTE
        dlog.addText(Logger::MARK, "no one to mark and %d is blocker", blocked);
        #endif
        bool do_free_blocker_block = true;
        if (BhvMarkDecisionGreedy::markDecision(wm) == MarkDec::MidMark){
            if (Strategy::i().self_Line() == Strategy::PostLine::back){
                if (ball_inertia.x > Setting::i()->mDefenseMove->mStartMidMark + 10.0){
                    auto block_eval_target = bhv_block::blocker_eval_mark_decision(wm);
                    vector<double> block_eval = block_eval_target.first;
                    vector<Vector2D> block_target = block_eval_target.second;
                    double tm_hpos_def_line = 0;
                    for (int i = 2; i <= 11; i++) {
                        if(Setting::i()->mStrategySetting->mIsGoalForward && i==2 )
                            continue;
                        double hpos_x = Strategy::i().getPosition(i).x;

                        if (hpos_x < tm_hpos_def_line)
                            tm_hpos_def_line = hpos_x;
                    }
                    if (block_target[wm.self().unum()].x > tm_hpos_def_line + Setting::i()->mDefenseMove->mBackBlockMaxXToDefHPosX){
                        do_free_blocker_block = false;
                    }
                }
            }
        }
        if (do_free_blocker_block){
            if (!blocked &&
                !(ball_inertia.x > -20 && Strategy::i().self_Line() == Strategy::PostLine::back)) {
                if (bhv_block::who_is_blocker(wm) == wm.self().unum()) {
                    if (bhv_block().execute(agent)) {
                        agent->debugClient().addMessage("BlockNoDec");
                        return true;
                    }
                }
            }
        }
        if (defenseBeInBack(agent))
            return true;
    }
    if (Setting::i()->mDefenseMove->mGoToDefendX){
        vector<double> th_mark_xs;
        if (Strategy::i().self_Line() == Strategy::PostLine::back){
            for (int t = 1; t <= 11; t++){
                if (t == wm.self().unum())
                    continue;
                if (Strategy::i().tm_Line(t) != Strategy::PostLine::back)
                    continue;
                if (global_how_mark[t] == MarkType::ThMark && global_tm_mark_target[t] > 0){
                    auto target = MarkPositionFinder::getThMarkTarget(t, global_tm_mark_target[t], wm, false);
                    th_mark_xs.push_back(target.pos.x);
                }
            }
            if (!th_mark_xs.empty()){
                double min_th_mark_x = 100;
                double max_th_mark_x = -100;
                double sum_th_mark_x = 0;
                for (auto & x: th_mark_xs){
                    if (x < min_th_mark_x)
                        min_th_mark_x = x;
                    if (x < max_th_mark_x)
                        max_th_mark_x = x;
                    sum_th_mark_x += x;
                }
                double avg_th_mark_x = sum_th_mark_x / static_cast<double>(th_mark_xs.size());
                Vector2D target = Strategy::i().getPosition(wm.self().unum());
                target.x = avg_th_mark_x;
                agent->debugClient().addCircle(target, 0.5);
                agent->debugClient().setTarget(target);
                agent->debugClient().addMessage("go to defend line");
                double dist_thr = wm.ball().distFromSelf() * 0.1;
                if (dist_thr < 1.0) dist_thr = 1.0;
                double dash_power = Strategy::get_normal_dash_power(wm);
                if (!Body_GoToPoint(target, dist_thr, dash_power
                ).execute(agent)) {
                    Body_TurnToBall().execute(agent);
                }

                if (wm.kickableOpponent()
                    && wm.ball().distFromSelf() < 18.0) {
                    agent->setNeckAction(new Neck_TurnToBall());
                } else {
                    agent->setNeckAction(new Neck_TurnToBallOrScan(0));
                }
                return true;
            }
        }
    }
    return false;
}

bool bhv_mark_execute::defenseGoBack(PlayerAgent *agent){
    const WorldModel & wm = agent->world();
    Vector2D ball_inertia = wm.ball().inertiaPoint(wm.interceptTable()->opponentReachCycle());
    double def_line_x = wm.ourDefensePlayerLineX();
    if (ball_inertia.absY() > 20 || ball_inertia.x > -5)
        return false;
    if(wm.self().unum() > 5)
        return false;
    if (ball_inertia.x < def_line_x - 3){
        vector<Vector2D> targets;
        if (ball_inertia.x > -30){
            targets.push_back(Vector2D(-36, 3));
            targets.push_back(Vector2D(-36, -8));
            targets.push_back(Vector2D(-36, 8));
            targets.push_back(Vector2D(-36, -3));
        }else{
            targets.push_back(Vector2D(-49, 2));
            targets.push_back(Vector2D(-49, -6));
            targets.push_back(Vector2D(-49, 6));
            targets.push_back(Vector2D(-49, -2));
        }
        Vector2D target = targets[wm.self().unum() - 2];
        Vector2D self_pos = wm.self().pos();
        if (target.dist(self_pos) < 1.0)
            return false;
        agent->debugClient().addMessage("goBack");
        if(!Body_GoToPoint(target, 1.0, 100, 1.3, 1, false, 20).execute(agent)){
            Body_TurnToPoint(target).execute(agent);
        }
        agent->setNeckAction(new Neck_TurnToBall());
        return true;
    }
    return false;
}
bool bhv_mark_execute::defenseBeInBack(PlayerAgent *agent){
    const WorldModel & wm = agent->world();
    MarkDec mark_dec = BhvMarkDecisionGreedy().markDecision(wm);
    if (mark_dec != MarkDec::MidMark)
        return false;

    double tm_pos_def_line = wm.ball().inertiaPoint(wm.interceptTable()->opponentReachCycle()).x;
    double tm_hpos_def_line = 0;
    for (int i = 2; i <= 11; i++) {
        if(Setting::i()->mStrategySetting->mIsGoalForward && i==2 )
            continue;
        const AbstractPlayerObject * tm = wm.ourPlayer(i);
        if (tm != nullptr && tm->unum() > 0){
            if (tm->pos().x < tm_pos_def_line)
                tm_pos_def_line = tm->pos().x;
        }
        double hpos_x = Strategy::i().getPosition(i).x;

        if (hpos_x < tm_hpos_def_line)
            tm_hpos_def_line = hpos_x;
    }
    Vector2D target_point = Strategy::i().getPosition(wm.self().unum());

    Vector2D ball_pos = wm.ball().inertiaPoint(wm.interceptTable()->opponentReachCycle());
//    if(Strategy::i().tm_Line(wm.self().unum()) == Strategy::PostLine::forward
//        ||Strategy::i().tm_Line(wm.self().unum()) == Strategy::PostLine::half)
//        if(ball_pos.x > tm_pos_def_line + 30 || ball_pos.x > -20)
//            return false;

//    if(new_ball.x > tm_pos_def_line){
//        Vector2D intersect = Line2D(Vector2D(tm_pos_def_line, 0), 90).intersection(Line2D(new_ball, Vector2D(-52,0)));
//        if (intersect.isValid()){
//            new_ball = (new_ball + intersect) / 2.0;
//            target_point = Strategy::i().getPositionWithBall(wm.self().unum(), new_ball, wm);
//        }
//    }
    #ifdef DEBUG_MARK_EXECUTE
    dlog.addText(Logger::TEAM,
                 __FILE__": defLineX %.1f defLineHX %.1f",
                 tm_pos_def_line, tm_hpos_def_line);
    #endif
    if (tm_pos_def_line < tm_hpos_def_line - 5){
        if(Strategy::i().tm_Line(wm.self().unum()) == Strategy::PostLine::back)
            target_point.x = tm_pos_def_line + 3.0;
        else if(Strategy::i().tm_Line(wm.self().unum()) == Strategy::PostLine::half)
            target_point.x = tm_pos_def_line + 10.0;
        else
            target_point.x = tm_pos_def_line + 20.0;
    }else{
        return false;
    }
    #ifdef DEBUG_MARK_EXECUTE
    dlog.addText(Logger::TEAM,
                 __FILE__": Be Back target=(%.1f %.1f)",
                 target_point.x, target_point.y);
    #endif
    target_point.x = min(target_point.x, Strategy::i().getPosition(wm.self().unum()).x);
    #ifdef DEBUG_MARK_EXECUTE
    dlog.addText(Logger::TEAM,
                 __FILE__": Be Back target=(%.1f %.1f)",
                 target_point.x, target_point.y);
    #endif
    double dist_thr = wm.ball().distFromSelf() * 0.1;
    if (dist_thr < 1.0) dist_thr = 1.0;
    #ifdef DEBUG_MARK_EXECUTE
    dlog.addText(Logger::TEAM,
                 __FILE__": Be Back new target=(%.1f %.1f) dist_thr=%.2f",
                 target_point.x, target_point.y,
                 dist_thr);
    #endif
    double dash_power = Strategy::get_normal_dash_power(wm);
    agent->debugClient().addMessage("be in back", dash_power);
    agent->debugClient().setTarget(target_point);
    agent->debugClient().addCircle(target_point, dist_thr);


    if (!Body_GoToPoint(target_point, 0.5, dash_power
    ).execute(agent)) {
        Body_TurnToBall().execute(agent);
    }

    if (wm.kickableOpponent()
        && wm.ball().distFromSelf() < 18.0) {
        agent->setNeckAction(new Neck_TurnToBall());
    } else {
        agent->setNeckAction(new Neck_TurnToBallOrScan(0));
    }
    return true;
}

bool bhv_mark_execute::do_tackle(PlayerAgent *agent) {
    const WorldModel &wm = agent->world();
    if (wm.self().pos().x < rcsc::ServerParam::i().theirPenaltyAreaLineX() - 5 &&
        Bhv_BasicTackle(0.9).execute(agent)) {
        agent->debugClient().addMessage("Tackle");
        return true;
    }
    if (Bhv_BasicTackle(0.8).execute(agent)) {
        agent->debugClient().addMessage("Tackle");
        return true;
    }
    if (wm.self().pos().x < rcsc::ServerParam::i().ourPenaltyAreaLineX() * 0.75 &&
        Bhv_BasicTackle(0.7).execute(agent)) {
        agent->debugClient().addMessage("Tackle");
        return true;
    }
    return false;
}

bool bhv_mark_execute::run_mark(PlayerAgent *agent, int mark_unum, MarkType marktype) {
    #ifdef DEBUG_MARK_EXECUTE
    dlog.addText(Logger::MARK, ">>>>run_mark for opp %d", mark_unum);
    #endif
    const WorldModel &wm = agent->world();
    const AbstractPlayerObject *opp = wm.theirPlayer(mark_unum);
    Target target;
    int opp_cycle = wm.interceptTable()->opponentReachCycle();
    Vector2D ball_inertia = wm.ball().inertiaPoint(opp_cycle);

    if (wm.theirPlayer(mark_unum) == NULL || wm.theirPlayer(mark_unum)->unum() < 1){
        #ifdef DEBUG_MARK_EXECUTE
        dlog.addText(Logger::MARK, ">>>>cancel mark, fastest opp is null", mark_unum);
        #endif
        return false;
    }

    int blocker = bhv_block::who_is_blocker(wm);
    bool want_block = false;
    bool back_to_def_flag = back_to_def(agent);

    double dist_thr = 1.0;
    string mark_type_str;
    target.pos = Strategy::i().getPosition(wm.self().unum());
    set_mark_target_thr(wm, opp, marktype, target, dist_thr);

    if (marktype == MarkType::Block) {
        want_block = true;
        #ifdef DEBUG_MARK_EXECUTE
        dlog.addText(Logger::MARK, ">>>> Block execute");
        #endif
        if (bhv_block().execute(agent)) {
            agent->debugClient().addMessage("block in mark");
            return true;
        }
        agent->debugClient().addMessage("cant block in mark");
    }
    #ifdef DEBUG_MARK_EXECUTE
    dlog.addCircle(Logger::MARK, target.pos, 1.0, 100, 0, 100);
    #endif
    agent->debugClient().addCircle(target.pos, 1.0);
    agent->debugClient().addMessage("mark %d %s %d", mark_unum,
                                    markTypeString(marktype).c_str());
    if (back_to_def_flag
        && Strategy::i().self_Line() == Strategy::PostLine::back
        && marktype != MarkType::ThMark) {
        Vector2D blockTarget;
        int blockCycle;
        bhv_block::block_cycle(wm, blocker, blockCycle, blockTarget, true);
        target.pos.x = std::min(blockTarget.x, target.pos.x);
        #ifdef DEBUG_MARK_EXECUTE
        dlog.addText(Logger::MARK, ">>>> Change target because of back_to_def, target:(0.1f, 0.1f)", target.pos.x,
                     target.pos.y);
        #endif
    }
    if (want_block) {
        #ifdef DEBUG_MARK_EXECUTE
        dlog.addText(Logger::MARK, ">>>> Want Block but cant!!");
        #endif
        return false;
    }

    agent->debugClient().addLine(wm.self().pos(), target.pos);
    #ifdef DEBUG_MARK_EXECUTE
    dlog.addText(Logger::MARK, "mark target (%.2f,%.2f)", target.pos.x, target.pos.y);
    #endif
    if(wm.gameMode().type() != GameMode::PlayOn){
        target.pos = change_position_set_play(wm, target.pos);
    }

    do_move_mark(agent, target, dist_thr, marktype, mark_unum);
    agent->debugClient().addMessage("domove(%.1f,%.1f)", target.pos.x, target.pos.y);
    Bhv_BasicMove::set_def_neck_with_ball(agent, target.pos, wm.theirPlayer(mark_unum), blocker);
    return true;
}

void bhv_mark_execute::set_mark_target_thr(const WorldModel & wm,
                                           const AbstractPlayerObject * opp,
                                           MarkType mark_type,
                                           Target & target,
                                           double & dist_thr){
    int self_unum = wm.self().unum();
    int opp_cycle = wm.interceptTable()->opponentReachCycle();
    Vector2D ball_inertia = wm.ball().inertiaPoint(opp_cycle);
    switch (mark_type) {
        case (MarkType::LeadProjectionMark): {
            target = MarkPositionFinder::getLeadProjectionMarkTarget(self_unum,
                                                                     opp->unum(), wm);
            double z_thr = std::max(1.0, ball_inertia.dist(target.pos) * 0.1);
            dist_thr = z_thr;
            if (ball_inertia.dist(target.pos) < 30 && opp->vel().r() > 0.1)
                dist_thr = 0.5;
            #ifdef DEBUG_MARK_EXECUTE
            dlog.addText(Logger::MARK, ">>>> LeadProj, target:(%.1f, %.1f) distthr=%.1f", target.pos.x, target.pos.y,
                         dist_thr);
            #endif
            break;
        }
        case MarkType::LeadNearMark: {
            target = MarkPositionFinder::getLeadNearMarkTarget(self_unum,
                                                               opp->unum(), wm);
            double z_thr = std::max(1.0, ball_inertia.dist(target.pos) * 0.1);
            dist_thr = 0.5 * z_thr;
            #ifdef DEBUG_MARK_EXECUTE
            dlog.addText(Logger::MARK, ">>>> LeadNear, target:(0.1f, 0.1f) distthr=%.1f", target.pos.x, target.pos.y,
                         dist_thr);
            #endif
            break;
        }
        case MarkType::ThMark: {
            target = MarkPositionFinder::getThMarkTarget(self_unum, opp->unum(), wm, true);
            dist_thr = 0.5;
            double z_thr = std::max(1.0, ball_inertia.dist(target.pos) * 0.1);
            dist_thr = 0.5 * z_thr;
            if (ball_inertia.x > 25 && dist_thr < 2.0)
                dist_thr = 2.0;
            #ifdef DEBUG_MARK_EXECUTE
            dlog.addText(Logger::MARK, ">>>> ThMark, target:(0.1f, 0.1f) distthr=%.1f", target.pos.x, target.pos.y,
                         dist_thr);
            #endif
            break;
        }
        case MarkType::DangerMark: {
            target = MarkPositionFinder::getDengerMarkTarget(self_unum, opp->unum(), wm);
            double z_thr = std::max(1.0, ball_inertia.dist(target.pos) * 0.1);
            dist_thr = 0.3 * z_thr;
            if (wm.gameMode().type() != GameMode::PlayOn){
                dist_thr *= 1.5;
            }
            #ifdef DEBUG_MARK_EXECUTE
            dlog.addText(Logger::MARK, ">>>> DangerMark, target:(0.1f, 0.1f) distthr=%.1f", target.pos.x, target.pos.y,
                         dist_thr);
            #endif
            break;
        }
        case MarkType::Goal_keep: {
            #ifdef DEBUG_MARK_EXECUTE
            dlog.addText(Logger::MARK, "keep goal in mark");
            #endif
            break;
        }
        default:
            break;
    }
    if (wm.gameMode().type() != GameMode::PlayOn){
        dist_thr = max(1.5, dist_thr);
    }
}
bool bhv_mark_execute::do_move_mark(PlayerAgent *agent, Target targ, double dist_thr, MarkType marktype, int opp_unum) {
    #ifdef DEBUG_MARK_EXECUTE
    dlog.addText(Logger::MARK, ">>>>do_move_mark");
    #endif
    agent->debugClient().addCircle(targ.pos, 0.5);
    agent->debugClient().addCircle(targ.pos, 0.3);
    agent->debugClient().addCircle(targ.pos, 0.1);
    const WorldModel &wm = agent->world();
    Vector2D target_pos = targ.pos;
    Vector2D self_pos = wm.self().pos();
    Vector2D opp_pos = wm.theirPlayer(opp_unum)->pos();

    Vector2D ball_pos = wm.ball().inertiaPoint(wm.interceptTable()->opponentReachCycle());
    Vector2D self_hpos = Strategy::i().getPosition(wm.self().unum());

    if (marktype != MarkType::ThMark)
        if (self_pos.dist(target_pos) < dist_thr && targ.th.degree() != 1000) {
            if (Body_TurnToAngle(targ.th).execute(agent)) {
                #ifdef DEBUG_MARK_EXECUTE
                dlog.addText(Logger::MARK, "turn in mark move %.2f",
                             targ.th.degree());
                #endif
                agent->debugClient().addMessage("mark:move:turn %.1f", targ.th.degree());
                return true;
            }
        }

    if (marktype == MarkType::ThMark) {
        #ifdef DEBUG_MARK_EXECUTE
        dlog.addText(Logger::MARK, ">>>>ThMark");
        #endif
        double dash_power = th_mark_power(agent, opp_pos, target_pos);
        th_mark_move(agent, targ, dash_power, dist_thr, opp_unum);
    }
    else if (marktype == MarkType::LeadProjectionMark || marktype == MarkType::LeadNearMark) {
        #ifdef DEBUG_MARK_EXECUTE
        dlog.addText(Logger::MARK, ">>>>is LeadProj or LeadNear mark");
        #endif
        double dash_power = lead_mark_power(agent, opp_pos, target_pos);
        lead_mark_move(agent, targ, dash_power, dist_thr, marktype, opp_pos);
    }
    else {
        #ifdef DEBUG_MARK_EXECUTE
        dlog.addText(Logger::MARK, ">>>>is other mark");
        #endif
        double dash_power = other_mark_power(agent, opp_pos, target_pos);
        other_mark_move(agent, targ, dash_power, dist_thr);
    }
    return true;
}

double bhv_mark_execute::th_mark_power(PlayerAgent * agent, Vector2D opp_pos, Vector2D target_pos){
    const WorldModel & wm = agent->world();
    int opp_min_cycle = wm.interceptTable()->opponentReachCycle();
    Vector2D ball_inertia = wm.ball().inertiaPoint(opp_min_cycle);
    Vector2D self_pos = wm.self().pos();
    double dash_power = Strategy::get_normal_dash_power(wm);
    double z = 1.0;
    if (wm.self().stamina() < 3500) {
        z = 0.5;
    }
    if (wm.self().stamina() < 4500) {
        z = 0.7;
    }
    if (abs(target_pos.x - self_pos.x) > 5 * z){
        dash_power = 100;
        agent->debugClient().addMessage("SA");
    }
    if (abs(target_pos.y - self_pos.y) > 5 * z){
        dash_power = 100;
        agent->debugClient().addMessage("SB");
    }
    if(ball_inertia.dist(opp_pos) < 20 * z && target_pos.dist(self_pos) > 2 && opp_pos.x - target_pos.x < 5 && target_pos.x < opp_pos.x){
        dash_power = 100;
        agent->debugClient().addMessage("SC");
    }
    if(opp_pos.x < target_pos.x && self_pos.x < target_pos.x - 2){
        dash_power = 100;
        agent->debugClient().addMessage("SD");
    }
    if(ball_inertia.dist(opp_pos) < 20 * z && opp_min_cycle <=2 && target_pos.x < opp_pos.x && opp_pos.x < target_pos.x + 7 * z){
        dash_power = 100;
        agent->debugClient().addMessage("SE");
    }
    if (wm.self().stamina() < 3000) {
        dash_power = Strategy::get_normal_dash_power(wm);
        agent->debugClient().addMessage("SF");
    }
    return dash_power;
}

void bhv_mark_execute::th_mark_move(PlayerAgent * agent, Target targ, double dash_power, double dist_thr, int opp_unum){
    const WorldModel & wm = agent->world();
    Vector2D self_pos = wm.self().pos();
    Vector2D target_pos = targ.pos;
    double body_dif = (targ.th - wm.self().body()).abs();
    int opp_min_cycle = wm.interceptTable()->opponentReachCycle();
    Vector2D ball_pos = wm.ball().inertiaPoint(wm.interceptTable()->opponentReachCycle());
    Vector2D self_hpos = Strategy::i().getPosition(wm.self().unum());
    Vector2D opp_pos = wm.theirPlayer(opp_unum)->pos();
    if (Setting::i()->mDefenseMove->mFixThMarkY){
        if (Strategy::i().self_Line() == Strategy::PostLine::back){
            if (abs(self_hpos.y - target_pos.y) > 5.0 && (ball_pos - opp_pos).th().abs() > 30.0) {
                target_pos.y = self_hpos.y;
                targ.pos.y = self_hpos.y;
                agent->debugClient().addCircle(targ.pos, 0.5);
                agent->debugClient().setTarget(targ.pos);
            }
        }
    }

    if (self_pos.dist(target_pos) < dist_thr && targ.th.degree() != 1000) {
        if (Body_TurnToAngle(targ.th).execute(agent)) {
            #ifdef DEBUG_MARK_EXECUTE
            dlog.addText(Logger::MARK, "turn in mark move %.2f",
                         targ.th.degree());
            #endif
            agent->debugClient().addMessage("mark:move:turn %.1f", targ.th.degree());
            return;
        }
    }

    if (self_pos.dist(target_pos) < 1) {
        if (body_dif < 20 && self_pos.dist(target_pos) < dist_thr / 2.0) {
            #ifdef DEBUG_MARK_EXECUTE
            dlog.addText(Logger::MARK, ">>>>do Dash A");
            #endif
            agent->debugClient().addMessage("mark:move:Adash (%.1f,%.1f) %.1f", target_pos.x, target_pos.y, dash_power);
            agent->doDash(dash_power, (target_pos - (wm.self().pos() + wm.self().vel())).th() - wm.self().body());
            return;
        }
    }
    else if (self_pos.dist(target_pos) < dist_thr + 2 && opp_min_cycle > 3) {
        #ifdef DEBUG_MARK_EXECUTE
        dlog.addText(Logger::MARK, ">>>>do Dash B");
        #endif
        if (body_dif < 20) {
            agent->debugClient().addMessage("mark:move:Bdash (%.1f,%.1f) %.1f", target_pos.x, target_pos.y, dash_power);
            agent->doDash(dash_power, (target_pos - (wm.self().pos() + wm.self().vel())).th() - wm.self().body());
            return;
        }
    }
    agent->debugClient().addMessage("mark:move:BGD (%.1f,%.1f) %.1f", target_pos.x, target_pos.y, dash_power);
    #ifdef DEBUG_MARK_EXECUTE
    dlog.addText(Logger::MARK, "Body Go To Point to (%.1f,%.1f) thr:%.1f power %.1f", target_pos.x, target_pos.y, dist_thr, dash_power);
    #endif
    if(Body_GoToPoint(target_pos, dist_thr, dash_power, 1.3, 1, false, 15).execute(agent)){
        #ifdef DEBUG_MARK_EXECUTE
        dlog.addText(Logger::MARK, "ran go to point");
        #endif
    }else{
        #ifdef DEBUG_MARK_EXECUTE
        dlog.addText(Logger::MARK, "did not run go to point");
        #endif
    }

}

double bhv_mark_execute::lead_mark_power(PlayerAgent * agent, Vector2D opp_pos, Vector2D target_pos){
    const WorldModel & wm = agent->world();
    Vector2D self_pos = wm.self().pos();
    int opp_min_cycle = wm.interceptTable()->opponentReachCycle();
    Vector2D ball_inertia = wm.ball().inertiaPoint(opp_min_cycle);
    double dash_power = Strategy::get_normal_dash_power(wm);
    if (opp_min_cycle < 3 && !(ball_inertia.dist(target_pos) > 15 && ball_inertia.x > -25
                               && Strategy::i().self_Line() != Strategy::PostLine::back))
        dash_power = 100;
    if (ball_inertia.x < wm.ourDefenseLineX() - 10) {
        dash_power = 100;
    }
    if (target_pos.dist(Vector2D(-52, 0)) < 25)
        if ((Strategy::i().self_Line() == Strategy::PostLine::back || target_pos.dist(ball_inertia) < 20))
            dash_power = 100;
    if (wm.interceptTable()->fastestOpponent())
        if (wm.interceptTable()->fastestOpponent()->pos().dist(target_pos) < 5)
            dash_power = 100;
    if (target_pos.x < -35)
        dash_power = 100;
    if (opp_pos.dist(ball_inertia) > 40 && self_pos.dist(target_pos) < 10)
        dash_power = Strategy::get_normal_dash_power(wm);
    if (opp_pos.dist(ball_inertia) > 30 && self_pos.dist(target_pos) < 5)
        dash_power = Strategy::get_normal_dash_power(wm);
    if (opp_pos.dist(ball_inertia) > 20 && self_pos.dist(target_pos) < 3)
        dash_power = Strategy::get_normal_dash_power(wm);
    if (wm.self().stamina() < 4500)
        dash_power = Strategy::get_normal_dash_power(wm);
    if (wm.self().stamina() < 5500 && Strategy::i().self_Line() == Strategy::PostLine::forward)
        dash_power = Strategy::get_normal_dash_power(wm);
    return dash_power;
}

void bhv_mark_execute::lead_mark_move(PlayerAgent * agent, Target targ, double dash_power, double dist_thr, MarkType mark_type, Vector2D opp_pos){
    const WorldModel & wm = agent->world();
    int opp_min_cycle = wm.interceptTable()->opponentReachCycle();
    Vector2D ball_inertia = wm.ball().inertiaPoint(opp_min_cycle);
    Vector2D self_pos = wm.self().pos();
    Vector2D target_pos = targ.pos;
    if ((self_pos.dist(Vector2D(-52.0, 0.0)) < 25.0 ||
         (self_pos.dist(target_pos) < 2 && ball_inertia.x < -30)
         || (abs(target_pos.x - wm.ourDefenseLineX()) < 5 && target_pos.x < -35)) && wm.self().stamina() > 3000) {
        AngleDeg dir = (target_pos - self_pos).th() - wm.self().body();
        if ((target_pos.dist(wm.self().pos()) < 1 || (abs(dir.degree()) < 10 && abs(dir.degree()) > 170)) &&
            agent->doDash(100, dir)) {
            #ifdef DEBUG_MARK_EXECUTE
            dlog.addText(Logger::MARK, "Do Dash A");
            #endif
            agent->debugClient().addMessage("mark:move:dash (%.1f,%.1f) %.1f", targ.pos.x, targ.pos.y, 100.0);
            return;
        }
    }
    #ifdef DEBUG_MARK_EXECUTE
    dlog.addText(Logger::MARK, "Mark:Body Go To Point to (%.1f,%.1f) thr:%.1f power %.1f", target_pos.x, target_pos.y, dist_thr, dash_power);
    #endif
    if(mark_type == MarkType::LeadNearMark){
        bool am_i_near = false;
        Line2D ball_opp_line = Line2D(ball_inertia, opp_pos);
        for(int i = 0; i < wm.teammatesFromBall().size() && i < 2; i++){
            if (wm.teammatesFromBall().at(i) == NULL)
                break;
            if (wm.teammatesFromBall().at(i)->unum() < 1)
                break;
            if (wm.teammatesFromBall().at(i)->unum() == wm.self().unum()){
                am_i_near = true;
                break;
            }
        }
        if(am_i_near){

            if(ball_opp_line.dist(self_pos) < 5){
                target_pos = ball_inertia;
                #ifdef DEBUG_MARK_EXECUTE
                dlog.addText(Logger::MARK, "##target pos change to ball!!!");
                #endif
            }
        }
        if(ball_opp_line.dist(self_pos) > 5 && Segment2D(ball_inertia, target_pos).projection(self_pos).isValid()){
            target_pos = Segment2D(ball_inertia, target_pos).projection(self_pos);
            #ifdef DEBUG_MARK_EXECUTE
            dlog.addText(Logger::MARK, "##target pos change to proj!!!");
            #endif
        }
    }
    Segment2D opp_ball_seg(ball_inertia, opp_pos);
    if (!opp_ball_seg.projection(self_pos).isValid())
        dist_thr = 0.1;
    double angle_thr = 15.0;
    if (self_pos.dist(target_pos) > 2.0)
        angle_thr = 20.0;
//    Body_GoToPoint( const Vector2D & point,
//    const double & dist_thr,
//    const double & max_dash_power,
//    const double & dash_speed = -1.0,
//    const int cycle = 100,
//    const bool save_recovery = true,
//    const double & dir_thr = 15.0 )
    if(Body_GoToPoint(target_pos, dist_thr, dash_power, 100, false, angle_thr).execute(agent)){
//    if(Body_GoToPoint(target_pos, dist_thr, dash_power).execute(agent)){
        agent->debugClient().addMessage("mark:move:BGD (%.1f,%.1f) %.1f", targ.pos.x, targ.pos.y, dash_power);
        #ifdef DEBUG_MARK_EXECUTE
        dlog.addText(Logger::MARK, "ran go to point");
        #endif
    }else{
        #ifdef DEBUG_MARK_EXECUTE
        dlog.addText(Logger::MARK, "did not run go to point");
        #endif
    }
}

double bhv_mark_execute::other_mark_power(PlayerAgent * agent, Vector2D opp_pos, Vector2D target_pos){
    const WorldModel & wm = agent->world();
    int opp_min_cycle = wm.interceptTable()->opponentReachCycle();
    Vector2D ball_inertia = wm.ball().inertiaPoint(opp_min_cycle);
    double dash_power = Strategy::get_normal_dash_power(wm);
    if (opp_min_cycle < 5 && !(ball_inertia.dist(target_pos) > 15 && ball_inertia.x > -25
                               && Strategy::i().self_Line() != Strategy::PostLine::back))
        dash_power = 100;
    if (ball_inertia.x < wm.ourDefenseLineX() - 10) {
        dash_power = 100;
    }
    if (target_pos.dist(Vector2D(-52, 0)) < 25)
        if ((Strategy::i().self_Line() == Strategy::PostLine::back || target_pos.dist(ball_inertia) < 20))
            dash_power = 100;
    if (wm.interceptTable()->fastestOpponent()->pos().dist(target_pos) < 5)
        dash_power = 100;
    if (target_pos.x < -35)
        dash_power = 100;
    if (opp_min_cycle <= 2)
        dash_power = 100;
    return dash_power;
}

void bhv_mark_execute::other_mark_move(PlayerAgent * agent, Target targ, double dash_power, double dist_thr){
    const WorldModel & wm = agent->world();
    int opp_min_cycle = wm.interceptTable()->opponentReachCycle();
    Vector2D ball_inertia = wm.ball().inertiaPoint(opp_min_cycle);
    Vector2D self_pos = wm.self().pos();
    Vector2D target_pos = targ.pos;
    if ((self_pos.dist(Vector2D(-52.0, 0.0)) < 25.0 ||
         (self_pos.dist(target_pos) < 2 && ball_inertia.x < -30)
         || (abs(target_pos.x - wm.ourDefenseLineX()) < 5 && target_pos.x < -35)) && wm.self().stamina() > 3000) {
        AngleDeg dir = (target_pos - self_pos).th() - wm.self().body();
        if ((target_pos.dist(wm.self().pos()) < 1 || (abs(dir.degree()) < 10 && abs(dir.degree()) > 170)) &&
            agent->doDash(100, dir)) {
            #ifdef DEBUG_MARK_EXECUTE
            dlog.addText(Logger::MARK, "Do Dash A");
            #endif
            return;
        }
    }
    #ifdef DEBUG_MARK_EXECUTE
    dlog.addText(Logger::MARK, "Body Go To Point to (%.1f,%.1f) thr:%.1f power %.1f", target_pos.x, target_pos.y, dist_thr, dash_power);
    #endif
    if(Body_GoToPoint(target_pos, dist_thr, dash_power).execute(agent)){
        #ifdef DEBUG_MARK_EXECUTE
        dlog.addText(Logger::MARK, "ran go to point");
        #endif
    }else{
        #ifdef DEBUG_MARK_EXECUTE
        dlog.addText(Logger::MARK, "did not run go to point");
        #endif
    }

}

bool bhv_mark_execute::back_to_def(PlayerAgent *agent) {
    const WorldModel &wm = agent->world();
    Vector2D ball_pos = wm.ball().inertiaPoint(wm.interceptTable()->opponentReachCycle());
    Vector2D home_pos = Strategy::i().getPosition(wm.self().unum());
    Vector2D self_pos = wm.self().pos();
    double def_line_x = std::min(wm.ourDefenseLineX(), ball_pos.x);
    if (wm.gameMode().type() != GameMode::PlayOn)
        return false;
    if (home_pos.x < self_pos.x - 2) {
        #ifdef DEBUG_MARK_EXECUTE
        dlog.addText(Logger::MARK, "backtodef and ballpos (%d,%d) , homeposx: %d and defx: %d",
                     ball_pos.x, ball_pos.y, home_pos.x, def_line_x);
        #endif
        if (ball_pos.x > -35 && home_pos.x < def_line_x - 5) {
            if (self_pos.dist(home_pos) > 5) {
                return true;
            }
        }
        else {
            if (self_pos.dist(home_pos) > 15) {
                return true;
            }
        }
    }
    return false;
}
#include "../setplay/bhv_set_play.h"
Vector2D bhv_mark_execute::change_position_set_play(const WorldModel &wm, Vector2D target){
    Vector2D ball = wm.ball().pos();
    if (target.dist(ball) < 11){
        for (int i = 1; i < 20; i++){
            Vector2D new_target = Vector2D::polar2vector(i, (Vector2D(-52.5,0) - target).th()) + target;
            if (new_target.dist(ball) > 11){
                target = new_target;
                break;
            }
        }
    }
    return Bhv_SetPlay().get_avoid_circle_point(wm, target);
}
bhv_mark_execute::bhv_mark_execute() {
    // TODO Auto-generated constructor stub
}

bhv_mark_execute::~bhv_mark_execute() {
    // TODO Auto-generated destructor stub
}