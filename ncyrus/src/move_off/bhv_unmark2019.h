
#ifndef TOKYOTECH_INTENAION_pos_H_2019
#define TOKYOTECH_INTENAION_pos_H_2019

#include <rcsc/game_time.h>
#include <rcsc/geom/vector_2d.h>

#include <rcsc/player/soccer_intention.h>

//unmark 2017

#include <vector>

#include <rcsc/player/world_model.h>
#include <rcsc/player/player_agent.h>
using namespace std;
using namespace rcsc;
class unmark_passer_2019 {
public:
	int unum;
	Vector2D ballpos;
	int oppmin_cycle;
	int cycle_recive_ball;
    unmark_passer_2019();
    unmark_passer_2019(int unum, Vector2D ballpos, int oppmin_cycle,
			int cycle_recive_ball);
};
class unmark_pass_from_me_2019 {
public:
	Vector2D pass_pos;
	int unum;
	double pass_eval;
    unmark_pass_from_me_2019(Vector2D pp, int u) {
		pass_pos = pp;
		unum = u;
		pass_eval = 0;
	}
};
class unmark_dribble_from_me_2019 {
public:
	Vector2D dribble_pos;
	double dribble_eval;
    unmark_dribble_from_me_2019(Vector2D dp) {
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
class unmark_pass_to_me_2019 {
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
	bool can_pass;
	bool can_pass_withoutnear;
	vector<unmark_pass_from_me_2019> passes;
	vector<unmark_dribble_from_me_2019> dribbles;
//	vector<unmark_shoot_from_me> shootes;
	void update_eval();
	bool update(const WorldModel & wm, unmark_passer_2019 passer,
			Vector2D unmark_target);
	int opp_min_dist_passer(const WorldModel & wm,Vector2D ball_pos);
	int min_opp_recive(const WorldModel & wm, Vector2D ball_pos, int max,int first_opp = 0);

};

class bhv_unmark_2019 {
public:
	Vector2D target_pos;
	double target_body;
	double dist2self;
	int cycle_start;
	unmark_passer_2019 passer;
	vector<unmark_pass_to_me_2019> passes;
	double eval_for_unmark;
	double eval_for_pass;
	bool unmark_for_posses;

    bhv_unmark_2019();
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
	bool update_for_pass(const WorldModel & wm, unmark_passer_2019 passer);
	bool update_for_unmark(const WorldModel & wm, unmark_passer_2019 passer);
};

class bhv_unmarkes_2019 {
public:
	vector<bhv_unmark_2019> unmarks;
	bhv_unmark_2019 * best_unmark;
	static bhv_unmark_2019 best_last_unmark; // = bhv_unmark();
	double min_tm_dist(const WorldModel & wm, Vector2D tmp);
	double min_opp_dist(const WorldModel & wm, Vector2D tmp);
	bool execute(PlayerAgent * agent);
	bool can_unmark(const WorldModel & wm);
	unmark_passer_2019 update_passer(const WorldModel & wm);
	void create_unmarks(const WorldModel & wm, unmark_passer_2019 passer);
//	void create_unmarks
	void update_best_unmark();
};
#endif
