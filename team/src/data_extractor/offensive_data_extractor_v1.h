//
// Created by nader on 2022-04-28.
//

#ifndef TEAM_OFFENSIVE_DATA_EXTRACTOR_V_1_H
#define TEAM_OFFENSIVE_DATA_EXTRACTOR_V_1_H
#include "offensive_data_extractor.h"

class OffensiveDataExtractorV1: public OffensiveDataExtractor{
public:
    OffensiveDataExtractorV1(){
        setOptions();
    }
    void setOptions();
    static OffensiveDataExtractorV1 &i();
};
#endif //TEAM_OFFENSIVE_DATA_EXTRACTOR_V_1_H
