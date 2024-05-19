// -*-c++-*-

/*!
  \file body_clear_ball2009.cpp
  \brief kick the ball to escape from dangerous situation
*/


/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <rcsc/player/player_evaluator.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/player_predicate.h>
#include <rcsc/math_util.h>

#include "body_turn_to_tackle.h"
#include "neck/neck_decision.h"
#include "move_def/bhv_mark_execute.h"
#include "move_def/cyrus_interceptable.h"
#include "basic_actions/basic_actions.h"
#include "basic_actions/body_intercept2009.h"
#include "chain_action/shoot_generator.h"


using namespace rcsc;


bool
Body_TurnToTackle::execute(PlayerAgent *agent) {
    const WorldModel &wm = agent->world();
    const int self_min = wm.interceptTable().selfStep();
    const int self_min_tackle = wm.interceptTable().selfStepTackle();
    const int mate_min = wm.interceptTable().teammateStep();
    const int opp_min = wm.interceptTable().opponentStep();

    if (mate_min <= opp_min)
        return false;
    if (self_min <= opp_min)
        return false;
    if (self_min <= 2)
        return false;
    if (self_min_tackle < opp_min && self_min_tackle < mate_min) {
        Body_Intercept2009(false).executeTackle(agent);
        agent->setNeckAction(new Neck_TurnToBall());
        agent->debugClient().addMessage("tackle intercept execute");
        return true;
    }
    Vector2D next_ball = wm.ball().inertiaPoint(1);
    if (next_ball.dist(wm.self().inertiaPoint(1)) < 1.7) {
        Body_TurnToPoint(next_ball).execute(agent);
        agent->setNeckAction(new Neck_TurnToBall());
        agent->debugClient().addMessage("turn to tackle");
        return true;
    }
    return false;
}