/*
 * bhv_block.h
 *
 *  Created on: Jun 25, 2017
 *      Author: nader
 */

#ifndef SRC_BHV_BLOCK_H_
#define SRC_BHV_BLOCK_H_

#include <rcsc/game_time.h>
#include <rcsc/geom/vector_2d.h>

#include <rcsc/player/soccer_intention.h>

//unmark 2017

#include <vector>

#include <rcsc/player/world_model.h>
#include <rcsc/player/player_agent.h>
using namespace std;
using namespace rcsc;

class bhv_block{
public:
    bool do_tackle_block(PlayerAgent *agent);
    double calc_tackle_prob(Vector2D ballPos, Vector2D selfPos, AngleDeg selfBody);
	bool execute(PlayerAgent * agent);
    static int who_is_blocker(const WorldModel & wm,vector<int> marker=vector<int>());
    static vector<double> blocker_eval(const WorldModel & wm);
    static vector<double> blocker_eval_mark_decision(const WorldModel & wm);
    static Vector2D get_block_center_pos( const WorldModel & wm );
    static double get_dribble_angle(const WorldModel & wm, Vector2D start_dribble);
    static void get_start_dribble(const WorldModel & wm, Vector2D & start_dribble, int & cycle);
    static void block_cycle(const WorldModel & wm,int unum, int & cycle, Vector2D & target,bool debug = false);
    static bool do_block_pass(PlayerAgent * agent);
};

#ifndef INTENAION_BLOCK_H
#define INTENAION_BLOCK_H

#include <rcsc/game_time.h>
#include <rcsc/geom/vector_2d.h>

#include <rcsc/player/soccer_intention.h>
#include "bhv_mark_decision_greedy.h"
#include "bhv_mark_execute.h"

#include "strategy.h"
#include "bhv_basic_tackle.h"
#include <rcsc/player/intercept_table.h>
#include <vector>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_turn_to_point.h>
#include <rcsc/action/body_turn_to_point.h>
#include <rcsc/action/body_turn_to_angle.h>
#include <rcsc/common/audio_memory.h>
#include <rcsc/player/world_model.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/common/say_message_parser.h>
#include <rcsc/player/say_message_builder.h>
#include <rcsc/action/neck_turn_to_ball_and_player.h>
class IntentionBlock
        : public rcsc::SoccerIntention {
private:
    int opp_unum;
    Vector2D first_opp_pos;
    Vector2D first_target_pos;
    Vector2D first_ball_pos;
    Vector2D first_start_pos;
//    rcsc::Vector2D M_target_point; // global coordinate
    int max_step;

    rcsc::GameTime start_time;

public:
    IntentionBlock(const int _opp_unum,
                  const Vector2D _first_opp_pos,
                  const Vector2D _first_target_pos,
                  const Vector2D _first_ball_pos,
                  const Vector2D _first_start_pos,
                  const int _max_step,
                  const rcsc::GameTime _start_time
                  ){
             opp_unum = _opp_unum;
             first_opp_pos = _first_opp_pos;
             first_target_pos = _first_target_pos;
             first_ball_pos = _first_ball_pos;
             first_start_pos = _first_start_pos;
             max_step = _max_step;
             start_time = _start_time;
    }

    bool finished( rcsc::PlayerAgent * agent ){
        return true;
        const WorldModel & wm = agent->world();
        int self_unum = wm.self().unum();
        int opp_reach_cycle = wm.interceptTable()->opponentReachCycle();
        int tm_reach_cycle = wm.interceptTable()->teammateReachCycle();
        int self_reach_cycle = wm.interceptTable()->selfReachCycle();
        int fastest_opp = wm.interceptTable()->fastestOpponent()->unum();
        Vector2D start_pos; int start_cycle;
        bhv_block::get_start_dribble(wm,start_pos,start_cycle);
        Vector2D target_pos;int target_cycle;
        bhv_block::block_cycle(wm,self_unum,target_cycle,target_pos);

        if(wm.theirPlayer(opp_unum)==NULL
                || wm.theirPlayer(opp_unum)->unum() < 1)
            return true;
        if (fastest_opp != opp_unum)
            return true;
        if(wm.time().cycle() > start_time.cycle() + max_step)
            return true;
        if(self_reach_cycle < opp_reach_cycle || tm_reach_cycle <= opp_reach_cycle)
            return true;
        double start_pos_dist = 3;
        if(target_cycle <= 5)
            start_pos_dist = 1.5;
        if(target_cycle <= 3)
            start_pos_dist = 0.5;
        if(start_pos.dist(first_start_pos) > start_pos_dist)
            return true;
        double target_pos_dist = 3;
        if(target_cycle <= 5)
            target_pos_dist = 1.5;
        if(target_cycle <= 3)
            target_pos_dist = 0.5;
        if(target_pos.dist(first_target_pos) > target_pos_dist)
            return true;

        return false;
    }

    bool execute( rcsc::PlayerAgent * agent ){
        const WorldModel & wm = agent->world();
        if ( Bhv_BasicTackle(0.8).execute(agent) )
            return true;
        agent->doPointto(first_target_pos.x,first_target_pos.y);
        agent->debugClient().addMessage("blockk");
        if ( ! Body_GoToPoint( first_target_pos, 0.5, 100
                               ).execute( agent ) )
        {
            Body_TurnToPoint(first_target_pos).execute( agent );
        }

        agent->setNeckAction(
                    new Neck_TurnToBallAndPlayer(
                        wm.theirPlayer(opp_unum), 1));
        return true;
    }
};

#endif
#endif
