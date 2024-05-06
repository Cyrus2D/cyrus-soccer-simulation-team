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


#ifndef USE_GENERIC_FACTORY
#include "roles/role_sample.h"

#include "roles/role_goalie.h"
#include "roles/role_player.h"

#include "chain_action/field_analyzer.h"

#include "setting.h"
#include <rcsc/formation/formation_static.h>
#include <rcsc/formation/formation_dt.h>
#endif

#include <rcsc/player/intercept_table.h>
#include <rcsc/player/world_model.h>
#include <rcsc/formation/formation_parser.h>
#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/param/cmd_line_parser.h>
#include <rcsc/param/param_map.h>
#include <rcsc/game_mode.h>

#include <set>
#include <fstream>
#include <iostream>
#include <cstdio>

using namespace rcsc;
bool Strategy::change9_11 = false;
const std::string Strategy::F433_BEFORE_KICK_OFF_CONF = "F433_before-kick-off.conf";
const std::string Strategy::F433_BEFORE_KICK_OFF_CONF_FOR_OUR_KICK = "F433_before-kick-off_for_our_kick.conf";
const std::string Strategy::F433_DEFENSE_FORMATION_CONF = "F433_defense-formation.conf";
const std::string Strategy::F433_DEFENSE_FORMATION_CONF_NO5 = "F433_defense-formation_no5.conf";
const std::string Strategy::F433_DEFENSE_FORMATION_CONF_NO6 = "F433_defense-formation_no6.conf";
const std::string Strategy::F433_DEFENSE_FORMATION_CONF_NO56 = "F433_defense-formation_no56.conf";
const std::string Strategy::F433_DEFENSE_FORMATION_CONF_FOR_NAMIRA = "F433_defense-formation_for_namira.conf";
const std::string Strategy::F433_DEFENSE_FORMATION_CONF_FOR_KN2C = "F433_defense-formation_for_kn2c.conf";
const std::string Strategy::F433_OFFENSE_FORMATION_CONF = "F433_offense-formation.conf";
const std::string Strategy::F433_OFFENSE_FORMATION_CONF_FOR_OXSY = "F433_offense-formation_for_oxsy.conf";
const std::string Strategy::F433_OFFENSE_FORMATION_CONF_FOR_GLD = "F433_offense-formation_for_gld.conf";
const std::string Strategy::F433_OFFENSE_FORMATION_CONF_FOR_KN2C = "F433_offense-formation_for_kn2c.conf";
const std::string Strategy::F433_OFFENSE_FORMATION_CONF_FOR_MT = "F433_offense-formation_for_mt.conf";
const std::string Strategy::F433_OFFENSE_FORMATION_CONF_FOR_HEL = "F433_offense-formation_for_hel.conf";
const std::string Strategy::F433_GOAL_KICK_OPP_FORMATION_CONF = "F433_goal-kick-opp.conf";
const std::string Strategy::F433_GOAL_KICK_OPP_FORMATION_CONF_FOR_NAMIRA = "F433_goal-kick-opp_for_namira.conf";
const std::string Strategy::F433_GOAL_KICK_OUR_FORMATION_CONF = "F433_goal-kick-our.conf";
const std::string Strategy::F433_KICKIN_OUR_FORMATION_CONF = "F433_kickin-our-formation.conf";
const std::string Strategy::F433_SETPLAY_OPP_FORMATION_CONF = "F433_setplay-opp-formation.conf";
const std::string Strategy::F433_SETPLAY_OUR_FORMATION_CONF = "F433_setplay-our-formation.conf";

    // rcsc::Formation::Ptr M_F523_before_kick_off;
    // rcsc::Formation::Ptr M_F523_defense;
    // rcsc::Formation::Ptr M_F523_goal_block;
    // rcsc::Formation::Ptr M_F523_goal_kick_opp;
    // rcsc::Formation::Ptr M_F523_goal_kick_our;
    // rcsc::Formation::Ptr M_F523_offense;
    // rcsc::Formation::Ptr M_F523_offense_wallbreak;
    // rcsc::Formation::Ptr M_F523_kickin_our;
    // rcsc::Formation::Ptr M_F523_setplay_opp;
    // rcsc::Formation::Ptr M_F523_setplay_our;
const std::string Strategy::F523_BEFORE_KICK_OFF_CONF = "F523-before-kick-off.conf";
const std::string Strategy::F523_DEFENSE_CONF = "F523-defense.conf";
const std::string Strategy::F523_GOAL_BLOCK_CONF = "F523-goal-block.conf";
const std::string Strategy::F523_GOAL_KICK_OPP_CONF = "F523-goal-kick-opp.conf";
const std::string Strategy::F523_GOAL_KICK_OUR_CONF = "F523-goal-kick-our.conf";
const std::string Strategy::F523_OFFENSE_CONF = "F523-offense.conf";
const std::string Strategy::F523_OFFENSE_WALLBREAK_CONF = "F523-offense-wallbreak.conf";
const std::string Strategy::F523_KICKIN_OUR_CONF = "F523-kickin-our.conf";
const std::string Strategy::F523_SETPLAY_OPP_CONF = "F523-setplay-opp.conf";
const std::string Strategy::F523_SETPLAY_OUR_CONF = "F523-setplay-our.conf";

const std::string Strategy::F0343_BEFORE_KICK_OFF_CONF = "F0343_before-kick-off.conf";
const std::string Strategy::F0343_DEFENSE_FORMATION_CONF = "F0343_defense-formation.conf";
const std::string Strategy::F0343_OFFENSE_FORMATION_CONF = "F0343_offense-formation.conf";
const std::string Strategy::F0343_GOAL_KICK_OPP_FORMATION_CONF = "F0343_goal-kick-opp.conf";
const std::string Strategy::F0343_GOAL_KICK_OUR_FORMATION_CONF = "F0343_goal-kick-our.conf";
const std::string Strategy::F0343_KICKIN_OUR_FORMATION_CONF = "F0343_kickin-our-formation.conf";
const std::string Strategy::F0343_SETPLAY_OPP_FORMATION_CONF = "F0343_setplay-opp-formation.conf";
const std::string Strategy::F0343_SETPLAY_OUR_FORMATION_CONF = "F0343_setplay-our-formation.conf";

const std::string Strategy::F550_BEFORE_KICK_OFF_CONF = "F550_before-kick-off.conf";
const std::string Strategy::F550_DEFENSE_FORMATION_CONF = "F550_defense-formation.conf";
const std::string Strategy::F550_OFFENSE_FORMATION_CONF = "F550_offense-formation.conf";
const std::string Strategy::F550_OFFENSE_FORMATION_CONF_FOR_GLD = "F550_offense-formation_for_gld.conf";
const std::string Strategy::F550_GOAL_KICK_OPP_FORMATION_CONF = "F550_goal-kick-opp.conf";
const std::string Strategy::F550_GOAL_KICK_OUR_FORMATION_CONF = "F550_goal-kick-our.conf";
const std::string Strategy::F550_KICKIN_OUR_FORMATION_CONF = "F550_kickin-our-formation.conf";
const std::string Strategy::F550_SETPLAY_OPP_FORMATION_CONF = "F550_setplay-opp-formation.conf";
const std::string Strategy::F550_SETPLAY_OUR_FORMATION_CONF = "F550_setplay-our-formation.conf";

const std::string Strategy::Fsh_BEFORE_KICK_OFF_CONF = "Fsh_before-kick-off.conf";
const std::string Strategy::Fsh_DEFENSE_FORMATION_CONF = "Fsh_defense-formation.conf";
const std::string Strategy::Fsh_OFFENSE_FORMATION_CONF = "Fsh_offense-formation.conf";
const std::string Strategy::Fsh_GOAL_KICK_OPP_FORMATION_CONF = "Fsh_goal-kick-opp.conf";
const std::string Strategy::Fsh_GOAL_KICK_OUR_FORMATION_CONF = "Fsh_goal-kick-our.conf";
const std::string Strategy::Fsh_KICKIN_OUR_FORMATION_CONF = "Fsh_kickin-our-formation.conf";
const std::string Strategy::Fsh_SETPLAY_OPP_FORMATION_CONF = "Fsh_setplay-opp-formation.conf";
const std::string Strategy::Fsh_SETPLAY_OUR_FORMATION_CONF = "Fsh_setplay-our-formation.conf";

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
    : M_goalie_unum( Unum_Unknown ),
      M_current_situation( Normal_Situation ),
      M_role_number( 11, 0 ),
      M_position_types( 11, Position_Center ),
      M_positions( 11 ),
      self_line( PostLine::golie ),
      self_post( pp_gk )
{
#ifndef USE_GENERIC_FACTORY
    //
    // roles
    //

    M_role_factory[RoleSample::name()] = &RoleSample::create;

    M_role_factory[RoleGoalie::name()] = &RoleGoalie::create;
    M_role_factory[RolePlayer::name()] = &RolePlayer::create;

#endif

    for ( size_t i = 0; i < M_role_number.size(); ++i )
    {
        M_role_number[i] = i + 1;
    }


    for(int i=0;i<=11;i++){
        tm_line[i] = PostLine::golie;
        tm_post[i] = pp_gk;
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
    M_F433_defense_formation_no6 = readFormation( configpath + F433_DEFENSE_FORMATION_CONF_NO6 );
    if ( ! M_F433_defense_formation_no6 )
    {
        std::cerr << "Failed to read defense no 6 formation" << std::endl;
        return false;
    }
    M_F433_defense_formation_no56 = readFormation( configpath + F433_DEFENSE_FORMATION_CONF_NO56 );
    if ( ! M_F433_defense_formation_no56 )
    {
        std::cerr << "Failed to read defense no 56 formation" << std::endl;
        return false;
    }
    M_F433_defense_formation_for_namira = readFormation( configpath + F433_DEFENSE_FORMATION_CONF_FOR_NAMIRA );
    if ( ! M_F433_defense_formation_for_namira )
    {
        std::cerr << "Failed to read defense formation" << std::endl;
        return false;
    }
    M_F433_defense_formation_for_kn2c = readFormation( configpath + F433_DEFENSE_FORMATION_CONF_FOR_KN2C );
    if ( ! M_F433_defense_formation_for_kn2c )
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
    M_F433_offense_formation_for_gld = readFormation( configpath + F433_OFFENSE_FORMATION_CONF_FOR_GLD );
    if ( ! M_F433_offense_formation_for_gld )
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
    M_F433_offense_formation_for_hel = readFormation( configpath + F433_OFFENSE_FORMATION_CONF_FOR_HEL );
    if ( ! M_F433_offense_formation_for_hel )
    {
        std::cerr << "Failed to read offense formation" << std::endl;
        return false;
    }
    M_F433_offense_formation_for_kn2c = readFormation( configpath + F433_OFFENSE_FORMATION_CONF_FOR_KN2C );
    if ( ! M_F433_offense_formation_for_kn2c )
    {
        std::cerr << "Failed to read offense formation" << std::endl;
        return false;
    }
    M_F433_goal_kick_opp_formation = readFormation( configpath + F433_GOAL_KICK_OPP_FORMATION_CONF );
    if ( ! M_F433_goal_kick_opp_formation )
    {
        return false;
    }
    M_F433_goal_kick_opp_formation_for_namira = readFormation( configpath + F433_GOAL_KICK_OPP_FORMATION_CONF_FOR_NAMIRA );
    if ( ! M_F433_goal_kick_opp_formation_for_namira )
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

    //sh
    M_Fsh_before_kick_off_formation = readFormation( configpath + Fsh_BEFORE_KICK_OFF_CONF );
    if ( ! M_Fsh_before_kick_off_formation )
    {
        std::cerr << "Failed to read before_kick_off formation" << std::endl;
        return false;
    }
    M_Fsh_defense_formation = readFormation( configpath + Fsh_DEFENSE_FORMATION_CONF );
    if ( ! M_Fsh_defense_formation )
    {
        std::cerr << "Failed to read defense formation" << std::endl;
        return false;
    }
    M_Fsh_offense_formation = readFormation( configpath + Fsh_OFFENSE_FORMATION_CONF );
    if ( ! M_Fsh_offense_formation )
    {
        std::cerr << "Failed to read offense formation" << std::endl;
        return false;
    }
    M_Fsh_goal_kick_opp_formation = readFormation( configpath + Fsh_GOAL_KICK_OPP_FORMATION_CONF );
    if ( ! M_Fsh_goal_kick_opp_formation )
    {
        return false;
    }
    M_Fsh_goal_kick_our_formation = readFormation( configpath + Fsh_GOAL_KICK_OUR_FORMATION_CONF );
    if ( ! M_Fsh_goal_kick_our_formation )
    {
        return false;
    }
    M_Fsh_kickin_our_formation = readFormation( configpath + Fsh_KICKIN_OUR_FORMATION_CONF );
    if ( ! M_Fsh_kickin_our_formation )
    {
        std::cerr << "Failed to read kickin our formation" << std::endl;
        return false;
    }
    M_Fsh_setplay_opp_formation = readFormation( configpath + Fsh_SETPLAY_OPP_FORMATION_CONF );
    if ( ! M_Fsh_setplay_opp_formation )
    {
        std::cerr << "Failed to read setplay opp formation" << std::endl;
        return false;
    }
    M_Fsh_setplay_our_formation = readFormation( configpath + Fsh_SETPLAY_OUR_FORMATION_CONF );
    if ( ! M_Fsh_setplay_our_formation )
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
    //0343
    M_F0343_before_kick_off_formation = readFormation( configpath + F0343_BEFORE_KICK_OFF_CONF );
    if ( ! M_F0343_before_kick_off_formation )
    {
        std::cerr << "Failed to read before_kick_off formation" << std::endl;
        return false;
    }
    M_F0343_defense_formation = readFormation( configpath + F0343_DEFENSE_FORMATION_CONF );
    if ( ! M_F0343_defense_formation )
    {
        std::cerr << "Failed to read defense formation" << std::endl;
        return false;
    }
    M_F0343_offense_formation = readFormation( configpath + F0343_OFFENSE_FORMATION_CONF );
    if ( ! M_F0343_offense_formation )
    {
        std::cerr << "Failed to read offense formation" << std::endl;
        return false;
    }
    M_F0343_goal_kick_opp_formation = readFormation( configpath + F0343_GOAL_KICK_OPP_FORMATION_CONF );
    if ( ! M_F0343_goal_kick_opp_formation )
    {
        return false;
    }
    M_F0343_goal_kick_our_formation = readFormation( configpath + F0343_GOAL_KICK_OUR_FORMATION_CONF );
    if ( ! M_F0343_goal_kick_our_formation )
    {
        return false;
    }
    M_F0343_kickin_our_formation = readFormation( configpath + F0343_KICKIN_OUR_FORMATION_CONF );
    if ( ! M_F0343_kickin_our_formation )
    {
        std::cerr << "Failed to read kickin our formation" << std::endl;
        return false;
    }
    M_F0343_setplay_opp_formation = readFormation( configpath + F0343_SETPLAY_OPP_FORMATION_CONF );
    if ( ! M_F0343_setplay_opp_formation )
    {
        std::cerr << "Failed to read setplay opp formation" << std::endl;
        return false;
    }
    M_F0343_setplay_our_formation = readFormation( configpath + F0343_SETPLAY_OUR_FORMATION_CONF );
    if ( ! M_F0343_setplay_our_formation )
    {
        std::cerr << "Failed to read setplay our formation" << std::endl;
        return false;
    }

    //550
    M_F550_before_kick_off_formation = readFormation( configpath + F550_BEFORE_KICK_OFF_CONF );
    if ( ! M_F550_before_kick_off_formation )
    {
        std::cerr << "Failed to read before_kick_off formation" << std::endl;
        return false;
    }
    M_F550_defense_formation = readFormation( configpath + F550_DEFENSE_FORMATION_CONF );
    if ( ! M_F550_defense_formation )
    {
        std::cerr << "Failed to read defense formation" << std::endl;
        return false;
    }
    M_F550_offense_formation = readFormation( configpath + F550_OFFENSE_FORMATION_CONF );
    if ( ! M_F550_offense_formation )
    {
        std::cerr << "Failed to read offense formation" << std::endl;
        return false;
    }
    M_F550_offense_formation_for_gld = readFormation( configpath + F550_OFFENSE_FORMATION_CONF_FOR_GLD );
    if ( ! M_F550_offense_formation_for_gld )
    {
        std::cerr << "Failed to read offense formation" << std::endl;
        return false;
    }
    M_F550_goal_kick_opp_formation = readFormation( configpath + F550_GOAL_KICK_OPP_FORMATION_CONF );
    if ( ! M_F550_goal_kick_opp_formation )
    {
        return false;
    }
    M_F550_goal_kick_our_formation = readFormation( configpath + F550_GOAL_KICK_OUR_FORMATION_CONF );
    if ( ! M_F550_goal_kick_our_formation )
    {
        return false;
    }
    M_F550_kickin_our_formation = readFormation( configpath + F550_KICKIN_OUR_FORMATION_CONF );
    if ( ! M_F550_kickin_our_formation )
    {
        std::cerr << "Failed to read kickin our formation" << std::endl;
        return false;
    }
    M_F550_setplay_opp_formation = readFormation( configpath + F550_SETPLAY_OPP_FORMATION_CONF );
    if ( ! M_F550_setplay_opp_formation )
    {
        std::cerr << "Failed to read setplay opp formation" << std::endl;
        return false;
    }
    M_F550_setplay_our_formation = readFormation( configpath + F550_SETPLAY_OUR_FORMATION_CONF );
    if ( ! M_F550_setplay_our_formation )
    {
        std::cerr << "Failed to read setplay our formation" << std::endl;
        return false;
    }

    //523
    M_F523_before_kick_off = readFormation( configpath + F523_BEFORE_KICK_OFF_CONF );
    if ( ! M_F523_before_kick_off )
    {
        std::cerr << "Failed to read before_kick_off formation" << std::endl;
        return false;
    }
    M_F523_defense = readFormation( configpath + F523_DEFENSE_CONF );
    if ( ! M_F523_defense )
    {
        std::cerr << "Failed to read defense formation" << std::endl;
        return false;
    }
    M_F523_goal_block = readFormation( configpath + F523_GOAL_BLOCK_CONF );
    if ( ! M_F523_goal_block )
    {
        std::cerr << "Failed to read goal_block formation" << std::endl;
        return false;
    }
    M_F523_goal_kick_opp = readFormation( configpath + F523_GOAL_KICK_OPP_CONF );
    if ( ! M_F523_goal_kick_opp )
    {
        std::cerr << "Failed to read goal_kick_opp formation" << std::endl;
        return false;
    }
    M_F523_goal_kick_our = readFormation( configpath + F523_GOAL_KICK_OUR_CONF );
    if ( ! M_F523_goal_kick_our )
    {
        std::cerr << "Failed to read goal_kick_our formation" << std::endl;
        return false;
    }
    M_F523_offense = readFormation( configpath + F523_OFFENSE_CONF );
    if ( ! M_F523_offense )
    {
        std::cerr << "Failed to read offense formation" << std::endl;
        return false;
    }
    M_F523_offense_wallbreak = readFormation( configpath + F523_OFFENSE_WALLBREAK_CONF );
    if ( ! M_F523_offense_wallbreak )
    {
        std::cerr << "Failed to read offense_wallbreak formation" << std::endl;
        return false;
    }
    M_F523_kickin_our = readFormation( configpath + F523_KICKIN_OUR_CONF );
    if ( ! M_F523_kickin_our )
    {
        std::cerr << "Failed to read kickin_our formation" << std::endl;
        return false;
    }
    M_F523_setplay_opp = readFormation( configpath + F523_SETPLAY_OPP_CONF );
    if ( ! M_F523_setplay_opp )
    {
        std::cerr << "Failed to read setplay_opp formation" << std::endl;
        return false;
    }
    M_F523_setplay_our = readFormation( configpath + F523_SETPLAY_OUR_CONF );
    if ( ! M_F523_setplay_our )
    {
        std::cerr << "Failed to read setplay_our formation" << std::endl;
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

//Formation::Ptr
//Strategy::readFormation( const std::string & filepath )
//{
//    Formation::Ptr f;
//
//    std::ifstream fin( filepath.c_str() );
//    if ( ! fin.is_open() )
//    {
//        std::cerr << __FILE__ << ':' << __LINE__ << ':'
//                  << " ***ERROR*** failed to open file [" << filepath << "]"
//                  << std::endl;
//        return f;
//    }
//
//    std::string temp, type;
//    fin >> temp >> type; // read training method type name
//    fin.seekg( 0 );
//
//    f = createFormation( type );
//
//    if ( ! f )
//    {
//        std::cerr << __FILE__ << ':' << __LINE__ << ':'
//                  << " ***ERROR*** failed to create formation [" << filepath << "]"
//                  << std::endl;
//        return f;
//    }
//
//    //
//    // read data from file
//    //
//    if ( ! f->read( fin ) )
//    {
//        std::cerr << __FILE__ << ':' << __LINE__ << ':'
//                  << " ***ERROR*** failed to read formation [" << filepath << "]"
//                  << std::endl;
//        f.reset();
//        return f;
//    }
//
//
//    //
//    // check role names
//    //
//    for ( int unum = 1; unum <= 11; ++unum )
//    {
//        const std::string role_name = f->getRoleName( unum );
//        if ( role_name == "Savior"
//             || role_name == "Goalie" )
//        {
//            if ( M_goalie_unum == Unum_Unknown )
//            {
//                M_goalie_unum = unum;
//            }
//
//            if ( M_goalie_unum != unum )
//            {
//                std::cerr << __FILE__ << ':' << __LINE__ << ':'
//                          << " ***ERROR*** Illegal goalie's uniform number"
//                          << " read unum=" << unum
//                          << " expected=" << M_goalie_unum
//                          << std::endl;
//                f.reset();
//                return f;
//            }
//        }
//
//
//#ifdef USE_GENERIC_FACTORY
//        SoccerRole::Ptr role = SoccerRole::create( role_name );
//        if ( ! role )
//        {
//            std::cerr << __FILE__ << ':' << __LINE__ << ':'
//                      << " ***ERROR*** Unsupported role name ["
//                      << role_name << "] is appered in ["
//                      << filepath << "]" << std::endl;
//            f.reset();
//            return f;
//        }
//#else
//        if ( M_role_factory.find( role_name ) == M_role_factory.end() )
//        {
//            std::cerr << __FILE__ << ':' << __LINE__ << ':'
//                      << " ***ERROR*** Unsupported role name ["
//                      << role_name << "] is appered in ["
//                      << filepath << "]" << std::endl;
//            f.reset();
//            return f;
//        }
//#endif
//    }
//
//    return f;
//}


/*-------------------------------------------------------------------*/
/*!

 */
//Formation::Ptr
//Strategy::createFormation( const std::string & type_name ) const
//{
//    Formation::Ptr f;
//
//#ifdef USE_GENERIC_FACTORY
//    f = Formation::create( type_name );
//#else
//    FormationFactory::const_iterator creator = M_formation_factory.find( type_name );
//    if ( creator == M_formation_factory.end() )
//    {
//        std::cerr << __FILE__ << ": " << __LINE__
//                  << " ***ERROR*** unsupported formation type ["
//                  << type_name << "]"
//                  << std::endl;
//        return f;
//    }
//    f = creator->second();
//#endif
//
//    if ( ! f )
//    {
//        std::cerr << __FILE__ << ": " << __LINE__
//                  << " ***ERROR*** unsupported formation type ["
//                  << type_name << "]"
//                  << std::endl;
//    }
//
//    return f;
//}

/*-------------------------------------------------------------------*/
/*!

 */
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
    updateFormation(wm);
    updatePosition( wm );

    if (get_formation_type() == FormationType::F433 && getFormation() == M_F433_defense_formation){
        dlog.addText(Logger::TEAM, "Current formation is F433 defense formation");
        bool p5_exist = true;
        bool p6_exist = true;
        double p5_dist_x = 1000;
        double p6_dist_x = 1000;
        const AbstractPlayerObject * p5 = wm.ourPlayer(5);
        if (p5 != nullptr && p5->unum() == 5){
            p5_dist_x = p5->pos().x - getPosition(5).x;
        }
        const AbstractPlayerObject * p6 = wm.ourPlayer(6);
        if (p6 != nullptr && p6->unum() == 6){
            p6_dist_x = p6->pos().x - getPosition(6).x;
        }
        if (p5_dist_x > 15)
            p5_exist = false;
        if (p6_dist_x > 15)
            p6_exist = false;
        dlog.addText(Logger::TEAM, "p5_dist_x = %f, p6_dist_x = %f", p5_dist_x, p6_dist_x);
        dlog.addText(Logger::TEAM, "p5_exist = %d, p6_exist = %d", p5_exist, p6_exist);
        auto new_formation = M_F433_defense_formation;
        if (!p5_exist && !p6_exist){
            new_formation = M_F433_defense_formation_no56;
            dlog.addText(Logger::TEAM, "Change to F433 defense formation no 56");
        }else if (!p5_exist){
            new_formation = M_F433_defense_formation_no5;
            dlog.addText(Logger::TEAM, "Change to F433 defense formation no 5");
        } else if (!p6_exist){
            new_formation = M_F433_defense_formation_no6;
            dlog.addText(Logger::TEAM, "Change to F433 defense formation no 6");
        }
        if (new_formation != M_F433_defense_formation){
            dlog.addText(Logger::TEAM, "Change formation");
            M_selected_formation = new_formation;
            updatePosition(wm);
        }
    }
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

    int role0 = M_role_number[unum0 - 1];
    int role1 = M_role_number[unum1 - 1];

    dlog.addText( Logger::TEAM,
                  __FILE__":(exchangeRole) unum=%d(role=%d) <-> unum=%d(role=%d)",
                  unum0, role0,
                  unum1, role1 );

    M_role_number[unum0 - 1] = role1;
    M_role_number[unum1 - 1] = role0;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool Strategy::isgoal_forward() const{
    return isGoal_forward;
}
bool
Strategy::isMarkerType( const int unum ) const
{
    int number = roleNumber( unum );

    if ( number == 2
         || number == 3
         || number == 4
         || number == 5 )
    {
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
SoccerRole::Ptr
Strategy::createRole( const int unum,
                      const WorldModel & world )
{
    const int number = roleNumber( unum );

    SoccerRole::Ptr role;

    if ( number < 1 || 11 < number )
    {
        std::cerr << __FILE__ << ": " << __LINE__
                  << " ***ERROR*** Invalid player number " << number
                  << std::endl;
        return role;
    }
    updateFormation(world);
    Formation::Ptr f = getFormation();
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

    int self_min = wm.interceptTable().selfStep();
    int mate_min = wm.interceptTable().teammateStep();
    int opp_min = wm.interceptTable().opponentStep();
    int our_min = std::min( self_min, mate_min );

    if ( isDefSit(wm,wm.self().unum())){
        M_current_situation = Defense_Situation;
        return;
    }else{
        M_current_situation = Offense_Situation;
        return;
    }
    if ( opp_min <= our_min - 2 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": Situation Defense" );
        M_current_situation = Defense_Situation;
        return;
    }

    if ( our_min <= opp_min - 2 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": Situation Offense" );
        M_current_situation = Offense_Situation;
        return;
    }

    dlog.addText( Logger::TEAM,
                  __FILE__": Situation Normal" );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Strategy::updatePosition( const WorldModel & wm)
{
    static GameTime s_update_time( 0, 0 );
    if ( s_update_time == wm.time() )
    {
        return;
    }
    s_update_time = wm.time();

    Formation::Ptr f = getFormation();
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

    dlog.addText( Logger::TEAM,
                  __FILE__": HOME POSITION: ball pos=(%.1f %.1f) step=%d",
                  ball_pos.x, ball_pos.y,
                  ball_step );

    M_positions.clear();
    f->getPositions( ball_pos, M_positions );

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
            if ( M_positions[unum-1].x > max_x )
            {
                dlog.addText( Logger::TEAM,
                              "____ %d offside. home_pos_x %.2f -> %.2f",
                              unum,
                              M_positions[unum-1].x, max_x );
                M_positions[unum-1].x = max_x;
            }
        }
    }

    M_position_types.clear();
    change9_11 = false;
    if(wm.interceptTable().firstTeammate() != NULL && wm.interceptTable().firstTeammate()->unum() == 11 && wm.interceptTable().opponentStep() < wm.interceptTable().opponentStep() ){
        if(wm.ball().inertiaPoint(wm.interceptTable().teammateStep()).x > 30){
//            change9_11 = true;
        }
    }
    //change position of 2 to center of 2 and 5
    //TODO should be check in the future
//    bool need_to_change_in_def = false;
//    double def_line_x = wm.ourDefensePlayerLineX();
//    const AbstractPlayerObject * tm5 = wm.ourPlayer(5);
//    if (tm5 == NULL || tm5->unum() < 1)
//        need_to_change_in_def = true;
//    else if (tm5->pos().x > def_line_x + 10 && ball_pos.x > -35){
//        need_to_change_in_def = true;
//    }
//    if(need_to_change_in_def && M_formation_type == FormationType::F433){
//        Vector2D old_hpos2 = M_positions[2 - 1];
//        Vector2D old_hpos3 = M_positions[3 - 1];
//        Vector2D old_hpos4 = M_positions[4 - 1];
//        Vector2D old_hpos5 = M_positions[5 - 1];
//        M_positions[2 - 1] = (old_hpos2 + old_hpos5) / 2.0;
//        M_positions[3 - 1] = (old_hpos5 + old_hpos3 + old_hpos3) / 3.0;
//        M_positions[4 - 1] = (old_hpos2 + old_hpos4 + old_hpos4) / 3.0;
//        M_positions[2 - 1].x = old_hpos2.x;
//        M_positions[3 - 1].x = old_hpos3.x;
//        M_positions[4 - 1].x = old_hpos4.x;
//    }
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
                      M_positions[unum-1].x, M_positions[unum-1].y,
                      type );
        dlog.addCircle( Logger::TEAM,
                        M_positions[unum-1], 0.5,
                        "#000000" );
    }
}


/*-------------------------------------------------------------------*/
/*!

 */
PositionType
Strategy::getPositionType( const int unum ) const
{
    const int number = roleNumber( unum );

    if ( number < 1 || 11 < number )
    {
        std::cerr << __FILE__ << ' ' << __LINE__
                  << ": Illegal number : " << number
                  << std::endl;
        return Position_Center;
    }

    try
    {
        return M_position_types.at( number - 1 );
    }
    catch ( std::exception & e )
    {
        std::cerr<< __FILE__ << ':' << __LINE__ << ':'
                 << " Exception caught! " << e.what()
                 << std::endl;
        return Position_Center;
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
rcsc::Vector2D
Strategy::getPositionWithBall( const int unum, rcsc::Vector2D ball, const WorldModel & wm ){
    try {
        Formation::Ptr f = getFormation();
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
        return positions.at(unum - 1);
    } catch (std::exception & e) {
        std::cout<<"ERRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRrrrrorrr"<<std::endl;
        return Vector2D::INVALIDATED;
    }

}

Vector2D
Strategy::getPosition( const int unum ) const
{
    int number = roleNumber( unum );

    if(change9_11){
        if(number == 11)
            number = 9;
        else if(number == 9)
            number = 11;
    }
    if ( number < 1 || 11 < number )
    {
        std::cerr << __FILE__ << ' ' << __LINE__
                  << ": Illegal number : " << number
                  << std::endl;
        return Vector2D::INVALIDATED;
    }

    try
    {
        if(number == 1 && isgoal_forward()){
            return M_positions.at(9 - 1)+Vector2D(0,-10);
        }
        if(number == 2 && isgoal_forward()){
            return M_positions.at(0);
        }
        if(!is5_forward){
            if(number == 5){
                Vector2D tmp = M_positions.at( number - 1 );
                tmp.x = std::min(5.0,tmp.x);
                return tmp;
            }else if(number == 6){
                Vector2D tmp = (M_positions.at( number - 1 ) + M_positions.at( 5 - 1 ));
                tmp /= 2.0;
                return tmp;
            }else if(number == 8){
                Vector2D tmp = (M_positions.at( number - 1 ) + M_positions.at( 6 - 1 ));
                tmp /= 2.0;
                return tmp;
            }else{
                return M_positions.at( number - 1 );
            }

        }
        return M_positions.at( number - 1 );
    }
    catch ( std::exception & e )
    {
        std::cerr<< __FILE__ << ':' << __LINE__ << ':'
                 << " Exception caught! " << e.what()
                 << std::endl;
        return Vector2D::INVALIDATED;
    }
}


std::vector<const AbstractPlayerObject *>
Strategy::myLineTmms(const WorldModel &wm, Strategy::PostLine tm_line) {
    std::vector<const AbstractPlayerObject *> results;
//    for (AbstractPlayerCont::const_iterator p = wm.ourPlayers().begin(), end =
//            wm.ourPlayers().end(); p != end; ++p) {
    for ( const AbstractPlayerObject * p : wm.ourPlayers() ){
        if (p == NULL)
            continue;
        if (!p->pos().isValid())
            continue;

        if (Strategy::i().tm_Line(p->unum()) == tm_line)
            results.push_back(p);
    }
    return results;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Strategy::updateFormation(const WorldModel &wm )
{
    int our_score = ( wm.ourSide() == LEFT
                      ? wm.gameMode().scoreLeft()
                      : wm.gameMode().scoreRight() );
    int opp_score = ( wm.ourSide() == LEFT
                      ? wm.gameMode().scoreRight()
                      : wm.gameMode().scoreLeft() );

    int time = wm.time().cycle();
    int opp_min = wm.interceptTable().opponentStep();
    int mate_min = std::min(wm.interceptTable().teammateStep(), wm.interceptTable().selfStep());
    auto ball_inertia_pos = wm.ball().inertiaPoint(std::min(opp_min, mate_min));
    isGoal_forward = Setting::i()->mStrategySetting->mIsGoalForward;

    if(our_score > opp_score)
        is5_forward = Setting::i()->mStrategySetting->mIs5ForwardWin;
    else if(our_score < opp_score)
        is5_forward = Setting::i()->mStrategySetting->mIs5ForwardLost;
    else
        is5_forward = Setting::i()->mStrategySetting->mIs5ForwardDraw;

    my_team_tactic = StringToTeamTactic(Setting::i()->mStrategySetting->mTeamTactic);
    if(our_score > opp_score)
    {
        M_formation_type = StringToFormationType(Setting::i()->mStrategySetting->mWinFormation);
    }
    else if(our_score < opp_score)
    {
        M_formation_type = StringToFormationType(Setting::i()->mStrategySetting->mLostFormation);
      //  if (time > 5400 && time < 6000){
    //            M_formation_type = StringToFormationType(string("0343"));
  //      }
//        if (time > 7500){
          //  M_formation_type = StringToFormationType(string("0343"));
        //}
//        if(!FieldAnalyzer::isFRA(wm))
//        {
//            if(time > 5400){
//                M_formation_type = StringToFormationType(string("0343"));
//            }
//            if(FieldAnalyzer::isHelius(wm) && time > 4500){
//                M_formation_type = StringToFormationType(string("0343"));
//            }
//        }
//        if(FieldAnalyzer::isRazi(wm))
//        {
//            if(time > 3500){
//                M_formation_type = StringToFormationType(string("0343"));
//            }
//        }
    }
    else
    {
        M_formation_type = StringToFormationType(Setting::i()->mStrategySetting->mDrawFormation);
    }

    if(M_formation_type == FormationType::F0343){
        tm_line[1] = PostLine::forward;

        tm_line[2] = PostLine::back;
        tm_line[3] = PostLine::back;
        tm_line[4] = PostLine::back;

        tm_line[5] = PostLine::half;
        tm_line[6] = PostLine::half;
        tm_line[7] = PostLine::half;
        tm_line[8] = PostLine::half;

        tm_line[9] = PostLine::forward;
        tm_line[10] = PostLine::forward;
        tm_line[11] = PostLine::forward;

        tm_post[1] = pp_gk;
        tm_post[2] = pp_cb;
        tm_post[3] = pp_lb;
        tm_post[4] = pp_rb;

        tm_post[5] = pp_ch;
        tm_post[6] = pp_ch;
        tm_post[7] = pp_lh;
        tm_post[8] = pp_rh;

        tm_post[9] = pp_lf;
        tm_post[10] = pp_rf;
        tm_post[11] = pp_cf;

        self_line = tm_line[wm.self().unum()];
        self_post = tm_post[wm.self().unum()];

        //
        // play on
        //
        if ( wm.gameMode().type() == GameMode::PlayOn )
        {
            switch ( M_current_situation ) {
            case Defense_Situation:{
                M_selected_formation = M_F0343_defense_formation;
                return;
            }
            case Offense_Situation: {
                M_selected_formation = M_F0343_offense_formation;
                return;
            }
            default:
                break;
            }
            M_selected_formation = M_F0343_offense_formation;
            return;
        }

        //
        // kick in, corner kick
        //
        if ( wm.gameMode().type() == GameMode::KickIn_
             || wm.gameMode().type() == GameMode::CornerKick_ )
        {
            if ( wm.ourSide() == wm.gameMode().side() )
            {
                // our kick-in or corner-kick
                M_selected_formation = M_F0343_kickin_our_formation;
                return;
            }
            else
            {
                M_selected_formation = M_F0343_setplay_opp_formation;
                return;
            }
        }

        //
        // our indirect free kick
        //
        if ( ( wm.gameMode().type() == GameMode::BackPass_
               && wm.gameMode().side() == wm.theirSide() )
             || ( wm.gameMode().type() == GameMode::IndFreeKick_
                  && wm.gameMode().side() == wm.ourSide() ) )
        {
            M_selected_formation = M_F0343_setplay_our_formation;
            return;
        }

        //
        // opponent indirect free kick
        //
        if ( ( wm.gameMode().type() == GameMode::BackPass_
               && wm.gameMode().side() == wm.ourSide() )
             || ( wm.gameMode().type() == GameMode::IndFreeKick_
                  && wm.gameMode().side() == wm.theirSide() ) )
        {
            M_selected_formation = M_F0343_setplay_opp_formation;
            return;
        }

        //
        // after foul
        //
        if ( wm.gameMode().type() == GameMode::FoulCharge_
             || wm.gameMode().type() == GameMode::FoulPush_ )
        {
            if ( wm.gameMode().side() == wm.ourSide() )
            {
                //
                // opponent (indirect) free kick
                //
                M_selected_formation = M_F0343_setplay_opp_formation;
                return;
            }
            else
            {
                //
                // our (indirect) free kick
                //
                M_selected_formation = M_F0343_setplay_our_formation;
                return;
            }
        }

        //
        // goal kick
        //
        if ( wm.gameMode().type() == GameMode::GoalKick_ || wm.gameMode().type() == GameMode::GoalieCatch_)
        {
            if ( wm.gameMode().side() == wm.ourSide() )
            {
                M_selected_formation = M_F0343_goal_kick_our_formation;
                return;
            }
            else
            {
                M_selected_formation = M_F0343_goal_kick_opp_formation;
                return;
            }
        }

        //
        // before kick off
        //
        if ( wm.gameMode().type() == GameMode::BeforeKickOff
             || wm.gameMode().type() == GameMode::AfterGoal_ )
        {
            M_selected_formation = M_F0343_before_kick_off_formation;
            return;
        }

        //
        // other set play
        //
        if ( wm.gameMode().isOurSetPlay( wm.ourSide() ) )
        {
            M_selected_formation = M_F0343_setplay_our_formation;
            return;
        }

        if ( wm.gameMode().type() != GameMode::PlayOn )
        {
            M_selected_formation = M_F0343_setplay_opp_formation;
            return;
        }

        //
        // unknown
        //
        switch ( M_current_situation ) {
        case Defense_Situation: {
            M_selected_formation = M_F0343_defense_formation;
            return;
        }
        case Offense_Situation: {
            M_selected_formation = M_F0343_offense_formation;
            return;
        }
        default:
            break;
        }

        M_selected_formation = M_F0343_offense_formation;
        return;
    }
    else if(M_formation_type == FormationType::Fsh){
        tm_line[1] = PostLine::golie;

        tm_line[2] = PostLine::back;
        tm_line[3] = PostLine::back;
        tm_line[4] = PostLine::back;
        tm_line[5] = PostLine::back;
        tm_line[6] = PostLine::back;

        tm_line[7] = PostLine::half;
        tm_line[8] = PostLine::half;

        tm_line[9] = PostLine::forward;
        tm_line[10] = PostLine::forward;
        tm_line[11] = PostLine::forward;

        tm_post[1] = pp_gk;
        tm_post[2] = pp_cb;
        tm_post[3] = pp_cb;
        tm_post[4] = pp_lb;

        tm_post[5] = pp_rh;
        tm_post[6] = pp_cb;
        tm_post[7] = pp_lh;
        tm_post[8] = pp_rh;

        tm_post[9] = pp_lf;
        tm_post[10] = pp_cf;
        tm_post[11] = pp_cf;

        self_line = tm_line[wm.self().unum()];
        self_post = tm_post[wm.self().unum()];

        //
        // play on
        //
        if ( wm.gameMode().type() == GameMode::PlayOn )
        {
            switch ( M_current_situation ) {
            case Defense_Situation: {
                M_selected_formation = M_Fsh_defense_formation;
                return;
            }
            case Offense_Situation: {
                M_selected_formation = M_Fsh_offense_formation;
                return;
            }
            default:
                break;
            }
            M_selected_formation = M_Fsh_offense_formation;
            return;
        }

        //
        // kick in, corner kick
        //
        if ( wm.gameMode().type() == GameMode::KickIn_
             || wm.gameMode().type() == GameMode::CornerKick_ )
        {
            if ( wm.ourSide() == wm.gameMode().side() )
            {
                // our kick-in or corner-kick
                M_selected_formation = M_Fsh_kickin_our_formation;return;
            }
            else
            {
                M_selected_formation = M_Fsh_setplay_opp_formation;return;
            }
        }

        //
        // our indirect free kick
        //
        if ( ( wm.gameMode().type() == GameMode::BackPass_
               && wm.gameMode().side() == wm.theirSide() )
             || ( wm.gameMode().type() == GameMode::IndFreeKick_
                  && wm.gameMode().side() == wm.ourSide() ) )
        {
            M_selected_formation = M_Fsh_setplay_our_formation;return;
        }

        //
        // opponent indirect free kick
        //
        if ( ( wm.gameMode().type() == GameMode::BackPass_
               && wm.gameMode().side() == wm.ourSide() )
             || ( wm.gameMode().type() == GameMode::IndFreeKick_
                  && wm.gameMode().side() == wm.theirSide() ) )
        {
            M_selected_formation = M_Fsh_setplay_opp_formation;return;
        }

        //
        // after foul
        //
        if ( wm.gameMode().type() == GameMode::FoulCharge_
             || wm.gameMode().type() == GameMode::FoulPush_ )
        {
            if ( wm.gameMode().side() == wm.ourSide() )
            {
                //
                // opponent (indirect) free kick
                //
                M_selected_formation = M_Fsh_setplay_opp_formation;return;
            }
            else
            {
                //
                // our (indirect) free kick
                //
                M_selected_formation = M_Fsh_setplay_our_formation;return;
            }
        }

        //
        // goal kick
        //
        if ( wm.gameMode().type() == GameMode::GoalKick_ || wm.gameMode().type() == GameMode::GoalieCatch_)
        {
            if ( wm.gameMode().side() == wm.ourSide() )
            {
                M_selected_formation = M_Fsh_goal_kick_our_formation;return;
            }
            else
            {
                M_selected_formation = M_Fsh_goal_kick_opp_formation;return;
            }
        }

        //
        // before kick off
        //
        if ( wm.gameMode().type() == GameMode::BeforeKickOff
             || wm.gameMode().type() == GameMode::AfterGoal_ )
        {
            M_selected_formation = M_Fsh_before_kick_off_formation;return;
        }

        //
        // other set play
        //
        if ( wm.gameMode().isOurSetPlay( wm.ourSide() ) )
        {
            M_selected_formation = M_Fsh_setplay_our_formation;return;
        }

        if ( wm.gameMode().type() != GameMode::PlayOn )
        {
            M_selected_formation = M_Fsh_setplay_opp_formation;return;
        }

        //
        // unknown
        //
        switch ( M_current_situation ) {
        case Defense_Situation: {
            M_selected_formation = M_Fsh_defense_formation;return;
        }
        case Offense_Situation: {
            M_selected_formation = M_Fsh_offense_formation;return;
        }
        default:
            break;
        }

        M_selected_formation = M_Fsh_offense_formation;return;
    }
    else if(M_formation_type == FormationType::HeliosFra){
        tm_line[1] = PostLine::golie;

        tm_line[2] = PostLine::back;
        tm_line[3] = PostLine::back;
        tm_line[4] = PostLine::back;
        tm_line[5] = PostLine::back;
        tm_line[6] = PostLine::half;

        tm_line[7] = PostLine::half;
        tm_line[8] = PostLine::forward;

        tm_line[9] = PostLine::forward;
        tm_line[10] = PostLine::forward;
        tm_line[11] = PostLine::forward;

        tm_post[1] = pp_gk;
        tm_post[2] = pp_cb;
        tm_post[3] = pp_cb;
        tm_post[4] = pp_lb;

        tm_post[5] = pp_rb;
        tm_post[6] = pp_ch;
        tm_post[7] = pp_lh;
        tm_post[8] = pp_rh;

        tm_post[9] = pp_lf;
        tm_post[10] = pp_cf;
        tm_post[11] = pp_cf;

        self_line = tm_line[wm.self().unum()];
        self_post = tm_post[wm.self().unum()];

        //
        // play on
        //
        if ( wm.gameMode().type() == GameMode::PlayOn )
        {
            switch ( M_current_situation ) {
            case Defense_Situation: {
                M_selected_formation = M_Fhel_defense_formation;return;
            }
            case Offense_Situation: {
                M_selected_formation = M_Fhel_offense_formation;return;
            }
            default:
                break;
            }
            M_selected_formation = M_Fhel_offense_formation;return;
        }

        //
        // kick in, corner kick
        //
        if ( wm.gameMode().type() == GameMode::KickIn_
             || wm.gameMode().type() == GameMode::CornerKick_ )
        {
            if ( wm.ourSide() == wm.gameMode().side() )
            {
                // our kick-in or corner-kick
                M_selected_formation = M_Fhel_kickin_our_formation;return;
            }
            else
            {
                M_selected_formation = M_Fhel_setplay_opp_formation;return;
            }
        }

        //
        // our indirect free kick
        //
        if ( ( wm.gameMode().type() == GameMode::BackPass_
               && wm.gameMode().side() == wm.theirSide() )
             || ( wm.gameMode().type() == GameMode::IndFreeKick_
                  && wm.gameMode().side() == wm.ourSide() ) )
        {
            M_selected_formation = M_Fhel_setplay_our_formation;return;
        }

        //
        // opponent indirect free kick
        //
        if ( ( wm.gameMode().type() == GameMode::BackPass_
               && wm.gameMode().side() == wm.ourSide() )
             || ( wm.gameMode().type() == GameMode::IndFreeKick_
                  && wm.gameMode().side() == wm.theirSide() ) )
        {
            M_selected_formation = M_Fhel_setplay_opp_formation;return;
        }

        //
        // after foul
        //
        if ( wm.gameMode().type() == GameMode::FoulCharge_
             || wm.gameMode().type() == GameMode::FoulPush_ )
        {
            if ( wm.gameMode().side() == wm.ourSide() )
            {
                //
                // opponent (indirect) free kick
                //
                M_selected_formation = M_Fhel_setplay_opp_formation;return;
            }
            else
            {
                //
                // our (indirect) free kick
                //
                M_selected_formation = M_Fhel_setplay_our_formation;return;
            }
        }

        //
        // goal kick
        //
        if ( wm.gameMode().type() == GameMode::GoalKick_ || wm.gameMode().type() == GameMode::GoalieCatch_)
        {
            if ( wm.gameMode().side() == wm.ourSide() )
            {
                M_selected_formation = M_Fhel_goal_kick_our_formation;return;
            }
            else
            {
                M_selected_formation = M_Fhel_goal_kick_opp_formation;return;
            }
        }

        //
        // before kick off
        //
        if ( wm.gameMode().type() == GameMode::BeforeKickOff
             || wm.gameMode().type() == GameMode::AfterGoal_ )
        {
            M_selected_formation = M_Fhel_before_kick_off_formation;return;
        }

        //
        // other set play
        //
        if ( wm.gameMode().isOurSetPlay( wm.ourSide() ) )
        {
            M_selected_formation = M_Fhel_setplay_our_formation;return;
        }

        if ( wm.gameMode().type() != GameMode::PlayOn )
        {
            M_selected_formation = M_Fhel_setplay_opp_formation;return;
        }

        //
        // unknown
        //
        switch ( M_current_situation ) {
        case Defense_Situation: {
            M_selected_formation = M_Fhel_defense_formation;return;
        }
        case Offense_Situation: {
            M_selected_formation = M_Fhel_offense_formation;return;
        }
        default:
            break;
        }

        M_selected_formation = M_Fhel_offense_formation;return;
    }
    else if(M_formation_type == FormationType::F433){
        tm_line[1] = PostLine::golie;

        tm_line[2] = PostLine::back;
        tm_line[3] = PostLine::back;
        tm_line[4] = PostLine::back;

        if(ball_inertia_pos.x < 15 || opp_min < mate_min){
            tm_line[5] = PostLine::back;
            tm_line[6] = PostLine::back;
            tm_post[5] = pp_cb;
            tm_post[6] = pp_cb;
        }
        else{
            tm_line[5] = PostLine::half;
            tm_line[6] = PostLine::half;
            tm_post[5] = pp_ch;
            tm_post[6] = pp_ch;
        }
        
        tm_line[7] = PostLine::half;
        tm_line[8] = PostLine::half;

        tm_line[9] = PostLine::forward;
        tm_line[10] = PostLine::forward;
        tm_line[11] = PostLine::forward;

        tm_post[1] = pp_gk;
        tm_post[2] = pp_cb;
        tm_post[3] = pp_lb;
        tm_post[4] = pp_rb;


        tm_post[7] = pp_lh;
        tm_post[8] = pp_rh;

        tm_post[9] = pp_lf;
        tm_post[10] = pp_rf;
        tm_post[11] = pp_cf;

        self_line = tm_line[wm.self().unum()];
        self_post = tm_post[wm.self().unum()];

        //
        // play on
        //
        if ( wm.gameMode().type() == GameMode::PlayOn )
        {
            switch ( M_current_situation ) {
            case Defense_Situation:{
                M_selected_formation = M_F433_defense_formation;
                return;
            }
            case Offense_Situation:{
                if(FieldAnalyzer::isGLD(wm)){
                    M_selected_formation = M_F433_offense_formation_for_gld;return;
                }else if(FieldAnalyzer::isHelius(wm)){
                    M_selected_formation = M_F433_offense_formation;return;
                }else if(FieldAnalyzer::isKN2C(wm)){
                    M_selected_formation = M_F433_offense_formation_for_kn2c;return;
                }else if(FieldAnalyzer::isMT(wm)){
                    M_selected_formation = M_F433_offense_formation_for_mt;return;
                }else if(is_open_deffense(wm)){
                    M_selected_formation = M_F433_offense_formation_for_oxsy;return;
                }else{
                    M_selected_formation = M_F433_offense_formation;return;
                }
//                }else if(FieldAnalyzer::isOxsy(wm) || FieldAnalyzer::isMT(wm) || our_score > opp_score){
//                    return M_F433_offense_formation_for_oxsy;
//                }else{
//                    return M_F433_offense_formation;
//                }
            }
            default:
                break;
            }
            M_selected_formation = M_F433_offense_formation;return;
        }

        //
        // kick in, corner kick
        //
        if ( wm.gameMode().type() == GameMode::KickIn_
             || wm.gameMode().type() == GameMode::CornerKick_ )
        {
            if ( wm.ourSide() == wm.gameMode().side() )
            {
                // our kick-in or corner-kick
                M_selected_formation = M_F433_kickin_our_formation;return;
            }
            else
            {
                M_selected_formation = M_F433_setplay_opp_formation;return;
            }
        }

        //
        // our indirect free kick
        //
        if ( ( wm.gameMode().type() == GameMode::BackPass_
               && wm.gameMode().side() == wm.theirSide() )
             || ( wm.gameMode().type() == GameMode::IndFreeKick_
                  && wm.gameMode().side() == wm.ourSide() ) )
        {
            M_selected_formation = M_F433_setplay_our_formation;return;
        }

        //
        // opponent indirect free kick
        //
        if ( ( wm.gameMode().type() == GameMode::BackPass_
               && wm.gameMode().side() == wm.ourSide() )
             || ( wm.gameMode().type() == GameMode::IndFreeKick_
                  && wm.gameMode().side() == wm.theirSide() ) )
        {
            M_selected_formation = M_F433_setplay_opp_formation;return;
        }

        //
        // after foul
        //
        if ( wm.gameMode().type() == GameMode::FoulCharge_
             || wm.gameMode().type() == GameMode::FoulPush_ )
        {
            if ( wm.gameMode().side() == wm.ourSide() )
            {
                //
                // opponent (indirect) free kick
                //
                M_selected_formation = M_F433_setplay_opp_formation;return;
            }
            else
            {
                //
                // our (indirect) free kick
                //
                M_selected_formation = M_F433_setplay_our_formation;return;
            }
        }

        //
        // goal kick
        //
        if ( wm.gameMode().type() == GameMode::GoalKick_ || wm.gameMode().type() == GameMode::GoalieCatch_)
        {
            if ( wm.gameMode().side() == wm.ourSide() )
            {
                M_selected_formation = M_F433_goal_kick_our_formation;return;
            }
            else
            {
                if(FieldAnalyzer::isNamira(wm) && opp_score > our_score) { M_selected_formation = M_F433_goal_kick_opp_formation_for_namira; return;}
                else { M_selected_formation = M_F433_goal_kick_opp_formation; return;}
            }
        }

        //
        // before kick off
        //
        if ( wm.gameMode().type() == GameMode::BeforeKickOff
             || wm.gameMode().type() == GameMode::AfterGoal_ )
        {
            if(wm.gameMode().type() == GameMode::BeforeKickOff){
                if ( wm.ourSide() == get_before_kick_off_side(wm) )
                    M_selected_formation = M_F433_before_kick_off_formation_for_our_kick;
                else
                    M_selected_formation = M_F433_before_kick_off_formation;
                return;
            }else{
                // after our goal
                if ( wm.gameMode().side() == wm.ourSide() )
                {
                    M_selected_formation = M_F433_before_kick_off_formation;return;
                }
                else
                {
                    M_selected_formation = M_F433_before_kick_off_formation_for_our_kick;return;
                }
            }

        }

        //
        // other set play
        //
        if ( wm.gameMode().isOurSetPlay( wm.ourSide() ) )
        {
            M_selected_formation = M_F433_setplay_our_formation;return;
        }

        if ( wm.gameMode().type() != GameMode::PlayOn )
        {
            M_selected_formation = M_F433_setplay_opp_formation;return;
        }

        //
        // unknown
        //
        switch ( M_current_situation ) {
        case Defense_Situation: {
            M_selected_formation = M_F433_defense_formation;return;
        }
        case Offense_Situation: {
            M_selected_formation = M_F433_offense_formation;return;
        }
        default:
            break;
        }

        M_selected_formation = M_F433_offense_formation;return;
    }
    else if(M_formation_type == FormationType::F523){
        tm_line[1] = PostLine::golie;
        tm_post[1] = pp_gk;

        tm_line[2] = PostLine::back;
        tm_line[3] = PostLine::back;
        tm_line[4] = PostLine::back;
        
        tm_post[2] = pp_cb;
        tm_post[3] = pp_cb;
        tm_post[4] = pp_cb;

        if(ball_inertia_pos.x < 15 || opp_min < mate_min - 2){
            tm_line[6] = PostLine::back;
            tm_line[5] = PostLine::back;
            tm_post[6] = pp_lb;
            tm_post[5] = pp_rb;
        }
        else{
            tm_line[6] = PostLine::half;
            tm_line[5] = PostLine::half;
            tm_post[6] = pp_lh;
            tm_post[5] = pp_rh;
        }
        tm_line[7] = PostLine::half;
        tm_line[8] = PostLine::half;
        tm_post[7] = pp_ch;
        tm_post[8] = pp_ch;

        tm_line[9] = PostLine::forward;
        tm_line[10] = PostLine::forward;
        tm_line[11] = PostLine::forward;
        tm_post[9] = pp_lf;
        tm_post[10] = pp_rf;
        tm_post[11] = pp_cf;

        self_line = tm_line[wm.self().unum()];
        self_post = tm_post[wm.self().unum()];

        //
        // play on
        //
        if ( wm.gameMode().type() == GameMode::PlayOn )
        {
            switch ( M_current_situation ) {
            case Defense_Situation:{
                if (ball_inertia_pos.x < -30) { M_selected_formation = M_F523_goal_block; return;}
                M_selected_formation = M_F523_defense;
                return;
            }
            case Offense_Situation:{
                M_selected_formation = M_F523_offense;return;
            }
            default:
                break;
            }
            M_selected_formation = M_F523_offense;return;
        }

        //
        // kick in, corner kick
        //
        if ( wm.gameMode().type() == GameMode::KickIn_
             || wm.gameMode().type() == GameMode::CornerKick_ )
        {
            if ( wm.ourSide() == wm.gameMode().side() )
            {
                // our kick-in or corner-kick
                M_selected_formation = M_F523_kickin_our;
            }
            else
            {
                M_selected_formation = M_F523_setplay_opp;
            }
            return;
        }

        //
        // our indirect free kick
        //
        if ( ( wm.gameMode().type() == GameMode::BackPass_
               && wm.gameMode().side() == wm.theirSide() )
             || ( wm.gameMode().type() == GameMode::IndFreeKick_
                  && wm.gameMode().side() == wm.ourSide() ) )
        {
            M_selected_formation = M_F523_setplay_our;return;
        }

        //
        // opponent indirect free kick
        //
        if ( ( wm.gameMode().type() == GameMode::BackPass_
               && wm.gameMode().side() == wm.ourSide() )
             || ( wm.gameMode().type() == GameMode::IndFreeKick_
                  && wm.gameMode().side() == wm.theirSide() ) )
        {
            M_selected_formation = M_F523_setplay_opp;return;
        }

        //
        // after foul
        //
        if ( wm.gameMode().type() == GameMode::FoulCharge_
             || wm.gameMode().type() == GameMode::FoulPush_ )
        {
            if ( wm.gameMode().side() == wm.ourSide() )
            {
                //
                // opponent (indirect) free kick
                //
                M_selected_formation = M_F523_setplay_opp;return;
            }
            else
            {
                //
                // our (indirect) free kick
                //
                M_selected_formation = M_F523_setplay_our;return;
            }
        }

        //
        // goal kick
        //
        if ( wm.gameMode().type() == GameMode::GoalKick_ || wm.gameMode().type() == GameMode::GoalieCatch_)
        {
            if ( wm.gameMode().side() == wm.ourSide() )
            {
                M_selected_formation = M_F523_goal_kick_our;return;
            }
            else
            {
                M_selected_formation = M_F523_goal_kick_opp;return;
            }
        }

        //
        // before kick off
        //
        if ( wm.gameMode().type() == GameMode::BeforeKickOff
             || wm.gameMode().type() == GameMode::AfterGoal_ )
        {
            if(wm.gameMode().type() == GameMode::BeforeKickOff){
                if ( wm.ourSide() == get_before_kick_off_side(wm) )
                    M_selected_formation = M_F523_before_kick_off;
                else
                    M_selected_formation = M_F523_before_kick_off;
            }else{
                // after our goal
                if ( wm.gameMode().side() == wm.ourSide() )
                {
                    M_selected_formation = M_F523_before_kick_off;
                }
                else
                {
                    M_selected_formation = M_F523_before_kick_off;
                }
            }
            return;
        }

        //
        // other set play
        //
        if ( wm.gameMode().isOurSetPlay( wm.ourSide() ) )
        {
            M_selected_formation = M_F523_setplay_our;return;
        }

        if ( wm.gameMode().type() != GameMode::PlayOn )
        {
            M_selected_formation = M_F523_setplay_opp;return;
        }

        //
        // unknown
        //
        switch ( M_current_situation ) {
        case Defense_Situation: {
            M_selected_formation = M_F523_defense;
            return;
        }
        case Offense_Situation: {
            M_selected_formation = M_F523_offense;
            return;
        }
        default:
            break;
        }

        M_selected_formation = M_F523_offense;return;
    }

    M_selected_formation = M_F433_offense_formation;return;
}

SideID Strategy::get_before_kick_off_side(const WorldModel &wm)
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

bool Strategy::is_open_deffense(const WorldModel &wm)
{
    int our_score = ( wm.ourSide() == LEFT
                      ? wm.gameMode().scoreLeft()
                      : wm.gameMode().scoreRight() );
    int opp_score = ( wm.ourSide() == LEFT
                      ? wm.gameMode().scoreRight()
                      : wm.gameMode().scoreLeft() );
    //if(FieldAnalyzer::isOxsy(wm)
    //        || FieldAnalyzer::isKN2C(wm))
    //    return true;
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
Strategy::get_normal_dash_power( const WorldModel & wm )
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
                      __FILE__": (get_normal_dash_power) correct DF line. keep max power" );
        // keep max power
        dash_power = ServerParam::i().maxDashPower();
    }
    else if ( s_recover_mode )
    {
        dash_power = my_inc - 25.0; // preffered recover value
        if ( dash_power < 0.0 ) dash_power = 0.0;

        dlog.addText( Logger::TEAM,
                      __FILE__": (get_normal_dash_power) recovering" );
    }
    // exist kickable teammate
    else if ( wm.kickableTeammate()
              && wm.ball().distFromSelf() < 20.0 )
    {
        dash_power = std::min( my_inc * 1.1,
                               ServerParam::i().maxDashPower() );
        dlog.addText( Logger::TEAM,
                      __FILE__": (get_normal_dash_power) exist kickable teammate. dash_power=%.1f",
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
                      __FILE__": (get_normal_dash_power) opponent ball dash_power=%.1f",
                      dash_power );
    }
    // normal
    else
    {
        dash_power = std::min( my_inc * 1.7,
                               ServerParam::i().maxDashPower() );
        dlog.addText( Logger::TEAM,
                      __FILE__": (get_normal_dash_power) normal mode dash_power=%.1f",
                      dash_power );
    }

    return dash_power;
}

bool Strategy::isDefSit(const WorldModel & wm,int unum) const{
    const int self_min = wm.interceptTable().selfStep();
    const int mate_min = wm.interceptTable().teammateStep();
    const int opp_min = wm.interceptTable().opponentStep();
    int myTeam_min = std::min(self_min,mate_min);
    int player_min = std::min(myTeam_min,opp_min);
    Vector2D ball_pos = wm.ball().inertiaPoint(player_min);
    double ballX = ball_pos.x;
    int dif = 0;
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
            if(tm_line[u-1] != PostLine::half)
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

    double zarib = 1;
    switch (dif) {
    case -4:
        zarib = 1.3;
        break;
    case -3:
        zarib = 1.2;
        break;
    case -2:
        zarib = 1.1;
        break;
    case -1:
        zarib = 1.05;
        break;
    case 0:
        zarib = 1.0;
        break;
    case 1:
        zarib = 0.95;
        break;
    case 2:
        zarib = 0.9;
        break;
    case 3:
        zarib = 0.8;
        break;
    case 4:
        zarib = 0.7;
        break;

    default:
        break;
    }
    //    if( double(myTeam_min) / double(opp_min) > 0.8
    //            ||(wm.lastKickerSide() == wm.theirSide() && self_line==PostLine::back)){
    //        return true;
    //    }else{
    //        return false;
    //    }
    if (self_min == myTeam_min && opp_min <= self_min)
        return true;
    if (opp_min - myTeam_min < dif
            || (wm.lastKickerSide() == wm.theirSide() && self_line==PostLine::back)
            || (wm.lastKickerSide() == wm.theirSide() && self_line==PostLine::half && wm.ball().pos().x < -35)) {
        return true;
    } else {
        if(wm.interceptTable().firstTeammate() != NULL && wm.interceptTable().firstTeammate()->unum() == wm.self().unum() && opp_min <= self_min)
            return true;
        return false;
    }
}
