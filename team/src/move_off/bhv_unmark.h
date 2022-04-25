
#ifndef TOKYOTECH_INTENAION_pos_H
#define TOKYOTECH_INTENAION_pos_H

#include <rcsc/game_time.h>
#include <rcsc/geom/vector_2d.h>

#include <rcsc/player/soccer_intention.h>

//unmark 2017

#include <vector>

#include <rcsc/player/world_model.h>
#include <rcsc/player/player_agent.h>
using namespace std;
using namespace rcsc;
class pass_prob {
public:
    double prob = 0.0;
    int pass_sender = 0;
    int pass_getter = 0;

    pass_prob(double prob, int pass_sender, int pass_getter)
            : prob(prob), pass_sender(pass_sender), pass_getter(pass_getter)
    {
    }

    static bool ProbCmp(pass_prob const &a, pass_prob const &b)
    {
        return a.prob < b.prob;
    }
};
class unmark_passer {
public:
	int unum = 0;
	Vector2D ballpos;
	int oppmin_cycle;
	int cycle_recive_ball;
	bool is_fastest;
	unmark_passer();
	unmark_passer(int unum, Vector2D ballpos, int oppmin_cycle,
			int cycle_recive_ball);
};
class unmark_pass_from_me {
public:
	Vector2D pass_pos;
	int unum;
	double pass_eval;
	unmark_pass_from_me(Vector2D pp, int u) {
		pass_pos = pp;
		unum = u;
		pass_eval = 0;
	}
};
class unmark_dribble_from_me {
public:
	Vector2D dribble_pos;
	double dribble_eval;
	unmark_dribble_from_me(Vector2D dp) {
		dribble_pos = dp;
		dribble_eval = 0;
	}
};
//class unmark_shoot_from_me {
//public:
//	Vector2D shoot_pos;
//	double shoot_eval;
//	unmark_shoot_from_me(Vector2D sp) {
//		shoot_pos = sp;
//		shoot_eval = 0;
//	}
//};
class unmark_pass_to_me {
public:
	Vector2D pass_target;
	double pass_eval;
	double dist2self_pos;
	double dist2home_pos;
	double dist2_unmark_target;
	double dist2_opp;
	double dist_target2ball;
	double dist_pass2ball;
	double dist2_tm;
	double min_diff;
	bool can_pass;
	bool can_pass_withoutnear;

    double e_x;
    double e_d_goal;
    double e_d_opp;
    double e_d_target;
    double e_d_hpos;
    double e_d_pos;
    double e_d_tm;
    double e_fastest;
    double e_d_ball;
    double e_pass;
    double e_diff;

	unmark_passer passer;
	vector<unmark_pass_from_me> passes;
	vector<unmark_dribble_from_me> dribbles;
//	vector<unmark_shoot_from_me> shootes;
	void update_eval(const WorldModel & wm);
	bool update(const WorldModel & wm, unmark_passer passer,
			Vector2D unmark_target);
	int opp_min_dist_passer(const WorldModel & wm,Vector2D ball_pos);
	int min_opp_recive(const WorldModel & wm, Vector2D ball_pos, int max,int first_opp = 0);

};

class bhv_unmark {
public:
    int number;
	Vector2D target_pos;
	double target_body;
	double dist2self;
	int cycle_start = 0;
	vector<unmark_passer> passers;
    unmark_passer best_passer;
	vector<unmark_pass_to_me> passes;
    unmark_pass_to_me best_pass;
	double eval_for_unmark;
	double eval_for_pass;
	bool good_for_unmark = false;
	bool good_for_pass = false;
	bool unmark_for_posses;

	bhv_unmark();
	double min_tm_dist(const WorldModel & wm, Vector2D tmp);
	double min_opp_dist(const WorldModel & wm, Vector2D tmp);
	double min_tm_home_dist(const WorldModel & wm, Vector2D tmp);
	bool is_good_for_pass_from_last(const WorldModel & wm);
	bool is_good_for_unmark_from_last(const WorldModel & wm);
	bool is_good_for_pass(const WorldModel & wm);
	bool is_good_for_unmark(const WorldModel & wm);
	Vector2D penalty_unmark(const WorldModel & wm);
//	int eval_for_penalty_unmark(const WorldModel & wm,Vector2D shoot_pos);
	void update_eval_for_unmark();
	void update_eval_for_pass();

	bool execute(PlayerAgent * agent);
	void set_unmark_angle(const WorldModel & wm);
	bool update_for_pass(const WorldModel & wm, vector<unmark_passer> passer);
	bool update_for_unmark(const WorldModel & wm);
    Vector2D get_avoid_circle_point( const WorldModel & wm, const Vector2D & target_point );
    bool
    can_go_to( const int count,
               const WorldModel & wm,
               const Circle2D & ball_circle,
               const Vector2D & target_point );
};
#include <CppDNN/DeepNueralNetwork.h>
class bhv_unmarkes {
public:
	vector<bhv_unmark> unmarks;
	bhv_unmark * best_unmark = NULL;
	static bhv_unmark best_last_unmark; // = bhv_unmark();
	static int last_fastest_tm;
	static int last_run;
	static Vector2D last_ball_inertia;
	double min_tm_dist(const WorldModel & wm, Vector2D tmp);
	double min_opp_dist(const WorldModel & wm, Vector2D tmp);
	bool execute(PlayerAgent * agent);
	bool can_unmark(const WorldModel & wm);
	vector<unmark_passer> update_passer(const WorldModel & wm);
    static DeepNueralNetwork * pass_prediction;
    static void load_dnn();
    vector<unmark_passer> update_passer_dnn_writer(const WorldModel & wm, PlayerAgent * agent);
    vector<unmark_passer> update_passer_dnn(const WorldModel & wm, PlayerAgent * agent);
    vector<pass_prob> predict_pass(vector<double> & features, vector<int> ignored_player, int kicker);
	void create_targets(const WorldModel & wm);
	void evaluate_targets(const WorldModel & wm, vector<unmark_passer> passer);
	void update_best_unmark(const WorldModel & wm);
};
#endif
