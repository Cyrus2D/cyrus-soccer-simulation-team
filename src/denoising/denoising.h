//
// Created by nader on 2023-05-15.
//

#ifndef CYRUS_DENOISING_H
#define CYRUS_DENOISING_H

#include <vector>
#include <rcsc/player/player_agent.h>
#include <iostream>
using namespace rcsc;
using namespace std;
class Denoising {
public:
    static Denoising * instance;
    static Denoising * i(){
        if (instance == nullptr){
            instance = new Denoising();
        }
        return instance;
    }

    std::vector<double> self_face_diff;
    std::vector<double> self_neck_diff;
    std::vector<double> self_body_diff;
    std::vector<double> self_pos_diff;

    void update(PlayerAgent * agent){
        if (!ServerParam::i().fullstateLeft())
            return;
        const WorldModel & wm = agent->world();
        const WorldModel & fwm = agent->fullstateWorld();

        if (wm.gameMode().type() != GameMode::PlayOn)
            return;

        if (wm.seeTime().cycle() != wm.time().cycle())
            return;
        AngleDeg face = wm.self().face();
        AngleDeg neck = wm.self().neck();
        AngleDeg body = wm.self().body();
        Vector2D pos = wm.self().pos();
        AngleDeg f_face = fwm.self().face();
        AngleDeg f_neck = fwm.self().neck();
        AngleDeg f_body = fwm.self().body();
        Vector2D fpos = fwm.self().pos();

        self_face_diff.push_back((face - f_face).abs());
        self_neck_diff.push_back((neck - f_neck).abs());
        self_body_diff.push_back((body - f_body).abs());
        self_pos_diff.push_back(pos.dist(fpos));
    }

    friend ostream &operator<<( ostream &output, const Denoising &D ) {
        output << "Self:" << endl;
        if (D.self_face_diff.size() == 0)
            return output;
        double self_face_diff_sum = 0;
        double self_face_diff_max = 0;
        double self_face_diff_avg = 0;
        for (auto & d: D.self_face_diff)
            self_face_diff_sum += d;
        self_face_diff_max = *max_element(D.self_face_diff.begin(), D.self_face_diff.end());
        self_face_diff_avg = self_face_diff_sum / double (D.self_face_diff.size());
        double self_neck_diff_sum = 0;
        double self_neck_diff_max = 0;
        double self_neck_diff_avg = 0;
        for (auto & d: D.self_neck_diff)
            self_neck_diff_sum += d;
        self_neck_diff_max = *max_element(D.self_neck_diff.begin(), D.self_neck_diff.end());
        self_neck_diff_avg = self_neck_diff_sum / double (D.self_neck_diff.size());
        double self_body_diff_sum = 0;
        double self_body_diff_max = 0;
        double self_body_diff_avg = 0;
        for (auto & d: D.self_body_diff)
            self_body_diff_sum += d;
        self_body_diff_max = *max_element(D.self_body_diff.begin(), D.self_body_diff.end());
        self_body_diff_avg = self_body_diff_sum / double (D.self_body_diff.size());

        double self_pos_diff_sum = 0;
        double self_pos_diff_max = 0;
        double self_pos_diff_avg = 0;
        for (auto & d: D.self_pos_diff)
            self_pos_diff_sum += d;
        self_pos_diff_max = *max_element(D.self_pos_diff.begin(), D.self_pos_diff.end());
        self_pos_diff_avg = self_pos_diff_sum / double (D.self_pos_diff.size());
        output << "face: count"<<D.self_face_diff.size() <<" max:"<<self_face_diff_max<<" avg:"<<self_face_diff_avg<<endl;
        output << "neck: count"<<D.self_neck_diff.size() <<" max:"<<self_neck_diff_max<<" avg:"<<self_neck_diff_avg<<endl;
        output << "body: count"<<D.self_body_diff.size() <<" max:"<<self_body_diff_max<<" avg:"<<self_body_diff_avg<<endl;
        output << "pos: count"<<D.self_pos_diff.size() <<" max:"<<self_pos_diff_max<<" avg:"<<self_pos_diff_avg<<endl;
        return output;
    }
};

class PlayerStateCandidate{
public:
    Vector2D pos;
    Vector2D vel;
    double body = -360;
    double prob = 1.0;

    PlayerStateCandidate(Vector2D pos_, Vector2D vel_=Vector2D::INVALIDATED, double body_=-360)
    {
        pos = pos_;
        vel = vel_;
        body = body_;
    }
};

class PlayerPredictedObj{
public:
    SideID side;
    int unum;
    vector<PlayerStateCandidate> candidates;
    PlayerPredictedObj (SideID side_, int unum_)
    {
        side = side_;
        unum = unum_;
    }
    PlayerPredictedObj(){}

    void update(const WorldModel & wm, const AbstractPlayerObject * p)
    {
//        if seen pos == 0
//          remove candidates
//          update old candidates
//          add new candidates
//        else
//          update old candidates
        candidates.clear();
        candidates.push_back(p->pos() - Vector2D(1,1));
    }

    void debug()
    {
        for (auto &c: candidates)
            dlog.addCircle(Logger::WORLD, c.pos, 0.1, 250, 0, 0);
    }
};

class CyrusDenoiser{
public:
    map<int, PlayerPredictedObj> teammates;
    map<int, PlayerPredictedObj> opponents;
    long last_updated_cycle = -1;
    long last_update_stopped = 0;
    GameMode::Type last_updated_game_mode = GameMode::Type::TimeOver;
    static CyrusDenoiser * instance;
    static CyrusDenoiser * i(){
        if (instance == nullptr)
            instance = new CyrusDenoiser();
        return instance;
    }

    void update(PlayerAgent * agent)
    {
        auto & wm = agent->world();
        last_updated_cycle = wm.time().cycle();
        last_update_stopped = wm.time().stopped();
        for (auto & p: wm.ourPlayers())
        {
            if (p->unum() <= 0)
                continue;
            if (teammates.find(p->unum()) == teammates.end() )
            {
                teammates.insert(make_pair(p->unum(), PlayerPredictedObj(p->side(), p->unum())));
            }
            teammates[p->unum()].update(wm, p);
        }
        for (auto & p: wm.theirPlayers())
        {
            if (p->unum() <= 0)
                continue;
            if (opponents.find(p->unum()) == opponents.end() )
            {
                opponents.insert(make_pair(p->unum(), PlayerPredictedObj(p->side(), p->unum())));
            }
            opponents[p->unum()].update(wm, p);
        }
    }

    void debug()
    {
        for (auto p: teammates)
            p.second.debug();
        for (auto p: opponents)
            p.second.debug();
    }
};
#endif //CYRUS_DENOISING_H
