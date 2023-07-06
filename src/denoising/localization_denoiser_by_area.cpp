//
// Created by nader on 2023-05-15.
//

#include "localization_denoiser_by_area.h"
#include <vector>
#include <rcsc/player/player_agent.h>
#include <iostream>
#include <rcsc/common/player_param.h>
#include "../dkm/dkm.hpp"
#include <rcsc/common/audio_memory.h>

// #define COUT_DEBUG

// #ifndef COUT_DEBUG
#define dd(x) ;
#else
#define dd(x) std::cout << #x << std::endl
#endif

// #define DEBUG_PLAYER_AREA
// #define DEBUG_DENOISE_AREA

using namespace rcsc;
using namespace std;


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

void PlayerPositionConvex::init() {
    const ServerParam& SP = ServerParam::i();
    const int N_ANGLE = 6;
    const int N_DASH = 10;

    for (int i = 0; i < PlayerParam::i().playerTypes(); i++){
        const PlayerType* ptype = PlayerTypeSet::i().get(i);

        convexes_with_body.emplace_back(std::vector<ConvexHull*>{});
        convexes_without_body.emplace_back(std::vector<ConvexHull*>{});


        // without body
        for(int dash_step = 1; dash_step <= N_DASH; dash_step++){
            const double max_dash_dist = ptype->dashDistanceTable()[0][dash_step - 1];

            std::vector<Vector2D> vertices;
            for (int angle_step = 0; angle_step < N_ANGLE; angle_step++){
                const AngleDeg dir = AngleDeg(360./N_ANGLE*angle_step);
                const Vector2D next_rel_pos = Vector2D::polar2vector(max_dash_dist, dir);
                vertices.emplace_back(next_rel_pos);
            }
            ConvexHull* area = new ConvexHull(vertices);
            area->compute();
            convexes_without_body.back().push_back(area);
        }

        // with body
        std::vector<std::vector<Vector2D>> rel_positions;
        const double max_turn = ptype->effectiveTurn(SP.maxMoment(), 0);
        for (int angle_step = 0; angle_step < N_ANGLE; angle_step++) {
            const AngleDeg dir = AngleDeg(360./N_ANGLE*angle_step);

            rel_positions.emplace_back(std::vector<Vector2D>{});

            bool can_turn_in_one_cycle = false;

            if (dir.abs() < max_turn)
                can_turn_in_one_cycle = true;


            const double omni_dash_max_accel = SP.maxDashPower()
                                               * ptype->effortMax()
                                               * SP.dashDirRate(dir.degree())
                                               * ptype->dashPowerRate();
            const double turn_dash_max_accel = SP.maxDashPower()
                                               * ptype->effortMax()
                                               * SP.dashDirRate(0.)
                                               * ptype->dashPowerRate();

            double omni_dash_speed = 0.;
            double turn_dash_speed = 0.;
            double turn2_dash_speed = 0.;
            double omni_dash_dist = 0.;
            double turn_dash_dist = 0.;
            double turn2_dash_dist = 0.;
            for(int dash_step = 1; dash_step <= N_DASH; dash_step++){
                double accel = omni_dash_max_accel;
                if (omni_dash_speed + accel > ptype->realSpeedMax(dir.degree()))
                    accel = ptype->realSpeedMax(dir.degree()) - omni_dash_speed;

                omni_dash_speed += accel;
                omni_dash_dist += omni_dash_speed;
                omni_dash_speed *= ptype->playerDecay();

                double max_dash_dist = omni_dash_dist;
                if (dash_step > 1 && can_turn_in_one_cycle){
                    accel = turn_dash_max_accel;
                    if (turn_dash_speed + accel > ptype->realSpeedMax(0))
                        accel = ptype->realSpeedMax(0.) - turn_dash_speed;
                    turn_dash_speed += accel;
                    turn_dash_dist += turn_dash_speed;
                    turn_dash_speed *= ptype->playerDecay();
                    if (turn_dash_dist > omni_dash_dist)
                        max_dash_dist = turn_dash_dist;
                } 
                else if (dash_step > 2) {
                    accel = turn_dash_max_accel;
                    if (turn2_dash_speed + accel > ptype->realSpeedMax(0))
                        accel = ptype->realSpeedMax(0.) - turn2_dash_speed;
                    turn2_dash_speed += accel;
                    turn2_dash_dist += turn2_dash_speed;
                    turn2_dash_speed *= ptype->playerDecay();
                    if (turn2_dash_dist > omni_dash_dist)
                        max_dash_dist = turn2_dash_dist;
                }
                
                rel_positions.back().emplace_back(Vector2D::polar2vector(max_dash_dist, dir));
            }
        }

        // Transpose
        for (uint j = 0; j < rel_positions.front().size(); j++) {
            std::vector<Vector2D> vertices;
            for (uint k = 0; k < rel_positions.size(); k++){
                vertices.emplace_back(rel_positions[k][j]);
            }
            ConvexHull *area = new ConvexHull(vertices);
            area->compute();
            convexes_with_body.back().emplace_back(area);
        }
    }
}

ConvexHull*
PlayerPositionConvex::get_convex_with_body(int ptype_id, int pos_count, Vector2D center, AngleDeg rotation) {
    ptype_id = std::max(Hetero_Default, ptype_id);
    std::cout << "PT: " << ptype_id << std::endl
              << "PC: " << pos_count << std::endl
              << "S1: " << convexes_with_body.size() << std::endl
              << "S2: " << convexes_with_body[ptype_id].size() << std::endl;
    if (pos_count <= 0 || pos_count >=10)
        return nullptr;


    const ConvexHull *origin = convexes_with_body[ptype_id][pos_count - 1];
    vector<Vector2D> vertices;
    for (const auto &v: origin->vertices()) {
        vertices.emplace_back(v.rotatedVector(rotation) /*+ center*/);
    }
    ConvexHull *area = new ConvexHull(vertices);
    area->compute();
    return area;

}

ConvexHull*
PlayerPositionConvex::get_convex_without_body(int ptype_id, int pos_count, Vector2D center) {
    ptype_id = std::max(Hetero_Default, ptype_id);
    std::cout << "PT: " << ptype_id << std::endl
              << "PC: " << pos_count << std::endl
              << "S1: " << convexes_with_body.size() << std::endl
              << "S2: " << convexes_with_body[ptype_id].size() << std::endl;
    if (pos_count <= 0 || pos_count >=10)
        return nullptr;

    const ConvexHull* origin = convexes_without_body[ptype_id][pos_count - 1];
    vector<Vector2D> vertices;
    for(const auto& v: origin->vertices()){
        vertices.emplace_back(v /*+ center*/);
    }
    ConvexHull *area = new ConvexHull(vertices);
    area->compute();
    return area;
}


void draw_poly(const Polygon2D &p, const char* color){
    const auto& vertices = p.vertices();
    for(uint i = 0; i < vertices.size()-1; i++){
        dlog.addLine(Logger::WORLD, vertices[i], vertices[i+1], color);
    }
    dlog.addLine(Logger::WORLD, vertices[0], vertices[vertices.size() - 1], color);
}

PlayerStateCandidateArea::PlayerStateCandidateArea(Vector2D pos_) {
    pos = pos_;
    cycle = 0;
}

#include <rcsc/player/object_table.h>


PlayerPredictedObjArea::PlayerPredictedObjArea(SideID side_, int unum_) {
    side = side_;
    unum = unum_;
    last_seen_time = GameTime(0, 0);
    std::string file_name = "vertices/v-" + std::to_string(unum-1);
    std::ifstream fin(file_name);
    area = nullptr;
    last_body = 0;
    body_valid = false;

    player_data.init();
}

PlayerPredictedObjArea::PlayerPredictedObjArea() {
}

void PlayerPredictedObjArea::update_candidates(const WorldModel &wm, const PlayerObject *p) {
    #ifdef DEBUG_DENOISE_AREA
    dlog.addText(Logger::WORLD, "########update candidates");
    #endif
    Vector2D rpos = p->pos() - wm.self().pos();
    double seen_dist = p->seen_dist();
    AngleDeg seen_dir = rpos.th();
    double avg_dist;
    double dist_err;
    
    Vector2D move_vector = Vector2D(0, 0);
    if (p->velValid() && p->velCount() <= 1){
        move_vector = p->inertiaFinalPoint() - p->pos();
    }

    if (area == nullptr) {
        if (object_table.getMovableObjInfo(seen_dist,
                                           &avg_dist,
                                           &dist_err)) {
            dd(A);
            std::vector<Vector2D> poses = {
                    wm.self().pos() + Vector2D::polar2vector(avg_dist - dist_err, seen_dir - 0.5),
                    wm.self().pos() + Vector2D::polar2vector(avg_dist + dist_err, seen_dir - 0.5),
                    wm.self().pos() + Vector2D::polar2vector(avg_dist - dist_err, seen_dir + 0.5),
                    wm.self().pos() + Vector2D::polar2vector(avg_dist + dist_err, seen_dir + 0.5),
            };
            ConvexHull tmp(poses);
            tmp.compute();
            area = new Polygon2D(tmp.toPolygon().vertices());
            dd(B);
            #ifdef DEBUG_PLAYER_AREA
            draw_poly(*area, "#FFFFFF");
            #endif
            dd(C);
        }
    }
    else{
        if (object_table.getMovableObjInfo(seen_dist,
                                           &avg_dist,
                                           &dist_err)) {
            dd(D);
            std::vector<Vector2D> poses = {
                    wm.self().pos() + Vector2D::polar2vector(avg_dist - dist_err, seen_dir - 0.5),
                    wm.self().pos() + Vector2D::polar2vector(avg_dist + dist_err, seen_dir - 0.5),
                    wm.self().pos() + Vector2D::polar2vector(avg_dist - dist_err, seen_dir + 0.5),
                    wm.self().pos() + Vector2D::polar2vector(avg_dist + dist_err, seen_dir + 0.5),
            };
            ConvexHull new_area(poses);
            new_area.compute();
            dd(E);
            int index;
            std::cout << "TC: " << wm.time().cycle()  << std::endl
                        << "TS: " << wm.time().stopped() << std::endl
                        << "LC: " << last_seen_time.cycle() << std::endl
                        << "LS: " << last_seen_time.stopped() << std::endl;
            if (wm.time().stopped() > 0){
                index = wm.time().stopped() - last_seen_time.stopped();
                std::cout << "1->index: " << index << std::endl; 
            }
            else{
                index = wm.time().cycle() - last_seen_time.cycle();
                std::cout << "2->index: " << index << std::endl; 
            }
            dd(F);
            #ifdef DEBUG_DENOISE_AREA
            dlog.addText(Logger::WORLD, "unum=%d", unum);
            dlog.addText(Logger::WORLD, "last_time=%d, index=%d",last_seen_time.cycle(), index);
            #endif
            dd(G);
            if (0 <= index && index < 9){
                dd(G2);
                std::vector<Vector2D> vertices;
                const ConvexHull* prob_area;// = player_data.convexes[index];
                dd(H);
                if (body_valid) {
                    dd(I);
                    prob_area = player_data.get_convex_with_body(p->playerTypePtr()->id(), index, p->pos(), last_body); 
                } else {
                    dd(J);
                    prob_area = player_data.get_convex_without_body(p->playerTypePtr()->id(), index, p->pos());
                }
                if (prob_area != nullptr){
                    dd(K);
                    std::cout << "PBV: " << prob_area->vertices().size() << std::endl;
                    std::cout << "AV:  " << area->vertices().size() << std::endl;
                    #ifdef DEBUG_DENOISE_AREA    
                    dlog.addText(Logger::WORLD, "pd.c.v=%d", prob_area->vertices().size());
                    #endif
                    for (auto& center: area->vertices()){
                        for (const auto& v: prob_area->vertices()){
                            vertices.push_back(v + center + move_vector);
                        }
                    }
                    dd(L);
                    ConvexHull area_conv(vertices);
                    area_conv.compute();
                    dd(M);
                    Polygon2D prob_poly(area_conv.toPolygon().vertices());
                    dd(N);
                    Polygon2D mutual_area = mutual_convex(prob_poly, new_area.toPolygon());
                    dd(O);
                    delete area;
                    area = nullptr;
                    #ifdef DEBUG_PLAYER_AREA
                    draw_poly(prob_poly, "#FF0000");
                    draw_poly(new_area.toPolygon(), "#0000FF");
                    #endif
                    if (mutual_area.vertices().size() > 2) {
                        area = new Polygon2D(mutual_area.vertices());
                        dd(P);
                        #ifdef DEBUG_PLAYER_AREA
                        draw_poly(*area, "#000000");
                        #endif
                    }
                }
                else {
                    dd(G3);
                    delete area;
                    dd(G4);
                    area = new Polygon2D(new_area.vertices());
                    dd(G5);
                    #ifdef DEBUG_PLAYER_AREA
                    draw_poly(*area, "#000000");
                    #endif
                }
            }
            else {
                dd(G3);
                delete area;
                dd(G4);
                area = new Polygon2D(new_area.vertices());
                dd(G5);
                #ifdef DEBUG_PLAYER_AREA
                draw_poly(*area, "#000000");
                #endif
            }
        }
    }
    last_seen_time = wm.time();

    if (p->bodyCount() == 0){
        last_body = p->body().degree();
        body_valid = true;
    }
    else
        body_valid = false;

}


Vector2D 
PlayerPredictedObjArea::vertices_avg(){
    Vector2D avg(0, 0);
    for(const auto& v: area->vertices())
        avg += v;
    avg /= (double)(area->vertices().size());
    return avg;
}

Vector2D 
PlayerPredictedObjArea::get_avg(){
    Vector2D avg(0, 0);
    double s = area->area();

    if (s == 0.){
        return vertices_avg();
    }
    
    vector<Vector2D> vs(area->vertices());
    vs.push_back(vs.front()); // 0 == n

    for (int i = 0; i < vs.size()-1; i++) {
        avg.x += (vs[i].x + vs[i+1].x) * (vs[i].x*vs[i+1].y - vs[i+1].x*vs[i].y);
        avg.y += (vs[i].y + vs[i+1].y) * (vs[i].x*vs[i+1].y - vs[i+1].x*vs[i].y);
    }


    avg *= 1/(6*s);

    #ifdef DEBUG_DENOISE_AREA
    dlog.addText(Logger::WORLD, "AREA AVG --------------------------------------------------------------------");
    dlog.addText(Logger::WORLD, "s=%.2f, avg=(%.2f, %.2f);", s, avg.x, avg.y);
    #endif

    return avg;
}

void PlayerPredictedObjArea::update_by_hear(const WorldModel &wm, const PlayerObject *p){
    if (wm.audioMemory().playerTime().cycle() != wm.time().cycle())
        return;
    
    dd(HA);
    auto memory = wm.audioMemory().player();
    Vector2D heard_pos = Vector2D::INVALIDATED;
    double error_dist = 0.25;
    double heard_body = -360.0;
    int heard_pos_count = 0;
    for (auto & a: memory){
        int unum = a.unum_;
        if (p->side() != wm.ourSide())
            unum -= 11;

        if (unum == p->unum()){
            heard_pos = a.pos_;
            error_dist = a.pos_count_ * 0.6 + 0.25;
            heard_body = a.body_;
            heard_pos_count = a.pos_count_;

            // dlog.addCircle(Logger::WORLD, a.pos_, error_dist, 255,0,0);
            break;
        }
    }

    dd(HB);
    if (!heard_pos.isValid())
        return;

    body_valid = false;

    
    std::vector<Vector2D> heard_area_vertices = {
        Vector2D(heard_pos.x + error_dist, heard_pos.y + error_dist),
        Vector2D(heard_pos.x + error_dist, heard_pos.y - error_dist),
        Vector2D(heard_pos.x - error_dist, heard_pos.y - error_dist),
        Vector2D(heard_pos.x - error_dist, heard_pos.y + error_dist),
    };
    ConvexHull tmp(heard_area_vertices);
    tmp.compute();
    Polygon2D heard_area(tmp.toPolygon().vertices());
    dd(HC);


    #ifdef DEBUG_PLAYER_AREA
    draw_poly(heard_area, "#FFFF00");
    #endif

    if (!area){
        area = new Polygon2D(heard_area.vertices());
        return;
    }
    int index = heard_pos_count;
    dd(HD);

    const ConvexHull* prob_area;
    if (body_valid){
        prob_area = player_data.get_convex_without_body(p->playerTypePtr()->id(), index, p->pos());
    } else {
        prob_area = player_data.get_convex_with_body(p->playerTypePtr()->id(), index, p->pos(), last_body); 
    }

    if (prob_area){
        vector<Vector2D> vertices;
        for (auto& center: heard_area.vertices()){
            for (const auto& v: prob_area->vertices()){
                vertices.push_back(v + center);
            }
        }
        ConvexHull tmp(vertices);
        tmp.compute();

        Polygon2D prob_poly(tmp.toPolygon().vertices());
        #ifdef DEBUG_PLAYER_AREA
        draw_poly(prob_poly, "#FF0000");
        #endif

        Polygon2D mutual = mutual_convex(*area, prob_poly);
        delete area;
        if (mutual.vertices().size() > 2){
            area = new Polygon2D(mutual.vertices());
        }
        else{
            area = new Polygon2D(heard_area.vertices());
        }
    }
    else if (index == 0){
        Polygon2D mutual = mutual_convex(*area, heard_area);
        delete area;
        if (mutual.vertices().size() > 2){
            area = new Polygon2D(mutual.vertices());
        }
        else{
            area = new Polygon2D(heard_area.vertices());
        }
    }
    else{
        delete area;
        area = new Polygon2D(heard_area.vertices());
    }
    #ifdef DEBUG_PLAYER_AREA
    draw_poly(*area, "#000000");
    #endif

    last_seen_time = wm.time();
    if (heard_body != -360.0){
        last_body = heard_body;
        body_valid = true;
    }
}

void PlayerPredictedObjArea::update(const WorldModel &wm, const PlayerObject *p, int cluster_count) {
    #ifdef DEBUG_DENOISE_AREA
    dlog.addText(Logger::WORLD, "==================================== %d %d", p->side(), p->unum());
    #endif

    if (p->seenPosCount() == 0) {
        update_candidates(wm, p);
    }
    else if (player_heard(wm, p)){
        update_by_hear(wm, p);
    } 
    else {
    }

    if (area)
        average_pos = get_avg();
    else
        average_pos = p->pos();
    
    Vector2D avg(0, 0);
    std::vector<std::array<double, 2>> pos_arr;
}



void PlayerPredictedObjArea::debug() {
}

PlayerPredictions *
LocalizationDenoiserByArea::create_prediction(SideID side, int unum){
    return new PlayerPredictedObjArea(side, unum);
}


std::string
LocalizationDenoiserByArea::get_model_name(){
    return "Area";
}

