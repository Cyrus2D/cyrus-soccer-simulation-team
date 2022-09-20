
#ifndef BHV_VORONOI
#define BHV_VORONOI

#include <rcsc/game_time.h>
#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_intention.h>
#include <vector>
#include <rcsc/player/world_model.h>
#include <rcsc/player/player_agent.h>
#include "strategy.h"
#include <rcsc/common/logger.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/common/server_param.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/abstract_player_object.h>
#include <rcsc/action/body_turn_to_angle.h>
#include <rcsc/action/neck_turn_to_ball.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include "chain_action/field_analyzer.h"
#include "bhv_basic_move.h"
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/body_turn_to_point.h>
#include <rcsc/common/audio_memory.h>
#include <rcsc/common/say_message_parser.h>
#include "../neck/neck_decision.h"
using namespace std;
using namespace rcsc;

#include <rcsc/player/soccer_intention.h>

enum unmark_type{
    ut_directpass,
    ut_tmpass,
    ut_freepos,
    ut_homepos
};

bool go_to_target(PlayerAgent * agent, Vector2D target, unmark_type type){
    const WorldModel & wm = agent->world();
    int mate_min = wm.interceptTable()->teammateReachCycle();
    int opp_min = wm.interceptTable()->opponentReachCycle();
    Vector2D self_pos = wm.self().pos();
    Vector2D ball_pos = wm.ball().inertiaPoint(mate_min);
    double dist2target = self_pos.dist(target);

    double dash_power = Strategy::i().get_normal_dash_power(wm);

    if(type == ut_directpass){
        if (dist2target < mate_min){
            dash_power = 100;
        }
        if ( opp_min < mate_min + 3){
            dash_power = 100;
        }
    }
    if(target.dist(ball_pos) < 20){
        dash_power += 10;
    }
    if(ball_pos.dist(Vector2D(52,0)) < 20)
        dash_power = 100;
    if(target.dist(Vector2D(52,0)) < 20)
        dash_power = 100;
    if(wm.getDistOpponentNearestToSelf(5,true) < 3){
        dash_power = 100;
    }
    double angle_thr = 20;
    if(mate_min <= 2 && !FieldAnalyzer::isHelius(wm))
        angle_thr = 90;
    if(!Body_GoToPoint(target, 0.5, dash_power,1.2,3,false,20).execute(agent)){
        if(self_pos.dist(Vector2D(52,0)) < 15){
            Body_TurnToAngle( (ball_pos - self_pos).th()).execute(agent);
        }else{
            if(angle_thr > 89){
                agent->doTurn(0);
            }else{
                AngleDeg A = (ball_pos - target).th() + AngleDeg(90);
                AngleDeg B = (ball_pos - target).th() + AngleDeg(-90);
                AngleDeg goal_Angle = (Vector2D(45,0) - target).th();
                if( (A-goal_Angle).abs() < (B-goal_Angle).abs()){
                    Body_TurnToAngle(A).execute(agent);
                }else{
                    Body_TurnToAngle(B).execute(agent);
                }
            }

        }
    }
    agent->debugClient().setTarget(target);
    NeckDecisionWithBall().setNeck(agent, NeckDecisionType::unmarking);
    return true;

}

class IntentionUnmark
        : public rcsc::SoccerIntention {
private:
    const rcsc::Vector2D M_target_point; // global coordinate
    int M_step;
    int M_ex_step;
    int M_last_ex_time;
    int M_start_passer;
    Vector2D M_start_ball_pos;
    unmark_type M_type;

public:
    IntentionUnmark( const rcsc::Vector2D & target_point,
                     const int max_step,
                     const int last_ex_time,
                     const int start_passer,
                     const Vector2D start_ball_pos,
                     const unmark_type type):
        M_target_point(target_point){
        M_step = max_step;
        M_last_ex_time = last_ex_time;
        M_start_ball_pos = start_ball_pos;
        M_start_passer = start_passer;
        M_type = type;
    }

    bool finished(  rcsc::PlayerAgent * agent ){
        const WorldModel & wm = agent->world();
        if(M_last_ex_time != wm.time().cycle() - 1)
            return true;
        if(wm.existKickableOpponent())
            return true;
        if(M_target_point.x > wm.offsideLineX() - 0.3)
            return true;
        if(wm.interceptTable()->selfReachCycle() <= wm.interceptTable()->teammateReachCycle())
            return true;
        if(wm.self().pos().x > wm.offsideLineX() - 0.3)
            return true;
        if(Strategy::i().isDefSit(wm,wm.self().unum()))
            return true;
        if(wm.gameMode().type() != GameMode::PlayOn)
            return true;
        if(M_step == 0)
            return true;
        if ( wm.audioMemory().passTime() == wm.time()
             && !wm.audioMemory().pass().empty()
             && wm.audioMemory().pass().front().receiver_ == wm.self().unum() )
        {
            return true;
        }
        Vector2D ballpos = wm.ball().inertiaPoint(wm.interceptTable()->teammateReachCycle());
        int passerunum = 0;
        if(wm.interceptTable()->fastestTeammate() != NULL)
            passerunum = wm.interceptTable()->fastestTeammate()->unum();
        if( passerunum != M_start_passer && ballpos.dist(M_start_ball_pos) > 6)
            return true;
        if(ballpos.dist(M_start_ball_pos) > 10)
            return true;
        if(M_target_point.dist(wm.ball().inertiaPoint(wm.interceptTable()->teammateReachCycle())) < 5)
            return true;
        return false;
    }

    bool execute( rcsc::PlayerAgent * agent ){
        M_step --;
        agent->debugClient().addMessage("voronoi inten");

        go_to_target(agent, M_target_point, M_type);
        //        if(Body_GoToPoint(M_target_point,0.5,100).execute(agent)){
        //            agent->debugClient().setTarget(M_target_point);
        //            Bhv_BasicMove().offense_set_neck_action(agent);
        //            M_last_ex_time = agent->world().time().cycle();
        return true;
        //        }
        return false;
    }
};
bool compair_vec_x(const Vector2D & x, const Vector2D & y){
    return x.x < y.x;
}



class bhv_unmark_voronoi{

public:
    bool execute(PlayerAgent *agent) {
        Timer timer1;
        const WorldModel &wm = agent->world();
        if(!can_unmark(wm))
            return false;

        dlog.addText(Logger::BLOCK, "start bhv voronoi unmark");
        vector<Vector2D> voronoiPoints = voronoi_points(agent);
        std::sort(voronoiPoints.begin(), voronoiPoints.end(), compair_vec_x);
        double t1 = timer1.elapsedReal();
        Timer timer2;
        auto passer_pos_unum = where_is_passer(wm);
        vector<unmark_type> type_of_unmark = best_type_of_unmark(wm);
        for(size_t i = 0; i < type_of_unmark.size(); i++){
            if(type_of_unmark[i] == ut_directpass){
                dlog.addText(Logger::BLOCK, " unmark type :direct");
            }
            else if(type_of_unmark[i] == ut_tmpass){
                dlog.addText(Logger::BLOCK, " unmark type :tmpass");
            }
            else if(type_of_unmark[i] == ut_homepos){
                dlog.addText(Logger::BLOCK, " unmark type :homepos");
            }
            else if(type_of_unmark[i] == ut_freepos){
                dlog.addText(Logger::BLOCK, " unmark type :freepos");
            }
        }
        double t2 = timer2.elapsedReal();
        Timer timer3;
        Vector2D best = Vector2D::INVALIDATED;
        double ev = -100;
        vector<double> evals;
        int num = 0;
        for (auto point : voronoiPoints) {
            double tmp = evaluate_point(agent, point, passer_pos_unum,num,type_of_unmark);
            num+=1;
            evals.push_back(tmp);
            if (ev < tmp) {
                ev = tmp;
                best = point;
            }
        }
        double t3 = timer3.elapsedReal();
        Timer timer4;
        num = 0;
        for(size_t i = 0; i < evals.size(); i++ ){
            if(ev > 0){
                dlog.addCircle(Logger::BLOCK, voronoiPoints[i], 0.1, evals[i] / ev * 255,0,0,true);
            }else{
                dlog.addCircle(Logger::BLOCK, voronoiPoints[i], 0.1, 0,0,0,true);
            }

            char str[16];
            snprintf( str, 16, "%d", num );
            dlog.addMessage(Logger::BLOCK,voronoiPoints[i].x,voronoiPoints[i].y,str,0,0,200);
            num+=1;
        }
        double t4 = timer4.elapsedReal();
        dlog.addText(Logger::BLOCK,"timer %f %f %f %f",t1,t2,t3,t4);
        if (best.isValid()) {
            dlog.addCircle(Logger::BLOCK, best, 0.5, 0, 0, 0, true);
            int now_passer = 0;
            if(wm.interceptTable()->fastestTeammate() != NULL)
                now_passer = wm.interceptTable()->fastestTeammate()->unum();
            agent->setIntention(new IntentionUnmark(best,5,wm.time().cycle(),now_passer,wm.ball().inertiaPoint(wm.interceptTable()->teammateReachCycle()),type_of_unmark[0]));
            agent->debugClient().addMessage("voronoi");
            go_to_target(agent,best,type_of_unmark[0]);
            return true;
        } else
            dlog.addText(Logger::BLOCK, "not valid point");
        return false;
    }
    bool can_unmark(const WorldModel & wm) {
        Strategy::PostLine pl_line = Strategy::i().self_Line();
        int stamina = wm.self().stamina();
        static const Rect2D penalty_area=Rect2D(Vector2D(38,-17),Vector2D(53,17));
        if (wm.ball().inertiaPoint(wm.interceptTable()->teammateReachCycle()).dist(
                    wm.self().pos()) > 30)
            return false;
        if(wm.ball().inertiaPoint(wm.interceptTable()->teammateReachCycle()).x < 30)
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


    vector<Vector2D> voronoi_points(rcsc::PlayerAgent *agent) {

        std::vector<Vector2D> v_points;
        if(Strategy::i().self_Line() != Strategy::PostLine::forward && Strategy::i().self_Line() != Strategy::Strategy::PostLine::half)
            return v_points;

        const WorldModel &wm = agent->world();
        const int mate_min = wm.interceptTable()->teammateReachCycle();
        Vector2D ballInertiaPos = wm.ball().inertiaPoint(mate_min);
        Vector2D self_home_pos = Strategy::i().getPosition(wm.self().unum());
        double offside_line_x = std::max(ballInertiaPos.x,wm.offsideLineX());
        vector<Vector2D> tm_home_poses;
        double min_tm_home_x = 100;
        for(int u = 1; u <= 11; u++){
            if(Strategy::i().tm_Line(u) != Strategy::PostLine::forward
                    && Strategy::i().tm_Line(u) != Strategy::PostLine::half)
                continue;
            if(u == wm.self().unum())
                continue;
            tm_home_poses.push_back(Strategy::i().getPosition(u));
        }
        for(size_t i=0; i < tm_home_poses.size(); i++){
            if(tm_home_poses[i].x < min_tm_home_x)
                min_tm_home_x = tm_home_poses[i].x;
        }
        VoronoiDiagram voronoi;
        voronoi.clear();
        double min_x = min_tm_home_x - 10;
        double max_x = offside_line_x - 0.3;

        double min_y = std::max(ballInertiaPos.y - 38.0, -33.0);
        double max_y = std::min(ballInertiaPos.y + 38.0, +33.0);

        const rcsc::Rect2D PITCH_RECT(rcsc::Vector2D(min_x,
                                                     min_y),
                                      rcsc::Size2D(max_x - min_x,
                                                   max_y - min_y));
        dlog.addRect(Logger::BLOCK,PITCH_RECT,255,0,0);


        vector< pair<Vector2D,int> > opps_pos;
        const PlayerPtrCont::const_iterator o_end = wm.opponentsFromSelf().end();
        for ( PlayerPtrCont::const_iterator o = wm.opponentsFromSelf().begin();
              o != o_end;
              ++o )
        {
            if( !(*o)->pos().isValid() )
                continue;
            opps_pos.push_back(make_pair((*o)->pos(),0));
        }
        opps_pos.push_back(make_pair(Vector2D((min_x + max_x) / 2.0, min_y),0));
        opps_pos.push_back(make_pair(Vector2D((min_x + max_x) / 2.0, max_y),0));

        for(size_t i = 0; i<opps_pos.size(); i++){
            if(opps_pos[i].second == -1 || opps_pos[i].second == 1)
                continue;
            for(size_t j = i + 1; j<opps_pos.size(); j++){
                if(opps_pos[j].second == -1 || opps_pos[j].second == 1)
                    continue;
                if( opps_pos[i].first.dist(opps_pos[j].first) < 3.0 ){
                    opps_pos.push_back( make_pair( (opps_pos[i].first + opps_pos[j].first)/2.0, -1 ));
                    opps_pos[i].second = 1;
                    opps_pos[j].second = 1;
                }
            }
        }

        for(size_t i=0;i<opps_pos.size();i++){
            if(opps_pos[i].second != 1){
                voronoi.addPoint(opps_pos[i].first);
                dlog.addCircle(Logger::BLOCK,opps_pos[i].first,0.3,200,0,0,true);
            }
        }

        voronoi.setBoundingRect(PITCH_RECT);
        voronoi.compute();


        //
        // check segments
        //
        for (VoronoiDiagram::Segment2DCont::const_iterator s = voronoi.segments().begin(),
             end = voronoi.segments().end();
             s != end;
             ++s) {
            dlog.addLine(Logger::BLOCK,s->origin(), s->terminal(),0,0,255);
            AngleDeg orgin2termin = (s->terminal() - s->origin()).th();
            for(int d = 1.0; d < s->origin().dist(s->terminal()); d += 1.0){
                Vector2D tar = s->origin() + Vector2D::polar2vector(d,orgin2termin);
                if(tar.dist(self_home_pos) > 8)
                    continue;
                v_points.push_back( tar );
            }
        }

        VoronoiDiagram::Vector2DCont V = voronoi.resultPoints();
        for (VoronoiDiagram::Vector2DCont::const_iterator it = V.begin(); it != V.end(); it++) {
            if (!(*it).isValid()) continue;
            if ((*it).absX() > max_x || (*it).absY() > max_y)
                continue;
            if((*it).dist(self_home_pos) > 8)
                continue;
            v_points.push_back((*it));
        }

        size_t v_points_size = v_points.size();
        for(size_t i = 0; i < v_points_size; i++){
            for(double d = 2; d <= 2.0; d+= 1.5){
                for(double a = -180; a < 180; a += 45){
                    Vector2D newpoint = v_points[i] + Vector2D::polar2vector(d,a);
                    if(newpoint.x > max_x || newpoint.y < min_y || newpoint.y > max_y)
                        continue;
                    if(newpoint.dist(self_home_pos) > 10)
                        continue;
                    v_points.push_back( newpoint);

                }
            }
        }
        for(double d = 0; d <= 6; d+= 2){
            for(double a = -180; a < 180; a+= 45){
                Vector2D tar = Vector2D::polar2vector(d,wm.self().body().degree() + a) + self_home_pos;
                if(tar.x > max_x || tar.absY() > 33)
                    continue;
                v_points.push_back(tar);
            }
        }
        std::vector<Vector2D> v_points_n;
        for(size_t i = 0; i < v_points.size(); i++){
            bool is_near_me = true;
            for(size_t j=0; j < tm_home_poses.size(); j++){
                if(v_points[i].dist(tm_home_poses[j]) + 2.0 < v_points[i].dist(self_home_pos)){
                    is_near_me = false;
                    break;
                }
            }
            if(v_points[i].dist(ballInertiaPos) < 5)
                is_near_me = false;
            if(is_near_me){
                v_points_n.push_back(v_points[i]);
                //                dlog.addCircle(Logger::BLOCK,v_points_n[v_points_n.size()-1],0.2,0,255,0,true);
            }
        }
        return v_points_n;

    }

    double evaluate_point(rcsc::PlayerAgent *agent, const Vector2D & point,const std::pair< vector<Vector2D>,vector<int> > & passer_pos_unum,const int & num, const vector<unmark_type> & type_of_unmark) {
        const WorldModel &wm = agent->world();
        dlog.addText(Logger::BLOCK,"$%d $$$$point(%.2f,%.2f)",num,point.x,point.y);
        double eval_ball_pass = 0; // 0,100
        double eval_tm_pass = 0; // [0,300]
        double eval_home_pos = 0;
        double eval_opp_pos = 0;
        double eval_tm_pos = 0;
        double eval_tm_hpos = 15;
        double eval_shoot = 0;
        double res;
        int fastest_tm_unum = 0;
        if(wm.interceptTable()->fastestTeammate() != NULL){
            fastest_tm_unum = wm.interceptTable()->fastestTeammate()->unum();
        }

        //ball pass && tm pass
        double tm_pass_number = 0;
        for(size_t i = 0; i < std::min(passer_pos_unum.first.size(), static_cast<size_t>(3)); i++){
            auto eval_pass = get_pass_eval_from(wm,passer_pos_unum.first[i],point);
            dlog.addText(Logger::BLOCK,"-----received pass from %d = %.1f",passer_pos_unum.second[i],eval_pass.first);
            if( passer_pos_unum.second[i] == fastest_tm_unum){
                eval_ball_pass = eval_pass.first;
            }else{
                eval_tm_pass += eval_pass.first;
                tm_pass_number ++;
            }
        }
        eval_ball_pass /= 18.4;
        eval_tm_pass /= std::max(1.0, tm_pass_number);
        eval_tm_pass /= 18.4;

        //eval tm hpos
        for(int t = 1; t <= 11; t++){
            if ( t == wm.self().unum() )
                continue;
            double dist = Strategy::i().getPosition(t).dist(point);
            if(dist < eval_tm_hpos)
                eval_tm_hpos = dist;
        }
        eval_tm_hpos = std::min(10.0, eval_tm_hpos);

        //eval h pos
        eval_home_pos = Strategy::i().getPosition(wm.self().unum()).dist(point);
        eval_home_pos = std::max(0.0, 10.0 - eval_home_pos);

        //eval opp
        eval_opp_pos = std::min(wm.getDistOpponentNearestTo(point,5),10.0);
        if(eval_opp_pos < 3)
            eval_opp_pos = 3;
        else if(eval_opp_pos < 6)
            eval_opp_pos = 7;
        else
            eval_opp_pos = 10.0;

        //eval tm
        eval_tm_pos = std::min(wm.getDistTeammateNearestTo(point,5),10.0);

        if(eval_tm_pos < 3)
            eval_tm_pos = 4;
        else if(eval_tm_pos < 6)
            eval_tm_pos = 7;
        else
            eval_tm_pos = 10.0;
        dlog.addText(Logger::BLOCK,"------first eval: direct:%.1f,tmpass:%.1f, tmhpos:%.1f,opp:%.1f,hpos:%.1f, tmpos:%.1f",eval_ball_pass,eval_tm_pass,eval_tm_hpos,eval_opp_pos,eval_home_pos,eval_tm_pos);
        res = evaluate_by_type(type_of_unmark,eval_ball_pass, eval_tm_pass, eval_tm_hpos, eval_home_pos, eval_opp_pos, eval_tm_pos);

        return res;

    }
    double evaluate_by_type(const vector<unmark_type> & type_of_unmark,
                            double eval_ball_pass,
                            double eval_tm_pass,
                            double eval_tm_hpos,
                            double eval_home_pos,
                            double eval_opp_pos,
                            double eval_tm_pos){
        double eval_directpass = 0;
        double eval_tmpass = 0;
        double eval_hompos = 0;
        double eval_freepos = 0;

        eval_directpass = eval_ball_pass * 10+
                eval_tm_pass * 2+
                eval_tm_hpos * 2+
                eval_home_pos * 3+
                eval_opp_pos * 3+
                eval_tm_pos * 3;
        eval_directpass /= 23.0;

        eval_tmpass = eval_ball_pass * 4+
                eval_tm_pass * 10+
                eval_tm_hpos * 3+
                eval_home_pos * 3+
                eval_opp_pos * 5+
                eval_tm_pos * 3;
        eval_tmpass /= 28.0;

        eval_hompos = eval_ball_pass * 6+
                eval_tm_pass * 2+
                eval_tm_hpos * 8+
                eval_home_pos * 10+
                eval_opp_pos * 3+
                eval_tm_pos * 5;
        eval_hompos /= 34.0;

        eval_freepos = eval_ball_pass * 4+
                eval_tm_pass * 2+
                eval_tm_hpos * 3+
                eval_home_pos * 5+
                eval_opp_pos * 10+
                eval_tm_pos * 3;
        eval_freepos /= 27.0;


        size_t size = type_of_unmark.size();
        double evals = 0;
        for(size_t i = 0; i < size; i++){
            double baseeval = 0;
            if(type_of_unmark[i] == ut_directpass){
                baseeval = eval_directpass;
            }
            else if(type_of_unmark[i] == ut_tmpass){
                baseeval = eval_tmpass;
            }
            else if(type_of_unmark[i] == ut_homepos){
                baseeval = eval_hompos;
            }
            else if(type_of_unmark[i] == ut_freepos){
                baseeval = eval_freepos;
            }
            double eval = std::pow(10,size - i + 1) * baseeval;
            evals += eval;
        }
        dlog.addText(Logger::BLOCK,"-----eval direct:%.1f, tmpass:%.1f, hompos:%.1f, freepos:%.1f = evals:%.1f",eval_directpass,eval_tmpass,eval_hompos, eval_freepos,evals);
        return evals;
    }

    pair<double,Vector2D> get_pass_eval_from(const WorldModel & wm, Vector2D ball,Vector2D candid){
        double all_eval = 0;
        double best_eval = 0;
        Vector2D bestpass = Vector2D::INVALIDATED;
        int pass_number = 0;
        for(double d = 0; d <= 6; d+=2){
            for(double a = -135; a <= 135; a+=45){
                Vector2D pass_target = candid + Vector2D::polar2vector(d,a + (ball - candid).th());
                double pass_speed = calc_pass_speed(d,ball.dist(pass_target));
                Sector2D pass_sector = Sector2D(ball,0,ball.dist(candid) + 5.0,(pass_target - ball).th() - AngleDeg(25),(pass_target - ball).th() + AngleDeg(25));
                vector<const AbstractPlayerObject *> opps;
                for(int i=1; i <= 11; i++){
                    const AbstractPlayerObject * opp = wm.theirPlayer(i);
                    if( opp!= NULL && opp->unum() == i){
                        if( pass_sector.contains(opp->pos())){
                            opps.push_back(opp);
                        }
                    }
                }

                Vector2D pass_vel = Vector2D::polar2vector(pass_speed,(pass_target - ball).th());
                int pass_cycle = 0;
                {
                    double move = 0;
                    double speed = pass_speed;
                    while(move < ball.dist(pass_target)){
                        pass_cycle++;
                        move += speed;
                        speed *= ServerParam::i().ballDecay();
                    }
                }

                Vector2D ball_sim = ball;
                ball_sim += pass_vel;
                pass_vel *= ServerParam::i().ballDecay();
                bool can_intercept = false;
                double min_dif = 5;
                for (int c = 1; c < pass_cycle; c++){
                    for(size_t o = 0; o < opps.size(); o++){
                        if(opps[o]->pos().dist(ball_sim) < c){
                            can_intercept = true;
                            break;
                        }
                        min_dif = std::min(min_dif,opps[o]->pos().dist(ball_sim) - c);
                    }
                    if(can_intercept)
                        break;
                    ball_sim += pass_vel;
                    pass_vel *= ServerParam::i().ballDecay();
                }
                if(!can_intercept){
                    double eval = pass_target.x + std::max(0.0, 40 - pass_target.dist(Vector2D(52,0))) + 3 * min_dif;
                    all_eval += eval;
                    pass_number += 1;
                    if(eval > best_eval){
                        best_eval = eval;
                        bestpass = pass_target;
                    }
                }
                if ( d == 0 )
                    break;
            }
        }

        return make_pair(all_eval / static_cast<double>(std::max(1,pass_number)) + best_eval, bestpass );
    }

    double calc_pass_speed(double self2pos,double ball2pos){

        double first = 1.5;
        double sum = 0;
        while (sum < ball2pos){
            first /= ServerParam::i().ballDecay();
            sum += first;
        }
        if(first > 3)
            first = 3;
        return first;
    }

    std::pair<vector<Vector2D>,vector<int> > where_is_passer(const WorldModel & wm){

        vector<Vector2D> res;
        vector<int> res_unum;
        Vector2D self_h_pos = Strategy::i().getPosition(wm.self().unum());
        Vector2D self_pos = wm.self().pos();
        int unum = wm.self().unum();
        const PlayerObject * fastest_tm = wm.interceptTable()->fastestTeammate();
        int fastest_tm_unum = 0;

        int mate_min = wm.interceptTable()->teammateReachCycle();
        Vector2D ballpos;
        if(fastest_tm != NULL){
            ballpos = wm.ball().inertiaPoint(mate_min);
            fastest_tm_unum = fastest_tm->unum();
            if(ballpos.dist(self_h_pos) < 30){
                res.push_back(ballpos);
                res_unum.push_back(fastest_tm_unum);
            }
        }else{
            ballpos = wm.ball().pos();
            if(ballpos.dist(self_h_pos) < 30){
                res.push_back(ballpos);
                res_unum.push_back(0);
            }
        }
        Segment2D ball2hpos = Segment2D(ballpos,self_h_pos);
        for(int t = 2; t <= 11; t++){
            const AbstractPlayerObject * tm = wm.ourPlayer(t);
            if( tm == NULL || tm->unum() != t)
                continue;
            if( t == wm.self().unum())
                continue;
            if( t == fastest_tm_unum)
                continue;
            Vector2D tm_pos = tm->pos();
            Vector2D proj = ball2hpos.projection(tm_pos);
            if(proj.isValid()
                    && proj.dist(tm_pos) < 10
                    && tm_pos.dist(self_h_pos) < 15){
                res.push_back(tm_pos);
                res_unum.push_back(tm->unum());
            }
        }
        vector<int> static_tm;
        Rect2D A = Rect2D(Vector2D(45.0,-34),Vector2D(52.5,-20));
        Rect2D B = Rect2D(Vector2D(30,-34),Vector2D(45, -20));
        Rect2D C = Rect2D(Vector2D(45, 20),Vector2D(52.5,34));
        Rect2D D = Rect2D(Vector2D(30, 20),Vector2D(45,  34));
        Rect2D E = Rect2D(Vector2D(45,-20),Vector2D(52.5,-8));
        Rect2D F = Rect2D(Vector2D(45,-8), Vector2D(52.5, 8));
        Rect2D G = Rect2D(Vector2D(45, 8), Vector2D(52.5,20));
        Rect2D H = Rect2D(Vector2D(30,-20),Vector2D(52.5,-8));
        Rect2D I = Rect2D(Vector2D(30,-8) ,Vector2D(52.5, 8));
        Rect2D J = Rect2D(Vector2D(30, 8) ,Vector2D(52.5,20));
        if(A.contains(ballpos) || E.contains(ballpos)){
            if(unum == 5){
                static_tm.push_back(7);
                static_tm.push_back(9);
            }else if(unum == 6){
                static_tm.push_back(7);
                static_tm.push_back(5);
            }else if(unum == 7){
                static_tm.push_back(9);
            }else if(unum == 8){
                static_tm.push_back(6);
                static_tm.push_back(10);
                static_tm.push_back(11);
            }else if(unum == 9){
                static_tm.push_back(7);
                static_tm.push_back(11);
            }else if(unum == 10){
                static_tm.push_back(11);
                static_tm.push_back(5);
                static_tm.push_back(7);
            }else if(unum == 11){
                static_tm.push_back(9);
                static_tm.push_back(7);
                static_tm.push_back(5);
            }
        }else if(B.contains(ballpos) || H.contains(ballpos)){
            if(unum == 5){
                static_tm.push_back(7);
                static_tm.push_back(9);
            }else if(unum == 6){
                static_tm.push_back(7);
                static_tm.push_back(5);
            }else if(unum == 7){
                static_tm.push_back(9);
            }else if(unum == 8){
                static_tm.push_back(6);
                static_tm.push_back(10);
                static_tm.push_back(5);
            }else if(unum == 9){
                static_tm.push_back(7);
                static_tm.push_back(5);
            }else if(unum == 10){
                static_tm.push_back(11);
                static_tm.push_back(5);
                static_tm.push_back(7);
            }else if(unum == 11){
                static_tm.push_back(9);
                static_tm.push_back(7);
                static_tm.push_back(5);
            }
        }else if(C.contains(ballpos) || G.contains(ballpos)){
            if(unum == 5){
                static_tm.push_back(6);
                static_tm.push_back(8);
                static_tm.push_back(10);
            }else if(unum == 6){
                static_tm.push_back(8);
                static_tm.push_back(10);
                static_tm.push_back(11);
            }else if(unum == 7){
                static_tm.push_back(10);
                static_tm.push_back(6);
                static_tm.push_back(11);
            }else if(unum == 8){
                static_tm.push_back(6);
                static_tm.push_back(10);
                static_tm.push_back(11);
            }else if(unum == 9){
                static_tm.push_back(7);
                static_tm.push_back(11);
            }else if(unum == 10){
                static_tm.push_back(8);
                static_tm.push_back(6);
            }else if(unum == 11){
                static_tm.push_back(10);
                static_tm.push_back(8);
                static_tm.push_back(6);
            }
        }else if(D.contains(ballpos) || J.contains(ballpos)){
            if(unum == 5){
                static_tm.push_back(6);
                static_tm.push_back(8);
                static_tm.push_back(10);
            }else if(unum == 6){
                static_tm.push_back(8);
                static_tm.push_back(10);
                static_tm.push_back(11);
            }else if(unum == 7){
                static_tm.push_back(10);
                static_tm.push_back(6);
                static_tm.push_back(11);
            }else if(unum == 8){
                static_tm.push_back(6);
                static_tm.push_back(10);
                static_tm.push_back(11);
            }else if(unum == 9){
                static_tm.push_back(7);
                static_tm.push_back(11);
            }else if(unum == 10){
                static_tm.push_back(8);
                static_tm.push_back(6);
            }else if(unum == 11){
                static_tm.push_back(10);
                static_tm.push_back(8);
                static_tm.push_back(6);
            }
        }else if(F.contains(ballpos) || I.contains(ballpos)){
            if(unum == 5){
                static_tm.push_back(6);
                static_tm.push_back(7);
            }else if(unum == 6){
                static_tm.push_back(8);
                static_tm.push_back(5);
                static_tm.push_back(7);
            }else if(unum == 7){
                static_tm.push_back(5);
                static_tm.push_back(6);
                static_tm.push_back(9);
            }else if(unum == 8){
                static_tm.push_back(6);
                static_tm.push_back(5);
                static_tm.push_back(10);
            }else if(unum == 9){
                static_tm.push_back(7);
                static_tm.push_back(5);
            }else if(unum == 10){
                static_tm.push_back(11);
                static_tm.push_back(5);
                static_tm.push_back(7);
            }else if(unum == 11){
                static_tm.push_back(5);
                static_tm.push_back(7);
                static_tm.push_back(9);
            }
        }
        for(size_t i = 0; i < res_unum.size(); i++){
            int unum = res_unum[i];
            dlog.addText(Logger::BLOCK,"ball unum = %d",unum);
        }
        for(size_t i = 0; i < static_tm.size(); i++){
            int unum = static_tm[i];
            dlog.addText(Logger::BLOCK,"passer unum = %d",unum);
            if(wm.ourPlayer(unum) == NULL || wm.ourPlayer(unum)->unum() != unum)
                continue;
            if(wm.ourPlayer(unum)->pos().dist(self_h_pos) > 25 || wm.ourPlayer(unum)->pos().dist(ballpos) > 25)
                continue;
            if(find(res_unum.begin(),res_unum.end(),unum) == res_unum.end()){
                res.push_back(wm.ourPlayer(unum)->pos());
                res_unum.push_back(wm.ourPlayer(unum)->unum());
                dlog.addLine(Logger::BLOCK,wm.self().pos(),wm.ourPlayer(unum)->pos());
            }
        }
        return std::make_pair(res,res_unum);
    }
    bool is_contain(Rect2D & A, Vector2D & p){
        if(!p.isValid()){
            return false;
        }
        if(A.contains(p))
            return true;
        return false;
    }

    Vector2D get_tm_pos(const WorldModel & wm, int unum){
        if(wm.ourPlayer(unum) != NULL){
            if(wm.ourPlayer(unum)->unum() == unum){
                if(wm.ourPlayer(unum)->pos().isValid())
                    return wm.ourPlayer(unum)->pos();
            }
        }
        return Vector2D::INVALIDATED;
    }

    vector<unmark_type> best_type_of_unmark(const WorldModel & wm){
        vector<unmark_type> res;
        int unum = wm.self().unum();
        Vector2D self_hpos = Strategy::i().getPosition(unum);
        int tm_cycle = wm.interceptTable()->teammateReachCycle();
        Vector2D ball = wm.ball().inertiaPoint(tm_cycle);
        int fast_tm = 0;
        if(wm.interceptTable()->fastestTeammate() != NULL)
            fast_tm = wm.interceptTable()->fastestTeammate()->unum();

        vector<Vector2D> tmpos;
        for(int i = 0; i<=11; i++){
            tmpos.push_back(get_tm_pos(wm,i));
        }
        Rect2D A = Rect2D(Vector2D(45.0,-34),Vector2D(52.5,-20));
        Rect2D B = Rect2D(Vector2D(30,-34),Vector2D(45, -20));
        Rect2D C = Rect2D(Vector2D(45, 20),Vector2D(52.5,34));
        Rect2D D = Rect2D(Vector2D(30, 20),Vector2D(45,  34));
        Rect2D E = Rect2D(Vector2D(45,-20),Vector2D(52.5,-8));
        Rect2D F = Rect2D(Vector2D(45,-8), Vector2D(52.5, 8));
        Rect2D G = Rect2D(Vector2D(45, 8), Vector2D(52.5,20));
        Rect2D H = Rect2D(Vector2D(30,-20),Vector2D(52.5,-8));
        Rect2D I = Rect2D(Vector2D(30,-8) ,Vector2D(52.5, 8));
        Rect2D J = Rect2D(Vector2D(30, 8) ,Vector2D(52.5,20));
        if(unum == 11){
            res.push_back(ut_directpass);
            res.push_back(ut_tmpass);
            res.push_back(ut_freepos);
            res.push_back(ut_homepos);
        }else if(unum == 10){
            if(A.contains(ball) || B.contains(ball)){
                if(fast_tm == 11){
                    res.push_back(ut_directpass);
                    res.push_back(ut_tmpass);
                    res.push_back(ut_freepos);
                    res.push_back(ut_homepos);
                }else{
                    res.push_back(ut_tmpass);
                    res.push_back(ut_directpass);
                    res.push_back(ut_freepos);
                    res.push_back(ut_homepos);
                }
            }else{
                res.push_back(ut_directpass);
                res.push_back(ut_tmpass);
                res.push_back(ut_freepos);
                res.push_back(ut_homepos);
            }
        }else if(unum == 9){
            if(A.contains(ball) || B.contains(ball) || F.contains(ball) || E.contains(ball) || I.contains(ball) || H.contains(ball)){
                res.push_back(ut_directpass);
                res.push_back(ut_tmpass);
                res.push_back(ut_freepos);
                res.push_back(ut_homepos);
            }else{
                if(fast_tm == 11){
                    res.push_back(ut_directpass);
                    res.push_back(ut_tmpass);
                    res.push_back(ut_freepos);
                    res.push_back(ut_homepos);
                }else{
                    res.push_back(ut_freepos);
                    res.push_back(ut_homepos);
                    res.push_back(ut_tmpass);
                    res.push_back(ut_directpass);
                }
            }
        }else if(unum == 8){
            if(A.contains(ball) || B.contains(ball)){
                if(is_contain(E,tmpos[10]) || is_contain(F,tmpos[10])){
                    res.push_back(ut_directpass);
                    res.push_back(ut_freepos);
                    res.push_back(ut_homepos);
                    res.push_back(ut_tmpass);
                }else{
                    res.push_back(ut_freepos);
                    res.push_back(ut_homepos);
                    res.push_back(ut_tmpass);
                    res.push_back(ut_directpass);
                }
            }else{
                res.push_back(ut_directpass);
                res.push_back(ut_tmpass);
                res.push_back(ut_freepos);
                res.push_back(ut_homepos);
            }
        }else if(unum == 7){
            if(A.contains(ball) || B.contains(ball) || H.contains(ball) || I.contains(ball) || E.contains(ball) || F.contains(ball)){
                res.push_back(ut_directpass);
                res.push_back(ut_freepos);
                res.push_back(ut_tmpass);
                res.push_back(ut_homepos);
            }else{
                res.push_back(ut_freepos);
                res.push_back(ut_homepos);
                res.push_back(ut_directpass);
                res.push_back(ut_tmpass);
            }
        }else if(unum == 6){
            if(J.contains(ball) || G.contains(ball) || D.contains(ball) || C.contains(ball)){
                res.push_back(ut_directpass);
                res.push_back(ut_freepos);
                res.push_back(ut_tmpass);
                res.push_back(ut_homepos);
            }else if(H.contains(ball) || B.contains(ball) || I.contains(ball)){
                res.push_back(ut_directpass);
                res.push_back(ut_freepos);
                res.push_back(ut_tmpass);
                res.push_back(ut_homepos);
            }else {
                res.push_back(ut_freepos);
                res.push_back(ut_homepos);
                res.push_back(ut_tmpass);
                res.push_back(ut_directpass);
            }
        }else if(unum == 5){
            if(A.contains(ball) || B.contains(ball) || H.contains(ball) || E.contains(ball)){
                res.push_back(ut_directpass);
                res.push_back(ut_freepos);
                res.push_back(ut_tmpass);
                res.push_back(ut_homepos);
            }else if(J.contains(ball) || I.contains(ball)){
                res.push_back(ut_directpass);
                res.push_back(ut_freepos);
                res.push_back(ut_tmpass);
                res.push_back(ut_homepos);
            }else {
                res.push_back(ut_freepos);
                res.push_back(ut_homepos);
                res.push_back(ut_tmpass);
                res.push_back(ut_directpass);
            }
        }else{
            res.push_back(ut_freepos);
            res.push_back(ut_homepos);
            res.push_back(ut_directpass);
            res.push_back(ut_tmpass);
        }
        return res;
    }

};


#endif
