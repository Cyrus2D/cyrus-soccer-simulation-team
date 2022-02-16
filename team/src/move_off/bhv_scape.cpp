/*
 * bhv_scape.cpp
 *
 *  Created on: Jun 25, 2017
 *      Author: nader
 */

#include "bhv_scape.h"
#include <rcsc/game_time.h>
#include <rcsc/geom/vector_2d.h>
#include <rcsc/geom/angle_deg.h>
#include <rcsc/player/soccer_intention.h>
#include <rcsc/common/logger.h>
#include <vector>
#include "../strategy.h"
#include "../bhv_basic_move.h"
#include "../setting.h"
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/common/server_param.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/abstract_player_object.h>
#include <rcsc/player/world_model.h>
#include <rcsc/action/body_turn_to_angle.h>
#include <rcsc/action/neck_turn_to_ball.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
using namespace rcsc;
using namespace std;

bool bhv_scape::can_scape(const WorldModel & wm){
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();
    Vector2D ball_iner = wm.ball().inertiaPoint(std::min(std::min(opp_min,mate_min),self_min));
    int unum = wm.self().unum();
    double stamina = wm.self().stamina();
    const Vector2D target_point = Strategy::i().getPosition( unum );
    Vector2D self_pos = wm.self().pos();
    double offside = std::max(wm.offsideLineX(),ball_iner.x) - 0.3;
    int passer = 0;
    if (wm.interceptTable()->fastestTeammate() != NULL)
        passer = wm.interceptTable()->fastestTeammate()->unum();
    if(passer < 1)
        return false;
    if( Strategy::i().self_Line() != Strategy::PostLine::forward )
        return false;
    if(stamina < 5500)
        return false;
    if(ball_iner.x< -25)
        return false;
    if(ball_iner.x > 34)
        return false;
    if(target_point.x < offside - 8)
        return false;
    if(self_pos.dist(target_point) > 20)
        return false;
    if(std::abs(target_point.x - self_pos.x) > 12)
        return false;
    if (self_pos.x > offside)
        return false;
    if(ball_iner.dist(target_point) > 35
            || ball_iner.dist(self_pos) > 35)
        return false;
    int fastest_tm = 0;
    if (wm.interceptTable()->fastestTeammate() != nullptr && wm.interceptTable()->fastestTeammate()->unum() > 0){
        fastest_tm = wm.interceptTable()->fastestTeammate()->unum();
    }
    if (fastest_tm == 9 && wm.self().unum() == 10)
        return false;
    if (fastest_tm == 10 && wm.self().unum() == 11)
        return false;
//    if(ball_iner.x < target_point.x - 20)
//        return false;
    if(Setting::i()->mOffensiveMove->mIs9BrokeOffside
            && Strategy::i().get_formation_type() == Strategy::FormationType::F433
            && unum == 11
            && ball_iner.y < -10
            && passer != 9 && passer != 10
            && ball_iner.x < 25)
        return false;

    return true;
}

bool bhv_scape::execute(rcsc::PlayerAgent * agent ){
	const WorldModel & wm = agent->world();
	const int self_min = wm.interceptTable()->selfReachCycle();
	const int mate_min = wm.interceptTable()->teammateReachCycle();
	const int opp_min = wm.interceptTable()->opponentReachCycle();
	Vector2D ball_iner = wm.ball().inertiaPoint(std::min(std::min(opp_min,mate_min),self_min));
	int unum = wm.self().unum();
	int stamina = wm.self().stamina();
	const Vector2D target_point = Strategy::i().getPosition( wm.self().unum() );
	double dash_power = Strategy::get_normal_dash_power( wm );
	Vector2D self_pos = wm.self().pos();
    double offside = std::max(wm.offsideLineX(),ball_iner.x) - 0.3;

    if(!can_scape(wm))
        return false;
    int passer = wm.interceptTable()->fastestTeammate()->unum();
	if(last_new_scape.M_start_cycle < wm.time().cycle() + 5){
		if(last_new_scape.M_passer == passer){
			if(last_new_scape.M_first_target.dist(wm.self().pos()) > 0.5){
				if(abs(last_new_scape.M_offside - offside) < 2.5 ){
					if(last_new_scape.M_ball_pos.dist(ball_iner) < 6){
                        last_new_scape.M_first_target.x = offside;
						run_last_scape(agent);
						return true;
					}
				}
			}
		}
	}
	vector<scape>scapes;
    double max_y_dist = 7;
    if(wm.self().unum()==9)
        max_y_dist = 3;
    for(double ft = -max_y_dist;ft<=max_y_dist;ft+=1){
        Vector2D first_taret = Vector2D(offside ,target_point.y + ft);
		if (first_taret.absX() > 50 || first_taret.absY() > 32.5)
			continue;
        if(is_near_to_tm_home_pos(wm,first_taret))
            continue;
        //		dlog.addLine(Logger::POSITIONING,self_pos,first_taret);
		scape tmp_scape = scape(first_taret,wm.time().cycle(),offside,ball_iner,passer);
		for(double scape_angle = -50;scape_angle <=50;scape_angle+=10){
//			dlog.addLine(Logger::POSITIONING,first_taret,first_taret + Vector2D::polar2vector(40,scape_angle));
			Line2D scape_line = Line2D(first_taret,scape_angle);
			for(double pass_angle = -90;pass_angle <=90;pass_angle+=5){
//				dlog.addLine(Logger::POSITIONING,ball_iner,ball_iner + Vector2D::polar2vector(50,pass_angle));
				Line2D pass_line = Line2D(ball_iner,pass_angle);
				Vector2D pass_target = pass_line.intersection(scape_line);
				if(pass_target.isValid() && pass_target.x > first_taret.x && pass_target.absY() < 33 && pass_target.absX() < 47){
					double dash_dist_from_first = pass_target.dist(first_taret);
					double pass_dist = pass_target.dist(ball_iner);

					double first_pass_vel_r = (pass_dist * (1.0 - ServerParam::i().ballDecay())) / (1.0 - pow(ServerParam::i().ballDecay() , (int)dash_dist_from_first));
					if(first_pass_vel_r > 3.0)
						first_pass_vel_r = 3.0;
					Vector2D first_pass_vel = Vector2D::polar2vector(first_pass_vel_r,(pass_target - ball_iner).th());
					Vector2D ball_pos = ball_iner;
					Vector2D ball_vel = first_pass_vel;
//					dlog.addText(Logger::POSITIONING,"ft(%.1f,%.1f)pt(%.1f,%.1f)dashFf%.1f passD%.1f,firstvelr%.1f",first_taret.x,first_taret.y,pass_target.x,pass_target.y,dash_dist_from_first,pass_dist,first_pass_vel_r);
					bool ball_reach = true;
					int pass_cycle = 0;
					double move_dist = 0;
					//					int sc = wm.self().playerTypePtr()->cyclesToReachDistance(wm.self().pos().dist(pass_target));
					for(int i=1;i<=40;i++){
						ball_pos+=ball_vel;
						move_dist += ball_vel.r();
						if(move_dist >= pass_target.dist(ball_iner)){
							ball_reach = true;
							pass_cycle = i;
							break;
						}
						ball_vel *= ServerParam::i().ballDecay();
					}
					if(!ball_reach){
//						dlog.addCircle(Logger::POSITIONING,pass_target,0.2,0,255,0);
						continue;
					}
					bool opp_reach = false;
					ball_pos = ball_iner;
					ball_vel = first_pass_vel;

					for(int bs = 1;bs <=pass_cycle;bs++){
						ball_pos += ball_vel;
						for(int o = 1;o<=11;o++){
							const AbstractPlayerObject * opp = wm.theirPlayer(o);
							if(opp==NULL || opp->unum() != o)
								continue;
							Vector2D opp_pos = opp->pos();
							double dist = opp_pos.dist(ball_pos);
							dist += 4.0;
							if ( dist < bs){
//								dlog.addText(Logger::POSITIONING,"opp%d in (%.1f,%.1f) in c:%d",opp->unum(),ball_pos.x,ball_pos.y,bs);
								opp_reach = true;
								break;
							}
						}
						if(opp_reach)
							break;

						ball_vel *= ServerParam::i().ballDecay();
					}
					if(opp_reach){
//						dlog.addCircle(Logger::POSITIONING,pass_target,0.2,255,0,0);
						continue;
					}
//					dlog.addCircle(Logger::POSITIONING,pass_target,0.2,0,0,255);
					tmp_scape.M_last_targets.push_back(pass_target);

				}
			}
		}
		if(tmp_scape.M_last_targets.size() > 0)
			scapes.push_back(tmp_scape);
	}
	if(scapes.size()==0)
		return false;
	//eval
	for(int i=0;i<scapes.size();i++){
		double eval = scapes[i].M_last_targets.size();
		double best_pass = 100;

		for(int j=0;j<scapes[i].M_last_targets.size();j++){
			double dist = scapes[i].M_last_targets[j].dist(Vector2D(52,0));
			if(dist<best_pass)
				best_pass = dist;
		}
		best_pass = 100 - best_pass; // change
		eval += best_pass;
		double dist=100;
		int onum;
		for(int o = 1;o<=11;o++){
			const AbstractPlayerObject * opp = wm.theirPlayer(o);
			if(opp==NULL || opp->unum() != o)
				continue;
			Vector2D opp_pos = opp->pos();
			if(opp_pos.dist(scapes[i].M_first_target) < dist){
				dist = opp_pos.dist(scapes[i].M_first_target);
				onum = o;
			}
		}

		//		eval -= ((dist * 3)*(best_pass	)/5);
		scapes[i].eval = eval - dist;
	}

	//best
	double max_val = 0;
	//	last_new_scape =
	for(int i=0;i<scapes.size();i++){
		if(scapes[i].eval > max_val){
			max_val = scapes[i].eval;
			last_new_scape = scapes[i];
		}
	}
	run_last_scape(agent);
	return true;
}
bool bhv_scape::run_last_scape(PlayerAgent * agent){

	const WorldModel & wm = agent->world();
	const int self_min = wm.interceptTable()->selfReachCycle();
	const int mate_min = wm.interceptTable()->teammateReachCycle();
	const int opp_min = wm.interceptTable()->opponentReachCycle();
	Vector2D ball_iner = wm.ball().inertiaPoint(std::min(std::min(opp_min,mate_min),self_min));
	int unum = wm.self().unum();
	int stamina = wm.self().stamina();
	const Vector2D target_point = Strategy::i().getPosition( wm.self().unum() );
	double dash_power = Strategy::get_normal_dash_power( wm );
	Vector2D self_pos = wm.self().pos();
	double offside = wm.offsideLineX();

	Vector2D best_target;
	double min_dist = 100;
	for(int i=0;i<last_new_scape.M_last_targets.size();i++){
		double dist = last_new_scape.M_last_targets[i].dist(Vector2D(52,0));
		if(dist<min_dist){
			min_dist = dist;
			best_target = last_new_scape.M_last_targets[i];
		}
	}
	if(-self_pos.x+last_new_scape.M_first_target.x < 3){
		double tar_dif_dir = ((best_target - self_pos).th()-wm.self().body()).abs();
		if(tar_dif_dir > 180)
			tar_dif_dir = 360- tar_dif_dir;
		if(tar_dif_dir < 10){
			//			agent->debugClient().addMessage("scape to (%.1f,%.1f)",last_new_scape.M_first_target.x,last_new_scape.M_first_target.y);
			agent->doDash(100,0);
			agent->debugClient().addMessage("KOSSHER");

			agent->setNeckAction( new Neck_TurnToBallOrScan() );
			return true;
		}else{
			agent->debugClient().addMessage("SCAPETURNING");

			//			agent->debugClient().addMessage("scape to (%.1f,%.1f)",last_new_scape.M_first_target.x,last_new_scape.M_first_target.y);
			Body_TurnToAngle((best_target - self_pos).th()).execute(agent);
			agent->setNeckAction( new Neck_TurnToBallOrScan() );
			return true;
		}

	}else {
		agent->debugClient().addMessage("SCAPING");
//		dlog.addCircle(Logger::POSITIONING,last_new_scape.M_first_target,1,0,258);
		//		agent->debugClient().addCircle(last_new_scape.M_first_target,2);
		agent->debugClient().addMessage("scape to (%.1f,%.1f)",last_new_scape.M_first_target.x,last_new_scape.M_first_target.y);
		Body_GoToPoint2010(last_new_scape.M_first_target,0.1,100).execute(agent);

//		agent->doPointto(last_new_scape.M_first_target.x,last_new_scape.M_first_target.y);

		agent->setNeckAction( new Neck_TurnToBallOrScan() );
		return true;
	}


}
//bool bhv_scape::execute(rcsc::PlayerAgent * agent ){
//	const WorldModel & wm = agent->world();
//	const int self_min = wm.interceptTable()->selfReachCycle();
//	const int mate_min = wm.interceptTable()->teammateReachCycle();
//	const int opp_min = wm.interceptTable()->opponentReachCycle();
//	Vector2D ball_iner = wm.ball().inertiaPoint(std::min(std::min(opp_min,mate_min),self_min));
//	int unum = wm.self().unum();
//	int stamina = wm.self().stamina();
//	const Vector2D target_point = Strategy::i().getPosition( wm.self().unum() );
//	double dash_power = Strategy::get_normal_dash_power( wm );
//	Vector2D self_pos = wm.self().pos();
//	double offside = wm.offsideLineX();
//
//	if(unum < 7)
//		return false;
//	if(stamina < 5500)
//		return false;
//	if(ball_iner.x<-10)
//		return false;
//	if(ball_iner.x > 40)
//		return false;
//	if(target_point.x < offside - 8)
//		return false;
//	if(self_pos.dist(target_point) > 15)
//		return false;
//	if(std::abs(target_point.x - self_pos.x) > 8)
//		return false;
//	if (self_pos.x>offside-0.6)
//		return false;
//	if(ball_iner.dist(target_point) > 35 ||ball_iner.dist(self_pos) > 35)
//		return false;
//	double opp_offside = wm.offsideLineX();
//	Vector2D ts[5];
//	static int cycle_select = 0;
//	ts[0] = Vector2D(opp_offside,target_point.y + 7);
//	ts[1] = Vector2D(opp_offside,target_point.y - 7);
//	ts[2] = Vector2D(opp_offside,target_point.y);
//	ts[3] = Vector2D(opp_offside,target_point.y + 3);
//	ts[4] = Vector2D(opp_offside,target_point.y - 3);
//
//	double max_dist = 0;
//	static int witch_t = 0;
//	if(wm.time().cycle() > cycle_select + 5){
//		cycle_select = wm.time().cycle();
//		for(int t=0;t<5;t++){
//			double min_dist = 100;
//			for(int i=1;i<=11;i++){
//				const AbstractPlayerObject * opp = wm.theirPlayer(i);
//				if(opp!=NULL && opp->unum()>0){
//					if(opp->pos().dist(ts[i])< min_dist)
//						min_dist = opp->pos().dist(ts[i]);
//				}
//			}
//			if(ts[t].absX() > 50 || ts[t].absY() > 33)
//				continue;
//			if(min_dist>max_dist){
//				max_dist = min_dist;
//				witch_t = t;
//			}
//		}
//	}
//	if(self_pos.x < offside - 2){
//		if(Bhv_BasicMove().off_gotopoint(agent,ts[witch_t],0.5,100)){
//
//		}
//		else Body_GoToPoint2010(ts[witch_t],0.5,100).execute(agent);
//		agent->doPointto(ts[witch_t].x,ts[witch_t].y);
//		agent->setNeckAction( new Neck_TurnToBallOrScan() );
//		return true;
//	}else{
//		if(std::abs(wm.self().body().degree()) < 10){
//			agent->doDash(100,0);
//			agent->doPointto(ts[witch_t].x,ts[witch_t].y);
//			agent->setNeckAction( new Neck_TurnToBallOrScan() );
//			return true;
//		}else{
//			Body_TurnToAngle(0).execute(agent);
//			agent->doPointto(ts[witch_t].x,ts[witch_t].y);
//			agent->setNeckAction( new Neck_TurnToBallOrScan() );
//			return true;
//		}
//	}
//	return false;
//}
