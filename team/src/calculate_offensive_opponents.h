//
// Created by aradf on 7/15/2022.
//

#ifndef SRC_CALCULATE_OFFENSIVE_OPPONENTS_H
#define SRC_CALCULATE_OFFENSIVE_OPPONENTS_H

#include "rcsc/geom/vector_2d.h"
#include "setting.h"
#include "rcsc/player/world_model.h"

using namespace rcsc;

class CalculateOffensiveOpponents {
public:
    vector<int> opponent_observe_count;
    vector<double> opponent_observe_pos_x_sum;
    vector<double> opponent_observe_pos_x_EWMA;

    //int opponent_offense_count;
    vector<int> final_offensive_opponents;
    static const int finalize_cycle = 500;
    static const int use_observation_cycle = 200;
    bool finalized;
    static constexpr double ALPHA = 0.25;

    CalculateOffensiveOpponents() {
        opponent_observe_pos_x_EWMA = vector<double>(11, 0);
        opponent_observe_pos_x_sum = vector<double>(11, 0);
        opponent_observe_count= vector<int>(11, 0);
        //opponent_offense_count = Setting::i()->mDefenseMove->mStaticOffensiveOpp.size();
        finalized = false;

    }


    static CalculateOffensiveOpponents *getInstance() {
        if (CalculateOffensiveOpponents::instance == nullptr)
            CalculateOffensiveOpponents::instance = new CalculateOffensiveOpponents();
        return CalculateOffensiveOpponents::instance;
    }

    static CalculateOffensiveOpponents *instance;

    void update(rcsc::Vector2D pos, int unum) {
        opponent_observe_count[unum - 1]++;
        opponent_observe_pos_x_sum[unum - 1] += pos.x;
        opponent_observe_pos_x_EWMA[unum - 1] = opponent_observe_pos_x_EWMA[unum - 1] * (1 - ALPHA) + pos.x * ALPHA;
    }

    void updatePlayers(const rcsc::WorldModel &wm) {
        if (Setting::i()->mDefenseMove == NULL || Setting::i()->mDefenseMove->mStaticOffensiveOpp.size() == 0)
            return;
        if (finalize(wm))
            return;
        if(wm.gameMode().type() == GameMode::KickOff_ || wm.gameMode().type() == GameMode::KickIn_ || wm.gameMode().type()==GameMode::BeforeKickOff)
            return;
        for (int i = 1; i <= 11; i++) {
            if (wm.theirPlayer(i) != NULL && wm.theirPlayer(i)->posCount() == 0) {
                std::cout << "*" << wm.time().cycle() << " " << wm.self().unum() << ": " << wm.theirPlayer(i)->unum()
                          << " " << wm.theirPlayer(i)->posCount() << std::endl;
                dlog.addText(Logger::TEAM, "CalculateOffensiveOpponents: opponent %d is observed", i);
                update(wm.theirPlayer(i)->pos(), i);
            }
        }
        dlog.addText(Logger::TEAM,
                     __FILE__": (updatePlayers) EWMA %s",
                     CalculateOffensiveOpponents::vec_to_str(opponent_observe_pos_x_EWMA).c_str());
        dlog.addText(Logger::TEAM,
                     __FILE__": (updatePlayers) sums %s",
                     CalculateOffensiveOpponents::vec_to_str(opponent_observe_pos_x_sum).c_str());
        dlog.addText(Logger::TEAM,
                     __FILE__": (updatePlayers) counts %s",
                     CalculateOffensiveOpponents::vec_to_str(opponent_observe_count).c_str());
        if (wm.time().cycle() > use_observation_cycle && wm.gameMode().type() != GameMode::KickOff_ &&
            wm.gameMode().type() != GameMode::KickIn_) {
            vector<double> ewma_copy= vector<double>(11, 0);
            for(int i=0;i<11;i++)
            {
                if(opponent_observe_count[i]>0)
                {
                    ewma_copy[i]=opponent_observe_pos_x_EWMA[i];
                }
                else
                {
                    ewma_copy[i]=9999;
                }
            }
            Setting::i()->mDefenseMove->mStaticOffensiveOpp = find_k_min_element_index(ewma_copy,
                                                                                       Setting::i()->mDefenseMove->mStaticOffensiveOpp.size());
            for (int i = 0; i < Setting::i()->mDefenseMove->mStaticOffensiveOpp.size(); i++)
                Setting::i()->mDefenseMove->mStaticOffensiveOpp[i]++; //adjust to unum

            dlog.addText(Logger::TEAM,
                         __FILE__": (updatePlayers) updated offensive opponents to %s",
                         CalculateOffensiveOpponents::vec_to_str(
                                 Setting::i()->mDefenseMove->mStaticOffensiveOpp).c_str());
        }
    }

    bool finalize(const rcsc::WorldModel &wm) {
        if (finalized)
            return true;
        if (wm.time().cycle() > finalize_cycle &&
            (wm.ball().pos().x > 0 || wm.gameMode().type() != rcsc::GameMode::PlayOn)) {
            vector<double> avg_opponent_observe_pos_x = vector<double>(11, 0);
            for (int i = 0; i < 11; i++) {
                if (opponent_observe_count[i] != 0)
                    avg_opponent_observe_pos_x[i] = opponent_observe_pos_x_sum[i] / opponent_observe_count[i];
                else
                    avg_opponent_observe_pos_x[i] = 9999;
            }

            Setting::i()->mDefenseMove->mStaticOffensiveOpp = find_k_min_element_index(avg_opponent_observe_pos_x,
                                                                                       Setting::i()->mDefenseMove->mStaticOffensiveOpp.size());
            for (int i = 0; i < Setting::i()->mDefenseMove->mStaticOffensiveOpp.size(); i++)
                Setting::i()->mDefenseMove->mStaticOffensiveOpp[i]++; //adjust to unum
            dlog.addText(Logger::TEAM,
                         __FILE__": (finalize) finalized opponents to %s",
                         CalculateOffensiveOpponents::vec_to_str(
                                 Setting::i()->mDefenseMove->mStaticOffensiveOpp).c_str());
            finalized = true;
            return true;
        }
        return false;
    }

private:
    static string vec_to_str(vector<int> vec) {
        string str = "[";
        for (int i = 0; i < vec.size(); i++) {
            str += to_string(i) + ":" + to_string(vec[i]);
            if (i != vec.size() - 1)
                str += ", ";
        }
        str += "]";
        return str;
    }
    static string vec_to_str(vector<double> vec) {
        string str = "[";
        for (int i = 0; i < vec.size(); i++) {
            str += to_string(i) + ":" + to_string(vec[i]);
            if (i != vec.size() - 1)
                str += ", ";
        }
        str += "]";
        return str;
    }

    //generated by github copilot, seems to work, dont know how
    //TODO understand this code
    static vector<int> find_k_min_element_index(vector<double> &vec, int k) {
        vector<int> index;
        vector<double> copy = vec;
        sort(copy.begin(), copy.end());
        for (int i = 0; i < k; i++) {
            for (int j = 0; j < vec.size(); j++) {
                if (vec[j] == copy[i]) {
                    index.push_back(j);
                    break;
                }
            }
        }
        return index;
    }

    //could be improved by using a heap
    //could be useful for the future, detecting defensive players
    static vector<int> find_k_max_element_index(vector<double> &v, int k) {
        vector<int> index;
        vector<double> v_copy = v;
        sort(v_copy.begin(), v_copy.end());
        for (int i = 0; i < k; i++) {
            for (int j = 0; j < v.size(); j++) {
                if (v_copy[v_copy.size() - 1 - i] == v[j]) {
                    index.push_back(j);
                    break;
                }
            }
        }
        return index;
    }
};

#endif //SRC_CALCULATE_OFFENSIVE_OPPONENTS_H
