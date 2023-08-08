#include "localization_denoiser_by_dnn.h"
#include "localization_denoiser_by_area.h"

#define COUT_DEBUG

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
    average_pos = p->pos();
    if (wm.self().unum() != 9)
        return;
    if(p->unum() != 5)
        return;
    
    if (p->posCount() == 0){
        average_pos = p->pos();
        return;
    }
    if (p->side() == wm.theirSide()) {
        update_history(wm);
        predict_by_dnn(wm, p);
    }
    dd(C);
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
    // model.mOutput(i - 2)

    std::vector<Vector2D> vertices;
    const double r = 10;
    const int labels = 20;
    for (int i = 0; i < labels*labels; i++) {
        double v = model.mOutput(i);
        
        int ix = i % 20;
        int iy = i / 20;

        ix -= 10;
        iy -= 10;

        double x = double(ix)/double(labels)*r + p->pos().x;
        double y = double(iy)/double(labels)*r + p->pos().y;
        
        if (v > 0.05){
            dlog.addCircle(Logger::WORLD, Vector2D(x, y), 0.1, "#FFFFFF", true);
            vertices.emplace_back(x, y);
        }else if (v > 0.01){
            dlog.addCircle(Logger::WORLD, Vector2D(x, y), 0.1, "#000000", true);
        }else if (v > 0.0005){
            dlog.addCircle(Logger::WORLD, Vector2D(x, y), 0.1, "#0000FF", true);
        }
    }
    dd(ja);

    ConvexHull area(vertices);
    dd(jb);
    area.compute();
    dd(jc);
    draw_poly(area.toPolygon(), "#000000");
    dd(jd);
}

void
PlayerPredictedDNN::update_history(const WorldModel & wm){
    std::cout << "###################c=" << wm.time().cycle() << std::endl;
    history.clear();
    dd(d);

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

    // std::cout << "u(b):" << ball.pos().x << std::endl;
    // std::cout << "u(b):" << ball.pos().y << std::endl;
    // std::cout << "u(b):" << double(ball.posCount()) << std::endl;
    // std::cout << "u(b):" << ball.vel().x << std::endl;
    // std::cout << "u(b):" << ball.vel().y << std::endl;
    // std::cout << "u(b):" << double(ball.velCount()) << std::endl;
    // std::cout << "u(b):" << 0 << std::endl;
    // std::cout << "u(b):" << 0 << std::endl;


    dd(e);
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
                // std::cout << "u(" << i << "):" << p->pos().x / 52.5 << std::endl;
                // std::cout << "u(" << i << "):" << p->pos().x << std::endl;
                // std::cout << "u(" << i << "):" << p->pos().y / 34. << std::endl;
                // std::cout << "u(" << i << "):" << p->pos().y << std::endl;
                // std::cout << "u(" << i << "):" << double(p->posCount()) / 30. << std::endl;
                // std::cout << "u(" << i << "):" << double(p->posCount()) << std::endl;
                // std::cout << "u(" << i << "):" << p->vel().x / 3. << std::endl;
                // std::cout << "u(" << i << "):" << p->vel().x << std::endl;
                // std::cout << "u(" << i << "):" << p->vel().y / 3. << std::endl;
                // std::cout << "u(" << i << "):" << p->vel().y << std::endl;
                // std::cout << "u(" << i << "):" << double(p->velCount()) / 30. << std::endl;
                // std::cout << "u(" << i << "):" << double(p->velCount()) << std::endl;
                // std::cout << "u(" << i << "):" << p->body().degree() / 180. << std::endl;
                // std::cout << "u(" << i << "):" << p->body().degree() << std::endl;
                // std::cout << "u(" << i << "):" << double(p->bodyCount()) / 30. << std::endl;
                // std::cout << "u(" << i << "):" << double(p->bodyCount()) << std::endl;
                // std::cout << "-----------------------------------------------" << std::endl;
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
