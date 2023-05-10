//
// Created by nader on 2022-04-28.
//

#include "offensive_data_extractor_v1.h"

void OffensiveDataExtractorV1::setOptions(){
    option.cycle = true; //
    option.ball_pos = true;
    option.ball_kicker_pos = false;
    option.ball_vel = false;
    option.offside_count = true;
    option.side = NONE;
    option.unum = BOTH;
    option.type = NONE;
    option.body = BOTH;
    option.face = NONE;
    option.tackling = NONE;
    option.kicking = NONE;
    option.card = NONE;
    option.pos = BOTH;
    option.relativePos = BOTH;
    option.polarPos = BOTH;
    option.vel = NONE;
    option.polarVel = NONE;
    option.pos_counts = NONE;
    option.vel_counts = NONE;
    option.body_counts = NONE;
    option.in_offside = TM;
    option.isKicker = TM;
    option.isGhost = TM;
    option.openAnglePass = TM;
    option.nearestOppDist = TM;
    option.polarGoalCenter = NONE;
    option.openAngleGoal = TM;
    option.dribleAngle = NONE;
    option.nDribleAngle = 12;
    option.history_size = 0;
    option.input_worldMode = NONE_FULLSTATE;
    option.output_worldMode = NONE_FULLSTATE;
    option.playerSortMode = X;
    option.kicker_first = false;
    option.use_convertor = false;
}

OffensiveDataExtractorV1 &OffensiveDataExtractorV1::i() {
    static OffensiveDataExtractorV1 instance;
    return instance;
}