// -*-c++-*-

/*!
  \file strategy.cpp
  \brief team strategh Source File
 */

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "strategy.h"
#include "roles/soccer_role.h"
#include "roles/role_sample.h"
#include "roles/role_goalie.h"
#include "roles/role_player.h"
#include "chain_action/field_analyzer.h"
#include "setting.h"

#include <rcsc/player/world_model.h>
#include <rcsc/formation/formation_parser.h>
#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/param/cmd_line_parser.h>
#include <rcsc/param/param_map.h>
#include <rcsc/game_mode.h>
#include <rcsc/formation/formation_static.h>

#include <set>
#include <iostream>
#include <cstdio>


using namespace rcsc;
const std::string Strategy::F433_BEFORE_KICK_OFF_CONF = "F433_before-kick-off.conf";
const std::string Strategy::F433_BEFORE_KICK_OFF_CONF_FOR_OUR_KICK = "F433_before-kick-off_for_our_kick.conf";
const std::string Strategy::F433_DEFENSE_FORMATION_CONF = "F433_defense-formation.conf";
const std::string Strategy::F433_OFFENSE_FORMATION_CONF = "F433_offense-formation.conf";
const std::string Strategy::F433_OFFENSE_FORMATION_CONF_FOR_OXSY = "F433_offense-formation_for_oxsy.conf";
const std::string Strategy::F433_OFFENSE_FORMATION_CONF_FOR_MT = "F433_offense-formation_for_mt.conf";
const std::string Strategy::F433_GOAL_KICK_OPP_FORMATION_CONF = "F433_goal-kick-opp.conf";
const std::string Strategy::F433_GOAL_KICK_OUR_FORMATION_CONF = "F433_goal-kick-our.conf";
const std::string Strategy::F433_KICKIN_OUR_FORMATION_CONF = "F433_kickin-our-formation.conf";
const std::string Strategy::F433_SETPLAY_OPP_FORMATION_CONF = "F433_setplay-opp-formation.conf";
const std::string Strategy::F433_SETPLAY_OUR_FORMATION_CONF = "F433_setplay-our-formation.conf";

const std::string Strategy::F523_BEFORE_KICK_OFF_CONF = "F523_before-kick-off.conf";
const std::string Strategy::F523_BEFORE_KICK_OFF_CONF_FOR_OUR_KICK = "F523_before-kick-off_for_our_kick.conf";
const std::string Strategy::F523_DEFENSE_FORMATION_CONF = "F523_defense-formation.conf";
const std::string Strategy::F523_DEFENSE_FORMATION_NO5_CONF = "F523_defense-formation_no5.conf";
const std::string Strategy::F523_DEFENSE_FORMATION_NO6_CONF = "F523_defense-formation_no6.conf";
const std::string Strategy::F523_DEFENSE_FORMATION_NO56_CONF = "F523_defense-formation_no56.conf";
const std::string Strategy::F523_OFFENSE_FORMATION_CONF = "F523_offense-formation.conf";
const std::string Strategy::F523_GOAL_KICK_OPP_FORMATION_CONF = "F523_goal-kick-opp.conf";
const std::string Strategy::F523_GOAL_KICK_OUR_FORMATION_CONF = "F523_goal-kick-our.conf";
const std::string Strategy::F523_KICKIN_OUR_FORMATION_CONF = "F523_kickin-our-formation.conf";
const std::string Strategy::F523_SETPLAY_OPP_FORMATION_CONF = "F523_setplay-opp-formation.conf";
const std::string Strategy::F523_SETPLAY_OUR_FORMATION_CONF = "F523_setplay-our-formation.conf";

const std::string Strategy::Fhel_BEFORE_KICK_OFF_CONF = "Fhel_before-kick-off.conf";
const std::string Strategy::Fhel_DEFENSE_FORMATION_CONF = "Fhel_defense-formation.conf";
const std::string Strategy::Fhel_OFFENSE_FORMATION_CONF = "Fhel_offense-formation.conf";
const std::string Strategy::Fhel_GOAL_KICK_OPP_FORMATION_CONF = "Fhel_goal-kick-opp.conf";
const std::string Strategy::Fhel_GOAL_KICK_OUR_FORMATION_CONF = "Fhel_goal-kick-our.conf";
const std::string Strategy::Fhel_KICKIN_OUR_FORMATION_CONF = "Fhel_kickin-our-formation.conf";
const std::string Strategy::Fhel_SETPLAY_OPP_FORMATION_CONF = "Fhel_setplay-opp-formation.conf";
const std::string Strategy::Fhel_SETPLAY_OUR_FORMATION_CONF = "Fhel_setplay-our-formation.conf";
/*-------------------------------------------------------------------*/
/*!

 */
Strategy::Strategy()
    : M_tm_line(12),
      M_tm_post(12),
      M_goalie_unum( Unum_Unknown ),
      M_current_situation( Normal_Situation ),
      M_num_to_role(12, 0 ),
      M_role_to_num(12, 0),
      M_position_types( 12, Position_Center ),
      M_positions( 12 )
{
#ifndef USE_GENERIC_FACTORY
    //
    // roles
    //

    M_role_factory[RoleSample::name()] = &RoleSample::create;

    M_role_factory[RoleGoalie::name()] = &RoleGoalie::create;
    M_role_factory[RolePlayer::name()] = &RolePlayer::create;

#endif

    for (size_t i = 0; i < M_num_to_role.size(); ++i )
    {
        M_num_to_role[i] = i;
        M_role_to_num[i] = i;
    }


    for(int i=0;i<=11;i++){
        M_tm_line[i] = PostLine::golie;
        M_tm_post[i] = pp_gk;
    }

    M_formation_type = FormationType::F433;
}

/*-------------------------------------------------------------------*/
/*!

 */
Strategy &
Strategy::instance()
{
    static Strategy s_instance;
    return s_instance;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Strategy::init( CmdLineParser & cmd_parser )
{
    ParamMap param_map( "HELIOS_base options" );

    // std::string fconf;
    //param_map.add()
    //    ( "fconf", "", &fconf, "another formation file." );

    //
    //
    //

    if ( cmd_parser.count( "help" ) > 0 )
    {
        param_map.printHelp( std::cout );
        return false;
    }

    //
    //
    //

    cmd_parser.parse( param_map );

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Strategy::read( const std::string & formation_dir )
{
    static bool s_initialized = false;

    if ( s_initialized )
    {
        std::cerr << __FILE__ << ' ' << __LINE__ << ": already initialized."
                  << std::endl;
        return false;
    }

    std::string configpath = formation_dir;
    if ( ! configpath.empty()
         && configpath[ configpath.length() - 1 ] != '/' )
    {
        configpath += '/';
    }

    //433
    M_F433_before_kick_off_formation = readFormation( configpath + F433_BEFORE_KICK_OFF_CONF );
    if ( ! M_F433_before_kick_off_formation )
    {
        std::cerr << "Failed to read before_kick_off formation" << std::endl;
        return false;
    }
    M_F433_before_kick_off_formation_for_our_kick = readFormation( configpath + F433_BEFORE_KICK_OFF_CONF_FOR_OUR_KICK );
    if ( ! M_F433_before_kick_off_formation_for_our_kick )
    {
        std::cerr << "Failed to read before_kick_off formation" << std::endl;
        return false;
    }
    M_F433_defense_formation = readFormation( configpath + F433_DEFENSE_FORMATION_CONF );
    if ( ! M_F433_defense_formation )
    {
        std::cerr << "Failed to read defense formation" << std::endl;
        return false;
    }
    M_F433_offense_formation = readFormation( configpath + F433_OFFENSE_FORMATION_CONF );
    if ( ! M_F433_offense_formation )
    {
        std::cerr << "Failed to read offense formation" << std::endl;
        return false;
    }
    M_F433_offense_formation_for_oxsy = readFormation( configpath + F433_OFFENSE_FORMATION_CONF_FOR_OXSY );
    if ( ! M_F433_offense_formation_for_oxsy )
    {
        std::cerr << "Failed to read offense formation" << std::endl;
        return false;
    }
    M_F433_offense_formation_for_mt = readFormation( configpath + F433_OFFENSE_FORMATION_CONF_FOR_MT );
    if ( ! M_F433_offense_formation_for_mt )
    {
        std::cerr << "Failed to read offense formation" << std::endl;
        return false;
    }
    M_F433_goal_kick_opp_formation = readFormation( configpath + F433_GOAL_KICK_OPP_FORMATION_CONF );
    if ( ! M_F433_goal_kick_opp_formation )
    {
        return false;
    }
    M_F433_goal_kick_our_formation = readFormation( configpath + F433_GOAL_KICK_OUR_FORMATION_CONF );
    if ( ! M_F433_goal_kick_our_formation )
    {
        return false;
    }
    M_F433_kickin_our_formation = readFormation( configpath + F433_KICKIN_OUR_FORMATION_CONF );
    if ( ! M_F433_kickin_our_formation )
    {
        std::cerr << "Failed to read kickin our formation" << std::endl;
        return false;
    }
    M_F433_setplay_opp_formation = readFormation( configpath + F433_SETPLAY_OPP_FORMATION_CONF );
    if ( ! M_F433_setplay_opp_formation )
    {
        std::cerr << "Failed to read setplay opp formation" << std::endl;
        return false;
    }
    M_F433_setplay_our_formation = readFormation( configpath + F433_SETPLAY_OUR_FORMATION_CONF );
    if ( ! M_F433_setplay_our_formation )
    {
        std::cerr << "Failed to read setplay our formation" << std::endl;
        return false;
    }

    //523
    M_F523_before_kick_off_formation = readFormation( configpath + F523_BEFORE_KICK_OFF_CONF );
    if ( ! M_F523_before_kick_off_formation )
    {
        std::cerr << "Failed to read before_kick_off formation" << std::endl;
        return false;
    }
    M_F523_before_kick_off_formation_for_our_kick = readFormation( configpath + F523_BEFORE_KICK_OFF_CONF_FOR_OUR_KICK );
    if ( ! M_F523_before_kick_off_formation_for_our_kick )
    {
        std::cerr << "Failed to read before_kick_off formation" << std::endl;
        return false;
    }
    M_F523_defense_formation = readFormation( configpath + F523_DEFENSE_FORMATION_CONF );
    if ( ! M_F523_defense_formation )
    {
        std::cerr << "Failed to read defense formation" << std::endl;
        return false;
    }
    M_F523_defense_formation_no5 = readFormation( configpath + F523_DEFENSE_FORMATION_NO5_CONF );
    if ( ! M_F523_defense_formation_no5 )
    {
        std::cerr << "Failed to read defense formation" << std::endl;
        return false;
    }
    M_F523_defense_formation_no6 = readFormation( configpath + F523_DEFENSE_FORMATION_NO6_CONF );
    if ( ! M_F523_defense_formation_no6 )
    {
        std::cerr << "Failed to read defense formation" << std::endl;
        return false;
    }
    M_F523_defense_formation_no56 = readFormation( configpath + F523_DEFENSE_FORMATION_NO56_CONF );
    if ( ! M_F523_defense_formation_no56 )
    {
        std::cerr << "Failed to read defense formation" << std::endl;
        return false;
    }
    M_F523_offense_formation = readFormation( configpath + F523_OFFENSE_FORMATION_CONF );
    if ( ! M_F523_offense_formation )
    {
        std::cerr << "Failed to read offense formation" << std::endl;
        return false;
    }
    M_F523_goal_kick_opp_formation = readFormation( configpath + F523_GOAL_KICK_OPP_FORMATION_CONF );
    if ( ! M_F523_goal_kick_opp_formation )
    {
        return false;
    }
    M_F523_goal_kick_our_formation = readFormation( configpath + F523_GOAL_KICK_OUR_FORMATION_CONF );
    if ( ! M_F523_goal_kick_our_formation )
    {
        return false;
    }
    M_F523_kickin_our_formation = readFormation( configpath + F523_KICKIN_OUR_FORMATION_CONF );
    if ( ! M_F523_kickin_our_formation )
    {
        std::cerr << "Failed to read kickin our formation" << std::endl;
        return false;
    }
    M_F523_setplay_opp_formation = readFormation( configpath + F523_SETPLAY_OPP_FORMATION_CONF );
    if ( ! M_F523_setplay_opp_formation )
    {
        std::cerr << "Failed to read setplay opp formation" << std::endl;
        return false;
    }
    M_F523_setplay_our_formation = readFormation( configpath + F523_SETPLAY_OUR_FORMATION_CONF );
    if ( ! M_F523_setplay_our_formation )
    {
        std::cerr << "Failed to read setplay our formation" << std::endl;
        return false;
    }
    //sh
    M_Fhel_before_kick_off_formation = readFormation( configpath + Fhel_BEFORE_KICK_OFF_CONF );
    if ( ! M_Fhel_before_kick_off_formation )
    {
        std::cerr << "Failed to read before_kick_off formation" << std::endl;
        return false;
    }
    M_Fhel_defense_formation = readFormation( configpath + Fhel_DEFENSE_FORMATION_CONF );
    if ( ! M_Fhel_defense_formation )
    {
        std::cerr << "Failed to read defense formation" << std::endl;
        return false;
    }
    M_Fhel_offense_formation = readFormation( configpath + Fhel_OFFENSE_FORMATION_CONF );
    if ( ! M_Fhel_offense_formation )
    {
        std::cerr << "Failed to read offense formation" << std::endl;
        return false;
    }
    M_Fhel_goal_kick_opp_formation = readFormation( configpath + Fhel_GOAL_KICK_OPP_FORMATION_CONF );
    if ( ! M_Fhel_goal_kick_opp_formation )
    {
        return false;
    }
    M_Fhel_goal_kick_our_formation = readFormation( configpath + Fhel_GOAL_KICK_OUR_FORMATION_CONF );
    if ( ! M_Fhel_goal_kick_our_formation )
    {
        return false;
    }
    M_Fhel_kickin_our_formation = readFormation( configpath + Fhel_KICKIN_OUR_FORMATION_CONF );
    if ( ! M_Fhel_kickin_our_formation )
    {
        std::cerr << "Failed to read kickin our formation" << std::endl;
        return false;
    }
    M_Fhel_setplay_opp_formation = readFormation( configpath + Fhel_SETPLAY_OPP_FORMATION_CONF );
    if ( ! M_Fhel_setplay_opp_formation )
    {
        std::cerr << "Failed to read setplay opp formation" << std::endl;
        return false;
    }
    M_Fhel_setplay_our_formation = readFormation( configpath + Fhel_SETPLAY_OUR_FORMATION_CONF );
    if ( ! M_Fhel_setplay_our_formation )
    {
        std::cerr << "Failed to read setplay our formation" << std::endl;
        return false;
    }

    s_initialized = true;
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */

Formation::Ptr
Strategy::readFormation( const std::string & filepath )
{
    Formation::Ptr f = FormationParser::parse( filepath );

    if ( ! f )
    {
        std::cerr << "(Strategy::createFormation) Could not create a formation from " << filepath << std::endl;
        return Formation::Ptr();
    }

    //
    // check role names
    //
    for ( int unum = 1; unum <= 11; ++unum )
    {
        const std::string role_name = f->roleName( unum );
        if ( role_name == "Savior"
             || role_name == "Goalie" )
        {
            if ( M_goalie_unum == Unum_Unknown )
            {
                M_goalie_unum = unum;
            }

            if ( M_goalie_unum != unum )
            {
                std::cerr << __FILE__ << ':' << __LINE__ << ':'
                          << " ***ERROR*** Illegal goalie's uniform number"
                          << " read unum=" << unum
                          << " expected=" << M_goalie_unum
                          << std::endl;
                f.reset();
                return f;
            }
        }


#ifdef USE_GENERIC_FACTORY
        SoccerRole::Ptr role = SoccerRole::create( role_name );
        if ( ! role )
        {
            std::cerr << __FILE__ << ':' << __LINE__ << ':'
                      << " ***ERROR*** Unsupported role name ["
                      << role_name << "] is appered in ["
                      << filepath << "]" << std::endl;
            f.reset();
            return f;
        }
#else
        if ( M_role_factory.find( role_name ) == M_role_factory.end() )
        {
            std::cerr << __FILE__ << ':' << __LINE__ << ':'
                      << " ***ERROR*** Unsupported role name ["
                      << role_name << "] is appered in ["
                      << filepath << "]" << std::endl;
            f.reset();
            return f;
        }
#endif
    }

    return f;
}

void
Strategy::update( const WorldModel & wm )
{
    static GameTime s_update_time( -1, 0 );

    if ( s_update_time == wm.time() )
    {
        return;
    }
    s_update_time = wm.time();

    updateSituation( wm );
    updatePosition( wm );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Strategy::exchangeRole( const int unum0,
                        const int unum1 )
{
    if ( unum0 < 1 || 11 < unum0
         || unum1 < 1 || 11 < unum1 )
    {
        std::cerr << __FILE__ << ':' << __LINE__ << ':'
                  << "(exchangeRole) Illegal uniform number. "
                  << unum0 << ' ' << unum1
                  << std::endl;
        dlog.addText( Logger::TEAM,
                      __FILE__":(exchangeRole) Illegal unum. %d %d",
                      unum0, unum1 );
        return;
    }

    if ( unum0 == unum1 )
    {
        std::cerr << __FILE__ << ':' << __LINE__ << ':'
                  << "(exchangeRole) same uniform number. "
                  << unum0 << ' ' << unum1
                  << std::endl;
        dlog.addText( Logger::TEAM,
                      __FILE__":(exchangeRole) same unum. %d %d",
                      unum0, unum1 );
        return;
    }

    int role0 = M_num_to_role[unum0];
    int role1 = M_num_to_role[unum1];

    dlog.addText( Logger::TEAM,
                  __FILE__":(exchangeRole) unum=%d(role=%d) <-> unum=%d(role=%d)",
                  unum0, role0,
                  unum1, role1 );


    M_num_to_role[unum0] = role1;
    M_num_to_role[unum1] = role0;

    M_role_to_num[role0] = unum1;
    M_role_to_num[role1] = unum0;
}

/*-------------------------------------------------------------------*/
/*!

 */
SoccerRole::Ptr
Strategy::createRole( const int unum,
                      const WorldModel & world )
{
    const int number = unumToRole(unum);

    SoccerRole::Ptr role;

    if ( number < 1 || 11 < number )
    {
        std::cerr << __FILE__ << ": " << __LINE__
                  << " ***ERROR*** Invalid player number " << number
                  << std::endl;
        return role;
    }
    updateFormation(world);
    Formation::Ptr f = getFormation( world );
    if ( ! f )
    {
        std::cerr << __FILE__ << ": " << __LINE__
                  << " ***ERROR*** faled to create role. Null formation" << std::endl;
        return role;
    }
    const std::string role_name = f->roleName( number );

#ifdef USE_GENERIC_FACTORY
    role = SoccerRole::create( role_name );
#else
    RoleFactory::const_iterator factory = M_role_factory.find( role_name );
    if ( factory != M_role_factory.end() )
    {
        role = factory->second();
    }
#endif
    if ( ! role )
    {
        std::cerr << __FILE__ << ": " << __LINE__
                  << " ***ERROR*** unsupported role name ["
                  << role_name << "]"
                  << std::endl;
    }
    return role;
}

/*-------------------------------------------------------------------*/
/*!

 */

void
Strategy::updateSituation( const WorldModel & wm )
{
    M_current_situation = Offense_Situation;

    if ( wm.gameMode().type() != GameMode::PlayOn )
    {
        if ( wm.gameMode().isPenaltyKickMode() )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": Situation PenaltyKick" );
            M_current_situation = PenaltyKick_Situation;
        }
        else if ( wm.gameMode().isPenaltyKickMode() )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": Situation OurSetPlay" );
            M_current_situation = OurSetPlay_Situation;
        }
        else
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": Situation OppSetPlay" );
            M_current_situation = OppSetPlay_Situation;
        }
        return;
    }

    M_current_situation = isDefenseSituation(wm, wm.self().unum()) ? Defense_Situation : Offense_Situation;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Strategy::updatePosition( const WorldModel & wm )
{
    static GameTime s_update_time( 0, 0 );
    if ( s_update_time == wm.time() )
    {
        return;
    }
    s_update_time = wm.time();

    updateFormation(wm);
    Formation::Ptr f = getFormation( wm );
    if ( ! f )
    {
        std::cerr << wm.teamName() << ':' << wm.self().unum() << ": "
                  << wm.time()
                  << " ***ERROR*** could not get the current formation" << std::endl;
        return;
    }

    int ball_step = 0;
    if ( wm.gameMode().type() == GameMode::PlayOn
         || wm.gameMode().type() == GameMode::GoalKick_ )
    {
        ball_step = std::min( 1000, wm.interceptTable().teammateStep() );
        ball_step = std::min( ball_step, wm.interceptTable().opponentStep() );
        ball_step = std::min( ball_step, wm.interceptTable().selfStep() );
    }

    Vector2D ball_pos = wm.ball().inertiaPoint( ball_step );

    if (wm.gameMode().type() != GameMode::PlayOn){
        ball_pos = wm.ball().pos();
        if (ball_pos.y >= 34.0)
            ball_pos.y = 33.99;
        if (ball_pos.y <= -34.0)
            ball_pos.y = -33.99;
        if (ball_pos.x >= 52.5)
            ball_pos.x = 52.49;
        if (ball_pos.x <= -52.5)
            ball_pos.x = -52.49;
    }

    dlog.addText( Logger::TEAM,
                  __FILE__": HOME POSITION: ball pos=(%.1f %.1f) step=%d",
                  ball_pos.x, ball_pos.y,
                  ball_step );

    vector<Vector2D> positions(11);
    f->getPositions( ball_pos, positions );

    for ( int unum = 1; unum <= 11; ++unum )
    {
        M_positions[unum] = positions[unum - 1];
    }

    bool use_ofside = true;
    if(M_formation_type == FormationType::HeliosFra){
        if(ball_pos.y > 0 && wm.self().pos().y < 0){
            use_ofside = false;
        }else if(ball_pos.y < 0 && wm.self().pos().y > 0){
            use_ofside = false;
        }
    }
    if ( ServerParam::i().useOffside() && use_ofside)
    {
        double max_x = wm.offsideLineX();
        if ( ServerParam::i().kickoffOffside()
             && ( wm.gameMode().type() == GameMode::BeforeKickOff
                  || wm.gameMode().type() == GameMode::AfterGoal_ ) )
        {
            max_x = 0.0;
        }
        else
        {
            int mate_step = wm.interceptTable().teammateStep();
            if ( mate_step < 50 )
            {
                Vector2D trap_pos = wm.ball().inertiaPoint( mate_step );
                if ( trap_pos.x > max_x ) max_x = trap_pos.x;
            }

            max_x -= 1.0;
        }

        for ( int unum = 1; unum <= 11; ++unum )
        {
            if ( M_positions[unum].x > max_x )
            {
                dlog.addText( Logger::TEAM,
                              "____ %d offside. home_pos_x %.2f -> %.2f",
                              unum,
                              M_positions[unum].x, max_x );
                M_positions[unum].x = max_x;
            }
        }
    }

    M_position_types.clear();

    for ( int unum = 1; unum <= 11; ++unum )
    {
        PositionType type = Position_Center;

        const RoleType role_type = f->roleType( unum );
        if ( role_type.side() == RoleType::Left )
        {
            type = Position_Left;
        }
        else if ( role_type.side() == RoleType::Right )
        {
            type = Position_Right;
        }

        M_position_types.push_back( type );

        dlog.addText( Logger::TEAM,
                      "__ %d home pos (%.2f %.2f) type=%d",
                      unum,
                      M_positions[unum].x, M_positions[unum].y,
                      type );
        dlog.addCircle( Logger::TEAM,
                        M_positions[unum], 0.5,
                        "#000000" );
    }
}


/*-------------------------------------------------------------------*/
/*!

 */
rcsc::Vector2D
Strategy::getPositionWithBall( const int unum, rcsc::Vector2D ball, const WorldModel & wm ){
    try {
        auto role = unumToRole(unum);
        updateFormation(wm);
        Formation::Ptr f = getFormation( wm );
        if ( ! f )
        {
            std::cerr << wm.teamName() << ':' << wm.self().unum() << ": "
                      << wm.time()
                      << " ***ERROR*** could not get the current formation" << std::endl;
            return Vector2D::INVALIDATED;
        }
        std::vector< rcsc::Vector2D > positions(11);
        positions.clear();
        f->getPositions( ball, positions );
        return positions.at(role - 1);
    } catch (std::exception & e) {
        std::cout<<"ERRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRrrrrorrr"<<std::endl;
        return Vector2D::INVALIDATED;
    }
}

Vector2D
Strategy::getPosition( const int unum ) const
{
    int number = unumToRole(unum);

    if ( number < 1 || 11 < number )
    {
        std::cerr << __FILE__ << ' ' << __LINE__
                  << ": Illegal number : " << number
                  << std::endl;
        return Vector2D::INVALIDATED;
    }

    try
    {
        return M_positions.at( number );
    }
    catch ( std::exception & e )
    {
        std::cerr<< __FILE__ << ':' << __LINE__ << ':'
                 << " Exception caught! " << e.what()
                 << std::endl;
        return Vector2D::INVALIDATED;
    }
}

/*-------------------------------------------------------------------*/
/*!

 */

Vector2D Strategy::getPreSetPlayPosition(const rcsc::WorldModel &wm , double cycle_dist) const 
{

    int unum = wm.self().unum();
    auto home_pos = getPosition(unum);
    auto ball_pos = wm.ball().pos();

    Vector2D closest_opponent = Vector2D::INVALIDATED;
    double min_dist = 1000;
    for (const auto &op : wm.theirPlayers()) {
        if (op == nullptr)
            continue;
        if (op->unum() <= 0)
            continue;
        if (!op->pos().isValid())
            continue;
        double dist = (op->pos() - home_pos).r();
        if (dist < min_dist) {
            min_dist = dist;
            closest_opponent = op->pos();
        }
    }

    if (!closest_opponent.isValid())
    {
        dlog.addText(Logger::TEAM, "No opponent found");
        return home_pos;
    }

    dlog.addText(Logger::TEAM, "Closest opponent: %.1f, %.1f mindist: $.1f", closest_opponent.x, closest_opponent.y, min_dist);

    if (min_dist > 10)
        return home_pos;

    Vector2D new_home_pos = home_pos;
    auto game_mode = wm.gameMode().type();
    if (game_mode == rcsc::GameMode::CornerKick_ || game_mode == rcsc::GameMode::KickIn_)
    {
        new_home_pos = home_pos + Vector2D(-cycle_dist, 0);
    }
    else
    {
        if (home_pos.dist(ball_pos) < 15)
        {
            auto ball_to_home_dir = (home_pos - ball_pos).th();
            new_home_pos = home_pos + Vector2D::polar2vector(cycle_dist, ball_to_home_dir);

            dlog.addText(Logger::TEAM, "Ball Close to home: %.1f, %.1f", new_home_pos.x, new_home_pos.y);
        }
        else
        {
            auto angle = 135.0;
            if (ball_pos.y > 0)
            {
                if (home_pos.y < ball_pos.y)
                {
                    angle = -135.0;
                }
            }
            else
            {
                if (home_pos.y < ball_pos.y){
                    angle = -135.0;
                }
            }
            new_home_pos = home_pos + Vector2D::polar2vector(cycle_dist, angle);
            dlog.addText(Logger::TEAM, "Ball Far from home: %.1f, %.1f, angle: %.1f", new_home_pos.x, new_home_pos.y, angle);
        }
    }
    
    if (new_home_pos.x < -52.0)
        new_home_pos.x = -52.0;
    if (new_home_pos.x > 52.0)
        new_home_pos.x = 52.0;
    if (new_home_pos.y < -33.0)
        new_home_pos.y = -33.0;
    if (new_home_pos.y > 33.0)
        new_home_pos.y = 33.0;
    if(new_home_pos.x > wm.offsideLineX())
        new_home_pos.x = wm.offsideLineX() - 0.5;
    return new_home_pos;
}


std::vector<const AbstractPlayerObject *>
Strategy::getTeammatesInPostLine(const rcsc::WorldModel &wm, PostLine tm_line) {
    std::vector<const AbstractPlayerObject *> results;
    for ( const AbstractPlayerObject * p : wm.ourPlayers() ){
        if (p == nullptr)
            continue;
        if (!p->pos().isValid())
            continue;

        if (Strategy::i().tmLine(p->unum()) == tm_line)
            results.push_back(p);
    } 
    return results;
}

/*-------------------------------------------------------------------*/
/*!

 */
Formation::Ptr
Strategy::getFormation( const WorldModel & wm ){
    return M_current_formation;
}

void Strategy::setFormation(const rcsc::Formation::Ptr &formation) {
    M_current_formation = formation;
}

void Strategy::updateFormation( const WorldModel & wm )
{
    my_team_tactic = stringToTeamTactic(Setting::i()->mStrategySetting->mTeamTactic);
    M_formation_type = stringToFormationType(Setting::i()->mStrategySetting->mFormation);

    if(M_formation_type == FormationType::HeliosFra)
        updateFormationFra(wm);
    else if(M_formation_type == FormationType::F433)
        updateFormation433(wm);
    else if(M_formation_type == FormationType::F523)
        updateFormation523(wm);
}

void Strategy::updateFormationFra( const WorldModel & wm ){
    M_tm_line[1] = PostLine::golie;

    M_tm_line[2] = PostLine::back;
    M_tm_line[3] = PostLine::back;
    M_tm_line[4] = PostLine::back;
    M_tm_line[5] = PostLine::back;
    M_tm_line[6] = PostLine::half;

    M_tm_line[7] = PostLine::half;
    M_tm_line[8] = PostLine::forward;

    M_tm_line[9] = PostLine::forward;
    M_tm_line[10] = PostLine::forward;
    M_tm_line[11] = PostLine::forward;

    M_tm_post[1] = pp_gk;
    M_tm_post[2] = pp_cb;
    M_tm_post[3] = pp_cb;
    M_tm_post[4] = pp_lb;

    M_tm_post[5] = pp_rb;
    M_tm_post[6] = pp_ch;
    M_tm_post[7] = pp_lh;
    M_tm_post[8] = pp_rh;

    M_tm_post[9] = pp_lf;
    M_tm_post[10] = pp_cf;
    M_tm_post[11] = pp_cf;

    if ( wm.gameMode().type() == GameMode::PlayOn )
    {
        //
        // play on
        //
        if (M_current_situation == Defense_Situation)
            M_current_formation = M_Fhel_defense_formation;
        else if (M_current_situation == Offense_Situation)
            M_current_formation = M_Fhel_offense_formation;
        else
            M_current_formation = M_Fhel_offense_formation;
    }
    else if ( wm.gameMode().type() == GameMode::KickIn_
              || wm.gameMode().type() == GameMode::CornerKick_ )
    {
        //
        // kick in, corner kick
        //
        if ( wm.ourSide() == wm.gameMode().side() )
        {
            // our kick-in or corner-kick
            M_current_formation = M_Fhel_kickin_our_formation;
        }
        else
        {
            M_current_formation = M_Fhel_setplay_opp_formation;
        }
    }
    else if ( ( wm.gameMode().type() == GameMode::BackPass_
                && wm.gameMode().side() == wm.theirSide() )
              || ( wm.gameMode().type() == GameMode::IndFreeKick_
                   && wm.gameMode().side() == wm.ourSide() ) )
    {
        //
        // our indirect free kick
        //
        M_current_formation = M_Fhel_setplay_our_formation;
    }
    else if ( ( wm.gameMode().type() == GameMode::BackPass_
                && wm.gameMode().side() == wm.ourSide() )
              || ( wm.gameMode().type() == GameMode::IndFreeKick_
                   && wm.gameMode().side() == wm.theirSide() ) )
    {
        //
        // opponent indirect free kick
        //
        M_current_formation = M_Fhel_setplay_opp_formation;
    }
    else if ( wm.gameMode().type() == GameMode::FoulCharge_
              || wm.gameMode().type() == GameMode::FoulPush_ )
    {
        //
        // after foul
        //
        if ( wm.gameMode().side() == wm.ourSide() )
        {
            //
            // opponent (indirect) free kick
            //
            M_current_formation = M_Fhel_setplay_opp_formation;
        }
        else
        {
            //
            // our (indirect) free kick
            //
            M_current_formation =  M_Fhel_setplay_our_formation;
        }
    }
    else if ( wm.gameMode().type() == GameMode::GoalKick_ || wm.gameMode().type() == GameMode::GoalieCatch_)
    {
        //
        // goal kick
        //
        if ( wm.gameMode().side() == wm.ourSide() )
        {
            M_current_formation = M_Fhel_goal_kick_our_formation;
        }
        else
        {
            M_current_formation = M_Fhel_goal_kick_opp_formation;
        }
    }
    else if ( wm.gameMode().type() == GameMode::BeforeKickOff
              || wm.gameMode().type() == GameMode::AfterGoal_ )
    {
        //
        // before kick off
        //
        M_current_formation =  M_Fhel_before_kick_off_formation;
    }
    else if ( wm.gameMode().isOurSetPlay( wm.ourSide() ) )
    {
        //
        // other set play
        //
        M_current_formation =  M_Fhel_setplay_our_formation;
    }
    else if ( wm.gameMode().type() != GameMode::PlayOn )
    {
        M_current_formation = M_Fhel_setplay_opp_formation;
    }
    else
    {
        //
        // unknown
        //
        if (M_current_situation == Defense_Situation)
            M_current_formation = M_Fhel_defense_formation;
        else if (M_current_situation == Offense_Situation)
            M_current_formation = M_Fhel_offense_formation;
        else
            M_current_formation = M_Fhel_offense_formation;
    }
}

void Strategy::updateFormation433( const WorldModel & wm ){
    int opp_min = wm.interceptTable().opponentStep();
    int mate_min = std::min(wm.interceptTable().teammateStep(), wm.interceptTable().selfStep());
    M_tm_line[1] = PostLine::golie;

    M_tm_line[2] = PostLine::back;
    M_tm_line[3] = PostLine::back;
    M_tm_line[4] = PostLine::back;

    if(wm.ball().pos().x < 15 || opp_min < mate_min - 2)
        M_tm_line[5] = PostLine::back;
    else
        M_tm_line[5] = PostLine::half;
    M_tm_line[6] = PostLine::half;
    M_tm_line[7] = PostLine::half;
    M_tm_line[8] = PostLine::half;

    M_tm_line[9] = PostLine::forward;
    M_tm_line[10] = PostLine::forward;
    M_tm_line[11] = PostLine::forward;

    M_tm_post[1] = pp_gk;
    M_tm_post[2] = pp_cb;
    M_tm_post[3] = pp_lb;
    M_tm_post[4] = pp_rb;

    M_tm_post[5] = pp_ch;
    M_tm_post[6] = pp_ch;
    M_tm_post[7] = pp_lh;
    M_tm_post[8] = pp_rh;

    M_tm_post[9] = pp_lf;
    M_tm_post[10] = pp_rf;
    M_tm_post[11] = pp_cf;

    if ( wm.gameMode().type() == GameMode::PlayOn )
    {
        //
        // play on
        //
        if (M_current_situation == Defense_Situation)
            M_current_formation = M_F433_defense_formation;
        else if (M_current_situation == Offense_Situation)
            if(FieldAnalyzer::isMT(wm)){
                M_current_formation = M_F433_offense_formation_for_mt;
            }else if(doesOpponentDefenseDense(wm)){
                M_current_formation = M_F433_offense_formation_for_oxsy;
            }else{
                M_current_formation = M_F433_offense_formation;
            }
        else
            M_current_formation = M_F433_offense_formation;
    }
    else if ( wm.gameMode().type() == GameMode::KickIn_
              || wm.gameMode().type() == GameMode::CornerKick_ )
    {
        //
        // kick in, corner kick
        //
        if ( wm.ourSide() == wm.gameMode().side() )
        {
            // our kick-in or corner-kick
            M_current_formation = M_F433_kickin_our_formation;
        }
        else
        {
            M_current_formation = M_F433_setplay_opp_formation;
        }
    }
    else if ( ( wm.gameMode().type() == GameMode::BackPass_
                && wm.gameMode().side() == wm.theirSide() )
              || ( wm.gameMode().type() == GameMode::IndFreeKick_
                   && wm.gameMode().side() == wm.ourSide() ) )
    {
        //
        // our indirect free kick
        //
        M_current_formation = M_F433_setplay_our_formation;
    }
    else if ( ( wm.gameMode().type() == GameMode::BackPass_
                && wm.gameMode().side() == wm.ourSide() )
              || ( wm.gameMode().type() == GameMode::IndFreeKick_
                   && wm.gameMode().side() == wm.theirSide() ) )
    {
        //
        // opponent indirect free kick
        //
        M_current_formation = M_F433_setplay_opp_formation;
    }
    else if ( wm.gameMode().type() == GameMode::FoulCharge_
              || wm.gameMode().type() == GameMode::FoulPush_ )
    {
        //
        // after foul
        //

        if ( wm.gameMode().side() == wm.ourSide() )
        {
            //
            // opponent (indirect) free kick
            //
            M_current_formation = M_F433_setplay_opp_formation;
        }
        else
        {
            //
            // our (indirect) free kick
            //
            M_current_formation = M_F433_setplay_our_formation;
        }
    }
    else if ( wm.gameMode().type() == GameMode::GoalKick_ || wm.gameMode().type() == GameMode::GoalieCatch_)
    {
        //
        // goal kick
        //
        if ( wm.gameMode().side() == wm.ourSide() )
        {
            M_current_formation = M_F433_goal_kick_our_formation;
        }
        else
        {
            M_current_formation = M_F433_goal_kick_opp_formation;
        }
    }
    else if ( wm.gameMode().type() == GameMode::BeforeKickOff
              || wm.gameMode().type() == GameMode::AfterGoal_ )
    {
        //
        // before kick off
        //
        if(wm.gameMode().type() == GameMode::BeforeKickOff)
        {
            if (wm.ourSide() == getBeforeKickOffSide(wm) )
                M_current_formation = M_F433_before_kick_off_formation_for_our_kick;
            else
                M_current_formation = M_F433_before_kick_off_formation;
        }
        else
        {
            // after our goal
            if ( wm.gameMode().side() == wm.ourSide() )
            {
                M_current_formation = M_F433_before_kick_off_formation;
            }
            else
            {
                M_current_formation = M_F433_before_kick_off_formation_for_our_kick;
            }
        }

    }
    else if ( wm.gameMode().isOurSetPlay( wm.ourSide() ) )
    {
        //
        // other set play
        //
        M_current_formation = M_F433_setplay_our_formation;
    }
    else if ( wm.gameMode().type() != GameMode::PlayOn )
    {
        M_current_formation = M_F433_setplay_opp_formation;
    }
    else
    {
        //
        // unknown
        //
        if (M_current_situation == Defense_Situation)
            M_current_formation = M_F433_defense_formation;
        else if (M_current_situation == Offense_Situation)
            M_current_formation = M_F433_offense_formation;
        else
            M_current_formation = M_F433_offense_formation;
    }
}

void Strategy::updateFormation523( const WorldModel & wm ){
    int opp_min = wm.interceptTable().opponentStep();
    int mate_min = std::min(wm.interceptTable().teammateStep(), wm.interceptTable().selfStep());
    M_tm_line[1] = PostLine::golie;

    M_tm_line[2] = PostLine::back;
    M_tm_line[3] = PostLine::back;
    M_tm_line[4] = PostLine::back;

    if(wm.ball().pos().x < 15 || opp_min < mate_min - 2){
        M_tm_line[5] = PostLine::back;
        M_tm_line[6] = PostLine::back;
        M_tm_post[5] = pp_cb;
        M_tm_post[6] = pp_cb;
    }
    else{
        M_tm_line[5] = PostLine::half;
        M_tm_line[6] = PostLine::half;
        M_tm_post[5] = pp_ch;
        M_tm_post[6] = pp_ch;
    }

    M_tm_line[7] = PostLine::half;
    M_tm_line[8] = PostLine::half;

    M_tm_line[9] = PostLine::forward;
    M_tm_line[10] = PostLine::forward;
    M_tm_line[11] = PostLine::forward;

    M_tm_post[1] = pp_gk;
    M_tm_post[2] = pp_cb;
    M_tm_post[3] = pp_lb;
    M_tm_post[4] = pp_rb;

    M_tm_post[7] = pp_lh;
    M_tm_post[8] = pp_rh;

    M_tm_post[9] = pp_lf;
    M_tm_post[10] = pp_rf;
    M_tm_post[11] = pp_cf;

    if ( wm.gameMode().type() == GameMode::PlayOn )
    {
        //
        // play on
        //
        if (M_current_situation == Defense_Situation)
            M_current_formation = M_F523_defense_formation;
        else if (M_current_situation == Offense_Situation)
            M_current_formation = M_F523_offense_formation;
        else
            M_current_formation = M_F523_offense_formation;
    }
    else if ( wm.gameMode().type() == GameMode::KickIn_
              || wm.gameMode().type() == GameMode::CornerKick_ )
    {
        //
        // kick in, corner kick
        //
        if ( wm.ourSide() == wm.gameMode().side() )
        {
            // our kick-in or corner-kick
            M_current_formation = M_F523_kickin_our_formation;
        }
        else
        {
            M_current_formation = M_F523_setplay_opp_formation;
        }
    }
    else if ( ( wm.gameMode().type() == GameMode::BackPass_
                && wm.gameMode().side() == wm.theirSide() )
              || ( wm.gameMode().type() == GameMode::IndFreeKick_
                   && wm.gameMode().side() == wm.ourSide() ) )
    {
        //
        // our indirect free kick
        //
        M_current_formation = M_F523_setplay_our_formation;
    }
    else if ( ( wm.gameMode().type() == GameMode::BackPass_
                && wm.gameMode().side() == wm.ourSide() )
              || ( wm.gameMode().type() == GameMode::IndFreeKick_
                   && wm.gameMode().side() == wm.theirSide() ) )
    {
        //
        // opponent indirect free kick
        //
        M_current_formation = M_F523_setplay_opp_formation;
    }
    else if ( wm.gameMode().type() == GameMode::FoulCharge_
              || wm.gameMode().type() == GameMode::FoulPush_ )
    {
        //
        // after foul
        //

        if ( wm.gameMode().side() == wm.ourSide() )
        {
            //
            // opponent (indirect) free kick
            //
            M_current_formation = M_F523_setplay_opp_formation;
        }
        else
        {
            //
            // our (indirect) free kick
            //
            M_current_formation = M_F523_setplay_our_formation;
        }
    }
    else if ( wm.gameMode().type() == GameMode::GoalKick_ || wm.gameMode().type() == GameMode::GoalieCatch_)
    {
        //
        // goal kick
        //
        if ( wm.gameMode().side() == wm.ourSide() )
        {
            M_current_formation = M_F523_goal_kick_our_formation;
        }
        else
        {
            M_current_formation = M_F523_goal_kick_opp_formation;
        }
    }
    else if ( wm.gameMode().type() == GameMode::BeforeKickOff
              || wm.gameMode().type() == GameMode::AfterGoal_ )
    {
        //
        // before kick off
        //
        if(wm.gameMode().type() == GameMode::BeforeKickOff)
        {
            if (wm.ourSide() == getBeforeKickOffSide(wm) )
                M_current_formation = M_F523_before_kick_off_formation_for_our_kick;
            else
                M_current_formation = M_F523_before_kick_off_formation;
        }
        else
        {
            // after our goal
            if ( wm.gameMode().side() == wm.ourSide() )
            {
                M_current_formation = M_F523_before_kick_off_formation;
            }
            else
            {
                M_current_formation = M_F523_before_kick_off_formation_for_our_kick;
            }
        }

    }
    else if ( wm.gameMode().isOurSetPlay( wm.ourSide() ) )
    {
        //
        // other set play
        //
        M_current_formation = M_F523_setplay_our_formation;
    }
    else if ( wm.gameMode().type() != GameMode::PlayOn )
    {
        M_current_formation = M_F523_setplay_opp_formation;
    }
    else
    {
        //
        // unknown
        //
        if (M_current_situation == Defense_Situation)
            M_current_formation = M_F523_defense_formation;
        else if (M_current_situation == Offense_Situation)
            M_current_formation = M_F523_offense_formation;
        else
            M_current_formation = M_F523_offense_formation;
    }
}

SideID Strategy::getBeforeKickOffSide(const rcsc::WorldModel &wm)
{
    SideID kickoff_side;
    const ServerParam & SP = ServerParam::i();
    if ( SP.halfTime() > 0 )
    {
        int half_time = SP.halfTime() * 10;
        int extra_half_time = SP.extraHalfTime() * 10;
        int normal_time = half_time * SP.nrNormalHalfs();
        int extra_time = extra_half_time * SP.nrExtraHalfs();
        int time_flag = 0;

        if ( wm.time().cycle() <= normal_time )
        {
            time_flag = ( wm.time().cycle() / half_time ) % 2;
        }
        else if ( wm.time().cycle() <= normal_time + extra_time )
        {
            int overtime = wm.time().cycle() - normal_time;
            time_flag = ( overtime / extra_half_time ) % 2;
        }
        dlog.addText(Logger::POSITIONING," time_flag: %d",time_flag);
        kickoff_side = ( time_flag == 0
                         ? LEFT
                         : RIGHT );
    }
    else
    {
        kickoff_side = LEFT;
    }
    return kickoff_side;
}

bool Strategy::doesOpponentDefenseDense(const rcsc::WorldModel &wm)
{
    int our_score = ( wm.ourSide() == LEFT
                      ? wm.gameMode().scoreLeft()
                      : wm.gameMode().scoreRight() );
    int opp_score = ( wm.ourSide() == LEFT
                      ? wm.gameMode().scoreRight()
                      : wm.gameMode().scoreLeft() );
    if(FieldAnalyzer::isAlice(wm))
        return true;
    if(FieldAnalyzer::isNexus(wm)
            && opp_score > our_score){
        return true;
    }
    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
double
Strategy::getNormalDashPower(const rcsc::WorldModel &wm )
{
    static bool s_recover_mode = false;

    if ( wm.self().staminaModel().capacityIsEmpty() )
    {
        return std::min( ServerParam::i().maxDashPower(),
                         wm.self().stamina() + wm.self().playerType().extraStamina() );
    }

    const int self_min = wm.interceptTable().selfStep();
    const int mate_min = wm.interceptTable().teammateStep();
    const int opp_min = wm.interceptTable().opponentStep();

    // check recover
    if ( wm.self().staminaModel().capacityIsEmpty() )
    {
        s_recover_mode = false;
    }
    else if ( wm.self().stamina() < ServerParam::i().staminaMax() * 0.5 )
    {
        s_recover_mode = true;
    }
    else if ( wm.self().stamina() > ServerParam::i().staminaMax() * 0.7 )
    {
        s_recover_mode = false;
    }

    /*--------------------------------------------------------*/
    double dash_power = ServerParam::i().maxDashPower();
    const double my_inc
            = wm.self().playerType().staminaIncMax()
            * wm.self().recovery();

    if ( wm.ourDefenseLineX() > wm.self().pos().x
         && wm.ball().pos().x < wm.ourDefenseLineX() + 20.0 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (getNormalDashPower) correct DF line. keep max power" );
        // keep max power
        dash_power = ServerParam::i().maxDashPower();
    }
    else if ( s_recover_mode )
    {
        dash_power = my_inc - 25.0; // preffered recover value
        if ( dash_power < 0.0 ) dash_power = 0.0;

        dlog.addText( Logger::TEAM,
                      __FILE__": (getNormalDashPower) recovering" );
    }
    // exist kickable teammate
    else if ( wm.kickableTeammate()
              && wm.ball().distFromSelf() < 20.0 )
    {
        dash_power = std::min( my_inc * 1.1,
                               ServerParam::i().maxDashPower() );
        dlog.addText( Logger::TEAM,
                      __FILE__": (getNormalDashPower) exist kickable teammate. dash_power=%.1f",
                      dash_power );
    }
    // in offside area
    else if ( wm.self().pos().x > wm.offsideLineX() )
    {
        dash_power = ServerParam::i().maxDashPower();
        dlog.addText( Logger::TEAM,
                      __FILE__": in offside area. dash_power=%.1f",
                      dash_power );
    }
    else if ( wm.ball().pos().x > 25.0
              && wm.ball().pos().x > wm.self().pos().x + 10.0
              && self_min < opp_min - 6
              && mate_min < opp_min - 6 )
    {
        dash_power = bound( ServerParam::i().maxDashPower() * 0.1,
                            my_inc * 0.5,
                            ServerParam::i().maxDashPower() );
        dlog.addText( Logger::TEAM,
                      __FILE__": (getNormalDashPower) opponent ball dash_power=%.1f",
                      dash_power );
    }
    // normal
    else
    {
        dash_power = std::min( my_inc * 1.7,
                               ServerParam::i().maxDashPower() );
        dlog.addText( Logger::TEAM,
                      __FILE__": (getNormalDashPower) normal mode dash_power=%.1f",
                      dash_power );
    }

    return dash_power;
}

bool Strategy::isDefenseSituation(const rcsc::WorldModel &wm, int unum) const{
    const int self_min = wm.interceptTable().selfStep();
    const int mate_min = wm.interceptTable().teammateStep();
    const int opp_min = wm.interceptTable().opponentStep();
    int myTeam_min = std::min(self_min,mate_min);
    int player_min = std::min(myTeam_min,opp_min);
    Vector2D ball_pos = wm.ball().inertiaPoint(player_min);
    double ballX = ball_pos.x;
    int dif = 0;
    PostLine self_line = i().tmLine(unum);

    if(self_line==PostLine::back){
        if( ballX < -20){
            dif = 3;
        }else if( ballX < 20){
            dif = 2;
        }else if( ballX < 40){
            dif = 1;
        }else{
            dif = 0;
        }
    }
    if(self_line==PostLine::half){
        if( ballX < -20){
            dif = 2;
        }else if( ballX < 20){
            dif = 0;
        }else if( ballX < 40){
            dif = -1;
        }else{
            dif = -2;
        }
    }
    if(self_line==PostLine::forward){
        double hafbak_line=0;
        double hafbak_number = 0;
        for(int u=2;u<=11;u++){
            if(i().tmLine(u) != PostLine::half)
                continue;
            const AbstractPlayerObject * tm = wm.ourPlayer(u);
            if(tm!=NULL && tm->unum()==u){
                hafbak_line+=tm->pos().x;
                hafbak_number++;
            }
        }
        if (hafbak_number>0)
            hafbak_line /= hafbak_number;

        if(hafbak_number>0 && ball_pos.x<-25 && hafbak_line < ballX+10){
            dif = 3;
        }
        else if(hafbak_number>0 && ball_pos.x<-25 && hafbak_line < ballX+15){
            dif = 2;
        }else if(ServerParam::i().ourPenaltyArea().contains(ball_pos)){
            dif = 4;
        }else if( ballX < -20){
            dif = 0;
        }else if( ballX < 20){
            dif = -2;
        }else if( ballX < 40){
            dif = -3;
        }else{
            dif = -3;
        }
    }

    if (self_min == myTeam_min && opp_min <= self_min)
        return true;
    if (opp_min - myTeam_min < dif
            || (wm.lastKickerSide() == wm.theirSide() && self_line==PostLine::back)
            || (wm.lastKickerSide() == wm.theirSide() && self_line==PostLine::half && wm.ball().pos().x < -35)) {
        return true;
    } else {
        if(wm.interceptTable().firstTeammate() != nullptr && wm.interceptTable().firstTeammate()->unum() == wm.self().unum() && opp_min <= self_min)
            return true;
        return false;
    }
}
