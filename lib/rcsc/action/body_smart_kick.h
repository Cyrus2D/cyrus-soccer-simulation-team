// -*-c++-*-

/*!
  \file body_smart_kick.h
  \brief smart kick action class header file.
*/

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
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

#ifndef RCSC_ACTION_BODY_SMARTKICK_H
#define RCSC_ACTION_BODY_SMARTKICK_H

#include <rcsc/action/kick_table.h>
#include <rcsc/player/soccer_action.h>
#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/world_model.h>

namespace rcsc {

/*!
  \class Body_SmartKick
  \brief smart kick action
 */
class Body_SmartKick
        : public BodyAction {
private:
    //! target point where the ball should move to
    const Vector2D M_target_point;
    //! desired ball first speed
    const double M_first_speed;
    //! threshold value for the ball first speed
    const double M_first_speed_thr;
    //! maximum number of kick steps
    const int M_max_step;

    //! result kick sequence holder
    KickTable::Sequence M_sequence;

public:
    /*!
      \brief construct with all parameters
      \param target_point target point where the ball should move to
      \param first_speed desired ball first speed
      \param first_speed_thr threshold value for the ball first speed
      \param max_step maximum number of kick steps
    */
    Body_SmartKick( const Vector2D & target_point,
                    const double & first_speed,
                    const double & first_speed_thr,
                    const int max_step )
        : M_target_point( target_point )
        , M_first_speed( first_speed )
        , M_first_speed_thr( first_speed_thr )
        , M_max_step( max_step )
    { }

    bool execute( rcsc::PlayerAgent * agent );

    /*!
      \brief get the result kick sequence
      \return kick sequence object
     */
    const
    KickTable::Sequence & sequence() const
    {
        return M_sequence;
    }

    static int M_kick_table [36][30];
    static void update(const rcsc::WorldModel & wm){



        for(double angle = 0; angle < 360; angle += 10){
            for(double speed = 0.1; speed <= 3; speed += 0.1){
                Vector2D tar = Vector2D::polar2vector(angle, 10) + wm.ball().pos();
                KickTable::Sequence seq;
                if ( rcsc::KickTable::instance().simulate( wm,
                                                           tar,
                                                           speed,
                                                           speed - 0.5,
                                                           1,
                                                           seq ))
                {
                    M_kick_table[static_cast<int>(angle/10)][static_cast<int>(speed*10) - 1] = 1;
                }else if ( rcsc::KickTable::instance().simulate( wm,
                                                                 tar,
                                                                 speed,
                                                                 speed - 0.5,
                                                                 1,
                                                                 seq )){
                    M_kick_table[static_cast<int>(angle/10)][static_cast<int>(speed*10) - 1] = 2;
                }else if ( rcsc::KickTable::instance().simulate( wm,
                                                                 tar,
                                                                 speed,
                                                                 speed - 0.5,
                                                                 1,
                                                                 seq )){
                    M_kick_table[static_cast<int>(angle/10)][static_cast<int>(speed*10) - 1] = 3;
                }else{
                    M_kick_table[static_cast<int>(angle/10)][static_cast<int>(speed*10) - 1] = 4;
                }

//                if(KickTable::instance().simulateOneStep(wm,tar, speed)){
//                    M_kick_table[static_cast<int>(angle/10)][static_cast<int>(speed*10) - 1] = 1;
//                }else if(KickTable::instance().simulateTwoStep(wm,tar, speed)){
//                    M_kick_table[static_cast<int>(angle/10)][static_cast<int>(speed*10) - 1] = 2;
//                }else if(KickTable::instance().simulateThreeStep(wm,tar, speed)){
//                    M_kick_table[static_cast<int>(angle/10)][static_cast<int>(speed*10) - 1] = 3;
//                }else{
//                    M_kick_table[static_cast<int>(angle/10)][static_cast<int>(speed*10) - 1] = 4;
//                }
            }
        }
    }
    static int get_kick_count(const rcsc::WorldModel & wm, double speed, AngleDeg angle){
        if(speed > 3)
            speed = 3;
        if(speed < 0.1)
            speed = 0.1;
        double ang = angle.degree() + 180;
        if(ang < 10)
            ang = 0;
        static int last_update = 0;
        if(last_update !=  wm.time().cycle()){
            update(wm);
            last_update = wm.time().cycle();
        }
        return M_kick_table[static_cast<int>(ang / 10)][static_cast<int>(speed * 10 - 1)];
    }
};

}

#endif
