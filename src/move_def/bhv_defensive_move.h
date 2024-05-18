// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////
#ifndef BHV_DEFENSIVE_MOVE_H
#define BHV_DEFENSIVE_MOVE_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/soccer_intention.h>
#include <rcsc/player/intercept_table.h>
#include <iostream>
#include <vector>

using namespace rcsc;
using namespace std;

#include "move_def/cyrus_interceptable.h"
class Bhv_DefensiveMove: public rcsc::SoccerBehavior {
public:
    Bhv_DefensiveMove() = default;
    bool execute(rcsc::PlayerAgent * agent) override;
    static bool setDefNeckWithBall(PlayerAgent * agent,
                                   Vector2D targetPoint,
                                   const AbstractPlayerObject* opp,
                                   int blocker);
};

#endif

