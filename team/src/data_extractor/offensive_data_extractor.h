//
// Created by aref on 11/28/19.
//

#ifndef CYRUS_OffensiveDataExtractor_H
#define CYRUS_OffensiveDataExtractor_H

#include <fstream>
#include <rcsc/geom.h>
#include <rcsc/player/player_agent.h>

#include "DEState.h"
//#include "shoot_generator.h"
enum ODEDataSide {
    NONE,
    TM,
    OPP,
    BOTH,
    Kicker
};

enum ODEWorldMode {
    FULLSTATE,
    NONE_FULLSTATE
};

enum ODEPlayerSortMode {
    X,
    ANGLE,
    UNUM,
    RANDOM,
};

class OffensiveDataExtractor {
private:
    struct Option {
    public:
        ODEDataSide side;
        ODEDataSide unum;
        ODEDataSide type;
        ODEDataSide body;
        ODEDataSide face;
        ODEDataSide tackling;
        ODEDataSide kicking;
        ODEDataSide card;
        ODEDataSide pos;
        ODEDataSide relativePos;
        ODEDataSide polarPos;
        ODEDataSide vel;
        ODEDataSide polarVel;
        ODEDataSide counts;
        ODEDataSide isKicker;
        ODEDataSide isGhost;
        ODEDataSide openAnglePass;
        ODEDataSide nearestOppDist;
        ODEDataSide polarGoalCenter;
        ODEDataSide openAngleGoal;
        ODEDataSide in_offside;

        ODEDataSide dribleAngle;
        int nDribleAngle;

        ODEWorldMode input_worldMode;
        ODEWorldMode output_worldMode;


        ODEPlayerSortMode playerSortMode;
        bool kicker_first;
        bool use_convertor;
        int history_size;
        Option();
    };

private:
    std::vector<double> features;
    std::ofstream fout;
    long last_update_cycle;
    std::vector<double> data;

public:

    OffensiveDataExtractor();

    ~OffensiveDataExtractor();
    Option option;
//    void update(const rcsc::PlayerAgent *agent,
//                const ActionStatePair *first_layer,
//                bool update_shoot=false);


    //accessors
    static OffensiveDataExtractor &i();
    static bool active;

    void extract_output(const DEState &state, int category, const rcsc::Vector2D &target, const int &unum,
                        const char *desc, double bell_speed);
    void update_for_shoot(const rcsc::PlayerAgent *agent, rcsc::Vector2D target, double bell_speed);

    void update_history(const rcsc::PlayerAgent *agent);

    void get_data(const DEState & state);
private:
    void init_file(const DEState &state);

    void extract_ball(const DEState & state);

    void extract_players(const DEState & state);

    void add_null_player(int unum, ODEDataSide side);

    void extract_pos(const DEPlayer *player, const DEState & state, ODEDataSide side);

    void extract_vel(const DEPlayer *player, ODEDataSide side, const DEState & state);

    void extract_pass_angle(const DEPlayer *player, const DEState & state, ODEDataSide side);

    void extract_goal_polar(const DEPlayer *player, ODEDataSide side);

    void extract_goal_open_angle(const DEPlayer *player, const DEState & state, ODEDataSide side);

    void extract_base_data(const DEPlayer *player, ODEDataSide side, const DEState & state);

    void extract_type(const DEPlayer *player, ODEDataSide side);

    void extract_history(const DEPlayer *player, ODEDataSide side);


    uint find_unum_index(const DEState & state, uint unum);

    double convertor_x(double x);

    double convertor_y(double y);

    double convertor_dist(double dist);

    double convertor_dist_x(double dist);

    double convertor_dist_y(double dist);

    double convertor_angle(double angle);

    double convertor_type(double type);

    double convertor_cycle(double cycle);

    double convertor_bv(double bv);

    double convertor_bvx(double bvx);

    double convertor_bvy(double bvy);

    double convertor_pv(double pv);

    double convertor_pvx(double pvx);

    double convertor_pvy(double pvy);

    double convertor_unum(double unum);

    double convertor_card(double card);

    double convertor_stamina(double stamina);

    double convertor_counts(double count);

    void extract_counts(const DEPlayer *player, ODEDataSide side, const DEState &state);

    void extract_kicker(const DEState &state);

    void extract_drible_angles(const DEState &state);

    std::vector<const DEPlayer *> sort_players(const DEState &state);
    std::vector<const DEPlayer *> sort_players2(const DEState &state);
    std::vector<const DEPlayer *> sort_players3(const DEState &state);
    static std::vector<std::vector<rcsc::Vector2D>> history_pos;
    static std::vector<std::vector<rcsc::Vector2D>> history_vel;
    static std::vector<std::vector<rcsc::AngleDeg>> history_body;
    static std::vector<std::vector<int>> history_pos_count;
    static std::vector<std::vector<int>> history_vel_count;
    static std::vector<std::vector<int>> history_body_count;
};

class ODEPolar {
public:
    double r;
    double teta;

    ODEPolar(rcsc::Vector2D p);
};


#endif //CYRUS_OffensiveDataExtractor_H
