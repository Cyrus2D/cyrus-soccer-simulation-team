//
// Created by nader on 2023-05-15.
//

#include "denoising.h"
#include <vector>
#include <rcsc/player/player_agent.h>
#include <iostream>
#include "../dkm/dkm.hpp"

using namespace rcsc;
using namespace std;

Denoising *Denoising::i() {
    if (instance == nullptr) {
        instance = new Denoising();
    }
    return instance;
}

Polygon2D mutual_convex(const Polygon2D &p1p, const Polygon2D &p2p) {
    std::vector<Vector2D> vertices;
    for (const auto &p: p1p.vertices()) {
        if (p2p.contains(p))
            vertices.push_back(p);
    }
    for (const auto &p: p2p.vertices()) {
        if (p1p.contains(p))
            vertices.push_back(p);
    }
    std::vector<std::tuple<Line2D, double, double>> p1l;
    std::vector<std::tuple<Line2D, double, double>> p2l;
    for (uint i = 0; i < p1p.vertices().size() - 1; i++) {
        p1l.emplace_back(std::tuple<Line2D, double, double>
                                 {
                                         {p1p.vertices()[i], p1p.vertices()[i + 1]},
                                         std::min(p1p.vertices()[i].x, p1p.vertices()[i + 1].x),
                                         std::max(p1p.vertices()[i].x, p1p.vertices()[i + 1].x)
                                 });
    }
    p1l.emplace_back(std::tuple<Line2D, double, double>
                             {
                                     {p1p.vertices()[p1p.vertices().size() - 1], p1p.vertices()[0]},
                                     std::min(p1p.vertices()[p1p.vertices().size() - 1].x, p1p.vertices()[0].x),
                                     std::max(p1p.vertices()[p1p.vertices().size() - 1].x, p1p.vertices()[0].x)
                             });
    for (int i = 0; i < p2p.vertices().size() - 1; i++) {
        p2l.emplace_back(std::tuple<Line2D, double, double>
                                 {
                                         {p2p.vertices()[i], p2p.vertices()[i + 1]},
                                         std::min(p2p.vertices()[i].x, p2p.vertices()[i + 1].x),
                                         std::max(p2p.vertices()[i].x, p2p.vertices()[i + 1].x)
                                 });
    }
    p2l.emplace_back(std::tuple<Line2D, double, double>
                             {
                                     {p2p.vertices()[p2p.vertices().size() - 1], p2p.vertices()[0]},
                                     std::min(p2p.vertices()[p2p.vertices().size() - 1].x, p2p.vertices()[0].x),
                                     std::max(p2p.vertices()[p2p.vertices().size() - 1].x, p2p.vertices()[0].x)
                             });


    for (const auto &d1: p1l) {
        const auto &l1 = std::get<0>(d1);
        const auto &min1_x = std::get<1>(d1);
        const auto &max1_x = std::get<2>(d1);
        for (const auto &d2: p2l) {
            const auto &l2 = std::get<0>(d2);
            const auto &min2_x = std::get<1>(d2);
            const auto &max2_x = std::get<2>(d2);
            Vector2D inter = l1.intersection(l2);
            if (!inter.isValid()) {
                continue;
            }
            if (!(min1_x < inter.x && inter.x < max1_x)) {
                continue;
            }
            if (!(min2_x < inter.x && inter.x < max2_x)) {
                continue;
            }
            vertices.emplace_back(inter);
        }
    }

    ConvexHull mutual(vertices);
    mutual.compute();
    return mutual.toPolygon();

}

void draw_poly(const Polygon2D &p, const char* color){
    const auto& vertices = p.vertices();
    for(int i = 0; i < vertices.size()-1; i++){
        dlog.addLine(Logger::WORLD, vertices[i], vertices[i+1], color);
    }
    dlog.addLine(Logger::WORLD, vertices[0], vertices[vertices.size() - 1], color);
}


void Denoising::update(PlayerAgent *agent) {
    if (!ServerParam::i().fullstateLeft())
        return;
    const WorldModel &wm = agent->world();
    const WorldModel &fwm = agent->fullstateWorld();

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

void Denoising::debug() {
//        output << "Self:" << endl;
//        if (D.self_face_diff.size() == 0)
//            return output;
//        double self_face_diff_sum = 0;
//        double self_face_diff_max = 0;
//        double self_face_diff_avg = 0;
//        for (auto & d: D.self_face_diff)
//            self_face_diff_sum += d;
//        self_face_diff_max = *max_element(D.self_face_diff.begin(), D.self_face_diff.end());
//        self_face_diff_avg = self_face_diff_sum / double (D.self_face_diff.size());
//        double self_neck_diff_sum = 0;
//        double self_neck_diff_max = 0;
//        double self_neck_diff_avg = 0;
//        for (auto & d: D.self_neck_diff)
//            self_neck_diff_sum += d;
//        self_neck_diff_max = *max_element(D.self_neck_diff.begin(), D.self_neck_diff.end());
//        self_neck_diff_avg = self_neck_diff_sum / double (D.self_neck_diff.size());
//        double self_body_diff_sum = 0;
//        double self_body_diff_max = 0;
//        double self_body_diff_avg = 0;
//        for (auto & d: D.self_body_diff)
//            self_body_diff_sum += d;
//        self_body_diff_max = *max_element(D.self_body_diff.begin(), D.self_body_diff.end());
//        self_body_diff_avg = self_body_diff_sum / double (D.self_body_diff.size());
//
//        double self_pos_diff_sum = 0;
//        double self_pos_diff_max = 0;
//        double self_pos_diff_avg = 0;
//        for (auto & d: D.self_pos_diff)
//            self_pos_diff_sum += d;
//        self_pos_diff_max = *max_element(D.self_pos_diff.begin(), D.self_pos_diff.end());
//        self_pos_diff_avg = self_pos_diff_sum / double (D.self_pos_diff.size());
//        output << "face: count"<<D.self_face_diff.size() <<" max:"<<self_face_diff_max<<" avg:"<<self_face_diff_avg<<endl;
//        output << "neck: count"<<D.self_neck_diff.size() <<" max:"<<self_neck_diff_max<<" avg:"<<self_neck_diff_avg<<endl;
//        output << "body: count"<<D.self_body_diff.size() <<" max:"<<self_body_diff_max<<" avg:"<<self_body_diff_avg<<endl;
//        output << "pos: count"<<D.self_pos_diff.size() <<" max:"<<self_pos_diff_max<<" avg:"<<self_pos_diff_avg<<endl;
//        return output;
}

#include <random>

static std::default_random_engine gen;

double get_random(double min = 0.0, double max = 1.0) {
    static std::uniform_real_distribution<double> dis(0.0, 1.0);
    return dis(gen) * (max - min) + min;


}

double get_random_normal(double mean, double std_dev, double min = 0.0, double max = 1.0) {
//    static std::default_random_engine gen;
    static std::normal_distribution<double> dis(mean, std_dev); // range [0, 1)
    while (true) {
        double rnd = dis(gen);
        if (rnd >= min && rnd <= max)
            return rnd;
    }
    return 0.0;
}

PlayerStateCandidate::PlayerStateCandidate(Vector2D pos_, Vector2D vel_, double body_) {
    pos = pos_;
    vel = vel_;
    body = body_;
}

PlayerStateCandidate PlayerStateCandidate::gen_random_next_by_dash(const WorldModel &wm, const PlayerObject *p) const {
    const ServerParam &SP = ServerParam::i();
    auto p_type = p->playerTypePtr();
    double dash_dir = get_random_normal(0, 25.0, -180.0, +180.0);
    dash_dir = AngleDeg(dash_dir).degree();
    double accel_dist = SP.maxDashPower() * p_type->effortMax() * SP.dashDirRate(dash_dir) * p_type->dashPowerRate();
    Vector2D accel = Vector2D::polar2vector(accel_dist, AngleDeg(body + dash_dir));
    Vector2D move = accel + vel;
    return {pos + move, move * p_type->playerDecay(), body};
}

PlayerStateCandidate PlayerStateCandidate::gen_random_next_by_turn(const WorldModel &wm, const PlayerObject *p) const {
    const ServerParam &SP = ServerParam::i();
    auto p_type = p->playerTypePtr();
    double max_turn = p_type->effectiveTurn(SP.maxMoment(), vel.r());
    double turn_moment = get_random(-max_turn, max_turn);
    return {pos + vel, vel * p_type->playerDecay(), AngleDeg(body + turn_moment).degree()};
}

PlayerStateCandidate
PlayerStateCandidate::gen_random_next_by_nothing(const WorldModel &wm, const PlayerObject *p) const {
    auto p_type = p->playerTypePtr();
    return {pos + vel, vel * p_type->playerDecay(), body};
}

PlayerStateCandidate PlayerStateCandidate::gen_random_next(const WorldModel &wm, const PlayerObject *p) const {
    double rnd = get_random(0.0, 2.0);
    if (p->isKickable())
        return gen_random_next_by_nothing(wm, p);
    if (rnd < 0.7)
        return gen_random_next_by_dash(wm, p);
    return gen_random_next_by_turn(wm, p);
}

vector<PlayerStateCandidate> PlayerStateCandidate::gen_max_next_candidates(const WorldModel &wm, const PlayerObject *p) const{
    vector<PlayerStateCandidate> res;
    const ServerParam &SP = ServerParam::i();

    auto p_type = p->playerTypePtr();
    const double max_turn = p_type->effectiveTurn( SP.maxMoment(), vel.r() );

    if (-180. < body && body < 180. && cycle == 0) {
        for (int i = 0; i < 6; i += 1) {
            double dash_dir = i * 60.0;
            if (std::abs(dash_dir) < max_turn) {
                PlayerStateCandidate tmp_turn(pos, vel * p_type->playerDecay(), body + dash_dir);
                res.push_back(tmp_turn);
                tmp_turn.cycle = cycle + 1;
            }
        }
    }
    if (-180. < body && body < 180.){
        for (int i = 0; i < 6; i += 1) {
            double dash_dir = i * 60.0;
            double accel_dist = SP.maxDashPower()
                                * p_type->effortMax()
                                * SP.dashDirRate(dash_dir)
                                * p_type->dashPowerRate();
            Vector2D accel = Vector2D::polar2vector(accel_dist, AngleDeg(dash_dir + body));
            Vector2D move = accel + vel;
            PlayerStateCandidate tmp_dash(pos + move, move * p_type->playerDecay(), body);
            tmp_dash.cycle = cycle + 1;
            res.push_back(tmp_dash);
        }
    }
    else{
        for (int i = 0; i < 6; i += 1) {
            double dash_dir = i * 60.0;
            double accel_dist = SP.maxDashPower()
                                * p_type->effortMax()
                                * SP.dashDirRate(0)
                                * p_type->dashPowerRate();
            Vector2D accel = Vector2D::polar2vector(accel_dist, AngleDeg(dash_dir));
            Vector2D move = accel + vel;
            PlayerStateCandidate tmp_dash(pos + move, move * p_type->playerDecay(), dash_dir);
            tmp_dash.cycle = cycle + 1;
            res.push_back(tmp_dash);
        }
    }
    return res;
}


#include <rcsc/player/object_table.h>


PlayerPredictedObj::PlayerPredictedObj(SideID side_, int unum_)
        : object_table() {
    side = side_;
    unum = unum_;
    last_seen_time = GameTime(0, 0);
    std::string file_name = "vertices/v-" + std::to_string(unum-1);
    std::ifstream fin(file_name);
    area = nullptr;

    player_data.init(fin, unum);
}

PlayerPredictedObj::PlayerPredictedObj() {
}

void PlayerPredictedObj::generate_new_candidates(const WorldModel &wm, const PlayerObject *p) {
    auto self_pos = wm.self().pos();
    Vector2D rpos = p->pos() - wm.self().pos();
    double seen_dist = p->seen_dist();
    AngleDeg seen_dir = rpos.th();
    double body = -360;
    if (p->bodyCount() == 0)
        body = p->body().degree();
    Vector2D vel = Vector2D::INVALIDATED;
    if (p->seenVelCount() == 0)
        vel = p->vel();
    double avg_dist;
    double dist_err;
    if (object_table.getMovableObjInfo(seen_dist,
                                       &avg_dist,
                                       &dist_err)) {
        dlog.addSector(Logger::WORLD, wm.self().pos(), avg_dist - dist_err, avg_dist + dist_err, seen_dir - 1.0, 2.0,
                       "#0000FF");
        candidates.clear();
        dlog.addText(Logger::WORLD, "########generate new candidates");
        dlog.addText(Logger::WORLD, "$$$$ seen_dist: %.2f avg_dist: %.2f dist_err: %.2f dir: %.2f",
                     seen_dist,
                     avg_dist,
                     dist_err,
                     seen_dir);
        for (double d = 0; d < 3; d++){
            for (double a = 0; a < 3; a++){
                double dist = avg_dist - dist_err + d * dist_err;
                double dir = seen_dir.degree() - 0.5 + 0.5 * a;
                Vector2D pos = self_pos + Vector2D::polar2vector(dist, dir);
                auto tmp_vel = vel;
                if (!tmp_vel.isValid()) {
                    tmp_vel = Vector2D(0, 0);
                }
                double tmp_body = body;
                PlayerStateCandidate candidate(pos, tmp_vel, tmp_body);
                candidates.push_back(candidate);
            }
        }
    }
}

void PlayerPredictedObj::check_candidates(const WorldModel &wm, const PlayerObject *p) {
    dlog.addText(Logger::WORLD, "########check candidates");
    auto self_pos = wm.self().pos();
    Vector2D rpos = p->pos() - wm.self().pos();
    double seen_dist = p->seen_dist();
    AngleDeg seen_dir = rpos.th();
    double body = -360;
    if (p->bodyCount() == 0)
        body = p->body().degree();
    Vector2D vel = Vector2D::INVALIDATED;
    if (p->seenVelCount() == 0)
        vel = p->vel();
    double avg_dist;
    double dist_err;
    if (object_table.getMovableObjInfo(seen_dist,
                                       &avg_dist,
                                       &dist_err)) {
        dlog.addSector(Logger::WORLD, wm.self().pos(), avg_dist - dist_err, avg_dist + dist_err, seen_dir - 1.0, 2.0,
                       "#00FF00");
        Sector2D seen_sec = Sector2D(wm.self().pos(), avg_dist - dist_err, avg_dist + dist_err,
                                     AngleDeg(seen_dir - 1.0),
                                     AngleDeg(seen_dir + 1.0));
        vector<PlayerStateCandidate> tmp;
        for (PlayerStateCandidate t: candidates) {
            if (seen_sec.contains(t.pos)) {
                tmp.push_back(t);
                if (vel.isValid())
                    tmp.at(tmp.size() - 1).vel = vel;
                if (body != -360.0)
                    tmp.at(tmp.size() - 1).body = body;
                dlog.addCircle(Logger::WORLD, t.pos, 0.1, "#00FF00");
            }
        }
        candidates = tmp;
//            auto new_end = std::remove_if(candidates.begin(), candidates.end(),
//                                          [&](const PlayerStateCandidate& candid)
//                                          { return !seen_sec.contains(candid.pos); });
//            if (new_end == candidates.begin())
//                candidates.clear();
//            else
//                candidates.erase(new_end, candidates.end());
    } else
        candidates.clear();
}

void PlayerPredictedObj::update_candidates(const WorldModel &wm, const PlayerObject *p) {
    dlog.addText(Logger::WORLD, "########update candidates");
    Vector2D rpos = p->pos() - wm.self().pos();
    double seen_dist = p->seen_dist();
    AngleDeg seen_dir = rpos.th();
    double avg_dist;
    double dist_err;
    std::cout << "A" << std::endl;
    if (area == nullptr) {
        if (object_table.getMovableObjInfo(seen_dist,
                                           &avg_dist,
                                           &dist_err)) {
            std::cout << "B" << std::endl;
            std::vector<Vector2D> poses = {
                    wm.self().pos() + Vector2D::polar2vector(avg_dist - dist_err, seen_dir - 0.5),
                    wm.self().pos() + Vector2D::polar2vector(avg_dist + dist_err, seen_dir - 0.5),
                    wm.self().pos() + Vector2D::polar2vector(avg_dist - dist_err, seen_dir + 0.5),
                    wm.self().pos() + Vector2D::polar2vector(avg_dist + dist_err, seen_dir + 0.5),
            };
            ConvexHull tmp(poses);
            tmp.compute();
            area = new Polygon2D(tmp.toPolygon().vertices());
            std::cout << "BB" << std::endl;
            draw_poly(*area, "#FFFFFF");
        }
    }
    else{
        if (object_table.getMovableObjInfo(seen_dist,
                                           &avg_dist,
                                           &dist_err)) {
            std::cout << "C" << std::endl;
            std::vector<Vector2D> poses = {
                    wm.self().pos() + Vector2D::polar2vector(avg_dist - dist_err, seen_dir - 0.5),
                    wm.self().pos() + Vector2D::polar2vector(avg_dist + dist_err, seen_dir - 0.5),
                    wm.self().pos() + Vector2D::polar2vector(avg_dist - dist_err, seen_dir + 0.5),
                    wm.self().pos() + Vector2D::polar2vector(avg_dist + dist_err, seen_dir + 0.5),
            };
            ConvexHull new_area(poses);
            new_area.compute();
            int index;
            if (wm.time().cycle() == last_seen_time.cycle()){
                index = wm.time().stopped() - last_seen_time.stopped() - 1;
            }
            else{
                index = wm.time().cycle() - last_seen_time.cycle() - 1;
            }
            std::cout << "C1" << std::endl;
            std::cout << "index=" << index << std::endl;
            dlog.addText(Logger::WORLD, "unum=%d", unum);
            dlog.addText(Logger::WORLD, "last_time=%d, index=%d",last_seen_time.cycle(), index);
            if (0 <= index && index < 9){
                std::vector<Vector2D> vertices;
                const ConvexHull* prob_area = player_data.convexes[index];
                dlog.addText(Logger::WORLD, "pd.c.v=%d", player_data.convexes[index]->vertices().size());
                std::cout << "C2" << std::endl;
                std::cout << area->vertices().size() << std::endl;
                Vector2D center = area->xyCenter();
                std::cout << "C21" << std::endl;
                for (const auto& v: prob_area->vertices()){
                    std::cout << v << std::endl;
                    std::cout << center << std::endl;
                    std::cout << v + center << std::endl;
                    vertices.push_back(v + center);
                }
                std::cout << "C3" << std::endl;
                Polygon2D prob_poly(vertices);
                Polygon2D mutual_area = mutual_convex(prob_poly, new_area.toPolygon());
                delete area;
                area = nullptr;
                std::cout << "C4" << std::endl;
                draw_poly(prob_poly, "#FF0000");
                draw_poly(new_area.toPolygon(), "#0000FF");
                std::cout << "C5" << std::endl;
                if (mutual_area.vertices().size() > 2) {
                    area = new Polygon2D(mutual_area.vertices());
                    draw_poly(*area, "#000000");
                    std::cout << "C6" << std::endl;
                }
                std::cout << "CC" << std::endl;
            }
            else {
                delete area;
                area = new Polygon2D(new_area.vertices());
                draw_poly(*area, "#000000");
            }
        }
    }
    last_seen_time = wm.time();

//    vector<PlayerStateCandidate> new_candidates;
//    if (candidates.empty())
//        return;
//    if (candidates.size() > 400)
//        return;
//    for (auto &c: candidates) {
//        dlog.addText(Logger::WORLD, "==== (%.1f, %.1f), (%.1f, %.1f), %.1f", c.pos.x, c.pos.y, c.vel.x, c.vel.y,
//                     c.body);
//        auto next_candidates = c.gen_max_next_candidates(wm, p); // todo not add candids on 2 or  cycles old candids
//        for (auto n: next_candidates)
//            new_candidates.push_back(n);
//    }
////        candidates.reserve(candidates.size() + new_candidates.size() ); // preallocate memory
//    for (auto nc: new_candidates)
//    {
//        bool is_far = true;
//        for (auto &c: candidates){
//            if (c.pos.dist(nc.pos) < 0.3 /*&& std::abs(nc.body - c.body) < 30*/){
//                is_far = false;
//                break;
//            }
//        }
//        if (is_far)
//            candidates.push_back(nc);
//    }

}

void PlayerPredictedObj::update(const WorldModel &wm, const PlayerObject *p, int cluster_count) {
    dlog.addText(Logger::WORLD, "==================================== %d %d size %d", p->side(), p->unum(),
                 candidates.size());
    if (p->seenPosCount() == 0) {
//        candidates.clear();
        update_candidates(wm, p);
//        check_candidates(wm, p);
//        if (candidates.empty()) {
//            generate_new_candidates(wm, p);
////            for (auto &c: candidates) {
////                dlog.addCircle(Logger::WORLD, c.pos, 0.1, "#0000FF");
////            }
//        }

    } else {
//        update_candidates(wm, p);
//        for (auto &c: candidates) {
//            dlog.addCircle(Logger::WORLD, c.pos, 0.1, "#FFFFFF");
//        }
    }

    Vector2D avg(0, 0);
    std::vector<std::array<double, 2>> pos_arr;
    if (!candidates.empty()) {
        for (auto &c: candidates) {
            avg += c.pos;
            pos_arr.push_back({c.pos.x, c.pos.y});
        }
        avg.x = avg.x / double(candidates.size());
        avg.y = avg.y / double(candidates.size());
        average_pos = avg;
//        dlog.addCircle(Logger::WORLD, avg, 0.2, "#FFFFFF", true);
        auto clustering_res = dkm::kmeans_lloyd(pos_arr, cluster_count);
        auto means = get<0>(clustering_res);
        auto labels = get<1>(clustering_res);
        candidates_means.clear();
        for (int i = 0; i < means.size(); i++) {
            candidates_means.emplace_back(Vector2D(means[i].at(0), means[i].at(1)));

        }
        int max_cycle = 0;
        for (auto & c: candidates)
            if (c.cycle > max_cycle)
                max_cycle = c.cycle;
        for (auto & c: candidates){
            dlog.addCircle(Logger::WORLD,
                           c.pos,
                           0.1,
                           int(double (c.cycle) / double (max_cycle) * 255),
                           int(double (c.cycle) / double (max_cycle) * 255),
                           int(double (c.cycle) / double (max_cycle) * 255),
                           false);
            dlog.addLine(Logger::WORLD,
                         c.pos,
                         Vector2D::polar2vector(0.1, c.body) + c.pos,
                         int(double (c.cycle) / double (max_cycle) * 255), 0, 0);
        }
//            auto labels = std::get<1>(dkm::kmeans_lloyd(pos_arr, 3));
//        for (int i = 0; i < means.size(); i++) {
//            if (i == 0) {
//                dlog.addCircle(Logger::WORLD, Vector2D(means[i].at(0), means[i].at(1)), 0.2, "#000000", true);
//            } else if (i == 1) {
//                dlog.addCircle(Logger::WORLD, Vector2D(means[i].at(0), means[i].at(1)), 0.2, "#00FF00", true);
//            } else if (i == 2) {
//                dlog.addCircle(Logger::WORLD, Vector2D(means[i].at(0), means[i].at(1)), 0.2, "#FF0000", true);
//            }
//        }
//        for (int i = 0; i < labels.size(); i++) {
//            const auto &c = candidates[i];
//            if (labels[i] == 0) {
//                dlog.addCircle(Logger::WORLD, c.pos, 0.1, "#000000");
//            } else if (labels[i] == 1) {
//                dlog.addCircle(Logger::WORLD, c.pos, 0.1, "#00FF00");
//            } else if (labels[i] == 2) {
//                dlog.addCircle(Logger::WORLD, c.pos, 0.1, "#FF0000");
//            }
//        }
    }



//        if seen pos == 0
//          remove candidates
//          update old candidates
//          add new candidates
//        else
//          update old candidates

}

void PlayerPredictedObj::debug() {
//        for (auto &c: candidates)
//            dlog.addCircle(Logger::WORLD, c.pos, 0.1, 250, 0, 0);
}

CyrusDenoiser *CyrusDenoiser::i() {
    if (instance == nullptr)
        instance = new CyrusDenoiser();
    return instance;
}

void CyrusDenoiser::update(PlayerAgent *agent) {
    auto &wm = agent->world();
    last_updated_cycle = wm.time().cycle();
    last_update_stopped = wm.time().stopped();
    for (auto &p: wm.teammates()) {
        if (p == nullptr)
            continue;
        if (p->unum() <= 0)
            continue;
        if (teammates.find(p->unum()) == teammates.end()) {
            teammates.insert(make_pair(p->unum(), PlayerPredictedObj(p->side(), p->unum())));
        }
        teammates[p->unum()].update(wm, p, cluster_count);
    }
    for (auto &p: wm.opponents()) {
        if (p == nullptr)
            continue;
        if (p->unum() <= 0)
            continue;
        if (opponents.find(p->unum()) == opponents.end()) {
            opponents.insert(make_pair(p->unum(), PlayerPredictedObj(p->side(), p->unum())));
        }
        if (opponents.find(p->unum()) != opponents.end())
            opponents[p->unum()].update(wm, p, cluster_count);
    }
}

const vector<PlayerStateCandidate> CyrusDenoiser::get_cluster_means(const WorldModel &wm, SideID side, int unum) {
    auto &players_list = (wm.self().side() == side ? teammates : opponents);
    if (players_list.find(unum) == players_list.end()) {
        return empty_vector;
    }
    vector<PlayerStateCandidate> res;
    res.push_back(PlayerStateCandidate(players_list.at(unum).average_pos));
    return res;
    return players_list[unum].candidates_means;
}

void CyrusDenoiser::debug() {
//        for (auto p: teammates)
//            p.second.debug();
//        for (auto p: opponents)
//            p.second.debug();
}