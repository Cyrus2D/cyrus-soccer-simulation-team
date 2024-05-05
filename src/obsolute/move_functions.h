//
// Created by nader on 4/30/24.
//

#ifndef CYRUS_MOVE_FUNCTIONS_H
#define CYRUS_MOVE_FUNCTIONS_H


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "bhv_basic_move.h"

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

bool can_opp_shoot_to_goal(const WorldModel &wm) {
    int opp_min = wm.interceptTable().opponentStep();
    Vector2D ballpos = wm.ball().inertiaPoint(opp_min);
    if (!ServerParam::i().ourPenaltyArea().contains(ballpos))
        return false;
    bool twostepshoot = false;
    if (opp_min > 0 && wm.ball().vel().r() * std::pow(ServerParam::i().ballDecay(), opp_min) > 0.5) {
        AngleDeg opp2goal = (ballpos - Vector2D(-52, 0)).th();
        AngleDeg opp2ball = (ballpos - wm.ball().pos()).th();
        if ((opp2goal - opp2ball).abs() < 9)
            twostepshoot = true;
    }
    bool can_goal = false;
    for (double y = -6.5; y < 6.6; y += 0.5) {
        Vector2D target(-52.5, y);
        Vector2D ball_vel_sim = Vector2D::polar2vector(2.9, (target - ballpos).th());
        Vector2D ball_pos_sim = ballpos;
        bool can_intercept = false;
        for (int c = 1; c <= 30; c++) {
            ball_pos_sim += ball_vel_sim;
            if (ball_pos_sim.x < -52.5)
                can_goal = true;
            for (int t = 1; t <= 5; t++) {
                const AbstractPlayerObject *tm = wm.ourPlayer(t);
                if (tm == NULL || tm->unum() != t)
                    continue;
                if (tm->pos().dist(ball_pos_sim) / tm->playerTypePtr()->realSpeedMax() > c + 2)
                    continue;
                int dc;
                int dt;
                int dv;
                int intercept_cycle = CutBallCalculator().cycles_to_cut_ball(tm,  ball_pos_sim, c, true, dc, dt, dv);
                if (intercept_cycle <= c) {
                    can_intercept = true;
                    break;
                }
            }
            if (can_intercept)
                break;
            ball_vel_sim *= ServerParam::i().ballDecay();
        }
        if (can_intercept) {
            dlog.addCircle(Logger::SHOOT, target, 0.2, 0, 255, 0, true);
        } else {
            dlog.addCircle(Logger::SHOOT, target, 0.2, 255, 0, 0, true);
        }
    }
    return can_goal;
}

vector<int> who_goto_goal(PlayerAgent *agent) {
    const WorldModel &wm = agent->world();
    vector<int> results;
    int opp_min = wm.interceptTable().opponentStep();
    Vector2D ballpos = wm.ball().inertiaPoint(opp_min);
    if (can_opp_shoot_to_goal(wm)) {
        double min_dist = 1000;
        Vector2D tar;
        for (int i = 2; i <= 5; i++) {
            if (ballpos.y > 5) {
                tar = Vector2D(-52, -4);
            } else if (ballpos.y < -5) {
                tar = Vector2D(-52, +4);
            } else {
                if (Strategy::i().getPosition(i).y > 0) {
                    tar = Vector2D(-52, +5);
                } else {
                    tar = Vector2D(-52, -5);
                }
            }
            int min_unum = 0;
            for (int t = 2; t <= 5; t++) {
                const AbstractPlayerObject *tm = wm.ourPlayer(t);
                if (tm == NULL || tm->unum() != t)
                    continue;
                if (tm->pos().dist(wm.ball().inertiaPoint(wm.interceptTable().opponentStep())) < 3)
                    continue;
                Vector2D pos = tm->pos();
                double dist = pos.dist(tar);
                if (dist < min_dist) {
                    min_dist = dist;
                    min_unum = t;
                }
            }
            if (min_unum == i)
                results.push_back(i);
        }
    }
    return results;
}


bool go_to_goal(PlayerAgent *agent) {
    const WorldModel &wm = agent->world();
    int opp_min = wm.interceptTable().opponentStep();
    Vector2D ballpos = wm.ball().inertiaPoint(opp_min);
    if (wm.self().pos().dist(wm.ball().inertiaPoint(wm.interceptTable().opponentStep())) < 3)
        return false;
    if (can_opp_shoot_to_goal(wm)) {
        double min_dist = 1000;
        int min_unum = 0;
        Vector2D tar;
        if (ballpos.y > 5) {
            tar = Vector2D(-52, -4);
        } else if (ballpos.y < -5) {
            tar = Vector2D(-52, +4);
        } else {
            if (Strategy::i().getPosition(wm.self().unum()).y > 0) {
                tar = Vector2D(-52, +5);
            } else {
                tar = Vector2D(-52, -5);
            }
        }
        for (int t = 2; t <= 5; t++) {
            const AbstractPlayerObject *tm = wm.ourPlayer(t);
            if (tm == NULL || tm->unum() != t)
                continue;
            Vector2D pos = tm->pos();
            double dist = pos.dist(tar);
            if (dist < min_dist) {
                min_dist = dist;
                min_unum = t;
            }
        }

        if (min_unum == wm.self().unum()) {
            Vector2D self_pos = wm.self().pos();

            if (ballpos.y > 5) {
                if (ballpos.x < -42) {
                    tar = Vector2D(-51, 0);
                } else if (ballpos.x < -39) {
                    tar = Vector2D(-51, +4);
                }
            } else if (ballpos.y < -5) {
                if (ballpos.x < -42) {
                    tar = Vector2D(-51, 0);
                } else if (ballpos.x < -39) {
                    tar = Vector2D(-52, -4);
                }
            } else {
                if (ballpos.x < -42) {
                    if (tar.y < 0)
                        tar = Vector2D(-51, -4);
                    else
                        tar = Vector2D(-51, +4);
                } else if (ballpos.x < -39) {
                    if (tar.y < 0)
                        tar = Vector2D(-51, -5);
                    else
                        tar = Vector2D(-51, +5);
                }
            }

            if (!Body_GoToPoint(tar, 1.5, 100, 2, 1, false, 20).execute(agent)) {
                if (self_pos.dist(tar) < 1.0 && wm.interceptTable().opponentStep() <= 1 &&
                    wm.self().body().abs() > 80 && wm.self().body().abs() < 100) {
                    agent->doDash(100, (tar - wm.self().pos()).th() - wm.self().body());
                } else {
                    if (ballpos.y > wm.self().pos().y) {
                        Body_TurnToAngle(90).execute(agent);
                    } else {
                        Body_TurnToAngle(-90).execute(agent);
                    }
                }
            }
            agent->debugClient().addMessage("goto goal");
            if (wm.kickableOpponent() && wm.ball().distFromSelf() < 18.0 || opp_min <= 2) {
                agent->setNeckAction(new Neck_TurnToBall());
            } else {
                agent->setNeckAction(new Neck_TurnToBallOrScan(0));
            }
            return true;
        }
    }
    return false;
}


#endif //CYRUS_MOVE_FUNCTIONS_H
