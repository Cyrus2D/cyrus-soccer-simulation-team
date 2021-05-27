// -*-c++-*-

/*!
  \file player_object.cpp
  \brief player object class Source File
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

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "player_object.h"
#include "fullstate_sensor.h"
#include "stamina_with_pointto.h"

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/common/player_type.h>

#define DEBUG_PRINT

namespace rcsc {

int PlayerObject::S_pos_count_thr = 30;
int PlayerObject::S_pos_opp_goalie_count_thr = 100;
int PlayerObject::S_vel_count_thr = 5;
int PlayerObject::S_face_count_thr = 2;

/*-------------------------------------------------------------------*/
/*!

*/
PlayerObject::PlayerObject()
    : AbstractPlayerObject()
    , M_ghost_count( 0 )
    , M_rpos( Vector2D::INVALIDATED )
    , M_rpos_count( 1000 )
    , M_pointto_angle( 0.0 )
    , M_pointto_count( 1000 )
    , M_tackle_count( 1000 )
{

}

/*-------------------------------------------------------------------*/
/*!

*/
PlayerObject::PlayerObject( const SideID side,
                            const Localization::PlayerT & p )
    : AbstractPlayerObject( side, p )
    , M_ghost_count( 0 )
    , M_rpos( p.rpos_ )
    , M_pointto_angle( 0.0 )
    , M_pointto_count( 1000 )
    , M_tackle_count( 1000 )
    , M_heard_stamina( -1.0 )


{
    M_dist_from_self = p.rpos_.r();

    if ( p.hasVel() )
    {
        M_vel = p.vel_;
        M_vel_count = 0;
    }

    if ( p.hasAngle() )
    {
        M_body = p.body_;
        M_body_count = 0;
        M_face = p.face_;
        M_face_count = 0;
    }

    if ( p.isPointing() )
    {
        M_pointto_angle = p.arm_;
        M_pointto_count = 0;
        M_seen_stamina_count = 0;
        StaminaWithPointto::setStaminaFromVisionData(M_seen_stamina, M_pointto_angle.degree());
    }

    M_kicked = p.kicked_;

    if ( p.isTackling() )
    {
        if ( M_tackle_count > ServerParam::i().tackleCycles() ) // no tackling recently
        {
            M_tackle_count = 0;
        }
    }
    else
    {
        M_tackle_count = 1000;
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
PlayerObject::set_count_thr( const int pos_thr,
                             const int vel_thr,
                             const int face_thr )
{
    S_pos_count_thr = pos_thr;
    S_vel_count_thr = vel_thr;
    S_face_count_thr = face_thr;
}

void
PlayerObject::set_count_thr_opp_goalie( const int pos_thr,
                                        const int vel_thr,
                                        const int face_thr )
{
    S_pos_opp_goalie_count_thr = pos_thr;
    S_vel_count_thr = vel_thr;
    S_face_count_thr = face_thr;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
PlayerObject::isTackling() const
{
    return M_tackle_count <= ServerParam::i().tackleCycles() - 2;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
PlayerObject::isKickable( const double & buf ) const
{
    if ( ! M_player_type )
    {
        return distFromBall() < ServerParam::i().defaultKickableArea();
    }

    return distFromBall() < M_player_type->kickableArea() - buf;
}

bool
PlayerObject::isTackleable( rcsc::Vector2D ball_pos, const double & buf ) const
{
    double tackle_dist = ServerParam::i().tackleDist();
    double tackle_width = ServerParam::i().tackleWidth();
    double body = M_body.degree();
    double ball_angle = (ball_pos - M_pos).th().degree();
    double dif_angle = body - ball_angle;
    if(dif_angle<0)dif_angle *= (-1.0);
    if(dif_angle>180)dif_angle = 360 - dif_angle;
    if(dif_angle > 90)
        return false;
    Line2D body_line = Line2D(M_pos,body);
    Vector2D ball_proj = body_line.projection(ball_pos);
    if(ball_proj.dist(M_pos) > tackle_dist)
        return false;
    if(body_line.dist(ball_pos) > tackle_width)
        return false;
    return true;
}
/*-------------------------------------------------------------------*/
/*!

*/
void
PlayerObject::update()
{
    if ( velValid() )
    {
        M_pos += M_vel;
        // speed is not decayed in internal update.
    }
    /*
    // wind effect
    //vel += wind_effect( ServerParam::i().windRand(),
    //                          ServerParam::i().windForce(),
    //                          ServerParam::i().windDir());
    */
    M_unum_count = std::min( 1000, M_unum_count + 1 );
    M_pos_count = std::min( 1000, M_pos_count + 1 );
    M_rpos_count = std::min( 1000, M_rpos_count + 1 );
    M_seen_pos_count = std::min( 1000, M_seen_pos_count + 1 );
    M_heard_pos_count = std::min( 1000, M_heard_pos_count + 1 );
    M_vel_count = std::min( 1000, M_vel_count + 1 );
    M_body_count = std::min( 1000, M_body_count + 1 );
    M_face_count = std::min( 1000, M_face_count + 1 );
    M_pointto_count = std::min( 1000, M_pointto_count + 1 );
    M_seen_stamina_count = std::min( 1000, M_seen_stamina_count + 1 );
    M_kicked = false;
    M_tackle_count = std::min( 1000, M_tackle_count + 1 );

    M_heard_stamina -= ServerParam::i().maxDashPower();
    M_seen_stamina -= ServerParam::i().maxDashPower();
    if ( playerTypePtr() )
    {
        M_heard_stamina += playerTypePtr()->staminaIncMax();
        M_seen_stamina += playerTypePtr()->staminaIncMax();
    }
    else
    {
        M_heard_stamina += ServerParam::i().defaultStaminaIncMax();
        M_seen_stamina += ServerParam::i().defaultStaminaIncMax();
    }
    if ( M_heard_stamina < -1.0 ) M_heard_stamina = -1.0;
    if ( M_seen_stamina < -1.0 ) M_seen_stamina = -1.0;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
PlayerObject::updateBySee( const SideID side,
                           const Localization::PlayerT & p )
{
    M_side = side;
    M_ghost_count = 0;

    // unum is updated only when unum is seen.
    if ( p.unum_ != Unum_Unknown )
    {
        M_unum = p.unum_;
        M_unum_count = 0;
#ifdef DEBUG_PRINT
        dlog.addText(Logger::WORLD,"unum is :%d",M_unum);
#endif
        if ( ! p.goalie_ )
        {
            // when unum is seen, goalie info is also seen
            M_goalie = false;
        }
    }

    if ( p.goalie_ )
    {
        M_goalie = true;
    }

    const Vector2D last_seen_move = p.pos_ - M_seen_pos;
    const int last_seen_pos_count = M_seen_pos_count;

    if ( p.hasVel() )
    {
        M_vel = p.vel_;
        M_vel_count = 0;
        M_seen_vel = p.vel_;
        M_seen_vel_count = 0;

#ifdef DEBUG_PRINT
        dlog.addText( Logger::WORLD,
                      __FILE__" (updateBySee) unum=%d. pos=(%.2f, %.2f) vel=(%.2f, %.2f)",
                      p.unum_, M_pos.x, M_pos.y, M_vel.x, M_vel.y );
#endif
    }
    else if ( 0 < M_pos_count
              && M_pos_count <= 2
              && p.rpos_.r() < 40.0 )
    {
        const double speed_max = ( M_player_type
                                   ? M_player_type->playerSpeedMax()
                                   : ServerParam::i().defaultPlayerSpeedMax() );
        const double decay = ( M_player_type
                               ? M_player_type->playerDecay()
                               : ServerParam::i().defaultPlayerDecay() );

        M_vel = last_seen_move / static_cast< double >( last_seen_pos_count );
        double tmp = M_vel.r();
        if ( tmp > speed_max )
        {
            M_vel *= speed_max / tmp;
        }
        M_vel *= decay;
        M_vel_count = last_seen_pos_count;

        M_seen_vel = M_vel;
        M_seen_vel_count = 0;
#ifdef DEBUG_PRINT
        dlog.addText( Logger::WORLD,
                      __FILE__" (updateBySee) unum=%d. update vel by pos diff."
                              "prev_pos=(%.2f, %.2f) old_pos=(%.2f, %.2f) -> vel=(%.3f, %.3f)",
                      p.unum_,
                      M_pos.x, M_pos.y, p.pos_.x, p.pos_.y, M_vel.x, M_vel.y );
#endif
    }
    else
    {
        M_vel.assign( 0.0, 0.0 );
        M_vel_count = 1000;
    }

    M_pos = p.pos_;
    M_rpos = p.rpos_;
    M_seen_pos = p.pos_;

    M_pos_count = 0;
    M_rpos_count = 0;
    M_seen_pos_count = 0;

    if ( p.hasAngle() )
    {
        M_body = p.body_;
        M_face = p.face_;
        M_body_count = M_face_count = 0;
    }
    else if ( last_seen_pos_count <= 2
              && last_seen_move.r2() > std::pow( 0.2, 2 ) ) // Magic Number
    {
        M_body = last_seen_move.th();
        M_body_count = std::max( 0, last_seen_pos_count - 1 );
        M_face = 0.0;
        M_face_count = 1000;
    }
    else if ( velValid()
              && vel().r2() > std::pow( 0.2, 2 ) ) // Magic Number
    {
        M_body = vel().th();
        M_body_count = velCount();
        M_face = 0.0;
        M_face_count = 1000;
    }

    if ( p.isPointing()
         && M_pointto_count >= ServerParam::i().pointToBan() )
    {
        M_pointto_angle = p.arm_;
        M_pointto_count = 0;
        M_seen_stamina_count = 0;
        StaminaWithPointto::setStaminaFromVisionData(M_seen_stamina, M_pointto_angle.degree());
    }

    M_kicked = p.kicked();

    if ( p.isTackling() )
    {
        if ( M_tackle_count > ServerParam::i().tackleCycles() ) // no tackling recently
        {
            M_tackle_count = 0;
        }
    }
    else
    {
        M_tackle_count = 1000;
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
PlayerObject::updateByFullstate( const FullstateSensor::PlayerT & p,
                                 const Vector2D & self_pos,
                                 const Vector2D & ball_pos )
{
    M_side = p.side_;
    M_unum = p.unum_;
    M_unum_count = 0;
    M_goalie = p.goalie_;

    M_pos = p.pos_;
    M_pos_count = 0;

    M_seen_pos = p.pos_;
    M_seen_pos_count = 0;

    M_vel = p.vel_;
    M_vel_count = 0;
    M_seen_vel = p.vel_;
    M_seen_vel_count = 0;

    M_body = p.body_;
    M_body_count = 0;
    M_face = p.body_ + p.neck_;
    M_face_count = 0;

    M_dist_from_ball = ( M_pos - ball_pos ).r();
    M_angle_from_ball = ( M_pos - ball_pos ).th();

    M_dist_from_self = M_rpos.r();
    M_angle_from_self = M_rpos.r();

    M_ghost_count = 0;

    M_rpos = p.pos_ - self_pos;
    M_rpos_count = 0;

    M_pointto_angle = M_face + p.pointto_dir_;
    M_pointto_count = 0;

    M_kicked = p.kicked_;

    if ( p.tackle_ )
    {
        if ( M_tackle_count > ServerParam::i().tackleCycles() ) // no tackling recently
        {
            M_tackle_count = 0;
        }
    }
    else
    {
        M_tackle_count = 1000;
    }
    M_seen_stamina = p.stamina_;
    M_seen_stamina_count = 0;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
PlayerObject::updateByHearPosCount( const SideID heard_side,
                                    const int heard_unum,
                                    const bool goalie,
                                    const Vector2D & heard_pos,
                                    int poscount,
                                    int sender,
                                    bool ourside)
{
    M_heard_pos = heard_pos;
    M_heard_pos_count = 0;

    M_ghost_count = 0;

    if ( heard_side != NEUTRAL )
    {
        M_side = heard_side;
    }

    if ( heard_unum != Unum_Unknown
         && unumCount() > 0 )
    {
        M_unum = heard_unum;
    }

    if ( goalie )
    {
        M_goalie = true;
    }

    if(ourside && sender == heard_unum){
        M_pos = heard_pos;
        M_pos_count = 0;
    }
    else if ( posCount() > poscount
              || (distFromSelf() > 20.0 && posCount() >= poscount) )
    {
        M_pos = heard_pos;
        M_pos_count = poscount + 1;
    }

    dlog.addCircle(Logger::WORLD,M_heard_pos,0.6,5,5,200,false);
}

/*-------------------------------------------------------------------*/
/*!

*/
void
PlayerObject::updateByHearPosCount( const SideID heard_side,
                                    const int heard_unum,
                                    const bool goalie,
                                    const Vector2D & heard_pos,
                                    const double & heard_body,
                                    const double & heard_stamina,
                                    int posCount,
                                    int sender,
                                    bool ourside)
{
    updateByHearPosCount( heard_side, heard_unum, goalie, heard_pos,posCount,sender, ourside );

    if ( heard_body != -360.0 )
    {
        if ( ourside && sender == heard_unum){
            M_body = heard_body;
            M_body_count = 0;
        }
        else if ( bodyCount() >= 2 )
        {
            M_body = heard_body;
            M_body_count = 1;
        }
    }

    if ( heard_stamina > 0.0 )
    {
        M_heard_stamina = heard_stamina;
    }
}

void
PlayerObject::updateByHear( const SideID heard_side,
                            const int heard_unum,
                            const bool goalie,
                            const Vector2D & heard_pos,
                            const bool ourside,
                            const int pos_count,
                            const int sender)
{
    M_heard_pos = heard_pos;
    M_heard_pos_count = pos_count;

    M_ghost_count = 0;

    if ( heard_side != NEUTRAL )
    {
        M_side = heard_side;
    }

    if ( heard_unum != Unum_Unknown
         && unumCount() > 0 )
    {
        M_unum = heard_unum;
        //M_unum_count = 1;
    }

    if ( goalie )
    {
        M_goalie = true;
    }

    if ( pos_count < M_pos_count){
        M_pos_count = pos_count;
        M_pos = heard_pos;
    }else if(sender == heard_unum && ourside){
        M_pos_count = pos_count;
        M_pos = heard_pos;
    }
    //    if ( posCount() >= 2
    //         || distFromSelf() > 20.0 )
    //    {
    //        M_pos = heard_pos;

    //        if ( posCount() > 1 )
    //        {
    //            M_pos_count = 1;
    //        }
    //    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
PlayerObject::updateByHear( const SideID heard_side,
                            const int heard_unum,
                            const bool goalie,
                            const Vector2D & heard_pos,
                            const double & heard_body,
                            const double & heard_stamina,
                            const bool ourside,
                            const int poscount,
                            const int sender)
{
    updateByHear( heard_side, heard_unum, goalie, heard_pos, ourside,poscount,sender );

    if ( heard_body != -360.0 )
    {
        if ( bodyCount() >= 2 )
        {
            M_body = heard_body;
            M_body_count = 1;
        }
    }

    if ( heard_stamina > 0.0 )
    {
        M_heard_stamina = heard_stamina;
    }
}

void
PlayerObject::updateByHearSender( const SideID heard_side,
                                  const int heard_unum,
                                  const bool goalie,
                                  const Vector2D & heard_pos,
                                  const int sender,
                                  const int pos_count,
                                  const bool ourside,
                                  const double dist2sender)
{
    M_heard_pos = heard_pos;
    M_heard_pos_count = pos_count;

    M_ghost_count = 0;

    if ( heard_side != NEUTRAL )
    {
        M_side = heard_side;
    }

    if ( heard_unum != Unum_Unknown
         && unumCount() > 0 )
    {
        M_unum = heard_unum;
        M_unum_count = 0;
    }

    if ( goalie )
    {
        M_goalie = true;
    }

    if ( pos_count < M_pos_count){
        M_pos_count = pos_count;
        M_pos = heard_pos;
    }else if(sender == heard_unum && ourside){
        M_pos_count = pos_count;
        M_pos = heard_pos;
    }else{
        double dist2self = distFromSelf();
        if(pos_count == M_pos_count){
            if(dist2sender < dist2self+20){
                M_pos_count = pos_count;
                M_pos = heard_pos;
            }
        }
    }
}

void
PlayerObject::updateByHearSender( const SideID heard_side,
                                  const int heard_unum,
                                  const bool goalie,
                                  const Vector2D & heard_pos,
                                  const double & heard_body,
                                  const double & heard_stamina,
                                  const int sender,
                                  const bool ourside,
                                  const int pos_count,
                                  const double dist2sender)
{
//    if (ourside)
        updateByHearSender( heard_side, heard_unum, goalie, heard_pos, sender, pos_count, ourside, dist2sender );
//    else
//        updateByHear( heard_side, heard_unum, goalie, heard_pos, pos_count, ourside, sender);

    if ( heard_body != -360.0 )
    {
        if (ourside && heard_unum == sender){
            M_body = heard_body;
            M_body_count = 0;
        }
        else if ( bodyCount() >= 2 && bodyCount() < pos_count)
        {
            M_body = heard_body;
            M_body_count = 1;
        }
    }

    if ( heard_stamina > 0.0 )
    {
        M_heard_stamina = heard_stamina;
    }
}
/*-------------------------------------------------------------------*/
/*!

*/
void
PlayerObject::updateSelfBallRelated( const Vector2D & self,
                                     const Vector2D & ball )
{
    M_dist_from_ball = ( M_pos - ball ).r();
    M_angle_from_ball = ( M_pos - ball ).th();
    M_dist_from_self = ( M_pos - self ).r();
    M_angle_from_self = ( M_pos - self ).th();

    if ( M_rpos_count > 0 )
    {
        M_rpos = M_pos - self;
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
PlayerObject::forget()
{
    M_pos_count
            = M_seen_pos_count
            = M_heard_pos_count
            = M_vel_count
            = M_seen_vel_count
            = M_face_count
            = M_pointto_count
            = M_seen_stamina_count
            = M_tackle_count
            = 1000;
}

}
