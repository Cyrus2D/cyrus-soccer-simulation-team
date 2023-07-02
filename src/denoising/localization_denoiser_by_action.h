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
#include "localization_denoiser.h"

using namespace rcsc;
using namespace std;


class PlayerStateCandidate {
public:
    Vector2D pos;
    Vector2D vel;
    double body = -360;
    double prob = 1.0;
    bool last_action_is_turn = false;
    double tmp_dist = 0;

    PlayerStateCandidate(Vector2D pos_, Vector2D vel_ = Vector2D::INVALIDATED, double body_ = -360);

    PlayerStateCandidate gen_random_next_by_dash(const WorldModel &wm, const PlayerObject *p) const;

    PlayerStateCandidate gen_random_next_by_turn(const WorldModel &wm, const PlayerObject *p) const;

    PlayerStateCandidate gen_random_next_by_nothing(const WorldModel &wm, const PlayerObject *p) const;

    PlayerStateCandidate gen_random_next(const WorldModel &wm, const PlayerObject *p) const;

    void gen_max_next_candidates(const WorldModel &wm, const PlayerObject *p, vector<PlayerStateCandidate> & vec) const;

    bool is_close(const PlayerStateCandidate & other) const{
        if (abs(pos.x - other.pos.x) < 0.2)
            if (abs(pos.y - other.pos.y) < 0.2)
                if (pos.dist(other.pos) < 0.2)
                    if (AngleDeg(body - other.body).abs() < 30.0)
                        if ((vel - other.vel).r() < 0.2)
                            return true;
        return false;
    }
};


class PlayerPredictionsByAction : public PlayerPredictions{
public:
    vector<PlayerStateCandidate> candidates;
    vector<PlayerStateCandidate> candidates_removed_by_similarity;
    vector<PlayerStateCandidate> candidates_removed_by_filtering;
    vector<PlayerStateCandidate> candidates_removed_by_updating;
    vector<PlayerStateCandidate> candidates_means;
    ulong max_candidates_size = 500;

    PlayerPredictionsByAction(SideID side_, int unum_):
            PlayerPredictions(side_, unum_){}

    void generate_new_candidates_by_see(const WorldModel &wm, const PlayerObject *p);

    void generate_new_candidates_by_hear(const WorldModel &wm, const PlayerObject *p);

    void filter_candidates_by_see(const WorldModel &wm, const PlayerObject *p);

    void filter_candidates_by_hear(const WorldModel &wm, const PlayerObject *p);

    void update_candidates(const WorldModel &wm, const PlayerObject *p);

    void update(const WorldModel &wm, const PlayerObject *p, int cluster_count) override;

    void clustering(int cluster_count);

    void remove_similar_candidates();

    void debug();
};

class LocalizationDenoiserByAction: public LocalizationDenoiser{
public:
    PlayerPredictions * create_prediction(SideID side, int unum) override;
    std::string get_model_name() override;

};

#endif //CYRUS_LOCALIZATION_DENOISER_BY_ACTION_H
