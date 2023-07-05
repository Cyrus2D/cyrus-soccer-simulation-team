//
// Created by nader on 2023-05-15.
//
#include <iostream>
#include <random>
#include <vector>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/object_table.h>
#include <rcsc/common/audio_memory.h>
#include "localization_denoiser.h"
#include "../dkm/dkm.hpp"
#include "../debugs.h"
#include "../sample_player.h"

using namespace rcsc;
using namespace std;

static std::default_random_engine gen;

PlayerPredictions::PlayerPredictions(SideID side_, int unum_)
        : object_table() {
    side = side_;
    unum = unum_;
}

PlayerPredictions::PlayerPredictions() {
}

bool PlayerPredictions::player_heard(const WorldModel & wm, const AbstractPlayerObject * p){
    auto memory = wm.audioMemory().player();
    if (wm.audioMemory().playerTime().cycle() != wm.time().cycle())
        return false;
    for (auto & a: memory){
        int unum = a.unum_;
        if (p->side() != wm.ourSide())
            unum -= 11;
        if (unum == p->unum()){
            return true;
        }
    }
    return false;
}

bool PlayerPredictions::player_seen(const AbstractPlayerObject * p){
    return p->seenPosCount() == 0;
}

void PlayerPredictions::update(const WorldModel &wm, const PlayerObject *p, int cluster_count) {
}

std::string
LocalizationDenoiser::get_model_name(){
    return "Abstract";
}

void LocalizationDenoiser::update_tests(PlayerAgent *agent){
    if (!ServerParam::i().fullstateLeft())
        return;

    static bool first_time = true;
    if (first_time){
        char buffer[80];
        time_t rawtime;
        struct tm *timeinfo;
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(buffer, sizeof(buffer), "%Y-%m-%d-%H-%M-%S", timeinfo);
        std::string time_str(buffer);
        
        std::string file_name = "/home/aref/data/DT-" 
                                + get_model_name() + '-'
                                + std::to_string(agent->world().self().unum()) + "-"
                                + std::to_string(SamplePlayer::player_port) + "-"
                                + time_str;
        fout = std::ofstream(file_name.c_str());
        first_time = false;
    }


    auto &wm = agent->world();
    struct PlayerTestRes {
        double count = 0.0;
        double base_noise = 0.0;
        double cyrus_noise = 0.0;
    };
    static vector<PlayerTestRes> player_test_res(23, PlayerTestRes());
    for (auto & p: teammates){
        auto t = wm.ourPlayer(p.first);
        if (t != nullptr
            && (p.second->player_heard(wm, t)
                || p.second->player_seen(t))
            && p.second->average_pos.isValid()){
            auto avg_pos = get_average_pos(wm, p.second->side, p.second->unum);
            Vector2D full_pos = agent->fullstateWorld().ourPlayer(p.first)->pos();
            Vector2D pos = wm.ourPlayer(p.first)->pos();

            if (avg_pos.isValid()){
                Vector2D cyrus_pos = avg_pos;
                player_test_res.at(p.first).count += 1;
                player_test_res.at(p.first).base_noise += full_pos.dist(pos);
                player_test_res.at(p.first).cyrus_noise += cyrus_pos.dist(full_pos);
            }
        }
    }
    for (auto & p: opponents){
        auto o = wm.theirPlayer(p.first);
        if (o != nullptr
            && (p.second->player_heard(wm, o)
                || p.second->player_seen(o))
            && p.second->average_pos.isValid()){
            auto avg_pos = get_average_pos(wm, p.second->side, p.second->unum);
            Vector2D full_pos = agent->fullstateWorld().theirPlayer(p.first)->pos();
            Vector2D pos = wm.theirPlayer(p.first)->pos();

            if (avg_pos.isValid()){
                Vector2D cyrus_pos = avg_pos;
                player_test_res.at(p.first + 11).count += 1;
                player_test_res.at(p.first + 11).base_noise += full_pos.dist(pos);
                player_test_res.at(p.first + 11).cyrus_noise += cyrus_pos.dist(full_pos);
            }
        }
    }
    if ((wm.time().cycle() == 2999 || wm.time().cycle() == 5999 || wm.time().cycle() == 599) && wm.time().stopped() == 0)
    {
        double base_noises = 0.0;
        double cyrus_noises = 0.0;
        double all_count = 0.0;
        for (int i = 1; i <= 22; i++)
        {
            auto & res = player_test_res.at(i);
            if (res.count > 0)
            {
                fout<<"side "<<(i <= 11 ? "T " : "O ")<<(i <= 11 ? i : i - 11)<<" "<<res.base_noise / res.count <<" -> "<<res.cyrus_noise / res.count<<endl;
                base_noises += res.base_noise;
                cyrus_noises += res.cyrus_noise;
                all_count += res.count;
            }
        }
        if (all_count > 0)
            fout<<"end "<<base_noises / all_count<<" -> "<<cyrus_noises / all_count<<endl;
    }

}

void LocalizationDenoiser::update_world_model(PlayerAgent * agent){
//    update world model!!
    auto &wm = agent->world();
    auto & wm_not_const = agent->world_not_const();
    for (auto &p: wm_not_const.M_teammates_from_self) {
        if (p == nullptr)
            continue;
        if (p->unum() <= 0)
            continue;
        if(teammates[p->unum()]->player_seen(p) || teammates[p->unum()]->player_heard(wm, p))
            if (teammates[p->unum()]->average_pos.isValid()){
                p->M_base_pos = p->M_pos;
                p->M_pos = teammates[p->unum()]->average_pos;
            }
    }
    for (auto &p: wm_not_const.M_opponents_from_self) {
        if (p == nullptr)
            continue;
        if (p->unum() <= 0)
            continue;
        if(opponents[p->unum()]->player_seen(p) || opponents[p->unum()]->player_heard(wm, p))
            if (opponents[p->unum()]->average_pos.isValid()){
                p->M_base_pos = p->M_pos;
                p->M_pos = opponents[p->unum()]->average_pos;
            }
    }
}

void LocalizationDenoiser::update(PlayerAgent *agent) {
    auto &wm = agent->world();
    if (wm.time().cycle() == last_updated_cycle && wm.time().stopped() == last_update_stopped)
        return;
    if (wm.gameMode().type() != GameMode::PlayOn){
        return;
    }

    last_updated_cycle = wm.time().cycle();
    last_update_stopped = wm.time().stopped();
    for (auto &p: wm.teammates()) {
        if (p == nullptr)
            continue;
        if (p->unum() <= 0)
            continue;
        if (teammates.find(p->unum()) == teammates.end()) {
            teammates.insert(make_pair(p->unum(), create_prediction(p->side(), p->unum())));
        }
        teammates[p->unum()]->update(wm, p, cluster_count);
    }
    for (auto &p: wm.opponents()) {
        if (p == nullptr)
            continue;
        if (p->unum() <= 0)
            continue;
        if (opponents.find(p->unum()) == opponents.end()) {
            opponents.insert(make_pair(p->unum(), create_prediction(p->side(), p->unum())));
        }
            opponents[p->unum()]->update(wm, p, cluster_count);
    }
    #ifdef DEBUG_ACTION_DENOISER
    update_tests(agent);
    #endif
    update_world_model(agent);
}

Vector2D LocalizationDenoiser::get_average_pos(const WorldModel &wm, SideID side, int unum){
    auto &players_list = (wm.self().side() == side ? teammates : opponents);
    if (players_list.find(unum) == players_list.end()) {
        return Vector2D::INVALIDATED;
    }
    return players_list.at(unum)->average_pos;
}

void LocalizationDenoiser::debug(PlayerAgent * agent) {
    #ifdef DEBUG_ACTION_DENOISER
    for (auto & p: agent->world().allPlayers()){
        dlog.addCircle(Logger::WORLD, p->M_base_pos, 0.1, 0, 0, 255, true);
        dlog.addCircle(Logger::WORLD, p->pos(), 0.1, 255, 0, 0, true);
    }
    #endif
}

PlayerPredictions* LocalizationDenoiser::create_prediction(SideID side, int unum) {
    return new PlayerPredictions(side, unum);
}