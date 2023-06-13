//
// Created by nader on 2023-05-15.
//

#ifndef CYRUS_DENOISING_H
#define CYRUS_DENOISING_H

#include <iostream>
#include <vector>
#include <map>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/object_table.h>
#include <rcsc/geom/convex_hull.h>

using namespace rcsc;
using namespace std;

Polygon2D mutual_convex(const Polygon2D &p1p, const Polygon2D &p2p);
void draw_poly(const Polygon2D &p, const char* color);

class PlayerPositionConvex {
public:
    int unum;
    std::vector<ConvexHull *> convexes_with_body;
    std::vector<ConvexHull *> convexes_without_body;

    PlayerPositionConvex()
            : unum(-1),
              convexes_with_body(),
              convexes_without_body(){}

    void init(std::ifstream &fin, int unum_);
    void init(const rcsc::WorldModel& wm, int unum_);

};


class Denoising {
public:
    static Denoising *instance;

    static Denoising *i();

    std::vector<double> self_pos_diff;

    void update(PlayerAgent *agent);

    void debug();
};


class PlayerStateCandidateArea {
public:
    Vector2D pos;
    int cycle;

    PlayerStateCandidateArea(Vector2D pos_);

};


class PlayerPredictedObjArea {
public:
    SideID side;
    int unum;
//    vector<PlayerStateCandidate> candidates;
//    vector<PlayerStateCandidate> candidates_means;
    ObjectTable object_table;
    Vector2D average_pos;
    PlayerPositionConvex player_data;
    Polygon2D* area;
    GameTime last_seen_time;

    PlayerPredictedObjArea(SideID side_, int unum_);

    PlayerPredictedObjArea();

    void generate_new_candidates(const WorldModel &wm, const PlayerObject *p);

    void check_candidates(const WorldModel &wm, const PlayerObject *p);

    void update_candidates(const WorldModel &wm, const PlayerObject *p);

    void update(const WorldModel &wm, const PlayerObject *p, int cluster_count);

    void debug();
};

class CyrusDenoiser {
public:
    map<int, PlayerPredictedObjArea> teammates;
    map<int, PlayerPredictedObjArea> opponents;
    vector<PlayerStateCandidateArea> empty_vector;
    int cluster_count = 1;
    long last_updated_cycle = -1;
    long last_update_stopped = 0;
    GameMode::Type last_updated_game_mode = GameMode::Type::TimeOver;

    static CyrusDenoiser *instance;

    static CyrusDenoiser *i();

    void update(PlayerAgent *agent);

    const vector<PlayerStateCandidateArea> get_cluster_means(const WorldModel &wm, SideID side, int unum);

    void debug();
};



#endif //CYRUS_DENOISING_H