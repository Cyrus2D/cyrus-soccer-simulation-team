//
// Created by nader on 4/30/24.
//

#ifndef CYRUS_BASIC_DEFENSIVE_MODE_H
#define CYRUS_BASIC_DEFENSIVE_MODE_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/soccer_intention.h>
#include <rcsc/player/intercept_table.h>
#include <iostream>
#include <vector>

#include "cyrus_interceptable.h"

using namespace rcsc;
using namespace std;

class Bhv_DefensiveMove : public rcsc::SoccerBehavior {
public:
    Bhv_DefensiveMove() = default;

    bool execute(rcsc::PlayerAgent *agent) override;

    static bool
    set_def_neck_with_ball(PlayerAgent *agent,
                           Vector2D targetPoint,
                           const AbstractPlayerObject *opp,
                           int blocker);
};

#endif //CYRUS_BASIC_DEFENSIVE_MODE_H
