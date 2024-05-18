//
// Created by ashkan on 3/30/18.
//

#include "bhv_offensive_move.h"
#include "../strategy.h"
#include "bhv_unmark.h"
#include "bhv_unmark2019.h"
#include "bhv_scape.h"
#include "../bhv_basic_move.h"
#include "../sample_communication.h"
#include "bhv_unmark_voronoi.h"
#include "bhv_scape_voronoi.h"
#include "../setting.h"
#include "../neck/neck_decision.h"
#include <rcsc/geom/voronoi_diagram.h>
#include "basic_actions/neck_turn_to_ball_or_scan.h"
#include "basic_actions/neck_turn_to_low_conf_teammate.h"

#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/player/intercept_table.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/common/audio_memory.h>
#include "basic_actions/body_go_to_point.h"
#include "basic_actions/basic_actions.h"


static bool low_stamina = false;

bhv_unmark bhv_unmarkes::best_last_unmark = bhv_unmark();
bhv_unmark_2019 bhv_unmarkes_2019::best_last_unmark = bhv_unmark_2019();
scape bhv_scape::last_new_scape = scape();

bool is_anti_offense(const WorldModel &wm) {
    std::vector<double> oppx;
    for (int i = 1; i <= 11; i++) {
        const AbstractPlayerObject *opp = wm.theirPlayer(i);
        if (opp == NULL || opp->unum() < 1)
            continue;
        oppx.push_back(opp->pos().x);
    }
    int size = oppx.size();
    for (int i = 0; i < size; i++) {
        for (int j = i; j < size; j++) {
            if (oppx[i] > oppx[j]) {
                double tmp = oppx[i];
                oppx[i] = oppx[j];
                oppx[j] = tmp;
            }
        }
    }
    int number_in_off = std::min(6, size);
    double sum_x = 0;
    for (int i = 0; i < number_in_off; i++) {
        sum_x += oppx[i];
    }
    double avg_x = sum_x / (double) number_in_off;
    Vector2D ball_pos = wm.ball().pos();
    if (avg_x < ball_pos.x - 25)
        return true;
    return false;
}


bool cyrus_offensive_move::execute(rcsc::PlayerAgent *agent, Bhv_BasicMove *bhv_basicMove) {

    const WorldModel &wm = agent->world();
    basicMove = bhv_basicMove;
#ifdef debug_unmark
    dlog.addText(Logger::POSITIONING,"start off>>%d",wm.time().cycle());

    dlog.addText(Logger::POSITIONING,"last static:farar:%d,lastscape:%d,havehard:%s,unmarking:%s,time:%d,static tar:%.2f,%.2f",
                 farar,last_target_set_scape,(have_hardunmark?"true":"false"),(unmarking?"true":"false"),startTime,staticTarget.x,staticTarget.y);
#endif

    Vector2D target_point = Strategy::i().getPosition(wm.self().unum());

    Vector2D centerGoalPos(52, 0);
    const int self_min = wm.interceptTable().selfStep();
    const int mate_min = wm.interceptTable().teammateStep();
    const int opp_min = wm.interceptTable().opponentStep();
    const Vector2D ballPos = wm.ball().inertiaPoint(mate_min);

    //Broker Offside
    if ( Setting::i()->mOffensiveMove->mIs9BrokeOffside
         && Strategy::i().self_Post() == Strategy::player_post::pp_lf
         && Strategy::i().get_formation_type() == Strategy::FormationType::F433)
    {
        if ( wm.ball().inertiaPoint(mate_min).x > -25
             && target_point.x < 40 ){
            target_point.x = wm.offsideLineX() + 10;
            Body_GoToPoint(target_point, 3, 100).execute(agent);
            NeckDecisionWithBall().setNeck(agent, NeckDecisionType::offensive_move);
            dlog.addText(Logger::ACTION, "Go to offside to Broke it");
            agent->debugClient().addMessage("BrokeOffside");
            return true;
        }
    }

    if (Strategy::i().self_Line() == Strategy::PostLine::back) {
        if (wm.self().stamina() < 4000 && !low_stamina) {
            low_stamina = true;
        } else if (low_stamina && wm.self().stamina() > 6000) {
            low_stamina = false;
        }
    }
    if (Strategy::i().self_Line() == Strategy::PostLine::half) {
        if (wm.self().stamina() < 3000 && !low_stamina) {
            low_stamina = true;
        } else if (low_stamina && wm.self().stamina() > 5000) {
            low_stamina = false;
        }
    }
    if (Strategy::i().self_Line() == Strategy::PostLine::forward) {
        if (wm.self().stamina() < 3000 && !low_stamina) {
            low_stamina = true;
        } else if (low_stamina && wm.self().stamina() > 5000) {
            low_stamina = false;
        }
    }
    if (!FieldAnalyzer::isFRA(wm) && BackFromOffside(agent)) {
        SampleCommunication().sayUnmark(agent);
        agent->debugClient().addMessage(" back offsie");
        return true;
    }

    bool unmark_or_scape = true;
    if(ballPos.x > target_point.x + 20 && ballPos.x > 10 && Strategy::i().self_Line() == Strategy::PostLine::back)
        unmark_or_scape = false;

    if(unmark_or_scape)
        if (!low_stamina || (ballPos.x > 35 && wm.self().stamina() > 3000)) {
            if(bhv_scape::can_scape(wm)){
                if(bhv_scape_voronoi().execute(agent)){
                    dlog.addText(Logger::POSITIONING, "voro scape is running");
                    return true;
                }
                if (pers_scap(agent)) {
//                    SampleCommunication().sayUnmark(agent);
                    dlog.addText(Logger::POSITIONING, "pers scape is running");
                    return true;
                }

//                if (bhv_scape().execute(agent)) {
//                    dlog.addText(Logger::POSITIONING, "scape is running");
//                    SampleCommunication().sayUnmark(agent);
//                    return true;
//                }
            }
            bool amIneartm = true;
            for (int t = 1; t <= 11; t++) {
                if (t == wm.interceptTable().firstTeammate()->unum())
                    continue;
                if (t == wm.self().unum())
                    continue;
                const AbstractPlayerObject *tm = wm.ourPlayer(t);
                if (tm == NULL || tm->unum() <= 0)
                    continue;
                if (tm->pos().dist(wm.ball().inertiaPoint(mate_min)) <
                        wm.self().pos().dist(wm.ball().inertiaPoint(mate_min))) {
                    amIneartm = false;
                }
            }
            for (auto & u: Setting::i()->mOffensiveMove->mUnmarkingAlgorithms){
                if (u == "main"){
                    if (bhv_unmarkes().execute(agent)) {
                        return true;
                    }
                }else if (u == "2019"){
                    if (bhv_unmarkes_2019().execute(agent)) {
                        return true;
                    }
                }else if (u == "voronoi"){
                    if (bhv_unmark_voronoi().execute(agent)) {
                        return true;
                    }
                }
            }
//            if (FieldAnalyzer::isYushan(wm)
//                || FieldAnalyzer::isMT(wm)
//                || FieldAnalyzer::isHelius(wm)
//                || FieldAnalyzer::isOxsy(wm))
//            {
//                if (bhv_unmarkes().execute(agent)) {
//                    return true;
//                }
//            }
//            else
//            {
//                if (bhv_unmarkes_2019().execute(agent)) {
//                    return true;
//                }
//            }
//            if (bhv_unmark_voronoi().execute(agent)) {
//                return true;
//            }
        }

    /*--------------------------------------------------------*/
    // chase ball

    if(wm.ball().pos().isValid())
        if (Vector2D::polar2vector(0.7, (target_point - wm.self().pos()).th()).dist(
                    wm.ball().inertiaPoint(1)) < 1) {
            target_point = Vector2D::polar2vector(2.0,
                                                  (target_point - wm.self().pos()).th().degree() + 90);
        }
    double dash_power = Strategy::get_normal_dash_power(wm);
    //	if(low_stamina)
    //		dash_power *=(2.0 / 3.0);


    double stamina = wm.self().stamina();
    Vector2D ball = wm.ball().inertiaPoint(mate_min);
    Vector2D self_pos = wm.self().pos();

    if (Strategy::i().self_Line() == Strategy::PostLine::forward) {
        if (self_pos.dist(target_point) > 2) {
            if (stamina > 4000 && ball.x > 20) {
                dash_power = 100;
            } else if (stamina > 3000 && ball.x > 30) {
                dash_power = 100;
            }
        }
    } else if (Strategy::i().self_Line() == Strategy::PostLine::half) {
        if (self_pos.dist(target_point) > 2) {
            if (stamina > 4500 && ball.x > 20) {
                dash_power = 100;
            } else if (stamina > 3500 && ball.x > 30) {
                dash_power = 100;
            } else if (stamina > 4500 && ball.x > -25) {
                if (is_anti_offense(wm))
                    dash_power = 100;
            }
        }
    }

    double dist_thr = wm.ball().distFromSelf() * 0.1;
    if(Strategy::i().self_Line() == Strategy::PostLine::back)
        dist_thr = wm.ball().distFromSelf() * 0.3;
    if (dist_thr < 1.0)
        dist_thr = 1.0;

    if(ballPos.x > target_point.x + 20 && ballPos.x > 20 && Strategy::i().self_Line() == Strategy::PostLine::back)
        dash_power = dash_power / 3.0 * 2.0;

    dlog.addText(Logger::TEAM,
                 __FILE__": Bhv_BasicMove target=(%.1f %.1f) dist_thr=%.2f", target_point.x,
                 target_point.y, dist_thr);

    agent->debugClient().addMessage("BasicMoveoff%.0f", dash_power);
    agent->debugClient().setTarget(target_point);
    agent->debugClient().addCircle(target_point, dist_thr);

    if (off_gotopoint(agent, target_point, dist_thr, dash_power)) {

    } else if (!Body_GoToPoint(target_point, dist_thr, dash_power).execute(agent)) {
        agent->doTurn(0.0);
        //		Body_TurnToBall().execute( agent );
    }
    NeckDecisionWithBall().setNeck(agent, NeckDecisionType::offensive_move);
    SampleCommunication().sayUnmark(agent);
    return true;
}
//scape

bool cyrus_offensive_move::pers_scap(PlayerAgent *agent) {

    const WorldModel &wm = agent->world();
    const int self_min = wm.interceptTable().selfStep();
    const int mate_min = wm.interceptTable().teammateStep();
    const int opp_min = wm.interceptTable().opponentStep();
    Vector2D ball = wm.ball().inertiaPoint(wm.interceptTable().teammateStep());
    Vector2D me = wm.self().pos();
    int num = wm.self().unum();
    double max_x = std::max(wm.offsideLineX(), wm.ball().inertiaPoint(mate_min).x);

    if (num == 9
            || (wm.interceptTable().firstTeammate() != NULL
                && (wm.interceptTable().firstTeammate()->unum() == 9 || wm.interceptTable().firstTeammate()->unum() == 7)
                && wm.self().unum() == 11))
    {
    }else
        return false;
//    return false;
    Vector2D homePos = Strategy::i().getPosition(num);
    if (wm.self().unum() > 8 && ball.x < 30 && ball.x > -10 && me.dist(homePos) < 8 &&
            wm.self().pos().x < max_x - 0.5 &&
            homePos.x > max_x - 8.0 &&
            wm.self().body().abs() < 20.0 &&
            mate_min < opp_min && self_min > mate_min && wm.self().stamina() > 5000 && me.absY() < 33) {
        agent->doDash(100, 0.0);
        agent->setNeckAction(new Neck_TurnToBallOrScan(0));
        return true;
    }

    double minStamina = (num >= 9) ? 4000 : 5000;
    double minYdiff = 5;

    if (max_x > 30) minYdiff = 4;
    else if (max_x > 25) minYdiff = 5;
    else if (max_x > 15) minYdiff = 6;
    else if (max_x > 10) minYdiff = 8;
    else minYdiff = 10;


    if (num > 8 && mate_min < opp_min && ball.x > -30.0 && ball.x + 33 > max_x &&
            homePos.x < 36.0 && wm.self().stamina() > minStamina && homePos.x > -10 &&
            fabs(me.y - homePos.y) < minYdiff &&
            me.x < max_x - 1.3 && me.absY() < 33) {

        if (!Body_GoToPoint(rcsc::Vector2D(me.x + 10, me.y),
                            0.5, ServerParam::i().maxDashPower()).execute(agent))
            Body_TurnToPoint(Vector2D(me.x + 10.0, me.y)).execute(agent);

        if (wm.kickableOpponent()
                && wm.ball().distFromSelf() < 12.0)
            agent->setNeckAction(new Neck_TurnToBall());
        else
            agent->setNeckAction(new Neck_TurnToBallOrScan(0));
        return true;

    }
    return false;

}

//back from offside
bool cyrus_offensive_move::BackFromOffside(PlayerAgent *agent) {
    const WorldModel &wm = agent->world();
    const Vector2D homePos = Strategy::i().getPosition(wm.self().unum());

    Vector2D self_pos = wm.self().pos();
    const int self_min = wm.interceptTable().selfStep();
    const int mate_min = wm.interceptTable().teammateStep();
    const int opp_min = wm.interceptTable().opponentStep();

    double max_x = std::max(wm.offsideLineX(), wm.ball().inertiaPoint(mate_min).x);

    Vector2D ball = wm.ball().inertiaPoint(
                std::min(self_min, std::min(mate_min, opp_min)));
    if ((Strategy::i().self_Line() == Strategy::PostLine::forward)
            && wm.self().pos().x > max_x - 0.6
            && wm.self().pos().x < max_x + 3.0
            && homePos.x > max_x - 4
            && wm.self().body().abs() < 20.0 && mate_min < opp_min
            && self_min > mate_min) {
        agent->doDash(100, 180.0);
        if (wm.kickableOpponent() && wm.ball().distFromSelf() < 18.0)
            agent->setNeckAction(new Neck_TurnToBall());
        else
            agent->setNeckAction(new Neck_TurnToBallOrScan(0));
        agent->debugClient().addMessage("back dash");
        return true;
    }

    if (self_pos.x > max_x - 1.0
            && (wm.self().stamina() > 4000 || (mate_min < opp_min))) {
        if (off_gotopoint(agent, Vector2D(self_pos.x - 10.0, self_pos.y), 0.5, ServerParam::i().maxDashPower())) {

        } else {
            Body_GoToPoint(Vector2D(self_pos.x - 10.0, self_pos.y), 0.5,
                           ServerParam::i().maxDashPower()).execute(agent);
        }
        NeckDecisionWithBall().setNeck(agent, NeckDecisionType::offensive_move);
        agent->debugClient().addMessage("back goto");
        return true;
    }
    return false;
}


bool cyrus_offensive_move::off_gotopoint(PlayerAgent *agent, Vector2D target, double distthr, double power) {
    const WorldModel &wm = agent->world();
    Vector2D self_pos = wm.self().pos();
    Vector2D tmp = target;
    double mindist = 100;
    int min_unum = 0;
    for (int i = 1; i <= 11; i++) {
        if (i == wm.self().unum())
            continue;
        const AbstractPlayerObject *tm = wm.ourPlayer(i);
        if (tm != NULL && tm->unum() > 0) {
            double dist = tm->pos().dist(self_pos);
            if (dist < mindist) {
                mindist = dist;
                min_unum = i;
            }
        }
    }
    if (mindist < 5 && wm.interceptTable().teammateStep() < 4) {
        Line2D move_line = Line2D(self_pos, target);
        const AbstractPlayerObject *near_tm = wm.ourPlayer(min_unum);
        Vector2D new_target;
        if (move_line.dist(near_tm->pos()) < 4) {
            AngleDeg move_angle = (target - self_pos).th();
            AngleDeg tm_angle = (near_tm->pos() - self_pos).th();
            double dif_angle = (move_angle - tm_angle).abs();
            if (dif_angle > 180)
                dif_angle = 360 - dif_angle;
            if (dif_angle < 80.0) {
                if (move_line.dist(near_tm->pos()) > 2) {
                    Vector2D proj = move_line.projection(near_tm->pos());
                    Vector2D bordar = proj - near_tm->pos();
                    new_target = near_tm->pos() + bordar + bordar;
                } else {
                    Vector2D proj = move_line.projection(near_tm->pos());
                    Vector2D bordar = proj - near_tm->pos();
                    new_target = proj + Vector2D::polar2vector(3.0, bordar.th()) + proj;
                }
                if (new_target.absX() < 52.5 && new_target.absY() < 34) {
                    target = new_target;
                    agent->debugClient().addMessage("change tar from(%.1f,%.1f) to (%.1f,%.1f)", tmp.x, tmp.y, target.x,
                                                    target.y);
                    agent->debugClient().setTarget(target);
                }
            }
        }

    }
    return Body_GoToPoint(target, distthr, power).execute(agent);
}

