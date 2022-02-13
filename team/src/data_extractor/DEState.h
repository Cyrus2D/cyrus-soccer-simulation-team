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
    int M_pos_count;
    int M_rpos_count;
    int M_vel_count;
    const BallObject * M_ball;
    DEBall():
    M_ball(nullptr)
    {
        M_pos = Vector2D::INVALIDATED;
        M_rpos = Vector2D::INVALIDATED;
        M_vel = Vector2D(0, 0);
    }
    DEBall(const BallObject & ball):
        M_ball(&ball){
        M_pos = ball.pos();
        M_vel = ball.vel();
    }
    const BallObject * ball() const{
        return M_ball;
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
    const int & rposCount() const{
        return M_rpos_count;
    }
    const int & posCount() const{
        return M_pos_count;
    }
    const int & velCount() const{
        return M_vel_count;
    }
    bool posValid() const{
        return M_pos.isValid();
    }
    bool velValid() const{
        return M_rpos.isValid();
    }
    bool rposValid() const{
        return M_vel.isValid();
    }
};
class DEPlayer{
public:
    Vector2D M_pos;
    Vector2D M_vel;
    AngleDeg M_body;
    AngleDeg M_face;
    int M_pos_count;
    int M_rpos_count;
    int M_vel_count;
    int M_body_count;
    int M_face_count;
    int M_unum;
    int M_side;
    double M_dist_from_ball;
    bool M_is_ghost;
    const PlayerType * M_player_type;
    const AbstractPlayerObject * M_player;

    DEPlayer():
        M_player(nullptr),
        M_is_ghost(false),
        M_player_type(nullptr){
        M_pos = Vector2D::INVALIDATED;
        M_vel = Vector2D(0, 0);
        M_body = 0;
        M_face = 0;
        M_unum = -1;
        M_side = 0;
        M_dist_from_ball = 0;
        M_is_ghost = false;
        M_body_count = 0;
        M_face_count = 0;
    }
    DEPlayer(const AbstractPlayerObject * p, DEBall & ball):
            M_player_type(p->playerTypePtr()),
            M_player(p)
        {
        M_pos = p->pos();
        M_vel = p->vel();
        M_body = p->body();
        M_face = p->face();
        M_unum = p->unum();
        M_side = p->side();
        M_dist_from_ball = pos().dist(ball.pos());
        M_is_ghost = p->isGhost();
        M_body_count = p->bodyCount();
        M_body_count = p->faceCount();
    }
    const AbstractPlayerObject * player() const{
        return M_player;
    }
    const int & unum() const{
        return M_unum;
    }
    const int & side() const{
        return M_side;
    }
    const Vector2D & pos() const{
        return M_pos;
    }
    const Vector2D & vel() const{
        return M_vel;
    }
    const AngleDeg & body() const{
        return M_body;
    }
    const AngleDeg & face() const{
        return M_face;
    }
    const int & rposCount() const{
        return M_rpos_count;
    }
    const int & posCount() const{
        return M_pos_count;
    }
    const int & velCount() const{
        return M_vel_count;
    }
    const int & bodyCount() const{
        return M_body_count;
    }
    const int & faceCount() const{
        return M_face_count;
    }
    double distFromBall() const{
        return M_dist_from_ball;
    }
    bool isGhost() const{
        return M_is_ghost;
    }
    const PlayerType * playerTypePtr() const{
        return M_player_type;
    }
    const bool isTackling() const{
        return false;
    }
    const bool kicked() const{
        return false;
    }
};
class DEState {
public:
    int M_cycle;
    DEBall M_ball;
    std::vector<DEPlayer> M_all_players;
    std::vector<DEPlayer*> M_teammates;
    std::vector<DEPlayer*> M_opponents;
    std::vector<DEPlayer*> M_unknown_players;
    DEPlayer * M_known_teammates[12];
    DEPlayer * M_known_opponents[12];
    std::vector<DEPlayer*> M_our_players;
    std::vector<DEPlayer*> M_their_players;
    int M_kicker_unum;
    DEPlayer* M_kicker_player;
    double M_offside_line_x;
    int M_offside_line_count;
    int M_our_side;
    const WorldModel &M_wm;

    DEState(const WorldModel & wm):
        M_wm(wm)
        {
        M_cycle = wm.time().cycle();
        M_offside_line_x = wm.offsideLineX();
        M_offside_line_count = wm.offsideLineCount();
        M_our_side = wm.ourSide();
        M_ball = DEBall(wm.ball());
        for (auto p: wm.allPlayers()){
            M_all_players.push_back(DEPlayer(p, M_ball));
        }
        updateVectors(wm);
    }
    int cycle() const{
        return M_cycle;
    }
    const DEBall & ball() const{
        return M_ball;
    }
    void updateVectors(const WorldModel & wm){
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
        // kicker
        //update rpos
    }
    std::vector<DEPlayer> & allPlayers(){
        return M_all_players;
    }
    std::vector<DEPlayer*> teammates() const{
        return M_teammates;
    }
    std::vector<DEPlayer*> opponents() const{
        return M_opponents;
    }
    std::vector<DEPlayer*> unknownPlayers() const{
        return M_unknown_players;
    }
    std::vector<DEPlayer*> ourPlayers() const{
        return M_our_players;
    }
    std::vector<DEPlayer*> theirPlayers() const{
        return M_their_players;
    }
    DEPlayer * ourPlayer(int i) const{
        if (i < 0 || i > 11)
            return nullptr;
        return M_known_teammates[i];
    }
    DEPlayer * theirPlayer(int i) const{
        if (i < 0 || i > 11)
            return nullptr;
        return M_known_opponents[i];
    }
    DEPlayer * kicker() const{
        return M_kicker_player;
    }
    int kickerUnum() const{
        return M_kicker_unum;
    }
    int offsideLineCount() const{
        return M_offside_line_count;
    }
    double offsideLineX() const{
        return M_offside_line_x;
    }
    int ourSide() const{
        return M_our_side;
    }
    const WorldModel & wm() const{
        return M_wm;
    }
};


#endif //TEAM_DESTATE_H
