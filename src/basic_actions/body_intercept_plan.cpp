// -*-c++-*-

/*!
  \file body_clear_ball2009.cpp
  \brief kick the ball to escape from dangerous situation
*/


/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <chrono>

#include <rcsc/player/player_evaluator.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/math_util.h>
#include <rcsc/common/logger.h>

#include "body_intercept_plan.h"
#include "neck/neck_decision.h"
#include "move_def/bhv_mark_execute.h"
#include "move_def/cyrus_interceptable.h"
#include "basic_actions/basic_actions.h"
#include "basic_actions/body_intercept2009.h"
#include "chain_action/shoot_generator.h"

#include "setting.h"
#include "strategy.h"
#include "move_def/bhv_block.h"
#include "move_def/bhv_tackle_intercept.h"
#include "chain_action/bhv_pass_kick_find_receiver.h"
#include "chain_action/action_chain_holder.h"
#include "basic_actions/body_go_to_point.h"
#include "chain_action/bhv_strict_check_shoot.h"


using namespace rcsc;


bool
Body_InterceptPlan::execute(PlayerAgent *agent) {
    const WorldModel &wm = agent->world();
    /*--------------------------------------------------------*/
    // chase ball
    const int self_min = wm.interceptTable().selfStep();
    const int mate_min = wm.interceptTable().teammateStep();
    const int opp_min = wm.interceptTable().opponentStep();

    auto intercept_tackle_info = bhv_tackle_intercept::intercept_cycle(wm);
    int cycle_intercept_tackle = intercept_tackle_info.first;

    {
        int self_tackle_min = wm.interceptTable().selfStepTackle();
        Vector2D balliner = wm.ball().inertiaPoint(self_min);
        Vector2D balliner_tackle = wm.ball().inertiaPoint(self_tackle_min);
        Vector2D balltackle = wm.ball().inertiaPoint(cycle_intercept_tackle);
        dlog.addCircle(Logger::INTERCEPT, balliner,1,0,0,0, true);
        dlog.addCircle(Logger::INTERCEPT, balliner_tackle,0.5,255,255,255, true);
        dlog.addCircle(Logger::INTERCEPT, balltackle,0.25,255,0,0, true);
    }

    Vector2D tm_catch_ball = wm.ball().inertiaPoint(std::min(self_min, mate_min));
    if (tm_catch_ball.x < -52.5) {
        if (wm.ball().vel().r() > 0.1) {
            Line2D ball_move = Line2D(wm.ball().pos(), wm.ball().vel().th());
            Line2D goal_line = Line2D(Vector2D(-52.5, 0), 90);
            Vector2D inters = ball_move.intersection(goal_line);
            if (inters.isValid()) {
                if (inters.absY() < 7.5) {
                    if (wm.ball().inertiaPoint(cycle_intercept_tackle).x > -54) {
                        if (bhv_tackle_intercept().execute(agent)) {
                            agent->debugClient().addMessage("tackle inter for prevent goal");
                            return true;
                        }
                    }
                }
            }
        }
    }
    Vector2D next_pass = Vector2D::INVALIDATED;
    Vector2D face = Vector2D(52, 0);
    if (self_min < opp_min + 1) {
        bool shoot_is_best = false;
        if(wm.ball().pos().dist(Vector2D(52,0)) < 20){
            Vector2D shoot_tar = Bhv_StrictCheckShoot(0).get_best_shoot(wm);
            if(shoot_tar.isValid()){
                next_pass = Vector2D(52, 0);
                face = shoot_tar;
                shoot_is_best = true;
            }
        }
        if(!shoot_is_best){
            const ActionChainGraph &chain_graph = ActionChainHolder::i().graph();
            if (!chain_graph.getAllChain().empty()){
                const CooperativeAction &first_action = chain_graph.getFirstAction();
                switch (first_action.category()) {
                    case CooperativeAction::Shoot: {
                        next_pass = Vector2D(52, 0);
                        face = first_action.targetPoint();
                        break;
                    }

                    case CooperativeAction::Dribble: {
                        face = first_action.targetPoint();
                        break;
                    }

                    case CooperativeAction::Hold: {
                        break;
                    }

                    case CooperativeAction::Pass: {
                        face = first_action.targetPoint();
                        next_pass = first_action.targetPoint();
                        if (self_min == 1) {
                            Bhv_PassKickFindReceiver(chain_graph).doSayPrePass(agent, first_action);
                        }
                        break;
                    }
                }
            }
        }
    }
    bool tm_drible = false;
    bool tm_pass = false;

    if (wm.lastKickerSide() == wm.ourSide()) {
        int cycle_for_drible = 2;
        const int real_set_play_count = wm.time().cycle() - wm.lastSetPlayStartTime().cycle();
        if (wm.ball().pos().x > 45 && wm.ball().pos().absY() > 30)
            if (real_set_play_count < 100)
                cycle_for_drible = 2;
        if (!wm.audioMemory().dribble().empty()
            && wm.audioMemory().dribbleTime().cycle()
               > wm.time().cycle() - cycle_for_drible) {
            if (wm.audioMemory().pass().empty()
                || (!wm.audioMemory().pass().empty()
                    && wm.audioMemory().passTime().cycle()
                       < wm.audioMemory().dribbleTime().cycle())) {
                if (opp_min > mate_min) {
                    if (wm.audioMemory().dribble().front().sender_
                        != wm.self().unum())
                        tm_drible = true;
                }
            }
        }
        int cycle_for_pass = 2;
        if (!wm.audioMemory().pass().empty()
            && wm.audioMemory().passTime().cycle()
               > wm.time().cycle() - cycle_for_pass) {
            if (opp_min > mate_min && mate_min - 3 < self_min) {
                if (wm.audioMemory().pass().front().receiver_
                    != wm.self().unum())
                    tm_pass = true;
            }
        }
    }
    Vector2D inertia_ball_pos = wm.ball().inertiaPoint(
            wm.interceptTable().opponentStep());
    int oppCycles = 0;
    vector<Vector2D> ball_cache = CyrusPlayerIntercept::createBallCache(wm);
    CyrusPlayerIntercept ourpredictor(wm, ball_cache);
    const PlayerObject *it = wm.interceptTable().firstOpponent();
    if (it != NULL && (*it).unum() >= 2) {
        const PlayerType *player_type = (*it).playerTypePtr();
        vector<CyrusOppInterceptTable> pred = ourpredictor.predict(*it, *player_type, 1000);
        CyrusOppInterceptTable tmp = CyrusPlayerIntercept::getBestIntercept(wm, pred);
        oppCycles = tmp.cycle;
        inertia_ball_pos = tmp.current_position;
        if (oppCycles > 100 || !inertia_ball_pos.isValid()) {
            oppCycles = wm.interceptTable().opponentStep();
            inertia_ball_pos = wm.ball().inertiaPoint(oppCycles);
        }
    }
    int dif = 2;
    if (opp_min < mate_min && self_min < mate_min && opp_min < self_min) {
        if (Strategy::i().selfLine() == PostLine::forward
            && wm.ball().inertiaPoint(opp_min).x > 25) {
            dif = 5;
        }
    }

    bool use_tackle_intercept = false;
    if (wm.kickableTeammate())
        return false;
    if (wm.ball().pos().x > -15 || wm.lastKickerSide() == wm.ourSide()) {
        if (!tm_drible && !tm_pass) {
            if (self_min <= 1) {
                if (!Body_Intercept2009(false, face).execute(agent)){
                    if (!Body_GoToPoint(wm.ball().inertiaPoint(1), 0.1, 100).execute(agent)){
                        Body_TurnToPoint(wm.ball().inertiaPoint(1)).execute(agent);
                    }
                }
                NeckDecisionWithBall().setNeck(agent, NeckDecisionType::intercept);
                agent->debugClient().addMessage("Intercept->Z");
                return true;
            } else if (self_min <= 3) {
                if (self_min <= mate_min) {
                    if (self_min < opp_min + dif) {
                        if (self_min <= opp_min) {
                            if (!Body_Intercept2009(false, face).execute(agent)){
                                if (!Body_GoToPoint(wm.ball().inertiaPoint(self_min), 0.1, 100).execute(agent)){
                                    Body_TurnToPoint(wm.ball().inertiaPoint(1)).execute(agent);
                                }
                            }
                            agent->debugClient().addMessage("Intercept->A");
                        } else {
                            if (!M_called_from_block && bhv_block().execute(agent)) {
                                agent->debugClient().addMessage("Intercept->Block");
                                return true;
                            }
                            Vector2D opp_target = wm.ball().inertiaPoint(opp_min);
                            if (!Body_GoToPoint(opp_target, 0.5, 100).execute(agent)) {
                                Body_TurnToPoint(face).execute(agent);
                            }
                            agent->debugClient().addMessage("Intercept->B");
                        }
                        NeckDecisionWithBall().setNeck(agent, NeckDecisionType::intercept);
                        return true;
                    } else {
                        use_tackle_intercept = true;
                    }
                }
            } else {
                if (self_min <= mate_min) {
                    if (self_min < opp_min + dif) {
                        if (self_min <= opp_min) {
                            if (!Body_Intercept2009(false, face).execute(agent)){
                                if (!Body_GoToPoint(wm.ball().inertiaPoint(1), 0.1, 100).execute(agent)){
                                    Body_TurnToPoint(wm.ball().inertiaPoint(1)).execute(agent);
                                }
                            }
                            agent->debugClient().addMessage("Intercept->C");
                        } else {
                            if (!M_called_from_block && bhv_block().execute(agent)) {
                                agent->debugClient().addMessage("Intercept->Block");
                                return true;
                            }
                            Vector2D opp_target = wm.ball().inertiaPoint(opp_min);
                            if (!Body_GoToPoint(opp_target, 0.5, 100).execute(agent)) {
                                Body_TurnToPoint(face).execute(agent);
                            }
                            agent->debugClient().addMessage("Intercept->D");

                        }
                        NeckDecisionWithBall().setNeck(agent, NeckDecisionType::intercept);
                        return true;
                    } else {
                        use_tackle_intercept = true;
                    }
                }
            }

        }
    } else {
        if (self_min <= 1) {
            if (!Body_Intercept2009(false).execute(agent)){
                if (!Body_GoToPoint(wm.ball().inertiaPoint(1), 0.1, 100).execute(agent)){
                    Body_TurnToPoint(wm.ball().inertiaPoint(1)).execute(agent);
                }
            }
            NeckDecisionWithBall().setNeck(agent, NeckDecisionType::intercept);
            agent->debugClient().addMessage("Intercept->Y");
            return true;
        } else if (self_min <= mate_min && self_min < opp_min + 3) {
            if (self_min <= opp_min) {
                if (!Body_Intercept2009(false).execute(agent)){
                    if (!Body_GoToPoint(wm.ball().inertiaPoint(1), 0.1, 100).execute(agent)){
                        Body_TurnToPoint(wm.ball().inertiaPoint(1)).execute(agent);
                    }
                }
                agent->debugClient().addMessage("Intercept->F");
            } else {
                if (!M_called_from_block && bhv_block().execute(agent)) {
                    agent->debugClient().addMessage("Intercept->Block");
                    return true;
                }
            }
            NeckDecisionWithBall().setNeck(agent, NeckDecisionType::intercept);
            return true;
        } else if (self_min <= mate_min) {
            use_tackle_intercept = true;
        }
    }

    int diff = 2;
    if (wm.ball().inertiaPoint(oppCycles).x > 35 && wm.ball().inertiaPoint(oppCycles).absY() < 20)
        diff = 0;
    agent->debugClient().addMessage("tc:%d", cycle_intercept_tackle);
    int self_min_tackle = wm.interceptTable().selfStep();
    if (self_min_tackle < 10 && self_min_tackle < opp_min - diff &&
        self_min_tackle < mate_min - diff) {
        agent->debugClient().addMessage("TackleIntercept");
        return Body_Intercept2009(false).executeTackle(agent);
    }
    if (cycle_intercept_tackle != 1000 && cycle_intercept_tackle < opp_min - diff &&
        cycle_intercept_tackle < mate_min - diff) {
        agent->debugClient().addMessage("Intercept->Tackle");
        return bhv_tackle_intercept().execute(agent);
    }
    return false;
}