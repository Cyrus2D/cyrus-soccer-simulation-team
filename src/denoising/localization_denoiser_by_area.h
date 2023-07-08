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
#include "localization_denoiser.h"

using namespace rcsc;
using namespace std;

Polygon2D mutual_convex(const Polygon2D &p1p, const Polygon2D &p2p);
void draw_poly(const Polygon2D &p, const char* color);

class PlayerPositionConvex {
public:
    std::vector<std::vector<ConvexHull *>> convexes_with_body;
    std::vector<std::vector<ConvexHull *>> convexes_without_body;

    PlayerPositionConvex()
            : convexes_with_body(),
              convexes_without_body()
              {}

    void init();
    ConvexHull* get_convex_with_body(int ptype_id, int pos_count, Vector2D center, AngleDeg rotation);
    ConvexHull* get_convex_without_body(int ptype_id, int pos_count, Vector2D center);

};



class PlayerStateCandidateArea {
public:
    Vector2D pos;
    int cycle;

    PlayerStateCandidateArea(Vector2D pos_);
};


class PlayerPredictedObjArea : public PlayerPredictions{
public:
    PlayerPositionConvex player_data;
    Polygon2D* area;
    GameTime last_seen_time;
    double last_body;
    bool body_valid;

    PlayerPredictedObjArea(SideID side_, int unum_);

    PlayerPredictedObjArea();

    void generate_new_candidates(const WorldModel &wm, const PlayerObject *p);

    void check_candidates(const WorldModel &wm, const PlayerObject *p);

    void update_candidates(const WorldModel &wm, const PlayerObject *p);
    void update_by_hear(const rcsc::WorldModel& wm, const rcsc::PlayerObject* p);

    void update(const WorldModel &wm, const PlayerObject *p, int cluster_count);

    void debug();

    rcsc::Vector2D vertices_avg();
    rcsc::Vector2D get_avg();
};

class BallPredictionArea: public BallPrediction {
public:
    rcsc::Polygon2D* area;
    rcsc::GameTime last_seen_time;
    rcsc::Vector2D last_vel;
    rcsc::Vector2D last_vel_err;

    BallPredictionArea();
    void update(const rcsc::WorldModel& wm, const int cluster_count) override;
    void update_by_see(const rcsc::WorldModel& wm);
    void update_by_hear(const rcsc::WorldModel& wm);
    
    rcsc::Vector2D vertices_avg();
    rcsc::Vector2D get_avg();
};

class LocalizationDenoiserByArea: public LocalizationDenoiser{
public:
    PlayerPredictions* create_prediction(SideID side, int unum) override;
    BallPrediction* create_ball_prediction() override;
    std::string get_model_name() override;

};


#endif //CYRUS_DENOISING_H