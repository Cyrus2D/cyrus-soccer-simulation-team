//
// Created by nader on 2022-05-19.
//

#ifndef SRC_NEXT_PASS_PREDICTOR_H
#define SRC_NEXT_PASS_PREDICTOR_H
#include "../move_off/bhv_unmark.h"


class NextPassPredictor {
public:
    static DeepNueralNetwork * pass_prediction;
    static void load_dnn();
    vector<pass_prob> predict_pass(vector<double> & features, vector<int> ignored_player, int kicker);
    vector<Vector2D> nextReceiverCandidates(const WorldModel &wm);
    int next_receiver(const WorldModel &wm);
    bool can_see_position(PlayerAgent * agent, const WorldModel & wm, Vector2D & target, double neck_angle=-360);
    bool pass_predictor_neck(rcsc::PlayerAgent * agent);
};
#endif //SRC_NEXT_PASS_PREDICTOR_H
