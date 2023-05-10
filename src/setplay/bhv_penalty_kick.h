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

#ifndef TOKYOTECH_BHV_PENALTY_KICK_H
#define TOKYOTECH_BHV_PENALTY_KICK_H

#include <rcsc/player/soccer_action.h>
#include <rcsc/geom/vector_2d.h>

class Bhv_PenaltyKick
    : public rcsc::SoccerBehavior {
private:

public:

    bool execute( rcsc::PlayerAgent * agent );

private:

    bool doKickerWait( rcsc::PlayerAgent * agent );
    bool doKickerSetup( rcsc::PlayerAgent * agent );
    bool doKickerReady( rcsc::PlayerAgent * agent );
    bool doKicker( rcsc::PlayerAgent * agent );

    bool doMove(rcsc::PlayerAgent * agent,const rcsc::Vector2D &  point,double thr);

    bool  setTargetline( rcsc::PlayerAgent * agent);
    // used only for one kick PK
    bool doOneKickShoot( rcsc::PlayerAgent * agent );
    // used only for multi kick PK
    bool doShoot( rcsc::PlayerAgent * agent );
    bool getShootTarget( const rcsc::PlayerAgent * agent,
                         rcsc::Vector2D * point,
                         double * first_speed );
    bool doDribble( rcsc::PlayerAgent * agent );
    bool doDribbleWhitball( rcsc::PlayerAgent * agent );
    bool doforceshoot(rcsc::PlayerAgent * agent);
    rcsc::Vector2D getPoint( rcsc::PlayerAgent * agent );
    rcsc::Vector2D getHoldPos( rcsc::PlayerAgent * agent );
    rcsc::Vector2D getDriblePos( rcsc::PlayerAgent * agent );

    bool doGoalieWait( rcsc::PlayerAgent * agent );
    bool doGoalieSetup( rcsc::PlayerAgent * agent );
    bool doGoalie( rcsc::PlayerAgent * agent );
    bool doSidedash(rcsc::PlayerAgent * agent ,
    		const rcsc::Vector2D & Target,double thr);
    bool doGoalieBasicMove( rcsc::PlayerAgent * agent );
    rcsc::Vector2D getGoalieMovePos( const rcsc::Vector2D & ball_pos,
                                     const rcsc::Vector2D & my_pos );
    bool isGoalieBallChaseSituation();

    bool doGoalieSlideChase( rcsc::PlayerAgent * agent );
};
#endif







    /********************************************************************************/

    /********************************************************************************/
    /*
     * Perspolis_Holld_ball.h
     *
     *  Created on: Mar 31, 2013
     *      Author: hossein
     */

    #ifndef BHV_PERSPOLIS_HOLLD_BALL_H_
    #define BHV_PERSPOLIS_HOLLD_BALL_H_







    // -*-c++-*-

    /*!
      \file Perspolis_hold_ball.h
      \brief stay there and keep the ball from opponent players.
    */

    /*
     *Copyright:

     Copyright (C) Hidehisa AKIYAMA

     This code is free software; you can redistribute it and/or
     modify it under the terms of the GNU Lesser General Public
     License as published by the Free Software Foundation; either
     version 3 of the License, or (at your option) any later version.

     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Lesser General Public License for more details.

     You should have received a copy of the GNU Lesser General Public
     License along with this library; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

     *EndCopyright:
     */
    #include <rcsc/player/soccer_action.h>
    #include <rcsc/geom/vector_2d.h>

    #include <vector>

    using namespace rcsc;

    namespace rcsc {

    class PlayerObject;
    class WorldModel;
    }
    /*!
      \class Perspolis_HoldBall
      \brief stay there and keep the ball from opponent players.
    */
    class Perspolis_HoldBall
        : public rcsc::BodyAction {
    public:

        //! default score value
        static const double DEFAULT_SCORE;

        /*!
          \struct KeepPoint
          \brief keep point info
         */
        struct KeepPoint {
            Vector2D pos_; //!< global position
            double kick_rate_; //!< kick rate at the position
            double score_; //!< evaluated score

            /*!
              \brief construct an illegal object
             */
            KeepPoint()
                : pos_( rcsc::Vector2D::INVALIDATED )
                , kick_rate_( 0.0 )
                , score_( -100000.0 )
              { }

            /*!
              \brief construct with arguments
              \param pos global position
              \param krate kick rate at the position
              \param score initial score
             */
            KeepPoint( const Vector2D & pos,
                       const double & krate,
                       const double & score )
                : pos_( pos )
                , kick_rate_( krate )
                , score_( score )
              { }

            /*!
              \brief reset object value
             */
            void reset()
              {
                  pos_.invalidate();
                  kick_rate_ = 0.0;
                  score_ = -100000.0;
              }
        };


    private:
        //! if true, agent will try to face to the target point
        const bool M_do_turn;
        //! face target point
        const Vector2D M_turn_target_point;
        //! next kick target point (if exist)
        const Vector2D M_kick_target_point;
    public:
        /*!
          \brief construct with all parameters
          \param do_turn if true, agent will try to face to the target point
          \param turn_target_point face target point
          \param kick_target_point intended next kick target point
        */
        Perspolis_HoldBall( const bool do_turn = false,
                           const Vector2D & turn_target_point = Vector2D( 0.0, 0.0 ),
                           const Vector2D & kick_target_point = Vector2D::INVALIDATED )
            : M_do_turn( do_turn )
            , M_turn_target_point( turn_target_point )
            , M_kick_target_point( kick_target_point )
          { }

        /*!
          \brief execute action
          \param agent pointer to the agent itself
          \return true if action is performed
        */
        bool execute( PlayerAgent * agent );


    private:

        /*!
          \brief kick the ball to avoid opponent
          \param agent pointer to agent itself
          \return true if action is performed
        */
        bool avoidOpponent( PlayerAgent * agent );

        /*!
          \brief search the best keep point
          \param wm const reference to the WorldModel instance
          \return estimated best keep point. if no point, INVALIDATED is returned.
         */
        Vector2D searchKeepPoint( const WorldModel & wm );

        /*!
          \brief create candidate keep points
          \param wm const reference to the WorldModel instance
          \param keep_points reference to the variable container
         */
        void createKeepPoints( const WorldModel & wm,
                               std::vector< KeepPoint > & keep_points );

        /*!
          \brief evaluate candidate keep points
          \param wm const reference to the WorldModel instance
          \param keep_points reference to the variable container
         */
        void evaluateKeepPoints( const WorldModel & wm,
                                 std::vector< KeepPoint > & keep_points );

        /*!
          \brief evaluate the keep point
          \param wm const reference to the WorldModel instance
          \param keep_point keep point value
         */
        double evaluateKeepPoint( const WorldModel & wm,
                                  const Vector2D & keep_point );

        /*!
          \brief if possible, turn to face target point
          \param agent agent pointer to agent itself
          \return true if action is performed
        */
        bool turnToPoint( PlayerAgent * agent );

        /*!
          \brief keep the ball at body front
          \param agent pointer to agent itself
          \return true if action is performed
        */
        bool keepFront( PlayerAgent * agent );

        /*!
          \brief keep the ball at reverse point from the kick target point
          \param agent pointer to agent itself
         */
        bool keepReverse( PlayerAgent * agent );

    };


    #endif /* PERSPOLIS_HOLLD_BALL_H_ */



