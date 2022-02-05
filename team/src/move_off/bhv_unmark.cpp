/*
 * bhv_unmark.cpp
 *
 *  Created on: Jun 25, 2017
 *      Author: nader
 */

#include "bhv_unmark.h"
#include "../strategy.h"
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
#define DEBUG_UNMARK
static const int VALID_PLAYER_THRESHOLD = 8;
Vector2D bhv_unmarkes::last_ball_inertia = Vector2D(0,0);
int bhv_unmarkes::last_fastest_tm = 0;
int bhv_unmarkes::last_run = 0;

unmark_passer::unmark_passer() {
	this->unum = 0;
	this->ballpos = Vector2D::INVALIDATED;
	this->oppmin_cycle = 0;
	this->cycle_recive_ball = 0;
}
unmark_passer::unmark_passer(int unum, Vector2D ball_pos, int opp_min_cycle, int cycle_receive_ball) {
	this->unum = unum;
	this->ballpos = ball_pos;
	this->oppmin_cycle = opp_min_cycle;
	this->cycle_recive_ball = cycle_receive_ball;
}

void unmark_pass_to_me::update_eval(const WorldModel & wm) {
    int diff_intercept_cycle = wm.interceptTable()->opponentReachCycle() - wm.interceptTable()->teammateReachCycle();
    Vector2D best_target = Vector2D(48, 0);
    if (wm.ball().inertiaPoint(wm.interceptTable()->teammateReachCycle()).y < -10)
        best_target = Vector2D(48, -6);
    e_x = min(102.0, pass_target.x + 52.5);
	e_d_goal = max(0.0, 40.0 - pass_target.dist(best_target));
	e_d_opp = min(dist2_opp, 10.0) * 3.0;
	e_d_target= -dist2_unmark_target;
	e_d_hpos = -dist2home_pos * 5.0;
	e_d_pos = -dist2self_pos;
	e_d_tm = 0;
    e_fastest = 0;
    e_d_ball = 0;
    e_pass = 0;
    e_diff = min(min_diff, 5.0);
	if (dist2_tm > 5)
        e_d_tm = min(dist2_tm, 10.0) - 5.0;
	if (dist_target2ball > 5)
        e_d_ball = min(dist_pass2ball, 10.0) - 5.0;
    if (passer.is_fastest)
        e_fastest = 10;
    else if(passer.is_fastest && diff_intercept_cycle <= 5)
        e_fastest = 15 + min(diff_intercept_cycle, 5) + 5;
    e_fastest *= 2.0;
	if (!can_pass)
        e_pass = -20.0;
	if(!can_pass && !can_pass_withoutnear)
        e_pass = -40.0;
	e_pass *= 3.0;
	dlog.addText(Logger::POSITIONING, "--------x:%.1f goal:%.1f opp:%.1f target:%.1f hpos:%.1f pos:%.1f tm:%.1f fast:%.1f pass:%.1f diff:%.1f",
                    e_x, e_d_goal, e_d_opp, e_d_target, e_d_hpos, e_d_pos, e_d_tm, e_fastest, e_pass, e_diff);
	double eval = e_x + e_d_goal + e_d_opp + e_d_target + e_d_hpos + e_d_pos + e_d_tm + e_fastest + e_pass + e_diff + e_d_ball;
	this->pass_eval = eval;
}
bool unmark_pass_to_me::update(const WorldModel & wm, unmark_passer passer,
		Vector2D unmark_target) {
    this->passer = passer;
    int min_dif = 100;
	can_pass = true;
	can_pass_withoutnear = true;
	Vector2D ball_pos = passer.ballpos;
	double ball_speed = 2.5;
	double ball_travel_dist = 0;
	int ball_travel_step = 0;
	int opp_min_dist2passer = opp_min_dist_passer(wm, ball_pos);
	if (opp_min_dist2passer < 1 || wm.theirPlayer(opp_min_dist2passer)->pos().dist(passer.ballpos) > 3)
	    ball_speed = 2.8;
    Vector2D ball_vel = Vector2D::polar2vector(ball_speed,
                                               (pass_target - ball_pos).th());
	while (ball_travel_dist < passer.ballpos.dist(pass_target)) {
		if (pass_target.dist(ball_pos) < 0.8)
			break;
		if (ball_vel.r() < 0.2){
		    can_pass = false;
            break;
		}
		ball_pos += ball_vel;
		ball_travel_dist += ball_vel.r();
		ball_travel_step++;
		ball_vel *= ServerParam::i().ballDecay();
		int opp_recive = min_opp_recive(wm, ball_pos, ball_travel_step + 1);
		if (opp_recive <= ball_travel_step){
            can_pass = false;
            break;
		}
		min_dif = min(min_dif, opp_recive - ball_travel_step);
	}
	if (!can_pass
			&& (opp_min_dist2passer < 1 || wm.theirPlayer(opp_min_dist2passer)->pos().dist(passer.ballpos) > 3)){
        return false;
	}

	if (!can_pass){
	    min_dif = 100;
        ball_pos = passer.ballpos;
        ball_vel = Vector2D::polar2vector(ball_speed,
                                          (pass_target - ball_pos).th());
        ball_travel_dist = 0;
        ball_travel_step = 0;

        while (ball_travel_dist < passer.ballpos.dist(pass_target)) {
            if (pass_target.dist(ball_pos) < 0.8)
                break;
            if (ball_vel.r() < 0.2){
                can_pass_withoutnear = false;
                break;
            }
            ball_pos += ball_vel;
            ball_travel_dist += ball_vel.r();
            ball_travel_step++;
            ball_vel *= ServerParam::i().ballDecay();
            int opp_recive = min_opp_recive(wm, ball_pos, ball_travel_step + 1, opp_min_dist2passer);
            if (opp_recive <= ball_travel_step){
                can_pass_withoutnear = false;
                break;
            }

            min_dif = min(min_dif, opp_recive - ball_travel_step);
        }

        if (!can_pass_withoutnear){
            return false;
        }
    }
    ball_pos = passer.ballpos;
	dist2self_pos = wm.self().pos().dist(pass_target);
	dist2home_pos = Strategy::i().getPosition(wm.self().unum()).dist(
			pass_target);
	dist2_unmark_target = unmark_target.dist(pass_target);
	dist_target2ball = unmark_target.dist(ball_pos);
	dist_pass2ball = pass_target.dist(ball_pos);
	dist2_opp = 100;
	dist2_tm = 100;
    for (int t = 2; t <= 11; t++) {
        const AbstractPlayerObject * tm = wm.ourPlayer(t);
        if (tm == NULL || tm->unum() < 1)
            continue;
        double tm_dist = tm->pos().dist(pass_target);
        if (tm_dist < dist2_tm)
            dist2_tm = tm_dist;
    }
	for (int o = 1; o <= 11; o++) {
		const AbstractPlayerObject * opp = wm.theirPlayer(o);
		if (opp == NULL || opp->unum() < 1)
			continue;
		double opp_dist = opp->pos().dist(pass_target);
		if (opp_dist < dist2_opp)
			dist2_opp = opp_dist;
	}
    this->min_diff = min_dif;
	update_eval(wm);
	return true;
}
int unmark_pass_to_me::opp_min_dist_passer(const WorldModel & wm,
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
int unmark_pass_to_me::min_opp_recive(const WorldModel & wm, Vector2D ball_pos,
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

bhv_unmark::bhv_unmark() {
	cycle_start = 0;
}
double bhv_unmark::min_tm_dist(const WorldModel & wm, Vector2D tmp) {
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
double bhv_unmark::min_opp_dist(const WorldModel & wm, Vector2D tmp) {
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
double bhv_unmark::min_tm_home_dist(const WorldModel & wm, Vector2D tmp) {
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
bool bhv_unmark::is_good_for_pass_from_last(const WorldModel & wm) {
	if (is_good_for_unmark(wm)) {
        return update_for_pass(wm, passers);
	}
	return false;
}
bool bhv_unmark::is_good_for_unmark_from_last(const WorldModel & wm) { //borna
	if (is_good_for_unmark(wm))
        return update_for_unmark(wm);
	return false;
}
Vector2D bhv_unmark::penalty_unmark(const WorldModel & wm) {
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

bool bhv_unmark::is_good_for_unmark(const WorldModel & wm) {
	Vector2D home_pos = Strategy::i().getPosition(wm.self().unum());
	Vector2D self_pos = wm.self().pos();
    double min_dist_ball = 5;
    double max_dist_home_pos = 15;
    double max_dist_self_pos = 15;
    double max_dist_x = 15;
    double max_x = 52;
    double max_y = 33;
	double min_dist_opp = 5;
	double min_dist_tm = 5;
	bool is_passer_x_more30 = false;
	bool is_passer_near_corner = false;
	int passer_near_target = 0;
	for (auto&passer:passers){
        if(passer.ballpos.x > 30)
            is_passer_x_more30 = true;
        if (passer.ballpos.dist(Vector2D(52, 34)) < 15
            || passer.ballpos.dist(Vector2D(52, -34)) < 15)
            is_passer_near_corner = true;
        dlog.addText(Logger::POSITIONING, "%.1f %.1f , %.1f %.1f , %.1f", target_pos.x, target_pos.y, passer.ballpos.x, passer.ballpos.y,target_pos.dist(passer.ballpos));
        if (target_pos.dist(passer.ballpos) < min_dist_ball)
            passer_near_target += 1;

	}
	if (target_pos.dist(Vector2D(52, 0)) < 20 && is_passer_x_more30
			&& Strategy::i().self_line == Strategy::PostLine::forward)
		min_dist_opp = 1.5;
	if (target_pos.dist(Vector2D(52, 34)) < 20
			|| target_pos.dist(Vector2D(52, -34)) < 20) {
		if (is_passer_near_corner) {
			if (self_pos.dist(Vector2D(52, 34)) < 20
					|| self_pos.dist(Vector2D(52, -34)) < 20) {
				min_dist_opp = 2;
				min_dist_tm = 2;
			}
		}
	}

	if (target_pos.dist(home_pos) > max_dist_home_pos) {
		dlog.addText(Logger::POSITIONING,
				"------cancel home pos max dist");
		return false;
	}
	if (target_pos.dist(self_pos) > max_dist_self_pos) {
		dlog.addText(Logger::POSITIONING,
				"------cancel self pos max dist");
		return false;
	}
	if (passer_near_target == passers.size()) {
		dlog.addText(Logger::POSITIONING,
				"------cancel min ball dist");
		return false;
	}
    double offside_x = wm.offsideLineX();
	if (target_pos.x > offside_x - 0.5) {
		dlog.addText(Logger::POSITIONING,
				"------cancel offside");
		return false;
	}
	if (target_pos.x < home_pos.x - max_dist_x) {
		dlog.addText(Logger::POSITIONING,
				"------cancel home pos x dist");
		return false;
	}
	if (min_opp_dist(wm, target_pos) < min_dist_opp) {
		dlog.addText(Logger::POSITIONING,
				"------cancel min dist opp");
		return false;
	}
	if (min_tm_dist(wm, target_pos) < min_dist_tm) {
		dlog.addText(Logger::POSITIONING,
				"------cancel min dist tm");
		return false;
	}
	if (min_tm_home_dist(wm, target_pos) < home_pos.dist(target_pos) - 10
	    || (target_pos.x < 35 && min_tm_home_dist(wm, target_pos) < home_pos.dist(target_pos) - 5)) {
		dlog.addText(Logger::POSITIONING,
				"------cancel near tm homepos");
		return false;
	}
	if (target_pos.x > max_x) {
		dlog.addText(Logger::POSITIONING, "------cancel max x");
		return false;
	}
	if (target_pos.absY() > max_y) {
		dlog.addText(Logger::POSITIONING, "------cancel max y");
		return false;
	}
	return true;
}

bool bhv_unmark::execute(PlayerAgent * agent) {
	const WorldModel & wm = agent->world();
	Vector2D new_target = target_pos;
	new_target = get_avoid_circle_point(wm, new_target);
	Vector2D self_pos = wm.self().pos();
    Vector2D self_hpos = Strategy::i().getPosition(wm.self().unum());
    agent->debugClient().addCircle(target_pos, 0.3);
    Vector2D ball_inertia = wm.ball().inertiaPoint(wm.interceptTable()->teammateReachCycle());
    if (good_for_pass){
        agent->debugClient().addMessage("passer:%d", best_passer.unum);
        if (best_passer.unum > 0)
            agent->debugClient().addLine(wm.ourPlayer(best_passer.unum)->pos(), best_passer.ballpos);
    }

    double best_pass_eval = -1000;
    unmark_pass_to_me * tmp;
    for(auto &p:passes){
        if(p.pass_eval > best_pass_eval){
            best_pass_eval = p.pass_eval;
            tmp = &p;
        }
    }
    int fastest_tm = wm.interceptTable()->fastestTeammate()->unum();
    int tm_reach_cycle = wm.interceptTable()->teammateReachCycle();
    int opp_reach_cycle = wm.interceptTable()->opponentReachCycle();
    double stamina = wm.self().stamina();
    double dash_power = Strategy::get_normal_dash_power(wm);
    double stamina_z = 1.0;
    if(Strategy::i().tm_Line(wm.self().unum()) == Strategy::PostLine::back)
        stamina_z = 1.4;
    if(Strategy::i().tm_Line(wm.self().unum()) == Strategy::PostLine::half)
        stamina_z = 1.2;
    if(wm.self().unum() == 5)
        stamina_z = 1.3;
	if (self_pos.dist(target_pos) > 0.5) {
		if (fastest_tm == best_passer.unum)
		    if(opp_reach_cycle - tm_reach_cycle < 5)
		        dash_power = 100;
		if (target_pos.dist(Vector2D(52, 0)) < 25)
			dash_power = 100;
		if (min_opp_dist(wm, target_pos) < 5)
			dash_power = 100;
        if (min_opp_dist(wm, self_pos) < 5)
            dash_power = 100;
        if (stamina < 6000 && dash_power > 99){
            dash_power = (Strategy::get_normal_dash_power(wm) + stamina / 8000.0 * dash_power) / 2.0;
        }
        if (stamina < 5000 * stamina_z)
            dash_power = Strategy::get_normal_dash_power(wm);
        if (Strategy::i().tm_Line(wm.self().unum()) != Strategy::PostLine::back){
            if (ball_inertia.x > 10
                && self_hpos.x > 10){
                if(self_pos.dist(self_hpos) > 10)
                    dash_power = 100;
            }
        }
        if (stamina < 4000 * stamina_z)
            dash_power = Strategy::get_normal_dash_power(wm);
        if (Strategy::i().tm_Line(wm.self().unum()) != Strategy::PostLine::back){
            if (ball_inertia.x > 30
                && self_hpos.x > 25){
                    dash_power = 100;
            }
        }
        if (stamina < 3500)
            dash_power = Strategy::get_normal_dash_power(wm);
		agent->debugClient().addMessage("Unmark");
		if (wm.ball().pos().x > 40 && self_pos.x > 40 && penalty_unmark(wm) != Vector2D(0.0,0.0)) {
			target_pos = penalty_unmark(wm);
		}
		agent->debugClient().addCircle(target_pos, 0.3);
        agent->debugClient().setTarget(target_pos);
        dlog.addCircle(Logger::POSITIONING, target_pos, 0.1, 255,255,0, true);
		Body_GoToPoint2010(new_target, 0.3, dash_power).execute(agent);
	}
	else if (abs(target_body - wm.self().body().degree()) > 10
			|| !ServerParam::i().theirPenaltyArea().contains(target_pos)) {
		agent->debugClient().addMessage("UnmarkTurn");
		Body_TurnToAngle(target_body).execute(agent);
	}
	else if (self_pos.dist(Vector2D(52.0, 0.0)) < 25.0) {
		agent->debugClient().addMessage("UnmarkDirect");
		agent->doDash(100, 0);
	}
	if (wm.existKickableOpponent() && wm.ball().distFromSelf() < 18.0) {
		agent->setNeckAction(new Neck_TurnToBall());
	}
	else {
		agent->setNeckAction(new Neck_TurnToBallOrScan());
	}
	return true;
}

void bhv_unmark::set_unmark_angle(const WorldModel & wm) {
	double ball_unpos_angle = (best_passer.ballpos - target_pos).th().degree();
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
bool bhv_unmark::update_for_pass(const WorldModel & wm, vector<unmark_passer> passers) {
	this->passers = passers;
	double div_dist = 1.0;
	double div_angle = 45;
	double max_dist_target = 3;
	for (double dist = 0; dist < max_dist_target; dist += div_dist) {
		for (double angle = 0; angle < 360; angle += div_angle) {
			unmark_pass_to_me pass;
			pass.pass_target = target_pos + Vector2D::polar2vector(dist, angle);
            for(auto &passer: passers){
                pass.passer = passer;
                dlog.addText(Logger::POSITIONING,"-------Pass to (%.1f,%.1f)", pass.pass_target.x, pass.pass_target.y);
                if (pass.update(wm, passer, target_pos)) {
                    #ifdef DEBUG_UNMARK
                    dlog.addText(Logger::POSITIONING,"-------OK can receive pass eval %.1f", pass.pass_eval);
                    #endif
                    passes.push_back(pass);
                }
                else{
                    #ifdef DEBUG_UNMARK
                    dlog.addText(Logger::POSITIONING,"-------Can not receive pass", pass.pass_target.x, pass.pass_target.y);
                    #endif
                }
            }
		}
	}
	if (passes.size() == 0){
        return false;
	}


	eval_for_pass = 0;
	double max_pass_eval = -1000;
	double sum_pass_eval = 0;
	for (int i = 0; i < passes.size(); i++) {
		if (max_pass_eval < passes[i].pass_eval){
            max_pass_eval = passes[i].pass_eval;
            best_pass = passes[i];
            best_passer = passes[i].passer;
		}
		if (passes[i].can_pass)
			sum_pass_eval += passes[i].pass_eval;
	}
    set_unmark_angle(wm);
	eval_for_pass = max_pass_eval + sum_pass_eval / (double) passes.size();
	double angle_move = (target_pos - wm.self().pos()).th().degree();
	double angle_pass = (best_passer.ballpos - wm.self().pos()).th().degree();
	double angle_dif = angle_move - angle_pass;
	if (angle_dif < 0)
		angle_dif *= (-1.0);
	if (angle_dif > 180)
		angle_dif = 360 - angle_dif;
	if (angle_dif > 110)
		eval_for_pass -= 10;
	else if (angle_dif > 75)
		eval_for_pass -= 5;
    #ifdef DEBUG_UNMARK
    dlog.addText(Logger::POSITIONING, "-------Pass Eval %.1f", eval_for_pass);
    #endif
	return true;
}
bool bhv_unmark::update_for_unmark(const WorldModel & wm) {
	set_unmark_angle(wm);
	eval_for_unmark = 0;
	double min_opp_distt = min_opp_dist(wm, target_pos);
	if (min_opp_distt < 1.5){
        return false;
	}

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
    #ifdef DEBUG_UNMARK
    dlog.addText(Logger::POSITIONING,"-------Eval for unmark %.1f", eval_for_unmark);
    #endif
	return true;
}

double bhv_unmarkes::min_tm_dist(const WorldModel & wm, Vector2D tmp) {
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
double bhv_unmarkes::min_opp_dist(const WorldModel & wm, Vector2D tmp) {
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

bool bhv_unmarkes::execute(PlayerAgent * agent) {
	const WorldModel & wm = agent->world();
    #ifdef DEBUG_UNMARK
	dlog.addText(Logger::POSITIONING,"start bhv_unmarks");
    #endif
	if (!can_unmark(wm))
		return false;
    #ifdef DEBUG_UNMARK
	dlog.addText(Logger::POSITIONING,"I can unmark");
    #endif
	vector<unmark_passer> passers = update_passer(wm);
    #ifdef DEBUG_UNMARK
	for(auto &passer: passers){
        dlog.addText(Logger::POSITIONING,"passer:%d in (%.2f,%.2f) after %d, oppminc:%d",passer.unum,passer.ballpos.x,passer.ballpos.y,passer.cycle_recive_ball,passer.oppmin_cycle);
	}
	dlog.addText(Logger::POSITIONING, "last run:%d lastTm:%d last_start:%d", last_run, last_fastest_tm, best_last_unmark.cycle_start);
    #endif

    if (last_fastest_tm == wm.interceptTable()->fastestTeammate()->unum()
        && last_run == wm.time().cycle() - 1){
        if (best_last_unmark.cycle_start >= wm.time().cycle() - 4){
            last_run = wm.time().cycle();
            return best_last_unmark.execute(agent);
        }

        if (best_last_unmark.cycle_start >= wm.time().cycle() - 7){
            Vector2D ball_inertia = wm.ball().inertiaPoint(wm.interceptTable()->teammateReachCycle());
            best_last_unmark.passers = passers;
            if (ball_inertia.dist(last_ball_inertia) < 5
                && (best_last_unmark.is_good_for_pass_from_last(wm)
                    || best_last_unmark.is_good_for_unmark_from_last(wm))){
                #ifdef DEBUG_UNMARK
                dlog.addText(Logger::POSITIONING,"last unmark is good");
                #endif
                last_run = wm.time().cycle();
                return best_last_unmark.execute(agent);
            }
        }
	}



	create_targets(wm);
	evaluate_targets(wm, passers);
    #ifdef DEBUG_UNMARK
	dlog.addText(Logger::POSITIONING,"number of unmark:%d",unmarks.size());
    #endif
	if (unmarks.size() == 0)
		return false;

	update_best_unmark(wm);
    if (best_unmark == NULL)
        return false;

    last_fastest_tm = wm.interceptTable()->fastestTeammate()->unum();
	best_last_unmark = *best_unmark;
    #ifdef DEBUG_UNMARK
	dlog.addText(Logger::POSITIONING,"best is #%d (%.2f,%.2f),%.2f,ep:%.1f,eu:%.1f,ps:%d,forpass:%d",
                    best_unmark->number,
                    best_unmark->target_pos.x,
                    best_unmark->target_pos.y,
                    best_unmark->target_body,
                    best_unmark->eval_for_pass,
                    best_unmark->eval_for_unmark,
                    best_unmark->passes.size(),
                    best_unmark->unmark_for_posses);
    #endif
    last_ball_inertia = wm.ball().inertiaPoint(wm.interceptTable()->teammateReachCycle());
    last_run = wm.time().cycle();
    best_last_unmark.cycle_start = wm.time().cycle();
	return best_unmark->execute(agent);
}

bool bhv_unmarkes::can_unmark(const WorldModel & wm) {
    Strategy::PostLine pl_line = Strategy::i().self_Line();
    int stamina = wm.self().stamina();
    static const Rect2D penalty_area=Rect2D(Vector2D(38,-17),Vector2D(53,17));
    if (wm.ball().inertiaPoint(wm.interceptTable()->teammateReachCycle()).dist(wm.self().pos()) > 30){
        #ifdef unmark_debug
        dlog.addText(Logger::POSITIONING, "Can not unmarking because dist to ball is more than 30");
        #endif
        return false;
    }

    if(wm.ball().inertiaPoint(wm.interceptTable()->teammateReachCycle()).x > 25
        && Strategy::i().getPosition(wm.self().unum()).x > 20){
        #ifdef unmark_debug
        dlog.addText(Logger::POSITIONING, "Can unmarking, because ballx>25 selfhpx > 20");
        #endif
        return true;
    }

    if(Strategy::i().getPosition(wm.self().unum()).dist(wm.self().pos()) > 15){
        #ifdef unmark_debug
        dlog.addText(Logger::POSITIONING, "Can not unmarking, because dist to hpos is more than 15");
        #endif
        return false;
    }

    if(penalty_area.contains(wm.self().pos())){
        #ifdef unmark_debug
        dlog.addText(Logger::POSITIONING, "Can unmarking because in pen area");
        #endif
        return true;
    }

    if (pl_line == Strategy::PostLine::back) {
        if (stamina > 5500){
            #ifdef unmark_debug
            dlog.addText(Logger::POSITIONING, "Can unmarking because back stamina > 5500");
            #endif
            return true;
        }
    }
    else if (pl_line == Strategy::PostLine::half) {
        if (stamina > 4500){
            #ifdef unmark_debug
            dlog.addText(Logger::POSITIONING, "Can unmarking because half stamina > 4500");
            #endif
            return true;
        }
    }
    else {
        if (stamina > 3500){
            #ifdef unmark_debug
            dlog.addText(Logger::POSITIONING, "Can unmarking because forward stamina > 3500");
            #endif
            return true;
        }
    }
    #ifdef unmark_debug
    dlog.addText(Logger::POSITIONING, "Can not unmarking no reason");
    #endif
	return false;
}
vector<unmark_passer> bhv_unmarkes::update_passer(const WorldModel & wm) {
	vector<unmark_passer> res;
    const PlayerObject * tm = wm.interceptTable()->fastestTeammate();
	if (tm != NULL && tm->unum() > 0) {
		int tm_cycle = wm.interceptTable()->teammateReachCycle();
		int opp_cycle = wm.interceptTable()->opponentReachCycle();
		Vector2D ball_pos = wm.ball().inertiaPoint(tm_cycle);
		vector<pair<double, int>> tm_passer;
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
			Circle2D tmp_area = Circle2D((ball_pos + wm.self().pos()) / 2.0, max(ball_pos.dist(wm.self().pos()) / 2.0, 10.0) + 5.0);
			if (!tmp_area.contains(tmp->pos()))
				continue;
			double eval = ((ball_pos + wm.self().pos()) / 2.0).dist(tmp->pos());
			tm_passer.push_back(pair<double, int>(eval, i));
		}
        sort(tm_passer.begin(), tm_passer.end());
        //sort and select 2
        for (auto& t:tm_passer){
            unmark_passer passer = unmark_passer(t.second,
                                                 wm.ourPlayer(t.second)->pos(), ball_pos.dist(wm.ourPlayer(t.second)->pos()) / 2.5, opp_cycle);
            passer.is_fastest = false;
            res.push_back(passer);
            if (res.size() == 2)
                break;
        }
		int unum = tm->unum();
		unmark_passer passer = unmark_passer(unum, ball_pos, tm_cycle,
				opp_cycle);
		passer.is_fastest = true;
		res.push_back(passer);
	}
	return res;
}

void bhv_unmarkes::create_targets(const WorldModel &wm){
    #ifdef DEBUG_UNMARK
    dlog.addText(Logger::POSITIONING, "create targets");
    #endif
    Vector2D home_pos = Strategy::i().getPosition(wm.self().unum());
    Vector2D self_pos = wm.self().pos();
    if (home_pos.dist(self_pos) < 7 || (home_pos.dist(self_pos) < 15 && home_pos.x > 30)){
        double div_dist = 1.0;
        double max_dist_to_pos = 10;
        for (double dist = 0; dist < max_dist_to_pos; dist += div_dist) {
            double div_angle = std::max(30.0, 90.0 / (dist + 1.0));
            for (double angle = 0; angle < 360; angle += div_angle) {
                bhv_unmark tmp_unmark;
                tmp_unmark.target_pos = self_pos
                                        + Vector2D::polar2vector(dist, angle);
                tmp_unmark.dist2self = tmp_unmark.target_pos.dist(self_pos); //borna
                #ifdef DEBUG_UNMARK
                dlog.addText(Logger::POSITIONING, "--unmark_target(%.2f,%.2f)",
                             tmp_unmark.target_pos.x, tmp_unmark.target_pos.y);
                #endif
                tmp_unmark.number = unmarks.size();
                unmarks.push_back(tmp_unmark);
                if (dist == 0)
                    break;
            }
        }
    }
    if (home_pos.dist(self_pos) >= 7){
        AngleDeg self_to_hpos = (home_pos - self_pos).th();
        double max_dist = std::min(home_pos.dist(self_pos) * 1.5, home_pos.dist(self_pos) + 5.0);
        double div_dist = 1.0;
        double max_dist_to_pos = 8;
        for (double dist = 0; dist < max_dist; dist += div_dist) {
            double max_angle = atan(5.0 / std::max(home_pos.dist(self_pos), 1.0)) * 180.0 / 3.14;
            if (dist < max_dist / 2.0)
                max_angle *= 2.0;
            double div_angle = 10;//std::max(30.0, 90.0 / (dist + 1.0));
            for (double angle = 0; angle <= max_angle; angle += div_angle) {
                for(int left_right = -1; left_right <= 1; left_right +=2){
                    bhv_unmark tmp_unmark;
                    tmp_unmark.target_pos = self_pos + Vector2D::polar2vector(dist, self_to_hpos + AngleDeg(left_right * angle));
                    if (tmp_unmark.target_pos.dist(home_pos) > 15)
                        continue;
                    tmp_unmark.dist2self = tmp_unmark.target_pos.dist(self_pos); //borna
                    #ifdef DEBUG_UNMARK
                    dlog.addText(Logger::POSITIONING, "--unmark_target(%.2f,%.2f)",
                                 tmp_unmark.target_pos.x, tmp_unmark.target_pos.y);
                    #endif
                    tmp_unmark.number = unmarks.size();
                    unmarks.push_back(tmp_unmark);
                    if (angle == 0)
                        break;
                }
                if (dist == 0)
                    break;
            }
        }
    }
}

void bhv_unmarkes::evaluate_targets(const WorldModel &wm, vector <unmark_passer> passers) {
    #ifdef DEBUG_UNMARK
	dlog.addText(Logger::POSITIONING, "create unmarks");
    #endif
	if (passers.size() > 0) {
		for (auto& u: unmarks){
            u.passers = passers;
            #ifdef DEBUG_UNMARK
            dlog.addText(Logger::POSITIONING, "#%d--unmark_target(%.2f,%.2f), start checking ...",
                         u.number, u.target_pos.x, u.target_pos.y);
            #endif
            if (u.is_good_for_unmark(wm)) {
                if (u.update_for_pass(wm, passers)){
                    #ifdef DEBUG_UNMARK
                    dlog.addText(Logger::POSITIONING, "#%d--unmark_target(%.2f,%.2f), is good for pass",
                                 u.number, u.target_pos.x, u.target_pos.y);
                    dlog.addCircle(Logger::POSITIONING,u.target_pos.x, u.target_pos.y,
                                   0.3, 58, 44, 255, false);
                    dlog.addMessage(Logger::POSITIONING,u.target_pos.x + 0.1,
                                    u.target_pos.y + 0.1,
                                    to_string(u.number).c_str(), 255, 255, 255);
                    #endif
                    u.good_for_pass = true;
                }
                else if (u.update_for_unmark(wm)){
                    #ifdef DEBUG_UNMARK
                    dlog.addText(Logger::POSITIONING, "#%d--unmark_target(%.2f,%.2f), is good for unmark",
                                 u.number, u.target_pos.x, u.target_pos.y);
                    dlog.addCircle(Logger::POSITIONING,u.target_pos.x, u.target_pos.y,
                                   0.3, 247, 0, 255, false);
                    dlog.addMessage(Logger::POSITIONING,u.target_pos.x + 0.1,
                                    u.target_pos.y + 0.1,
                                    to_string(u.number).c_str(), 255, 255, 255);
                    #endif
                    u.good_for_unmark = true;
                }
                else{
                    #ifdef DEBUG_UNMARK
                    dlog.addText(Logger::POSITIONING, "#%d--unmark_target(%.2f,%.2f), "
                                                      "is not good, not pass not unmark",
                                 u.number, u.target_pos.x, u.target_pos.y);
                    dlog.addCircle(Logger::POSITIONING,u.target_pos.x, u.target_pos.y,
                                   0.3, 255, 150, 44, false);
                    dlog.addMessage(Logger::POSITIONING,u.target_pos.x + 0.1,
                                    u.target_pos.y + 0.1,
                                    to_string(u.number).c_str(), 255, 255, 255);
                    #endif
                }
            }else{
                #ifdef DEBUG_UNMARK
                dlog.addText(Logger::POSITIONING, "#%d--unmark_target(%.2f,%.2f), is not good for unmark",
                             u.number, u.target_pos.x, u.target_pos.y);
                dlog.addCircle(Logger::POSITIONING,u.target_pos.x, u.target_pos.y,
                               0.3, 0, 0, 0, false);
                dlog.addMessage(Logger::POSITIONING,u.target_pos.x + 0.1,
                                u.target_pos.y + 0.1,
                                to_string(u.number).c_str(), 0, 0, 0);
                #endif
            }
		}
	}
    #ifdef DEBUG_UNMARK
    for (auto& u: unmarks){
        dlog.addText(Logger::POSITIONING, "#%d pass:%.1f", u.number, u.eval_for_pass);
    }
    for (auto& u: unmarks){
        if(u.good_for_pass)
            dlog.addText(Logger::POSITIONING, "#%d x:%.1f goal:%.1f opp:%.1f tar:%.1f hpos:%.1f pos:%.1f tm:%.1f fast:%.1f ball:%.1f pass:%.1f diff:%.1f", u.number,
                         u.best_pass.e_x,
                         u.best_pass.e_d_goal,
                         u.best_pass.e_d_opp,
                         u.best_pass.e_d_target,
                         u.best_pass.e_d_hpos,
                         u.best_pass.e_d_pos,
                         u.best_pass.e_d_tm,
                         u.best_pass.e_fastest,
                         u.best_pass.e_d_ball,
                         u.best_pass.e_pass,
                         u.best_pass.e_diff);

    }
    #endif
}
void bhv_unmarkes::update_best_unmark(const WorldModel & wm) { //borna
    best_unmark = NULL;
    #ifdef DEBUG_UNMARK
    int fastest_tm = wm.interceptTable()->fastestTeammate()->unum();
    double max_eval_pass = -1000;
    double min_eval_pass = 1000;
    double max_eval_unmark = -1000;
    double min_eval_unmark = 1000;
    for (int i = 0; i < unmarks.size(); i++) {
        if(unmarks[i].good_for_pass){
            if(unmarks[i].eval_for_pass > max_eval_pass)
                max_eval_pass = unmarks[i].eval_for_pass;
            if(unmarks[i].eval_for_pass < min_eval_pass)
                min_eval_pass = unmarks[i].eval_for_pass;
        }else if(unmarks[i].good_for_unmark){
            if(unmarks[i].eval_for_unmark > max_eval_unmark)
                max_eval_unmark = unmarks[i].eval_for_unmark;
            if(unmarks[i].eval_for_unmark < min_eval_unmark)
                min_eval_unmark = unmarks[i].eval_for_unmark;
        }

    }
    if (max_eval_pass == min_eval_pass){
        for (int i = 0; i < unmarks.size(); i++) {
            if(unmarks[i].good_for_pass){
                dlog.addCircle(Logger::POSITIONING,unmarks[i].target_pos.x, unmarks[i].target_pos.y, 0.2, 0, 0, 255, true);
                if(unmarks[i].best_passer.unum == fastest_tm)
                    dlog.addCircle(Logger::POSITIONING,unmarks[i].target_pos.x, unmarks[i].target_pos.y, 0.05, 255, 255, 255, true);
            }
        }
    }
    else{
        for (int i = 0; i < unmarks.size(); i++) {
            if(unmarks[i].good_for_pass){
                int r = 179 - static_cast<int>((unmarks[i].eval_for_pass - min_eval_pass)/(max_eval_pass - min_eval_pass) * 179);
                int g = 171 - static_cast<int>((unmarks[i].eval_for_pass - min_eval_pass)/(max_eval_pass - min_eval_pass) * 171);;
                int b = 255;
                dlog.addCircle(Logger::POSITIONING,unmarks[i].target_pos.x, unmarks[i].target_pos.y, 0.3, r, g, b, true);
                if(unmarks[i].best_passer.unum == fastest_tm)
                    dlog.addCircle(Logger::POSITIONING,unmarks[i].target_pos.x, unmarks[i].target_pos.y, 0.1, 255, 255, 255, true);
            }
        }
    }

    if (max_eval_unmark == min_eval_unmark){
        for (int i = 0; i < unmarks.size(); i++) {
            if(unmarks[i].good_for_unmark){
                dlog.addCircle(Logger::POSITIONING,unmarks[i].target_pos.x, unmarks[i].target_pos.y, 0.3, 247, 0, 255, true);
            }
        }
    }
    else{
        for (int i = 0; i < unmarks.size(); i++) {
            if(unmarks[i].good_for_unmark){
                int r = 255 - static_cast<int>((unmarks[i].eval_for_unmark - min_eval_unmark)/(max_eval_unmark - min_eval_unmark) * 10);
                int g = 180 - static_cast<int>((unmarks[i].eval_for_unmark - min_eval_unmark)/(max_eval_unmark - min_eval_unmark) * 180);;
                int b = 255;
                dlog.addCircle(Logger::POSITIONING,unmarks[i].target_pos.x, unmarks[i].target_pos.y, 0.3, r, g, b, true);
            }
        }
    }
    #endif

	double max_eval = -1000;
	for (int i = 0; i < unmarks.size(); i++) {
		if (unmarks[i].good_for_pass) {
                if (max_eval < unmarks[i].eval_for_pass) {
                    best_unmark = &unmarks[i];
                    max_eval = unmarks[i].eval_for_pass;
                }
//            }
        }
	}
    if (max_eval != -1000)
        return;
    for (int i = 0; i < unmarks.size(); i++) {
        if (unmarks[i].good_for_unmark)
            if (max_eval < unmarks[i].eval_for_unmark) {
                best_unmark = &unmarks[i];
                max_eval = unmarks[i].eval_for_unmark;
            }
    }
}

Vector2D bhv_unmark::get_avoid_circle_point( const WorldModel & wm,
                        const Vector2D & target_point )
{
    const ServerParam & SP = ServerParam::i();

    const double avoid_radius
            = 3 + wm.self().playerType().playerSize();
    const Circle2D ball_circle( wm.ball().pos(), avoid_radius );

    if ( can_go_to( -1, wm, ball_circle, target_point ) )
    {
        return target_point;
    }

    AngleDeg target_angle = ( target_point - wm.self().pos() ).th();
    AngleDeg ball_target_angle = ( target_point - wm.ball().pos() ).th();
    bool ball_is_left = wm.ball().angleFromSelf().isLeftOf( target_angle );

    const int ANGLE_DIVS = 6;
    std::vector< Vector2D > subtargets;
    subtargets.reserve( ANGLE_DIVS );

    const int angle_step = ( ball_is_left ? 1 : -1 );

    int count = 0;
    int a = angle_step;

    for ( int i = 1; i < ANGLE_DIVS; ++i, a += angle_step, ++count )
    {
        AngleDeg angle = ball_target_angle + (180.0/ANGLE_DIVS)*a;
        Vector2D new_target = wm.ball().pos()
                              + Vector2D::from_polar( avoid_radius + 1.0, angle );

        if ( new_target.absX() > SP.pitchHalfLength() + SP.pitchMargin() - 1.0
             || new_target.absY() > SP.pitchHalfWidth() + SP.pitchMargin() - 1.0 )
        {
            break;
        }

        if ( can_go_to( count, wm, ball_circle, new_target ) )
        {
            return new_target;
        }
    }

    a = -angle_step;
    for ( int i = 1; i < ANGLE_DIVS*2; ++i, a -= angle_step, ++count )
    {
        AngleDeg angle = ball_target_angle + (180.0/ANGLE_DIVS)*a;
        Vector2D new_target = wm.ball().pos()
                              + Vector2D::from_polar( avoid_radius + 1.0, angle );

        if ( new_target.absX() > SP.pitchHalfLength() + SP.pitchMargin() - 1.0
             || new_target.absY() > SP.pitchHalfWidth() + SP.pitchMargin() - 1.0 )
        {
            break;
        }

        if ( can_go_to( count, wm, ball_circle, new_target ) )
        {
            return new_target;
        }
    }

    return target_point;
}

bool
bhv_unmark::can_go_to( const int count,
           const WorldModel & wm,
           const Circle2D & ball_circle,
           const Vector2D & target_point )
{
    Segment2D move_line( wm.self().pos(), target_point );

    int n_intersection = ball_circle.intersection( move_line, NULL, NULL );

    if ( n_intersection == 0 )
    {
        return true;
    }

    if ( n_intersection == 1 )
    {
        AngleDeg angle = ( target_point - wm.self().pos() ).th();

        if ( ( angle - wm.ball().angleFromSelf() ).abs() > 80.0 )
        {
            return true;
        }
    }

    return false;
}