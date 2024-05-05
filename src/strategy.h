// -*-c++-*-

/*!
  \file strategy.h
  \brief team strategy manager Header File
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

#ifndef STRATEGY_H
#define STRATEGY_H

#include "roles/soccer_role.h"

#include <rcsc/formation/formation.h>
#include <rcsc/geom/vector_2d.h>
#include <rcsc/types.h>
#include <boost/shared_ptr.hpp>
#include <map>
#include <vector>
#include <string>
#include <rcsc/player/abstract_player_object.h>

// # define USE_GENERIC_FACTORY 1

namespace rcsc {
class CmdLineParser;
class WorldModel;
}

enum PositionType {
	Position_Left = -1,
	Position_Center = 0,
	Position_Right = 1,
};

enum SituationType {
	Normal_Situation,
	Offense_Situation,
	Defense_Situation,
	OurSetPlay_Situation,
	OppSetPlay_Situation,
	PenaltyKick_Situation,
};


class Strategy {
public:
    static bool change9_11;
    static const std::string F433_BEFORE_KICK_OFF_CONF;
    static const std::string F433_BEFORE_KICK_OFF_CONF_FOR_OUR_KICK;
    static const std::string F433_DEFENSE_FORMATION_CONF;
    static const std::string F433_DEFENSE_FORMATION_CONF_NO5;
    static const std::string F433_DEFENSE_FORMATION_CONF_NO6;
    static const std::string F433_DEFENSE_FORMATION_CONF_NO56;
    static const std::string F433_DEFENSE_FORMATION_CONF_FOR_NAMIRA;
    static const std::string F433_DEFENSE_FORMATION_CONF_FOR_KN2C;
    static const std::string F433_OFFENSE_FORMATION_CONF_FOR_OXSY;
    static const std::string F433_OFFENSE_FORMATION_CONF_FOR_KN2C;
    static const std::string F433_OFFENSE_FORMATION_CONF_FOR_MT;
    static const std::string F433_OFFENSE_FORMATION_CONF_FOR_HEL;
    static const std::string F433_OFFENSE_FORMATION_CONF;
    static const std::string F433_OFFENSE_FORMATION_CONF_FOR_GLD;
    static const std::string F433_GOAL_KICK_OPP_FORMATION_CONF;
    static const std::string F433_GOAL_KICK_OPP_FORMATION_CONF_FOR_NAMIRA;
    static const std::string F433_GOAL_KICK_OUR_FORMATION_CONF;
    static const std::string F433_KICKIN_OUR_FORMATION_CONF;
    static const std::string F433_SETPLAY_OPP_FORMATION_CONF;
    static const std::string F433_SETPLAY_OUR_FORMATION_CONF;

	static const std::string F0343_BEFORE_KICK_OFF_CONF;
	static const std::string F0343_DEFENSE_FORMATION_CONF;
	static const std::string F0343_OFFENSE_FORMATION_CONF;
	static const std::string F0343_GOAL_KICK_OPP_FORMATION_CONF;
	static const std::string F0343_GOAL_KICK_OUR_FORMATION_CONF;
	static const std::string F0343_KICKIN_OUR_FORMATION_CONF;
	static const std::string F0343_SETPLAY_OPP_FORMATION_CONF;
	static const std::string F0343_SETPLAY_OUR_FORMATION_CONF;

	static const std::string F550_BEFORE_KICK_OFF_CONF;
	static const std::string F550_DEFENSE_FORMATION_CONF;
    static const std::string F550_OFFENSE_FORMATION_CONF_FOR_GLD;
    static const std::string F550_OFFENSE_FORMATION_CONF;
	static const std::string F550_GOAL_KICK_OPP_FORMATION_CONF;
	static const std::string F550_GOAL_KICK_OUR_FORMATION_CONF;
	static const std::string F550_KICKIN_OUR_FORMATION_CONF;
	static const std::string F550_SETPLAY_OPP_FORMATION_CONF;
	static const std::string F550_SETPLAY_OUR_FORMATION_CONF;

	static const std::string Fsh_BEFORE_KICK_OFF_CONF;
	static const std::string Fsh_DEFENSE_FORMATION_CONF;
	static const std::string Fsh_OFFENSE_FORMATION_CONF;
	static const std::string Fsh_GOAL_KICK_OPP_FORMATION_CONF;
	static const std::string Fsh_GOAL_KICK_OUR_FORMATION_CONF;
	static const std::string Fsh_KICKIN_OUR_FORMATION_CONF;
	static const std::string Fsh_SETPLAY_OPP_FORMATION_CONF;
	static const std::string Fsh_SETPLAY_OUR_FORMATION_CONF;

    static const std::string Fhel_BEFORE_KICK_OFF_CONF;
    static const std::string Fhel_DEFENSE_FORMATION_CONF;
    static const std::string Fhel_OFFENSE_FORMATION_CONF;
    static const std::string Fhel_GOAL_KICK_OPP_FORMATION_CONF;
    static const std::string Fhel_GOAL_KICK_OUR_FORMATION_CONF;
    static const std::string Fhel_KICKIN_OUR_FORMATION_CONF;
    static const std::string Fhel_SETPLAY_OPP_FORMATION_CONF;
    static const std::string Fhel_SETPLAY_OUR_FORMATION_CONF;

    static const std::string F523_BEFORE_KICK_OFF_CONF;
    static const std::string F523_DEFENSE_CONF;
    static const std::string F523_GOAL_BLOCK_CONF;
    static const std::string F523_GOAL_KICK_OPP_CONF;
    static const std::string F523_GOAL_KICK_OUR_CONF;
    static const std::string F523_OFFENSE_CONF;
    static const std::string F523_OFFENSE_WALLBREAK_CONF;
    static const std::string F523_KICKIN_OUR_CONF;
    static const std::string F523_SETPLAY_OPP_CONF;
    static const std::string F523_SETPLAY_OUR_CONF;

    enum class FormationType{
        F550,
        F0343,
        Fsh,
        F433,
        HeliosFra,
        F523
	};
    FormationType StringToFormationType(std::string formation){
        if(formation.compare("sh")==0)
        {
            return FormationType::Fsh;
        }
        else if(formation.compare("HeliosFra")==0)
        {
            return FormationType::HeliosFra;
        }
        else if(formation.compare("433")==0)
        {
            return FormationType::F433;
        }
        else if(formation.compare("0343") == 0)
        {
            return FormationType::F0343;
        }
        else if(formation.compare("550") == 0)
        {
            return FormationType::F550;
        }
        else if(formation.compare("523") == 0)
        {
            return FormationType::F523;
        }
        else
        {
            return FormationType::HeliosFra;
        }
    }

	FormationType M_formation_type;
    enum class PostLine{
        golie,
        back,
        half,
        forward
	};
	enum player_post{
		pp_gk,pp_cb,pp_rb,pp_lb,pp_ch,pp_rh,pp_lh,pp_cf,pp_rf,pp_lf
	};
    enum class TeamTactic{
        AllDef, Normal, AllOff
    };
    TeamTactic StringToTeamTactic(std::string formation){
        if(formation.compare("AllDef")==0)
        {
            return TeamTactic::AllDef;
        }
        else if(formation.compare("Normal")==0)
        {
            return TeamTactic::Normal;
        }
        else if(formation.compare("AllOff")==0)
        {
            return TeamTactic::AllOff;
        }
        else
        {
            return TeamTactic::Normal;
        }
    }
    TeamTactic my_team_tactic;

    PostLine self_line;
    PostLine tm_line[12];
	player_post self_post;
	player_post tm_post[12];

    PostLine self_Line(){
		return self_line;
	}
	player_post self_Post(){
		return self_post;
	}
    PostLine tm_Line(size_t unum){
		return tm_line[unum];
	}
    player_post tm_Post(size_t unum){
		return tm_post[unum];
	}

private:
	//
	// factories
	//
#ifndef USE_GENERIC_FACTORY
//typedef std::map< std::string, SoccerRole::Creator > RoleFactory;
//	typedef std::map< std::string, rcsc::Formation::Creator > FormationFactory;
//
//	RoleFactory M_role_factory;
//	FormationFactory M_formation_factory;
    typedef std::map< std::string, SoccerRole::Creator > RoleFactory;

    RoleFactory M_role_factory;
#endif


	//
	// formations
	//

    rcsc::Formation::Ptr M_F433_before_kick_off_formation;
    rcsc::Formation::Ptr M_F433_before_kick_off_formation_for_our_kick;
    rcsc::Formation::Ptr M_F433_defense_formation;
    rcsc::Formation::Ptr M_F433_defense_formation_no5;
    rcsc::Formation::Ptr M_F433_defense_formation_no6;
    rcsc::Formation::Ptr M_F433_defense_formation_no56;
    rcsc::Formation::Ptr M_F433_defense_formation_for_namira;
    rcsc::Formation::Ptr M_F433_defense_formation_for_kn2c;
    rcsc::Formation::Ptr M_F433_offense_formation;
    rcsc::Formation::Ptr M_F433_offense_formation_for_oxsy;
    rcsc::Formation::Ptr M_F433_offense_formation_for_kn2c;
    rcsc::Formation::Ptr M_F433_offense_formation_for_mt;
    rcsc::Formation::Ptr M_F433_offense_formation_for_hel;
    rcsc::Formation::Ptr M_F433_offense_formation_for_gld;
    rcsc::Formation::Ptr M_F433_goal_kick_opp_formation;
    rcsc::Formation::Ptr M_F433_goal_kick_opp_formation_for_namira;
    rcsc::Formation::Ptr M_F433_goal_kick_our_formation;
    rcsc::Formation::Ptr M_F433_kickin_our_formation;
    rcsc::Formation::Ptr M_F433_setplay_opp_formation;
    rcsc::Formation::Ptr M_F433_setplay_our_formation;

    rcsc::Formation::Ptr M_F523_before_kick_off;
    rcsc::Formation::Ptr M_F523_defense;
    rcsc::Formation::Ptr M_F523_goal_block;
    rcsc::Formation::Ptr M_F523_goal_kick_opp;
    rcsc::Formation::Ptr M_F523_goal_kick_our;
    rcsc::Formation::Ptr M_F523_offense;
    rcsc::Formation::Ptr M_F523_offense_wallbreak;
    rcsc::Formation::Ptr M_F523_kickin_our;
    rcsc::Formation::Ptr M_F523_setplay_opp;
    rcsc::Formation::Ptr M_F523_setplay_our;

	rcsc::Formation::Ptr M_F550_before_kick_off_formation;
	rcsc::Formation::Ptr M_F550_defense_formation;
	rcsc::Formation::Ptr M_F550_offense_formation;
    rcsc::Formation::Ptr M_F550_offense_formation_for_gld;
	rcsc::Formation::Ptr M_F550_goal_kick_opp_formation;
	rcsc::Formation::Ptr M_F550_goal_kick_our_formation;
	rcsc::Formation::Ptr M_F550_kickin_our_formation;
	rcsc::Formation::Ptr M_F550_setplay_opp_formation;
	rcsc::Formation::Ptr M_F550_setplay_our_formation;

	rcsc::Formation::Ptr M_F0343_before_kick_off_formation;
	rcsc::Formation::Ptr M_F0343_defense_formation;
	rcsc::Formation::Ptr M_F0343_offense_formation;
	rcsc::Formation::Ptr M_F0343_goal_kick_opp_formation;
	rcsc::Formation::Ptr M_F0343_goal_kick_our_formation;
	rcsc::Formation::Ptr M_F0343_kickin_our_formation;
	rcsc::Formation::Ptr M_F0343_setplay_opp_formation;
	rcsc::Formation::Ptr M_F0343_setplay_our_formation;

	rcsc::Formation::Ptr M_Fsh_before_kick_off_formation;
	rcsc::Formation::Ptr M_Fsh_defense_formation;
	rcsc::Formation::Ptr M_Fsh_offense_formation;
	rcsc::Formation::Ptr M_Fsh_goal_kick_opp_formation;
	rcsc::Formation::Ptr M_Fsh_goal_kick_our_formation;
	rcsc::Formation::Ptr M_Fsh_kickin_our_formation;
	rcsc::Formation::Ptr M_Fsh_setplay_opp_formation;
	rcsc::Formation::Ptr M_Fsh_setplay_our_formation;

    rcsc::Formation::Ptr M_Fhel_before_kick_off_formation;
    rcsc::Formation::Ptr M_Fhel_defense_formation;
    rcsc::Formation::Ptr M_Fhel_offense_formation;
    rcsc::Formation::Ptr M_Fhel_goal_kick_opp_formation;
    rcsc::Formation::Ptr M_Fhel_goal_kick_our_formation;
    rcsc::Formation::Ptr M_Fhel_kickin_our_formation;
    rcsc::Formation::Ptr M_Fhel_setplay_opp_formation;
    rcsc::Formation::Ptr M_Fhel_setplay_our_formation;

	int M_goalie_unum;


	// situation type
	SituationType M_current_situation;

	// role assignment
	std::vector< int > M_role_number;

	// current home positions
	std::vector< PositionType > M_position_types;
	std::vector< rcsc::Vector2D > M_positions;


	// private for singleton
	Strategy();

	// not used
	Strategy( const Strategy & );
	const Strategy & operator=( const Strategy & );
public:
    void set_position(int unum, rcsc::Vector2D tar){
        M_positions[unum - 1] = tar;
    }
	static
	Strategy & instance();

	static
	Strategy & i()
	{
		return instance();
	}

	//
	// initialization
	//

	bool init( rcsc::CmdLineParser & cmd_parser );
	bool read( const std::string & config_dir );


	//
	// update
	//

	void update( const rcsc::WorldModel & wm );


	void exchangeRole( const int unum0,
			const int unum1 );

	//
	// accessor to the current information
	//

	int goalieUnum() const { return M_goalie_unum; }

	int roleNumber( const int unum ) const
	{
		if ( unum < 1 || 11 < unum ) return unum;
		return M_role_number[unum - 1];
	}

	bool isMarkerType( const int unum ) const;
        bool isgoal_forward() const;
        bool isGoal_forward;
        bool is5_forward;


	SoccerRole::Ptr createRole( const int unum,
			const rcsc::WorldModel & wm );
	PositionType getPositionType( const int unum ) const;
	rcsc::Vector2D getPosition( const int unum ) const;
    rcsc::Vector2D getPositionWithBall( const int unum, rcsc::Vector2D ball, const rcsc::WorldModel & wm );
    std::vector<const rcsc::AbstractPlayerObject*> myLineTmms(const rcsc::WorldModel & wm, Strategy::PostLine tm_line);
	bool isDefSit(const rcsc::WorldModel & wm,int unum) const;
    FormationType get_formation_type(){
        return M_formation_type;
    }
	void set_formation_type(FormationType ft){
		M_formation_type = ft;
	}
private:
	void updateSituation( const rcsc::WorldModel & wm );
	// update the current position table
	void updatePosition( const rcsc::WorldModel & wm, rcsc::Formation::Ptr selected_formation = nullptr );

	rcsc::Formation::Ptr readFormation( const std::string & filepath );
	rcsc::Formation::Ptr createFormation( const std::string & type_name ) const;

//	rcsc::Formation::Ptr getFormation( const rcsc::WorldModel & wm );

public:
	static
	double get_normal_dash_power( const rcsc::WorldModel & wm );

    rcsc::Formation::Ptr getFormation( const rcsc::WorldModel & wm );

    rcsc::SideID get_before_kick_off_side(const rcsc::WorldModel & wm);
    bool is_open_deffense(const rcsc::WorldModel & wm);
};

#endif
