//
// Created by nader on 2023-05-15.
//
#include <iostream>
#include <random>
#include <vector>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/object_table.h>
#include <rcsc/common/audio_memory.h>
#include "localization_denoiser_by_action.h"
#include "../dkm/dkm.hpp"
#include "../debugs.h"


using namespace rcsc;
using namespace std;


static std::default_random_engine gen;
typedef vector<PlayerStateCandidate> V_PSC;
typedef PlayerStateCandidate PSC;
typedef LocalizationDenoiserByAction LDA;

double LDA::t1 = 0.0;
double LDA::t2 = 0.0;
double LDA::t3 = 0.0;
double LDA::t4 = 0.0;
double LDA::t5 = 0.0;
double LDA::t6 = 0.0;
double LDA::t7 = 0.0;
double LDA::t8 = 0.0;
double LDA::t9 = 0.0;
double LDA::t10 = 0.0;

double get_random(double min = 0.0, double max = 1.0) {
    static std::uniform_real_distribution<double> dis(0.0, 1.0);
    return dis(gen) * (max - min) + min;
}

double get_random_normal(double mean, double std_dev, double min = 0.0, double max = 1.0) {
    static std::normal_distribution<double> dis(mean, std_dev); // range [0, 1)
    while (true) {
        double rnd = dis(gen);
        if (rnd >= min && rnd <= max)
            return rnd;
    }
    return 0.0;
}

vector<double> get_range(double min, double max, double step){
    int size = static_cast<int>((max - min) / step);
    vector<double> res(size);
    double i = 0.0;
    for (auto& elem : res)
    {
        elem = min + i * step;
        i += 1.0;
    }
    return res;
}

PSC::PlayerStateCandidate(Vector2D pos_, Vector2D vel_, double body_) {
    pos = pos_;
    vel = vel_;
    body = body_;
}

PSC
PSC::gen_random_next_by_dash(const WorldModel &wm, const PlayerObject *p) const {
    const ServerParam &SP = ServerParam::i();
    auto p_type = p->playerTypePtr();
    double dash_dir = get_random_normal(0, 25.0, -180.0, +180.0);
    double accel_dist = SP.maxDashPower() * p_type->effortMax() * SP.dashDirRate(dash_dir) * p_type->dashPowerRate();
    Vector2D accel = Vector2D::polar2vector(accel_dist, AngleDeg(body + dash_dir));
    Vector2D move = accel + vel;
    return {pos + move, move * p_type->playerDecay(), body};
}

PSC
PSC::gen_random_next_by_turn(const WorldModel &wm, const PlayerObject *p) const {
    const ServerParam &SP = ServerParam::i();
    auto p_type = p->playerTypePtr();
    double max_turn = p_type->effectiveTurn(SP.maxMoment(), vel.r());
    double turn_moment = get_random(-max_turn, max_turn);
    return {pos + vel, vel * p_type->playerDecay(), AngleDeg(body + turn_moment).degree()};
}

PSC
PSC::gen_random_next_by_nothing(const WorldModel &wm, const PlayerObject *p) const {
    auto p_type = p->playerTypePtr();
    return {pos + vel, vel * p_type->playerDecay(), body};
}

PSC
PSC::gen_random_next(const WorldModel &wm, const PlayerObject *p) const {
    double rnd = get_random(0.0, 1.0);
    if (p->isKickable())
        return gen_random_next_by_nothing(wm, p);
    if (rnd < 0.7)
        return gen_random_next_by_dash(wm, p);
    return gen_random_next_by_turn(wm, p);
}

void
PSC::gen_max_next_candidates(const WorldModel &wm, const PlayerObject *p, vector<PlayerStateCandidate> & res) const{
    auto p_type = p->playerTypePtr();
    const ServerParam &SP = ServerParam::i();
    double max_turn_moment = p_type->effectiveTurn(SP.maxMoment(), vel.r());
    if (!last_action_is_turn)
        for (int t = -4; t <= 4; t++){
            double next_body = body + t * static_cast<double>(max_turn_moment) / 4.0;
            res.emplace_back(pos + vel, vel * p_type->playerDecay(), next_body);
            res.back().last_action_is_turn = true;
        }
    static vector<double> accel_terms = {0.5, 1.0};
    for (auto & accel_term : accel_terms){
        double accel_dist = SP.maxDashPower() * p_type->effortMax() * SP.dashDirRate(0) * p_type->dashPowerRate();
        Vector2D accel = Vector2D::polar2vector(accel_dist * accel_term, AngleDeg(body));
        Vector2D move = accel + vel;
        res.emplace_back(pos + move, move * p_type->playerDecay(), body);
    }
//    return res;
}

PlayerPredictions::PlayerPredictions(SideID side_, int unum_)
        : object_table() {
    side = side_;
    unum = unum_;

}

PlayerPredictions::PlayerPredictions() {
}

void PlayerPredictions::generate_new_candidates_by_see(const WorldModel &wm, const PlayerObject *p) {
    auto p_type = p->playerTypePtr();
    const auto & self_pos = wm.self().pos();
    auto rel_pos = p->pos() - wm.self().pos();
    const double & seen_dist = p->seen_dist();
    auto seen_dir = rel_pos.th();
    double body = (p->bodyCount() == 0 ? p->body().degree(): -360);
    auto vel = (p->seenVelCount() == 0 ? p->vel() : Vector2D::INVALIDATED);
    double avg_dist;
    double dist_err;
    if (object_table.getMovableObjInfo(seen_dist,
                                       &avg_dist,
                                       &dist_err)) {
        #ifdef DEBUG_ACTION_DENOISER
        dlog.addSector(Logger::WORLD, wm.self().pos(), avg_dist - dist_err, avg_dist + dist_err,
                       seen_dir - 1.0, 2.0,
                       "#0000FF");
        dlog.addText(Logger::WORLD, "########generate new candidates");
        dlog.addText(Logger::WORLD, "$$$$ seen_dist: %.2f avg_dist: %.2f dist_err: %.2f dir: %.2f",
                     seen_dist,
                     avg_dist,
                     dist_err,
                     seen_dir);
        #endif
        candidates.clear();
        candidates.reserve(max_candidates_size);
        static vector<double> dist_range = {0.0, 1.0, 2.0};
        static vector<double> dir_range = {0.0, 1.0, 2.0};
        for (auto & d: dist_range){
            for (auto & a: dir_range){
                double dist = avg_dist - dist_err + d * dist_err;
                double dir = seen_dir.degree() - 0.5 + 0.5 * a;
                Vector2D pos = self_pos + Vector2D::polar2vector(dist, dir);
                vector<AngleDeg> all_body;
                if (body != -360.0)
                    all_body.emplace_back(body);
                else
                    for (auto & body_tmp: get_range(-180.0, 180.0, 45.0))
                        all_body.emplace_back(body_tmp);
                vector<Vector2D> all_vel;
                if (vel.isValid())
                    all_vel.push_back(vel);
                else
                {
                    all_vel.emplace_back(0, 0);
                    for (auto & body_tmp: get_range(-180.0, 180.0, 45.0))
                        all_vel.push_back(Vector2D::polar2vector(p_type->realSpeedMax()*p_type->playerDecay(), body_tmp));
                }
                for (auto & b: all_body){
                    for (auto & v: all_vel){
                        if (vel.isValid() || v.r() == 0 || (v.th() - b).abs() < 10)
                            candidates.emplace_back(pos, v, b.degree());
                    }
                }
            }
        }
    }
    if (!candidates_removed_by_filtering.empty()){
        int choose_count = candidates.size() / 2;
        V_PSC tmp;

        for (auto & c: candidates){
            c.tmp_dist = 1000;
            for (auto & f: candidates_removed_by_filtering){
                c.tmp_dist = min(c.tmp_dist, c.pos.dist(f.pos));
            }
        }
        sort(candidates.begin(), candidates.end(),
             [](const PSC & a, const PSC & b) -> bool
             {
                 return a.tmp_dist < b.tmp_dist;
             });
        for (int i = 0; i < choose_count && i < candidates.size(); i ++){
            if (candidates.at(i).tmp_dist > 3.0){
                choose_count = i - 1;
                break;
            }
        }
        if (choose_count > 0){
            candidates.erase(candidates.begin() + choose_count, candidates.end());
        }
    }
}

void PlayerPredictions::generate_new_candidates_by_hear(const WorldModel &wm, const PlayerObject *p){
    auto memory = wm.audioMemory().player();
    Vector2D heard_pos = Vector2D::INVALIDATED;
    double error_dist = 0.5;
    double heard_body = -360.0;
    if (wm.audioMemory().playerTime().cycle() != wm.time().cycle())
        return;
    for (auto & a: memory){
        int unum = a.unum_;
        if (p->side() != wm.ourSide())
            unum -= 11;
        if (unum == p->unum()){
            heard_pos = a.pos_;
            error_dist = a.pos_count_ + 0.5;
            heard_body = a.body_;
            dlog.addCircle(Logger::WORLD, a.pos_, error_dist, 255,0,0);
            break;
        }
    }
    if (!heard_pos.isValid())
        return;
    for (double d: {0.0, 0.3, 0.7, 1.0}){
        for(double a: {0.0, 90.0, 180.0, 270.0}){
            Vector2D new_pos = heard_pos + Vector2D::polar2vector(d * error_dist, a + heard_body);
            vector<double> all_body;
            if (heard_body != -360.0)
                all_body.emplace_back(heard_body);
            else
                all_body = {0.0, 90.0, 180.0, 270.0};
            for (auto & body: all_body)
                candidates.emplace_back(new_pos, Vector2D(0, 0), body);
            if (d == 0.0)
                break;
            if (heard_body != -360.0)
                break;
        }
    }
}

void PlayerPredictions::filter_candidates_by_see(const WorldModel &wm, const PlayerObject *p) {
    #ifdef DEBUG_ACTION_DENOISER
    dlog.addText(Logger::WORLD, "######## filter candidates ########");
    #endif
    candidates_removed_by_filtering.clear();
    candidates_removed_by_filtering.reserve(candidates.size());
    if (p->seenPosCount() != 0)
        return;
    auto self_pos = wm.self().pos();
    auto rel_pos = p->pos() - wm.self().pos();
    double seen_dist = p->seen_dist();
    auto seen_dir = rel_pos.th();
    double body = (p->bodyCount() == 0 ? p->body().degree(): -360);
    auto vel = (p->seenVelCount() == 0 ? p->vel() : Vector2D::INVALIDATED);
    double avg_dist;
    double dist_err;
    if (object_table.getMovableObjInfo(seen_dist,
                                       &avg_dist,
                                       &dist_err)) {
        #ifdef DEBUG_ACTION_DENOISER
        dlog.addSector(Logger::WORLD, wm.self().pos(), avg_dist - dist_err,
                       avg_dist + dist_err, seen_dir - 1.0, 2.0,
                       "#00FF00");
        #endif
        Sector2D seen_sec = Sector2D(wm.self().pos(), avg_dist - dist_err, avg_dist + dist_err,
                                     AngleDeg(seen_dir - 0.6),
                                     AngleDeg(seen_dir + 0.6));
        V_PSC tmp;
        tmp.reserve(candidates.size());
        for (PSC t: candidates) {
            bool ignored = false;
            if (seen_sec.contains(t.pos)) {
                if (body != -360.0){
                    if (AngleDeg(body - t.body).abs() > 60)
                        ignored = true;
                }
                if (p->velCount() == 0){
                }
                if (!ignored){
                    tmp.push_back(t);
                    if (vel.isValid())
                        tmp.back().vel = vel;
                    if (body != -360.0)
                        tmp.back().body = body;
                }
            }
            else{
                ignored = true;
            }
            if (ignored)
                candidates_removed_by_filtering.push_back(t);
        }
        candidates = tmp;
    } else
        candidates.clear();
}

void PlayerPredictions::filter_candidates_by_hear(const WorldModel &wm, const PlayerObject *p){
    auto memory = wm.audioMemory().player();
    if (wm.audioMemory().playerTime().cycle() != wm.time().cycle())
        return;
    for (auto & a: memory){
        int unum = a.unum_;
        if (p->side() != wm.ourSide())
            unum -= 11;
        if (unum == p->unum()){
            double err = a.pos_count_ + 0.5;
            dlog.addCircle(Logger::WORLD, a.pos_, err, 255,0,0);
            V_PSC tmp;
            for (auto c: candidates)
                if (c.pos.dist(a.pos_) < err)
                    tmp.push_back(c);
            candidates = tmp;
            return;
        }
    }
}

void PlayerPredictions::update_candidates(const WorldModel &wm, const PlayerObject *p) {
    #ifdef DEBUG_ACTION_DENOISER
    dlog.addText(Logger::WORLD, "######## update candidates ########");
    dlog.addText(Logger::WORLD, "candidate size before update: %d", candidates.size());
    #endif
    V_PSC new_candidates;
    new_candidates.reserve(max_candidates_size);
    if (candidates.empty())
        return;
    ulong candidates_processed = 0;
    ulong init_size = candidates.size();
    candidates.reserve(2 * max_candidates_size);
    for (ulong i = 0; i < init_size; i++){
        if (candidates.size() - init_size + init_size - candidates_processed >= max_candidates_size)
            break;
        candidates_processed += 1;
        candidates.at(i).gen_max_next_candidates(wm, p, candidates);
    }
    candidates.erase(candidates.begin(), candidates.begin() + candidates_processed);

    #ifdef DEBUG_ACTION_DENOISER
        dlog.addText(Logger::WORLD, "old candidates size: %d candidate processed: %d new candidate size: %d",
                     candidates.size(), candidates_processed, new_candidates.size());
    #endif
}

void PlayerPredictions::remove_similar_candidates() {
    sort(candidates.begin(), candidates.end(), [](const PlayerStateCandidate & a, const PlayerStateCandidate & b) -> bool
    {
        return a.pos.x < b.pos.x;
    });
    ulong size = candidates.size();
    for (ulong i = 0; i < size; i++){
        for (ulong j = i + 1; j < size; j++){
            auto & f1 = candidates.at(i);
            auto & f2 = candidates.at(j);
            bool should_removed = f1.is_close(f2);
//            auto s9 = std::chrono::high_resolution_clock::now();
            if (should_removed){
                candidates.erase(candidates.begin() + j);
                size -= 1;
                j -= 1;
            }
            if (f1.pos.x + 0.2 < f2.pos.x)
                break;
//            auto s10 = std::chrono::high_resolution_clock::now();
//            LDA::t7 += std::chrono::duration<double, std::milli>(s9-s8).count();
//            LDA::t8 += std::chrono::duration<double, std::milli>(s10-s9).count();
        }
    }
//    auto tmp = candidates;
//    candidates.clear();
//
//    for (auto nc: tmp)
//    {
//        auto s8 = std::chrono::high_resolution_clock::now();
//        bool is_far = true;
//        for (auto &c: candidates){
//            if (c.is_close(nc)){
//                is_far = false;
//                break;
//            }
//        }
//        auto s9 = std::chrono::high_resolution_clock::now();
//        if (is_far)
//            candidates.push_back(nc);
//        auto s10 = std::chrono::high_resolution_clock::now();
//        LDA::t7 += std::chrono::duration<double, std::milli>(s10-s9).count();
//        LDA::t8 += std::chrono::duration<double, std::milli>(s9-s8).count();
//        #ifdef DEBUG_ACTION_DENOISER
//        else
//            candidates_removed_by_similarity.push_back(nc);
//        #endif
//    }
}

void PlayerPredictions::clustering(int cluster_count){
    average_pos.invalidate();
    Vector2D avg(0, 0);
    if (!candidates.empty()) {
        for (auto &c: candidates)
            avg += c.pos;
        avg.x = avg.x / double(candidates.size());
        avg.y = avg.y / double(candidates.size());
        average_pos = avg;
    }

    candidates_means.clear();
    std::vector<std::array<double, 2>> pos_arr;
    if (!candidates.empty()) {
        if (cluster_count == 1)
        {
            candidates_means.emplace_back(average_pos);
        }
        else
        {
            for (auto &c: candidates)
                pos_arr.push_back({c.pos.x, c.pos.y});
            auto clustering_res = dkm::kmeans_lloyd(pos_arr, cluster_count);
            auto means = get<0>(clustering_res);
            auto labels = get<1>(clustering_res);
            candidates_means.clear();
            for (auto & mean : means) {
                candidates_means.emplace_back(Vector2D(mean.at(0), mean.at(1)));
            }
        }
    }
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
#include <chrono>
void PlayerPredictions::update(const WorldModel &wm, const PlayerObject *p, int cluster_count) {
    #ifdef DEBUG_ACTION_DENOISER
    dlog.addText(Logger::WORLD, "==================================== %d %d size %d", p->side(), p->unum(),
                 candidates.size());
    #endif
    update_candidates(wm, p);
    filter_candidates_by_see(wm, p);
    filter_candidates_by_hear(wm, p);
    remove_similar_candidates();

    if (candidates.empty()) {
        bool seen_player = player_seen(p);
        bool heard_player = player_heard(wm , p);
        if (seen_player && heard_player){
            generate_new_candidates_by_see(wm, p);
            filter_candidates_by_hear(wm, p);
        } else if (seen_player){
            generate_new_candidates_by_see(wm, p);
        } else if (heard_player){
            generate_new_candidates_by_hear(wm, p);
        }
    }
    clustering(cluster_count);

    #ifdef DEBUG_ACTION_DENOISER
    debug();
    #endif
}

void PlayerPredictions::debug() {
//    for (auto &c: candidates_removed_by_similarity)
//        dlog.addCircle(Logger::WORLD, c.pos, 0.05, 0, 0, 0);
//    for (auto &c: candidates_removed_by_filtering)
//        dlog.addCircle(Logger::WORLD, c.pos, 0.05, 255, 255, 255);
    for (auto &c: candidates_removed_by_updating)
        dlog.addCircle(Logger::WORLD, c.pos, 0.05, 0, 255, 0);
    for (auto &c: candidates){
        dlog.addCircle(Logger::WORLD, c.pos, 0.1, 250, 176, 2);
        dlog.addLine(Logger::WORLD, c.pos, c.pos + Vector2D::polar2vector(0.1, c.body), 250, 176, 2);
//        dlog.addLine(Logger::WORLD, c.pos + Vector2D(0.05, 0.05), c.pos + c.vel + Vector2D(0.05, 0.05), 250, 0, 0);
    }
    dlog.addCircle(Logger::WORLD, average_pos, 0.1, 255, 255, 255, true);
}

LDA * LDA::i() {
    if (instance == nullptr)
        instance = new LDA();
    return instance;
}

void LDA::update_tests(PlayerAgent *agent){
    if (!ServerParam::i().fullstateLeft())
        return;
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
            && p.second.player_heard(wm, t)
            && p.second.player_seen(t)
            && p.second.average_pos.isValid()){
            auto xxx = LDA::i()->get_cluster_means(wm, p.second.side, p.second.unum);
            Vector2D full_pos = agent->fullstateWorld().ourPlayer(p.first)->pos();
            Vector2D pos = wm.ourPlayer(p.first)->pos();

            if (!xxx.empty()){
                Vector2D cyrus_pos = xxx.at(0).pos;
                player_test_res.at(p.first).count += 1;
                player_test_res.at(p.first).base_noise += full_pos.dist(pos);
                player_test_res.at(p.first).cyrus_noise += cyrus_pos.dist(full_pos);
            }
        }
    }
    for (auto & p: opponents){
        auto o = wm.theirPlayer(p.first);
        if (o != nullptr
            && p.second.player_heard(wm, o)
            && p.second.player_seen(o)
            && p.second.average_pos.isValid()){
            auto xxx = LDA::i()->get_cluster_means(wm, p.second.side, p.second.unum);
            Vector2D full_pos = agent->fullstateWorld().theirPlayer(p.first)->pos();
            Vector2D pos = wm.theirPlayer(p.first)->pos();

            if (!xxx.empty()){
                Vector2D cyrus_pos = xxx.at(0).pos;
                player_test_res.at(p.first + 11).count += 1;
                player_test_res.at(p.first + 11).base_noise += full_pos.dist(pos);
                player_test_res.at(p.first + 11).cyrus_noise += cyrus_pos.dist(full_pos);
            }
        }
    }
    if ((wm.time().cycle() == 2999 || wm.time().cycle() == 5999) && wm.time().stopped() == 0)
    {
        double base_noises = 0.0;
        double cyrus_noises = 0.0;
        double all_count = 0.0;
        for (int i = 1; i <= 22; i++)
        {
            auto & res = player_test_res.at(i);
            if (res.count > 0)
            {
                cout<<"side "<<(i <= 11 ? "T " : "O ")<<(i <= 11 ? i : i - 11)<<" "<<res.base_noise / res.count <<" -> "<<res.cyrus_noise / res.count<<endl;
                base_noises += res.base_noise;
                cyrus_noises += res.cyrus_noise;
                all_count += res.count;
            }
        }
        if (all_count > 0)
            cout<<"end"<<base_noises / all_count<<" -> "<<cyrus_noises / all_count<<endl;
    }

}

void LDA::update_world_model(PlayerAgent * agent){
//    update world model!!
    auto &wm = agent->world();
    auto & wm_not_const = agent->world_not_const();
    for (auto &p: wm_not_const.M_teammates_from_self) {
        if (p == nullptr)
            continue;
        if (p->unum() <= 0)
            continue;
        if(teammates[p->unum()].player_seen(p) || teammates[p->unum()].player_heard(wm, p))
            if (teammates[p->unum()].average_pos.isValid()){
                p->M_base_pos = p->M_pos;
                p->M_pos = teammates[p->unum()].average_pos;
            }
    }
    for (auto &p: wm_not_const.M_opponents_from_self) {
        if (p == nullptr)
            continue;
        if (p->unum() <= 0)
            continue;
        if(opponents[p->unum()].player_seen(p) || opponents[p->unum()].player_heard(wm, p))
            if (opponents[p->unum()].average_pos.isValid()){
                p->M_base_pos = p->M_pos;
                p->M_pos = opponents[p->unum()].average_pos;
            }
    }
}

void LDA::update(PlayerAgent *agent) {
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
            teammates.insert(make_pair(p->unum(), PlayerPredictions(p->side(), p->unum())));
        }
        teammates[p->unum()].update(wm, p, cluster_count);
    }
    for (auto &p: wm.opponents()) {
        if (p == nullptr)
            continue;
        if (p->unum() <= 0)
            continue;
        if (opponents.find(p->unum()) == opponents.end()) {
            opponents.insert(make_pair(p->unum(), PlayerPredictions(p->side(), p->unum())));
        }
            opponents[p->unum()].update(wm, p, cluster_count);
    }
    #ifdef DEBUG_ACTION_DENOISER
    update_tests(agent);
    #endif
    update_world_model(agent);
}

V_PSC LDA::get_cluster_means(const WorldModel &wm, SideID side, int unum) {
    auto &players_list = (wm.self().side() == side ? teammates : opponents);
    if (players_list.find(unum) == players_list.end()) {
        return empty_vector;
    }
    V_PSC res;
    res.emplace_back(players_list.at(unum).average_pos);
    return res;
}

void LDA::debug(PlayerAgent * agent) {
    #ifdef DEBUG_ACTION_DENOISER
    for (auto & p: agent->world().allPlayers()){
        dlog.addCircle(Logger::WORLD, p->M_base_pos, 0.1, 0, 0, 255, true);
        dlog.addCircle(Logger::WORLD, p->pos(), 0.1, 255, 0, 0, true);
    }
    #endif
}