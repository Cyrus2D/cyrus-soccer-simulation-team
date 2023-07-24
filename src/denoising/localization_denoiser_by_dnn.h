//
// Created by nader on 2023-05-15.
//

#ifndef CYRUS_DENOISING_DNN_H
#define CYRUS_DENOISING_DNN_H

#include <iostream>
#include <vector>
#include <map>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/object_table.h>
#include <rcsc/geom/convex_hull.h>
#include "localization_denoiser.h"
#include <CppDNN/DeepNueralNetwork.h>

using namespace rcsc;
using namespace std;


class PlayerPredictedDNN : public PlayerPredictions{
public:
    DeepNueralNetwork model;
    std::vector<double> history;


    PlayerPredictedDNN(SideID side_, int unum_);

    void update(const WorldModel &wm, const PlayerObject *p, int cluster_count);
    void update_history(const rcsc::WorldModel & wm);
    void predict_by_dnn(const rcsc::WorldModel & wm, const rcsc::PlayerObject * p);

    void debug();

    rcsc::Vector2D vertices_avg();
    rcsc::Vector2D get_avg();
};

class LocalizationDenoiserByDNN: public LocalizationDenoiser{
public:
    PlayerPredictions* create_prediction(SideID side, int unum) override;
    std::string get_model_name() override;

};



#endif //CYRUS_DENOISING_DNN_H