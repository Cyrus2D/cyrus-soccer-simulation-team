//
// Created by nader on 2023-05-15.
//

#ifndef CYRUS_LOCALIZATION_DENOISER_BY_ACTION_H
#define CYRUS_LOCALIZATION_DENOISER_BY_ACTION_H

#include <iostream>
#include <vector>
#include <map>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/object_table.h>

using namespace rcsc;
using namespace std;


class PlayerStateCandidate {
public:
    Vector2D pos;
    Vector2D vel;
    double body = -360;
    double prob = 1.0;
    bool last_action_is_turn = false;

    PlayerStateCandidate(Vector2D pos_, Vector2D vel_ = Vector2D::INVALIDATED, double body_ = -360);

    PlayerStateCandidate gen_random_next_by_dash(const WorldModel &wm, const PlayerObject *p) const;

    PlayerStateCandidate gen_random_next_by_turn(const WorldModel &wm, const PlayerObject *p) const;

    PlayerStateCandidate gen_random_next_by_nothing(const WorldModel &wm, const PlayerObject *p) const;

    PlayerStateCandidate gen_random_next(const WorldModel &wm, const PlayerObject *p) const;

    vector<PlayerStateCandidate> gen_max_next_candidates(const WorldModel &wm, const PlayerObject *p) const;

    bool is_close(const PlayerStateCandidate & other) const{
        if (pos.dist(other.pos) < 0.2)
            if (AngleDeg(body - other.body).abs() < 30.0)
                if ((vel - other.vel).r() < 0.2)
                    return true;
        return false;
    }
};


class PlayerPredictions {
public:
    SideID side;
    int unum{};
    vector<PlayerStateCandidate> candidates;
    vector<PlayerStateCandidate> candidates_removed_by_similarity;
    vector<PlayerStateCandidate> candidates_removed_by_filtering;
    vector<PlayerStateCandidate> candidates_removed_by_updating;
    vector<PlayerStateCandidate> candidates_means;
    ObjectTable object_table;
    Vector2D average_pos;
    ulong max_candidates_size = 1000;

    PlayerPredictions(SideID side_, int unum_);

    PlayerPredictions();

    void generate_new_candidates(const WorldModel &wm, const PlayerObject *p);

    void filter_candidates(const WorldModel &wm, const PlayerObject *p);

    void update_candidates(const WorldModel &wm, const PlayerObject *p);

    void update(const WorldModel &wm, const PlayerObject *p, int cluster_count);

    void clustering(int cluster_count);

    void remove_similar_candidates();

    void debug();
};

class LocalizationDenoiserByAction {
public:
    map<int, PlayerPredictions> teammates;
    map<int, PlayerPredictions> opponents;
    vector<PlayerStateCandidate> empty_vector;
    int cluster_count = 3;
    long last_updated_cycle = -1;
    long last_update_stopped = 0;
    GameMode::Type last_updated_game_mode = GameMode::Type::TimeOver;

    static LocalizationDenoiserByAction *instance;

    static LocalizationDenoiserByAction *i();

    void update(PlayerAgent *agent);

    void update_tests(PlayerAgent *agent);

    vector<PlayerStateCandidate> get_cluster_means(const WorldModel &wm, SideID side, int unum);

    void debug();
};

#endif //CYRUS_LOCALIZATION_DENOISER_BY_ACTION_H
