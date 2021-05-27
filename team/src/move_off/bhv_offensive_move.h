//
// Created by ashkan on 3/30/18.
//

#ifndef CYRUS_CYRUS_OFFENSIVE_MOVE_H
#define CYRUS_CYRUS_OFFENSIVE_MOVE_H

#include <rcsc/player/player_agent.h>
#include "../bhv_basic_move.h"

using namespace rcsc;
using namespace std;

class cyrus_offensive_move {
    Bhv_BasicMove *basicMove;
public:
    bool execute(rcsc::PlayerAgent *agent, Bhv_BasicMove *bhv_basicMove);

private:
    bool BackFromOffside(PlayerAgent *agent);

    bool off_gotopoint(PlayerAgent *agent, Vector2D target, double power, double distthr);

    bool pers_scap(PlayerAgent *agent);

    bool voronoi_unmark(PlayerAgent *agent);

    vector<Vector2D> voronoi_points(rcsc::PlayerAgent *agent);

    double evaluate_point(rcsc::PlayerAgent *agent, Vector2D point);
};


#endif //CYRUS_CYRUS_OFFENSIVE_MOVE_H
