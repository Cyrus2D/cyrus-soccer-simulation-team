#include "setting.h"
using namespace std;
using namespace rapidjson;


ChainActionSetting::ChainActionSetting(Value & value){
    if(value.HasMember("ChainDeph"))
    {
        mChainDeph = value["ChainDeph"].GetInt();
    }
    if(value.HasMember("ChainNodeNumber"))
    {
        mChainNodeNumber = value["ChainNodeNumber"].GetInt();
    }
    if(value.HasMember("UseShootSafe"))
    {
        mUseShootSafe = value["UseShootSafe"].GetBool();
    }
    if(value.HasMember("DribblePosCountZ"))
    {
        mDribblePosCountZ = value["DribblePosCountZ"].GetFloat();
    }
    if(value.HasMember("DribblePosCountMaxFrontOpp"))
    {
        mDribblePosCountMaxFrontOpp = value["DribblePosCountMaxFrontOpp"].GetInt();
    }
    if(value.HasMember("DribblePosCountMaxBehindOpp"))
    {
        mDribblePosCountMaxBehindOpp = value["DribblePosCountMaxBehindOpp"].GetInt();
    }
}

StrategySetting::StrategySetting(Value & value){
    if(value.HasMember("Formation")){
        auto & o = (value.GetObject())["Formation"];
        if(o.HasMember("Win")){
            mWinFormation = o["Win"].GetString();
        }
        if(o.HasMember("Lost")){
            mLostFormation = o["Lost"].GetString();
        }
        if(o.HasMember("Draw")){
            mDrawFormation = o["Draw"].GetString();
        }
    }
    if(value.HasMember("Is5ForwardWin")){
        mIs5ForwardWin = value["Is5ForwardWin"].GetBool();
    }
    if(value.HasMember("Is5ForwardDraw")){
        mIs5ForwardDraw = value["Is5ForwardDraw"].GetBool();
    }
    if(value.HasMember("Is5ForwardLost")){
        mIs5ForwardLost = value["Is5ForwardLost"].GetBool();
    }
    if(value.HasMember("TeamTactic")){
        mTeamTactic = value["TeamTactic"].GetString();
    }
    if(value.HasMember("IsGoalForward")){
        mIsGoalForward = value["IsGoalForward"].GetBool();
    }
}

OffensiveMoveSetting::OffensiveMoveSetting(Value &value)
{
    if(value.HasMember("Is9BrokeOffside"))
    {
        mIs9BrokeOffside = value["Is9BrokeOffside"].GetBool();
    }
}

DefenseMoveSetting::DefenseMoveSetting(Value & value){
    if(value.HasMember("PassBlock")){
        mUsePassBlock = value["PassBlock"].GetBool();
    }
    if(value.HasMember("StartMidMark")){
        mStartMidMark = value["StartMidMark"].GetDouble();
    }
    if(value.HasMember("StaticOffensiveOpp")){
        for (SizeType i = 0; i < value["StaticOffensiveOpp"].GetArray().Size(); i++){
            mStaticOffensiveOpp.push_back(value["StaticOffensiveOpp"].GetArray()[i].GetInt());
        }

    }
    if(value.HasMember("MidTh_BackInMark")){
        mMidTh_BackInMark = value["MidTh_BackInMark"].GetBool();
    }
    if(value.HasMember("MidTh_BackInBlock")){
        mMidTh_BackInBlock = value["MidTh_BackInBlock"].GetBool();
    }
    if(value.HasMember("MidTh_HalfInMark")){
        mMidTh_HalfInMark = value["MidTh_HalfInMark"].GetBool();
    }
    if(value.HasMember("MidTh_HalfInBlock")){
        mMidTh_HalfInBlock = value["MidTh_HalfInBlock"].GetBool();
    }
    if(value.HasMember("MidTh_ForwardInMark")){
        mMidTh_ForwardInMark = value["MidTh_ForwardInMark"].GetBool();
    }
    if(value.HasMember("MidTh_ForwardInBlock")){
        mMidTh_ForwardInBlock = value["MidTh_ForwardInBlock"].GetBool();
    }
    if(value.HasMember("MidTh_RemoveNearOpps")){
        mMidTh_RemoveNearOpps = value["MidTh_RemoveNearOpps"].GetBool();
    }
    if(value.HasMember("MidTh_DistanceNearOpps")){
        mMidTh_DistanceNearOpps = value["MidTh_DistanceNearOpps"].GetDouble();
    }
    if(value.HasMember("MidTh_XNearOpps")){
        mMidTh_XNearOpps = value["MidTh_XNearOpps"].GetDouble();
    }
    if(value.HasMember("MidTh_PosDistZ")){
        mMidTh_PosDistZ = value["MidTh_PosDistZ"].GetDouble();
    }
    if(value.HasMember("MidTh_HPosDistZ")){
        mMidTh_HPosDistZ = value["MidTh_HPosDistZ"].GetDouble();
    }
    if(value.HasMember("MidTh_PosMaxDistMark")){
        mMidTh_PosMaxDistMark = value["MidTh_PosMaxDistMark"].GetDouble();
    }
    if(value.HasMember("MidTh_HPosMaxDistMark")){
        mMidTh_HPosMaxDistMark = value["MidTh_HPosMaxDistMark"].GetDouble();
    }
    if(value.HasMember("MidTh_HPosYMaxDistMark")){
        mMidTh_HPosYMaxDistMark = value["MidTh_HPosYMaxDistMark"].GetDouble();
    }
    if(value.HasMember("MidTh_PosMaxDistBlock")){
        mMidTh_PosMaxDistBlock = value["MidTh_PosMaxDistBlock"].GetDouble();
    }
    if(value.HasMember("MidTh_HPosMaxDistBlock")){
        mMidTh_HPosMaxDistBlock = value["MidTh_HPosMaxDistBlock"].GetDouble();
    }
    if(value.HasMember("MidTh_HPosYMaxDistBlock")){
        mMidTh_HPosYMaxDistBlock = value["MidTh_HPosYMaxDistBlock"].GetDouble();
    }
    if(value.HasMember("Mid_UseProjectionMark")){
        mMid_UseProjectionMark = value["Mid_UseProjectionMark"].GetBool();
    }
    if(value.HasMember("MidProj_PosMaxDistMark")){
        mMidProj_PosMaxDistMark = value["MidProj_PosMaxDistMark"].GetDouble();
    }
    if(value.HasMember("MidProj_HPosMaxDistBlock")){
        mMidProj_HPosMaxDistBlock = value["MidProj_HPosMaxDistBlock"].GetDouble();
    }
    if(value.HasMember("MidProj_PosMaxDistMark")){
        mMidProj_PosMaxDistMark = value["MidProj_PosMaxDistMark"].GetDouble();
    }
    if(value.HasMember("MidProj_HPosMaxDistBlock")){
        mMidProj_HPosMaxDistBlock = value["MidProj_HPosMaxDistBlock"].GetDouble();
    }
    if(value.HasMember("MidNear_StartX")){
        mMidNear_StartX = value["MidNear_StartX"].GetDouble();
    }
    if(value.HasMember("MidNear_BackInMark")){
        mMidNear_BackInMark = value["MidNear_BackInMark"].GetBool();
    }
    if(value.HasMember("MidNear_BackInBlock")){
        mMidNear_BackInBlock = value["MidNear_BackInBlock"].GetBool();
    }
    if(value.HasMember("MidNear_HalfInMark")){
        mMidNear_HalfInMark = value["MidNear_HalfInMark"].GetBool();
    }
    if(value.HasMember("MidNear_HalfInBlock")){
        mMidNear_HalfInBlock = value["MidNear_HalfInBlock"].GetBool();
    }
    if(value.HasMember("MidNear_ForwardInMark")){
        mMidNear_ForwardInMark = value["MidNear_ForwardInMark"].GetBool();
    }
    if(value.HasMember("MidNear_ForwardInBlock")){
        mMidNear_ForwardInBlock = value["MidNear_ForwardInBlock"].GetBool();
    }
    if(value.HasMember("MidNear_OppsDistXToBall")){
        mMidNear_OppsDistXToBall = value["MidNear_OppsDistXToBall"].GetDouble();
    }
    if(value.HasMember("MidNear_MarkAgain")){
        mMidNear_MarkAgain = value["MidNear_MarkAgain"].GetBool();
    }
    if(value.HasMember("MidNear_BlockAgain")){
        mMidNear_BlockAgain = value["MidNear_BlockAgain"].GetBool();
    }
    if(value.HasMember("MidNear_MarkAgainMaxDistToChangeCost")){
        mMidNear_MarkAgainMaxDistToChangeCost = value["MidNear_MarkAgainMaxDistToChangeCost"].GetDouble();
    }
    if(value.HasMember("MidNear_MarkAgainChangeCostZ")){
        mMidNear_MarkAgainChangeCostZ = value["MidNear_MarkAgainChangeCostZ"].GetDouble();
    }
    if(value.HasMember("MidNear_PosMaxDistMark")){
        mMidNear_PosMaxDistMark = value["MidNear_PosMaxDistMark"].GetDouble();
    }
    if(value.HasMember("MidNear_HPosMaxDistBlock")){
        mMidNear_HPosMaxDistBlock = value["MidNear_HPosMaxDistBlock"].GetDouble();
    }
    if(value.HasMember("MidNear_PosMaxDistMark")){
        mMidNear_PosMaxDistMark = value["MidNear_PosMaxDistMark"].GetDouble();
    }
    if(value.HasMember("MidNear_HPosMaxDistBlock")){
        mMidNear_HPosMaxDistBlock = value["MidNear_HPosMaxDistBlock"].GetDouble();
    }
    if(value.HasMember("Goal_ForwardInMark")){
        mGoal_ForwardInMark = value["Goal_ForwardInMark"].GetBool();
    }
    if(value.HasMember("Goal_ForwardInBlock")){
        mGoal_ForwardInBlock = value["Goal_ForwardInBlock"].GetBool();
    }
    if(value.HasMember("Goal_PosMaxDistMark")){
        mGoal_PosMaxDistMark = value["Goal_PosMaxDistMark"].GetDouble();
    }
    if(value.HasMember("Goal_HPosMaxDistMark")){
        mGoal_HPosMaxDistMark = value["Goal_HPosMaxDistMark"].GetDouble();
    }
    if(value.HasMember("Goal_OffsideMaxDistMark")){
        mGoal_OffsideMaxDistMark = value["Goal_OffsideMaxDistMark"].GetDouble();
    }
    if(value.HasMember("Goal_PosMaxDistBlock")){
        mGoal_PosMaxDistBlock = value["Goal_PosMaxDistBlock"].GetDouble();
    }
    if(value.HasMember("Goal_HPosMaxDistBlock")){
        mGoal_HPosMaxDistBlock = value["Goal_HPosMaxDistBlock"].GetDouble();
    }
    if(value.HasMember("Goal_OffsideMaxDistBlock")){
        mGoal_OffsideMaxDistBlock = value["Goal_OffsideMaxDistBlock"].GetDouble();
    }
}

void Setting::SetTeamName(string teamName){
    if(teamName.length() == 0 || mSeted)
        return;
    mSeted = true;
    FILE* fp = fopen("./data/settings/teams.conf", "r");
    char readBuffer[65536];
    FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    Document d;
    d.ParseStream(is);
    fclose(fp);
    for(auto & t: d.GetObject()){
        mTeamsConfig[t.name.GetString()] = t.value.GetString();
    }
    if(mTeamsConfig.find(teamName) != mTeamsConfig.end())
        mJsonPath = mTeamsConfig[teamName];
    ReadJson();
}

void Setting::ReadJson(){
    if(mJsonPath.length() == 0)
        return;
    std::cout<<"Read Setting: "<<mJsonPath.c_str()<<std::endl;
    FILE* fp = fopen(mJsonPath.c_str(), "r");
    char readBuffer[65536];
    FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    Document d;
    d.ParseStream(is);
    fclose(fp);
    if(d.HasMember("Strategy")){
        delete mStrategySetting;
        mStrategySetting = new StrategySetting(d["Strategy"]);
    }
    if(d.HasMember("ChainAction")){
        delete mChainAction;
        mChainAction = new ChainActionSetting(d["ChainAction"]);
    }
    if(d.HasMember("OffensiveMove")){
        delete mOffensiveMove;
        mOffensiveMove = new OffensiveMoveSetting(d["OffensiveMove"]);
    }
    if(d.HasMember("DefenseMove")){
        delete mDefenseMove;
        mDefenseMove = new DefenseMoveSetting(d["DefenseMove"]);
    }
}
