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
#include <rcsc/player/intercept_table.h>
#include <vector>
#include <rcsc/common/logger.h>
#include <string>
#define DESDebug

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
    DEBall(){}
    DEBall(const BallObject & ball):
        M_ball(&ball){
        M_pos = ball.pos();
        M_vel = Vector2D(0, 0);//ball.vel();
        M_rpos = ball.rpos();
        M_pos_count = 0;//ball.posCount();
        M_rpos_count = 0;//ball.rposCount();
        M_vel_count = 0;//ball.velCount();
    }
    const BallObject * ball() const{
        return M_ball;
    }
    void update_pos(Vector2D pos){
        M_pos = pos;
    }
    void update_rpos(Vector2D & kicker_pos){
        M_rpos = kicker_pos - M_pos;
    }
    Vector2D & pos() {
        return M_pos;
    }
    Vector2D & vel() {
        return M_vel;
    }
    Vector2D & rpos() {
        return M_rpos;
    }
    int & rposCount() {
        return M_rpos_count;
    }
    int & posCount() {
        return M_pos_count;
    }
    int & velCount() {
        return M_vel_count;
    }
    bool posValid() {
        return M_pos_count <= 10;
    }
    bool velValid() {
        return M_vel_count <= 10;
    }
    bool rposValid() {
        return M_rpos_count <= 5;
    }
};
class DEPlayer{
public:
    Vector2D M_pos;
    Vector2D M_vel;
    AngleDeg M_body;
    AngleDeg M_face;
    int M_pos_count;
    int M_vel_count;
    int M_body_count;
    int M_face_count;
    int M_unum;
    int M_side;
    double M_dist_from_ball;
    bool M_is_ghost;
    const PlayerType * M_player_type;
    const AbstractPlayerObject * M_player;
    bool M_body_valid;
    DEPlayer(const AbstractPlayerObject * p, DEBall & ball):
            M_player_type(p->playerTypePtr()),
            M_player(p)
        {
        M_pos = p->pos();
        M_vel = Vector2D(0, 0);//p->vel();
        M_body = p->body();
        M_face = p->face();
        M_unum = p->unum();
        M_side = p->side();
        M_dist_from_ball = pos().dist(ball.pos());
        M_is_ghost = p->isGhost();
        M_pos_count = 0;//p->posCount();
        M_vel_count = 0;//p->velCount();
        M_body_count = 0;//p->bodyCount();
        M_face_count = 0;//p->faceCount();
        M_body_valid = false;
    }
    const AbstractPlayerObject * player(){
        return M_player;
    }
    int & unum(){
        return M_unum;
    }
    int & side(){
        return M_side;
    }
    Vector2D & pos(){
        return M_pos;
    }
    Vector2D & vel(){
        return M_vel;
    }
    AngleDeg & body(){
        return M_body;
    }
    AngleDeg & face(){
        return M_face;
    }
    int & posCount(){
        return M_pos_count;
    }
    int & velCount(){
        return M_vel_count;
    }
    int & bodyCount(){
        return M_body_count;
    }
    int & faceCount(){
        return M_face_count;
    }
    double distFromBall(){
        return M_dist_from_ball;
    }
    bool isGhost(){
        return M_is_ghost;
    }
    const PlayerType * playerTypePtr(){
        return M_player_type;
    }
    bool isTackling(){
        return false;
    }
    bool kicked(){
        return false;
    }
    bool bodyValid(){
        return M_body_count <= 2;
    }
    void updateForKicker(Vector2D & ball_pos){
        M_dist_from_ball = M_pos.dist(ball_pos);
    }
};
class DEState {
public:
    int M_cycle;
    DEBall M_ball;
    std::vector<DEPlayer*> M_all_players;
    std::vector<DEPlayer*> M_teammates;
    std::vector<DEPlayer*> M_opponents;
    std::vector<DEPlayer*> M_unknown_players;
    std::vector<DEPlayer*> M_known_teammates;
    std::vector<DEPlayer*> M_known_opponents;
    std::vector<DEPlayer*> M_our_players;
    std::vector<DEPlayer*> M_their_players;
    int M_kicker_unum;
    DEPlayer* M_kicker_player;
    double M_offside_line_x;
    int M_offside_line_count;
    int M_our_side;
    const WorldModel &M_wm;
    bool current_cycle;
    DEState(const WorldModel & wm):
        M_wm(wm)
    {
        M_cycle = wm.time().cycle();
        current_cycle = true;
        M_offside_line_x = wm.offsideLineX();
        M_offside_line_count = wm.offsideLineCount();
        M_our_side = wm.ourSide();
        M_ball = DEBall(wm.ball());
        #ifdef DESDebug
        dlog.addText(Logger::BLOCK, "Start updating M_all_players");
        #endif
        for (auto p: wm.allPlayers()){
            #ifdef DESDebug
            dlog.addText(Logger::BLOCK, "##side:%d, unum:%d is added to all_players", p->side(), p->unum());
            #endif
            M_all_players.push_back(new DEPlayer(p, M_ball));
        }
        #ifdef DESDebug
        dlog.addText(Logger::BLOCK, "End updating M_all_players");
        #endif
        updateVectors(wm);
    }
    ~DEState(){
        for (auto p : M_all_players)
        {
            delete p;
        }
        M_all_players.clear();
    }
    int cycle(){
        return M_cycle;
    }
    DEBall & ball(){
        return M_ball;
    }
    void updateVectors(const WorldModel & wm){
        #ifdef DESDebug
        dlog.addText(Logger::BLOCK, "Start updating Vectors");
        #endif
        for (int i = 0; i <= 11; i++){
            M_known_teammates.push_back(nullptr);//DEPlayer();
            M_known_opponents.push_back(nullptr);//DEPlayer();
        }
        for (auto p: M_all_players){
            #ifdef DESDebug
            dlog.addText(Logger::BLOCK, "--P side%d unum%d", p->side(), p->unum());
            #endif
            if (p->side() == ourSide()){
                #ifdef DESDebug
                dlog.addText(Logger::BLOCK, "###is tm");
                #endif
                M_our_players.push_back(p);
                M_teammates.push_back(p);
                #ifdef DESDebug
                dlog.addText(Logger::BLOCK, "####pushed to ourPlayers, teammates");
                #endif
                if (p->unum() != -1){
                    M_known_teammates[p->unum()] = p;
                    #ifdef DESDebug
                    dlog.addText(Logger::BLOCK, "#####add to kn tm %d %d, add %s", p->unum(), M_known_teammates[p->unum()]->unum(), p);
                    #endif
                }
            }else{
                #ifdef DESDebug
                dlog.addText(Logger::BLOCK, "###is opp");
                #endif
                M_their_players.push_back(p);
                #ifdef DESDebug
                dlog.addText(Logger::BLOCK, "#####pushed to their_players");
                #endif
                if(p->side() == -ourSide()){
                    #ifdef DESDebug
                    dlog.addText(Logger::BLOCK, "#####pushed to opponents");
                    #endif
                    M_opponents.push_back(p);
                }else{
                    #ifdef DESDebug
                    dlog.addText(Logger::BLOCK, "#####pushed to unknown_players");
                    #endif
                    M_unknown_players.push_back(p);
                }
                if (p->unum() != -1){
                    #ifdef DESDebug
                    dlog.addText(Logger::BLOCK, "#####pushed to known_opponents");
                    #endif
                    M_known_opponents[p->unum()] = p;
                }
            }
        }
        #ifdef DESDebug
        dlog.addText(Logger::BLOCK, "----M_known_teammates--- %d", M_known_teammates.size());
        for (auto p: M_known_teammates){
            if (p != nullptr)
                dlog.addText(Logger::BLOCK, "## tm %d side %d %s", p->unum(), p->side(), p);
        }
        #endif
        int fastest_unum = wm.interceptTable()->fastestTeammate()->unum();
        int tm_reach = wm.interceptTable()->teammateReachCycle();
        int self_reach = wm.interceptTable()->selfReachCycle();
        if (self_reach <= tm_reach){
            M_kicker_unum = wm.self().unum();
            M_kicker_player = M_known_teammates[wm.self().unum()];
        }else{
            M_kicker_unum = fastest_unum;
            M_kicker_player = M_known_teammates[fastest_unum];
        }
        if (M_kicker_unum == -1){
            M_kicker_player = nullptr;
        }
    }
    std::vector<DEPlayer*> & allPlayers(){
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
    DEPlayer * kicker(){
        return M_kicker_player;
    }
    int kickerUnum(){
        return M_kicker_unum;
    }
    bool updateKicker(int unum, Vector2D kicker_pos=Vector2D::INVALIDATED){
        if (unum < 0 || unum > 11)
            return false;
        if (M_known_teammates[unum] == nullptr)
            return false;
        M_kicker_unum = unum;
        M_kicker_player = M_known_teammates[unum];
        if (kicker_pos.isValid()){
            M_kicker_player->M_pos = kicker_pos;
        }
        M_ball.update_pos(M_kicker_player->pos() + Vector2D(0.2, 0));
        M_ball.update_rpos(M_kicker_player->pos());
        for (auto p: M_all_players){
            p->updateForKicker(M_ball.pos());
        }
        return true;
    }
    int offsideLineCount(){
        return M_offside_line_count;
    }
    double offsideLineX(){
        return M_offside_line_x;
    }
    int ourSide(){
        return M_our_side;
    }
    const WorldModel & wm(){
        return M_wm;
    }
};


#endif //TEAM_DESTATE_H
