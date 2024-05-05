//
// Created by nader on 2020-10-17.
//

#ifndef CYRUS_MARK_POSITION_FINDER_H
#define CYRUS_MARK_POSITION_FINDER_H

#include "move_def/bhv_mark_decision_greedy.h"
class MarkPositionFinder{
public:
    static Target getLeadProjectionMarkTarget(int tmUnum, int oppUnum, const WorldModel &wm);

    static Target getLeadNearMarkTarget(int tmUnum, int oppUnum, const WorldModel &wm);

    static Target getThMarkTarget(size_t tmUnum, size_t oppUnum, const WorldModel &wm, bool debug = false);

    static Target getThMarkTarget2(size_t tmUnum, size_t oppUnum, const WorldModel &wm, bool debug = false);

    static Target getDengerMarkTarget(int tmUnum, int oppUnum, const WorldModel &wm);

};
#endif //CYRUS_MARK_POSITION_FINDER_H
