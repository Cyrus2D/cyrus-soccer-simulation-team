#ifndef BHV_VORONOI_SCAPE
#define BHV_VORONOI_SCAPE

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


bool go_to_target(PlayerAgent * agent, Vector2D target){
    const WorldModel & wm = agent->world();
    int mate_min = wm.interceptTable()->teammateReachCycle();
    int opp_min = wm.interceptTable()->opponentReachCycle();
    Vector2D self_pos = wm.self().pos();
    Vector2D ball_pos = wm.ball().inertiaPoint(mate_min);
    double dist2target = self_pos.dist(target);

    double dash_power = 100;//Strategy::i().get_normal_dash_power(wm);

    double angle_thr = 20;
    if(!Body_GoToPoint(target, 1, dash_power,1.2,3,false,20).execute(agent)){
        if(wm.self().body().abs() < 15){
            agent->doDash(100);
        }else{
            Body_TurnToAngle(0).execute(agent);
        }
    }
    agent->debugClient().addMessage("voro scape");
    agent->debugClient().setTarget(target);
    NeckDecisionWithBall().setNeck(agent);
//    agent->doPointto(target.x,target.y);
    return true;

}

class IntentionScape
        : public rcsc::SoccerIntention {
private:
    const rcsc::Vector2D M_target_point; // global coordinate
    int M_step;
    int M_ex_step;
    int M_last_ex_time;
    int M_start_passer;
    Vector2D M_start_ball_pos;

public:
    IntentionScape( const rcsc::Vector2D & target_point,
                     const int max_step,
                     const int last_ex_time,
                     const int start_passer,
                     const Vector2D start_ball_pos):
        M_target_point(target_point){
        M_step = max_step;
        M_last_ex_time = last_ex_time;
        M_start_ball_pos = start_ball_pos;
        M_start_passer = start_passer;
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

        go_to_target(agent, M_target_point);
        //        if(Body_GoToPoint(M_target_point,0.5,100).execute(agent)){
        //            agent->debugClient().setTarget(M_target_point);
        //            Bhv_BasicMove().offense_set_neck_action(agent);
        //            M_last_ex_time = agent->world().time().cycle();
        return true;
        //        }
        return false;
    }
};
bool compair_vec_y(const Vector2D & x, const Vector2D & y){
    return x.y < y.y;
}



class bhv_scape_voronoi{

public:
    bool execute(PlayerAgent *agent) {
        Timer timer1;
        const WorldModel &wm = agent->world();
        if(!can_scape(wm))
            return false;

        dlog.addText(Logger::MARK, "start bhv voronoi unmark");
        vector<Vector2D> voronoiPoints = voronoi_points(agent);
        std::sort(voronoiPoints.begin(), voronoiPoints.end(), compair_vec_y);
        double t1 = timer1.elapsedReal();
        Timer timer2;

        double t2 = timer2.elapsedReal();
        Timer timer3;
        Vector2D best = Vector2D::INVALIDATED;
        double ev = -100;
        vector<double> evals;
        int num = 0;
        for (auto point : voronoiPoints) {
            double tmp = evaluate_point(agent, point,num);
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
                dlog.addCircle(Logger::MARK, voronoiPoints[i], 0.1, evals[i] / ev * 255,0,0,true);
            }else{
                dlog.addCircle(Logger::MARK, voronoiPoints[i], 0.1, 0,0,0,true);
            }

            char str[16];
            snprintf( str, 16, "%d", num );
            dlog.addMessage(Logger::MARK,voronoiPoints[i].x,voronoiPoints[i].y,str,0,0,200);
            num+=1;
        }
        double t4 = timer4.elapsedReal();
        dlog.addText(Logger::MARK,"timer %f %f %f %f",t1,t2,t3,t4);
        if (best.isValid()) {
            dlog.addCircle(Logger::MARK, best, 0.5, 0, 0, 0, true);
            int now_passer = 0;
            if(wm.interceptTable()->fastestTeammate() != NULL)
                now_passer = wm.interceptTable()->fastestTeammate()->unum();
            agent->setIntention(new IntentionScape(best,5,wm.time().cycle(),now_passer,wm.ball().inertiaPoint(wm.interceptTable()->teammateReachCycle())));
            agent->debugClient().addMessage("voronoi");
            go_to_target(agent,best);
            return true;
        } else
            dlog.addText(Logger::MARK, "not valid point");
        return false;
    }
    bool can_scape(const WorldModel & wm) {
        Strategy::PostLine pl_line = Strategy::i().self_Line();
        double stamina = wm.self().stamina();
        if (wm.ball().inertiaPoint(wm.interceptTable()->teammateReachCycle()).dist(
                    wm.self().pos()) > 30)
            return false;
        if(wm.ball().inertiaPoint(wm.interceptTable()->teammateReachCycle()).x < -15)
            return false;
        if(wm.ball().inertiaPoint(wm.interceptTable()->teammateReachCycle()).x > 35)
            return false;
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
        if(Strategy::i().self_Line() != Strategy::PostLine::forward)
            return v_points;

        const WorldModel &wm = agent->world();
        const int mate_min = wm.interceptTable()->teammateReachCycle();
        Vector2D ballInertiaPos = wm.ball().inertiaPoint(mate_min);
        Vector2D self_home_pos = Strategy::i().getPosition(wm.self().unum());
        double offside_line_x = std::max(ballInertiaPos.x,wm.offsideLineX());
        vector<Vector2D> tm_home_poses;
        double min_tm_home_x = 100;
        for(int u = 1; u <= 11; u++){
            if(Strategy::i().tm_Line(u) != Strategy::PostLine::forward)
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
        double min_x = offside_line_x - 10;
        double max_x = offside_line_x - 0.3;


        double mpy = 0;
        for(int i = 1; i<=11; i++){
            const AbstractPlayerObject * opp = wm.theirPlayer(i);
            if(opp == NULL || opp->unum() != i)
                continue;
            if(opp->pos().x > max_x - 15){
                if(opp->pos().y < mpy)
                    mpy = opp->pos().y;
            }
        }
        double min_y = std::max(ballInertiaPos.y - 38.0, -33.0 - (34 + mpy) + 5);
        double max_y = std::min(ballInertiaPos.y + 38.0, +33.0);


        const rcsc::Rect2D PITCH_RECT(rcsc::Vector2D(min_x,
                                                     min_y),
                                      rcsc::Size2D(max_x - min_x,
                                                   max_y - min_y));
        dlog.addRect(Logger::MARK,PITCH_RECT,255,0,0);


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
        opps_pos.push_back(make_pair(Vector2D((max_x), min_y),0));
        opps_pos.push_back(make_pair(Vector2D((max_x), max_y),0));

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
                dlog.addCircle(Logger::MARK,opps_pos[i].first,0.3,200,0,0,true);
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
            dlog.addLine(Logger::MARK,s->origin(), s->terminal(),0,0,255);
        }

        VoronoiDiagram::Vector2DCont V = voronoi.resultPoints();
        for (VoronoiDiagram::Vector2DCont::const_iterator it = V.begin(); it != V.end(); it++) {
            if (!(*it).isValid()) continue;
            dlog.addText(Logger::MARK,"voro point %.1f,%.1f",(*it).x,(*it).y);
            if ((*it).absX() > max_x || (*it).y > max_y || (*it).y < -33){
                dlog.addText(Logger::MARK,"con for max");
                continue;
            }
            if((*it).dist(Vector2D(max_x, self_home_pos.y)) > 12){
                dlog.addText(Logger::MARK,"con for hpos:hpos: %.1f,%.1f",self_home_pos.x,self_home_pos.y);
                dlog.addText(Logger::MARK,"con for hpos:hpos: %.1f,%.1f",max_x,self_home_pos.y);
                continue;
            }
            Vector2D p = (*it);
            if(p.x > max_x - 8){
                p.x = max_x;
                dlog.addCircle(Logger::MARK,p,1,0,0,255,true);
                for(double y = -2; y <= 2; y+=1)
                    v_points.push_back(p + Vector2D(0,y));
            }
        }


        std::vector<Vector2D> v_points_n;
        for(size_t i = 0; i < v_points.size(); i++){
            bool is_near_me = true;
            for(size_t j=0; j < tm_home_poses.size(); j++){
                if(v_points[i].dist(tm_home_poses[j]) + 3.0 < v_points[i].dist(self_home_pos)){
                    is_near_me = false;
                    break;
                }
            }
            if(v_points[i].dist(ballInertiaPos) < 5)
                is_near_me = false;

//            if(is_near_me && bhv_scape_voronoi::can_receive_th_pass(wm, v_points[i])){
            if(is_near_me){
                v_points_n.push_back(v_points[i]);
                dlog.addCircle(Logger::MARK,v_points_n[v_points_n.size() - 1],0.5,0,255,0);
            }
        }
        return v_points_n;

    }

    static bool can_receive_th_pass(const WorldModel & wm, Vector2D target){
        Vector2D ball_inertia = wm.ball().inertiaPoint(wm.interceptTable()->teammateReachCycle());
        double min_angle = -70;
        double max_angle = 70;
        double angle_step = 15;
        double min_dist = 6;
        double max_dist = 30;
        double dist_step = 2.0;
        PlayerType s_type = wm.self().playerType();
        for(double angle = min_angle; angle <= max_angle; angle += angle_step){
            int self_step = s_type.cyclesToReachDistance(dist_step) - 0.5;
            for(double dist = min_dist; dist <= max_dist; dist += dist_step){
                Vector2D pass_target = target + Vector2D::polar2vector(dist, angle);
                double ball_dist = ball_inertia.dist(pass_target);
                double first_speed = calc_first_term_geom_series(ball_dist, ServerParam::i().ballDecay(), self_step);
                if (first_speed > 3.0)
                    continue;
                Vector2D ball = ball_inertia;
                Vector2D vel = (pass_target - ball_inertia).setLengthVector(first_speed);
                bool opp_catch = false;
                for (int s = 1; s <= self_step; s++){
                    ball += vel;
                    vel *= ServerParam::i().ballDecay();
                    for(int o = 1; o <= 11; o++){
                        const AbstractPlayerObject * opp = wm.theirPlayer(o);
                        if(opp == NULL || opp->unum() < 1)
                            continue;
                        int opp_cycle = opp->playerTypePtr()->cyclesToReachDistance(ball.dist(opp->pos() + opp->vel())) + 1;
                        if (opp_cycle < s){
                            opp_catch = true;
                            break;
                        }
                    }
                    if (opp_catch)
                        break;
                }
                if (!opp_catch)
                    return true;
            }
        }
        return false;
    }
    double evaluate_point(rcsc::PlayerAgent *agent, const Vector2D & point,const int & num) {
        const WorldModel &wm = agent->world();
        dlog.addText(Logger::MARK,"$%d $$$$point(%.2f,%.2f)",num,point.x,point.y);
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
        dlog.addText(Logger::MARK,"------first eval: direct:%.1f,tmpass:%.1f, tmhpos:%.1f,opp:%.1f,hpos:%.1f, tmpos:%.1f",eval_ball_pass,eval_tm_pass,eval_tm_hpos,eval_opp_pos,eval_home_pos,eval_tm_pos);
//        res = evaluate_by_type(eval_ball_pass, eval_tm_pass, eval_tm_hpos, eval_home_pos, eval_opp_pos, eval_tm_pos);
        res = eval_tm_hpos + eval_home_pos + eval_tm_pos + eval_opp_pos;
        return res;
    }
};

#endif

