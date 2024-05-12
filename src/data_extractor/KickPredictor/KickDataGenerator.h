//
// Created by nader on 5/12/24.
//

#ifndef CYRUS_KICKDATAGENERATOR_H
#define CYRUS_KICKDATAGENERATOR_H

#include <vector>
#include <map>
#include <rcsc/geom.h>
#include <rcsc/player/player_agent.h>

using namespace std;
using namespace rcsc;

class KickDataGeneratorObservedInterface {
public:
    virtual vector<double> getFeatures() = 0;
};

class KickDataGeneratorObservedPlayer: public KickDataGeneratorObservedInterface {
public:
    Vector2D pos;
    Vector2D vel;
    AngleDeg bodyDir;
    SideID side;
    int unum;
    bool isKickable;
    bool isValid;

    KickDataGeneratorObservedPlayer() :
        pos(Vector2D::INVALIDATED),
        vel(Vector2D::INVALIDATED),
        bodyDir(0),
        side(NEUTRAL),
        unum(-1),
        isKickable(false),
        isValid(false) {}

    KickDataGeneratorObservedPlayer(const AbstractPlayerObject * p){
        if (p == nullptr) {
            isValid = false;
            return;
        }
        if (p->unum() <= 0) {
            isValid = false;
            return;
        }
        isValid = true;
        pos = p->pos();
        vel = p->vel();
        bodyDir = p->body();
        side = p->side();
        unum = p->unum();
        isKickable = p->ballReachStep() == 0;
    }

    vector<double> getFeatures() override {
        vector<double> features;
        features.push_back(isValid ? pos.x / 52.5 : -2);
        features.push_back(isValid ? pos.y / 34.0 : -2);
        features.push_back(isValid ? vel.x / 3.0 : -2);
        features.push_back(isValid ? vel.y / 3.0 : -2);
        features.push_back(isValid ? bodyDir.degree() / 180.0 : -2);
        features.push_back(isValid ? side : -2);
        features.push_back(isValid ? unum / 11.0 : -2);
        features.push_back(isValid ? isKickable : -2);
        return features;
    }
};

class KickDataGeneratorBall: public KickDataGeneratorObservedInterface{
public:
    Vector2D pos;
    Vector2D vel;
    bool isValid;

    KickDataGeneratorBall() : pos(Vector2D::INVALIDATED), vel(Vector2D::INVALIDATED), isValid(false) {}

    KickDataGeneratorBall(const BallObject * p){
        if (p == nullptr) {
            isValid = false;
            return;
        }
        isValid = true;
        pos = p->pos();
        vel = p->vel();
    }

    vector<double> getFeatures() override {
        vector<double> features;
        features.push_back(isValid ? pos.x / 52.5 : -2);
        features.push_back(isValid ? pos.y / 34.0 : -2);
        features.push_back(isValid ? vel.x / 3.0 : -2);
        features.push_back(isValid ? vel.y / 3.0 : -2);
        return features;
    }
};

class
class KickDataGeneratorCycleData {
public:
    map<int, KickDataGeneratorObservedInterface *> objects_map;


class KickDataGenerator {

};


#endif //CYRUS_KICKDATAGENERATOR_H
