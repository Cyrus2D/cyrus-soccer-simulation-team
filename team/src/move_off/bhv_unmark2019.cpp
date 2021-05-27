/*
 * bhv_unmark.cpp
 *
 *  Created on: Jun 25, 2017
 *      Author: nader
 */

#include "bhv_unmark2019.h"
#include "strategy.h"
#include <rcsc/common/logger.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/common/server_param.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/abstract_player_object.h>
#include <rcsc/action/body_turn_to_angle.h>
#include <rcsc/action/neck_turn_to_ball.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include "field_analyzer.h"

#define unmark_debug
static const int VALID_PLAYER_THRESHOLD = 8;

unmark_passer_2019::unmark_passer_2019() {
	this->unum = 0;
	this->ballpos = Vector2D::INVALIDATED;
	this->oppmin_cycle = 0;
	this->cycle_recive_ball = 0;
}
unmark_passer_2019::unmark_passer_2019(int unum, Vector2D ballpos, int oppmin_cycle,
		int cycle_recive_ball) {
	this->unum = unum;
	this->ballpos = ballpos;
	this->oppmin_cycle = oppmin_cycle;
	this->cycle_recive_ball = cycle_recive_ball;
}

void unmark_pass_to_me_2019::update_eval() {
	pass_eval = pass_target.x;
	pass_eval += max(0.0, 40.0 - pass_target.dist(Vector2D(52, 0)));
	pass_eval += dist2_opp;
	pass_eval -= dist2_unmark_target;
	pass_eval -= dist2home_pos;
	pass_eval -= dist2self_pos;
	if (dist2_tm = !7) //borna
		pass_eval += dist2_tm;
	if (dist_target2ball > 20) { //borna
		pass_eval -= dist_pass2ball;
	}
	if (!can_pass)
		pass_eval /= 10.0;
}
bool unmark_pass_to_me_2019::update(const WorldModel & wm, unmark_passer_2019 passer,
		Vector2D unmark_target) {
	can_pass = true;
	can_pass_withoutnear = true;
	Vector2D ball_pos = passer.ballpos;
	double ball_speed = 2.5;
	Vector2D ball_vel = Vector2D::polar2vector(ball_speed,
			(pass_target - ball_pos).th());
	double ball_travel_dist = 0;
	int ball_travel_step = 0;
	int opp_min_dist2passer = opp_min_dist_passer(wm, ball_pos);
	while (ball_travel_dist < passer.ballpos.dist(pass_target)) {
		if (pass_target.dist(ball_pos) < 0.8)
			break;
		ball_pos += ball_vel;
		ball_travel_dist += ball_vel.r();
		ball_travel_step++;
		ball_vel *= ServerParam::i().ballDecay();
		int opp_recive = min_opp_recive(wm, ball_pos, ball_travel_step + 1);
		if (opp_recive <= ball_travel_step)
			can_pass = false;
	}
	if (!can_pass
			&& wm.theirPlayer(opp_min_dist2passer)->pos().dist(passer.ballpos)
					> 3)
		return false;
	ball_pos = passer.ballpos;
	ball_vel = Vector2D::polar2vector(ball_speed,
			(pass_target - ball_pos).th());
	ball_travel_dist = 0;
	ball_travel_step = 0;

	while (ball_travel_dist < passer.ballpos.dist(pass_target)) {
		if (pass_target.dist(ball_pos) < 0.8)
			break;
		ball_pos += ball_vel;
		ball_travel_dist += ball_vel.r();
		ball_travel_step++;
		ball_vel *= ServerParam::i().ballDecay();
		int opp_recive = min_opp_recive(wm, ball_pos, ball_travel_step + 1,
				opp_min_dist2passer);
		if (opp_recive <= ball_travel_step)
			can_pass_withoutnear = false;
	}

	if (!can_pass_withoutnear)
		return false;
	dist2self_pos = wm.self().pos().dist(pass_target);
	dist2home_pos = Strategy::i().getPosition(wm.self().unum()).dist(
			pass_target);
	dist2_unmark_target = unmark_target.dist(pass_target);
	dist_target2ball = unmark_target.dist(ball_pos);
	dist_pass2ball = pass_target.dist(ball_pos) / 2;
	dist2_opp = 70;
	dist2_tm = 7;
	if (!ServerParam::i().theirPenaltyArea().contains(wm.self().pos())) {
		for (int t = 2; t <= 11; t++) {
			const AbstractPlayerObject * tm = wm.ourPlayer(t);
			if (tm == NULL || tm->unum() < 1)
				continue;
			double tm_dist = tm->pos().dist(ball_pos);
			if (tm_dist < dist2_tm)
				dist2_tm = tm_dist;
		}
	}
	for (int o = 1; o <= 11; o++) {
		const AbstractPlayerObject * opp = wm.theirPlayer(o);
		if (opp == NULL || opp->unum() < 1)
			continue;
		double opp_dist = opp->pos().dist(ball_pos);
		if (opp_dist < dist2_opp)
			dist2_opp = opp_dist;
	}

	update_eval();
	return true;
}
int unmark_pass_to_me_2019::opp_min_dist_passer(const WorldModel & wm,
		Vector2D ball_pos) {
	double min = 100;
	int unum = 0;
	for (int o = 1; o <= 11; o++) {
		const AbstractPlayerObject * opp = wm.theirPlayer(o);
		if (opp == NULL || opp->unum() < 1)
			continue;
		double opp_dist = opp->pos().dist(ball_pos);
		if (opp_dist < min) {
			min = opp_dist;
			unum = o;
		}
	}
	return unum;
}
int unmark_pass_to_me_2019::min_opp_recive(const WorldModel & wm, Vector2D ball_pos,
		int max, int first_opp) {
	int min = 100;
	for (int o = 1; o <= 11; o++) {
		if (o == first_opp)
			continue;
		const AbstractPlayerObject * opp = wm.theirPlayer(o);
		if (opp == NULL || opp->unum() < 1)
			continue;
		double opp_dist = opp->pos().dist(ball_pos);
		if (opp_dist < min)
			min = opp_dist;
		if (min < max)
			return min;
	}
	return min;
}

bhv_unmark_2019::bhv_unmark_2019() {
	cycle_start = 0;
}
double bhv_unmark_2019::min_tm_dist(const WorldModel & wm, Vector2D tmp) {
	double min = 100;
	for (int t = 1; t <= 11; t++) {
		const AbstractPlayerObject * tm = wm.ourPlayer(t);
		if (tm != NULL && tm->unum() > 0 && tm->unum() != wm.self().unum()) {
			double dist = tm->pos().dist(tmp);
			if (dist < min)
				min = dist;
		}
	}
	return min;
}
double bhv_unmark_2019::min_opp_dist(const WorldModel & wm, Vector2D tmp) {
	double min = 100;
	for (int o = 1; o <= 11; o++) {
		const AbstractPlayerObject * opp = wm.theirPlayer(o);
		if (opp != NULL && opp->unum() > 0) {
			double dist = opp->pos().dist(tmp);
			if (dist < min)
				min = dist;
		}
	}
	return min;
}
double bhv_unmark_2019::min_tm_home_dist(const WorldModel & wm, Vector2D tmp) {
	double min = 100;
	for (int i = 1; i <= 11; i++) {
		if (i == wm.self().unum())
			continue;
		Vector2D home_pos = Strategy::i().getPosition(i);
		double dist = home_pos.dist(tmp);
		if (dist < min)
			min = dist;
	}
	return min;
}
bool bhv_unmark_2019::is_good_for_pass_from_last(const WorldModel & wm) {
	if (is_good_for_pass(wm)) {
//		if (passer.unum == wm.interceptTable()->fastestTeammate()->unum()) { //borna
		const PlayerObject * tm = wm.interceptTable()->fastestTeammate();
		if (tm != NULL && tm->unum() > 0) {
			passer.cycle_recive_ball =
					wm.interceptTable()->teammateReachCycle();
			passer.oppmin_cycle = wm.interceptTable()->opponentReachCycle();
			Vector2D ball_pos = wm.ball().inertiaPoint(
					passer.cycle_recive_ball);
			if (ball_pos.dist(passer.ballpos) > 5)
				return false;
			return update_for_pass(wm, passer);
		}
//		} else {
//			const AbstractPlayerObject * tm = wm.ourPlayer(passer.unum);
//			if (tm != NULL && tm->unum() > 0) {
//				passer.cycle_recive_ball =
//						wm.interceptTable()->teammateReachCycle();
//				passer.oppmin_cycle = wm.interceptTable()->opponentReachCycle();
//				Vector2D ball_pos = wm.ball().inertiaPoint(
//						passer.cycle_recive_ball);
//				if (ball_pos.dist(passer.ballpos) > 5)
//					return false;
//				return update_for_pass(wm, passer);
//			}
//		}
	}
	return false;
}
bool bhv_unmark_2019::is_good_for_unmark_from_last(const WorldModel & wm) { //borna
	if (is_good_for_unmark(wm)) {
//		if (passer.unum == wm.interceptTable()->fastestTeammate()->unum()) {
		const PlayerObject * tm = wm.interceptTable()->fastestTeammate();
		if (tm != NULL && tm->unum() > 0) {
			passer.cycle_recive_ball =
					wm.interceptTable()->teammateReachCycle();
			passer.oppmin_cycle = wm.interceptTable()->opponentReachCycle();
			Vector2D ball_pos = wm.ball().inertiaPoint(
					passer.cycle_recive_ball);
			if (ball_pos.dist(passer.ballpos) > 5)
				return false;
			return update_for_unmark(wm, passer);
		}
//		} else {
//			const AbstractPlayerObject * tm = wm.ourPlayer(passer.unum);
//			if (tm != NULL && tm->unum() > 0) {
//				passer.cycle_recive_ball =
//						wm.interceptTable()->teammateReachCycle();
//				passer.oppmin_cycle = wm.interceptTable()->opponentReachCycle();
//				Vector2D ball_pos = wm.ball().inertiaPoint(
//						passer.cycle_recive_ball);
//				if (ball_pos.dist(passer.ballpos) > 5)
//					return false;
//				return update_for_unmark(wm, passer);
//			}
//		}
	}
	return false;
}
Vector2D bhv_unmark_2019::penalty_unmark(const WorldModel & wm) {
	Vector2D home_pos = Strategy::i().getPosition(wm.self().unum());
	Vector2D self_pos = wm.self().pos();
	double div_dist = 1.0;
	double div_angle = 20;
	double maxeval = 0;
	int tm_cycle = wm.interceptTable()->teammateReachCycle();
	Vector2D ball_inner = wm.ball().inertiaPoint(tm_cycle);
	Vector2D best_target = Vector2D(0.0,0.0);
	for (double y = self_pos.y + 3; y < self_pos.y - 4; y -= div_dist) {
		for (int x = 40; x < 53; x++) {
			Vector2D tmp_target = Vector2D(x, y);
			if(tmp_target.dist(self_pos) > 10)
				continue;
			bool can_goal = false;
			bool opp_can_reach = false;
			can_goal = FieldAnalyzer::can_shoot_from(true, tmp_target,
					wm.getPlayerCont(
							new OpponentOrUnknownPlayerPredicate(wm.ourSide())),
					VALID_PLAYER_THRESHOLD);
			double eval =/* max(100 - tmp_target.dist(ball_inner), 0.0)
					+ */ max(100 - tmp_target.dist(self_pos), 0.0)
					+ max(100 - tmp_target.dist(Vector2D(52.0, 0)), 0.0);
			if (can_goal) {
				if (maxeval < eval) {
					maxeval = eval;
					best_target = tmp_target;
				}
			}
		}
	}
	return best_target;
}
bool bhv_unmark_2019::is_good_for_pass(const WorldModel & wm) {
	dlog.addText(Logger::POSITIONING, "-start_igfp");
	Vector2D home_pos = Strategy::i().getPosition(wm.self().unum());
	Vector2D self_pos = wm.self().pos();
	double min_dist_tm = 5;
	double min_dist_opp = 5;
	if (target_pos.dist(Vector2D(52, 0)) < 20 && passer.ballpos.x > 30
			&& Strategy::i().self_line == Strategy::PostLine::forward)
		min_dist_opp = 1.5;
	if (target_pos.dist(Vector2D(52, 34)) < 20
			|| target_pos.dist(Vector2D(52, -34)) < 20) {
		if (passer.ballpos.dist(Vector2D(52, 34)) < 15
				|| passer.ballpos.dist(Vector2D(52, -34)) < 15) {
			if (self_pos.dist(Vector2D(52, 34)) < 20
					|| self_pos.dist(Vector2D(52, -34)) < 20) {
				min_dist_opp = 2;
				min_dist_tm = 2;
			}
		}
	}
	double div_angle = 20;
	double min_dist_ball = 5;
	double max_dist_home_pos = 8;
	double max_dist_self_pos = 11;
	double max_dist_x = 5;
	double max_x = 52;
	double max_y = 33;
	double offside_x = wm.offsideLineX();
	if (target_pos.dist(home_pos) > max_dist_home_pos) {
		dlog.addText(Logger::POSITIONING,
				"------target_pos.dist(home_pos) > max_dist_home_pos");
		return false;
	}
	if (target_pos.dist(self_pos) > max_dist_self_pos) {
		dlog.addText(Logger::POSITIONING,
				"------target_pos.dist(self_pos) > max_dist_self_pos");
		return false;
	}
	if (target_pos.dist(passer.ballpos) < min_dist_ball) {
		dlog.addText(Logger::POSITIONING,
				"------target_pos.dist(passer.ballpos) < min_dist_ball");
		return false;
	}
	if (target_pos.x > offside_x - 0.5) {
		dlog.addText(Logger::POSITIONING,
				"------target_pos.x > offside_x - 0.5");
		return false;
	}
	if (target_pos.x < home_pos.x - max_dist_x) {
		dlog.addText(Logger::POSITIONING,
				"------target_pos.x < home_pos.x + max_dist_x, %.2f , %.2f , %.2f",
				target_pos.x, home_pos.x, max_dist_x);
		return false;
	}
	if (min_opp_dist(wm, target_pos) < min_dist_opp) {
		dlog.addText(Logger::POSITIONING,
				"------min_opp_dist(wm, target_pos) < min_dist_opp");
		return false;
	}
	if (min_tm_dist(wm, target_pos) < min_dist_tm) {
		dlog.addText(Logger::POSITIONING,
				"------min_tm_dist(wm, target_pos) < min_dist_tm");
		return false;
	}
	if (min_tm_home_dist(wm, target_pos) < home_pos.dist(target_pos) + 2) {
		dlog.addText(Logger::POSITIONING,
				"------min_tm_home_dist(wm, target_pos) < home_pos.dist(target_pos) + 2");
		return false;
	}
	if (target_pos.x > max_x) {
		dlog.addText(Logger::POSITIONING, "------target_pos.x > max_x");
		return false;
	}
	if (target_pos.absY() > max_y) {
		dlog.addText(Logger::POSITIONING, "------target_pos.absY() > max_y");
		return false;
	}
	return true;
}
bool bhv_unmark_2019::is_good_for_unmark(const WorldModel & wm) {
	dlog.addText(Logger::POSITIONING, "-----start_igfu");
	Vector2D home_pos = Strategy::i().getPosition(wm.self().unum());
	Vector2D self_pos = wm.self().pos();
	double min_dist_opp = 5;
	double min_dist_tm = 5;
	if (target_pos.dist(Vector2D(52, 0)) < 20 && passer.ballpos.x > 30
			&& Strategy::i().self_line == Strategy::PostLine::forward)
		min_dist_opp = 1.5;
	if (target_pos.dist(Vector2D(52, 34)) < 20
			|| target_pos.dist(Vector2D(52, -34)) < 20) {
		if (passer.ballpos.dist(Vector2D(52, 34)) < 15
				|| passer.ballpos.dist(Vector2D(52, -34)) < 15) {
			if (self_pos.dist(Vector2D(52, 34)) < 20
					|| self_pos.dist(Vector2D(52, -34)) < 20) {
				min_dist_opp = 2;
				min_dist_tm = 2;
			}
		}
	}
	double min_dist_ball = 5;
	double max_dist_home_pos = 10;
	double max_dist_self_pos = 11;
	double max_dist_x = 5;
	double max_x = 52;
	double max_y = 33;
	double offside_x = wm.offsideLineX();

	if (target_pos.dist(home_pos) > max_dist_home_pos) {
		dlog.addText(Logger::POSITIONING,
				"------target_pos.dist(home_pos) > max_dist_home_pos");
		return false;
	}
	if (target_pos.dist(self_pos) > max_dist_self_pos) {
		dlog.addText(Logger::POSITIONING,
				"------target_pos.dist(self_pos) > max_dist_self_pos");
		return false;
	}
	if (target_pos.dist(passer.ballpos) < min_dist_ball) {
		dlog.addText(Logger::POSITIONING,
				"------target_pos.dist(passer.ballpos) < min_dist_ball");
		return false;
	}
	if (target_pos.x > offside_x - 0.5) {
		dlog.addText(Logger::POSITIONING,
				"------target_pos.x > offside_x - 0.5");
		return false;
	}
	if (target_pos.x < home_pos.x - max_dist_x) {
		dlog.addText(Logger::POSITIONING,
				"------target_pos.x < home_pos.x + max_dist_x");
		return false;
	}
	if (min_opp_dist(wm, target_pos) < min_dist_opp) {
		dlog.addText(Logger::POSITIONING,
				"------min_opp_dist(wm, target_pos) < min_dist_opp");
		return false;
	}
	if (min_tm_dist(wm, target_pos) < min_dist_tm) {
		dlog.addText(Logger::POSITIONING,
				"------min_tm_dist(wm, target_pos) < min_dist_tm");
		return false;
	}
	if (min_tm_home_dist(wm, target_pos) < home_pos.dist(target_pos) + 2) {
		dlog.addText(Logger::POSITIONING,
				"------min_tm_home_dist(wm, target_pos) < home_pos.dist(target_pos) + 2");
		return false;
	}
	if (target_pos.x > max_x) {
		dlog.addText(Logger::POSITIONING, "------target_pos.x > max_x");
		return false;
	}
	if (target_pos.absY() > max_y) {
		dlog.addText(Logger::POSITIONING, "------target_pos.absY() > max_y");
		return false;
	}
	return true;
}

bool bhv_unmark_2019::execute(PlayerAgent * agent) {
	const WorldModel & wm = agent->world();
	Vector2D self_pos = wm.self().pos();
//	cout<<wm.time().cycle()<<endl;
	if (self_pos.dist(target_pos) > 0.5) {
		double dash_power = Strategy::get_normal_dash_power(wm);
		if (passer.oppmin_cycle < 5)
			dash_power = 100;
		if (target_pos.dist(Vector2D(52, 0)) < 25)
			dash_power = 100;
		if (min_opp_dist(wm, target_pos) < 5)
			dash_power = 100;
//		agent->debugClient().addMessage("passer: %d", passer.unum);
//		agent->debugClient().addCircle(wm.ourPlayer(passer.unum)->pos(), 3.0);
//		for (int i = 0; i < passes.size(); i++) {
//
//		}
		agent->debugClient().addMessage("UNMARKING");
		if (wm.ball().pos().x > 40 && self_pos.x > 40 && penalty_unmark(wm) != Vector2D(0.0,0.0)) {
			target_pos = penalty_unmark(wm);
//			cout << "UNMARkiiiNG<<endl";
		}
//		agent->doPointto(target_pos.x, target_pos.y);
		Body_GoToPoint2010(target_pos, 0.3, dash_power).execute(agent);
//		Bhv_BasicMove().ofensive_body_go_to_point(agent, target_pos, 0.3,
//				dash_power);
	} else if (abs(target_body - wm.self().body().degree()) > 10
			|| !ServerParam::i().theirPenaltyArea().contains(target_pos)) {
//		agent->doPointto(target_pos.x, target_pos.y);
		agent->debugClient().addMessage("UNMARK_TURNING");

		Body_TurnToAngle(target_body).execute(agent);
	} else if (self_pos.dist(Vector2D(52.0, 0.0)) < 25.0) {
//		agent->doPointto(target_pos.x, target_pos.y);
		agent->debugClient().addMessage("KOSSHER");

		agent->doDash(100, 0);
	}
	if (wm.existKickableOpponent() && wm.ball().distFromSelf() < 18.0) {
		agent->setNeckAction(new Neck_TurnToBall());
	} else {
		agent->setNeckAction(new Neck_TurnToBallOrScan());
	}
	return true;
}
void bhv_unmark_2019::set_unmark_angle(const WorldModel & wm) {
	double ball_unpos_angle = (passer.ballpos - target_pos).th().degree();
	if (target_pos.y < 0) {
		if (ball_unpos_angle < -90) {
			target_body = ball_unpos_angle - 90;
		} else if (ball_unpos_angle < 0) {
			target_body = ball_unpos_angle + 90;
		} else if (ball_unpos_angle < 90) {
			target_body = ball_unpos_angle + 90;
		} else {
			target_body = ball_unpos_angle - 90;
		}
	} else {
		if (ball_unpos_angle < -90) {
			target_body = ball_unpos_angle + 90;
		} else if (ball_unpos_angle < 0) {
			target_body = ball_unpos_angle - 90;
		} else if (ball_unpos_angle < 90) {
			target_body = ball_unpos_angle - 90;
		} else {
			target_body = ball_unpos_angle + 90;
		}
	}
	if (ServerParam::i().theirPenaltyArea().contains(target_pos)) {
		Vector2D left_dirac(52, -7);
		Vector2D right_dirac(52, -7);
		if (target_pos.dist(left_dirac) < target_pos.dist(right_dirac)) {
			target_body = (left_dirac - target_pos).th().degree();
		} else {
			target_body = (right_dirac - target_pos).th().degree();
		}
	}
}
bool bhv_unmark_2019::update_for_pass(const WorldModel & wm, unmark_passer_2019 passer) {
	cycle_start = wm.time().cycle();
	this->passer = passer;
	double div_dist = 0.5;
	double div_angle = 45;
	double min_dist_opp = 3;
	double min_dist_tm = 3;
	double min_dist_ball = 3;
	double max_dist_passer = 35;
	double max_dist_target = 3;
	for (double dist = 0; dist < max_dist_target; dist += div_dist) {
		for (double angle = 0; angle < 360; angle += div_angle) {
			unmark_pass_to_me_2019 pass;
			pass.pass_target = target_pos + Vector2D::polar2vector(dist, angle);
			if (pass.pass_target.dist(passer.ballpos) < min_dist_ball)
				continue;
			if (pass.pass_target.dist(passer.ballpos) > max_dist_passer)
				continue;
			if (pass.pass_target.dist(target_pos) > max_dist_target)
				continue;
			if (min_opp_dist(wm, pass.pass_target) < min_dist_opp)
				continue;
			if (min_tm_dist(wm, pass.pass_target) < min_dist_tm)
				continue;
			if (pass.update(wm, passer, target_pos)) {
				passes.push_back(pass);
			}
		}
	}
	if (passes.size() == 0)
		return false;
	set_unmark_angle(wm);
	eval_for_pass = 0;
	double max_pass_eval = 0;
	double sum_pass_eval = 0;
	for (int i = 0; i < passes.size(); i++) {

		if (max_pass_eval < passes[i].pass_eval)
			max_pass_eval = passes[i].pass_eval;
		if (passes[i].can_pass)
			sum_pass_eval += passes[i].pass_eval;
//		else
//			//borna
//			sum_pass_eval += passes[i].pass_eval;
	}
	eval_for_pass = max_pass_eval + /* (double) passes.size() / 10.0 */
	sum_pass_eval / (double) passes.size();
	double angle_move = (target_pos - wm.self().pos()).th().degree();
	double angle_pass = (passer.ballpos - wm.self().pos()).th().degree();
	double angle_dif = angle_move - angle_pass;
	if (angle_dif < 0)
		angle_dif *= (-1.0);
	if (angle_dif > 180)
		angle_dif = 360 - angle_dif;
	if (angle_dif > 110)
		eval_for_pass /= (2.5);
	else if (angle_dif > 75)
		eval_for_pass /= (1.5);
	unmark_for_posses = true;
	return true;
}
bool bhv_unmark_2019::update_for_unmark(const WorldModel & wm,
		unmark_passer_2019 passer) {
	cycle_start = wm.time().cycle();
	this->passer = passer;

	set_unmark_angle(wm);
	eval_for_unmark = 0;
	double min_opp_distt = min_opp_dist(wm, target_pos);
	if (min_opp_distt < 1.5)
		return false;
	double min_tm_distt = min_tm_dist(wm, target_pos);
	double home_dist = target_pos.dist(
			Strategy::i().getPosition(wm.self().unum()));
	eval_for_unmark = min_opp_distt * 5.0 + min_tm_distt * 2.0
			- 3.0 * home_dist;

	double angle_move = (target_pos - wm.self().pos()).th().degree();
	double angle_pass = (wm.ball().pos() - wm.self().pos()).th().degree();
	double angle_dif = angle_move - angle_pass;
	if (angle_dif < 0)
		angle_dif *= (-1.0);
	if (angle_dif > 180)
		angle_dif = 360 - angle_dif;
	if (angle_dif > 110)
		eval_for_unmark /= (2.5);
	else if (angle_dif > 75)
		eval_for_unmark /= (1.5);
	unmark_for_posses = false;
	return true;
}

double bhv_unmarkes_2019::min_tm_dist(const WorldModel & wm, Vector2D tmp) {
	double min = 100;
	for (int t = 1; t <= 11; t++) {
		const AbstractPlayerObject * tm = wm.ourPlayer(t);
		if (tm != NULL && tm->unum() > 0 && tm->unum() != wm.self().unum()) {
			double dist = tm->pos().dist(tmp);
			if (dist < min)
				min = dist;
		}
	}
	return min;
}
double bhv_unmarkes_2019::min_opp_dist(const WorldModel & wm, Vector2D tmp) {
	double min = 100;
	for (int o = 1; o <= 11; o++) {
		const AbstractPlayerObject * opp = wm.theirPlayer(o);
		if (opp != NULL && opp->unum() > 0) {
			double dist = opp->pos().dist(tmp);
			if (dist < min)
				min = dist;
		}
	}
	return min;
}

bool bhv_unmarkes_2019::execute(PlayerAgent * agent) {
	const WorldModel & wm = agent->world();
#ifdef DEBUG_UNMARK
	dlog.addText(Logger::POSITIONING,"start bhv_unmarks");
#endif
//	std::cout << "before canunmark" << std::endl;
	if (!can_unmark(wm))
		return false;
#ifdef DEBUG_UNMARK
	dlog.addText(Logger::POSITIONING,"I can unmark");
#endif
//	std::cout << "before updatepasser" << std::endl;
	unmark_passer_2019 passer = update_passer(wm);
#ifdef DEBUG_UNMARK
	dlog.addText(Logger::POSITIONING," passer:%d in (%.2f,%.2f) after %d, oppminc:%d",passer.unum,passer.ballpos.x,passer.ballpos.y,passer.cycle_recive_ball,passer.oppmin_cycle);
#endif
	if (best_last_unmark.cycle_start > wm.time().cycle() - 3){
//		std::cout << "run last unmark" << std::endl;
				return best_last_unmark.execute(agent);
	}
	if (best_last_unmark.cycle_start > wm.time().cycle() - 7
			&& best_last_unmark.passer.unum == passer.unum
			&& passer.ballpos.dist(best_last_unmark.passer.ballpos) < 5
			&& ((ServerParam::i().theirPenaltyArea().contains(wm.ball().pos())
					&& passer.ballpos.dist(best_last_unmark.passer.ballpos)
							< 2.5)
					|| (!ServerParam::i().theirPenaltyArea().contains(
							wm.ball().pos())))
			&& (best_last_unmark.is_good_for_pass_from_last(wm)
					|| best_last_unmark.is_good_for_unmark_from_last(wm))) {
#ifdef DEBUG_UNMARK
		dlog.addText(Logger::POSITIONING,"last unmark is good");
#endif
//		std::cout << "run last unmark" << std::endl;
		return best_last_unmark.execute(agent);
	}
//	std::cout << "before create unmarks" << std::endl;
	create_unmarks(wm, passer);
#ifdef DEBUG_UNMARK
	dlog.addText(Logger::POSITIONING,"number of unmark:%d",unmarks.size());
#endif
	if (unmarks.size() == 0)
		return false;
//	std::cout << "before update best unmark" << std::endl;
	update_best_unmark();
	best_last_unmark = *best_unmark;
#ifdef DEBUG_UNMARK
	dlog.addText(Logger::POSITIONING,"best is(%.2f,%.2f),%.2f,ep:%0.2,eu:%0.2f,ps:%d,forpass:%d",best_unmark->target_pos.x,best_unmark->target_pos.y,best_unmark->target_body,best_unmark->eval_for_pass,best_unmark->eval_for_unmark,best_unmark->passes.size(),best_unmark->unmark_for_posses);
#endif
	return best_unmark->execute(agent);
}

bool bhv_unmarkes_2019::can_unmark(const WorldModel & wm) {
	Strategy::PostLine pl_line = Strategy::i().self_Line();
	int stamina = wm.self().stamina();
	 static const Rect2D penalty_area=Rect2D(Vector2D(38,-17),Vector2D(53,17));
	  if (wm.ball().inertiaPoint(wm.interceptTable()->teammateReachCycle()).dist(
			wm.self().pos()) > 30)
		return false;
	  if(penalty_area.contains(wm.self().pos()))
	  	    return true;
	  if (pl_line == Strategy::PostLine::back) {
		if (stamina > 4500)
			return true;
	} else if (pl_line == Strategy::PostLine::half) {
		if (stamina > 3500)
			return true;
	} else {
		if (stamina > 3000)
			return true;
	}

	return false;
}
unmark_passer_2019 bhv_unmarkes_2019::update_passer(const WorldModel & wm) {
	const PlayerObject * tm = wm.interceptTable()->fastestTeammate();
	if (tm != NULL && tm->unum() > 0) {
		int tm_cycle = wm.interceptTable()->teammateReachCycle();
		int opp_cycle = wm.interceptTable()->opponentReachCycle();
		Vector2D ball_pos = wm.ball().inertiaPoint(tm_cycle);
//		if (ball_pos.dist(wm.self().pos()) > 20) { //bonra
		double max_eval = -10;
		int best_tm = 0;
		for (int i = 2; i <= 11; i++) {
			if (i == wm.self().unum())
				continue;
			if (i == tm->unum())
				continue;
			const AbstractPlayerObject * tmp = wm.ourPlayer(i);
			if (tmp == NULL || tmp->unum() < 2)
				continue;
			if (tmp->posCount() > 5)
				continue;
			Line2D passTome = Line2D(ball_pos, wm.self().pos());
			if (passTome.dist(tmp->pos()) > 20)
				continue;
			Vector2D tm_on_passTome = passTome.projection(tmp->pos());
			//borna

			Circle2D pass_contain = Circle2D(
					Vector2D((wm.self().pos().x + ball_pos.x) / 2,
							(wm.self().pos().y + ball_pos.y) / 2),
					wm.self().pos().dist(ball_pos) / 2);
			if (!pass_contain.contains(tm_on_passTome))
				continue;
//				if (wm.self().pos().x >= ball_pos.x) {
//					if (wm.self().pos().y >= ball_pos.y) {
//						if (!((wm.self().pos().x >= tm_on_passTome.x
//								&& tm_on_passTome.x >= ball_pos.x)
//								&& (wm.self().pos().y >= tm_on_passTome.y
//										&& tm_on_passTome.y >= ball_pos.y)))
//							continue;
//					} else {
//						if (!((wm.self().pos().x >= tm_on_passTome.x
//								&& tm_on_passTome.x >= ball_pos.x)
//								&& (wm.self().pos().y <= tm_on_passTome.y
//										&& tm_on_passTome.y <= ball_pos.y)))
//							continue;
//					}
//				} else {
//					if (wm.self().pos().y >= ball_pos.y) {
//						if (!((wm.self().pos().x <= tm_on_passTome.x
//								&& tm_on_passTome.x <= ball_pos.x)
//								&& (wm.self().pos().y >= tm->pos().y
//										&& tm_on_passTome.y >= ball_pos.y)))
//							continue;
//					} else {
//						if (!((wm.self().pos().x <= tm_on_passTome.x
//								&& tm_on_passTome.x <= ball_pos.x)
//								&& (wm.self().pos().y <= tm_on_passTome.y
//										&& tm_on_passTome.y <= ball_pos.y)))
//							continue;
//					}
//				}

//			double ball_me_angle = (wm.self().pos() - ball_pos).th().degree();
//			double me_ball_angle = (ball_pos - wm.self().pos()).th().degree();
//			double me_tm_angle = (tm->pos() - wm.self().pos()).th().degree();
//			double ball_tm_angle = (tm->pos() - ball_pos).th().degree();
//			double dif1 = ball_me_angle - ball_tm_angle;
//			if (dif1 < 0)
//				dif1 *= (-1.0);
//			if (dif1 > 180)
//				dif1 = 360 - dif1;
//			double dif2 = me_ball_angle - me_tm_angle;
//			if (dif2 < 0)
//				dif2 *= (-1.0);
//			if (dif2 > 180)
//				dif2 = 360 - dif2;
//			if (dif1 > 90 || dif2 > 90)
//				continue;

			double eval = pow(20.0, 2.0) - passTome.dist2(tmp->pos());
			if (eval > max_eval) {
				max_eval = eval;
				best_tm = i;
			}
		}
		if (best_tm > 1) {
			if (ball_pos.dist(wm.self().pos()) > 20) {
				int unum = best_tm;
				unmark_passer_2019 passer = unmark_passer_2019(unum,
						wm.ourPlayer(best_tm)->pos(), 5, opp_cycle);
				return passer;
			}
			if (ball_pos.dist(wm.self().pos()) > 10 && max_eval > 300) {
				int unum = best_tm;
				unmark_passer_2019 passer = unmark_passer_2019(unum,
						wm.ourPlayer(best_tm)->pos(), 5, opp_cycle);
				return passer;
			}
		}
//		}

		int unum = tm->unum();
		unmark_passer_2019 passer = unmark_passer_2019(unum, ball_pos, tm_cycle,
				opp_cycle);
		return passer;
	}
	unmark_passer_2019 passer = unmark_passer_2019(0, Vector2D(-100, -100), 0, 0);
	return passer;
}

void bhv_unmarkes_2019::create_unmarks(const WorldModel & wm, unmark_passer_2019 passer) {
	dlog.addText(Logger::POSITIONING, "create unmarks");
	if (passer.unum == 0) {
		return;
	} else {
		double div_dist = 1.0;
		double div_angle = 20;
		double max_dist_home_pos = 8;
		Vector2D home_pos = Strategy::i().getPosition(wm.self().unum());
		Vector2D self_pos = wm.self().pos();
//		if (self_pos.x > 35 && wm.ball().pos().x > 35) {
//			Vector2D
//		target =
//	}
		for (double dist = 0; dist < max_dist_home_pos; dist += div_dist) {
			for (double angle = 0; angle < 360; angle += div_angle) {
				bhv_unmark_2019 tmp_unmark;
//				Vector2D targets_center = Vector2D(  //borna
//						(home_pos.x + self_pos.x) / 2,
//						(home_pos.y + self_pos.y) / 2);
				tmp_unmark.target_pos = home_pos
						+ Vector2D::polar2vector(dist, angle);
				tmp_unmark.dist2self = tmp_unmark.target_pos.dist(self_pos); //borna
				dlog.addText(Logger::POSITIONING, "--unmark_target(%.2f,%.2f)",
						tmp_unmark.target_pos.x, tmp_unmark.target_pos.y);
				if (!tmp_unmark.is_good_for_pass(wm)) {
					dlog.addText(Logger::POSITIONING,
							"----is not good for pass");
					if (!tmp_unmark.is_good_for_unmark(wm)) {
						dlog.addText(Logger::POSITIONING,
								"----is not good for unmark");
						continue;
					} else {
						dlog.addText(Logger::POSITIONING,
								"----is good for unmark");
						if (tmp_unmark.update_for_unmark(wm, passer)) {
							unmarks.push_back(tmp_unmark);
						}
					}
				} else {
					dlog.addText(Logger::POSITIONING, "----is good for pass");
					if (tmp_unmark.update_for_pass(wm, passer)) {
						unmarks.push_back(tmp_unmark);
					}
				}
				if (dist == 0)
					break;
			}
		}
	}
}
void bhv_unmarkes_2019::update_best_unmark() { //borna
	double max_eval = -1000;
	for (int i = 0; i < unmarks.size(); i++) {
		if (unmarks[i].unmark_for_posses)
			if (unmarks[i].dist2self <= 5) {
				if (max_eval < unmarks[i].eval_for_pass) {
					best_unmark = &unmarks[i];
					max_eval = unmarks[i].eval_for_pass;
				}
			}
	}
	if (max_eval == -1000) {
		for (int i = 0; i < unmarks.size(); i++) {
			if (unmarks[i].unmark_for_posses)
				if (unmarks[i].dist2self <= 10) {
					if (max_eval < unmarks[i].eval_for_pass) {
						best_unmark = &unmarks[i];
						max_eval = unmarks[i].eval_for_pass;
					}
				}
		}
	}
	if (max_eval == -1000) {
		for (int i = 0; i < unmarks.size(); i++) {
			if (unmarks[i].unmark_for_posses)
				if (max_eval < unmarks[i].eval_for_pass) {
					best_unmark = &unmarks[i];
					max_eval = unmarks[i].eval_for_pass;
				}
		}
	}
	if (max_eval == -1000) {
		for (int i = 0; i < unmarks.size(); i++) {
			if (!unmarks[i].unmark_for_posses)
				if (max_eval < unmarks[i].eval_for_pass) {
					best_unmark = &unmarks[i];
					max_eval = unmarks[i].eval_for_pass;
				}
		}
	}
}
