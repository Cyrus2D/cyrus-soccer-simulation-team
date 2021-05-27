//
// Created by nader on 2020-10-17.
//

#include "best_match_finder.h"
#include <sstream>
#include <algorithm>
#include <math.h>
#include <rcsc/common/logger.h>

using namespace rcsc;

#define DEBUG_PRINT

void BestMatchFinder::fun(vector<vector<int> > &tasks_best_agents,
                          vector<size_t> &sorted_tasks,
                          vector<int> actions,
                          int pointer,
                          double mark_eval[12][12],
                          double &best_actions_cost,
                          vector<int> &best_actions) {
    stringstream log_str;
    double actions_cost = 0;
    double actions_cost_pointer = 0;
    for (int a = 0; a < actions.size(); a++) {
        double e = 0;
        if (actions[a] != 0) {
            e = mark_eval[sorted_tasks[a]][actions[a]];
        }
        else {
            e = 100 * (actions.size() - a);//std::pow(100, actions.size() - a);
        }
        actions_cost += e;
    }
    for (int a = 0; a < pointer; a++) {
        double e = 0;
        if (actions[a] != 0) {
            e = mark_eval[sorted_tasks[a]][actions[a]];
        }
        else {
            e = 100 * (actions.size() - a);//std::pow(100, actions.size() - a);
        }
        actions_cost_pointer += e;
    }
    #ifdef DEBUG_PRINT
    log_str << "actions:opp:tm, ...";
    for (int i = 0; i < actions.size(); i++) {
        log_str << sorted_tasks[i] << ":" << actions[i] << ", ";
    }
    log_str << " costs: " << actions_cost << " costs pointer: " << actions_cost_pointer << " best: " << best_actions_cost << " p: "<<pointer<< endl;
    string str = log_str.str();
    dlog.addText(Logger::MARK, str.c_str());
    #endif
    if (actions_cost < best_actions_cost) {
        best_actions_cost = actions_cost;
        best_actions = actions;
    }
    if (actions_cost_pointer < best_actions_cost && pointer < actions.size()){
        for (int a = 0; a < tasks_best_agents[pointer].size(); a++) {
            int agent = tasks_best_agents[pointer][a];
            if (agent != 0) {
                bool agent_used = false;
                for (int i = 0; i < actions.size(); i++) {
                    if (agent == actions[i])
                        agent_used = true;
                }
                if (agent_used)
                    continue;
            }
            actions[pointer] = agent;
            fun(tasks_best_agents, sorted_tasks, actions, pointer + 1, mark_eval, best_actions_cost, best_actions);
            actions[pointer] = 0;
        }
    }

}


pair<vector<int>, double>
BestMatchFinder::find_best_dec(double mark_eval[12][12], vector<size_t> agents, vector<size_t> tasks) {
    #ifdef DEBUG_PRINT
    dlog.addText(Logger::MARK, "tmm agents count: %d", agents.size());
    for (int i = 0; i < agents.size(); i++) {
        dlog.addText(Logger::MARK, "agents:%d", agents[i]);
    }
    dlog.addText(Logger::MARK, "opp tasks count: %d", tasks.size());
    for (int i = 0; i < tasks.size(); i++) {
        dlog.addText(Logger::MARK, "tasks:%d", tasks[i]);
    }
    #endif
    if (tasks.size() == 0 || agents.size() == 0)
        return make_pair(vector<int>(), 100000);
    vector<vector<int> > tasks_best_agents;
    for (int o = 0; o < tasks.size(); o++) {
        int task = tasks[o];
        #ifdef DEBUG_PRINT
        dlog.addText(Logger::MARK, "task danger=> id:%d , unum:%d", o, task);
        #endif
        tasks_best_agents.push_back(vector<int>());

        vector<pair<double, int>> task_costs;
        for (int agent = 1; agent <= 11; agent++) {
            if (find(agents.begin(), agents.end(), agent) != agents.end())
                task_costs.push_back(pair<double, int>(mark_eval[task][agent], agent));
        }
        sort(task_costs.begin(), task_costs.end());
        for (int i = 0; i < 3; i++) {
            if (task_costs[i].first == 1000)
                break;
            tasks_best_agents[tasks_best_agents.size() - 1].push_back(task_costs[i].second);
            #ifdef DEBUG_PRINT
            dlog.addText(Logger::MARK, "agent%d:%d , eval:%.2f", i, task_costs[i].second, task_costs[i].first);
            #endif
            if (i == task_costs.size() - 1)
                break;
        }
        tasks_best_agents[tasks_best_agents.size() - 1].push_back(0);
    }
    #ifdef DEBUG_PRINT
    for (int i = 0; i < tasks_best_agents.size(); i++) {
        dlog.addText(Logger::MARK, "task:%d", tasks[i]);
        for (int j = 0; j < tasks_best_agents[i].size(); j++) {
            dlog.addText(Logger::MARK, "---- %d", tasks_best_agents[i][j]);
        }
    }
    dlog.addText(Logger::MARK, "start Back");
    #endif

    vector<int> action(tasks_best_agents.size(), 0);
    vector<int> best_actions(tasks_best_agents.size(), 0);
    double best_actions_cost = 10000000;
    fun(tasks_best_agents, tasks, action, 0, mark_eval, best_actions_cost, best_actions);

    #ifdef DEBUG_PRINT
    dlog.addText(Logger::MARK, "best action:");
    for (int i = 0; i < best_actions.size(); i++) {
        dlog.addText(Logger::MARK, "----%d", best_actions[i]);
    }
    #endif
    return make_pair(best_actions, best_actions_cost);
}
