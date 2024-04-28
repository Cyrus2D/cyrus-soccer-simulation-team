/*
 * cyrus_interceptable.cpp
 *
 *  Created on: Jun 25, 2017
 *      Author: nader
 */

#include "cyrus_interceptable.h"
#include <rcsc/common/server_param.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/common/logger.h>
#include "basic_actions/body_intercept2009.h"
CyrusOppInterceptTable::CyrusOppInterceptTable(int cycle,
		rcsc::Vector2D current_position, int turn_cycle, double dist_ball) {
	this->cycle = cycle;
	this->current_position = current_position;
	this->turn_cycle = turn_cycle;
	this->dist_ball = dist_ball;
}


vector<CyrusOppInterceptTable> CyrusPlayerIntercept::predict(
		const PlayerObject & player, const PlayerType & player_type,
		const int max_cycle) const {
	vector<CyrusOppInterceptTable> res;
	const double penalty_x_abs = ServerParam::i().pitchHalfLength()
																	- ServerParam::i().penaltyAreaLength();
	const double penalty_y_abs = ServerParam::i().penaltyAreaHalfWidth();

	const int pos_count = std::min(player.seenPosCount(), player.posCount());
	const Vector2D & player_pos = (
			player.seenPosCount() <= player.posCount() ?
					player.seenPos() : player.pos());
	int min_cycle = 0;
	{
		Vector2D ball_to_player = player_pos - M_world.ball().pos();
		ball_to_player.rotate(-M_world.ball().vel().th());
		min_cycle = static_cast<int>(std::floor(
				ball_to_player.absY() / player_type.realSpeedMax()));
	}

	if (player.isTackling()) {
		min_cycle += std::max(0,
				ServerParam::i().tackleCycles() - player.tackleCount() - 2);
	}

	min_cycle = std::max(0,
			min_cycle - std::min(player.seenPosCount(), player.posCount()));

#ifdef DEBUG
	dlog.addText( Logger::INTERCEPT,
			"Intercept Player %d %d (%.1f %.1f)---- min_cycle=%d max_cycle=%d",
			player.side(),
			player.unum(),
			player.pos().x, player.pos().y,
			min_cycle, max_cycle );
#endif
	if (min_cycle > max_cycle) {
		return res;
	}

	const std::size_t MAX_LOOP = std::min(static_cast<std::size_t>(max_cycle),
			M_ball_pos_cache.size());

	for (std::size_t cycle = static_cast<std::size_t>(min_cycle);
			cycle < MAX_LOOP; ++cycle) {
		const Vector2D & ball_pos = M_ball_pos_cache.at(cycle);
#ifdef DEBUG2
		dlog.addText( Logger::INTERCEPT,
				"*** cycle=%d  ball(%.2f %.2f)",
				cycle, ball_pos.x, ball_pos.y );
#endif
		const double control_area = (
				(player.goalie() && ball_pos.absX() > penalty_x_abs
						&& ball_pos.absY() < penalty_y_abs) ?
								ServerParam::i().catchableArea() :
								player_type.kickableArea());

		if (control_area + player_type.realSpeedMax() * (cycle + pos_count)
				+ 0.5 < player_pos.dist(ball_pos)) {
			// never reach
#ifdef DEBUG2
			dlog.addText( Logger::INTERCEPT,
					"--->cycle=%d  never reach! ball(%.2f %.2f)",
					cycle, ball_pos.x, ball_pos.y );
#endif
			continue;
		}

		if (canReachAfterTurnDash(cycle, player, player_type, control_area,
				ball_pos)) {
#ifdef DEBUG
			dlog.addText( Logger::INTERCEPT,
					"--->cycle=%d  Sucess! ball(%.2f %.2f)",
					cycle, ball_pos.x, ball_pos.y );
#endif
			int n_turn = predictTurnCycle(cycle, player, player_type,
					control_area, ball_pos);
			double dist_ball = (player.pos()
					+ (player.pos().polar2vector(cycle,
							(ball_pos - player.pos()).th()))).dist(ball_pos);
			CyrusOppInterceptTable tmp(cycle, ball_pos, n_turn, dist_ball);
			res.push_back(tmp);
		}
	}

	return res;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool CyrusPlayerIntercept::canReachAfterTurnDash(const int cycle,
		const PlayerObject & player, const PlayerType & player_type,
		const double & control_area, const Vector2D & ball_pos) const {
	int n_turn = predictTurnCycle(cycle, player, player_type, control_area,
			ball_pos);
#ifdef DEBUG2
	dlog.addText( Logger::INTERCEPT,
			"______ loop %d  turn step = %d",
			cycle, n_turn );
#endif

	int n_dash = cycle - n_turn;
	if (n_dash < 0) {
		return false;
	}

	return canReachAfterDash(n_turn, n_dash, player, player_type, control_area,
			ball_pos);
}

/*-------------------------------------------------------------------*/
/*!

 */
int CyrusPlayerIntercept::predictTurnCycle(const int cycle,
		const PlayerObject & player, const PlayerType & player_type,
		const double & control_area, const Vector2D & ball_pos) const {
	//     if ( player.bodyCount() > std::min( player.seenPosCount(), player.posCount() ) )
	//     {
	//         return 0;
	//     }

	const Vector2D & ppos = (
			player.seenPosCount() <= player.posCount() ?
					player.seenPos() : player.pos());
	const Vector2D & pvel = (
			player.seenVelCount() <= player.velCount() ?
					player.seenVel() : player.vel());

	Vector2D inertia_pos = player_type.inertiaPoint(ppos, pvel, cycle);
	Vector2D target_rel = ball_pos - inertia_pos;
	double target_dist = target_rel.r();
	double turn_margin = 180.0;
	if (control_area < target_dist) {
		turn_margin = AngleDeg::asin_deg(control_area / target_dist);
	}
	turn_margin = std::max(turn_margin, 12.0);

	double angle_diff = (target_rel.th() - player.body()).abs();

	if (target_dist < 5.0 // XXX magic number XXX
			&& angle_diff > 90.0) {
		// assume back dash
		angle_diff = 180.0 - angle_diff;
	}

	int n_turn = 0;

	double speed = player.vel().r();
	if (angle_diff > turn_margin) {
		double max_turn = player_type.effectiveTurn(
				ServerParam::i().maxMoment(), speed);
		angle_diff -= max_turn;
		speed *= player_type.playerDecay();
		++n_turn;
	}

	//return bound( 0, n_turn - player.bodyCount(), 5 );
	return n_turn;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool CyrusPlayerIntercept::canReachAfterDash(const int n_turn,
		const int max_dash, const PlayerObject & player,
		const PlayerType & player_type, const double & control_area,
		const Vector2D & ball_pos) const {
	const int pos_count = std::min(player.seenPosCount(), player.posCount());
	const Vector2D & ppos = (
			player.seenPosCount() <= player.posCount() ?
					player.seenPos() : player.pos());
	const Vector2D & pvel = (
			player.seenVelCount() <= player.velCount() ?
					player.seenVel() : player.vel());

	Vector2D player_pos = inertia_n_step_point(ppos, pvel, n_turn + max_dash,
			player_type.playerDecay());

	Vector2D player_to_ball = ball_pos - player_pos;
	double player_to_ball_dist = player_to_ball.r();
	player_to_ball_dist -= control_area;

	if (player_to_ball_dist < 0.0) {
#ifdef DEBUG
		dlog.addText( Logger::INTERCEPT,
				"______ %d (%.1f %.1f) can reach(1). turn=%d dash=0",
				player.unum(),
				player.pos().x, player.pos().y,
				n_turn );
#endif
		return true;
	}

	int estimate_dash = player_type.cyclesToReachDistance(player_to_ball_dist);
	int n_dash = estimate_dash;
	if (player.side() != M_world.ourSide()) {
		n_dash -= bound(0, pos_count - n_turn,
				std::min(6, M_world.ball().seenPosCount() + 1));
	} else {
		//n_dash -= bound( 0, pos_count - n_turn, 1 );
		n_dash -= bound(0, pos_count - n_turn,
				std::min(1, M_world.ball().seenPosCount()));
	}

	if (player.isTackling()) {
		n_dash += std::max(0,
				ServerParam::i().tackleCycles() - player.tackleCount() - 2);
	}

	if (n_dash <= max_dash) {
#ifdef DEBUG
		dlog.addText( Logger::INTERCEPT,
				"______ %d (%.1f %.1f) can reach(2). turn=%d dash=%d(%d) dist=%.3f",
				player.unum(),
				player.pos().x, player.pos().y,
				n_turn, n_dash, estimate_dash,
				player_to_ball_dist );
#endif
		return true;
	}

	return false;
}
CyrusOppInterceptTable CyrusPlayerIntercept::getBestIntercept(const WorldModel & wm,
		const vector<CyrusOppInterceptTable> table) {
	const ServerParam & SP = ServerParam::i();
	const std::vector<CyrusOppInterceptTable> cache = table;

	if (cache.empty()) {
		CyrusOppInterceptTable tmp = CyrusOppInterceptTable(1000,
				Vector2D::INVALIDATED, 1000, 1000);
		return tmp;
	}

#ifdef DEBUG_PRINT
	dlog.addText(Logger::INTERCEPT, "==ashkam=== getBestIntercept =====");
#endif

	const Vector2D goal_pos(-65.0, 0.0);
	const double max_pitch_x =
			(SP.keepawayMode() ?
					SP.keepawayLength() * 0.5 - 1.0 : SP.pitchHalfLength() - 1.0);
	const double max_pitch_y = (
			SP.keepawayMode() ?
					SP.keepawayWidth() * 0.5 - 1.0 : SP.pitchHalfWidth() - 1.0);
	const double penalty_x = -SP.ourPenaltyAreaLineX();
	const double penalty_y = SP.penaltyAreaHalfWidth();
	const PlayerObject opp_ball = *(wm.interceptTable().firstOpponent());
	const double speed_max = opp_ball.playerTypePtr()->realSpeedMax() * 0.9;
	const int opp_min = min(wm.interceptTable().teammateStep(),
			wm.interceptTable().selfStep());
	//const PlayerObject * fastest_opponent = table.firstOpponent();

	const CyrusOppInterceptTable * attacker_best =
			static_cast<CyrusOppInterceptTable *>(0);
	double attacker_score = 0.0;

	const CyrusOppInterceptTable * forward_best =
			static_cast<CyrusOppInterceptTable *>(0);
	double forward_score = 0.0;

	const CyrusOppInterceptTable * noturn_best =
			static_cast<CyrusOppInterceptTable *>(0);
	double noturn_score = 10000.0;

	const CyrusOppInterceptTable * nearest_best =
			static_cast<CyrusOppInterceptTable *>(0);
	double nearest_score = 10000.0;

#ifdef USE_GOALIE_MODE
	const InterceptInfo * goalie_best = static_cast< InterceptInfo * >( 0 );
	double goalie_score = -10000.0;

	const InterceptInfo * goalie_aggressive_best = static_cast< InterceptInfo * >( 0 );
	double goalie_aggressive_score = -10000.0;
#endif

	const std::size_t MAX = cache.size();
	for (std::size_t i = 0; i < MAX; ++i) {
		/* if ( M_save_recovery
		 && cache[i].mode() != InterceptInfo::NORMAL )
		 {
		 continue;
		 }*/

		const int cycle = cache[i].cycle;
		const Vector2D self_pos = opp_ball.inertiaPoint(cycle);
		Vector2D ball_pos = wm.ball().inertiaPoint(cycle);
		Vector2D ball_vel = wm.ball().vel() * std::pow(SP.ballDecay(), cycle);

#ifdef DEBUG_PRINT_INTERCEPT_LIST
		dlog.addText( Logger::INTERCEPT,
				"intercept %d: cycle=%d t=%d d=%d pos=(%.2f %.2f) vel=(%.2f %.1f) trap_ball_dist=%f",
				i, cycle, cache[i].turnCycle(), cache[i].dashCycle(),
				ball_pos.x, ball_pos.y,
				ball_vel.x, ball_vel.y,
				cache[i].ballDist() );
#endif

		if (ball_pos.absX() > max_pitch_x || ball_pos.absY() > max_pitch_y) {
			continue;
		}
		bool attacker = false;
		if (ball_vel.x < -0.5 && ball_vel.r2() > std::pow(speed_max, 2)
		&& ball_pos.x > -47.0
		//&& std::fabs( ball_pos.y - wm.self().pos().y ) < 10.0
		&& (ball_pos.x < -35.0 || ball_pos.x < wm.ourDefenseLineX())) {
#ifdef DEBUG_PRINT
			dlog.addText(Logger::INTERCEPT, "___ %d attacker", i);
#endif
			attacker = true;
		}

		const double opp_rate = (attacker ? 0.95 : 0.7);
#if 0
		if ( attacker
				&& opp_min <= cycle - 5
				&& ball_vel.r2() > std::pow( 1.2, 2 ) )
		{
#ifdef DEBUG_PRINT
			dlog.addText( Logger::INTERCEPT,
					"___ %d attacker ignores opponent cycle=%d: ball_vel=(%.1f %.1f)",
					i,
					opp_min,
					ball_vel.x, ball_vel.y );
#endif
		}
		else
#endif
			if (cycle >= opp_min * opp_rate) {
#ifdef DEBUG_PRINT
				dlog.addText(Logger::INTERCEPT,
						"___ %d failed: cycle=%d pos=(%.1f %.1f) turn=%d opp_min=%d rated=%.2f",
						i, cycle, ball_pos.x, ball_pos.y, cache[i].cycle, opp_min,
						opp_min * opp_rate);
#endif
				continue;
			}

		// attacker type

		if (attacker) {
			double goal_dist = 100.0 - std::min(100.0, ball_pos.dist(goal_pos));
			double x_diff = -47.0 + ball_pos.x;

			double score = (goal_dist / 100.0)
																			* std::exp(-(x_diff * x_diff) / (2.0 * 100.0));
#ifdef DEBUG_PRINT
			dlog.addText(Logger::INTERCEPT,
					"___ %d attacker cycle=%d pos=(%.1f %.1f) turn=%d score=%f",
					i, cycle, ball_pos.x, ball_pos.y, cache[i].cycle, score);
#endif
			if (score > attacker_score) {
				attacker_best = &cache[i];
				attacker_score = score;
#ifdef DEBUG_PRINT
				dlog.addText(Logger::INTERCEPT,
						"___ %d updated attacker_best score=%f", i, score);
#endif
			}

			continue;
		}

		// no turn type

		if (cache[i].turn_cycle == 0) {
			//double score = ball_pos.x;
			//double score = wm.self().pos().dist2( ball_pos );
			double score = cycle;
			//if ( ball_vel.x > 0.0 )
			//{
			//    score *= std::exp( - std::pow( ball_vel.r() - 1.0, 2.0 )
			//                       / ( 2.0 * 1.0 ) );
			//}
#ifdef DEBUG_PRINT
			dlog.addText(Logger::INTERCEPT,
					"___ %d noturn cycle=%d pos=(%.1f %.1f) turn=%d score=%f",
					i, cycle, ball_pos.x, ball_pos.y, cache[i].turn_cycle,
					score);
#endif
			if (score < noturn_score) {
				noturn_best = &cache[i];
				noturn_score = score;
#ifdef DEBUG_PRINT
				dlog.addText(Logger::INTERCEPT,
						"___ %d updated noturn_best score=%f", i, score);
#endif
			}

			continue;
		}

		// forward type

		//         if ( ball_vel.x > 0.5
		//              && ball_pos.x > wm.offsideLineX() - 15.0
		//              && ball_vel.r() > speed_max * 0.98
		//              && cycle <= opp_min - 5 )
		if (ball_vel.x < -0.1 && cycle <= opp_min - 5
				&& ball_vel.r2() > std::pow(0.6, 2)) {
			double score = (100.0 * 100.0)
																			- std::min(100.0 * 100.0, ball_pos.dist2(goal_pos));
#ifdef DEBUG_PRINT
			dlog.addText(Logger::INTERCEPT,
					"___ %d forward cycle=%d pos=(%.1f %.1f) turn=%d score=%f",
					i, cycle, ball_pos.x, ball_pos.y, cache[i].turn_cycle,
					score);
#endif
			if (score > forward_score) {
				forward_best = &cache[i];
				forward_score = score;
#ifdef DEBUG_PRINT
				dlog.addText(Logger::INTERCEPT,
						"___ %d updated forward_best score=%f", i, score);
#endif
			}

			continue;
		}

		// other: select nearest one

		{
			//double d = wm.self().pos().dist2( ball_pos );
			double d = self_pos.dist2(ball_pos);
#ifdef DEBUG_PRINT
			dlog.addText(Logger::INTERCEPT,
					"___ %d other cycle=%d pos=(%.1f %.1f) turn=%d dist2=%.2f",
					i, cycle, ball_pos.x, ball_pos.y, cache[i].turn_cycle, d);
#endif
			if (d < nearest_score) {
				nearest_best = &cache[i];
				nearest_score = d;
#ifdef DEBUG_PRINT
				dlog.addText(Logger::INTERCEPT,
						"___ %d updated nearest_best score=%f", i,
						nearest_score);
#endif
			}
		}

	}

	if (attacker_best) {
		dlog.addText(Logger::INTERCEPT,
				"<--- attacker best: cycle=%d(t=%d) score=%f",
				attacker_best->cycle, attacker_best->cycle, attacker_score);

		return *attacker_best;
	}

	if (noturn_best && forward_best) {
		//const Vector2D forward_ball_pos = wm.ball().inertiaPoint( forward_best->reachCycle() );
		//const Vector2D forward_ball_vel
		//    = wm.ball().vel()
		//    * std::pow( SP.ballDecay(), forward_best->reachCycle() );

		if (forward_best->cycle >= 5) {
			dlog.addText(Logger::INTERCEPT,
					"<--- forward best(1): cycle=%d(t=%d) score=%f",
					forward_best->cycle, forward_best->turn_cycle,
					forward_score);
		}

		Vector2D noturn_ball_vel = wm.ball().vel()
																		* std::pow(SP.ballDecay(), noturn_best->cycle);

		const double noturn_ball_speed = noturn_ball_vel.r();
		if (noturn_ball_vel.x < -0.1
				&& (noturn_ball_speed > speed_max
						|| noturn_best->cycle <= forward_best->cycle + 2)) {
			dlog.addText(Logger::INTERCEPT,
					"<--- noturn best(1): cycle=%d(t=%d) score=%f",
					noturn_best->cycle, noturn_best->turn_cycle, noturn_score);
			return *noturn_best;
		}
	}

	if (forward_best) {
		dlog.addText(Logger::INTERCEPT,
				"<--- forward best(2): cycle=%d(t=%d,d=%d) score=%f",
				forward_best->cycle, forward_best->turn_cycle, forward_score);

		return *forward_best;
	}

	Vector2D fastest_pos = wm.ball().inertiaPoint(cache[0].cycle);

	Vector2D fastest_vel = wm.ball().vel()
																	* std::pow(SP.ballDecay(), cache[0].cycle);

	if ((fastest_pos.x < 33.0 || fastest_pos.absY() > 20.0)
			&& (cache[0].cycle >= 10
					//|| wm.ball().vel().r() < 1.5 ) )
					|| fastest_vel.r() < 1.2)) {
		dlog.addText(Logger::INTERCEPT, "<--- fastest best: cycle=%d(t=%d)",
				cache[0].cycle, cache[0].turn_cycle);
		return cache[0];
	}

	if (noturn_best && nearest_best) {
		Vector2D noturn_self_pos = opp_ball.inertiaPoint(noturn_best->cycle);

		Vector2D noturn_ball_pos = wm.ball().inertiaPoint(noturn_best->cycle);

		Vector2D nearest_self_pos = opp_ball.inertiaPoint(nearest_best->cycle);

		Vector2D nearest_ball_pos = wm.ball().inertiaPoint(nearest_best->cycle);

		//         if ( wm.self().pos().dist2( noturn_ball_pos )
		//              < wm.self().pos().dist2( nearest_ball_pos ) )
		if (noturn_self_pos.dist2(noturn_ball_pos)
				< nearest_self_pos.dist2(nearest_ball_pos)) {
			dlog.addText(Logger::INTERCEPT,
					"<--- noturn best(2): cycle=%d(t=%d) score=%f",
					noturn_best->cycle, noturn_best->turn_cycle, noturn_score);

			return *noturn_best;
		}

		if (nearest_best->cycle <= noturn_best->cycle + 2) {
			Vector2D nearest_ball_vel = wm.ball().vel()
																			* std::pow(SP.ballDecay(), nearest_best->cycle);

			const double nearest_ball_speed = nearest_ball_vel.r();
			if (nearest_ball_speed < 0.7) {
				dlog.addText(Logger::INTERCEPT,
						"<--- nearest best(2): cycle=%d(t=%d) score=%f",
						nearest_best->cycle, nearest_best->turn_cycle,
						nearest_score);
				return *nearest_best;
			}

			Vector2D noturn_ball_vel = wm.ball().vel()
																			* std::pow(SP.ballDecay(), noturn_best->cycle);

			if (nearest_best->dist_ball
					< opp_ball.playerTypePtr()->kickableArea() - 0.4
					&& nearest_best->dist_ball < noturn_best->dist_ball
					&& noturn_ball_vel.x > -0.5
					&& noturn_ball_vel.r2() > std::pow(1.0, 2)
			&& noturn_ball_pos.x < nearest_ball_pos.x) {
				dlog.addText(Logger::INTERCEPT,
						"<--- nearest best(3): cycle=%d(t=%d) score=%f",
						nearest_best->cycle, nearest_best->turn_cycle,
						nearest_score);
				return *nearest_best;
			}

			Vector2D nearest_self_pos = opp_ball.inertiaPoint(
					nearest_best->cycle);

			if (nearest_ball_speed > 0.7
					//&& wm.self().pos().dist( nearest_ball_pos ) < wm.self().playerType().kickableArea() )
					&& nearest_self_pos.dist(nearest_ball_pos)
					< opp_ball.playerTypePtr()->kickableArea()) {
				dlog.addText(Logger::INTERCEPT,
						"<--- nearest best(4): cycle=%d(t=%d) score=%f",
						nearest_best->cycle, nearest_best->turn_cycle,
						nearest_score);
				return *nearest_best;
			}
		}

		dlog.addText(Logger::INTERCEPT,
				"<--- noturn best(3): cycle=%d(t=%d) score=%f",
				noturn_best->cycle, noturn_best->turn_cycle, noturn_score);

		return *noturn_best;
	}

	if (noturn_best) {
		dlog.addText(Logger::INTERCEPT,
				"<--- noturn best only: cycle=%d(t=%d) score=%f",
				noturn_best->cycle, noturn_best->turn_cycle, noturn_score);

		return *noturn_best;
	}

	if (nearest_best) {
		dlog.addText(Logger::INTERCEPT,
				"<--- nearest best only: cycle=%d(t=%d) score=%f",
				nearest_best->cycle, nearest_best->turn_cycle, nearest_score);

		return *nearest_best;
	}

	if (opp_ball.pos().x < -40.0 && wm.ball().vel().r() > 1.8
			&& wm.ball().vel().th().abs() < 100.0 && cache[0].cycle > 1) {
		const CyrusOppInterceptTable * chance_best =
				static_cast<CyrusOppInterceptTable *>(0);
		for (std::size_t i = 0; i < MAX; ++i) {
			if (cache[i].cycle <= cache[0].cycle + 3
					&& cache[i].cycle <= opp_min - 2) {
				chance_best = &cache[i];
			}
		}

		if (chance_best) {
			dlog.addText(Logger::INTERCEPT,
					"<--- chance best only: cycle=%d(t=%d)", chance_best->cycle,
					chance_best->turn_cycle);
			return *chance_best;
		}
	}

	return cache[0];

}

vector<Vector2D> CyrusPlayerIntercept::createBallCache(const WorldModel & wm) {
	const ServerParam & SP = ServerParam::i();
	const std::size_t MAX_CYCLE = 30;

	vector<Vector2D> ball_pos_cache;
	const double pitch_x_max = (
			SP.keepawayMode() ?
					SP.keepawayLength() * 0.5 : SP.pitchHalfLength() + 5.0);
	const double pitch_y_max = (
			SP.keepawayMode() ?
					SP.keepawayWidth() * 0.5 : SP.pitchHalfWidth() + 5.0);
	const double bdecay = SP.ballDecay();

	Vector2D bpos = wm.ball().pos();
	Vector2D bvel = wm.ball().vel();

	ball_pos_cache.push_back(bpos);

	if (wm.self().isKickable()) {
		return ball_pos_cache;
	}

	for (std::size_t i = 1; i <= MAX_CYCLE; ++i) {
		bpos += bvel;
		bvel *= bdecay;

		ball_pos_cache.push_back(bpos);

		if (i >= 5 && bvel.r2() < 0.01 * 0.01) {
			// ball stopped
			break;
		}

		if (bpos.absX() > pitch_x_max || bpos.absY() > pitch_y_max) {
			// out of pitch
			break;
		}
	}

	if (ball_pos_cache.size() == 1) {
		ball_pos_cache.push_back(bpos);
	}
	return ball_pos_cache;
}
