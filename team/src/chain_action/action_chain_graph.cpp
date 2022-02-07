// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hiroki SHIMORA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "action_chain_graph.h"
#include "../strategy.h"
#include "hold_ball.h"
#include "../setting.h"
#include "DataExtractor.h"

#include <rcsc/player/player_agent.h>
#include <rcsc/common/server_param.h>
#include <rcsc/common/logger.h>
#include <rcsc/time/timer.h>
#include <rcsc/player/intercept_table.h>
#include <string>
#include <sstream>
#include <queue>
#include <utility>
#include <limits>
#include <cstdio>
#include <cmath>

// #define DEBUG_PROFILE
// #define ACTION_CHAIN_DEBUG
// #define DEBUG_PAINT_EVALUATED_POINTS

//#define ACTION_CHAIN_LOAD_DEBUG
//#define DEBUG_COMPARE_SEARCH_TYPES

#ifdef ACTION_CHAIN_LOAD_DEBUG
#include <iostream>
#endif

using namespace rcsc;

size_t ActionChainGraph::DEFAULT_MAX_CHAIN_LENGTH = 3;
size_t ActionChainGraph::DEFAULT_MAX_EVALUATE_LIMIT = 2000;

std::vector< std::pair< Vector2D, double > > ActionChainGraph::S_evaluated_points;


namespace {

const double HEAT_COLOR_SCALE = 128.0;
const double HEAT_COLOR_PERIOD = 2.0 * M_PI;

inline
int
heat_color( const double & x )
{
    //return std::floor( ( std::cos( x ) + 1.0 ) * HEAT_COLOR_SCALE );
    return static_cast< int >( std::floor( ( std::cos( x ) + 1.0 ) * HEAT_COLOR_SCALE ) );
}

inline
void
debug_paint_evaluate_color( const Vector2D & pos,
                            const double & value,
                            const double & min,
                            const double & max )
{
    double position = ( value - min ) / ( max - min );
    if ( position < 0.0 ) position = -position;
    if ( position > 2.0 ) position = std::fmod( position, 2.0 );
    if ( position > 1.0 ) position = 1.0 - ( position - 1.0 );

    double shift = 0.5 * position + 1.7 * ( 1.0 - position );
    double x = shift + position * HEAT_COLOR_PERIOD;

    int r = heat_color( x );
    int g = heat_color( x + M_PI*0.5 );
    int b = heat_color( x + M_PI );

    // dlog.addText( Logger::ACTION_CHAIN,
    //               "(paint) (%.2f %.2f) eval=%f -> %d %d %d",
    //               pos.x, pos.y, value, r, g, b );
    char str[16];
    snprintf( str, 16, "%lf", value );
    dlog.addMessage( Logger::ACTION_CHAIN,
                     pos, str, "#ffffff" );
    dlog.addRect( Logger::ACTION_CHAIN,
                  pos.x - 0.1, pos.y - 0.1, 0.2, 0.2,
                  r, g, b, true );
}

}


/*-------------------------------------------------------------------*/
/*!

 */
ActionChainGraph::ActionChainGraph( const FieldEvaluator::ConstPtr & evaluator,
                                    const ActionGenerator::ConstPtr & generator)
    : M_evaluator( evaluator ),
      M_action_generator( generator ),
      M_chain_count( 0 ),
      M_best_chain_count( 0 ),
      M_best_chain(),
      M_best_evaluation( -std::numeric_limits< double >::max() )
{
    M_max_chain_length = static_cast<unsigned long >(Setting::i()->mChainAction->mChainDeph);
    M_max_evaluate_limit = Setting::i()->mChainAction->mChainNodeNumber;
    #ifdef DEBUG_PAINT_EVALUATED_POINTS
    S_evaluated_points.clear();
    S_evaluated_points.reserve( DEFAULT_MAX_EVALUATE_LIMIT + 1 );
    #endif
}

/*-------------------------------------------------------------------*/
/*!

 */

void ActionChainGraph::add_hold_to_result(const WorldModel & wm){
    const PredictState current_state( wm );

    PredictState::ConstPtr result_state( new PredictState( current_state, 1 ) );
    CooperativeAction::Ptr action( new HoldBall( wm.self().unum(),
                                                 wm.ball().pos(),
                                                 1,
                                                 "defaultHold" ) );
    action->setFinalAction( true );

    M_best_chain.push_back( ActionStatePair( action, result_state ) );

}

void ActionChainGraph::add_nosafe_to_result(){
    for(int i=0; i < M_best_chain_danger.size(); i++){
        M_best_chain.push_back(M_best_chain_danger[i]);
    }
}

void ActionChainGraph::add_onekick_to_result(){
    M_best_chain.clear();
    for(int i=0; i < M_result_one_kick.size(); i++){
        M_best_chain.push_back(M_result_one_kick[i]);
    }
}

void ActionChainGraph::add_onekick_nosafe_to_result(){
    M_best_chain.clear();
    for(int i=0; i < M_result_one_kick_danger.size(); i++){
        M_best_chain.push_back(M_result_one_kick_danger[i]);
    }
}
#include <vector>
void get_color(vector<pair<int, Vector2D>> all_target, vector<pair<int, double>> all_eval){
    if (all_target.size() == 0)
        return;
    double min_eval = all_eval[0].second;
    double max_eval = all_eval[0].second;
    for (auto &res:all_eval){
        double eval = res.second;
        if (eval > max_eval)
            max_eval = eval;
        if (eval < min_eval)
            min_eval = eval;
    }
    for (int i = 0; i < all_target.size(); i++){
        char num[8];
        snprintf(num, 8, "%d", all_target[i].first);
        dlog.addMessage(Logger::ACTION, all_target[i].second + Vector2D(0.1, 0.1), num);
        double eval = all_eval[i].second;
        int color = eval - min_eval;
        if (max_eval - min_eval > 0)
            color /= (max_eval - min_eval);
        dlog.addCircle(Logger::ACTION,all_target[i].second, 0.1,color * 255,0,0,true);
    }
}

bool ActionChainGraph::choose_better_action(bool choose_onkick){
    #ifdef ACTION_CHAIN_DEBUG
        dlog.addText(Logger::ACTION, "");
    #endif
    #ifdef ACTION_CHAIN_DEBUG
    vector<pair<int, Vector2D>> all_target;
    vector<pair<int, double>> all_eval;
    dlog.addText(Logger::ACTION, ">> Choose better action");
    #endif
    ActionStatePair best = M_best_chain[0];
    dlog.addRect(Logger::ACTION,best.action().targetPoint().x - 0.1, best.action().targetPoint().y - 0.1,0.2, 0.2,0,0,255, true);
    int best_i = 0;
    if(best.action().category() == CooperativeAction::Pass){
        int best_tm = std::min(best.action().M_tm_min_dif_cycle,5);
        int best_opp = best.action().M_opp_min_dif_cycle;
        int best_eval = best_tm + 10 * best_opp;
        for(int i=0;i<M_all_pass.size();i++){
            ActionStatePair tmp = M_all_pass[i];
            if(tmp.action().targetPlayerUnum() != M_best_chain[0].action().targetPlayerUnum()){
                continue;
            }
            if(tmp.action().targetPoint().dist(M_best_chain[0].action().targetPoint()) > 8){
                #ifdef ACTION_CHAIN_DEBUG
                dlog.addText(Logger::ACTION,"num %d:%d (%0.2f,%0.2f) > Cancel dist",i,tmp.action().index(),tmp.action().targetPoint().x,tmp.action().targetPoint().y);
                #endif
                continue;
            }
            if(choose_onkick && tmp.action().kickCount() > 1){
                #ifdef ACTION_CHAIN_DEBUG
                dlog.addText(Logger::ACTION,"num %d:%d (%0.2f,%0.2f) > Cancel for kick count",i,tmp.action().index(),tmp.action().targetPoint().x,tmp.action().targetPoint().y);
                #endif
                continue;
            }
            if(tmp.action().danger() > M_best_chain[0].action().danger()){
                #ifdef ACTION_CHAIN_DEBUG
                dlog.addCircle(Logger::ACTION, tmp.action().targetPoint(), 0.1,0,0,0,false);
                dlog.addText(Logger::ACTION,"num %d:%d (%0.2f,%0.2f) > Cancel more danger than best",i,tmp.action().index(),tmp.action().targetPoint().x,tmp.action().targetPoint().y);
                #endif
                continue;
            }
            if(tmp.action().safeWithNoise() != M_best_chain[0].action().safeWithNoise() && !tmp.action().safeWithNoise()){
                #ifdef ACTION_CHAIN_DEBUG
                dlog.addCircle(Logger::ACTION, tmp.action().targetPoint(), 0.1,0,0,0,false);
                dlog.addText(Logger::ACTION,"num %d:%d (%0.2f,%0.2f) > Cancel safe with noise",i,tmp.action().index(),tmp.action().targetPoint().x,tmp.action().targetPoint().y);
                #endif
                continue;
            }
            int tmp_opp = tmp.action().M_opp_min_dif_cycle;
            int tmp_tm = std::min(tmp.action().M_tm_min_dif_cycle,5);
            int eval = tmp_tm + 10 * tmp_opp;

            double thr = 0;
            if(best.action().description() == "strictThrough"
                    && (tmp.action().description() == "strictLead"
                        || tmp.action().description() == "strictDirect" )){
                if(best.action().kickCount() > 1
                        && tmp.action().kickCount() == 1){
                    thr = 10;
                }
            }else if(tmp.action().description() == "strictThrough"
                     && (best.action().description() == "strictLead"
                         || best.action().description() == "strictDirect" )){
                if(best.action().kickCount() == 1
                        && tmp.action().kickCount() > 1){
                    thr = -10;
                }
            }
            #ifdef ACTION_CHAIN_DEBUG
            all_eval.push_back(pair<int, double>(i, eval + thr));
            all_target.push_back(pair<int, Vector2D>(i, tmp.action().targetPoint()));
            dlog.addText(Logger::ACTION,"num %d:%d (%0.2f,%0.2f),tm:%d,opp:%d,eval:%d, ev+thr:%.1f",i,tmp.action().index(),tmp.action().targetPoint().x,tmp.action().targetPoint().y,tmp_tm,tmp_opp,eval, eval+thr);
            #endif
            if(eval + thr > best_eval){
                best_eval = eval;
                best = tmp;
                best_i = i;
            }
        }
        M_best_chain[0] = best;
        #ifdef ACTION_CHAIN_DEBUG
        dlog.addText(Logger::ACTION,"best is: %d index:%d", best_i, M_best_chain[0].action().index());
        dlog.addCircle(Logger::ACTION,best.action().targetPoint(), 0.05,0,0,250,true);
        get_color(all_target, all_eval);
        dlog.addRect(Logger::ACTION,best.action().targetPoint().x - 0.1, best.action().targetPoint().y - 0.1,0.2, 0.2,255,0,0, false);
        #endif
        return true;
    }else if(best.action().category() == CooperativeAction::Dribble){

        if(best.state().ball().pos().dist(Vector2D(52,-10)) < 10){
            double best_eval = best.state().ball().pos().x + std::max(0.0, 40 - best.state().ball().pos().dist(Vector2D(52,0)));
            for(int i=0;i<M_all_dribble.size();i++){
                ActionStatePair tmp = M_all_dribble[i];
                double eval = tmp.state().ball().pos().x + std::max(0.0, 40 - tmp.state().ball().pos().dist(Vector2D(52,0)));
                if(eval > best_eval){
                    best_eval = eval;
                    best = tmp;
                }
            }
        }else{
            int best_opp = best.action().M_opp_min_dif_cycle;
            int out_target = 0;
            if (best.action().targetPoint().absX() > 51.5 || best.action().targetPoint().absY() > 33)
                out_target = 3;
            else if (best.action().targetPoint().absX() > 50.5 || best.action().targetPoint().absY() > 32)
                out_target = 2;
            else if (best.action().targetPoint().absX() > 49.5 || best.action().targetPoint().absY() > 31)
                out_target = 1;
            int best_eval = 4 * best_opp - 4 * out_target;
            for(int i=0;i<M_all_dribble.size();i++){
                ActionStatePair tmp = M_all_dribble[i];
                if(tmp.action().targetPoint().dist(M_best_chain[0].action().targetPoint()) < 3 /*&& tmp.action().danger() <= M_result[0].action().danger() && tmp.action().safeWithNoise() == M_result[0].action().safeWithNoise()*/){
                    int tmp_opp = tmp.action().M_opp_min_dif_cycle;
                    int tmp_out_target = 0;
                    if (tmp.action().targetPoint().absX() > 51.5 || tmp.action().targetPoint().absY() > 33)
                        tmp_out_target = 3;
                    else if (tmp.action().targetPoint().absX() > 50.5 || tmp.action().targetPoint().absY() > 32)
                        tmp_out_target = 2;
                    else if (tmp.action().targetPoint().absX() > 48.5 || tmp.action().targetPoint().absY() > 31)
                        tmp_out_target = 1;
                    int eval = 4 * tmp_opp - 4 * tmp_out_target;
                    if(eval > best_eval){
                        best_eval = eval;
                        best = tmp;
                    }
                }
            }
        }

        M_best_chain[0] = best;
        return true;
    }
    return false;
}
void
ActionChainGraph::calculateResult( const PlayerAgent* agent)
{
    const WorldModel &wm = DataExtractor::i().option.output_worldMode == FULLSTATE ? agent->fullstateWorld() : agent->world();
    debugPrintCurrentState( wm );

    #if (defined DEBUG_PROFILE) || (defined ACTION_CHAIN_LOAD_DEBUG)
    Timer timer;
    #endif

    unsigned long n_evaluated = 0;
    M_chain_count = 0;
    M_best_chain_count = 0;


    //
    // best first
    //
    calculateResultBestFirstSearch( wm, &n_evaluated );
    #ifdef ACTION_CHAIN_DEBUG
    write_chain_log( ">>>>> best chain : ",
                     wm,
                     -1,
                     M_best_chain,
                     M_best_evaluation );
    write_chain_log( ">>>>> best chain one kick: ",
                     wm,
                     -1,
                     M_result_one_kick,
                     M_best_evaluation_one_kick );

    write_chain_log( ">>>>> best chain danger: ",
                     wm,
                     -1,
                     M_best_chain_danger,
                     M_best_evaluation_danger );
    write_chain_log( ">>>>> best chain danger one kick: ",
                     wm,
                     -1,
                     M_result_one_kick_danger,
                     M_best_evaluation_one_kick_danger );
    dlog.addText(Logger::ACTION_CHAIN, "----------------------------------");
    #endif
    bool best_chain_is_safe = false;
    if ( M_best_chain.empty() )
    {
        #ifdef ACTION_CHAIN_DEBUG
        dlog.addText(Logger::ACTION_CHAIN, ">>>> best safe chain is empty");
        #endif
        if( M_best_chain_danger.empty()){
            #ifdef ACTION_CHAIN_DEBUG
            dlog.addText(Logger::ACTION_CHAIN, ">>>> best danger chain is empty and hold > add hold");
            #endif
            add_hold_to_result(wm);
        }else if(wm.interceptTable()->opponentReachCycle() > 3 && wm.gameMode().type() != GameMode::PlayOn && !wm.gameMode().isPenaltyKickMode()){
            #ifdef ACTION_CHAIN_DEBUG
            dlog.addText(Logger::ACTION_CHAIN, ">>>> opp is not near and hold  > add hold");
            #endif
            add_hold_to_result(wm);
        }
        else{
            #ifdef ACTION_CHAIN_DEBUG
            dlog.addText(Logger::ACTION_CHAIN, ">>>> best danger is best > add danger to results > choose better");
            #endif
            add_nosafe_to_result();
            if(!choose_better_action()){
                #ifdef ACTION_CHAIN_DEBUG
                dlog.addText(Logger::ACTION_CHAIN, ">>>> cant select best danger and hold > add hold");
                #endif
                add_hold_to_result(wm);
            }
        }
    }else{
        #ifdef ACTION_CHAIN_DEBUG
        dlog.addText(Logger::ACTION_CHAIN, ">>>> select best from safe > choose better action");
        #endif
        choose_better_action();
        best_chain_is_safe = true;
    }
    #ifdef ACTION_CHAIN_DEBUG
    dlog.addText(Logger::ACTION_CHAIN, "best kick count is : %d", M_best_chain[0].action().kickCount());
    if(wm.interceptTable()->opponentReachCycle() <= (wm.self().goalie() ? 6:3))
        dlog.addText(Logger::ACTION_CHAIN, "opp is near");
    if( M_best_chain[0].action().category() == CooperativeAction::Pass)
        dlog.addText(Logger::ACTION_CHAIN, "best is pass");
    if( std::string(M_best_chain[0].action().description()).compare("strictThrough") == 0)
        dlog.addText(Logger::ACTION_CHAIN, "best is th pass");
    if(M_best_chain[0].action().kickCount() > 1)
        dlog.addText(Logger::ACTION_CHAIN, "kickcount is >1");
    #endif

    ActionStatePair *first_layer = M_best_chain.begin().base();
    DataExtractor::i().update(agent, first_layer);

    if(wm.interceptTable()->opponentReachCycle() <= (wm.self().goalie() ? 6:3)
            && M_best_chain[0].action().category() == CooperativeAction::Pass
            && std::string(M_best_chain[0].action().description()).compare("strictThrough") != 0
            && M_best_chain[0].action().kickCount() > 1)
    {
        #ifdef ACTION_CHAIN_DEBUG
        dlog.addText(Logger::ACTION_CHAIN, "want to one kick pass");
        #endif
        if(best_chain_is_safe){
            if(!M_result_one_kick.empty()){
                #ifdef ACTION_CHAIN_DEBUG
                dlog.addText(Logger::ACTION_CHAIN, "have safe one kick chain and add to best");
                #endif
                add_onekick_to_result();
                choose_better_action(true);
            }else if(!M_result_one_kick_danger.empty()){
                #ifdef ACTION_CHAIN_DEBUG
                dlog.addText(Logger::ACTION_CHAIN, "have not safe one kick chain and add onekick danger to best");
                #endif
                add_onekick_nosafe_to_result();
                choose_better_action(true);
            }
        }else{
            if(!M_result_one_kick_danger.empty()){
                #ifdef ACTION_CHAIN_DEBUG
                dlog.addText(Logger::ACTION_CHAIN, "best is not safe and select danger one kick pass");
                #endif
                add_onekick_nosafe_to_result();
                choose_better_action(true);
            }
        }
    }
    #ifdef ACTION_CHAIN_DEBUG
    write_chain_log( ">>>>> best chain: ",
                     wm,
                     M_best_chain_count,
                     M_best_chain,
                     M_best_evaluation );
    #endif

    #if (defined DEBUG_PROFILE) || (defined ACTION_CHAIN_LOAD_DEBUG)
    const double msec = timer.elapsedReal();
    #ifdef DEBUG_PROFILE
    dlog.addText( Logger::ACTION_CHAIN,
                  __FILE__": PROFILE size=%d elapsed %f [ms]",
                  M_chain_count,
                  msec );
    #endif
    #ifdef ACTION_CHAIN_LOAD_DEBUG
    std::fprintf( stderr,
                  "# recursive search: %2d [%ld] % 10lf msec, n=%4lu, average=% 10lf\n",
                  wm.self().unum(),
                  wm.time().cycle(),
                  msec,
                  n_evaluated,
                  ( n_evaluated == 0 ? 0.0 : msec / n_evaluated ) );
    #endif
    #endif

    #ifdef DEBUG_PAINT_EVALUATED_POINTS
    if ( ! S_evaluated_points.empty() )
    {
        double min_eval = +std::numeric_limits< double >::max();
        double max_eval = -std::numeric_limits< double >::max();
        const std::vector< std::pair< Vector2D, double > >::const_iterator end = S_evaluated_points.end();
        for ( std::vector< std::pair< Vector2D, double > >::const_iterator it = S_evaluated_points.begin();
              it != end;
              ++it )
        {
            if ( min_eval > it->second ) min_eval = it->second;
            if ( max_eval < it->second ) max_eval = it->second;
        }

        if ( std::fabs( min_eval - max_eval ) < 1.0e-5 )
        {
            max_eval = min_eval + 1.0e-5;
        }

        for ( std::vector< std::pair< Vector2D, double > >::const_iterator it = S_evaluated_points.begin();
              it != end;
              ++it )
        {
            debug_paint_evaluate_color( it->first, it->second, min_eval, max_eval );
        }
    }
    #endif
}

/*-------------------------------------------------------------------*/
/*!

 */
void
ActionChainGraph::calculateResultChain( const WorldModel & wm,
                                        unsigned long * n_evaluated )
{
    M_best_chain.clear();
    M_best_evaluation = -std::numeric_limits< double >::max();

    const PredictState current_state( wm );

    std::vector< ActionStatePair > empty_path;
    double evaluation;

    doSearch( wm, current_state, empty_path,
              &M_best_chain, &evaluation, n_evaluated,
              M_max_chain_length,
              M_max_evaluate_limit );

    M_best_evaluation = evaluation;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
ActionChainGraph::doSearch( const WorldModel & wm,
                            const PredictState & state,
                            const std::vector< ActionStatePair > & path,
                            std::vector< ActionStatePair > * result,
                            double * result_evaluation,
                            unsigned long * n_evaluated,
                            unsigned long max_chain_length,
                            long max_evaluate_limit )
{
    if ( path.size() > max_chain_length )
    {
        return false;
    }


    //
    // check evaluation limit
    //
    if ( max_evaluate_limit != -1
         && *n_evaluated >= static_cast< unsigned int >( max_evaluate_limit ) )
    {
        dlog.addText( Logger::ACTION_CHAIN,
                      "cut by max evaluate limit %d", *n_evaluated );
#if 0
#if defined( ACTION_CHAIN_DEBUG ) || defined( ACTION_CHAIN_LOAD_DEBUG )
        std::cerr << "Max evaluate limit " << max_evaluate_limit
                  << " exceeded" << std::endl;
#endif
#endif
        return false;
    }


    //
    // check current state
    //
    std::vector< ActionStatePair > best_path = path;
    double max_ev = (*M_evaluator)( state,wm, path );
#ifdef DO_MONTECARLO_SEARCH
    double eval_sum = 0;
    long n_eval = 0;
#endif

    ++M_chain_count;
#ifdef ACTION_CHAIN_DEBUG
    write_chain_log( wm, M_chain_count, path, max_ev );
#endif
#ifdef DEBUG_PAINT_EVALUATED_POINTS
    S_evaluated_points.push_back( std::pair< Vector2D, double >( state.ball().pos(), max_ev ) );
#endif
    ++ (*n_evaluated);


    //
    // generate candidates
    //
    std::vector< ActionStatePair > candidates;
    if ( path.empty()
         || ! ( *( path.rbegin() ) ).action().isFinalAction() )
    {
        M_action_generator->generate( &candidates, state, wm, path );
    }


    //
    // test each candidate
    //
    for ( std::vector< ActionStatePair >::const_iterator it = candidates.begin();
          it != candidates.end();
          ++it )
    {
        std::vector< ActionStatePair > new_path = path;
        std::vector< ActionStatePair > candidate_result;

        new_path.push_back( *it );

        double ev;
        const bool exist = doSearch( wm,
                                     (*it).state(),
                                     new_path,
                                     &candidate_result,
                                     &ev,
                                     n_evaluated,
                                     max_chain_length,
                                     max_evaluate_limit );
        if ( ! exist )
        {
            continue;
        }
#ifdef ACTION_CHAIN_DEBUG
        write_chain_log( wm, M_chain_count, candidate_result, ev );
#endif
#ifdef DEBUG_PAINT_EVALUATED_POINTS
        S_evaluated_points.push_back( std::pair< Vector2D, double >( it->state().ball().pos(), ev ) );
#endif
        if ( ev > max_ev )
        {
            max_ev = ev;
            best_path = candidate_result;
        }

#ifdef DO_MONTECARLO_SEARCH
        eval_sum += ev;
        ++n_eval;
#endif
    }

    *result = best_path;
#ifdef DO_MONTECARLO_SEARCH
    if ( n_eval == 0 )
    {
        *result_evaluation = max_ev;
    }
    else
    {
        *result_evaluation = eval_sum / n_eval;
    }
#else
    *result_evaluation = max_ev;
#endif

    return true;
}

class ChainComparator
{
public:
    bool operator()( const std::pair< std::vector< ActionStatePair >,
                     double > & a,
                     const std::pair< std::vector< ActionStatePair >,
                     double > & b )
    {
        return ( a.second < b.second );
    }
};

/*-------------------------------------------------------------------*/
/*!

 */
double opp_min_dist(const WorldModel & wm, Vector2D point){
    double min = 100;
    for(int i = 1; i<=11; i++){
        const AbstractPlayerObject * opp = wm.theirPlayer(i);
        if(opp!=NULL && opp->unum()>0 && !opp->goalie()){
            double dist = opp->pos().dist(point);
            if(dist < min)
                min = dist;
        }
    }
    return min;
}
#include "field_analyzer.h"
double calc_danger_eval_for_target(const WorldModel & wm,Vector2D target,int spend){
    //	return 0;
    int our_score = ( wm.ourSide() == LEFT
                      ? wm.gameMode().scoreLeft()
                      : wm.gameMode().scoreRight() );
    int opp_score = ( wm.ourSide() == LEFT
                      ? wm.gameMode().scoreRight()
                      : wm.gameMode().scoreLeft() );
    double aray[16] = { 40, 35, 25, 15, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0 };
    double def[16] = { 50, 40, 35, 20, 18, 16, 10, 5, 4, 3, 0, 0, 0, 0, 0, 0 };
    double cen[16] = { 50, 40, 35, 15, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0 };
    double cen_gld[16] = { 50, 40, 35, 30, 15, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0 };
    double forw_out_[16] = { 10, 8, 5, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    double forw_pen_[16] = { 20, 10, 8, 5, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    double forw_pen_helios[16] = { 10, 7, 5, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    double forw_pen_oxsy[16] = { 30, 25, 20, 15, 12, 10, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    for(int i = 0;i<16;i++){
        if(wm.ball().pos().x < -20){
            aray[i] = def[i];
        }else if(wm.ball().pos().x < 20){
            if(FieldAnalyzer::isGLD(wm))
                aray[i] = cen_gld[i];
            else
                aray[i] = cen[i];
        }else{
            if(target.x > 35 && target.absY() < 15)
                aray[i] = forw_out_[i];
            else{
                if (FieldAnalyzer::isOxsy(wm) ||
                    FieldAnalyzer::isAlice(wm) ||
                    FieldAnalyzer::isYushan(wm) ||
                    FieldAnalyzer::isJyo(wm))
                {
                    aray[i] = forw_pen_oxsy[i];
                }
                else if (FieldAnalyzer::isHelius(wm)){
                    aray[i] = forw_pen_helios[i];
                }
                else{
                    aray[i] = forw_pen_oxsy[i];
//                    aray[i] = forw_pen_[i];
                }
            }
        }
    }
    double dist_opp_target = opp_min_dist(wm,target) /*- spend*/;
    if(dist_opp_target>10)
        dist_opp_target = 10;
    double d = aray[(int)dist_opp_target];;
    if(target.x > 45 && target.absY() < 10)
        d/=5.0;
    return d;
}
double calc_danger_eval_for_bhv(const WorldModel & wm,ActionStatePair bhv){
    Vector2D bhv_target = bhv.M_action->targetPoint();
    return calc_danger_eval_for_target(wm,bhv_target,bhv.M_action->targetPoint().dist(wm.ourPlayer(bhv.M_action->targetPlayerUnum())->pos()));

}
double calc_danger_eval_for_chain(const WorldModel & wm,std::vector< ActionStatePair > series){
    //	return 0;
    int chain_size = series.size();
    double max_danger_eval = 0;
    for(int i = 0;i <chain_size;i++){
        ActionStatePair bhv = series[i];
        double danger_eval = calc_danger_eval_for_bhv(wm,bhv);
        if(danger_eval > max_danger_eval)
            max_danger_eval = danger_eval;
    }
    if(wm.opponentTeamName().compare("ae2")==0||wm.opponentTeamName().compare("FURY")==0||wm.opponentTeamName().compare("PersianGulf2017"))
        max_danger_eval/=3.0;
    return max_danger_eval;
}
bool ActionChainGraph::is_anti_offense(const WorldModel & wm){
    std::vector<double> oppx;
    for(int i=1; i <= 11; i++){
        const AbstractPlayerObject * opp = wm.theirPlayer(i);
        if(opp == NULL || opp->unum() < 1)
            continue;
        oppx.push_back(opp->pos().x);
    }
    int size = oppx.size();
    for(int i=0; i<size;i++){
        for(int j=i;j<size; j++){
            if(oppx[i] > oppx[j]){
                double tmp = oppx[i];
                oppx[i] = oppx[j];
                oppx[j] = tmp;
            }
        }
    }
    int number_in_off = std::min(6,size);
    double sum_x = 0;
    for(int i = 0;i<number_in_off;i++){
        sum_x += oppx[i];
    }
    double avg_x = sum_x / (double)number_in_off;
    Vector2D ball_pos = wm.ball().pos();
    if(avg_x < ball_pos.x - 25)
        return true;
    return false;
}
void
ActionChainGraph::calculateResultBestFirstSearch( const WorldModel & wm,
                                                  unsigned long * n_evaluated )

{
    //
    // initialize
    //
    M_best_chain.clear();
    M_best_evaluation = -std::numeric_limits< double >::max();
    *(n_evaluated) = 0;


    bool use_danger_eval = true;
    if(wm.self().isKickable())
        if(is_anti_offense(wm))
            use_danger_eval = false;

    //
    // create priority queue
    //
    std::priority_queue< std::pair< std::vector< ActionStatePair >, double >,
            std::vector< std::pair< std::vector< ActionStatePair >, double > >,
            ChainComparator > queue;


    //
    // check current state
    //
    const PredictState current_state( wm );
    const std::vector< ActionStatePair > empty_path;

    double current_evaluation = (*M_evaluator)(current_state,wm, empty_path);
    if(Strategy::i().my_team_tactic == Strategy::TeamTactic::AllDef){
        current_evaluation = 100;
    }
    if(wm.countOpponentsIn(Circle2D(wm.ball().pos(),2.0),3,true) > 0)
        current_evaluation -= 20;

    double danger_eval = calc_danger_eval_for_target(wm,current_state.ball().pos(),0);
    if(use_danger_eval)
        current_evaluation -= danger_eval;

    if(wm.gameMode().type()!=GameMode::PlayOn)
        current_evaluation -= 100;
    ++M_chain_count;
    ++(*n_evaluated);
    #ifdef ACTION_CHAIN_DEBUG
    write_chain_log( wm, M_chain_count, empty_path, current_evaluation );
    #endif
    #ifdef DEBUG_PAINT_EVALUATED_POINTS
    S_evaluated_points.push_back( std::pair< Vector2D, double >
                                  ( current_state.ball().pos(), current_evaluation ) );
    #endif
    M_best_chain = empty_path;
    M_best_chain_danger = empty_path;

    M_best_evaluation = current_evaluation;
    M_best_evaluation_danger = -1000;
    M_best_evaluation_one_kick = -1000;
    M_best_evaluation_one_kick_danger = -1000;

    queue.push( std::pair< std::vector< ActionStatePair >, double >
                ( empty_path, current_evaluation ) );

    //
    // main loop
    //
    if(wm.interceptTable()->fastestTeammate()!=NULL)
        if(wm.interceptTable()->teammateReachCycle() < wm.interceptTable()->selfReachCycle()
                || wm.interceptTable()->selfReachCycle() > 2)
            return;
    int opp_min = wm.interceptTable()->opponentReachCycle();
    for(;;)
    {
        //
        // pick up most valuable action chain
        //
        if ( queue.empty() )
        {
            break;
        }

        std::vector< ActionStatePair > series = queue.top().first;
        queue.pop();

        //
        // get state candidates
        //
        const PredictState * state;
        if ( series.empty() )
        {
            state = &current_state;
        }
        else
        {
            state = &((*(series.rbegin())).state());
        }

        //
        // generate action candidates
        //
        std::vector< ActionStatePair > candidates;
        if ( series.size() < M_max_chain_length
             && ( series.empty()
                  || ! ( *( series.rbegin() ) ).action().isFinalAction() ) )
        {
            //            if(!series.empty() && series.front().action().category() == CooperativeAction::Dribble){}
            //            else
            M_action_generator->generate( &candidates, *state, wm, series );
            #ifdef ACTION_CHAIN_DEBUG
            dlog.addText( Logger::ACTION_CHAIN,
                          ">>>> generate (%s[%d]) candidate_size=%d <<<<<",
                          ( series.empty() ? "empty" : series.rbegin()->action().description() ),
                          ( series.empty() ? -1 : series.rbegin()->action().index() ),
                          candidates.size() );
            #endif
        }


        //
        // evaluate each candidate and push to priority queue
        //
        for ( std::vector< ActionStatePair >::const_iterator it = candidates.begin();
              it != candidates.end();
              ++ it )
        {
            ++M_chain_count;
            std::vector< ActionStatePair > candidate_series = series;
            candidate_series.push_back( *it );
            double ev = (*M_evaluator)( (*it).state(),wm, candidate_series );
            #ifdef ACTION_CHAIN_DEBUG
            write_chain_log( wm, M_chain_count, candidate_series, ev );
            dlog.addText(Logger::ACTION_CHAIN, "__  ev:%.1f", ev);
            #endif

            bool opp_can_intercept_before_kick = false;
            bool opp_can_intercept_before_kick_maybe = false;
            if(candidate_series.size() >= 1){
                int kick_step = candidate_series[0].action().kickCount();
                if(opp_min < kick_step)
                    opp_can_intercept_before_kick = true;
                else if( opp_min <= kick_step)
                    opp_can_intercept_before_kick_maybe = true;
            }

            if(wm.self().unum() == 8 || wm.self().unum() == 4 || wm.self().unum() == 10){
                if(wm.ball().pos().x > 0 && wm.ball().pos().y > 15){
                    if(candidate_series.front().action().targetPoint().y < 15){
                        ev += 20;
                        #ifdef ACTION_CHAIN_DEBUG
                        dlog.addText(Logger::ACTION_CHAIN, "__  ev:%.1f ball to up", ev);
                        #endif
                    }

                }
            }
            if(wm.opponentsFromBall().size() > 0){
                if(wm.opponentsFromBall().front()->distFromBall() < 2.5){
                    if(candidate_series.front().action().kickCount() > 1){
                        if(candidate_series.front().action().description() != "strictThrough"){
                            ev -= 15;
                            #ifdef ACTION_CHAIN_DEBUG
                            dlog.addText(Logger::ACTION_CHAIN, "__  ev:%.1f kick count > opp cycle", ev);
                            #endif
                        }
                    }
                }
            }

            if(candidate_series.size() == 1 && candidate_series[0].action().category() == CooperativeAction::Pass){
                M_all_pass.push_back(candidate_series[0]);
            }
            else if(candidate_series.size() == 1 && candidate_series[0].action().category() == CooperativeAction::Dribble){
                M_all_dribble.push_back(candidate_series[0]);
            }
            if(!candidate_series[0].action().safeWithNoise() || candidate_series[0].action().danger() > 0){
                double danger_eval = calc_danger_eval_for_chain(wm,candidate_series);
                if(use_danger_eval)
                    ev -= danger_eval;

                double spend = candidate_series[candidate_series.size() - 1].state().spendTime();
                spend /= 10;
                ev -= spend;
                #ifdef ACTION_CHAIN_DEBUG
                if(!candidate_series[0].action().safeWithNoise())
                    dlog.addText(Logger::ACTION_CHAIN,"__  danger because safewith noise");
                else
                    dlog.addText(Logger::ACTION_CHAIN,"__  danger because danger=%d",candidate_series[0].action().danger());
                dlog.addText(Logger::ACTION_CHAIN,"__  danger eval:%.1f spend:%.1f ev:%.1f", danger_eval, spend, ev);
                #endif
                if ( ev > M_best_evaluation_danger )
                {
                    #ifdef ACTION_CHAIN_DEBUG
                    dlog.addText(Logger::ACTION_CHAIN, "__  is best danger");
                    #endif
                    M_best_chain_count = M_chain_count;
                    M_best_evaluation_danger = ev;
                    M_best_chain_danger = candidate_series;
                }
                if(candidate_series[0].action().kickCount() == 1 && candidate_series[0].action().category() == CooperativeAction::Pass){
                    if ( ev > M_best_evaluation_one_kick_danger )
                    {
                        #ifdef ACTION_CHAIN_DEBUG
                        dlog.addText(Logger::ACTION_CHAIN, "__  is best danger one kick pass");
                        #endif
                        M_best_chain_count = M_chain_count;
                        M_best_evaluation_one_kick_danger = ev;
                        M_result_one_kick_danger = candidate_series;
                    }
                }
            }else{
                double danger_eval = calc_danger_eval_for_chain(wm,candidate_series);
                if(use_danger_eval)
                    ev -= danger_eval;
                double spend = candidate_series[candidate_series.size() - 1].state().spendTime();
                spend /= 10;
                ev -= spend;
                double z_kick = 1.0;
                if(opp_can_intercept_before_kick)
                    z_kick *= 0.7;
                if(opp_can_intercept_before_kick_maybe)
                    z_kick *= 0.85;
                ev *= z_kick;
                #ifdef ACTION_CHAIN_DEBUG
                dlog.addText(Logger::ACTION_CHAIN,"__  danger eval:%.1f spend:%.1f zkick:%.1f ev:%.1f", danger_eval, spend, z_kick, ev);
                #endif
                if ( ev > M_best_evaluation )
                {
                    #ifdef ACTION_CHAIN_DEBUG
                    dlog.addText(Logger::ACTION_CHAIN, "__  is best");
                    #endif
                    M_best_chain_count = M_chain_count;
                    M_best_evaluation = ev;
                    M_best_chain = candidate_series;
                }
                if(candidate_series[0].action().kickCount() == 1 && candidate_series[0].action().category() == CooperativeAction::Pass){
                    if ( ev > M_best_evaluation_one_kick )
                    {
                        #ifdef ACTION_CHAIN_DEBUG
                        dlog.addText(Logger::ACTION_CHAIN, "__  is best best one kick pass");
                        #endif
                        M_best_chain_count = M_chain_count;
                        M_best_evaluation_one_kick = ev;
                        M_result_one_kick = candidate_series;
                    }
                }
            }

            ++(*n_evaluated);

            #ifdef DEBUG_PAINT_EVALUATED_POINTS
            S_evaluated_points.push_back( std::pair< Vector2D, double >
                                          ( it->state().ball().pos(), ev ) );
            #endif


            if ( M_max_evaluate_limit != -1
                 && *n_evaluated >= static_cast< unsigned int >( M_max_evaluate_limit ) )
            {
                #ifdef ACTION_CHAIN_DEBUG
                dlog.addText( Logger::ACTION_CHAIN,
                              "***** over max evaluation count *****" );
                #endif
                return;
            }
            if ( !PlayerAgent::canProcessMore() )
            {
                #ifdef ACTION_CHAIN_DEBUG
                dlog.addText( Logger::ACTION_CHAIN,
                              "***** over max evaluation time *****" );
                #endif
                return;
            }
            queue.push( std::pair< std::vector< ActionStatePair >, double >
                        ( candidate_series, ev ) );
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
ActionChainGraph::debugPrintCurrentState( const WorldModel & wm )
{

}

/*-------------------------------------------------------------------*/
/*!

 */
void
ActionChainGraph::debug_send_chain( PlayerAgent * agent,
                                    const std::vector< ActionStatePair > & path )
{
    const double DIRECT_PASS_DIST = 3.0;

    const PredictState current_state( DataExtractor::i().option.output_worldMode == FULLSTATE ? agent->fullstateWorld() : agent->world() );

    for ( size_t i = 0; i < path.size(); ++i )
    {
        std::string action_string = "?";
        std::string target_string = "";

        const CooperativeAction & action = path[i].action();
        const PredictState * s0;
        const PredictState * s1;

        if ( i == 0 )
        {
            s0 = &current_state;
            s1 = &(path[0].state());
        }
        else
        {
            s0 = &(path[i - 1].state());
            s1 = &(path[i].state());
        }


        switch( action.category() )
        {
        case CooperativeAction::Hold:
        {
            action_string = "hold";
            break;
        }

        case CooperativeAction::Dribble:
        {
            action_string = "dribble";

            agent->debugClient().addLine( s0->ball().pos(), s1->ball().pos() );
            break;
        }


        case CooperativeAction::Pass:
        {
            action_string = "pass";

            std::stringstream buf;
            buf << action.targetPlayerUnum();
            target_string = buf.str();

            if ( i == 0
                 && s0->ballHolderUnum() == agent->world().self().unum() )
            {
                agent->debugClient().setTarget( action.targetPlayerUnum() );

                if ( s0->ourPlayer( action.targetPlayerUnum() )->pos().dist( s1->ball().pos() )
                     > DIRECT_PASS_DIST )
                {
                    agent->debugClient().addLine( s0->ball().pos(), s1->ball().pos() );
                }
            }
            else
            {
                agent->debugClient().addLine( s0->ball().pos(), s1->ball().pos() );
            }

            break;
        }

        case CooperativeAction::Shoot:
        {
            action_string = "shoot";

            agent->debugClient().addLine( s0->ball().pos(),
                                          Vector2D( ServerParam::i().pitchHalfLength(),
                                                    0.0 ) );

            break;
        }

        case CooperativeAction::Move:
        {
            action_string = "move";
            break;
        }
        default:
        {
            action_string = "?";

            break;
        }
        }

        if ( action.description() )
        {
            action_string += '|';
            action_string += action.description();
            if ( ! target_string.empty() )
            {
                action_string += ':';
                action_string += target_string;
            }
        }

        agent->debugClient().addMessage( "%s", action_string.c_str() );

        dlog.addText( Logger::ACTION_CHAIN,
                      "chain %d %s, time = %lu",
                      i, action_string.c_str(), s1->spendTime() );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
ActionChainGraph::write_chain_log( const WorldModel & world,
                                   const int count,
                                   const std::vector< ActionStatePair > & path,
                                   const double & eval )
{
    write_chain_log( "", world, count, path, eval );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
ActionChainGraph::write_chain_log( const std::string & pre_log_message,
                                   const WorldModel & world,
                                   const int count,
                                   const std::vector< ActionStatePair > & path,
                                   const double & eval )
{
    if ( ! pre_log_message.empty() )
    {
        dlog.addText( Logger::ACTION_CHAIN, pre_log_message.c_str() );
    }

    if (count >= 0)
        dlog.addText( Logger::ACTION_CHAIN,
                      "%d: ",
                      count);

    const PredictState current_state( world );

    for ( size_t i = 0; i < path.size(); ++i )
    {
        const CooperativeAction & a = path[i].action();
        const PredictState * s0;
        const PredictState * s1;

        if ( i == 0 )
        {
            s0 = &current_state;
            s1 = &(path[0].state());
        }
        else
        {
            s0 = &(path[i - 1].state());
            s1 = &(path[i].state());
        }


        switch ( a.category() ) {
        case CooperativeAction::Hold:
        {
            dlog.addText( Logger::ACTION_CHAIN,
                          "__ %d: hold (%s) t=%d",
                          i, a.description(), s1->spendTime() );
            break;
        }

        case CooperativeAction::Dribble:
        {
            dlog.addText( Logger::ACTION_CHAIN,
                          "__ %d: dribble (%s[%d]) t=%d unum=%d target=(%.2f %.2f)",
                          i, a.description(), a.index(), s1->spendTime(),
                          s0->ballHolderUnum(),
                          a.targetPoint().x, a.targetPoint().y );
            break;
        }

        case CooperativeAction::Pass:
        {
            dlog.addText( Logger::ACTION_CHAIN,
                          "__ %d: pass (%s[%d]) t=%d from[%d](%.2f %.2f)-to[%d](%.2f %.2f)",
                          i, a.description(), a.index(), s1->spendTime(),
                          s0->ballHolderUnum(),
                          s0->ball().pos().x, s0->ball().pos().y,
                          s1->ballHolderUnum(),
                          a.targetPoint().x, a.targetPoint().y );
            break;
        }

        case CooperativeAction::Shoot:
        {
            dlog.addText( Logger::ACTION_CHAIN,
                          "__ %d: shoot (%s) t=%d unum=%d",
                          i, a.description(), s1->spendTime(),
                          s0->ballHolderUnum() );

            break;
        }

        case CooperativeAction::Move:
        {
            dlog.addText( Logger::ACTION_CHAIN,
                          "__ %d: move (%s)",
                          i, a.description(), s1->spendTime() );
            break;
        }

        default:
        {
            dlog.addText( Logger::ACTION_CHAIN,
                          "__ %d: ???? (%s)",
                          i, a.description(), s1->spendTime() );
            break;
        }
        }
    }
}
