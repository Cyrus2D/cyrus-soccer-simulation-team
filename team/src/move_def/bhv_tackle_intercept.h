/*
 * bhv_tackle_intercept.h
 *
 *  Created on: Jun 25, 2017
 *      Author: nader
 */

#ifndef SRC_BHV_TACKLE_INTERCEPT_H_
#define SRC_BHV_TACKLE_INTERCEPT_H_

#include <rcsc/game_time.h>
#include <rcsc/geom/vector_2d.h>

#include <rcsc/player/soccer_intention.h>
#include <rcsc/player/intercept_table.h>
#include <vector>
#include "bhv_basic_tackle.h"
#include <rcsc/player/world_model.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/body_turn_to_angle.h>
#include <rcsc/action/body_turn_to_point.h>
#include <rcsc/action/neck_turn_to_ball.h>
#include <rcsc/player/action_effector.h>
#include <rcsc/geom.h>
using namespace std;
using namespace rcsc;

class bhv_tackle_intercept{
public:
        bool execute(PlayerAgent * agent){
//            return false;
            const WorldModel & wm = agent->world();
            pair<int, AngleDeg> intercept_info = intercept_cycle(wm);
            if(intercept_info.first != 1000){
                Vector2D ballpos = wm.ball().inertiaPoint(intercept_info.first);
                Vector2D self_ball = ballpos - wm.self().inertiaPoint(1);
                Vector2D self_ball_rotate = self_ball.rotate(-wm.self().body());
                if(self_ball_rotate.x < 0 || self_ball_rotate.absY() > ServerParam::i().tackleWidth()){
                    Body_TurnToAngle(intercept_info.second).execute(agent);
                    agent->setNeckAction( new Neck_TurnToBall() );
                    agent->debugClient().addMessage("tackle intercept turn %.1f",intercept_info.second.degree());
                    return true;
                }
                agent->doDash(100);
                Vector2D next_pos = agent->effector().queuedNextMyPos();
                self_ball = ballpos - next_pos;
                self_ball_rotate = self_ball.rotate(-wm.self().body());
                if(self_ball_rotate.x < 0){
                    agent->doDash(70);
                    next_pos = agent->effector().queuedNextMyPos();
                    self_ball = ballpos - next_pos;
                    self_ball_rotate = self_ball.rotate(-wm.self().body());
                    if(self_ball_rotate.x < 0){
                        agent->doDash(40);
                        next_pos = agent->effector().queuedNextMyPos();
                        self_ball = ballpos - next_pos;
                        self_ball_rotate = self_ball.rotate(-wm.self().body());
                        if(self_ball_rotate.x < 0){
                            Body_TurnToAngle(intercept_info.second).execute(agent);
                        }
                    }
                }
                agent->setNeckAction( new Neck_TurnToBall() );
                agent->debugClient().addMessage("tackle intercept dash");
                return true;
            }else{
                return false;
            }
        }

        static pair<int,AngleDeg> intercept_cycle(const WorldModel & wm){
            dlog.addText(Logger::INTERCEPT,"*********calc tackle intercept");
            Vector2D ballpos = wm.ball().pos();
            Vector2D ballvel = wm.ball().vel();
            const int self_min = wm.interceptTable()->selfReachCycle();
            const int mate_min = wm.interceptTable()->teammateReachCycle();
            const int opp_min = wm.interceptTable()->opponentReachCycle();

            double best_eval = 0;
            pair<int,AngleDeg> best_tackle = make_pair(1000,AngleDeg(0));
            for(int c = 1; c<=std::min(self_min,std::min(mate_min,opp_min)); c++){
                if(ballpos.absX() > 52.5 || ballpos.absY() > 34)
                    break;
                ballpos += ballvel;

                double next_body;
                int n_turn = predict_player_turn_cycle_direct_tackle(wm.self().playerTypePtr(),
                                                                     wm.self().body(),
                                                                     wm.self().vel().r(),
                                                                     wm.self().pos().dist(ballpos),
                                                                     (ballpos - wm.self().pos()).th(),
                                                                     ServerParam::i().tackleWidth(),
                                                                     wm.self().pos(),
                                                                     wm.self().vel(),
                                                                     ballpos,
                                                                     next_body,
                                                                     true);

                double dist = wm.self().inertiaPoint(n_turn).dist(ballpos);
                dist -= ServerParam::i().tackleDist();
                dist += 0.15;

                int n_dash = wm.self().playerType().cyclesToReachDistance(dist);
                dlog.addText(Logger::INTERCEPT,"tackle intercept c:%d, dt:%d,t:%d,d:%d",c,n_dash+n_turn,n_turn,n_dash);

                if(n_dash + n_turn <= c){
                    double eval = intercept_tackle_eval(wm,n_dash,n_turn,c,next_body,ballpos);
                    if( c == opp_min)
                        eval /= 2.0;
                    if(eval > Bhv_BasicTackle::calc_takle_prob(wm,ballpos) && eval > best_eval){
                        best_tackle = make_pair(c,AngleDeg(next_body));
                        best_eval = eval;
                        dlog.addText(Logger::INTERCEPT,"tackle intercept eval: %.1f is best",eval);
                    }else
                        dlog.addText(Logger::INTERCEPT,"tackle intercept eval: %.1f",eval);
                }
                {
                    double next_body;
                    int n_turn = predict_player_turn_cycle_direct_tackle(wm.self().playerTypePtr(),
                                                                         wm.self().body(),
                                                                         wm.self().vel().r(),
                                                                         wm.self().pos().dist(ballpos),
                                                                         (ballpos - wm.self().pos()).th(),
                                                                         ServerParam::i().tackleWidth(),
                                                                         wm.self().pos(),
                                                                         wm.self().vel(),
                                                                         ballpos,
                                                                         next_body,
                                                                         false);

                    double dist = wm.self().inertiaPoint(n_turn).dist(ballpos);
                    dist -= ServerParam::i().tackleDist();
                    dist += 0.15;

                    int n_dash = wm.self().playerType().cyclesToReachDistance(dist);
                    dlog.addText(Logger::INTERCEPT,"tackle intercept c:%d, dt:%d,t:%d,d:%d",c,n_dash+n_turn,n_turn,n_dash);

                    if(n_dash + n_turn <= c){
                        double eval = intercept_tackle_eval(wm,n_dash,n_turn,c,next_body,ballpos);
                        if( c == opp_min)
                            eval /= 2.0;
                        if(eval > Bhv_BasicTackle::calc_takle_prob(wm,ballpos) && eval > best_eval){
                            best_tackle = make_pair(c,AngleDeg(next_body));
                            best_eval = eval;
                            dlog.addText(Logger::INTERCEPT,"tackle intercept eval: %.1f is best",eval);
                        }else
                            dlog.addText(Logger::INTERCEPT,"tackle intercept eval: %.1f",eval);
                    }
                }
                dlog.addCircle(Logger::INTERCEPT,ballpos.x,ballpos.y,0.2,250,0,0);
                ballvel *= ServerParam::i().ballDecay();
            }
            return best_tackle;
        }

        static double intercept_tackle_eval(const WorldModel & wm, int dash_cycle, int turn_cycle, int ball_cycle, double body, Vector2D target){
            Vector2D self_pos = wm.self().inertiaPoint(std::max(turn_cycle, ball_cycle));
            double first_dist = self_pos.dist(target);
            double move_dist = 0;

            double speed = 0;
            double accel = ServerParam::i().maxDashPower() * wm.self().playerType().dashPowerRate() * wm.self().playerType().effortMax();
            for ( int counter = 1; counter <= dash_cycle; ++counter )
            {
                if ( speed + accel > wm.self().playerType().playerSpeedMax() )
                {
                    accel = wm.self().playerType().playerSpeedMax() - speed;
                }
                speed += accel;
                move_dist += speed;
                speed *= wm.self().playerType().playerDecay();
            }
            double last_dist = std::max(0.0, first_dist - move_dist);
            double width_dist = Line2D(self_pos,body).dist(target);
            dlog.addText(Logger::INTERCEPT,"dist %.2f, %.2f",last_dist,width_dist);
            double eval = 1.0 - ( std::pow( last_dist / ServerParam::i().tackleDist(),
                                                             ServerParam::i().tackleExponent() )
                                                   + std::pow( width_dist / ServerParam::i().tackleWidth(),
                                                               ServerParam::i().tackleExponent() ) );
            if(target.x > 35 && target.absY() < 10){
                AngleDeg to_up_tir = (Vector2D(52.0, -6) - target).th();
                AngleDeg to_down_tir = (Vector2D(52.0, +6) - target).th();
                if(AngleDeg(body).isWithin(to_up_tir,to_down_tir)){
                    eval *= 1.3;
                }
            }
            return eval;
        }

        static int predict_player_turn_cycle_direct_tackle(const rcsc::PlayerType * ptype,
                                                    const rcsc::AngleDeg & player_body, const double & player_speed,
                                                    const double & target_dist, const rcsc::AngleDeg & target_angle,
                                                    const double & dist_thr, const rcsc::Vector2D player_pos,
                                                    const rcsc::Vector2D player_vel, const rcsc::Vector2D target_pos,
                                                    double & next_body_angle, bool check_tackle_aria) {
            const ServerParam & SP = ServerParam::i();

            int n_turn = 0;

            double angle_diff = (target_angle - player_body).abs();

            bool turn_to_left = !player_body.isLeftOf(target_angle);
            double speed = player_speed;
            Vector2D next_pos = player_pos;
            Vector2D next_vel = player_vel;
            next_body_angle = player_body.degree();
            while (true){
                Vector2D self2ball = target_pos - next_pos;
                Vector2D self2ball_rotate = self2ball.rotate( -next_body_angle );
                if(self2ball_rotate.x > 0 && self2ball_rotate.absY() < dist_thr && check_tackle_aria)
                    break;
                n_turn++;
                angle_diff -= ptype->effectiveTurn(SP.maxMoment(), speed);
                if (angle_diff <= 0) {
                    next_body_angle = target_angle.degree();
                    break;
                } else {
                    if(turn_to_left)
                        next_body_angle -= ptype->effectiveTurn(SP.maxMoment(), speed);
                    else
                        next_body_angle += ptype->effectiveTurn(SP.maxMoment(), speed);
                }
                next_pos += next_vel;
                next_vel *= ptype->playerDecay();
                speed *= ptype->playerDecay();
            }

            return n_turn;

        }
};
#endif
