//
// Created by aref on 11/28/19.
//

#ifndef CYRUS_OffensiveDataExtractor_H
#define CYRUS_OffensiveDataExtractor_H

#include <fstream>
#include <rcsc/geom.h>
#include <rcsc/player/player_agent.h>
//#include "../chain_action/action_state_pair.h"
#include "../chain_action/cooperative_action.h"
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
        bool cycle;
        bool ball_pos;
        bool ball_vel;
        bool ball_kicker_pos;
        bool offside_count;
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
        ODEDataSide pos_counts;
        ODEDataSide vel_counts;
        ODEDataSide body_counts;
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
    void update(const rcsc::PlayerAgent *agent,
                const CooperativeAction &action,
                bool update_shoot=false);
    std::string get_header();

    //accessors
    static OffensiveDataExtractor &i();
    static bool active;

    void extract_output(DEState &state,
                        int category,
                        const rcsc::Vector2D &target,
                        const int &unum,
                        const char *desc,
                        double bell_speed);
    void update_for_shoot(const rcsc::PlayerAgent *agent, rcsc::Vector2D target, double bell_speed);

    void update_history(const rcsc::PlayerAgent *agent);

    std::vector<double> get_data(DEState &state);
private:
    void init_file(DEState &state);

    void extract_ball(DEState &state);

    void extract_players(DEState &state);

    void add_null_player(int unum, ODEDataSide side);

    void extract_pos(DEPlayer *player, DEState &state, ODEDataSide side);

    void extract_vel(DEPlayer *player, ODEDataSide side, DEState &state);

    void extract_pass_angle(DEPlayer *player, DEState &state, ODEDataSide side);

    void extract_goal_polar(DEPlayer *player, ODEDataSide side);

    void extract_goal_open_angle(DEPlayer *player, DEState &state, ODEDataSide side);

    void extract_base_data(DEPlayer *player, ODEDataSide side, DEState &state);

    void extract_type(DEPlayer *player, ODEDataSide side);

    void extract_history(DEPlayer *player, ODEDataSide side);


    uint find_unum_index(DEState &state, uint unum);

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

    void extract_counts(DEPlayer *player, ODEDataSide side, DEState &state);

    void extract_kicker(DEState &state);

    void extract_drible_angles(DEState &state);

    std::vector<DEPlayer *> sort_players(DEState &state);
    std::vector<DEPlayer *> sort_players2(DEState &state);
    std::vector<DEPlayer *> sort_players3(DEState &state);
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
