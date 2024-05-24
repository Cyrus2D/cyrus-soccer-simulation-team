/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "bhv_defensive_move.h"

#include "strategy.h"

#include "move_def/bhv_basic_tackle.h"
#include "move_off/bhv_unmark.h"
#include "move_off/bhv_scape.h"
#include "move_def/bhv_block.h"
#include "move_def/bhv_mark_execute.h"
#include "move_def/bhv_tackle_intercept.h"
#include "chain_action/field_analyzer.h"

#include "move_def/cyrus_interceptable.h"
#include "chain_action/bhv_pass_kick_find_receiver.h"
#include "chain_action/action_chain_holder.h"
#include "basic_actions/basic_actions.h"
#include "basic_actions/body_go_to_point.h"
#include "basic_actions/body_intercept2009.h"
#include "basic_actions/neck_turn_to_ball_or_scan.h"
#include "basic_actions/neck_turn_to_low_conf_teammate.h"
#include "chain_action/shoot_generator.h"
#include "chain_action/bhv_strict_check_shoot.h"
#include <thread>
#include <chrono>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/player/intercept_table.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/common/audio_memory.h>

#include <rcsc/player/abstract_player_object.h>
#include <rcsc/player/cut_ball_calculator.h>
#include "neck/neck_offensive_intercept_neck.h"
#include "move_off/bhv_offensive_move.h"
#include "neck/next_pass_predictor.h"
#include "neck/neck_decision.h"
#include "setting.h"

#define DEBUG_PRINT
// #define DEBUG_PRINT_INTERCEPT_LIST

#define USE_GOALIE_MODE

using namespace rcsc;

bool Bhv_DefensiveMove::setDefNeckWithBall(PlayerAgent *agent, Vector2D targetPoint, const AbstractPlayerObject *opp,
                                           int blocker) {
    const WorldModel &wm = agent->world();
    Vector2D next_target = Vector2D::INVALIDATED;
    int self_min = wm.interceptTable().selfStep();
    int ball_pos_count = wm.ball().posCount();
    int ball_pos_count_s = wm.ball().seenPosCount();
    int ball_vel_count = wm.ball().velCount();
    int ball_vel_count_s = wm.ball().seenVelCount();
    Vector2D ball_pos = wm.ball().pos();
    Vector2D self_pos = wm.self().pos();
    AngleDeg self_body = agent->effector().queuedNextMyBody();
    int min_ball_count = std::min(std::min(ball_pos_count, ball_pos_count_s),
                                  std::min(ball_vel_count, ball_vel_count_s));
    AngleDeg ball_angle = (ball_pos - self_pos).th();
    Vector2D ball_iner = wm.ball().inertiaPoint(self_min);
    const double next_view_width = agent->effector().queuedNextViewWidth().width();
    bool should_see_ball = false;
    if (min_ball_count >= std::min(wm.interceptTable().opponentStep(), 5))
        should_see_ball = true;
    bool can_see_ball = false;
    if ((self_body - ball_angle).abs() < next_view_width * 0.5 + 85)
        can_see_ball = true;
    if (wm.self().isKickable())
        should_see_ball = false;
    std::vector<std::pair<Vector2D, double>> targs;
    for (int i = 0; i < wm.opponentsFromSelf().size() && i <= 3; i++) {
        if (wm.opponentsFromSelf().at(i) != NULL && wm.opponentsFromSelf().at(i)->posCount() > 0)
            targs.emplace_back(wm.opponentsFromSelf().at(i)->pos(), 0.9);
    }

    for (int i = 0; i < wm.teammatesFromSelf().size() && i <= 2; i++) {
        if (wm.teammatesFromSelf().at(i)->posCount() > 0)
            targs.emplace_back(make_pair(wm.teammatesFromSelf().at(i)->pos(), 0.8));
    }
    if (opp->posCount() > 0)
        targs.emplace_back(make_pair(opp->pos(), 1.3));
    if (wm.interceptTable().firstOpponent() != NULL
        && wm.interceptTable().firstOpponent()->posCount() > 0) {
        targs.emplace_back(make_pair(wm.interceptTable().firstOpponent()->pos(), 1.1));
    }
    if (blocker != 0 && blocker != wm.self().unum() && wm.ourPlayer(blocker) != NULL &&
        wm.ourPlayer(blocker)->posCount() > 0)
        targs.emplace_back(make_pair(wm.ourPlayer(blocker)->pos(), 0.9));
    if (wm.ball().posCount() > 0)
        targs.emplace_back(make_pair(wm.ball().pos(), 1.2));

    vector<const AbstractPlayerObject *> myLine = Strategy::i().getTeammatesInPostLine(wm, Strategy::i().tmLine(
            wm.self().unum()));
    for (auto &i : myLine) {
        if (i->unum() != wm.self().unum() && i->posCount() > 2)
            targs.emplace_back(make_pair(i->pos(), 0.7));
    }
    if (ball_iner.x < -35 && wm.getOurGoalie() != NULL && wm.getOurGoalie()->posCount() > 2) {
        targs.emplace_back(make_pair(wm.getOurGoalie()->pos(), 0.6));
    }


    for (auto &targ : targs) {
        dlog.addText(Logger::TEAM,
                     __FILE__": def neck target=(%.1f %.1f) eval=%.2f", targ.first.x,
                     targ.first.y, targ.second);
    }


    double best_neck = -1000;
    double best_eval = 0;
    for (double neck = self_body.degree() - 90; neck <= self_body.degree() + 90; neck += 10) {
        AngleDeg min_see(neck - next_view_width / 2.0 + 10);
        AngleDeg max_see = (neck + next_view_width / 2.0 - 10);
        double eval = 0;
        if (should_see_ball) {
            if (can_see_ball) {
                if (ball_angle.isWithin(min_see, max_see)) {
                    for (auto &targ : targs) {
                        AngleDeg targ_angle = (targ.first - self_pos).th();
                        if (targ_angle.isWithin(min_see, max_see)) {
                            eval += (wm.dirCount(targ_angle) * targ.second);
                        }
                    }
                }
            } else {
                for (auto &targ : targs) {
                    AngleDeg targ_angle = (targ.first - self_pos).th();
                    if (targ_angle.isWithin(min_see, max_see)) {
                        eval += (wm.dirCount(targ_angle) * targ.second);
                    }
                }
            }
        } else {
            for (auto &targ : targs) {
                AngleDeg targ_angle = (targ.first - self_pos).th();
                if (targ_angle.isWithin(min_see, max_see)) {
                    eval += (wm.dirCount(targ_angle) * targ.second);
                }
            }
        }
        if (eval > best_eval) {
            best_eval = eval;
            best_neck = neck;
        }
    }
    dlog.addText(Logger::TEAM,
                 __FILE__": def neck %.1f eval=%.2f", best_neck, best_eval);
    if (best_eval != 0) {
        agent->setNeckAction(new Neck_TurnToPoint(self_pos + Vector2D::polar2vector(10, best_neck)));
    } else if (should_see_ball && can_see_ball) {
        agent->setNeckAction(new Neck_TurnToBall());
    } else {
        agent->setNeckAction(new Neck_TurnToBallOrScan(1));
    }
    return true;
}


bool Bhv_DefensiveMove::execute(rcsc::PlayerAgent *agent) {

    const WorldModel &wm = agent->world();
    /*--------------------------------------------------------*/
    // chase ball
    const int self_min = wm.interceptTable().selfStep();
    const int mate_min = wm.interceptTable().teammateStep();
    const int opp_min = wm.interceptTable().opponentStep();

    if (wm.interceptTable().firstOpponent() == NULL || wm.interceptTable().firstOpponent()->unum() < 1)
        return false;

    bool mark_or_block = true;
    if (Strategy::i().tmLine(wm.self().unum()) == PostLine::forward) {

        if (wm.ball().pos().x < -20) {
            if (wm.self().stamina() < 4000) {
                mark_or_block = false;
            }
        } else if (wm.self().stamina() < 5000) {
            mark_or_block = false;
        }
    }
    if (mark_or_block) {
        dlog.addText(Logger::BLOCK, "block or mark");
        if (bhv_mark_execute().execute(agent)){
            agent->debugClient().addMessage("MarkEx");
            return true;
        }
    }

    double min_x_hpos = 1000;
    for (int i = 2; i <= 11; i++) {
        double hposx = Strategy::i().getPosition(i).x;
        if (hposx < min_x_hpos)
            min_x_hpos = hposx;
    }

    Vector2D inertia_ball_pos = wm.ball().inertiaPoint(
            wm.interceptTable().opponentStep());
    double mark_x_line = std::max(wm.ourDefenseLineX(), min_x_hpos);

    Vector2D target_point = Strategy::i().getPosition(wm.self().unum());

    if (wm.audioMemory().waitRequest().size() > 0) {
        if (wm.audioMemory().waitRequestTime().cycle() > wm.time().cycle() - 4) {
            if (Strategy::i().tmLine(wm.self().unum()) == PostLine::back) {
                if (wm.audioMemory().waitRequest().front().sender_ != wm.self().unum()) {
                    const AbstractPlayerObject *tm = wm.ourPlayer(wm.audioMemory().waitRequest().front().sender_);
                    if (tm != NULL && tm->unum() > 1) {
                        target_point.x = tm->pos().x;
                    }
                }
            }
        }
    }

    double min_x_strategy = 100;
    for (int i = 2; i <= 11; i++) {
        double x = Strategy::i().getPosition(i).x;
        if (x < min_x_strategy)
            min_x_strategy = x;
    }

    double dash_power = Strategy::getNormalDashPower(wm);
    if (wm.ball().pos().x < wm.self().pos().x
        && Strategy::i().tmLine(wm.self().unum()) == PostLine::back) {
        dash_power = 100;
    }
    if (Strategy::i().tmLine(wm.self().unum()) == PostLine::back || Strategy::i().tmLine(wm.self().unum()) == PostLine::half) {
        if (wm.ball().inertiaPoint(opp_min).x < -20
            && (wm.self().pos().dist(target_point) > 4
                || wm.self().pos().x > target_point.x + 2)) {
            dash_power = 100;
        }
    }

    if (wm.self().pos().x < min_x_strategy)
        dash_power = 100;
    if (Strategy::i().tmLine(wm.self().unum()) == PostLine::back && abs(inertia_ball_pos.x - wm.ourDefenseLineX()) < 20) {
        dash_power = 100;
    }
    double dist_thr = wm.ball().distFromSelf() * 0.1;
    if (dist_thr < 1.0)
        dist_thr = 1.0;

    if(wm.self().unum() < 5){
        if(inertia_ball_pos.x > 0 && wm.self().pos().x < 0){
            if(wm.self().stamina() < 6000){
                target_point.x = std::min(target_point.x, -1.0);
            }
        }
    }
    dlog.addText(Logger::TEAM,
                 __FILE__": Bhv_BasicMove target=(%.1f %.1f) dist_thr=%.2f", target_point.x,
                 target_point.y, dist_thr);

    agent->debugClient().addMessage("BasicMoveDef%.0f", dash_power);
    agent->debugClient().setTarget(target_point);
    agent->debugClient().addCircle(target_point, dist_thr);
    double base_def_pos_x = Strategy::i().getPosition(2).x;
    if (wm.self().stamina() < 4500 && wm.self().pos().x < base_def_pos_x) {
        agent->addSayMessage(new WaitRequestMessage());
    }
    if (!Body_GoToPoint(target_point, dist_thr, dash_power).execute(agent)) {
        Body_TurnToPoint(target_point).execute(agent);
    }
    const AbstractPlayerObject *nearest_opp = wm.opponentsFromSelf().at(0);
    if (nearest_opp != nullptr)
        setDefNeckWithBall(agent, wm.ball().pos(), nearest_opp, 0);
    else {
        if (wm.kickableOpponent() && wm.ball().distFromSelf() < 18.0) {
            agent->setNeckAction(new Neck_TurnToBall());
        } else {
            agent->setNeckAction(new Neck_TurnToBallOrScan(0));
        }
    }
    return true;
}