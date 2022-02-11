//
// Created by nader on 2022-02-10.
//

#ifndef TEAM_DESTATE_H
#define TEAM_DESTATE_H
#include <rcsc/geom/vector_2d.h>
#include <rcsc/geom/angle_deg.h>
#include <rcsc/player/abstract_player_object.h>
#include <rcsc/player/ball_object.h>
#include <rcsc/player/world_model.h>
#include <vector>
using namespace rcsc;
class DEBall{
public:
    Vector2D M_pos;
    Vector2D M_rpos;
    Vector2D M_vel;
    DEBall(){
        M_pos = Vector2D::INVALIDATED;
        M_rpos = Vector2D::INVALIDATED;
        M_vel = Vector2D(0, 0);
    }
    DEBall(const BallObject & ball){
        M_pos = ball.pos();
        M_vel = ball.vel();
    }
    void update_rpos(Vector2D & kicker_pos){
        M_rpos = kicker_pos - M_pos;
    }
    const Vector2D & pos() const{
        return M_pos;
    }
    const Vector2D & vel() const{
        return M_vel;
    }
    const Vector2D & rpos() const{
        return M_rpos;
    }
    bool posValid() const{

    }
    bool velValid() const{

    }
    bool rposValid() const{

    }
};
class DEPlayer{
public:
    Vector2D M_pos;
    Vector2D M_vel;
    AngleDeg M_body;
    AngleDeg M_face;
    int M_unum;
    int M_side;
    double M_dist_from_ball;
    DEPlayer(){
        M_pos = Vector2D::INVALIDATED;
        M_vel = Vector2D(0, 0);
        M_body = 0;
        M_face = 0;
        M_unum = -1;
        M_side = 0;
        M_dist_from_ball = 0;
    }
    DEPlayer(const AbstractPlayerObject * p, DEBall & ball){
        M_pos = p->pos();
        M_vel = p->vel();
        M_body = p->body();
        M_face = p->face();
        M_unum = p->unum();
        M_side = p->side();
        M_dist_from_ball = pos().dist(ball.pos());
    }
    const int & unum(){
        return M_unum;
    }
    const int & side(){
        return M_side;
    }
    const Vector2D & pos(){
        return M_pos;
    }
    const Vector2D & vel(){
        return M_vel;
    }
    const AngleDeg & body(){
        return M_body;
    }
    const AngleDeg & face(){
        return M_face;
    }
    const double & distFromBall(){
        return M_dist_from_ball;
    }
};
class DEState {
public:
    DEBall M_ball;
    std::vector<DEPlayer> M_all_players;
    std::vector<DEPlayer*> M_teammates;
    std::vector<DEPlayer*> M_opponents;
    std::vector<DEPlayer*> M_unknown_players;
    DEPlayer * M_known_teammates[12];
    DEPlayer * M_known_opponents[12];
    std::vector<DEPlayer*> M_our_players;
    std::vector<DEPlayer*> M_their_players;
    DEState(const WorldModel & wm){
        M_ball = DEBall(wm.ball());
        for (auto p: wm.allPlayers()){
            M_all_players.push_back(DEPlayer(p, M_ball));
        }
        updateVectors();
    }
    const DEBall & ball() const{
        return M_ball;
    }
    void updateVectors(){
        for (int i = 0; i <= 11; i++){
            M_known_teammates[i] = nullptr;//DEPlayer();
            M_known_opponents[i] = nullptr;//DEPlayer();
        }
        for (auto p: M_all_players){
            if (p.side() == -1){
                M_our_players.push_back(&p);
                M_teammates.push_back(&p);
                if (p.unum() != -1){
                    M_known_teammates[p.unum()] = &p;
                }
            }else{
                M_their_players.push_back(&p);
                if(p.side() == 1){
                    M_opponents.push_back(&p);
                }else{
                    M_unknown_players.push_back(&p);
                }
                if (p.unum() != -1){
                    M_known_opponents[p.unum()] = &p;
                }
            }
        }
    }
    std::vector<DEPlayer> & allPlayers(){
        return M_all_players;
    }
    std::vector<DEPlayer*> teammates(){
        return M_teammates;
    }
    std::vector<DEPlayer*> opponents(){
        return M_opponents;
    }
    std::vector<DEPlayer*> unknownPlayers(){
        return M_unknown_players;
    }
    std::vector<DEPlayer*> ourPlayers(){
        return M_our_players;
    }
    std::vector<DEPlayer*> theirPlayers(){
        return M_their_players;
    }
    DEPlayer * ourPlayer(int i){
        if (i < 0 || i > 11)
            return nullptr;
        return M_known_teammates[i];
    }
    DEPlayer * theirPlayer(int i){
        if (i < 0 || i > 11)
            return nullptr;
        return M_known_opponents[i];
    }
};


#endif //TEAM_DESTATE_H
