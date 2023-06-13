//
// Created by nader on 2023-05-15.
//

#ifndef CYRUS_LOCALIZATION_DENOISER_H
#define CYRUS_LOCALIZATION_DENOISER_H

#include <iostream>
#include <vector>
#include <map>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/object_table.h>

using namespace rcsc;
using namespace std;


class PlayerPredictions {
public:
    SideID side;
    int unum{};
    Vector2D average_pos;

    ObjectTable object_table;

    PlayerPredictions(SideID side_, int unum_);

    PlayerPredictions();

    virtual void update(const WorldModel &wm, const PlayerObject *p, int cluster_count);

    bool player_heard(const WorldModel & wm, const AbstractPlayerObject * p);

    bool player_seen(const AbstractPlayerObject * p);
};


class LocalizationDenoiser {
public:
    map<int, PlayerPredictions *> teammates;
    map<int, PlayerPredictions *> opponents;
    int cluster_count = 1;
    long last_updated_cycle = -1;
    long last_update_stopped = 0;

    void update(PlayerAgent *agent);

    void update_tests(PlayerAgent *agent);

    void update_world_model(PlayerAgent * agent);

    void debug(PlayerAgent * agent);

    Vector2D get_average_pos(const WorldModel &wm, SideID side, int unum);

    virtual PlayerPredictions* create_prediction(SideID side, int unum);
};

#endif //CYRUS_LOCALIZATION_DENOISER_H
