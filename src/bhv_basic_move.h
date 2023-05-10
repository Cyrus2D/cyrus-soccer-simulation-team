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
#ifndef BHV_BASIC_MOVE_H
#define BHV_BASIC_MOVE_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/soccer_intention.h>
#include <rcsc/player/intercept_table.h>
#include <iostream>
#include <vector>
using namespace rcsc;
using namespace std;

#include "move_def/cyrus_interceptable.h"
class Bhv_BasicMove: public rcsc::SoccerBehavior {
public:
    Bhv_BasicMove() {
    }

    bool execute(rcsc::PlayerAgent * agent);
    bool intercept_plan(rcsc::PlayerAgent * agent, bool from_block = false);
    bool TurnToTackle(rcsc::PlayerAgent * agent);
    static vector<int> who_goto_goal(PlayerAgent *agent);
    bool DefSitPlan(rcsc::PlayerAgent * agent);
    bool OffSitPlan(rcsc::PlayerAgent * agent);
    bool BackFromOffside(PlayerAgent * agent);
    double getDashPower(const rcsc::PlayerAgent * agent);
    bool off_gotopoint(PlayerAgent * agent,Vector2D target,double power,double distthr);
    int blockDecision(rcsc::PlayerAgent * agent);
    bool pers_scap(PlayerAgent * agent);
    bool voro_unmark(PlayerAgent * agent);
    static bool set_def_neck_with_ball(PlayerAgent * agent, Vector2D targetPoint, const AbstractPlayerObject* opp, int blocker);
};

//class Body_Intercept2022
//    : public BodyAction {
//private:
//    //! if true, agent must save recovery parameter
//    const bool M_save_recovery;
//    //! facing target target point. if specified this, plaeyr try to turn neck to this point.
//    const Vector2D M_face_point;
//    const Vector2D M_origin_target;
//    const double M_origin_dist;
//public:
//    /*!
//      \brief construct with all parameters
//      \param save_recovery if true, agent must save recovery parameter
//      \param face_point desired body facing point
//    */
//    explicit
//        Body_Intercept2022( const bool save_recovery = true,
//                        const Vector2D & face_point = Vector2D::INVALIDATED )
//        : M_save_recovery( save_recovery )
//        , M_face_point( face_point )
//        , M_origin_target( Vector2D::INVALIDATED)
//        , M_origin_dist ( 0 )
//      { }
//
//    explicit
//        Body_Intercept2022( const Vector2D & origin_target,
//                        const Vector2D & face_point = Vector2D::INVALIDATED )
//        : M_save_recovery( false )
//        , M_face_point( face_point )
//        , M_origin_target ( origin_target )
//        , M_origin_dist ( 0 )
//      { }
//    explicit
//        Body_Intercept2022( const Vector2D & origin_target,
//                        const double & origin_dist,
//                        const Vector2D & face_point = Vector2D::INVALIDATED )
//        : M_save_recovery( false )
//        , M_face_point( face_point )
//        , M_origin_target ( origin_target )
//        , M_origin_dist ( origin_dist )
//      { }
//    /*!
//      \brief execute action
//      \param agent pointer to the agent itself
//      \return true if action is performed
//    */
//    bool execute( PlayerAgent * agent );
//    bool executeTackle( PlayerAgent * agent );
//
//
//private:
//
//    /*!
//      \brief if kickable opponent exists, perform special action
//      \param agent pointer to the agent itself
//      \return true if action is performed
//    */
//    bool doKickableOpponentCheck( PlayerAgent * agent );
//
//    /*!
//      \brief calculate best interception point using cached table
//      \param wm const refefence to the WorldModel
//      \param table const pointer to the cached table
//      \return interception info object
//    */
//    rcsc::InterceptInfo getBestIntercept( const WorldModel & wm,
//                                    const InterceptTable * table,
//                                    int &ignore_intercept) const;
//
//    rcsc::InterceptInfo getBestInterceptTackle( const WorldModel & wm,
//                                    const InterceptTable * table,
//                                    int &ignore_intercept) const;
//
//    rcsc::InterceptInfo getBestIntercept_Test( const WorldModel & wm,
//                                         const InterceptTable * table ) const;
//
//    /*!
//      \brief try to perform ball wait action
//      \param agent pointer to the agent itself
//      \param target_point intercept target ball point
//      \param info interception info that is considered now
//      \return true if action is performed
//    */
//    bool doWaitTurn( PlayerAgent * agent,
//                     const Vector2D & target_point,
//                     const InterceptInfo & info );
//
//    /*!
//      \brief adjutment dash action. if possible try to perform turn action.
//      \param agent pointer to agent itself
//      \param target_point intercept target ball point
//      \param info interception info that is considered now
//      \return true if action is performed
//    */
//    bool doInertiaDash( PlayerAgent * agent,
//                        const Vector2D & target_point,
//                        const InterceptInfo & info );
//
//};

#endif

