/*
 * cyrus_interceptable.h
 *
 *  Created on: Jun 25, 2017
 *      Author: nader
 */

#ifndef SRC_CYRUS_INTERCEPTABLE_H_
#define SRC_CYRUS_INTERCEPTABLE_H_

#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/soccer_intention.h>
#include <iostream>
#include <vector>
using namespace rcsc;
using namespace std;
class CyrusOppInterceptTable {
public:
	int cycle;
	rcsc::Vector2D current_position;
	int turn_cycle;
	double dist_ball;
	CyrusOppInterceptTable(int cycle, rcsc::Vector2D current_position,
			int turn_cycle, double dist_ball);
	/*		cycle(cycle), current_position(current_position) {
	 }*/
};

class CyrusPlayerIntercept {
private:
	//! const reference to the WorldModel instance
	const rcsc::WorldModel & M_world;
	//! const reference to the predicted ball position cache instance
	const std::vector<Vector2D> & M_ball_pos_cache;

	// not used
	CyrusPlayerIntercept();

public:

	CyrusPlayerIntercept(const WorldModel & world,
			const std::vector<Vector2D> & ball_pos_cache) :
				M_world(world), M_ball_pos_cache(ball_pos_cache) {
	}

	/*!
	 \brief destructor. nothing to do
	 */
	~CyrusPlayerIntercept() {
	}

	vector<CyrusOppInterceptTable> predict(const PlayerObject & player,
			const PlayerType & player_type, const int max_cycle) const;
	static CyrusOppInterceptTable getBestIntercept(const WorldModel & wm,
					const vector<CyrusOppInterceptTable> table);
    static vector<Vector2D> createBallCache(const WorldModel & wm);
private:

	bool canReachAfterTurnDash(const int cycle, const PlayerObject & player,
			const PlayerType & player_type, const double & control_area,
			const Vector2D & ball_pos) const;

	int predictTurnCycle(const int cycle, const PlayerObject & player,
			const PlayerType & player_type, const double & control_aera,
			const Vector2D & ball_pos) const;
	bool canReachAfterDash(const int n_turn, const int n_dash,
			const PlayerObject & player, const PlayerType & player_type,
			const double & control_area, const Vector2D & ball_pos) const;
	int predictFinal(const PlayerObject & player,
			const PlayerType & player_type) const;

};

#endif /* SRC_CYRUS_INTERCEPTABLE_H_ */
