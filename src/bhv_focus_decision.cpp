#include "bhv_focus_decision.h"
#include "focus_reset.h"
#include "focus_move_to_point.h"
#include <rcsc/player/player_agent.h>

bool Bhv_FocusDecision::executeReset( rcsc::PlayerAgent * agent ){
    return Focus_Reset().execute(agent);
}

bool Bhv_FocusDecision::executeSample( rcsc::PlayerAgent * agent, Vector2D target){
    return Focus_MoveToPoint(target).execute(agent);
}
bool Bhv_FocusDecision::executePass( rcsc::PlayerAgent * agent, int unum, Vector2D target ){
    const WorldModel & wm = agent->world();
    if (wm.ball().velCount() > 1){
        return Focus_MoveToPoint(wm.ball().pos()).execute(agent);
    }
    return Focus_MoveToPoint(target).execute(agent);
}
bool Bhv_FocusDecision::executeDribble( rcsc::PlayerAgent * agent, Vector2D target ){
    const WorldModel & wm = agent->world();
    if (wm.ball().velCount() > 1){
        return Focus_MoveToPoint(wm.ball().pos()).execute(agent);
    }
    return Focus_MoveToPoint(target).execute(agent);
}
bool Bhv_FocusDecision::executeShoot( rcsc::PlayerAgent * agent, Vector2D target ){
    const WorldModel & wm = agent->world();
    if (wm.ball().velCount() > 1){
        return Focus_MoveToPoint(wm.ball().pos()).execute(agent);
    }
    return Focus_MoveToPoint(target).execute(agent);
}
bool Bhv_FocusDecision::executeMark( rcsc::PlayerAgent * agent, int unum, Vector2D target ){
    const WorldModel & wm = agent->world();
    if (wm.ball().velCount() > 2){
        return Focus_MoveToPoint(wm.ball().pos()).execute(agent);
    }
    return Focus_MoveToPoint(target).execute(agent);
}
bool Bhv_FocusDecision::executeBlock( rcsc::PlayerAgent * agent, Vector2D target ){
    const WorldModel & wm = agent->world();
    if (wm.ball().velCount() > 1){
        return Focus_MoveToPoint(wm.ball().pos()).execute(agent);
    }
    return Focus_MoveToPoint(target).execute(agent);
}
bool Bhv_FocusDecision::executeOffMove( rcsc::PlayerAgent * agent ){
    const WorldModel & wm = agent->world();
    return Focus_MoveToPoint(wm.ball().pos()).execute(agent);
}
bool Bhv_FocusDecision::executeDeffMove( rcsc::PlayerAgent * agent ){
    const WorldModel & wm = agent->world();
    return Focus_MoveToPoint(wm.ball().pos()).execute(agent);
}

bool Bhv_FocusDecision::executeIntercept( rcsc::PlayerAgent * agent, Vector2D next_target){
    const WorldModel & wm = agent->world();
    auto ball_vel = wm.ball().vel();
    auto ball_pos = wm.ball().pos() + ball_vel;
    return Focus_MoveToPoint(ball_pos).execute(agent);
}

bool Bhv_FocusDecision::executeMove( rcsc::PlayerAgent * agent ){
    const WorldModel & wm = agent->world();
    auto ball_pos = wm.ball().pos();
    return Focus_MoveToPoint(ball_pos).execute(agent);
}