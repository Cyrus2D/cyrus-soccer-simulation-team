#include "localization_denoiser_by_dnn.h"

// #define COUT_DEBUG

#ifndef COUT_DEBUG
#define dd(x) ;
#else
#define dd(x) std::cout << #x << std::endl
#endif

PlayerPredictedDNN::PlayerPredictedDNN(SideID side_, int unum_)
    : PlayerPredictions(side_, unum_) {
    model = DeepNueralNetwork();
    model.ReadFromKeras("denoise-model.txt");
    history = std::vector<double>();
}

void
PlayerPredictedDNN::update(const WorldModel &wm, const PlayerObject *p, int cluster_count) {
    dd(A);
    if (p->posCount() == 0){
        update_history(wm);
        average_pos = p->pos();
        return;
    }
    dd(B);
    if (p->side() == wm.theirSide())
        predict_by_dnn(wm, p);
    else
        average_pos = p->pos();
    dd(C);
    update_history(wm);
}

void
PlayerPredictedDNN::predict_by_dnn(const WorldModel& wm, const PlayerObject * p){
    if (history.size() == 0){
        average_pos = p->pos();
        return;
    }

    dd(g);
    MatrixXd input(history.size(),1);
    dd(h);
    for (int i = 0; i < history.size(); i += 1){
        input(i ,0) = history[i];
    }

    dd(i);
    model.Calculate(input);
    dd(j);

    int index = p->unum(); 
    // 1 -> 0, 1
    // 2 -> 2, 3
    // 3 -> 4, 5
    
    dd(k);
    index *= 2;
    average_pos = Vector2D(model.mOutput(index - 2) * 52.5, model.mOutput(index - 1) * 34.);
    dd(l);

    dlog.addText(Logger::WORLD, "history size: %d", history.size());
    dlog.addText(Logger::WORLD, "p(%d) -> (%.2f, %.2f)", unum, average_pos.x, average_pos.y);
}

void
PlayerPredictedDNN::update_history(const WorldModel & wm){
    std::cout << "###################c=" << wm.time().cycle() << std::endl;
    history.clear();
    dd(d);

    const BallObject& ball = wm.ball();

    // Ball
    history.push_back(ball.M_base_pos.x / 52.5);
    history.push_back(ball.M_base_pos.y / 34.);
    history.push_back(double(ball.posCount()) / 30.);
    history.push_back(ball.vel().x / 3.);
    history.push_back(ball.vel().y / 3.);
    history.push_back(double(ball.velCount()) / 30.);
    history.push_back(0.); 
    history.push_back(0.);

    dd(e);
    for (int i = 1; i <= 11; i++){
        for (const auto player_list: {&WorldModel::ourPlayer, &WorldModel::theirPlayer}){
            const AbstractPlayerObject* p = (wm.*player_list)(i);

            if (p){
                history.push_back(p->M_base_pos.x / 52.5);
                history.push_back(p->M_base_pos.y / 34.);
                history.push_back(double(p->posCount()) / 30.);
                history.push_back(p->vel().x / 3.);
                history.push_back(p->vel().y / 3.);
                history.push_back(double(p->velCount()) / 30.);
                history.push_back(p->body().degree() / 180.); 
                history.push_back(double(p->bodyCount()) / 30.);
                std::cout << "u(" << i << ")" << p->M_base_pos.x / 52.5 << std::endl;
                std::cout << "u(" << i << ")" << p->M_base_pos.x << std::endl;
                std::cout << "u(" << i << ")" << p->M_base_pos.y / 34. << std::endl;
                std::cout << "u(" << i << ")" << p->M_base_pos.y << std::endl;
                std::cout << "u(" << i << ")" << double(p->posCount()) / 30. << std::endl;
                std::cout << "u(" << i << ")" << double(p->posCount()) << std::endl;
                std::cout << "u(" << i << ")" << p->vel().x / 3. << std::endl;
                std::cout << "u(" << i << ")" << p->vel().x << std::endl;
                std::cout << "u(" << i << ")" << p->vel().y / 3. << std::endl;
                std::cout << "u(" << i << ")" << p->vel().y << std::endl;
                std::cout << "u(" << i << ")" << double(p->velCount()) / 30. << std::endl;
                std::cout << "u(" << i << ")" << double(p->velCount()) << std::endl;
                std::cout << "u(" << i << ")" << p->body().degree() / 180. << std::endl;
                std::cout << "u(" << i << ")" << p->body().degree() << std::endl;
                std::cout << "u(" << i << ")" << double(p->bodyCount()) / 30. << std::endl;
                std::cout << "u(" << i << ")" << double(p->bodyCount()) << std::endl;
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
    dd(f);
}

PlayerPredictions *LocalizationDenoiserByDNN::create_prediction(SideID side, int unum)
{
    return new PlayerPredictedDNN(side, unum);
}

std::string LocalizationDenoiserByDNN::get_model_name()
{
    return "DNN";
}
