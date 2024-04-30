#ifndef INTENAION_MARK_H
#define INTENAION_MARK_H

#include <rcsc/game_time.h>
#include <rcsc/geom/vector_2d.h>

#include <rcsc/player/soccer_intention.h>
#include "bhv_mark_decision_greedy.h"
#include "bhv_mark_execute.h"
#include "mark_position_finder.h"
#include "strategy.h"
#include "bhv_basic_move.h"
#include <rcsc/player/intercept_table.h>
#include <vector>
#include "basic_actions/neck_turn_to_ball_or_scan.h"
#include "basic_actions/neck_turn_to_point.h"
#include "basic_actions/body_turn_to_point.h"
#include "basic_actions/body_turn_to_angle.h"
#include <rcsc/common/audio_memory.h>
#include <rcsc/player/world_model.h>
#include <rcsc/player/player_agent.h>
#include "basic_actions/body_go_to_point.h"
#include <rcsc/common/say_message_parser.h>
#include <rcsc/player/say_message_builder.h>
#include "basic_actions/neck_turn_to_ball_and_player.h"

#include <rcsc/common/logger.h>

class IntentionMark
        : public rcsc::SoccerIntention {
private:
    int opp_unum;
    MarkDec mark_dec;
    MarkType mark_type;
    Vector2D first_opp_pos;
    Vector2D first_target_pos;
    Vector2D first_ball_pos;
    int first_fastest_opp;
    rcsc::Vector2D M_target_point; // global coordinate
    double M_dash_power;
    double M_buffer;
    int max_step;

    rcsc::GameTime start_time;

public:
    IntentionMark(const MarkDec _mark_dec,
                  const MarkType _mark_type,
                  const int _opp_unum,
                  const Vector2D _first_opp_pos,
                  const Vector2D _first_target_pos,
                  const Vector2D _first_ball_pos,
                  const int _first_fastest_opp,
                  const int _max_step,
                  const rcsc::GameTime _start_time
    ) {
        mark_dec = _mark_dec;
        mark_type = _mark_type;
        opp_unum = _opp_unum;
        first_opp_pos = _first_opp_pos;
        first_target_pos = _first_target_pos;
        first_ball_pos = _first_ball_pos;
        first_fastest_opp = _first_fastest_opp;
        max_step = _max_step;
        start_time = _start_time;
    }

    bool finished(rcsc::PlayerAgent *agent) {
        const WorldModel &wm = agent->world();
        int self_unum = wm.self().unum();
        int opp_reach_cycle = wm.interceptTable().opponentStep();
        Vector2D ball_pos = wm.ball().inertiaPoint(opp_reach_cycle);
        int fastest_opp = wm.interceptTable().firstOpponent()->unum();

        if (wm.time().cycle() > start_time.cycle() + max_step)
            return true;
        if (!Strategy::i().isDefSit(wm, self_unum))
            return true;
        if (ball_pos.dist(first_ball_pos) > 20)
            return true;
        if (mark_dec != BhvMarkDecisionGreedy::markDecision(wm))
            return true;
        if (wm.theirPlayer(opp_unum) == NULL
            || wm.theirPlayer(opp_unum)->unum() < 1)
            return true;
        if (fastest_opp == opp_unum)
            return true;
        if(ball_pos.x > -35)
            return false;
        return false;
    }

    bool execute(rcsc::PlayerAgent *agent) {
        Target targ;
        const WorldModel &wm = agent->world();
        int opp_reach_cycle = wm.interceptTable().opponentStep();
        Vector2D ball_iner = wm.ball().inertiaPoint(opp_reach_cycle);
        int self_unum = wm.self().unum();
        double dist_thr = 1.0;
        string mark_type_str = "";
        const AbstractPlayerObject *opp = wm.theirPlayer(opp_unum);
        targ.pos = Strategy::i().getPosition(wm.self().unum());
        int blocker = bhv_block::who_is_blocker(wm);
        switch (mark_type) {
            case MarkType::LeadProjectionMark:
                targ = MarkPositionFinder::getLeadProjectionMarkTarget(self_unum, opp->unum(), wm);
                mark_type_str = "LeadProj";
                dist_thr = 1.5;
                break;
            case MarkType::LeadNearMark:
                targ = MarkPositionFinder::getLeadNearMarkTarget(self_unum,
                                                                       opp->unum(), wm);
                mark_type_str = "LeadNear";
                dist_thr = 0.5;
                break;
            case MarkType::ThMark:
                targ = MarkPositionFinder::getThMarkTarget(self_unum, opp->unum(), wm);
                dist_thr = 1.0;
                mark_type_str = "Th";
                break;
            case MarkType::DangerMark:
                targ = MarkPositionFinder::getDengerMarkTarget(self_unum, opp->unum(), wm);
                dist_thr = 0.2;
                mark_type_str = "Danger";
                break;
            case MarkType::Block:
                bhv_block().execute(agent);
                mark_type_str = "Block";
                agent->debugClient().addMessage("block in mark");
                return true;
            default:
                break;
        }
        dlog.addCircle(Logger::MARK, targ.pos, 1.0, 100, 0, 100);
        agent->doPointto(targ.pos.x, targ.pos.y);
        if (opp->posCount() > 3)
            if (Body_TurnToPoint(opp->pos()).execute(agent))
                Bhv_BasicMove::set_def_neck_with_ball(agent, targ.pos, wm.theirPlayer(opp_unum), blocker);


        agent->debugClient().addMessage("mark %d %s", opp_unum,
                                        mark_type_str.c_str());
        agent->debugClient().addLine(wm.self().pos(), targ.pos);
        if (do_move_mark(agent, targ, dist_thr)) {
            agent->debugClient().addMessage("bodygoto(%.1f,%.1f)", targ.pos.x, targ.pos.y);
        } else {
            Vector2D proj;
            switch (mark_type) {
                case MarkType::LeadProjectionMark:
                    proj = Line2D(opp->pos(), ball_iner).projection(targ.pos);
                    agent->debugClient().addMessage("bodyturnto(%.1f,%.1f)", proj.x, proj.y);
                    Body_TurnToPoint(proj).execute(agent);
                    break;
                case MarkType::LeadNearMark:
                    proj = Line2D(opp->pos(), ball_iner).projection(targ.pos);
                    agent->debugClient().addMessage("bodyturnto(%.1f,%.1f)", proj.x, proj.y);
                    Body_TurnToPoint(proj).execute(agent);
                    break;
                case MarkType::ThMark:
                    if (opp->pos().x < targ.pos.x) {
                        double t =
                                (((opp->pos() - targ.pos).th().degree()) + (ball_iner - targ.pos).th().degree()) / 2.0;
                        Body_TurnToAngle(t).execute(agent);
                        agent->debugClient().addMessage("bodyturnto(%.1f)", t);
                    } else {
                        if (ball_iner.y > targ.pos.y) {
                            Body_TurnToAngle(-90.0).execute(agent);
                            agent->debugClient().addMessage("bodyturnto(%.1f)", -90.0);
                        } else {
                            Body_TurnToAngle(+90.0).execute(agent);
                            agent->debugClient().addMessage("bodyturnto(%.1f)", 90.0);
                        }
                    }
                    break;
                case MarkType::DangerMark:
                    const AbstractPlayerObject *opp = wm.theirPlayer(opp_unum);
                    Vector2D targ = opp->pos() + opp->vel() - Vector2D(0.5, 0);
                    int opp_cycle = wm.interceptTable().opponentStep();
                    Vector2D ball_iner = wm.ball().inertiaPoint(opp_cycle);

                    if ((ball_iner - targ).th().abs() > 70) {
                        targ = opp->pos() + Vector2D::polar2vector(0.2, (ball_iner - opp->pos()).th());
                        targ += opp->vel();
                        double t = ((ball_iner - opp->pos()).th() + 90).degree();
                        agent->debugClient().addMessage("bodyturnto(%.1f)", 90.0);
                        Body_TurnToAngle(t).execute(agent);
                    } else {
                        Body_TurnToPoint(opp->pos()).execute(agent);
                        agent->debugClient().addMessage("bodyturnto(%.1f,%.1f)", opp->pos().x, opp->pos().y);
                    }
                    break;
                    //                default:
                    //                    break;
            }

        }

//        if(wm.theirPlayer(opp_unum)->posCount() > 1){
//            agent->setNeckAction(new Neck_TurnToPoint(wm.theirPlayer(opp_unum)->pos()));
//        }else{
//            agent->setNeckAction(
//                    new Neck_TurnToBallAndPlayer(
//                            wm.theirPlayer(opp_unum), 1));
//        }
        Bhv_BasicMove::set_def_neck_with_ball(agent, targ.pos, opp, blocker);


        return true;
    }

    bool do_move_mark(PlayerAgent *agent, Target targ, double dist_thr) {
        const WorldModel &wm = agent->world();
        Vector2D target_pos = targ.pos;
        Vector2D self_pos = wm.self().pos();
        int oppmin = wm.interceptTable().opponentStep();
        int distToOpp = 10;
        if (target_pos.x < -35 && target_pos.absY() < 10) {
            distToOpp = 20;
        }
        if (self_pos.dist(target_pos) > 1 ||
            wm.gameMode().type() != wm.gameMode().PlayOn
            || target_pos.dist(wm.interceptTable().firstOpponent()->pos()) > distToOpp) {
            double dash_power = Strategy::get_normal_dash_power(wm);
            if (oppmin < 5)
                dash_power = 100;
            if (target_pos.dist(Vector2D(-52, 0)) < 25)
                dash_power = 100;
            if (wm.interceptTable().firstOpponent()->pos().dist(target_pos) < 5)
                dash_power = 100;
            return Body_GoToPoint(target_pos, dist_thr, dash_power).execute(agent);
        } else if (self_pos.dist(Vector2D(-52.0, 0.0)) < 25.0) {
            agent->debugClient().addMessage("KOSSHER");
            agent->doDash(100, (target_pos - self_pos).th() - wm.self().body());
            return true;
        } else
            return false;
    }

};

#endif
