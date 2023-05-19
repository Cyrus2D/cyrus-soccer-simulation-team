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

using namespace rcsc;
using namespace std;

class Denoising {
public:
    static Denoising *instance;

    static Denoising *i();

    std::vector<double> self_face_diff;
    std::vector<double> self_neck_diff;
    std::vector<double> self_body_diff;
    std::vector<double> self_pos_diff;

    void update(PlayerAgent *agent);

    void debug();
};


class PlayerStateCandidate {
public:
    Vector2D pos;
    Vector2D vel;
    double body = -360;
    double prob = 1.0;
    int cycle = 0;

    PlayerStateCandidate(Vector2D pos_, Vector2D vel_ = Vector2D::INVALIDATED, double body_ = -360);

    PlayerStateCandidate gen_random_next_by_dash(const WorldModel &wm, const PlayerObject *p) const;

    PlayerStateCandidate gen_random_next_by_turn(const WorldModel &wm, const PlayerObject *p) const;

    PlayerStateCandidate gen_random_next_by_nothing(const WorldModel &wm, const PlayerObject *p) const;

    PlayerStateCandidate gen_random_next(const WorldModel &wm, const PlayerObject *p) const;

    vector<PlayerStateCandidate> gen_max_next_candidates(const WorldModel &wm, const PlayerObject *p) const;
};


class PlayerPredictedObj {
public:
    SideID side;
    int unum;
    vector<PlayerStateCandidate> candidates;
    vector<PlayerStateCandidate> candidates_means;
    ObjectTable object_table;
    Vector2D average_pos;

    PlayerPredictedObj(SideID side_, int unum_);

    PlayerPredictedObj();

    void generate_new_candidates(const WorldModel &wm, const PlayerObject *p);

    void check_candidates(const WorldModel &wm, const PlayerObject *p);

    void update_candidates(const WorldModel &wm, const PlayerObject *p);

    void update(const WorldModel &wm, const PlayerObject *p, int cluster_count);

    void debug();
};

class CyrusDenoiser {
public:
    map<int, PlayerPredictedObj> teammates;
    map<int, PlayerPredictedObj> opponents;
    vector<PlayerStateCandidate> empty_vector;
    int cluster_count = 3;
    long last_updated_cycle = -1;
    long last_update_stopped = 0;
    GameMode::Type last_updated_game_mode = GameMode::Type::TimeOver;

    static CyrusDenoiser *instance;

    static CyrusDenoiser *i();

    void update(PlayerAgent *agent);

    const vector<PlayerStateCandidate> get_cluster_means(const WorldModel &wm, SideID side, int unum);

    void debug();
};

#endif //CYRUS_DENOISING_H
