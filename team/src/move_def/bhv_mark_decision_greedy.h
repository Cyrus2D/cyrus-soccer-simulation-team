/*
 * bhvmarkdecitiongreedy.h
 *
 *  Created on: Apr 12, 2017
 *      Author: nader
 */

#ifndef SRC_bhv_mark_decision_greedy_H_
#define SRC_bhv_mark_decision_greedy_H_

#include <rcsc/game_time.h>
#include <rcsc/geom/vector_2d.h>

#include <rcsc/player/soccer_intention.h>
#include <vector>
#include "strategy.h"
#include "bhv_block.h"
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/world_model.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/say_message_builder.h>
#include <rcsc/player/freeform_parser.h>
#include <rcsc/common/audio_memory.h>
#include <rcsc/common/say_message_parser.h>

using namespace std;
using namespace rcsc;

enum class MarkType {
    NoType = 0,
    LeadProjectionMark = 1,
    LeadNearMark = 2,
    ThMark = 3,
    DangerMark = 4,
    Block = 5,
    Goal_keep = 6
};
std::string markTypeString(MarkType markType);

enum class MarkDec {
    NoDec = 0,
    AntiDef = 1,
    MidMark = 2,
    GoalMark = 3,
    JustBlock = 4
};
struct Target {
    Target() {}

    Target(const Vector2D &_pos, const AngleDeg &_th) : pos(_pos), th(_th) {}

    Vector2D pos;
    AngleDeg th = 1000;
};
typedef pair<size_t, double> UnumEval;
class BhvMarkDecisionGreedy {
public:
    struct MarkAction {
        size_t opp;
        MarkType type;
    };

    static bool use_home_pos;
    static std::pair<long, int> last_mark;

    BhvMarkDecisionGreedy();

    virtual ~BhvMarkDecisionGreedy();

    void getMarkTargets(PlayerAgent *agent, MarkType &mark_type, int &mark_unum, bool &blocked);

    static MarkDec markDecision(const WorldModel &wm);

    static void midMarkDecision(PlayerAgent *agent, MarkType &mark_type, int &mark_unum, bool &blocked);

    static void midMarkThMarkCostFinder(const WorldModel &wm, double mark_eval[][12], bool used_hpos, vector<double> block_eval, bool fastest_opp_marked, Target opp_targets [], bool on_anti_offense);

    static void midMarkLeadMarkOffensiveCostFinder(const WorldModel &wm, double mark_eval[][12], bool used_hpos, vector<double> block_eval, bool fastest_opp_marked);

    static void midMarkLeadMarkCostFinder(const WorldModel &wm, double mark_eval[][12], bool used_hpos, vector<double> block_eval, bool fastest_opp_marked);

    static bool canCenterHalfMarkLeadNear(const WorldModel &wm, int t, Vector2D opp_pos, Vector2D ball_inertia);
    static void goalMarkLeadMarkCostFinder(const WorldModel &wm, double mark_eval[][12], vector<int> who_go_to_goal);

    static vector<UnumEval> oppEvaluatorMidMark(const WorldModel &wm, bool use_ball_dist=false);

    static vector<UnumEval> oppEvaluatorGoalMark(const WorldModel &wm);

    static vector<int> getOppOffensiveStatic(const WorldModel & wm);

    static vector<size_t> getOppOffensive(const WorldModel & wm, bool &fastest_opp_marked);

    static void goalMarkDecision(PlayerAgent *agent, MarkType &mark_type, int &mark_unum, bool &blocked);

    static void antiDefMarkDecision(const WorldModel &wm, MarkType &mark_type, int &mark_unum, bool &blocked);

    vector<int> whoCanPassToOpp(const WorldModel &wm, int o);

    static int whoBlocker(const WorldModel &wm);

    double cycleBlockSoft(const WorldModel &wm, Vector2D blocker, Vector2D opp);

    double getDir(const WorldModel &wm, rcsc::Vector2D start_pos);

};

#endif /* SRC_bhv_mark_decision_greedy_H_ */
