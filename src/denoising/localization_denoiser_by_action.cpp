//
// Created by nader on 2023-05-15.
//
#include <iostream>
#include <random>
#include <vector>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/object_table.h>
#include "localization_denoiser_by_action.h"
#include "../dkm/dkm.hpp"
#include "../debugs.h"


using namespace rcsc;
using namespace std;


static std::default_random_engine gen;
typedef vector<PlayerStateCandidate> V_PSC;
typedef PlayerStateCandidate PSC;
typedef LocalizationDenoiserByAction LDA;

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

V_PSC
PSC::gen_max_next_candidates(const WorldModel &wm, const PlayerObject *p) const{
    V_PSC res;
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
    return res;
}

PlayerPredictions::PlayerPredictions(SideID side_, int unum_)
        : object_table() {
    side = side_;
    unum = unum_;

}

PlayerPredictions::PlayerPredictions() {
}

void PlayerPredictions::generate_new_candidates(const WorldModel &wm, const PlayerObject *p) {
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
        vector<pair<double, PSC>> dist_candidates;
        for (auto c: candidates){
            dist_candidates.emplace_back(1000, c);
            for (auto & f: candidates_removed_by_filtering){
                dist_candidates.back().first = min(dist_candidates.back().first, c.pos.dist(f.pos));
            }
        }
        sort(dist_candidates.begin(), dist_candidates.end(),
             [](const pair<double, PSC> & a, const pair<double, PSC> & b) -> bool
             {
                 return a.first < b.first;
             });

        for (int i = 0; i < choose_count; i ++){
            if (dist_candidates.at(i).first < 3.0)
                tmp.push_back(dist_candidates.at(i).second);
        }
        if (!tmp.empty())
            candidates = tmp;
    }
}

void PlayerPredictions::filter_candidates(const WorldModel &wm, const PlayerObject *p) {
    #ifdef DEBUG_ACTION_DENOISER
    dlog.addText(Logger::WORLD, "######## filter candidates ########");
    #endif
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
        candidates_removed_by_filtering.clear();
        V_PSC tmp;
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

void PlayerPredictions::update_candidates(const WorldModel &wm, const PlayerObject *p) {
    #ifdef DEBUG_ACTION_DENOISER
    dlog.addText(Logger::WORLD, "######## update candidates ########");
    dlog.addText(Logger::WORLD, "candidate size before update: %d", candidates.size());
    #endif
    V_PSC new_candidates;
    if (candidates.empty())
        return;
    ulong candidates_processed = 0;
    for (auto &c: candidates) {
        if (new_candidates.size() + candidates_processed >= max_candidates_size)
            break;
        candidates_processed += 1;
        #ifdef DEBUG_ACTION_DENOISER
        dlog.addText(Logger::WORLD, "==== (%.1f, %.1f), (%.1f, %.1f), %.1f", c.pos.x, c.pos.y, c.vel.x, c.vel.y,
                     c.body);
        #endif
        for (auto n: c.gen_max_next_candidates(wm, p)){
            if (new_candidates.size() + candidates_processed >= max_candidates_size)
                break;
            new_candidates.push_back(n);
            #ifdef DEBUG_ACTION_DENOISER
            dlog.addText(Logger::WORLD, "====== (%.1f, %.1f), (%.1f, %.1f), %.1f", n.pos.x, n.pos.y, n.vel.x, n.vel.y,
                         n.body);
            #endif
        }
    }
    for (ulong i = candidates_processed; i < candidates.size(); i++)
        new_candidates.push_back(candidates.at(i));

    #ifdef DEBUG_ACTION_DENOISER
        dlog.addText(Logger::WORLD, "old candidates size: %d candidate processed: %d new candidate size: %d",
                     candidates.size(), candidates_processed, new_candidates.size());
    #endif
    candidates.clear();
    candidates = new_candidates;
}

void PlayerPredictions::remove_similar_candidates() {
    auto tmp = candidates;
    candidates.clear();
    for (auto nc: tmp)
    {
        bool is_far = true;
        for (auto &c: candidates){
            if (c.is_close(nc)){
                is_far = false;
                break;
            }
        }
        if (is_far)
            candidates.push_back(nc);
        #ifdef DEBUG_ACTION_DENOISER
        else
            candidates_removed_by_similarity.push_back(nc);
        #endif
    }
}

void PlayerPredictions::clustering(int cluster_count){
    average_pos.invalidate();
    candidates_means.clear();
    Vector2D avg(0, 0);
    if (!candidates.empty()) {
        for (auto &c: candidates)
            avg += c.pos;
        avg.x = avg.x / double(candidates.size());
        avg.y = avg.y / double(candidates.size());
        average_pos = avg;
    }

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

void PlayerPredictions::update(const WorldModel &wm, const PlayerObject *p, int cluster_count) {
    #ifdef DEBUG_ACTION_DENOISER
    dlog.addText(Logger::WORLD, "==================================== %d %d size %d", p->side(), p->unum(),
                 candidates.size());
    #endif
    if (p->seenPosCount() == 0) {
        update_candidates(wm, p);
        filter_candidates(wm, p);
        remove_similar_candidates();
        if (candidates.empty()) {
            generate_new_candidates(wm, p);
        }

    } else {
        update_candidates(wm, p);
        remove_similar_candidates();
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
    auto &wm = agent->world();
    struct PlayerTestRes {
        double count = 0.0;
        double base_noise = 0.0;
        double cyrus_noise = 0.0;
    };
    static vector<PlayerTestRes> player_test_res(23, PlayerTestRes());
    for (auto & p: teammates){
        if (wm.ourPlayer(p.first) != nullptr && wm.ourPlayer(p.first)->seenPosCount() == 0){
            auto xxx = LDA::i()->get_cluster_means(wm, p.second.side, p.second.unum);
            Vector2D full_pos = agent->fullstateWorld().ourPlayer(p.first)->pos();
            Vector2D pos = wm.ourPlayer(p.first)->seenPos();

            if (!xxx.empty()){
                Vector2D cyrus_pos = xxx.at(0).pos;
                player_test_res.at(p.first).count += 1;
                player_test_res.at(p.first).base_noise += full_pos.dist(pos);
                player_test_res.at(p.first).cyrus_noise += cyrus_pos.dist(full_pos);
            }
        }
    }
    for (auto & p: opponents){
        if (wm.theirPlayer(p.first) != nullptr && wm.theirPlayer(p.first)->seenPosCount() == 0){
            auto xxx = LDA::i()->get_cluster_means(wm, p.second.side, p.second.unum);
            Vector2D full_pos = agent->fullstateWorld().theirPlayer(p.first)->pos();
            Vector2D pos = wm.theirPlayer(p.first)->seenPos();

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
        if (opponents.find(p->unum()) != opponents.end())
            opponents[p->unum()].update(wm, p, cluster_count);
    }
    update_tests(agent);

//    update world model!!
//    auto & wm_not_const = agent->world_not_const();
//    for (auto &p: wm_not_const.M_teammates_from_self) {
//        if (p == nullptr)
//            continue;
//        if (p->unum() <= 0)
//            continue;
//        if(p->seenPosCount() == 0)
//            if (teammates[p->unum()].average_pos.isValid())
//                p->M_pos = teammates[p->unum()].average_pos;
//    }
//    for (auto &p: wm.M_opponents_from_self) {
//        if (p == nullptr)
//            continue;
//        if (p->unum() <= 0)
//            continue;
//        if(p->seenPosCount() == 0)
//            if (opponents[p->unum()].average_pos.isValid())
//                p->M_pos = opponents[p->unum()].average_pos;
//    }

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

void LDA::debug() {
}