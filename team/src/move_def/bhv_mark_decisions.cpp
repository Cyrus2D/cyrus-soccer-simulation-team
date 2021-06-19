//
// Created by nader on 2020-10-17.
//
#include "bhv_mark_decision_greedy.h"
#include "mark_position_finder.h"
#include "best_match_finder.h"
#include "../bhv_basic_move.h"
#include <rcsc/geom.h>
#include "../setting.h"
#include <rcsc/math_util.h>
#include <rcsc/common/server_param.h>

#define DEBUG_PRINT


void BhvMarkDecisionGreedy::midMarkDecision(PlayerAgent *agent, MarkType &mark_type, int &mark_unum, bool &blocked) {
    const WorldModel &wm = agent->world();
    int opp_reach_cycle = wm.interceptTable()->opponentReachCycle();
    Vector2D ball_inertia = wm.ball().inertiaPoint(opp_reach_cycle);
    size_t fastest_opp = (wm.interceptTable()->fastestOpponent() == NULL ? 0
                                                                         : wm.interceptTable()->fastestOpponent()->unum());
    double mark_eval[12][12];
    for (int t = 1; t <= 11; t++) {
        for (int o = 1; o <= 11; o++) {
            mark_eval[o][t] = 1000;
        }
    }

    bool fastest_opp_marked = false; // true: fastest is offensive can not be blocked by def players
    auto offensive_opps = getOppOffensive(wm, fastest_opp_marked);
    vector<double> block_eval = bhv_block::blocker_eval_mark_decision(wm);

    bool used_hpos = false;
    if (opp_reach_cycle > 10) {
        #ifdef DEBUG_PRINT
        dlog.addText(Logger::MARK, " ###used Hpos###");
        #endif
        used_hpos = false;
    }

    MarkType how_mark[12];
    size_t tm_mark_target[12];
    size_t opp_marker[12];
    size_t opp_mark_count[12];
    for (int i = 1; i <= 11; i++) {
        tm_mark_target[i] = 0;
        opp_marker[i] = 0;
        opp_mark_count[i] = 0;
        how_mark[i] = MarkType::NoType;
    }

    //Tm Def Mark Opp Off
    {
        #ifdef DEBUG_PRINT
        dlog.addText(Logger::MARK, "----------------------------------------------------------"
                                   "----------------------------------------------------------------");
        dlog.addText(Logger::MARK, "Tm Def Mark Opp Off");
        #endif
        bool on_anti_offense = isAntiOffensive(wm);
        vector <UnumEval> opp_eval = oppEvaluatorMidMark(wm, offensive_opps);
        Target opp_targets[12];
        midMarkThMarkCostFinder(wm, mark_eval, used_hpos, block_eval, fastest_opp_marked, opp_targets, on_anti_offense);
        vector <size_t> temp_tms = midMarkThMarkMarkerFinder(mark_eval, fastest_opp);
        vector <size_t> temp_opps = midMarkThMarkMarkedFinder(offensive_opps, opp_eval);
        if (Setting::i()->mDefenseMove->mMidTh_RemoveNearOpps)
            temp_opps = midMarkThMarkRemoveCloseOpp(wm, temp_opps, opp_targets);;
        auto action = BestMatchFinder::find_best_dec(mark_eval, temp_tms, temp_opps);
        midMarkThMarkSetResults(wm, action, temp_opps, how_mark, tm_mark_target, opp_marker, opp_mark_count,
                                fastest_opp, fastest_opp_marked, opp_targets);

    }

    //Other mark Other
    if (ball_inertia.x < Setting::i()->mDefenseMove->mMidNear_StartX) {
        #ifdef DEBUG_PRINT
        dlog.addText(Logger::MARK, "-------------------------------------------------------------------------"
                                   "-------------------------------------------------------------------------");
        dlog.addText(Logger::MARK, "Tm Mark Opp");
        #endif
        vector <UnumEval> opp_eval = oppEvaluatorMidMark(wm, offensive_opps, true);
        midMarkLeadMarkCostFinder(wm, mark_eval, used_hpos, block_eval, fastest_opp_marked);
        vector <size_t> temp_tms = midMarkLeadMarkMarkerFinder(mark_eval, fastest_opp, tm_mark_target);
        vector <size_t> temp_opps = midMarkLeadMarkMarkedFinder(wm, offensive_opps, opp_eval, fastest_opp, ball_inertia,
                                                                opp_marker, how_mark);
        auto action = BestMatchFinder::find_best_dec(mark_eval, temp_tms, temp_opps);
        midMarkLeadMarkSetResults(wm, action, temp_opps, how_mark, tm_mark_target, opp_marker, opp_mark_count,
                                  fastest_opp);
    }

    #ifdef DEBUG_PRINT
    for (size_t t = 1; t <= 11; t++) {
        dlog.addText(Logger::MARK, "Tm %d Opp %d in %s", t, tm_mark_target[t], markTypeString(how_mark[t]).c_str());
        if (tm_mark_target[t] > 0) {
            dlog.addLine(Logger::MARK, wm.theirPlayer(tm_mark_target[t])->pos(), wm.ourPlayer(t)->pos(), 255, 0, 0);
        }
    }
    #endif
    for (size_t t = 1; t <= 11; t++) {
        if (how_mark[t] == MarkType::ThMarkFastestOpp || how_mark[t] == MarkType::ThMarkFar)
            how_mark[t] = MarkType::ThMark;
    }

    mark_unum = tm_mark_target[wm.self().unum()];
    mark_type = how_mark[wm.self().unum()];
    for (int i = 1; i <= 11; i++) {
        if (fastest_opp == tm_mark_target[i]) {
            blocked = true;
            break;
        }
    }
}


bool BhvMarkDecisionGreedy::isAntiOffensive(const WorldModel &wm) {
    bool on_anti_offense = false;
    int tm_number = 0;
    double avg_x = 0;
    for (int i = 2; i <= 11; i++) {
        const AbstractPlayerObject *tm = wm.ourPlayer(i);
        if (!tm || tm->unum() != i)
            continue;
        if (Strategy::i().tm_Line(i) == Strategy::PostLine::back && i != 5)
            continue;
        Vector2D tm_pos = tm->pos();
        Vector2D tm_hpos = Strategy::i().getPosition(i);
        tm_number += 1;
        avg_x += (tm_pos.x - tm_hpos.x);
    }
    avg_x /= static_cast<double>(tm_number);
    if (avg_x > 15)
        on_anti_offense = true;
    return on_anti_offense;
}


vector <UnumEval> BhvMarkDecisionGreedy::oppEvaluatorMidMark(const WorldModel &wm, vector<size_t> offensive_opps, bool use_ball_dist) {
    vector <UnumEval> opp_eval;
    size_t fastest_opp = (wm.interceptTable()->fastestOpponent() == NULL ? 0
                                                                         : wm.interceptTable()->fastestOpponent()->unum());
    Vector2D ball_inertia = wm.ball().inertiaPoint(wm.interceptTable()->opponentReachCycle());
    for (size_t o = 1; o <= 11; o++) {
        const AbstractPlayerObject *opp = wm.theirPlayer(o);
        auto EvalNode = opp_eval.insert(opp_eval.end(), make_pair(o, -1000));
        if (opp == NULL
            || opp->unum() < 1
            || opp->goalie())
            continue;
        Vector2D opp_pos = opp->pos();
        if (o == fastest_opp)
            opp_pos = wm.ball().inertiaPoint(wm.interceptTable()->opponentReachCycle());
        double opp_pos_x = -std::max(opp_pos.x - wm.ourDefenseLineX(), 0.0) + 105.0;
        opp_pos_x += std::max(0.0, 50.0 - opp_pos.dist(Vector2D(-52, 0))) * 1.2;
        if (use_ball_dist) {
            double ball_dist = opp_pos.dist(ball_inertia);
            ball_dist = (30 - min(30.0, ball_dist));// / 30.0;
            opp_pos_x += ball_dist;
        }

        if (find(offensive_opps.begin(), offensive_opps.end(), o) != offensive_opps.end()
            && opp_pos.x < wm.ourDefenseLineX() + 10){
            opp_pos_x += 50;
        }
        EvalNode->second = opp_pos_x;
    }
    sort(opp_eval.begin(), opp_eval.end(), [](UnumEval &p1, UnumEval &p2) -> bool { return p1.second > p2.second; });
    for (int o = 1; o < opp_eval.size(); o++) {
        double prev_eval = opp_eval[o - 1].second;
        if (abs(opp_eval[o].second - prev_eval) < 5) {
            opp_eval[o].second = prev_eval;
        }
    }
    opp_eval.insert(opp_eval.begin(), make_pair(0, -1000));
    #ifdef DEBUG_PRINT
    for (size_t i = 1; i <= 11; i++)
        dlog.addText(Logger::MARK, "oppdanger %d is oppunum %d with eval %.2f", i, opp_eval[i].first,
                     opp_eval[i].second);
    dlog.addText(Logger::MARK, "OurDefLine %.1f", wm.ourDefenseLineX());
    #endif
    return opp_eval;
}


void BhvMarkDecisionGreedy::midMarkThMarkCostFinder(const WorldModel &wm, double mark_eval[][12], bool used_hpos,
                                                    vector<double> block_eval, bool fastest_opp_marked,
                                                    Target opp_targets[], bool on_anti_offense) {
    double hpos_z = Setting::i()->mDefenseMove->mMidTh_HPosDistZ;
    double pos_z = Setting::i()->mDefenseMove->mMidTh_PosDistZ;
    double mark_eval_pos[12][12];
    double mark_eval_hpos[12][12];
    int opp_reach_cycle = wm.interceptTable()->opponentReachCycle();
    Vector2D ball_inertia = wm.ball().inertiaPoint(opp_reach_cycle);
    size_t fastest_opp = (wm.interceptTable()->fastestOpponent() == NULL ? 0
                                                                         : wm.interceptTable()->fastestOpponent()->unum());
    double tm_pos_def_line = wm.ourDefenseLineX();
    double tm_hpos_def_line = 0;
    for (int i = 2; i <= 11; i++) {
        double hpos_x = Strategy::i().getPosition(i).x;
        if (hpos_x < tm_hpos_def_line)
            tm_hpos_def_line = hpos_x;
    }
    bool def_is_in_back = false;
    if (tm_pos_def_line < tm_hpos_def_line - 5) {
        def_is_in_back = true;
    }
    for (int t = 1; t <= 11; t++) {
        for (int o = 1; o <= 11; o++) {
            mark_eval[o][t] = 1000;
            mark_eval_pos[o][t] = 1000;
            mark_eval_hpos[o][t] = 1000;
        }
    }
    for (size_t t = 2; t <= 11; t++) {
        const AbstractPlayerObject *tm = wm.ourPlayer(t);
        if (tm == NULL
            || tm->unum() < 1)
            continue;
        Vector2D tm_pos = tm->pos();
        Vector2D tm_hpos = Strategy::i().getPosition(t);
        #ifdef DEBUG_PRINT
        dlog.addText(Logger::MARK, ">>>Tm:%d %.1f,%.1f %.1f,%.1f", t, tm_pos.x, tm_pos.y, tm_hpos.x, tm_hpos.y);
        #endif

        for (size_t o = 1; o <= 11; o++) {
            const AbstractPlayerObject *opp = wm.theirPlayer(o);
            if (opp == NULL
                || opp->unum() < 1)
                continue;
            Target opp_pos;
            opp_pos.pos = opp->pos();
            #ifdef DEBUG_PRINT
            dlog.addText(Logger::MARK, "----Opp:%d %.1f,%.1f", o, opp_pos.pos.x, opp_pos.pos.y);
            #endif
            double max_hpos_dist = 15;
            double max_hpos_dist_y = 15;
            double max_pos_dist = 15;
            if (o == fastest_opp) {
                opp_pos.pos = ball_inertia;
            }
            opp_pos = MarkPositionFinder::getThMarkTarget(t, o, wm);
            opp_targets[o] = opp_pos;
            #ifdef DEBUG_PRINT
            dlog.addText(Logger::MARK, "----DefDec mark off: tm %d opp %d THMark in %.1f,%.1f", t, o, opp_pos.pos.x,
                         opp_pos.pos.y);
            #endif
            if (opp->unum() == fastest_opp && !fastest_opp_marked) {
                max_hpos_dist = Setting::i()->mDefenseMove->mMidTh_HPosMaxDistBlock;
                max_pos_dist = Setting::i()->mDefenseMove->mMidTh_PosMaxDistBlock;
                max_hpos_dist_y = Setting::i()->mDefenseMove->mMidTh_HPosYMaxDistBlock;
                opp_pos.pos = ball_inertia;
                if (tm_pos.dist(opp_pos.pos) < 10 && opp_pos.pos.x < tm_hpos_def_line + 15) {
                    max_hpos_dist = Setting::i()->mDefenseMove->mMidTh_HPosMaxDistBlock * 1.1;
                }
                if (Strategy::i().tm_Line(t) == Strategy::PostLine::back) {
                    if (opp_pos.pos.x > tm_hpos_def_line + 12 && opp_pos.pos.x > wm.ourDefenseLineX() + 12)
                        max_hpos_dist = Setting::i()->mDefenseMove->mMidTh_HPosMaxDistBlock * 0.75;
                }
                if (tm_pos.dist(opp_pos.pos) > max_pos_dist) {
                    #ifdef DEBUG_PRINT
                    dlog.addText(Logger::MARK, "------DefDec mark off tm %d opp %d cancle for MaxPos", t, o);
                    #endif
                    continue;
                }
                if (tm_hpos.dist(opp_pos.pos) > max_hpos_dist) {
                    #ifdef DEBUG_PRINT
                    dlog.addText(Logger::MARK, "------DefDec mark off tm %d opp %d cancle for MaxHPos", t, o);
                    #endif
                    continue;
                }
                if (abs(tm_hpos.y - opp_pos.pos.y) > max_hpos_dist_y) {
                    #ifdef DEBUG_PRINT
                    dlog.addText(Logger::MARK, "------DefDec mark off tm %d opp %d cancle for MaxHPosY", t, o);
                    #endif
                    continue;
                }
                mark_eval[o][t] = block_eval[t];
            }
            else {
                max_hpos_dist = Setting::i()->mDefenseMove->mMidTh_HPosMaxDistMark;
                max_pos_dist = Setting::i()->mDefenseMove->mMidTh_PosMaxDistMark;
                max_hpos_dist_y = Setting::i()->mDefenseMove->mMidTh_HPosYMaxDistMark;
                double dist_tm_opp = opp_pos.pos.dist(used_hpos ? tm_hpos : tm_pos);
                if (tm_pos.dist(opp_pos.pos) < 10 && opp_pos.pos.x < tm_hpos_def_line + 10) {
                    max_hpos_dist_y = Setting::i()->mDefenseMove->mMidTh_HPosYMaxDistMark * 1.3;
                }
                if ((opp_pos.pos.y > 15 && (Strategy::i().tm_Post(t) == Strategy::player_post::pp_rb ||
                                            Strategy::i().tm_Post(t) == Strategy::player_post::pp_rh)
                     && ball_inertia.y > 10)
                    || (opp_pos.pos.y < -15 && (Strategy::i().tm_Post(t) == Strategy::player_post::pp_lb ||
                                                Strategy::i().tm_Post(t) == Strategy::player_post::pp_lh)
                        && ball_inertia.y < -10)) {
                    if (ball_inertia.x > 20) {
                        max_hpos_dist = Setting::i()->mDefenseMove->mMidTh_HPosMaxDistMark * 2.0;
                        max_pos_dist = Setting::i()->mDefenseMove->mMidTh_PosMaxDistMark * 2.0;
                        max_hpos_dist_y = Setting::i()->mDefenseMove->mMidTh_HPosYMaxDistMark * 2.0;
                    }
                    else {
                        max_hpos_dist = Setting::i()->mDefenseMove->mMidTh_HPosMaxDistMark * 1.7;
                        max_pos_dist = Setting::i()->mDefenseMove->mMidTh_PosMaxDistMark * 1.7;
                        max_hpos_dist_y = Setting::i()->mDefenseMove->mMidTh_HPosYMaxDistMark * 1.5;
                    }
                }
                if ((opp_pos.pos.y > 0 && (Strategy::i().tm_Post(t) == Strategy::player_post::pp_rb ||
                                           Strategy::i().tm_Post(t) == Strategy::player_post::pp_rh)
                     && ball_inertia.y < -5)
                    || (opp_pos.pos.y < 0 && (Strategy::i().tm_Post(t) == Strategy::player_post::pp_lb ||
                                              Strategy::i().tm_Post(t) == Strategy::player_post::pp_lh)
                        && ball_inertia.y > +5)) {
                    max_hpos_dist = Setting::i()->mDefenseMove->mMidTh_HPosMaxDistMark * 2.0;
                    max_pos_dist = Setting::i()->mDefenseMove->mMidTh_PosMaxDistMark * 2.0;
                    max_hpos_dist_y = Setting::i()->mDefenseMove->mMidTh_HPosYMaxDistMark * 2.0;
                }
                if (on_anti_offense) {
                    max_hpos_dist = Setting::i()->mDefenseMove->mMidTh_HPosMaxDistMark * 2.0;
                    max_pos_dist = Setting::i()->mDefenseMove->mMidTh_PosMaxDistMark * 2.0;
                    max_hpos_dist_y = Setting::i()->mDefenseMove->mMidTh_HPosYMaxDistMark * 3.0;
                }
                if (tm_pos.dist(opp_pos.pos) > max_pos_dist) {
                    #ifdef DEBUG_PRINT
                    dlog.addText(Logger::MARK, "------DefDec mark off tm %d opp %d cancle for MaxPos", t, o);
                    #endif
                    continue;
                }
                if (def_is_in_back && opp_pos.pos.x < tm_hpos_def_line) {
                    max_hpos_dist = Setting::i()->mDefenseMove->mMidTh_HPosMaxDistMark * 2.5;
                }
                if (tm_hpos.dist(opp_pos.pos) > max_hpos_dist) {

                    #ifdef DEBUG_PRINT
                    dlog.addText(Logger::MARK, "------DefDec mark off tm %d opp %d cancle for MaxHPos", t, o);
                    #endif
                    continue;
                }
                if (abs(tm_hpos.y - opp_pos.pos.y) > max_hpos_dist_y) {
                    #ifdef DEBUG_PRINT
                    dlog.addText(Logger::MARK, "------DefDec mark off tm %d opp %d cancle for MaxHPosY", t, o);
                    #endif
                    continue;
                }
                dist_tm_opp = opp_pos.pos.dist(used_hpos ? tm_hpos : tm_pos) +
                              0.1 * opp_pos.pos.dist(!used_hpos ? tm_hpos : tm_pos);
                dist_tm_opp =
                        (opp_pos.pos.dist(tm_pos) * pos_z + opp_pos.pos.dist(tm_hpos) * hpos_z) / (hpos_z + pos_z);
                mark_eval[o][t] = dist_tm_opp;
                mark_eval_pos[o][t] = opp_pos.pos.dist(tm_pos);
                mark_eval_hpos[o][t] = opp_pos.pos.dist(tm_hpos);
                if (Strategy::i().tm_Line(t) != Strategy::PostLine::back
                    && o != fastest_opp) {
                    if (opp_reach_cycle > 4) {
                        mark_eval[o][t] += 10;
                    }
                }
            }
            if (tm->isTackling())
                mark_eval[o][t] += 15;
            if (t == 5) {
                if (tm->seenStaminaCount() < 10) {
                    if (tm->seenStamina() < 4000) {
                        mark_eval[o][t] += 10;
                    }
                }
            }
            #ifdef DEBUG_PRINT
            dlog.addText(Logger::MARK, "------DefDec mark off tm %d opp %d eval %.2f", t, o, mark_eval[o][t]);
            #endif
        }
    }
    #ifdef DEBUG_PRINT
    for (size_t o = 1; o <= 11; o++) {
        for (size_t t = 1; t <= 11; t++) {
            dlog.addText(Logger::MARK, "opp %d tm %d = %.1f , pos = %.1f , hpos = %.1f", o, t, mark_eval[o][t],
                         mark_eval_pos[o][t], mark_eval_hpos[o][t]);
        }
    }
    #endif
}


vector <size_t> BhvMarkDecisionGreedy::midMarkThMarkMarkerFinder(double (*mark_eval)[12], size_t fastest_opp) {
    vector <size_t> temp_tms;
    for (size_t t = 2; t <= 11; t++) {
        if (Strategy::i().tm_Line(t) == Strategy::PostLine::back) {
            if (Setting::i()->mDefenseMove->mMidTh_BackInMark
                || Setting::i()->mDefenseMove->mMidTh_BackInBlock) {
                dlog.addText(Logger::MARK, "Tm %d can TH Mark", t);
                temp_tms.push_back(t);
            }
            for (size_t o = 2; o <= 11; o++) {
                if (o == fastest_opp) {
                    if (!Setting::i()->mDefenseMove->mMidTh_BackInBlock)
                        mark_eval[o][t] = 1000;
                }
                else {
                    if (!Setting::i()->mDefenseMove->mMidTh_BackInMark)
                        mark_eval[o][t] = 1000;
                }
            }
        }
        else if (Strategy::i().tm_Line(t) == Strategy::PostLine::half) {
            if (Setting::i()->mDefenseMove->mMidTh_HalfInMark
                || Setting::i()->mDefenseMove->mMidTh_HalfInBlock) {
                dlog.addText(Logger::MARK, "Tm %d can TH Mark", t);
                temp_tms.push_back(t);
            }
            for (size_t o = 2; o <= 11; o++) {
                if (o == fastest_opp) {
                    if (!Setting::i()->mDefenseMove->mMidTh_HalfInBlock)
                        mark_eval[o][t] = 1000;
                }
                else {
                    if (!Setting::i()->mDefenseMove->mMidTh_HalfInMark)
                        mark_eval[o][t] = 1000;
                }
            }
        }
        else if (Strategy::i().tm_Line(t) == Strategy::PostLine::forward) {
            if (Setting::i()->mDefenseMove->mMidTh_ForwardInMark
                || Setting::i()->mDefenseMove->mMidTh_ForwardInBlock) {
                dlog.addText(Logger::MARK, "Tm %d can TH Mark", t);
                temp_tms.push_back(t);
            }
            for (size_t o = 2; o <= 11; o++) {
                if (o == fastest_opp) {
                    if (!Setting::i()->mDefenseMove->mMidTh_ForwardInBlock)
                        mark_eval[o][t] = 1000;
                }
                else {
                    if (!Setting::i()->mDefenseMove->mMidTh_ForwardInMark)
                        mark_eval[o][t] = 1000;
                }
            }
        }
    }
    return temp_tms;
}


vector <size_t>
BhvMarkDecisionGreedy::midMarkThMarkMarkedFinder(vector <size_t> &offensive_opps, vector <UnumEval> &opp_eval) {
    vector <size_t> temp_opps;
    int max_opp_marked_here = 4;
    for (size_t d = 1; d <= 6; d++) {
        if (opp_eval[d].second < -999)
            break;
        if (max_opp_marked_here == temp_opps.size())
            break;
        size_t o = opp_eval[d].first;
        if (std::find(offensive_opps.begin(), offensive_opps.end(), o) == offensive_opps.end()) {
            #ifdef DEBUG_PRINT
            dlog.addText(Logger::MARK, "Opp %d is not offensive", o);
            #endif
            continue;
        }
        #ifdef DEBUG_PRINT
        dlog.addText(Logger::MARK, "Opp %d can TH Marked", o);
        #endif
        temp_opps.push_back(o);
    }
    return temp_opps;
}


vector <size_t> BhvMarkDecisionGreedy::midMarkThMarkRemoveCloseOpp(const WorldModel &wm, vector <size_t> &temp_opps,
                                                                   Target opp_targets[]) {
    vector <size_t> new_temp_opps;
    dlog.addText(Logger::MARK, "Start to remove same opp");
    for (int i = 0; i < temp_opps.size(); i++) {
        int o1 = temp_opps[i];
        if (wm.theirPlayer(o1)->pos().x <
            Strategy::i().getPosition(2).x + Setting::i()->mDefenseMove->mMidTh_XNearOpps) {
            dlog.addText(Logger::MARK, "--opp %d added to checked, it is danger", o1);
            new_temp_opps.push_back(o1);
            continue;
        }
        dlog.addText(Logger::MARK, "--opp %d", o1);
        Vector2D o1_target = opp_targets[o1].pos;
        int danger_opp = o1;
        double danger_eval = wm.theirPlayer(o1)->pos().x;
        for (int j = 0; j < temp_opps.size(); j++) {
            if (i == j)continue;
            int o2 = temp_opps[j];
            Vector2D o2_target = opp_targets[o2].pos;
            if (o1_target.dist(o2_target) < Setting::i()->mDefenseMove->mMidTh_DistanceNearOpps) {
                dlog.addText(Logger::MARK, "----opp %d is near, my pos (%.1f, %.1f), other pos (%.1f, %.1f)", o2,
                             o1_target.x, o1_target.y, o2_target.x, o2_target.y);
                if (wm.theirPlayer(o2)->pos().x < danger_eval) {
                    dlog.addText(Logger::MARK, "----opp %d is danger than me", o2);
                    danger_eval = wm.theirPlayer(o2)->pos().x;;
                    danger_opp = o2;
                }
            }
            else {
                dlog.addText(Logger::MARK, "----opp %d is far", o2);
            }
        }
        if (danger_opp == o1) {
            dlog.addText(Logger::MARK, "--->opp %d added to temp", danger_opp);
            new_temp_opps.push_back(danger_opp);
        }
    }
    return new_temp_opps;
}


void BhvMarkDecisionGreedy::midMarkThMarkSetResults(const WorldModel &wm, pair<vector<int>, double> &action,
                                                    vector <size_t> &temp_opps, MarkType how_mark[],
                                                    size_t tm_mark_target[], size_t opp_marker[],
                                                    size_t opp_mark_count[], size_t fastest_opp,
                                                    bool fastest_opp_marked, Target opp_targets[]) {
    for (size_t a = 0; a < action.first.size(); a++) {
        if (action.first[a] == 0)
            continue;
        size_t tm = action.first[a];
        size_t opp = temp_opps[a];
        tm_mark_target[tm] = opp;
        opp_marker[opp] = tm;
        opp_mark_count[opp]++;
        Vector2D opp_pos = wm.theirPlayer(opp)->pos();
        Vector2D mark_target = opp_targets[opp].pos;
        if (opp == fastest_opp && !fastest_opp_marked) {
            how_mark[tm] = MarkType::Block;
            #ifdef DEBUG_PRINT
            dlog.addText(Logger::MARK, "**mark Off: %d block %d", tm, opp);
            #endif
        }
        else {
            if (opp == fastest_opp) {
                how_mark[tm] = MarkType::ThMarkFastestOpp;
                #ifdef DEBUG_PRINT
                dlog.addText(Logger::MARK, "**mark Off: %d THmarkFastestOpp %d", tm, opp);
                #endif
            }
            else if (opp_pos.x > mark_target.x + 5) {
                how_mark[tm] = MarkType::ThMarkFar;
                #ifdef DEBUG_PRINT
                dlog.addText(Logger::MARK, "**mark Off: %d THmarkFar %d", tm, opp);
                #endif
            }
            else {
                how_mark[tm] = MarkType::ThMark;
                #ifdef DEBUG_PRINT
                dlog.addText(Logger::MARK, "**mark Off: %d THmark %d", tm, opp);
                #endif
            }
        }
    }
}


void BhvMarkDecisionGreedy::midMarkLeadMarkCostFinder(const WorldModel &wm, double mark_eval[][12], bool used_hpos,
                                                      vector<double> block_eval, bool fastest_opp_marked) {
    int opp_reach_cycle = wm.interceptTable()->opponentReachCycle();
    Vector2D ball_inertia = wm.ball().inertiaPoint(opp_reach_cycle);
    size_t fastest_opp = (wm.interceptTable()->fastestOpponent() == NULL ? 0
                                                                         : wm.interceptTable()->fastestOpponent()->unum());
    for (int t = 1; t <= 11; t++) {
        for (int o = 1; o <= 11; o++) {
            mark_eval[o][t] = 1000;
        }
    }
    for (int t = 2; t <= 11; t++) {
        const AbstractPlayerObject *tm = wm.ourPlayer(t);
        if (tm == NULL
            || tm->unum() < 1)
            continue;
        Vector2D tm_pos = tm->pos();
        Vector2D tm_hpos = Strategy::i().getPosition(t);
        #ifdef DEBUG_PRINT
        dlog.addText(Logger::MARK, "###Tm:%d %.1f,%.1f %.1f,%.1f", t, tm_pos.x, tm_pos.y, tm_hpos.x, tm_hpos.y);
        #endif
        for (int o = 1; o <= 11; o++) {
            const AbstractPlayerObject *opp = wm.theirPlayer(o);
            if (opp == NULL
                || opp->unum() < 1)
                continue;
            Target opp_pos;
            opp_pos.pos = opp->pos();
            #ifdef DEBUG_PRINT
            dlog.addText(Logger::MARK, "-----Opp:%d %.1f,%.1f", o, opp_pos.pos.x, opp_pos.pos.y);
            #endif
            opp_pos = MarkPositionFinder::getLeadNearMarkTarget(t, o, wm);
            double max_hpos_dist = Setting::i()->mDefenseMove->mMidNear_HPosMaxDistMark; //15
            double max_pos_dist = Setting::i()->mDefenseMove->mMidNear_PosMaxDistMark; //20
            if (opp->unum() == fastest_opp) {
                max_hpos_dist = Setting::i()->mDefenseMove->mMidNear_HPosMaxDistBlock; //20
                max_pos_dist = Setting::i()->mDefenseMove->mMidNear_PosMaxDistBlock; //20
                opp_pos.pos = ball_inertia;
                if (Strategy::i().tm_Post(t) == Strategy::i().player_post::pp_ch
                    && ball_inertia.x > Strategy::i().getPosition(2).x + 10) {
                    max_hpos_dist = Setting::i()->mDefenseMove->mMidNear_HPosMaxDistBlock * 0.75;
                    if (!canCenterHalfMarkLeadNear(wm, t, opp_pos.pos, ball_inertia)) {
                        #ifdef DEBUG_PRINT
                        dlog.addText(Logger::MARK, "------DefDec mark off tm %d opp %d max hpos changed, it is ch", t,
                                     o);
                        #endif
                        max_hpos_dist = Setting::i()->mDefenseMove->mMidProj_HPosMaxDistBlock * 0.5;;
                    }
                }
                if (tm_pos.dist(opp_pos.pos) > max_pos_dist) {
                    #ifdef DEBUG_PRINT
                    dlog.addText(Logger::MARK, "---------DefDec mark tm %d opp %d cancle for MaxPos", t, o);
                    #endif
                    continue;
                }
                if (tm_hpos.dist(opp_pos.pos) > max_hpos_dist) {
                    #ifdef DEBUG_PRINT
                    dlog.addText(Logger::MARK, "---------DefDec mark tm %d opp %d cancle for MaxHPos", t, o);
                    #endif
                    continue;
                }
                mark_eval[o][t] = block_eval[t];
            }
            else {
                if (!opp_pos.pos.isValid())
                    continue;
                double dist_tm_opp = opp_pos.pos.dist(use_home_pos ? tm_hpos : tm_pos);
                if (tm_pos.dist(opp_pos.pos) < 10) {
                    max_hpos_dist *= 1.3; //20
                }
                if (opp_pos.pos.dist(ball_inertia) < 25) {
                    max_hpos_dist *= 1.3;
//                    max_pos_dist = 20;
                }
                if (Strategy::i().tm_Post(t) != Strategy::i().player_post::pp_ch)
                    max_hpos_dist /= 1.5;
                if (Strategy::i().tm_Line(t) == Strategy::PostLine::forward) {
                    if (abs(opp_pos.pos.y - tm_hpos.y) < 15)
                        max_hpos_dist *= 1.3;
                }
                if (!canCenterHalfMarkLeadNear(wm, t, opp_pos.pos, ball_inertia)) {
                    #ifdef DEBUG_PRINT
                    dlog.addText(Logger::MARK, "---------DefDec mark tm %d opp %d cancle for center half", t, o);
                    #endif
                    continue;
                }
                if (tm_pos.dist(opp_pos.pos) > max_pos_dist) {
                    #ifdef DEBUG_PRINT
                    dlog.addText(Logger::MARK, "---------DefDec mark tm %d opp %d cancle for MaxPos", t, o);
                    #endif
                    continue;
                }
                if (tm_hpos.dist(opp_pos.pos) > max_hpos_dist) {
                    #ifdef DEBUG_PRINT
                    dlog.addText(Logger::MARK, "---------DefDec mark tm %d opp %d cancle for MaxHPos", t, o);
                    #endif
                    continue;
                }
                if ((opp_pos.pos.x - tm_hpos.x) > 15) {
                    #ifdef DEBUG_PRINT
                    dlog.addText(Logger::MARK, "---------DefDec mark tm %d opp %d cancle for MaxHPosX", t, o);
                    #endif
                    continue;
                }
                mark_eval[o][t] = dist_tm_opp;
            }
            if (tm->isTackling())
                mark_eval[o][t] += 15;
            #ifdef DEBUG_PRINT
            dlog.addText(Logger::MARK, "---------DefDec mark tm %d opp %d eval %.2f", t, o, mark_eval[o][t]);
            #endif
        }
    }
    #ifdef DEBUG_PRINT
    for (size_t o = 1; o <= 11; o++) {
        for (size_t t = 1; t <= 11; t++) {
            dlog.addText(Logger::MARK, "opp %d tm %d = %.1f", o, t, mark_eval[o][t]);
        }
    }
    #endif
}


vector <size_t> BhvMarkDecisionGreedy::midMarkLeadMarkMarkerFinder(double (*mark_eval)[12], size_t fastest_opp,
                                                                   size_t tm_mark_target[12]) {
    vector <size_t> temp_tms;
    for (size_t t = 2; t <= 11; t++) {
        if (tm_mark_target[t] != 0)
            continue;
        if (Strategy::i().tm_Line(t) == Strategy::PostLine::back) {
            if (Setting::i()->mDefenseMove->mMidNear_BackInMark
                || Setting::i()->mDefenseMove->mMidNear_BackInBlock) {
                dlog.addText(Logger::MARK, "Tm %d can Near Mark", t);
                temp_tms.push_back(t);
            }
            for (size_t o = 2; o <= 11; o++) {
                if (o == fastest_opp) {
                    if (!Setting::i()->mDefenseMove->mMidNear_BackInBlock)
                        mark_eval[o][t] = 1000;
                }
                else {
                    if (!Setting::i()->mDefenseMove->mMidNear_BackInMark)
                        mark_eval[o][t] = 1000;
                }
            }
        }
        else if (Strategy::i().tm_Line(t) == Strategy::PostLine::half) {
            if (Setting::i()->mDefenseMove->mMidNear_HalfInMark
                || Setting::i()->mDefenseMove->mMidNear_HalfInBlock) {
                dlog.addText(Logger::MARK, "Tm %d can Near Mark", t);
                temp_tms.push_back(t);
            }
            for (size_t o = 2; o <= 11; o++) {
                if (o == fastest_opp) {
                    if (!Setting::i()->mDefenseMove->mMidNear_HalfInBlock)
                        mark_eval[o][t] = 1000;
                }
                else {
                    if (!Setting::i()->mDefenseMove->mMidNear_HalfInMark)
                        mark_eval[o][t] = 1000;
                }
            }
        }
        else if (Strategy::i().tm_Line(t) == Strategy::PostLine::forward) {
            if (Setting::i()->mDefenseMove->mMidNear_ForwardInMark
                || Setting::i()->mDefenseMove->mMidNear_ForwardInBlock) {
                dlog.addText(Logger::MARK, "Tm %d can Near Mark", t);
                temp_tms.push_back(t);
            }
            for (size_t o = 2; o <= 11; o++) {
                if (o == fastest_opp) {
                    if (!Setting::i()->mDefenseMove->mMidNear_ForwardInBlock)
                        mark_eval[o][t] = 1000;
                }
                else {
                    if (!Setting::i()->mDefenseMove->mMidNear_ForwardInMark)
                        mark_eval[o][t] = 1000;
                }
            }
        }
    }
    return temp_tms;
}


vector <size_t>
BhvMarkDecisionGreedy::midMarkLeadMarkMarkedFinder(const WorldModel &wm, vector <size_t> &offensive_opps,
                                                   vector <UnumEval> &opp_eval, size_t fastest_opp,
                                                   Vector2D &ball_inertia, size_t opp_marker[], MarkType how_mark[]) {
    vector <size_t> temp_opps;
    for (int d = 1; d <= 11; d++) {
        if (opp_eval[d].second < -999)
            break;
        size_t o = opp_eval[d].first;
        const AbstractPlayerObject *Opp = wm.theirPlayer(o);
        if (Opp->unum() != fastest_opp)
            if (Opp->pos().x > ball_inertia.x + Setting::i()->mDefenseMove->mMidNear_OppsDistXToBall)
                continue;
        if (opp_marker[o] != 0) {

            if (Opp->unum() == fastest_opp) {
//                        if (!Setting::i()->mDefenseMove->mMidNear_BlockAgain) {
                if (how_mark[opp_marker[Opp->unum()]] != MarkType::ThMarkFastestOpp)
                    continue;
            }
            else {
                if (how_mark[opp_marker[Opp->unum()]] == MarkType::ThMark)
                    continue;
//                        if (Opp->pos().x < wm.ourDefensePlayerLineX() + 10)
//                            continue;
//                        if (!Setting::i()->mDefenseMove->mMidNear_MarkAgain)
//                            continue;
//                        if(Opp->pos().dist(Opp->pos()) < Setting::i()->mDefenseMove->mMidNear_MarkAgainMaxDistToChangeCost)
//                            for(int t=1; t <= 11; t++){
//                                mark_eval[o][t] *= Setting::i()->mDefenseMove->mMidNear_MarkAgainChangeCostZ;
//                            }
            }
        }
        temp_opps.push_back(o);
        #ifdef DEBUG_PRINT
        dlog.addText(Logger::MARK, "Opp %d can be LN Marked", o);
        #endif
    }
    return temp_opps;
}


void BhvMarkDecisionGreedy::midMarkLeadMarkSetResults(const WorldModel &wm, pair<vector<int>, double> &action,
                                                      vector <size_t> &temp_opps, MarkType how_mark[],
                                                      size_t tm_mark_target[], size_t opp_marker[],
                                                      size_t opp_mark_count[], size_t fastest_opp) {
    for (size_t a = 0; a < action.first.size(); a++) {
        if (action.first[a] == 0)
            continue;
        size_t tm = action.first[a];
        size_t opp = temp_opps[a];
        tm_mark_target[tm] = opp;
        opp_marker[opp] = tm;
        opp_mark_count[opp]++;
        if (opp == fastest_opp) {
            how_mark[tm] = MarkType::Block;
            #ifdef DEBUG_PRINT
            dlog.addText(Logger::MARK, "mark Off: %d block %d", tm, opp);
            #endif
        }
        else {
            if (how_mark[tm] == MarkType::NoType) {
                how_mark[tm] = MarkType::LeadProjectionMark;
                #ifdef DEBUG_PRINT
                dlog.addText(Logger::MARK, "mark Off: %d LP %d", tm, opp);
                #endif
            }
            else {
                how_mark[tm] = MarkType::LeadNearMark;
                #ifdef DEBUG_PRINT
                dlog.addText(Logger::MARK, "mark Off: %d LN %d", tm, opp);
                #endif
            }

        }
    }
}


bool BhvMarkDecisionGreedy::needProjectMark(const WorldModel &wm, int opp_unum, int tm_unum) {
    const AbstractPlayerObject *opp = wm.theirPlayer(opp_unum);
    int opp_min = wm.interceptTable()->opponentReachCycle();
    Vector2D ball_inertia = wm.ball().inertiaPoint(opp_min);
    double first_pass_speed = calc_first_term_geom_series_last(1.5,
                                                               ball_inertia.dist(opp->pos()),
                                                               ServerParam::i().ballDecay());
    if (first_pass_speed > 3.0)
        first_pass_speed = calc_first_term_geom_series_last(1.0,
                                                            ball_inertia.dist(opp->pos()),
                                                            ServerParam::i().ballDecay());
    if (first_pass_speed > 3.0)
        first_pass_speed = calc_first_term_geom_series_last(0.5,
                                                            ball_inertia.dist(opp->pos()),
                                                            ServerParam::i().ballDecay());
    if (first_pass_speed > 3.0)
        first_pass_speed = calc_first_term_geom_series_last(0.0,
                                                            ball_inertia.dist(opp->pos()),
                                                            ServerParam::i().ballDecay());
    if (first_pass_speed > 3.0)
        return false;

    int pass_cycle = calc_length_geom_series(first_pass_speed,
                                             ball_inertia.dist(opp->pos()),
                                             ServerParam::i().ballDecay());
    Vector2D ball_pos = ball_inertia;
    Vector2D ball_vel = Vector2D::polar2vector(first_pass_speed, (opp->pos() - ball_inertia).th());
    Segment2D pass_segment(ball_inertia, opp->pos());
    for (int c = 1; c < pass_cycle; c++) {
        ball_pos += ball_vel;
        for (int t = 2; t <= 11; t++) {
            if (t == tm_unum)
                continue;
            const AbstractPlayerObject *tm = wm.ourPlayer(t);
            if (tm == nullptr || tm->unum() != t)
                continue;
            if (!pass_segment.projection(tm->pos()).isValid())
                continue;
            int reach_cycle = tm->playerTypePtr()->cyclesToReachDistance(ball_pos.dist(tm->pos()));
            if (reach_cycle < c)
                return false;
        }
        ball_vel *= ServerParam::i().ballDecay();
    }
    return true;
}


bool
BhvMarkDecisionGreedy::canCenterHalfMarkLeadNear(const WorldModel &wm, int t, Vector2D opp_pos, Vector2D ball_inertia) {
    if (Strategy::i().tm_Post(t) != Strategy::i().player_post::pp_ch)
        return true;
    const AbstractPlayerObject *tm5 = wm.ourPlayer(5);
    const AbstractPlayerObject *tm2 = wm.ourPlayer(2);
    if (!tm2 || !tm5 || tm2->unum() != 2 || tm5->unum() != 5)
        return false;

    if (tm2->pos().dist(tm5->pos()) > 15) {
        double max_y = std::max(tm2->pos().y, tm5->pos().y);
        double min_y = std::min(tm2->pos().y, tm5->pos().y);
        if (opp_pos.y > max_y + 5.0 || opp_pos.y < min_y - 5.0) {
            return false;
        }
    }
    double avg_y = (tm2->pos().y + tm5->pos().y) / 2.0;
    //TODO it needs a check
    if (opp_pos.y > avg_y + 15 || opp_pos.y < avg_y - 15)
        return false;
    if (opp_pos.x > (tm2->pos().x + tm5->pos().x) / 2 + 15)
        return false;
    return true;
}


void BhvMarkDecisionGreedy::goalMarkDecision(PlayerAgent *agent, MarkType &mark_type, int &mark_unum, bool &blocked) {
    const WorldModel &wm = agent->world();
    int fastest_opp = (wm.interceptTable()->fastestOpponent() == NULL ? 0
                                                                      : wm.interceptTable()->fastestOpponent()->unum());
    Vector2D ball_pos = wm.ball().inertiaPoint(wm.interceptTable()->opponentReachCycle());
    //determine arbitrary offside line
    double tm_hpos_x_min = 0;
    for (int i = 2; i <= 11; i++) {
        double tm_hpos_x = Strategy::i().getPosition(i).x;
        if (tm_hpos_x < tm_hpos_x_min)
            tm_hpos_x_min = tm_hpos_x;
    }
    double mark_eval[12][12];
    for (int t = 1; t <= 11; t++) {
        for (int o = 1; o <= 11; o++) {
            mark_eval[o][t] = 1000;
        }
    }

    vector<int> who_go_to_goal = Bhv_BasicMove::who_goto_goal(agent);
    vector <UnumEval> opp_eval = oppEvaluatorGoalMark(wm);

    MarkType how_mark[12];
    int tm_mark_target[12];
    for (int i = 1; i <= 11; i++) {
        how_mark[i] = MarkType::NoType;
        tm_mark_target[i] = 0;
    }

    goalMarkLeadMarkCostFinder(wm, mark_eval, who_go_to_goal);
    {
        vector <size_t> temp_opps;
        vector <size_t> temp_tms;
        for (int t = 2; t < 12; ++t) {
            if (Strategy::i().tm_Line(t) == Strategy::PostLine::forward) {
                if (!Setting::i()->mDefenseMove->mGoal_ForwardInMark
                    && !Setting::i()->mDefenseMove->mGoal_ForwardInBlock)
                    continue;
                for (size_t o = 2; o <= 11; o++) {
                    if (o == fastest_opp) {
                        if (!Setting::i()->mDefenseMove->mGoal_ForwardInBlock)
                            mark_eval[o][t] = 1000;
                    }
                    else {
                        if (!Setting::i()->mDefenseMove->mGoal_ForwardInMark)
                            mark_eval[o][t] = 1000;
                    }
                }
            }
            temp_tms.push_back(t);
        }

        for (int d = 0; d < 6; d++) {
            if (opp_eval[d].second == -1000)
                break;
            int o = opp_eval[d].first;
            temp_opps.push_back(o);
        }
        auto action = BestMatchFinder::find_best_dec(mark_eval, temp_tms, temp_opps);
        #ifdef DEBUG_PRINT
        dlog.addText(Logger::MARK, "after action created");
        #endif
        for (int a = 0; a < action.first.size(); a++) {
            if (action.first[a] == 0)
                continue;
            int tm = action.first[a];
            int opp = temp_opps[a];
            tm_mark_target[tm] = opp;
            if (opp == fastest_opp) {
                how_mark[tm] = MarkType::Block;
                #ifdef DEBUG_PRINT
                dlog.addText(Logger::MARK, ">>>>mark Off: tm %d block opp %d", tm, opp);
                #endif
            }
            else {
                how_mark[tm] = MarkType::DangerMark;
                #ifdef DEBUG_PRINT
                dlog.addText(Logger::MARK, ">>>>mark Off: tm %d danger opp %d", tm, opp);
                #endif
            }
        }
    }
    #ifdef DEBUG_PRINT
    for (size_t t = 1; t <= 11; t++) {
        dlog.addText(Logger::MARK, "Tm %d Opp %d in %s", t, tm_mark_target[t], markTypeString(how_mark[t]).c_str());
        if (tm_mark_target[t] > 0) {
            dlog.addLine(Logger::MARK, wm.theirPlayer(tm_mark_target[t])->pos(), wm.ourPlayer(t)->pos(), 255, 0, 0);
        }
    }
    #endif
    mark_unum = tm_mark_target[wm.self().unum()];
    mark_type = how_mark[wm.self().unum()];
    for (int i = 1; i <= 11; i++) {
        if (fastest_opp == tm_mark_target[i]) {
            blocked = true;
            break;
        }
    }
    if (how_mark[wm.self().unum()] != MarkType::NoType) {
        last_mark = make_pair(wm.time().cycle(), tm_mark_target[wm.self().unum()]);
    }
}


vector <UnumEval> BhvMarkDecisionGreedy::oppEvaluatorGoalMark(const WorldModel &wm) {
    vector <UnumEval> opp_eval;
    Vector2D ball_pos = wm.ball().inertiaPoint(wm.interceptTable()->opponentReachCycle());
    for (int o = 1; o <= 11; o++) {
        const AbstractPlayerObject *Opp = wm.theirPlayer(o);
        auto EvalNode = opp_eval.insert(opp_eval.end(), make_pair(o, -1000));
        if (Opp == NULL
            || Opp->unum() < 1
            || Opp->goalie()
            || Opp->pos().x > -20)
            continue;
        double DistOppGoal = max(0.0, 40.0 - Opp->pos().dist(Vector2D(-52, 0))) - Opp->pos().x;
        if (ball_pos.x < -30) {
            if (ball_pos.absY() + 5 < Opp->pos().absY()
                && Opp->pos().absY() > 15) {
                DistOppGoal /= 2.0;
            }
        }
        EvalNode->second = DistOppGoal;
    }
    sort(opp_eval.begin(), opp_eval.end(), [](UnumEval &p1, UnumEval &p2) -> bool { return p1.second > p2.second; });
    #ifdef DEBUG_PRINT
    for (auto &pe : opp_eval)
        dlog.addText(Logger::MARK, "sorted:opp %d eval %.1f", pe.first, pe.second);
    #endif
    return opp_eval;
}


void BhvMarkDecisionGreedy::goalMarkLeadMarkCostFinder(const WorldModel &wm, double mark_eval[][12],
                                                       vector<int> who_go_to_goal) {
    int fastest_opp = (wm.interceptTable()->fastestOpponent() == NULL ? 0
                                                                      : wm.interceptTable()->fastestOpponent()->unum());
    Vector2D ball_pos = wm.ball().inertiaPoint(wm.interceptTable()->opponentReachCycle());
    vector<double> block_eval = bhv_block::blocker_eval(wm);
    for (int t = 1; t <= 11; t++) {
        for (int o = 1; o <= 11; o++) {
            mark_eval[o][t] = 1000;
        }
    }
    double tm_hpos_x_min = 0;
    for (int i = 2; i <= 11; i++) {
        double tm_hpos_x = Strategy::i().getPosition(i).x;
        if (tm_hpos_x < tm_hpos_x_min)
            tm_hpos_x_min = tm_hpos_x;
    }
    Rect2D top_corner_rect = Rect2D(Vector2D(-52.5, -34.0), Vector2D(-30.0, -13.0));
    Rect2D bottom_corner_rect = Rect2D(Vector2D(-52.5, 13.0), Vector2D(-30.0, 34.0));
    Rect2D top_corner_rect_ball = Rect2D(Vector2D(-52.5, -34.0), Vector2D(-30.0, -20.0));
    Rect2D bottom_corner_rect_ball = Rect2D(Vector2D(-52.5, 20.0), Vector2D(-30.0, 34.0));

    for (int t = 2; t <= 11; t++) {
        const AbstractPlayerObject *tm = wm.ourPlayer(t);
        if (tm == NULL
            || tm->unum() < 1)
            continue;
        #ifdef DEBUG_PRINT
        dlog.addText(Logger::MARK, "###tm %d", t);
        #endif
        bool goto_goal = false;
        for (auto go_goal_nums : who_go_to_goal) {
            if (go_goal_nums == tm->unum())
                goto_goal = true;
        }
        if (goto_goal) {
            #ifdef DEBUG_PRINT
            dlog.addText(Logger::MARK, "------continue because goto goal for tm %d", t);
            #endif
            continue;
        }
        const Vector2D &tm_pos = tm->pos();
        const Vector2D &tm_hpos = Strategy::i().getPosition(t);
        for (int o = 1; o <= 11; o++) {
            const AbstractPlayerObject *opp = wm.theirPlayer(o);
            if (opp == NULL
                || opp->unum() < 1)
                continue;
            #ifdef DEBUG_PRINT
            dlog.addText(Logger::MARK, "-----$opp %d", o);
            #endif
            Target opp_pos;
            opp_pos.pos = opp->pos();
            double max_hpos_dist = Setting::i()->mDefenseMove->mGoal_HPosMaxDistMark;
            double max_pos_dist = Setting::i()->mDefenseMove->mGoal_PosMaxDistMark;
            double max_dist_to_offside = Setting::i()->mDefenseMove->mGoal_OffsideMaxDistMark;
            opp_pos = MarkPositionFinder::getDengerMarkTarget(t, o, wm);
            if (opp->unum() == fastest_opp) {
                max_hpos_dist = Setting::i()->mDefenseMove->mGoal_HPosMaxDistBlock;
                max_pos_dist = Setting::i()->mDefenseMove->mGoal_PosMaxDistBlock;
                max_dist_to_offside = Setting::i()->mDefenseMove->mGoal_OffsideMaxDistBlock;
                if (Strategy::i().tm_Line(t) == Strategy::PostLine::back) {
                    max_hpos_dist = Setting::i()->mDefenseMove->mGoal_HPosMaxDistBlock * 0.6;
                    max_pos_dist = Setting::i()->mDefenseMove->mGoal_PosMaxDistBlock * 0.6;
                    max_dist_to_offside = Setting::i()->mDefenseMove->mGoal_OffsideMaxDistBlock * 0.6;
                }
                if (opp_pos.pos.x > tm_hpos_x_min + max_dist_to_offside){
                    #ifdef DEBUG_PRINT
                    dlog.addText(Logger::MARK, "--------cancel for max_dist_to_offside %.1f > %.1f", opp_pos.pos.x, tm_hpos_x_min + max_dist_to_offside);
                    #endif
                    continue;
                }
                if (opp_pos.pos.dist(tm_pos) > max_pos_dist){
                    #ifdef DEBUG_PRINT
                    dlog.addText(Logger::MARK, "--------cancel for max_pos_dist %.1f > %.1f", opp_pos.pos.dist(tm_pos), max_pos_dist);
                    #endif
                    continue;
                }
                if (opp_pos.pos.dist(tm_hpos) > max_hpos_dist) {
                    #ifdef DEBUG_PRINT
                    dlog.addText(Logger::MARK, "--------cancel for max_hpos_dist %.1f > %.1f", opp_pos.pos.dist(tm_pos), max_hpos_dist);
                    #endif
                    continue;
                }
                mark_eval[o][t] = block_eval[t];
            }
            else {
                double dist_tm_opp = opp_pos.pos.dist(use_home_pos ? tm_hpos : tm_pos);
                if (ball_pos.x < -30) {
                    if (ball_pos.absY() + 5 < opp_pos.pos.absY()
                        && opp_pos.pos.absY() > 12) {
                        dist_tm_opp *= 2;
                    }
                }
                if ((top_corner_rect_ball.contains(ball_pos) && top_corner_rect.contains(opp->pos()) &&
                     top_corner_rect.contains(tm_hpos))
                    || (bottom_corner_rect_ball.contains(ball_pos) && bottom_corner_rect.contains(opp->pos()) &&
                        bottom_corner_rect.contains(tm_hpos))) {
                    max_hpos_dist = Setting::i()->mDefenseMove->mGoal_HPosMaxDistMark * 2.0;
                    max_pos_dist = Setting::i()->mDefenseMove->mGoal_PosMaxDistMark * 2.0;
                    max_dist_to_offside = Setting::i()->mDefenseMove->mGoal_OffsideMaxDistMark * 2.0;
                }
                if (opp_pos.pos.x > tm_hpos_x_min + max_dist_to_offside){
                    #ifdef DEBUG_PRINT
                    dlog.addText(Logger::MARK, "--------cancel for max_dist_to_offside %.1f > %.1f", opp_pos.pos.x, tm_hpos_x_min + max_dist_to_offside);
                    #endif
                    continue;
                }
                if (opp_pos.pos.dist(tm_pos) > max_pos_dist){
                    #ifdef DEBUG_PRINT
                    dlog.addText(Logger::MARK, "--------cancel for max_pos_dist %.1f > %.1f", opp_pos.pos.dist(tm_pos), max_pos_dist);
                    #endif
                    continue;
                }
                if (opp_pos.pos.dist(tm_hpos) > max_hpos_dist) {
                    #ifdef DEBUG_PRINT
                    dlog.addText(Logger::MARK, "--------cancel for max_hpos_dist %.1f > %.1f", opp_pos.pos.dist(tm_pos), max_hpos_dist);
                    #endif
                    continue;
                }
                mark_eval[o][t] = dist_tm_opp;
            }
            if (tm->isTackling())
                mark_eval[o][t] += 10;
            #ifdef DEBUG_PRINT
            dlog.addText(Logger::MARK, "-----$def mark off tm %d opp %d eval %.2f", t, o, mark_eval[o][t]);
            #endif
        }
    }
}


void
BhvMarkDecisionGreedy::antiDefMarkDecision(const WorldModel &wm, MarkType &marktype, int &markunum, bool &blocked) {
    int fastest_opp = (wm.interceptTable()->fastestOpponent() == NULL ? 0
                                                                      : wm.interceptTable()->fastestOpponent()->unum());
    double mark_eval[12][12];
    for (int t = 1; t <= 11; t++) {
        for (int o = 1; o <= 11; o++) {
            mark_eval[o][t] = 1000;
        }
    }
    int OppUnum[12];
    double opp_eval[12];
    for (int o = 1; o <= 11; o++) {
        const AbstractPlayerObject *Opp = wm.theirPlayer(o);
        OppUnum[o] = o;
        opp_eval[o] = -1000;
        if (Opp == NULL
            || Opp->unum() < 1
            || Opp->goalie())
            continue;
        double DistOppGoal = -Opp->pos().x;
        opp_eval[o] = DistOppGoal;
    }
    //    for (int i = 1; i <= 11; i++)
    //        dlog.addText(Logger::MARK, "oppdanger %d is oppunum %d with eval %.2f", i, OppUnum[i], opp_eval[i]);
    for (int t = 2; t <= 11; t++) {
        const AbstractPlayerObject *Tm = wm.ourPlayer(t);
        if (Tm == NULL
            || Tm->unum() < 1
            || Strategy::i().tm_Line(t) == Strategy::PostLine::forward
            || Strategy::i().tm_Line(t) == Strategy::PostLine::half)
            continue;
        Vector2D tm_pos = Tm->pos();
        Vector2D tm_hpos = Strategy::i().getPosition(t);
        for (int o = 1; o <= 11; o++) {
            const AbstractPlayerObject *Opp = wm.theirPlayer(o);
            if (Opp == NULL
                || Opp->unum() < 1
                || Opp->goalie()
                || o == wm.interceptTable()->fastestOpponent()->unum())
                continue;
            Vector2D opp_pos = Opp->pos();
            double dist_opp_tm = opp_pos.dist(tm_pos);
            if (dist_opp_tm > 15
                || tm_hpos.dist(opp_pos) > 15)
                continue;
            mark_eval[o][t] = dist_opp_tm;
        }
    }
    for (int i = 1; i <= 11; i++) {
        for (int j = i; j <= 11; j++) {
            if (opp_eval[i] < opp_eval[j]) {
                double tmpe = opp_eval[i];
                opp_eval[i] = opp_eval[j];
                opp_eval[j] = tmpe;
                int tmpu = OppUnum[i];
                OppUnum[i] = OppUnum[j];
                OppUnum[j] = tmpu;
            }
        }
    }
    //    for (int i = 1; i <= 11; i++)
    //        dlog.addText(Logger::MARK, "oppdanger %d is oppunum %d with eval %.2f", i, OppUnum[i], opp_eval[i]);
    MarkType how_mark[12];
    int tm_mark_target[12];
    int opp_marker[12];
    for (int i = 1; i <= 11; i++) {
        tm_mark_target[i] = 0;
        opp_marker[i] = 0;
        how_mark[i] = MarkType::NoType;
    }
    for (int d = 1; d <= 3; d++) {
        if (opp_eval[d] == -1000)
            break;
        int o = OppUnum[d];
        double BestEval = 1000;
        int BestTm = 0;
        for (int t = 2; t <= 11; t++) {
            if (Strategy::i().tm_Line(t) != Strategy::PostLine::back)
                continue;
            if (t == whoBlocker(wm))
                continue;
            if (mark_eval[o][t] < BestEval) {
                BestEval = mark_eval[o][t];
                BestTm = t;
            }
        }
        //        dlog.addText(Logger::MARK, "o: %d t: %d", o, BestTm);
        if (BestTm == 0)
            continue;
        tm_mark_target[BestTm] = o;
        opp_marker[o] = BestTm;
        how_mark[BestTm] = MarkType::LeadNearMark;
    }
    markunum = tm_mark_target[wm.self().unum()];
    marktype = how_mark[wm.self().unum()];
    for (int i = 1; i <= 11; i++) {
        if (fastest_opp == tm_mark_target[i]) {
            blocked = true;
            break;
        }
    }
}
