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
    if (!Setting::i()->mNeck->mExecutePass)
        return false;

    const WorldModel &wm = agent->world();
    if (wm.ball().velCount() > 1)
    {
        agent->debugClient().addMessage("PassFocusA");
        agent->setFocusAction(new Focus_MoveToPoint(wm.ball().pos()));
    }
    else
    {
        agent->debugClient().addMessage("PassFocusB");
        agent->setFocusAction(new Focus_MoveToPoint(target));
    }
    return true;
}
bool Bhv_FocusDecision::executeDribble(rcsc::PlayerAgent *agent, Vector2D target)
{
    if (!Setting::i()->mNeck->mExecuteDribble)
        return false;

    const WorldModel &wm = agent->world();
    if (wm.ball().velCount() > 1)
    {
        agent->debugClient().addMessage("DribbleFocusA");
        agent->setFocusAction(new Focus_MoveToPoint(wm.ball().pos()));
    }
    else
    {
        agent->debugClient().addMessage("DribbleFocusB");
        agent->setFocusAction(new Focus_MoveToPoint(target));
    }
    return true;
}
bool Bhv_FocusDecision::executeShoot(rcsc::PlayerAgent *agent, Vector2D target)
{
    if (!Setting::i()->mNeck->mExecuteShoot)
        return false;

    const WorldModel &wm = agent->world();
    if (wm.ball().velCount() > 1)
    {
        agent->debugClient().addMessage("ShootFocusA");
        agent->setFocusAction(new Focus_MoveToPoint(wm.ball().pos()));
    }
    else
    {
        agent->debugClient().addMessage("ShootFocusB");
        agent->setFocusAction(new Focus_MoveToPoint(target));
    }
    return true;
}
bool Bhv_FocusDecision::executeMark(rcsc::PlayerAgent *agent, int unum, Vector2D target)
{
    if (!Setting::i()->mNeck->mExecuteMark)
        return false;

    const WorldModel &wm = agent->world();
    if (wm.ball().velCount() > 2)
    {
        agent->debugClient().addMessage("MarkFocusA");
        agent->setFocusAction(new Focus_MoveToPoint(wm.ball().pos()));
    }
    else
    {
        agent->debugClient().addMessage("MarkFocusB");
        agent->setFocusAction(new Focus_MoveToPoint(target));
    }
    return true;
}
bool Bhv_FocusDecision::executeBlock(rcsc::PlayerAgent *agent, Vector2D target)
{
    if (!Setting::i()->mNeck->mExecuteBlock)
        return false;

    const WorldModel &wm = agent->world();
    if (wm.ball().velCount() > 1)
    {
        agent->debugClient().addMessage("BlockFocusA");
        agent->setFocusAction(new Focus_MoveToPoint(wm.ball().pos()));
    }
    else
    {
        agent->debugClient().addMessage("BlockFocusB");
        agent->setFocusAction(new Focus_MoveToPoint(target));
    }
    return true;
}
bool Bhv_FocusDecision::executeOffMove(rcsc::PlayerAgent *agent)
{
    if (!Setting::i()->mNeck->mExecuteOffMove)
        return false;
    agent->debugClient().addMessage("OffMoveFocus");
    const WorldModel &wm = agent->world();
    agent->setFocusAction(new Focus_MoveToPoint(wm.ball().pos()));
    return true;
}
bool Bhv_FocusDecision::executeDeffMove(rcsc::PlayerAgent *agent)
{
    if (!Setting::i()->mNeck->mExecuteDeffMove)
        return false;
    agent->debugClient().addMessage("DeffMoveFocus");
    const WorldModel &wm = agent->world();
    agent->setFocusAction(new Focus_MoveToPoint(wm.ball().pos()));
    return true;
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
    if (!Setting::i()->mNeck->mExecuteMove)
        return false;
    agent->debugClient().addMessage("MoveFocus");
    const WorldModel &wm = agent->world();
    auto ball_pos = wm.ball().pos();
    return Focus_MoveToPoint(ball_pos).execute(agent);
}