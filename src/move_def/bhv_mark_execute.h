/*
 * bhv_mark_execute.h
 *
 *  Created on: Apr 12, 2017
 *      Author: nader
 */

#ifndef SRC_BHV_MARK_EXECUTE_H_
#define SRC_BHV_MARK_EXECUTE_H_

#include <rcsc/game_time.h>
#include <rcsc/geom/vector_2d.h>

#include <rcsc/player/soccer_intention.h>
#include "bhv_mark_decision_greedy.h"
#include "bhv_block.h"
#include "strategy.h"
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

using namespace std;
using namespace rcsc;

struct Target;
enum class MarkType;
class bhv_mark_execute {
public:
    bhv_mark_execute();

    virtual ~bhv_mark_execute();

    bool execute(PlayerAgent *agent);

    bool run_mark(PlayerAgent *agent, int mark_unum, MarkType markType);

    void set_mark_target_thr(const WorldModel & wm,
                             const AbstractPlayerObject * opp,
                             MarkType mark_type,
                             Target & target,
                             double & dist_thr);

    bool do_move_mark(PlayerAgent *agent, Target target, double dist_thr, MarkType mark_type, int opp_unum);

    double th_mark_power(PlayerAgent * agent, Vector2D opp_pos, Vector2D target_pos);

    void th_mark_move(PlayerAgent * agent, Target targ, double dash_power, double dist_thr, int opp_unum);

    double lead_mark_power(PlayerAgent * agent, Vector2D opp_pos, Vector2D target_pos);

    void lead_mark_move(PlayerAgent * agent, Target targ, double dash_power, double dist_thr, MarkType mark_type, Vector2D opp_pos);

    double other_mark_power(PlayerAgent * agent, Vector2D opp_pos, Vector2D target_pos);

    void other_mark_move(PlayerAgent * agent, Target targ, double dash_power, double dist_thr);

    bool do_tackle(PlayerAgent * agent);

    bool back_to_def(PlayerAgent *agent);

    bool defenseBeInBack(PlayerAgent *agent);

    bool defenseGoBack(PlayerAgent *agent);

    Vector2D change_position_set_play(const WorldModel &wm, Vector2D target);
};

#endif /* SRC_BHV_MARK_EXECUTE_H_ */
