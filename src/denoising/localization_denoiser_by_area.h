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

    PlayerPredictedObjArea(SideID side_, int unum_);

    PlayerPredictedObjArea();

    void generate_new_candidates(const WorldModel &wm, const PlayerObject *p);

    void check_candidates(const WorldModel &wm, const PlayerObject *p);

    void update_candidates(const WorldModel &wm, const PlayerObject *p);

    void update(const WorldModel &wm, const PlayerObject *p, int cluster_count);

    void debug();

    rcsc::Vector2D area_avg();
};

class LocalizationDenoiserByArea: public LocalizationDenoiser{
public:
    PlayerPredictions * create_prediction(SideID side, int unum) override;
    std::string get_model_name() override;

};


#endif //CYRUS_DENOISING_H