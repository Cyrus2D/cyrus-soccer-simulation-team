// -*-c++-*-

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
#include "config.h"
#endif

#include "sample_trainer.h"

#include <rcsc/trainer/trainer_command.h>
#include <rcsc/trainer/trainer_config.h>
#include <rcsc/coach/global_world_model.h>
#include <rcsc/common/basic_client.h>
#include <rcsc/common/player_param.h>
#include <rcsc/common/player_type.h>
#include <rcsc/common/server_param.h>
#include <rcsc/param/param_map.h>
#include <rcsc/param/cmd_line_parser.h>
#include <rcsc/random.h>
#include <iostream>

using namespace rcsc;
using namespace std;

/*-------------------------------------------------------------------*/
/*!

 */
SampleTrainer::SampleTrainer()
: TrainerAgent()
{

}

/*-------------------------------------------------------------------*/
/*!

 */
SampleTrainer::~SampleTrainer()
{

}

/*-------------------------------------------------------------------*/
/*!

 */
bool
SampleTrainer::initImpl( CmdLineParser & cmd_parser )
{
	bool result = TrainerAgent::initImpl( cmd_parser );

#if 0
	ParamMap my_params;

	std::string formation_conf;
	my_map.add()
        						( &conf_path, "fconf" )
								;

	cmd_parser.parse( my_params );
#endif

	if ( cmd_parser.failed() )
	{
		std::cerr << "coach: ***WARNING*** detected unsupported options: ";
		cmd_parser.print( std::cerr );
		std::cerr << std::endl;
	}

	if ( ! result )
	{
		return false;
	}

	return true;
}

/*-------------------------------------------------------------------*/
/*!

 */

double random_gen(double min,double max){
	srand(time(0));
	double rand_int = rand();
	double rand_yek = rand_int/(double)RAND_MAX;
	rand_yek*=(max-min) + min;
	return rand_yek;
}
void
SampleTrainer::actionImpl()
{
	if ( world().teamNameLeft().empty() )
	{
		doTeamNames();
		return;
	}
	srand(time(0));
	cout<<"****"<<world().time().cycle()<<endl;
	doChangeMode( PM_PlayOn );
	if(world().time().cycle()<100)
		return;
	static int state = 0;
	for(int i=1;i<11;i++){
		doMovePlayer( config().teamName(),
				i, // uniform number
				Vector2D(-52,i*2),
				0 );

	}
	for(int i=3;i<=11;i++){
		doMovePlayer( world().teamNameRight(),
				i, // uniform number
				Vector2D(52,i*2+20),
				0 );

	}
	doRecover();

	if(exist_opp_kickable()/* || world().gameMode().type()!=PM_PlayOn*/)
		state = 0;
	if(state==0){
		state = 1;
		Vector2D pos11(random_gen(-10,10),random_gen(-10,10));
		doMovePlayer( world().teamNameLeft(),
				11, // uniform number
				pos11,
				0 );
		Vector2D pos2 = Vector2D::polar2vector(random_gen(-10,20),(Vector2D(52,0)-pos11).th()) + pos11 + Vector2D(random_gen(-15,5),random_gen(-5,5));
		doMovePlayer( world().teamNameRight(),
				2, // uniform number
				Vector2D(10,0),
				0 );


		// move ball to center
		doMoveBall( pos11 + Vector2D( 0.5, 0.0 ),
				Vector2D( 0.0, 0.0 ) );
		// change playmode to play_on

	}

	doSay( "move player" );
	//////////////////////////////////////////////////////////////////
	// Add your code here.

	//    sampleAction();
	//recoverForever();
	//doSubstitute();
	//    doKeepaway();
}

/*-------------------------------------------------------------------*/
/*!

 */
#include <cmath>

void
SampleTrainer::sampleAction()
{
	// sample training to test a ball interception.

	static int ball_time_inter = 1000;
	static int xx = 0;
	if(world().existKickablePlayer()){
		xx =1;
		ball_time_inter = world().time().cycle();
	}
	if((ball_time_inter +3 < world().time().cycle() && xx==1)){
		xx=0;
		doMoveBall( Vector2D( 0.0, 0.0 ),
				Vector2D::polar2vector(3,90));
		Vector2D pos = Vector2D::polar2vector(20,90);
		pos += Vector2D(rand()/RAND_MAX*2-1,rand()/RAND_MAX*2-1);
		double angle = rand()/RAND_MAX*360 - 180;
		doMovePlayer( config().teamName(),
				2, // uniform number
				pos,
				angle );
	}
	if(world().ball().pos().y > 30){
		xx=0;
		doMoveBall( Vector2D( 0.0, 0.0 ),
				Vector2D::polar2vector(3,90));
		Vector2D pos = Vector2D::polar2vector(20,90);
		pos += Vector2D(rand()/RAND_MAX*2-1,rand()/RAND_MAX*2-1);
		double angle = rand()/RAND_MAX*360 - 180;
		doMovePlayer( config().teamName(),
				2, // uniform number
				pos,
				angle );
	}

	return;
	static int s_state = 0;
	static int s_wait_counter = 0;

	static Vector2D s_last_player_move_pos;

	if ( world().existKickablePlayer() )
	{
		s_state = 1;
	}

	switch ( s_state ) {
	case 0:
		// nothing to do
		break;
	case 1:
		// exist kickable left player

		// recover stamina
		doRecover();
		// move ball to center
		doMoveBall( Vector2D( 0.0, 0.0 ),
				Vector2D( 0.0, 0.0 ) );
		// change playmode to play_on
		doChangeMode( PM_PlayOn );
		{
			// move player to random point
			UniformReal uni01( 0.0, 1.0 );
			Vector2D move_pos
			= Vector2D::polar2vector( 15.0, //20.0,
					AngleDeg( 360.0 * uni01() ) );
			s_last_player_move_pos = move_pos;

			doMovePlayer( config().teamName(),
					1, // uniform number
					move_pos,
					move_pos.th() - 180.0 );
		}
		// change player type
		{
			static int type = 0;
			doChangePlayerType( world().teamNameLeft(), 1, type );
			type = ( type + 1 ) % PlayerParam::i().playerTypes();
		}

		doSay( "move player" );
		s_state = 2;
		std::cout << "trainer: actionImpl init episode." << std::endl;
		break;
	case 2:
		++s_wait_counter;
		if ( s_wait_counter > 3
				&& ! world().playersLeft().empty() )
		{
			// add velocity to the ball
			//UniformReal uni_spd( 2.7, 3.0 );
			//UniformReal uni_spd( 2.5, 3.0 );
			UniformReal uni_spd( 2.3, 2.7 );
			//UniformReal uni_ang( -50.0, 50.0 );
			UniformReal uni_ang( -10.0, 10.0 );
			Vector2D velocity
			= Vector2D::polar2vector( uni_spd(),
					s_last_player_move_pos.th()
					+ uni_ang() );
			doMoveBall( Vector2D( 0.0, 0.0 ),
					velocity );
			s_state = 0;
			s_wait_counter = 0;
			std::cout << "trainer: actionImpl start ball" << std::endl;
		}
		break;

	}
}

/*-------------------------------------------------------------------*/
/*!

 */
void
SampleTrainer::recoverForever()
{
	if ( world().playersLeft().empty() )
	{
		return;
	}

	if ( world().time().stopped() == 0
			&& world().time().cycle() % 50 == 0 )
	{
		// recover stamina
		doRecover();
	}
}

/*-------------------------------------------------------------------*/
/*!

 */
void
SampleTrainer::doSubstitute()
{
	static bool s_substitute = false;
	if ( ! s_substitute
			&& world().time().cycle() == 0
			&& world().time().stopped() >= 10 )
	{
		std::cerr << "trainer " << world().time() << " team name = "
				<< world().teamNameLeft()
				<< std::endl;

		if ( ! world().teamNameLeft().empty() )
		{
			UniformSmallInt uni( 0, PlayerParam::i().ptMax() );
			doChangePlayerType( world().teamNameLeft(),
					1,
					uni() );

			s_substitute = true;
		}
	}

	if ( world().time().stopped() == 0
			&& world().time().cycle() % 100 == 1
			&& ! world().teamNameLeft().empty() )
	{
		static int type = 0;
		doChangePlayerType( world().teamNameLeft(), 1, type );
		type = ( type + 1 ) % PlayerParam::i().playerTypes();
	}
}

/*-------------------------------------------------------------------*/
/*!

 */
void
SampleTrainer::doKeepaway()
{
	if ( world().trainingTime() == world().time() )
	{
		std::cerr << "trainer: "
				<< world().time()
				<< " keepaway training time." << std::endl;
	}

}

bool SampleTrainer::exist_opp_kickable(){
	for(int i=1;i<=11;i++){
		const GlobalPlayerObject * opp = world().teammate(i);
		if(opp!=NULL){
			const PlayerType * ptype = opp->playerTypePtr();
			if(opp->pos().dist(world().ball().pos()) < ptype->kickableArea())
				return true;
		}
	}
	return false;
}
