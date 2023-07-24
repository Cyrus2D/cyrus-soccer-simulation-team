#include "localization_denoiser_by_dnn.h"

PlayerPredictedDNN::PlayerPredictedDNN(SideID side_, int unum_)
    : PlayerPredictions(side_, unum_) {
    model = DeepNueralNetwork();
    model.ReadFromKeras("denoise-model.txt");
}

void
PlayerPredictedDNN::update(const WorldModel &wm, const PlayerObject *p, int cluster_count) {
    if (p->posCount() == 0){
        update_history(wm);
        average_pos = p->pos();
        return;
    }

    predict_by_dnn(wm, p);
    update_history(wm);
}

void
PlayerPredictedDNN::predict_by_dnn(const WorldModel& wm, const PlayerObject * p){
    MatrixXd input(history.size(),1); // 463 12
    for (int i = 0; i < history.size(); i += 1){
        input(i ,0) = history[i];
    }

    model.Calculate(input);

    int index = p->unum(); 
    // 1 -> 0, 1
    // 2 -> 2, 3
    // 3 -> 4, 5
    
    index *= 2;
    average_pos = Vector2D(model.mOutput(index - 2) * 52.5, model.mOutput(index - 1) * 34.);
}

void
PlayerPredictedDNN::update_history(const WorldModel & wm){
    history.clear();

    const BallObject& ball = wm.ball();

    // Ball
    history.push_back(ball.pos().x / 52.5);
    history.push_back(ball.pos().y / 34.);
    history.push_back(double(ball.posCount()) / 30.);
    history.push_back(ball.vel().x / 3.);
    history.push_back(ball.vel().y / 3.);
    history.push_back(double(ball.velCount()) / 30.);
    history.push_back(0.); 
    history.push_back(0.);

    for (int i = 1; i <= 11; i++){
        for (const auto player_list: {&WorldModel::ourPlayer, &WorldModel::theirPlayer}){
            const AbstractPlayerObject* p = (wm.*player_list)(i);

            if (p){
                history.push_back(p->pos().x / 52.5);
                history.push_back(p->pos().y / 34.);
                history.push_back(double(p->posCount()) / 30.);
                history.push_back(p->vel().x / 3.);
                history.push_back(p->vel().y / 3.);
                history.push_back(double(p->velCount()) / 30.);
                history.push_back(p->body().degree() / 180.); 
                history.push_back(double(p->bodyCount()) / 30.);
            }
            else {
                history.push_back(-2.);
                history.push_back(-2.);
                history.push_back(-2.);
                history.push_back(-2.);
                history.push_back(-2.);
                history.push_back(-2.);
                history.push_back(-2.);
                history.push_back(-2.);
            }
        }
    }
}