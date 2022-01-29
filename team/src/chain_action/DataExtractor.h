//
// Created by aref on 11/28/19.
//

#ifndef CYRUS_DATAEXTRACTOR_H
#define CYRUS_DATAEXTRACTOR_H

#include <fstream>
#include <rcsc/geom.h>
#include <rcsc/player/player_agent.h>
#include "action_state_pair.h"
//#include "shoot_generator.h"
enum DataSide {
    NONE,
    TM,
    OPP,
    BOTH,
    Kicker
};

enum WorldMode {
    FULLSTATE,
    NONE_FULLSTATE
};

enum PlayerSortMode {
    X,
    ANGLE,
    UNUM,
    RANDOM,
};

class DataExtractor {
private:
    struct Option {
    public:
        DataSide side;
        DataSide unum;
        DataSide type;
        DataSide body;
        DataSide face;
        DataSide tackling;
        DataSide kicking;
        DataSide card;
        DataSide pos;
        DataSide relativePos;
        DataSide polarPos;
        DataSide vel;
        DataSide polarVel;
        DataSide counts;
        DataSide isKicker;
        DataSide openAnglePass;
        DataSide nearestOppDist;
        DataSide polarGoalCenter;
        DataSide openAngleGoal;
        DataSide in_offside;

        DataSide dribleAngle;
        int nDribleAngle;

        WorldMode input_worldMode;
        WorldMode output_worldMode;


        PlayerSortMode playerSortMode;
        bool kicker_first;
        bool use_convertor;
        int history_size;
        Option();
    };

private:
    std::ofstream fout;
    long last_update_cycle;
    std::vector<double> data;

public:

    DataExtractor();

    ~DataExtractor();
    Option option;
    void update(const rcsc::PlayerAgent *agent,
                const ActionStatePair *first_layer,
                bool update_shoot=false);


    //accessors
    static DataExtractor &i();

    void extract_output(const rcsc::WorldModel &wm, int category, const rcsc::Vector2D &target, const int &unum,
                        const char *desc, double bell_speed);
    void update_for_shoot(const rcsc::PlayerAgent *agent, rcsc::Vector2D target, double bell_speed);

    void update_history(const rcsc::PlayerAgent *agent);
private:
    void init_file(const rcsc::WorldModel &wm);

    void extract_ball(const rcsc::WorldModel &wm);

    void extract_players(const rcsc::WorldModel &wm);

    void add_null_player(int unum, DataSide side);

    void extract_pos(const rcsc::AbstractPlayerObject *player, const rcsc::WorldModel &wm, DataSide side);

    void extract_vel(const rcsc::AbstractPlayerObject *player, DataSide side);

    void extract_pass_angle(const rcsc::AbstractPlayerObject *player, const rcsc::WorldModel &wm, DataSide side);

    void extract_goal_polar(const rcsc::AbstractPlayerObject *player, DataSide side);

    void extract_goal_open_angle(const rcsc::AbstractPlayerObject *player, const rcsc::WorldModel &wm, DataSide side);

    void extract_base_data(const rcsc::AbstractPlayerObject *player, DataSide side);

    void extract_type(const rcsc::AbstractPlayerObject *player, DataSide side);

    void extract_history(const rcsc::AbstractPlayerObject *player, DataSide side);


    uint find_unum_index(const rcsc::WorldModel &wm, uint unum);

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

    void extract_counts(const rcsc::AbstractPlayerObject *player, DataSide side);

    void extract_kicker(const rcsc::WorldModel &wm);

    void extract_drible_angles(const rcsc::WorldModel &wm);

    std::vector<const rcsc::AbstractPlayerObject *> sort_players(const rcsc::WorldModel &wm);
    static std::vector<std::vector<rcsc::Vector2D>> history_pos;
    static std::vector<std::vector<rcsc::Vector2D>> history_vel;
    static std::vector<std::vector<rcsc::AngleDeg>> history_body;
    static std::vector<std::vector<int>> history_pos_count;
    static std::vector<std::vector<int>> history_vel_count;
    static std::vector<std::vector<int>> history_body_count;
};

class Polar {
public:
    double r;
    double teta;

    Polar(rcsc::Vector2D p);
};


#endif //CYRUS_DATAEXTRACTOR_H
