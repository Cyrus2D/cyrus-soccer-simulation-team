#ifndef BHV_VORONOI_SCAPE
#define BHV_VORONOI_SCAPE

#include <rcsc/game_time.h>
#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_intention.h>
#include <vector>
#include <rcsc/player/world_model.h>
#include <rcsc/player/player_agent.h>
#include "strategy.h"
#include <rcsc/common/logger.h>
#include "basic_actions/body_go_to_point.h"
#include <rcsc/common/server_param.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/abstract_player_object.h>
#include "basic_actions/body_turn_to_angle.h"
#include "basic_actions/neck_turn_to_ball.h"
#include "basic_actions/neck_turn_to_ball_or_scan.h"
#include "chain_action/field_analyzer.h"
#include "bhv_basic_move.h"
#include "basic_actions/body_go_to_point.h"
#include "basic_actions/body_turn_to_point.h"
#include <rcsc/common/audio_memory.h>
#include <rcsc/common/say_message_parser.h>
#include "../neck/neck_decision.h"

using namespace std;
using namespace rcsc;

#include <rcsc/player/soccer_intention.h>


class IntentionScape
        : public rcsc::SoccerIntention {
private:
    const rcsc::Vector2D M_target_point; // global coordinate
    int M_step;
    int M_ex_step;
    int M_last_ex_time;
    int M_start_passer;
    Vector2D M_start_ball_pos;

public:
    IntentionScape( const rcsc::Vector2D & target_point,
                     const int max_step,
                     const int last_ex_time,
                     const int start_passer,
                     const Vector2D start_ball_pos):
        M_target_point(target_point){
        M_step = max_step;
        M_last_ex_time = last_ex_time;
        M_start_ball_pos = start_ball_pos;
        M_start_passer = start_passer;
    }

    bool finished(  const rcsc::PlayerAgent * agent );

    bool execute( rcsc::PlayerAgent * agent );
};

class bhv_scape_voronoi{

public:
    bool execute(PlayerAgent *agent);
    bool can_scape(const WorldModel & wm);
    vector<Vector2D> voronoi_points(rcsc::PlayerAgent *agent);
    static double can_receive_th_pass(const WorldModel & wm, Vector2D target);
    double evaluate_point(rcsc::PlayerAgent *agent, const Vector2D & point,const int & num);
};

#endif

