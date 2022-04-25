#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"

#include <vector>
#include <iostream>
#include <string>
#include <istream>
#include <map>
#include <cstdio>

using namespace std;
using namespace rapidjson;
class ChainActionSetting{
public:
    ChainActionSetting(){}
    ChainActionSetting(Value & value);
    int mChainDeph = 2;
    int mChainNodeNumber = 2000;
    bool mUseShootSafe = false;
    double mDribblePosCountZ = 0.8;
    int mDribblePosCountMaxFrontOpp = 8;
    int mDribblePosCountMaxBehindOpp = 4;
};

class StrategySetting{
public:
    StrategySetting(){}
    StrategySetting(Value & value);
    string mWinFormation = "433";
    string mLostFormation = "433";
    string mDrawFormation = "433";
    string mTeamTactic = "Normal";
    bool mIs5ForwardWin = true;
    bool mIs5ForwardDraw = true;
    bool mIs5ForwardLost = true;
    bool mIsGoalForward = false;
};

class OffensiveMoveSetting{
public:
    OffensiveMoveSetting(){}
    OffensiveMoveSetting(Value & value);
    bool mIs9BrokeOffside = false;
    vector<string> mUnmarkingAlgorithms;
    string mMainUnmarkPassPredictionDNN;
    bool mUseUnmarkPassPredictionDNN = false;
};

class DefenseMoveSetting{
public:
    DefenseMoveSetting(){}
    DefenseMoveSetting(Value & value);
    bool mUsePassBlock = false;
    double mStartMidMark = -30.0;
    vector<int> mStaticOffensiveOpp;
    bool mMidTh_BackInMark = true;
    bool mMidTh_BackInBlock = true;
    bool mMidTh_HalfInMark = true;
    bool mMidTh_HalfInBlock = true;
    bool mMidTh_ForwardInMark = false;
    bool mMidTh_ForwardInBlock = true;
    bool mMidTh_RemoveNearOpps = true;
    double mMidTh_DistanceNearOpps = 5.0;
    double mMidTh_XNearOpps = 3.0;
    double mMidTh_PosDistZ = 1.0;
    double mMidTh_HPosDistZ = 0.7;
    double mMidTh_PosMaxDistMark = 15.0;
    double mMidTh_HPosMaxDistMark = 15.0;
    double mMidTh_HPosYMaxDistMark = 7.0;
    double mMidTh_PosMaxDistBlock = 20.0;
    double mMidTh_HPosMaxDistBlock = 20.0;
    double mMidTh_HPosYMaxDistBlock = 15.0;
    double mMidProj_PosMaxDistMark = 10.0;
    double mMidProj_HPosMaxDistMark = 10.0;
    double mMidProj_PosMaxDistBlock = 25.0;
    double mMidProj_HPosMaxDistBlock = 25.0;
    bool mMid_UseProjectionMark = true;
    double mMidNear_StartX = 30.0;
    bool mMidNear_BackInMark = false;
    bool mMidNear_BackInBlock = false;
    bool mMidNear_HalfInMark = true;
    bool mMidNear_HalfInBlock = true;
    bool mMidNear_ForwardInMark = true;
    bool mMidNear_ForwardInBlock = true;
    double mMidNear_OppsDistXToBall = 25.0;
    bool mMidNear_MarkAgain = true;
    bool mMidNear_BlockAgain = true;
    double mMidNear_MarkAgainMaxDistToChangeCost = 5.0;
    double mMidNear_MarkAgainChangeCostZ = 1.4;
    double mMidNear_PosMaxDistMark = 10.0;
    double mMidNear_HPosMaxDistMark = 15.0;
    double mMidNear_PosMaxDistBlock = 20.0;
    double mMidNear_HPosMaxDistBlock = 20.0;
    bool mGoal_ForwardInMark = false;
    bool mGoal_ForwardInBlock = true;
    double mGoal_PosMaxDistMark = 10.0;
    double mGoal_HPosMaxDistMark = 10.0;
    double mGoal_OffsideMaxDistMark = 10.0;
    double mGoal_PosMaxDistBlock = 25.0;
    double mGoal_HPosMaxDistBlock = 25.0;
    double mGoal_OffsideMaxDistBlock = 25.0;
};

class Setting{
public:
    Setting(){
        mStrategySetting = new StrategySetting();
        mChainAction = new ChainActionSetting();
        mOffensiveMove = new OffensiveMoveSetting();
        mDefenseMove = new DefenseMoveSetting();
    }
    static Setting * i(){
        if(instance == nullptr)
            instance = new Setting();
        return instance;
    }
    void SetTeamName(string teamName);
    void ReadJson();
    static Setting * instance;
    map<string, string> mTeamsConfig;
    string mJsonPath = "./data/settings/Other.json";
    bool mSeted = false;
    StrategySetting * mStrategySetting;
    ChainActionSetting * mChainAction;
    OffensiveMoveSetting * mOffensiveMove;
    DefenseMoveSetting * mDefenseMove;
};
