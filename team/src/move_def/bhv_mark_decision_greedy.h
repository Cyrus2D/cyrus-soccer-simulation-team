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
    ThMarkFastestOpp = 4,
    ThMarkFar = 5,
    DangerMark = 6,
    Block = 7,
    Goal_keep = 8
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

    static vector <size_t> getOppOffensive(const WorldModel &wm, bool &fastest_opp_marked);

    static vector<int> getOppOffensiveStatic(const WorldModel &wm);

    static void midMarkDecision(PlayerAgent *agent, MarkType &mark_type, int &mark_unum, bool &blocked);

    static bool isAntiOffensive(const WorldModel & wm);

    static vector <UnumEval> oppEvaluatorMidMark(const WorldModel &wm, bool use_ball_dist = false);

    static void
    midMarkThMarkCostFinder(const WorldModel &wm, double mark_eval[][12], bool used_hpos, vector<double> block_eval,
                            bool fastest_opp_marked, Target opp_targets[], bool on_anti_offense);

    static vector <size_t> midMarkThMarkMarkerFinder(double mark_eval[][12], size_t fastest_opp);

    static vector <size_t> midMarkThMarkMarkedFinder(vector <size_t> &offensive_opps, vector <UnumEval> &opp_eval);

    static vector <size_t>
    midMarkThMarkRemoveCloseOpp(const WorldModel &wm, vector <size_t> &temp_opps, Target opp_targets[]);

    static void
    midMarkThMarkSetResults(const WorldModel &wm, pair<vector<int>, double> &action, vector <size_t> &temp_opps,
                            MarkType how_mark[], size_t tm_mark_target[], size_t opp_marker[], size_t opp_mark_count[],
                            size_t fastest_opp, bool fastest_opp_marked, Target opp_targets[]);

    static void
    midMarkLeadMarkCostFinder(const WorldModel &wm, double mark_eval[][12], bool used_hpos, vector<double> block_eval,
                              bool fastest_opp_marked);

    static vector <size_t>
    midMarkLeadMarkMarkerFinder(double mark_eval[][12], size_t fastest_opp, size_t tm_mark_target[12]);

    static vector <size_t>
    midMarkLeadMarkMarkedFinder(const WorldModel &wm, vector <size_t> &offensive_opps, vector <UnumEval> &opp_eval,
                                size_t fastest_opp, Vector2D &ball_inertia, size_t opp_marker[], MarkType how_mark[]);

    static void midMarkLeadMarkSetResults(const WorldModel &wm, pair<vector<int>, double> &action,
                                          vector <size_t> &temp_opps, MarkType how_mark[],
                                          size_t tm_mark_target[], size_t opp_marker[],
                                          size_t opp_mark_count[], size_t fastest_opp);

    static bool needProjectMark(const WorldModel &wm, int opp_unum, int tm_unum);

    static bool canCenterHalfMarkLeadNear(const WorldModel &wm, int t, Vector2D opp_pos, Vector2D ball_inertia);

    static void goalMarkDecision(PlayerAgent *agent, MarkType &mark_type, int &mark_unum, bool &blocked);

    static vector <UnumEval> oppEvaluatorGoalMark(const WorldModel &wm);

    static void goalMarkLeadMarkCostFinder(const WorldModel &wm, double mark_eval[][12], vector<int> who_go_to_goal);

    static void antiDefMarkDecision(const WorldModel &wm, MarkType &mark_type, int &mark_unum, bool &blocked);

    vector<int> whoCanPassToOpp(const WorldModel &wm, int o);

    static int whoBlocker(const WorldModel &wm);

    double cycleBlockSoft(const WorldModel &wm, Vector2D blocker, Vector2D opp);

    double getDir(const WorldModel &wm, rcsc::Vector2D start_pos);

};

#endif /* SRC_bhv_mark_decision_greedy_H_ */
