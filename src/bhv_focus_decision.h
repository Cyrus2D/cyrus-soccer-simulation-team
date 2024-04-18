#ifndef FOCUS_DECISION_H
#define FOCUS_DECISION_H

#include <rcsc/player/soccer_action.h>
#include <rcsc/geom/vector_2d.h>
using namespace rcsc;
class Bhv_FocusDecision {
public:
    bool executeReset( rcsc::PlayerAgent * agent );
    bool executeSample( rcsc::PlayerAgent * agent, Vector2D target);
    bool executePass( rcsc::PlayerAgent * agent, int unum, Vector2D target );
    bool executeDribble( rcsc::PlayerAgent * agent, Vector2D target );
    bool executeShoot( rcsc::PlayerAgent * agent, Vector2D target );
    bool executeMark( rcsc::PlayerAgent * agent, int unum, Vector2D target );
    bool executeBlock( rcsc::PlayerAgent * agent, Vector2D target );
    bool executeOffMove( rcsc::PlayerAgent * agent );
    bool executeMove( rcsc::PlayerAgent * agent );
    bool executeDeffMove( rcsc::PlayerAgent * agent );
    bool executeIntercept( rcsc::PlayerAgent * agent, Vector2D next_target);
};

#endif