/*
 * bhvmarkdecitiongreedy.cpp
 *
 *  Created on: Apr 12, 2017
 *      Author: nader
 */
#include "../strategy.h"
#include "bhv_block.h"
#include "bhv_mark_decision_greedy.h"
#include "bhv_basic_move.h"
#include <algorithm>
#include <rcsc/geom.h>
#include <vector>
#include <sstream>
#include "../setting.h"

using namespace std;
using namespace rcsc;


#define DEBUG_PRINT

std::string markTypeString(MarkType mark_type) {
    switch (mark_type) {
        case MarkType::NoType:
            return std::string("NoType");
        case MarkType::LeadProjectionMark:
            return std::string("LeadProjectionMark");
        case MarkType::LeadNearMark:
            return std::string("LeadNearMark");
        case MarkType::ThMark:
            return std::string("ThMark");
        case MarkType::DangerMark:
            return std::string("DangerMark");
        case MarkType::Block:
            return std::string("Block");
        case MarkType::Goal_keep:
            return std::string("Goal_keep");
    }
    return string("non valid");
}


bool BhvMarkDecisionGreedy::use_home_pos = false;
pair<long, int> BhvMarkDecisionGreedy::last_mark = make_pair(10000, 0);


void
BhvMarkDecisionGreedy::getMarkTargets(PlayerAgent *agent, MarkType &mark_type, int &mark_unum, bool &blocked) {
    const WorldModel &wm = agent->world();
    use_home_pos = false;

    switch (markDecision(wm)) {
        case MarkDec::AntiDef:
            #ifdef DEBUG_PRINT
            dlog.addText(Logger::MARK, "**MarkDec select MarkDec::AntiDef");
            #endif
            antiDefMarkDecision(wm, mark_type, mark_unum, blocked);
            break;
        case MarkDec::MidMark:
            #ifdef DEBUG_PRINT
            dlog.addText(Logger::MARK, "**MarkDec select MarkDec::MidMark");
            #endif
            midMarkDecision(agent, mark_type, mark_unum, blocked);
            break;
        case MarkDec::GoalMark:
            #ifdef DEBUG_PRINT
            dlog.addText(Logger::MARK, "**MarkDec select MarkDec::GoalMark");
            #endif
            goalMarkDecision(agent, mark_type, mark_unum, blocked);
            break;
        case MarkDec::JustBlock:
            if (bhv_block::who_is_blocker(wm) == wm.self().unum()) {
                #ifdef DEBUG_PRINT
                dlog.addText(Logger::MARK, "**MarkDec select MarkDec::JustBlock");
                #endif
                mark_type = MarkType::Block;
                mark_unum = wm.interceptTable()->fastestOpponent()->unum();
            }
            else {
                #ifdef DEBUG_PRINT
                dlog.addText(Logger::MARK, "**MarkDec select MarkDec::JustBlock But Self is not Blocker");
                #endif
                mark_type = MarkType(0);
                mark_unum = 0;
            }
            break;
        default:
            #ifdef DEBUG_PRINT
            dlog.addText(Logger::MARK, "**MarkDec select None");
            #endif
            mark_type = MarkType(0);
            mark_unum = 0;
    }
    return;
}

double def_line_x = 0;
double min_our_def_pos_x = 100;

MarkDec BhvMarkDecisionGreedy::markDecision(const WorldModel &wm) {
    int opp_reach_cycle = wm.interceptTable()->opponentReachCycle();
    int tm_reach_cycle = wm.interceptTable()->teammateReachCycle();
    Vector2D ball_inertia = wm.ball().inertiaPoint(std::min(opp_reach_cycle, tm_reach_cycle));
    double tm_offense_pos_x_avg = 0;
    double tm_offense_hpos_x_avg = 0;
    size_t tm_offense_count = 0;
    double tm_defense_pos_x_avg = 0;
    double tm_defense_hpos_x_avg = 0;
    size_t tm_defense_count = 0;
    double tm_pos_x_avg = 0;
    double tm_hpos_x_avg = 0;
    size_t tm_count = 0;
    min_our_def_pos_x = 1000;
    vector<const AbstractPlayerObject *> my_line = Strategy::i().myLineTmms(wm, Strategy::PostLine::back);
    for (size_t i = 0; i < my_line.size(); i++) {
        if (my_line.at(i)->pos().x < min_our_def_pos_x)
            min_our_def_pos_x = my_line.at(i)->pos().x;
    }

    for (size_t i = 2; i < 12; i++) {
        if (wm.ourPlayer(i) != NULL && wm.ourPlayer(i)->unum() > 1) {
            if (Strategy::i().tm_Line(i) != Strategy::PostLine::back) {
                tm_offense_pos_x_avg += wm.ourPlayer(i)->pos().x;
                tm_offense_hpos_x_avg += Strategy::i().getPosition(i).x;
                tm_offense_count += 1;
            }
            if (Strategy::i().tm_Line(i) != Strategy::PostLine::forward) {
                tm_defense_pos_x_avg += wm.ourPlayer(i)->pos().x;
                tm_defense_hpos_x_avg += Strategy::i().getPosition(i).x;
                tm_defense_count += 1;
            }
            tm_pos_x_avg += wm.ourPlayer(i)->pos().x;
            tm_hpos_x_avg += Strategy::i().getPosition(i).x;
            tm_count += 1;
        }
    }

    if (tm_offense_count != 0) {
        tm_offense_pos_x_avg /= static_cast<double>(tm_offense_count);
        tm_offense_hpos_x_avg /= static_cast<double>(tm_offense_count);
    }
    if (tm_defense_count != 0) {
        tm_defense_pos_x_avg /= static_cast<double>(tm_defense_count);
        tm_defense_hpos_x_avg /= static_cast<double>(tm_defense_count);
    }
    if (tm_count != 0) {
        tm_pos_x_avg /= static_cast<double>(tm_count);
        tm_hpos_x_avg /= static_cast<double>(tm_count);
    }
    def_line_x = wm.ourDefenseLineX();

    #ifdef DEBUG_PRINT
    dlog.addText(Logger::MARK, "**WM.DefLine:%.1f, DefX:%0.1, HDefX:%0.1, OffX:%.1f, HOffX:%.1f, TmX:%.1f, HTmX:%.1f",
                 def_line_x, tm_pos_x_avg, tm_hpos_x_avg, tm_defense_pos_x_avg, tm_defense_hpos_x_avg,
                 tm_offense_pos_x_avg, tm_offense_hpos_x_avg);
    #endif

    if (def_line_x < 0 && def_line_x < min_our_def_pos_x) {
        def_line_x = min_our_def_pos_x;
        #ifdef DEBUG_PRINT
        dlog.addText(Logger::MARK, "DefLine Changed to %.1f", def_line_x);
        #endif
    }

    def_line_x = std::min(def_line_x, wm.ball().inertiaPoint(2).x);
    #ifdef DEBUG_PRINT
    dlog.addText(Logger::MARK, "DefLine Changed to %.1f", def_line_x);
    #endif
    if (wm.interceptTable()->opponentReachCycle() < 10) {
        #ifdef DEBUG_PRINT
        dlog.addText(Logger::MARK, "TmDefH changed to deflinx to %.1f", def_line_x);
        #endif
        tm_defense_hpos_x_avg = def_line_x;
    }
    if (ball_inertia.x > 30 && !Strategy::i().isDefSit(wm, wm.self().unum())) {
        #ifdef DEBUG_PRINT
        dlog.addText(Logger::MARK, "MarkDec::AntiDef");
        #endif
        return MarkDec::NoDec;
    }

    if (ball_inertia.x > Setting::i()->mDefenseMove->mStartMidMark) {
        #ifdef DEBUG_PRINT
        dlog.addText(Logger::MARK, "MarkDec::MidMark_A");
        #endif
        return MarkDec::MidMark;
    }
    else {
        #ifdef DEBUG_PRINT
        dlog.addText(Logger::MARK, "MarkDec::GoalMark");
        #endif
        return MarkDec::GoalMark;
    }
}


#include "../chain_action/field_analyzer.h"

vector<int> BhvMarkDecisionGreedy::getOppOffensiveStatic(const WorldModel &wm) {
    vector<int> res = Setting::i()->mDefenseMove->mStaticOffensiveOpp;
    return res;
}


vector<size_t> BhvMarkDecisionGreedy::getOppOffensive(const WorldModel &wm, bool &fastest_opp_marked) {
    vector<size_t> offensive_opps;
    int opp_reach_cycle = wm.interceptTable()->opponentReachCycle();
    Vector2D ball_inertia = wm.ball().inertiaPoint(opp_reach_cycle);
    size_t fastest_opp = (wm.interceptTable()->fastestOpponent() == NULL ? 0
                                                                         : wm.interceptTable()->fastestOpponent()->unum());
    size_t opp_offense_number = 0;
    auto static_offensive_opps = getOppOffensiveStatic(wm);

    for (int o = 1; o <= 11; o++) {
        const AbstractPlayerObject *opp = wm.theirPlayer(o);
        bool opp_in_static_offense = false;
        if (find(static_offensive_opps.begin(), static_offensive_opps.end(), o) != static_offensive_opps.end())
            opp_in_static_offense = true;
        if (opp == NULL
            || opp->unum() < 1
            || opp->goalie())
            continue;
        Vector2D OppPos = opp->pos();
        double OppOff2OffsideLine = 10;
        bool fastest_blocked = true;
        if (o == fastest_opp) {
            if (wm.ourPlayer(5) != nullptr && wm.ourPlayer(5)->unum() == 5
                && wm.ourPlayer(5)->pos().x < wm.ourDefenseLineX() + 10) {
                OppPos = wm.ball().inertiaPoint(opp_reach_cycle);
                OppOff2OffsideLine = 15;
            }
            else {
                if (ball_inertia.x > 25)
                    OppOff2OffsideLine = 10;
                else
                    OppOff2OffsideLine = 10;
            }
            if (OppPos.x > wm.ourDefenseLineX() + OppOff2OffsideLine) {
                dlog.addText(Logger::MARK, "%d is not Offense Opp x=%.1f, dl=%.1f, m=%.1f", o, OppPos.x,
                             wm.ourDefenseLineX(), OppOff2OffsideLine);
                fastest_blocked = false;
            }
            if (OppPos.x > ball_inertia.x + 8 && OppPos.x > 15) {
                dlog.addText(Logger::MARK, "%d is not Offense Opp x=%.1f, bx=%.1f, m=%.1f", o, OppPos.x,
                             ball_inertia.x);
                fastest_blocked = false;
            }
            if (fastest_blocked) {
                offensive_opps.push_back(o);
                dlog.addText(Logger::MARK, "%d is Offense Opp Fastest", o);
                continue;
            }
        }
        if (o != fastest_opp || !fastest_blocked) {
            if (wm.ourPlayer(5) != nullptr && wm.ourPlayer(5)->unum() == 5
                && wm.ourPlayer(5)->pos().x < wm.ourDefenseLineX() + 10) {
                if (ball_inertia.x > 25)
                    OppOff2OffsideLine = 25;
                else if (ball_inertia.x > 0)
                    OppOff2OffsideLine = 20;
                else
                    OppOff2OffsideLine = 15;
            }
            else {
                if (ball_inertia.x > 25)
                    OppOff2OffsideLine = 25;
                else if (ball_inertia.x > 0)
                    OppOff2OffsideLine = 20;
                else
                    OppOff2OffsideLine = 15;
            }
            if (!opp_in_static_offense)
                OppOff2OffsideLine /= 3.0;

            if (OppPos.x > wm.ourDefenseLineX() + OppOff2OffsideLine) {
                dlog.addText(Logger::MARK, "%d is not Offense Opp x=%.1f, dl=%.1f, m=%.1f", o, OppPos.x,
                             wm.ourDefenseLineX(), OppOff2OffsideLine);
                continue;
            }
            if (OppPos.x > ball_inertia.x + 8 && OppPos.x > 15) {
                dlog.addText(Logger::MARK, "%d is not Offense Opp x=%.1f, bx=%.1f, m=%.1f", o, OppPos.x,
                             ball_inertia.x);
                continue;
            }
            if (OppPos.x > ball_inertia.x + 10) {
                dlog.addText(Logger::MARK, "%d is not Offense Opp x=%.1f, bx=%.1f, m=%.1f B", o, OppPos.x,
                             ball_inertia.x);
                continue;
            }
            if (!fastest_blocked) {
                fastest_opp_marked = true;
            }
        }


        offensive_opps.push_back(o);
        dlog.addText(Logger::MARK, "%d is Offense Opp", o);
        opp_offense_number++;
    }
    return offensive_opps;
}


vector<int> BhvMarkDecisionGreedy::whoCanPassToOpp(const WorldModel &wm, int o) {
    const AbstractPlayerObject *opp = wm.theirPlayer(o);
    Vector2D opp_pos = opp->pos();
    vector<int> opps;
    for (int i = 1; i <= 11; i++) {
        const AbstractPlayerObject *passer = wm.theirPlayer(i);
        if (passer == NULL || passer->unum() < 1)
            continue;
        Vector2D passer_pos = passer->pos();
        bool can_tm_cut = false;
        for (int t = 2; t <= 11; t++) {
            const AbstractPlayerObject *tm = wm.ourPlayer(t);
            if (tm == NULL || tm->unum() < 1)
                continue;
            Vector2D tm_pos = tm->pos();
            if (passer_pos.dist(tm_pos) > passer_pos.dist(opp_pos)) {
            }
            else {
                double a1 = (opp_pos - passer_pos).th().degree() - 15;
                double a2 = (opp_pos - passer_pos).th().degree() + 15;
                double target_angle = (tm_pos - passer_pos).th().degree();
                if (abs(a1 - a2) > 330) {
                    if (a1 < 0)
                        a1 += 360;
                    else if (a2 < 0)
                        a2 += 360;
                    if (target_angle < 0)
                        target_angle += 360;
                    if (bound(a1, a2, target_angle) == target_angle) {
                        can_tm_cut = true;
                    }
                    else {
                    }
                }
                else if (bound(a1, a2, target_angle) == target_angle) {
                    can_tm_cut = true;
                }
                else {
                }
            }
        }
        if (!can_tm_cut)
            opps.push_back(i);
    }
    return opps;
}


int BhvMarkDecisionGreedy::whoBlocker(const WorldModel &wm) {
    return bhv_block().who_is_blocker(wm);
}


double BhvMarkDecisionGreedy::cycleBlockSoft(const WorldModel &wm, Vector2D blocker, Vector2D opp) {
    Vector2D target = opp + Vector2D::polar2vector(5, getDir(wm, opp));
    Line2D target_line = Line2D(opp, target);
    target = target_line.intersection(Line2D(Vector2D(-52.5, 34), Vector2D(-52.5, -34)));
    double dist_to_target = target.dist(opp);
    double dist_discount = dist_to_target / 10.0;
    Vector2D backer = Vector2D::polar2vector(dist_discount, (target - opp).th());
    double opp_target_angle = (target - opp).th().degree();
    double opp_blocker_angle = (blocker - opp).th().degree();
    double dif_mosallas = opp_target_angle - opp_blocker_angle;
    if (dif_mosallas < 0)
        dif_mosallas = -dif_mosallas;
    if (dif_mosallas > 180)
        dif_mosallas = 360 - dif_mosallas;
    if (dif_mosallas < 90) {
        double blocker_opp_angle = (opp - blocker).th().degree();
        double block_sim_angle = blocker_opp_angle - dif_mosallas;
        if (blocker_opp_angle > 0)
            block_sim_angle = blocker_opp_angle + dif_mosallas;
        Line2D blocker_line = Line2D(blocker, block_sim_angle);
        Vector2D block_target = blocker_line.intersection(target_line);
        return opp.dist(block_target);
    }
    else {
        double a = opp.dist(target);
        double b = opp.dist(blocker);
        return sqrt(pow(a, 2) + pow(b, 2) - 2 * a * b * cos(dif_mosallas * M_PI / 180));
    }
}

double BhvMarkDecisionGreedy::getDir(const WorldModel &wm, rcsc::Vector2D start_pos) {
    if (wm.self().unum() == 1) {
        Vector2D goalie = Vector2D(-52, 0);
        double thr_side = 8;
        if (start_pos.x > -40) {
            thr_side = 12;
            if (start_pos.x < -30) {
                thr_side = 8;
            }
            if (start_pos.y < (-1 * thr_side)) {
                goalie.y = -7;
            }
            else if (start_pos.y > thr_side) {
                goalie.y = 7;
            }
        }
        else {
            if (start_pos.y < (-1 * thr_side)) {
                goalie.y = 7;
            }
            else if (start_pos.y > thr_side) {
                goalie.y = -7;
            }
        }
        return (goalie - start_pos).th().degree();
    }
    double best_dir = -180.0;
    double max_eval = -100;
    const ServerParam &sp = ServerParam::i();
    const PlayerObject *nearest_opp = wm.interceptTable()->fastestOpponent();
    for (int i = -180; i < 180; i += 10) {
        Vector2D next_pos = start_pos + Vector2D::polar2vector(5, i);
        if (next_pos.absX() > 52.5 || next_pos.absY() > 34)
            continue;
        double eval = -(next_pos.x);
        eval += std::max(0.0, 40.0 - sp.ourTeamGoalPos().dist(next_pos));
        double dangerPoint = (52.5 - next_pos.absX()) / 10.0;
        for (int t = 1; next_pos.x < -36 && t <= 11; t++) {
            const AbstractPlayerObject *tm = wm.ourPlayer(t);
            if (tm == NULL || tm->unum() < 0) continue;
            double dist = next_pos.dist(tm->pos());
            if (dist <= 10) {
                eval -= (10 - dist) / 4;
            }
        }
        if (eval > max_eval) {
            max_eval = eval;
            best_dir = i;
        }
    }
    for (double dir = best_dir - 10; dir <= best_dir + 10; dir += 2) {
        Vector2D next_pos = start_pos + Vector2D::polar2vector(5, dir);
        if (next_pos.absX() > 52.5 || next_pos.absY() > 34)
            continue;
        double eval = -(next_pos.x);
        eval += std::max(0.0, 40.0 - sp.ourTeamGoalPos().dist(next_pos));
        for (int t = 1; t <= 11; t++) {
            const AbstractPlayerObject *tm = wm.ourPlayer(t);
            if (tm == NULL || tm->unum() < 0) continue;
            double dist = next_pos.dist(tm->pos());
            if (dist <= 10) {
                eval -= (10 - dist) / 4;
            }
        }
        if (eval > max_eval) {
            max_eval = eval;
            best_dir = dir;
        }
    }
    return best_dir;
}


BhvMarkDecisionGreedy::BhvMarkDecisionGreedy() {
    // TODO Auto-generated constructor stub
}


BhvMarkDecisionGreedy::~BhvMarkDecisionGreedy() {
    // TODO Auto-generated destructor stub
}
