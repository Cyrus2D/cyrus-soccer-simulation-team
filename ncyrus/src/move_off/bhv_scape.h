/*
 * bhv_scape.h
 *
 *  Created on: Jun 25, 2017
 *      Author: nader
 */


#ifndef SCAPE_H
#define SCAPE_H


#include <rcsc/player/player_agent.h>
#include <rcsc/geom/vector_2d.h>

#include <vector>
using namespace rcsc;
using namespace std;
class scape{
public:
	Vector2D M_first_target;
	vector<Vector2D> M_last_targets;
	int M_start_cycle;
	double M_offside;
	Vector2D M_ball_pos;
	int M_passer;
	double eval;
	scape(Vector2D first_target,int start_cycle,double offside,Vector2D ball_pos,int passer){
		M_first_target = first_target;
//				M_last_targets = last_taget;
		M_start_cycle = start_cycle;
		M_offside = offside;
		M_ball_pos = ball_pos;
		M_passer = passer;
	}
	scape(){

	}
};
#include"strategy.h"
class bhv_scape {
public:
    static bool can_scape(const WorldModel & wm);
	bool run_last_scape(PlayerAgent * agent);
    bool is_near_to_tm_home_pos(const WorldModel & wm,Vector2D first_taret){
        Vector2D self_home = Strategy::i().getPosition(wm.self().unum());
        for(int i=9;i<=11;i++){
            if(wm.self().unum() == i)
                continue;
            Vector2D tm_home = Strategy::i().getPosition(i);
            if(tm_home.dist(first_taret) < self_home.dist(first_taret))
                return true;
        }
        return false;
    }

	bool execute(PlayerAgent * agent);
	static scape last_new_scape;
};
#endif
