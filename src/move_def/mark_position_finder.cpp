//
// Created by nader on 2020-10-17.
//

#include "mark_position_finder.h"
#include "../chain_action/field_analyzer.h"
#include <rcsc/geom.h>
#include "../debugs.h"
#include "../setting.h"
using namespace rcsc;
Target
MarkPositionFinder::getLeadProjectionMarkTarget(int tmUnum, int oppUnum, const WorldModel &wm) {
    Target target;
    int opp_reach_cycle = wm.interceptTable().opponentStep();
    Vector2D ball_pos = wm.ball().inertiaPoint(opp_reach_cycle);
    if (wm.getDistTeammateNearestTo(ball_pos, 5) > 5) {
        ball_pos.x -= 4.0;
    }
    Vector2D tm_pos = wm.ourPlayer(tmUnum)->pos();
    Vector2D opp_pos = wm.theirPlayer(oppUnum)->pos();
    Vector2D best = Vector2D::polar2vector(1, (ball_pos - opp_pos).th()) + opp_pos;
    double min_dist = best.dist(tm_pos);

    for(double d = 2; d < opp_pos.dist(ball_pos); d += 2){
        Vector2D candid = Vector2D::polar2vector(d, (ball_pos - opp_pos).th()) + opp_pos;
        if (d > 12)
            break;
        if(candid.dist(tm_pos) < min_dist){
            min_dist = candid.dist(tm_pos);
            best = candid;
        }
    }
    target.pos = best;
    AngleDeg th = (ball_pos - opp_pos).th() + AngleDeg(90);
    AngleDeg goal = (Vector2D(-52,0), target.pos).th();

    if((th - goal).abs() < 90){
        target.th = th;
    }else{
        target.th = th + AngleDeg(180);
    }

    return target;
}

Target
MarkPositionFinder::getLeadNearMarkTarget(int tmUnum, int oppUnum, const WorldModel &wm) {
    Target target;
    const AbstractPlayerObject *opp = wm.theirPlayer(oppUnum);
    int opp_reach_cycle = wm.interceptTable().opponentStep();
    Vector2D ball_pos = wm.ball().inertiaPoint(opp_reach_cycle);
    double dist2opp = 0.2;
    Vector2D opp_vel = opp->vel() / 0.4 * 2.0 * opp->playerTypePtr()->playerSpeedMax();
    target.pos = opp->pos() + opp_vel;
    target.pos += Vector2D::polar2vector(dist2opp, (ball_pos - target.pos).th());
    return target;
}

Target
MarkPositionFinder::getThMarkTarget(size_t tmUnum, size_t oppUnum, const WorldModel &wm, bool debug) {
    Target target;
    const AbstractPlayerObject *opp = wm.theirPlayer(oppUnum);
    int opp_min = wm.interceptTable().opponentStep();


    Vector2D ball_inertia = wm.ball().inertiaPoint(opp_min);
    Vector2D opp_vel = opp->vel() / 0.4 * 2.0 * opp->playerTypePtr()->playerSpeedMax();
    if (opp_vel.x > -1)
        opp_vel.x = -1;
    target.pos = opp->pos() + opp_vel;
    Vector2D opp_pos = target.pos;

    bool isGoalieForward=Setting::i()->mStrategySetting->mIsGoalForward;
    double tm_def_hpos_x = isGoalieForward?Strategy::i().getPosition(3).x:Strategy::i().getPosition(2).x;
    if(ball_inertia.x > -15){
        tm_def_hpos_x -= Setting::i()->mDefenseMove->mMidTh_PosFinderHPosXNegativeTerm;
    }
    double tm_def_pos_x = wm.ourDefenseLineX();
    bool opp_can_pass_now = false;
    if ((opp_min <= 3 && ball_inertia.x < tm_def_pos_x + 30)
        || (opp_min <= 5 && ball_inertia.x < tm_def_pos_x + 20)){
        opp_can_pass_now = true;
    }

    double opp_near_offside_line_x = 1000;
    for(int o = 1; o <= 11; o++){
        const AbstractPlayerObject * opp = wm.theirPlayer(o);
        if(opp != nullptr && opp->unum() > 0){
            if(opp->pos().x < tm_def_pos_x + 3
               && opp->pos().x > tm_def_pos_x - 0.5){
//                if(opp->pos().x < ball_inertia.x - 30)
//                    continue;
//                Vector2D pass_tar = (opp->pos() - Vector2D(10,0));
//                AngleDeg pass_ang = (pass_tar - ball_inertia).th();
//                Triangle2D pas_sec(ball_inertia, opp->pos() + Vector2D(2,8),opp->pos() + Vector2D(2,-8));
//                if(opp->pos().x < ball_inertia.x - 15)
//                    if(wm.existTeammateIn(pas_sec, 5, false))
//                        continue;
//                if(debug)dlog.addText(Logger::MARK, "opp %d can receive th pass", o);
                if(opp->pos().x < opp_near_offside_line_x){
                    opp_near_offside_line_x = opp->pos().x;
                }
            }
        }
    }

    if(abs(tm_def_hpos_x - tm_def_pos_x) < 2){
        #ifdef DEBUG_POSITION_FINDER
        dlog.addText(Logger::MARK,"abs(tm_def_hpos_x - tm_def_pos_x) < 2");
        #endif
        tm_def_hpos_x = tm_def_pos_x;
        double dist_to_opp = 2;
        if(opp_min <= 2)
            dist_to_opp = 3;
        #ifdef DEBUG_POSITION_FINDER
        dlog.addText(Logger::MARK,"A");
        #endif
        double dist_backward = 5;
        if(ball_inertia.x > tm_def_pos_x + 15){
            if((ball_inertia - target.pos).th().abs() < 30){
                dist_backward = 2;
            }else{
                dist_backward = 3.5;
            }
        }else{
            if((ball_inertia - target.pos).th().abs() < 30){
                dist_backward = 3;
            }else{
                dist_backward = 5;
            }
        }
        if(ball_inertia.x > -25 ){
            dist_backward += Setting::i()->mDefenseMove->mMidTh_PosFinderBackDistXPlusTerm;
        }
        if(ball_inertia.x < -25 && opp_pos.x < -32.5)
            dist_backward = 1.0;

        target.pos.x = std::max(std::min(tm_def_hpos_x + 2, target.pos.x - dist_to_opp), tm_def_pos_x - dist_backward);
        double zy = 7.0;
        if(ball_inertia.x < -17 && ball_inertia.absY() < 25 && target.pos.absY() < 15 && abs(target.pos.y - ball_inertia.y) < 20)
            zy = 12.0;
        double dist_y = (ball_inertia.y - target.pos.y) / zy;
        if(ball_inertia.y > 20 && target.pos.absY() < 15)
            dist_y = -1;
        if(ball_inertia.y < 20 && target.pos.absY() < 15)
            dist_y = +1;
        target.pos.y =  dist_y + target.pos.y;
        if(target.pos.y < ball_inertia.y){
            target.pos.y = std::min(target.pos.y, ball_inertia.y);
        }else{
            target.pos.y = std::max(target.pos.y, ball_inertia.y);
        }
    }
    else if(tm_def_hpos_x > tm_def_pos_x){         //D  H   --->
        #ifdef DEBUG_POSITION_FINDER
        dlog.addText(Logger::MARK,"tm_def_pos_x < tm_def_hpos_x");
        dlog.addText(Logger::MARK,"can pass now:%d opp near offside:%.1f", opp_can_pass_now, opp_near_offside_line_x);
        #endif
        if(opp_can_pass_now && opp_near_offside_line_x < 1000){
            #ifdef DEBUG_POSITION_FINDER
            dlog.addText(Logger::MARK,"back of opp");
            #endif
            target.pos.x = opp_near_offside_line_x - 1;
        }else{
            #ifdef DEBUG_POSITION_FINDER
            dlog.addText(Logger::MARK,"go forward step step, tm tired:%d tm tired x:%.1f defpos x:%.1f", tm_is_tired, tm_tired_x, tm_def_pos_x);
            #endif
            target.pos.x = tm_def_pos_x;
            if (ball_inertia.x > tm_def_pos_x + 15){
                target.pos.x += 5.0;
            }
            else{
                target.pos.x += 1.5;
            }
        }
        if(target.pos.x < -36){
            #ifdef DEBUG_POSITION_FINDER
            dlog.addText(Logger::MARK,"go forward step step targx < -36, tm tired:%d tm tired x:%.1f defpos x:%.1f", tm_is_tired, tm_tired_x, tm_def_pos_x);
            #endif
            target.pos.x = tm_def_pos_x;
            if (ball_inertia.x > tm_def_pos_x + 15){
                target.pos.x += 5.0;
            }
            else{
                target.pos.x += 1.5;
            }
        }
        target.pos.x = std::min(target.pos.x, tm_def_hpos_x);
        if(target.pos.x < -40)
            target.pos.x = -40;

        double zy = 7.0;
        if(ball_inertia.x < -17 && ball_inertia.absY() < 25 && target.pos.absY() < 15 && abs(target.pos.y - ball_inertia.y) < 20)
            zy = 12.0;
        double dist_y = (ball_inertia.y - target.pos.y) / zy;
        if(ball_inertia.y > 20 && target.pos.absY() < 15)
            dist_y = -1;
        if(ball_inertia.y < 20 && target.pos.absY() < 15)
            dist_y = +1;
        target.pos.y =  dist_y + target.pos.y;
        if(target.pos.y < ball_inertia.y){
            target.pos.y = std::min(target.pos.y, ball_inertia.y);
        }else{
            target.pos.y = std::max(target.pos.y, ball_inertia.y);
        }
    }else{                                  //H  D   <---
        if(target.pos.x < tm_def_hpos_x){        //O  H  D
            target.pos.x = tm_def_hpos_x;
            target.pos.y += std::min((ball_inertia.y - target.pos.y) / 10.0, 3.0);
            #ifdef DEBUG_POSITION_FINDER
            dlog.addText(Logger::MARK,"F");
            #endif
        }else if(target.pos.x < tm_def_pos_x){   //H  O  D
            target.pos.x = std::max(tm_def_hpos_x, target.pos.x - 2);
            #ifdef DEBUG_POSITION_FINDER
            dlog.addText(Logger::MARK,"G");
            #endif
        }else{                              //H  D  O
            target.pos.x = std::min(std::max(tm_def_hpos_x, target.pos.x - 2),tm_def_hpos_x + 3);
            #ifdef DEBUG_POSITION_FINDER
            dlog.addText(Logger::MARK,"H");
            #endif
        }
        double zy = 7.0;
        if(ball_inertia.x < -17 && ball_inertia.absY() < 25 && target.pos.absY() < 15 && abs(target.pos.y - ball_inertia.y) < 20)
            zy = 12.0;
        double dist_y = (ball_inertia.y - target.pos.y) / zy;
        if(ball_inertia.y > 20 && target.pos.absY() < 15)
            dist_y = -1;
        if(ball_inertia.y < 20 && target.pos.absY() < 15)
            dist_y = +1;
        target.pos.y =  dist_y + target.pos.y;
        if(target.pos.y < ball_inertia.y){
            target.pos.y = std::min(target.pos.y, ball_inertia.y);
        }else{
            target.pos.y = std::max(target.pos.y, ball_inertia.y);
        }
    }
    //    if(abs(target.pos.x - (opp->pos() + opp_vel).x ) < 1){
    //        target.pos.y += std::min((ball_inertia.y - target.pos.y) / 10.0, 3.0);
    //    }
    if(opp->pos().x > target.pos.x - 3)
        target.th = (opp->pos() - target.pos).th();
    else{
        if(ball_inertia.y > target.pos.y){
            target.th = AngleDeg(90);
        }else{
            target.th = AngleDeg(-90);
        }
    }
//    if (ball_inertia.x > 25 && !tm_is_tired){
//        target.pos.x = tm_def_hpos_x;
//    }

    return target;
}

Target
MarkPositionFinder::getThMarkTarget2(size_t tmUnum, size_t oppUnum, const WorldModel &wm, bool debug) {
    Target target;
    const AbstractPlayerObject *opp = wm.theirPlayer(oppUnum);
    int opp_min = wm.interceptTable().opponentStep();
    Vector2D ball_inertia = wm.ball().inertiaPoint(opp_min);
    double offside_line_except_self = 0;
    for (int t = 2; t <= 11; t++) {
        if (t == tmUnum)
            continue;
        const AbstractPlayerObject *tm = wm.ourPlayer(t);
        if (tm == nullptr || tm->unum() != t)
            continue;
        if (tm->pos().x < offside_line_except_self)
            offside_line_except_self = tm->pos().x;
    }
    target.pos = Strategy::i().getPosition(tmUnum);
    if (target.pos.x > offside_line_except_self + 2)
        target.pos.x = offside_line_except_self + 2;

    if (Setting::i()->mDefenseMove->mUpdateThMarkTargetForSideDefenders)
        updateThMarkTargetForSideDefender(tmUnum, wm, target, debug);

    Vector2D opp_vel = Vector2D(0, 0);
    if (oppUnum != -1){
        opp->vel() / 0.4 * 2.0 * opp->playerTypePtr()->playerSpeedMax();
        if (opp_vel.x > -1)
            opp_vel.x = -1;

        Vector2D opp_pos = opp->pos() + opp_vel;

        if (opp_pos.x < wm.ourDefenseLineX()){
            if (opp_pos.y > target.pos.y)
                target.th = AngleDeg(90);
            else
                target.th = AngleDeg(-90);
        }
        else{
            target.th = (opp_pos - target.pos).th();
        }
    }
    else{
        if (ball_inertia.y > target.pos.y)
            target.th = AngleDeg(90);
        else
            target.th = AngleDeg(-90);
    }

    return target;
}

Vector2D getOffenderSideOpponentPos(const WorldModel & wm, int side){ // up:-1, down:1
    double ourDefenceLineX = wm.ourDefenseLineX();
    Vector2D side_opp_pos = Vector2D::INVALIDATED;
    double y = 0;
    for (auto opp_candid: wm.theirPlayers()){
        if (opp_candid->pos().x > ourDefenceLineX + 10)
            continue;
        if (side == -1){
            if (opp_candid->pos().y < y){
                y = opp_candid->pos().y;
                side_opp_pos = opp_candid->pos();
            }
        }
        else{
            if (opp_candid->pos().y > y){
                y = opp_candid->pos().y;
                side_opp_pos = opp_candid->pos();
            }
        }
    }
    return side_opp_pos;
}

Vector2D getOurDefenderSidePosExceptX(const WorldModel & wm, int side, int tmUnum){
    double ourDefenceLineX = wm.ourDefenseLineX();
    Vector2D side_tm_pos_except_self = Vector2D::INVALIDATED;
    double y = 0;
    for (int t = 2; t <= 11; t++) {
        const AbstractPlayerObject *tm = wm.ourPlayer(t);
        if (tm == nullptr || tm->unum() != t)
            continue;
        if (t == tmUnum)
            continue;
        if (tm->pos().x > ourDefenceLineX + 10)
            continue;
        if (side == -1){
            if (tm->pos().y < y){
                y = tm->pos().y;
                side_tm_pos_except_self = tm->pos();
            }
        }
        else{
            if (tm->pos().y > y){
                y = tm->pos().y;
                side_tm_pos_except_self = tm->pos();
            }
        }
    }
    return side_tm_pos_except_self;
}

void
MarkPositionFinder::updateThMarkTargetForSideDefender(size_t tmUnum, const WorldModel &wm, Target &target, bool debug){
    int opp_min = wm.interceptTable().opponentStep();
    Vector2D ball_inertia = wm.ball().inertiaPoint(opp_min);
    if (Strategy::i().tm_Post(tmUnum) != Strategy::pp_lb &&
        Strategy::i().tm_Post(tmUnum) != Strategy::pp_rb)
        return;
    int side = Strategy::i().tm_Post(tmUnum) == Strategy::pp_lb ? -1 : 1;
    Vector2D side_opp_pos = getOffenderSideOpponentPos(wm, side);
    if (!side_opp_pos.isValid())
        return;
    if (side_opp_pos.x < target.pos.x - 5)
        return;
    if (side_opp_pos.x > target.pos.x + 5)
        return;
    if (side_opp_pos.y * side < 0)
        return;
    Vector2D side_tm_pos_except_self = getOurDefenderSidePosExceptX(wm, side, tmUnum);
    if (!side_tm_pos_except_self.isValid())
        return;
    if (side_tm_pos_except_self.absY() > side_opp_pos.absY() - 3.0)
        return;
    if (side_opp_pos.absY() > target.pos.absY())
        return;
    target.pos.y = side_opp_pos.y;
}

Target
MarkPositionFinder::getDengerMarkTarget(int tmUnum, int oppUnum, const WorldModel &wm) {
    Target target;
    const AbstractPlayerObject *opp = wm.theirPlayer(oppUnum);
    Vector2D ball_inertia = wm.ball().inertiaPoint(wm.interceptTable().opponentStep());

    if (abs(ball_inertia.y - opp->pos().x) > 10
        || ball_inertia.x - 2 > wm.ourDefenseLineX()) {
        Vector2D opp_pos = opp->pos() +
                           Vector2D::polar2vector(opp->playerTypePtr()->playerSpeedMax() *
                                                  (opp->vel().r() / 0.4), opp->vel().th());

        AngleDeg crossAngle = (ball_inertia - opp_pos).th();
//        if (abs(opp_pos.x - ball_inertia.x) < 5){
//            target.pos = opp_pos
//                         + Vector2D::polar2vector(0.5, crossAngle)
//                         + Vector2D(-1.0, 0);
//        }
//        else{
            target.pos = opp_pos +
                         Vector2D::polar2vector(1, crossAngle);
//        }
        if (ball_inertia.x < target.pos.x) {
            target.pos += Vector2D(-1, 0);
        } else {
            target.pos += Vector2D(-0.2, 0);
        }
        target.th = (ball_inertia - target.pos).th();
    } else {
        Vector2D opp_pos = opp->pos();

        // calculate danger goal area;
        auto goalie = wm.getOurGoalie();
        if (goalie) {
            Vector2D goal_post_left(-ServerParam::i().pitchHalfLength(), -ServerParam::i().goalHalfWidth() + 1);
            Vector2D goal_post_right(-ServerParam::i().pitchHalfLength(), ServerParam::i().goalHalfWidth() - 1);
            Vector2D &danger_goal_post = goal_post_left.dist(goalie->pos()) > goal_post_right.dist(goalie->pos())
                                         ? goal_post_left : goal_post_right;

            //    int ball_time_to_reach_goal = danger_goal_post.dist(opp_pos) / ServerParam::i().ballSpeedMax();


            target.pos = opp_pos + Vector2D::polar2vector(1, (danger_goal_post - opp_pos).th());
        } else {
            Vector2D opp_vel = opp->vel() / 0.4 * 2.0 * opp->playerTypePtr()->playerSpeedMax();

            target.pos = opp_pos + opp_vel - Vector2D(0.5, 0);
        }
        target.th = (target.pos - Vector2D(-52, target.pos.y)).th();
    }

    return target;
}
