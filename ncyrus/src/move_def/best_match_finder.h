//
// Created by nader on 2020-10-17.
//

#ifndef CYRUS_BEST_MATCH_FINDER_H
#define CYRUS_BEST_MATCH_FINDER_H

#include <vector>
using namespace std;
class BestMatchFinder{
public:
    static void fun(vector<vector<int> > &best_marker,
             vector<size_t> &opp_sorted,
             vector<int> action,
             int pointer,
             double mark_eval[12][12],
             double &best_eval,
             vector<int> &best_action);

    static pair<vector<int>, double> find_best_dec(double mark_eval[12][12], vector<size_t> marker, vector<size_t> marked);
};

#endif //CYRUS_BEST_MATCH_FINDER_H
