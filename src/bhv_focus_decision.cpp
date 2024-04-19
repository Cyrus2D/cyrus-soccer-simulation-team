#include "bhv_focus_decision.h"
#include "focus_reset.h"
#include "setting.h"
#include "focus_move_to_point.h"
#include <rcsc/player/player_agent.h>
#include <rcsc/player/soccer_action.h>
bool Bhv_FocusDecision::executeReset(rcsc::PlayerAgent *agent)
{
    agent->setFocusAction(new Focus_Reset());
    return true;
}

bool Bhv_FocusDecision::executeSample(rcsc::PlayerAgent *agent, Vector2D target)
{
    agent->setFocusAction(new Focus_MoveToPoint(target));
    return true;
}
bool Bhv_FocusDecision::executePass(rcsc::PlayerAgent *agent, int unum, Vector2D target)
{
    if (!Setting::i()->mNeck->mExecuteIntercept)
        return false;
    agent->debugClient().addMessage("InterceptFocus");
    const WorldModel &wm = agent->world();
    auto ball_vel = wm.ball().vel();
    auto ball_pos = wm.ball().pos() + ball_vel;
    return Focus_MoveToPoint(ball_pos).execute(agent);
}
bool Bhv_FocusDecision::executeDribble(rcsc::PlayerAgent *agent, Vector2D target)
{
    if (!Setting::i()->mNeck->mExecuteIntercept)
        return false;
    agent->debugClient().addMessage("InterceptFocus");
    const WorldModel &wm = agent->world();
    auto ball_vel = wm.ball().vel();
    auto ball_pos = wm.ball().pos() + ball_vel;
    return Focus_MoveToPoint(ball_pos).execute(agent);
}
bool Bhv_FocusDecision::executeShoot(rcsc::PlayerAgent *agent, Vector2D target)
{
    if (!Setting::i()->mNeck->mExecuteIntercept)
        return false;
    agent->debugClient().addMessage("InterceptFocus");
    const WorldModel &wm = agent->world();
    auto ball_vel = wm.ball().vel();
    auto ball_pos = wm.ball().pos() + ball_vel;
    return Focus_MoveToPoint(ball_pos).execute(agent);
}
bool Bhv_FocusDecision::executeMark(rcsc::PlayerAgent *agent, int unum, Vector2D target)
{
    if (!Setting::i()->mNeck->mExecuteIntercept)
        return false;
    agent->debugClient().addMessage("InterceptFocus");
    const WorldModel &wm = agent->world();
    auto ball_vel = wm.ball().vel();
    auto ball_pos = wm.ball().pos() + ball_vel;
    return Focus_MoveToPoint(ball_pos).execute(agent);
}
bool Bhv_FocusDecision::executeBlock(rcsc::PlayerAgent *agent, Vector2D target)
{
    if (!Setting::i()->mNeck->mExecuteIntercept)
        return false;
    agent->debugClient().addMessage("InterceptFocus");
    const WorldModel &wm = agent->world();
    auto ball_vel = wm.ball().vel();
    auto ball_pos = wm.ball().pos() + ball_vel;
    return Focus_MoveToPoint(ball_pos).execute(agent);
}
bool Bhv_FocusDecision::executeOffMove(rcsc::PlayerAgent *agent)
{
    if (!Setting::i()->mNeck->mExecuteIntercept)
        return false;
    agent->debugClient().addMessage("InterceptFocus");
    const WorldModel &wm = agent->world();
    auto ball_vel = wm.ball().vel();
    auto ball_pos = wm.ball().pos() + ball_vel;
    return Focus_MoveToPoint(ball_pos).execute(agent);
}
bool Bhv_FocusDecision::executeDeffMove(rcsc::PlayerAgent *agent)
{
    if (!Setting::i()->mNeck->mExecuteIntercept)
        return false;
    agent->debugClient().addMessage("InterceptFocus");
    const WorldModel &wm = agent->world();
    auto ball_vel = wm.ball().vel();
    auto ball_pos = wm.ball().pos() + ball_vel;
    return Focus_MoveToPoint(ball_pos).execute(agent);
}

bool Bhv_FocusDecision::executeIntercept(rcsc::PlayerAgent *agent, Vector2D next_target)
{
    if (!Setting::i()->mNeck->mExecuteIntercept)
        return false;
    agent->debugClient().addMessage("InterceptFocus");
    const WorldModel &wm = agent->world();
    auto ball_vel = wm.ball().vel();
    auto ball_pos = wm.ball().pos() + ball_vel;
    return Focus_MoveToPoint(ball_pos).execute(agent);
}

bool Bhv_FocusDecision::executeMove(rcsc::PlayerAgent *agent)
{
    if (!Setting::i()->mNeck->mExecuteIntercept)
        return false;
    agent->debugClient().addMessage("InterceptFocus");
    const WorldModel &wm = agent->world();
    auto ball_vel = wm.ball().vel();
    auto ball_pos = wm.ball().pos() + ball_vel;
    return Focus_MoveToPoint(ball_pos).execute(agent);
}