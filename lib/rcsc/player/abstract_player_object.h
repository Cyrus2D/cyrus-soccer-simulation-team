// -*-c++-*-

/*!
 \file abstract_player_object.h
 \brief abstract player object class Header File
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
#ifndef RCSC_PLAYER_ABSTRACT_PLAYER_OBJECT_H
#define RCSC_PLAYER_ABSTRACT_PLAYER_OBJECT_H

//#include <rcsc/player/world_model.h>
#include <rcsc/player/localization.h>
#include <rcsc/common/player_type.h>
#include <rcsc/geom/vector_2d.h>
#include <rcsc/geom/angle_deg.h>
#include <rcsc/types.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/circle_2d.h>
#include <vector>
#include <rcsc/common/logger.h>

namespace rcsc {

class AbstractPlayerObject;
class PlayerEvaluator;
class WorldModel;

typedef std::vector<const AbstractPlayerObject *> AbstractPlayerCont;

/*!
 \class AbstractPlayerObject
 \brief abstact player object class
 */
class AbstractPlayerObject {
protected:

    SideID M_side; //!< team side
    int M_player_unique_code;
    int M_unum; //!< uniform number
    int M_unum_count; //!< accuracy count
    bool M_goalie; //!< goalie flag

    int M_type; //!< player type id
    const PlayerType * M_player_type; //!< player type reference
    Card M_card; //!< card information

    Vector2D M_pos; //!< global coordinate
    int M_pos_count; //!< main accuracy counter
    Vector2D M_predict_pos;

    Vector2D M_seen_pos; //!< last seen global coordinate
    int M_seen_pos_count; //!< count since last observation

    Vector2D M_heard_pos; //!< last heard global coordinate
    int M_heard_pos_count; //!< count since last observation

    Vector2D M_vel; //!< velocity
    int M_vel_count; //!< accuracy count

    Vector2D M_seen_vel; //!< last seen velocity
    int M_seen_vel_count; //!< count since last observation

    AngleDeg M_body; //!< global body angle
    int M_body_count; //!< body angle accuracy
    AngleDeg M_face; //!< global neck angle
    int M_face_count; //!< face angle accuracy

    bool M_kicked; //!< kicking state

    double M_dist_from_ball; //!< distance from ball
    AngleDeg M_angle_from_ball; //!< angle from ball
    double M_dist_from_self; //!< distance from self
    AngleDeg M_angle_from_self; //!< angle from self

    double M_seen_stamina; //!< seen stamina
    int M_seen_stamina_count; //!< seen stamina count
public:

    /*!
         \brief initialize member variables.
         */
    AbstractPlayerObject();

    /*!
         \brief initialize member variables using observed info
         \param side analyzed side info
         \param p analyzed seen player info
         */
    AbstractPlayerObject(const SideID side, const Localization::PlayerT & p);

    /*!
         \brief destructor. nothing to do
         */
    virtual ~AbstractPlayerObject() {
    }

    // ------------------------------------------
    /*!
         \brief check if this player is self or not
         \return true if this player is self
         */
    virtual
    bool isSelf() const {
        return false;
    }

    /*!
         \brief check if this player is ghost object or not
         \return true if this player may be ghost object
         */
    virtual
    bool isGhost() const {
        return false;
    }

    /*!
         \brief get the counter value as a ghost recognition
         \return count as a ghost recognition
         */
    virtual
    int ghostCount() const {
        return 0;
    }

    /*!
         \brief check if player is tackling or not
         \return checked result
         */
    virtual
    bool isTackling() const = 0;

    /*!
         \brief update player type id
         \param type new player type id
         */
    virtual
    void setPlayerType(const int type);

    /*!
         \brief update card state
         \param card new card type
         */
    void setCard(const Card card) {
        M_card = card;
    }

    // ------------------------------------------

    /*!
         \brief get team side id
         \return side id (LEFT,RIGHT,NEUTRAL)
         */
    SideID side() const {
        return M_side;
    }

    /*!
         \brief get player's uniform number
         \return uniform number. if unknown player, returned -1
         */
    int unum() const {
        return M_unum;
    }

    int player_unique_code() const {
        return M_player_unique_code;
    }
    /*!
         \brief get uniform number accuracy count
         \return count since last observation
         */
    int unumCount() const {
        return M_unum_count;
    }

    /*!
         \brief get goalie flag
         \return true if this player is goalie
         */
    bool goalie() const {
        return M_goalie;
    }

    /*!
         \brief get the player type id
         \return player type id
         */
    int type() const {
        return M_type;
    }

    /*!
         \brief get the player type as a pointer.
         \return player type pointer variable
         */
    const PlayerType * playerTypePtr() const {
        return M_player_type;
    }

    /*!
         \brief get card type
         \return card type{NO_CARD,YELLOW,RED}
         */
    Card card() const {
        return M_card;
    }

    /*!
         \brief get global position
         \return const reference to the point object
         */
    const Vector2D & pos() const {
        return M_pos;
    }
    const Vector2D & predict_pos() const {
        return M_predict_pos;
    }
    /*!
         \brief get global position accuracy
         \return count since last observation
         */
    int posCount() const {
        return M_pos_count;
    }

    /*!
         \brief get the last seen position
         \return const reference to the point object
         */
    const Vector2D & seenPos() const {
        return M_seen_pos;
    }

    /*!
         \brief get the number of cycles since last observation
         \return count since last seen
         */
    int seenPosCount() const {
        return M_seen_pos_count;
    }

    /*!
         \brief get the last heard position
         \return const reference to the point object
         */
    const Vector2D & heardPos() const {
        return M_heard_pos;
    }

    /*!
         \brief get the number of cycles since last observation
         \return count since last observation
         */
    int heardPosCount() const {
        return M_heard_pos_count;
    }

    /*!
         \brief get velocity
         \return const reference to the vector object
         */
    const Vector2D & vel() const {
        return M_vel;
    }

    /*!
         \brief get velocity accuracy
         \return count from last observation
         */
    int velCount() const {
        return M_vel_count;
    }

    /*!
         \brief get the last seen velocity
         \return const reference to the vector object
         */
    const Vector2D & seenVel() const {
        return M_seen_vel;
    }

    /*!
         \brief get the number of cycles since last observation
         \return count since last seen
         */
    int seenVelCount() const {
        return M_seen_vel_count;
    }

    /*!
         \brief get global body angle
         \return const reference to the angle object
         */
    const AngleDeg & body() const {
        return M_body; // global body angle
    }

    /*!
         \brief get global body angle accuracy
         \return count from last observation
         */
    int bodyCount() const {
        return M_body_count;
    }

    /*!
         \brief get global neck angle
         \return const reference to the angle object
         */
    const AngleDeg & face() const {
        return M_face; // global neck angle
    }

    /*!
         \brief get global neck angle accuracy
         \return count from last observation
         */
    int faceCount() const {
        return M_face_count;
    }

    /*!
         \brief get kicking state information
         \return true if player performed the kick.
         */
    bool kicked() const {
        return M_kicked;
    }

    /*!
         \brief get distance from ball
         \return distance value from ball
         */
    const
    double & distFromBall() const {
        return M_dist_from_ball;
    }

    /*!
         \brief get angle from ball
         \return absolute angle value from ball
         */
    const AngleDeg & angleFromBall() const {
        return M_angle_from_ball;
    }

    /*!
         \brief get distance from self
         \return distance value from self
         */
    const
    double & distFromSelf() const {
        return M_dist_from_self;
    }

    /*!
         \brief get global angle from self position
         \return angle value from self position
         */
    const AngleDeg & angleFromSelf() const {
        return M_angle_from_self;
    }

    /*!
         \brief get current estimated kick power rate
         \return calculated kick rate value
         */
    virtual
    double kickRate() const;

    /*!
         \brief estimate reach point
         \param n_step this method estimates ball point after this steps
         \return estimated point vector
         */
    Vector2D inertiaPoint(const int n_step) const {
        return (playerTypePtr() ?
                    playerTypePtr()->inertiaPoint(pos(), vel(), n_step) :
                    Vector2D::INVALIDATED);
    }

    /*!
         \brief estimate final reach point
         \return estimated point vector
         */
    Vector2D inertiaFinalPoint() const {
        return (playerTypePtr() ?
                    playerTypePtr()->inertiaFinalPoint(pos(), vel()) :
                    Vector2D::INVALIDATED);
    }

    /*!
      \brief get the seen stamina inf
      \return seen stamina value
     */
    const
    double & seenStamina() const
      {
          return M_seen_stamina;
      }

    /*!
      \brief get the seen stamina inf
      \return seen stamina value
     */
    const
    int & seenStaminaCount() const
      {
          return M_seen_stamina_count;
      }
    // ------------------------------------------
    /*!
         \brief template method. check if player is in the region
         \param region template resion. REGION must have method contains()
         \return true if region contains player position
         */
    template<typename REGION>
    bool isWithin(const REGION & region) const {
        return region.contains(this->pos());
    }

    /*!
         \brief get minimum evaluation value within the input container using evaluator
         \param cont container of AbstractPlayerObject
         \param evaluator evaluator object (has to be dynamically allocated)
         */
    static double get_minimum_evaluation(const AbstractPlayerCont & cont,
                                         const PlayerEvaluator * evaluator);

    /*!
         \brief get maximum evaluation value within the input container using evaluator
         \param cont container of AbstractPlayerObject
         \param evaluator evaluator object (has to be dynamically allocated)
         */
    static double get_maximum_evaluation(const AbstractPlayerCont & cont,
                                         const PlayerEvaluator * evaluator);

    int cycles_to_cut_ball_with_safe_thr_dist(const WorldModel & wm, const Vector2D & ball_pos,
                           const int max_cycle,
                           bool check_tackle,
                           int & dash_cycle,
                           int & turn_cycle,
                           int & view_cycle,
                           Vector2D predict_pos = Vector2D::INVALIDATED,
                           Vector2D predict_vel = Vector2D::INVALIDATED,
                           double safe_dist_thr = 0,
                           AngleDeg predict_body = AngleDeg::INVALIDATED,
                           bool check_omni=false) const {

        //    	update_opps_pos(wm);
        if(!predict_pos.isValid())
            predict_pos = pos();
        if(!predict_vel.isValid())
            predict_vel = vel();
        if(!predict_body.isValid())
            predict_body = body();
        int reach_cycle_kick = 1000;
        int reach_cycle_tackle = 1000;
        int reach_cycle_omni = 1000;

        int direct_kick_dash_cycle = 100;
        int direct_tackle_dash_cycle = 100;
        int omni_kick_dash_cycle = 100;

        int direct_kick_turn_cycle = 90;
        int direct_tackle_turn_cycle = 90;
        int omni_kick_turn_cycle = 90;

        int direct_kick_view_cycle = 0;
        int direct_tackle_view_cycle = 0;
        int omni_kick_view_cycle = 0;

        reach_cycle_kick = cycles_to_cut_ball_direct_kick(wm, ball_pos,
                                                          max_cycle, direct_kick_dash_cycle, direct_kick_turn_cycle,direct_kick_view_cycle,predict_pos,predict_vel,predict_body,safe_dist_thr);
        if (check_tackle)
            reach_cycle_tackle = cycles_to_cut_ball_direct_tackle(wm, ball_pos,
                                                                  max_cycle, direct_tackle_dash_cycle, direct_tackle_turn_cycle,direct_tackle_view_cycle,predict_pos,predict_vel,predict_body);
        if (max_cycle <= 5 && check_omni)
            reach_cycle_omni = cycles_to_cut_ball_omni_kick(wm, ball_pos,
                                                              max_cycle, omni_kick_dash_cycle, omni_kick_turn_cycle,omni_kick_view_cycle,predict_pos,predict_vel,predict_body,safe_dist_thr);
//        }
        //    		if(opp_type[opponent->unum()]!=-1){
        //    			reach_cycle_omni = predictOpponentReachStep_omni(wm,opponent,ball_pos,cycle,opp_min_dif_cycle);
        //    		}
        //		    	}

//        dlog.addText(Logger::DRIBBLE,"ck:%d %d %d,ct:%d %d %d",reach_cycle_kick,direct_kick_dash_cycle,direct_kick_turn_cycle,reach_cycle_tackle,direct_tackle_dash_cycle,direct_tackle_turn_cycle);
        int opp_reach_cycle = reach_cycle_kick;
        dash_cycle = direct_kick_dash_cycle;
        turn_cycle = direct_kick_turn_cycle;
        view_cycle = direct_kick_view_cycle;
        if (reach_cycle_omni < opp_reach_cycle && check_omni){
            opp_reach_cycle = reach_cycle_omni;
            dash_cycle = omni_kick_dash_cycle;
            turn_cycle = omni_kick_turn_cycle;
            view_cycle = omni_kick_view_cycle;
        }
        if( reach_cycle_tackle < opp_reach_cycle && check_tackle){
            opp_reach_cycle = reach_cycle_tackle+1;
            dash_cycle = direct_tackle_dash_cycle+1;
            turn_cycle = direct_tackle_turn_cycle;
            view_cycle = direct_tackle_view_cycle;
        }
        return opp_reach_cycle;
    }
    int cycles_to_cut_ball(const WorldModel & wm, const Vector2D & ball_pos,
                           const int max_cycle,
                           bool check_tackle,
                           int & dash_cycle,
                           int & turn_cycle,
                           int & view_cycle,
                           Vector2D predict_pos = Vector2D::INVALIDATED,
                           Vector2D predict_vel = Vector2D::INVALIDATED,
                           AngleDeg predict_body = AngleDeg::INVALIDATED,
                           bool check_omni=false) const {

        //    	update_opps_pos(wm);
        if(!predict_pos.isValid())
            predict_pos = pos();
        if(!predict_vel.isValid())
            predict_vel = vel();
        if(!predict_body.isValid())
            predict_body = body();
        int reach_cycle_kick = 1000;
        int reach_cycle_tackle = 1000;
        int reach_cycle_omni = 1000;

        int direct_kick_dash_cycle = 100;
        int direct_tackle_dash_cycle = 100;
        int omni_kick_dash_cycle = 100;

        int direct_kick_turn_cycle = 90;
        int direct_tackle_turn_cycle = 90;
        int omni_kick_turn_cycle = 90;

        int direct_kick_view_cycle = 0;
        int direct_tackle_view_cycle = 0;
        int omni_kick_view_cycle = 0;

        reach_cycle_kick = cycles_to_cut_ball_direct_kick(wm, ball_pos,
                                                          max_cycle, direct_kick_dash_cycle, direct_kick_turn_cycle,direct_kick_view_cycle,predict_pos,predict_vel,predict_body,0);
        if (check_tackle)
            reach_cycle_tackle = cycles_to_cut_ball_direct_tackle(wm, ball_pos,
                                                                  max_cycle, direct_tackle_dash_cycle, direct_tackle_turn_cycle,direct_tackle_view_cycle,predict_pos,predict_vel,predict_body);
        //    		if(opp_type[opponent->unum()]!=-1){
        if (max_cycle <= 5 && check_omni)
            reach_cycle_omni = cycles_to_cut_ball_omni_kick(wm, ball_pos,
                                                            max_cycle, omni_kick_dash_cycle, omni_kick_turn_cycle,
                                                            omni_kick_view_cycle, predict_pos, predict_vel,
                                                            predict_body, 0);
//        }
//        dlog.addText(Logger::DRIBBLE,"ck:%d %d %d,ct:%d %d %d",reach_cycle_kick,direct_kick_dash_cycle,direct_kick_turn_cycle,reach_cycle_tackle,direct_tackle_dash_cycle,direct_tackle_turn_cycle);
        int opp_reach_cycle = reach_cycle_kick;
        dash_cycle = direct_kick_dash_cycle;
        turn_cycle = direct_kick_turn_cycle;
        view_cycle = direct_kick_view_cycle;
        if (reach_cycle_omni < opp_reach_cycle && check_omni){
            opp_reach_cycle = reach_cycle_omni;
            dash_cycle = omni_kick_dash_cycle;
            turn_cycle = omni_kick_turn_cycle;
            view_cycle = omni_kick_view_cycle;
        }
        if( reach_cycle_tackle < opp_reach_cycle && check_tackle){
            opp_reach_cycle = reach_cycle_tackle+1;
            dash_cycle = direct_tackle_dash_cycle+1;
            turn_cycle = direct_tackle_turn_cycle;
            view_cycle = direct_tackle_view_cycle;
        }
        return opp_reach_cycle;
    }
    int cycles_to_cut_ball_direct_kick(const WorldModel & wm,
                                       const Vector2D & ball_pos,
                                       const int cycle,
                                       int & dash_cycle,
                                       int & turn_cycle,
                                       int & view_cycle,
                                       Vector2D predict_pos,
                                       Vector2D predict_vel,
                                       AngleDeg predict_body,
                                       double safe_dist_thr) const {
        view_cycle = 0;
        static const Rect2D penalty_area(
                    Vector2D(ServerParam::i().theirPenaltyAreaLineX(),
                             -ServerParam::i().penaltyAreaHalfWidth()),
                    Size2D(ServerParam::i().penaltyAreaLength(),
                           ServerParam::i().penaltyAreaWidth()));
        static const double CONTROL_AREA_BUF = 0.1;
        const ServerParam & SP = ServerParam::i();

        const PlayerType * ptype = playerTypePtr();

        double control_area = (
                    goalie() && penalty_area.contains(ball_pos) ?
                        SP.catchableArea() : ptype->kickableArea());
        Vector2D opp_pos = predict_pos;
        Vector2D iner_pos = ptype->inertiaPoint(opp_pos,predict_vel,cycle);

        double target_dist = opp_pos.dist(ball_pos);
        double dash_dist = iner_pos.dist(ball_pos) - safe_dist_thr;

        bool back_dash = false;
        double next_body_angle = predict_body.degree();
        int n_turn = predict_player_turn_cycle_direct_kick(ptype, predict_body,
                                                           predict_vel.r(), target_dist, (ball_pos - opp_pos).th(),
                                                           control_area, predict_pos, predict_vel, ball_pos, back_dash,
                                                           next_body_angle);
        if (target_dist - control_area
                > ptype->realSpeedMax() * (double) (cycle + posCount())) {
            turn_cycle = n_turn;
            dash_cycle = ptype->cyclesToReachDistance(target_dist);
//            dlog.addText(Logger::PASS, "ctcb: return A");
            return std::max(cycle + 1, dash_cycle + turn_cycle + 1);
        }
        if (target_dist - control_area < 0.001) {
            dash_cycle = 0;
            turn_cycle = 0;
            return 0;
        }

        int reach_step_with_out_action = 1000;
        for (int c = 1; c <= cycle; c++) {
            Vector2D c_inertia_pos = ptype->inertiaPoint(predict_pos, predict_vel, c);
            double dash_dist = c_inertia_pos.dist(ball_pos);
            if (dash_dist - control_area - CONTROL_AREA_BUF < 0.001) {
                reach_step_with_out_action = c;
//                dlog.addText(Logger::PASS, "ctcb: wotjout %d", reach_step_with_out_action);
                break;
            }
        }



        Line2D player_move_line = Line2D(opp_pos, next_body_angle);
        Circle2D intercept_circle = Circle2D(ball_pos, control_area);
        Vector2D intercept_1 = Vector2D::INVALIDATED;
        Vector2D intercept_2 = Vector2D::INVALIDATED;
        Vector2D best_intercept;
        int intercept_number = intercept_circle.intersection(player_move_line,
                                                             &intercept_1, &intercept_2);
        if (intercept_number == 0) {
            best_intercept = ball_pos;
        } else if (intercept_number == 1) {
            best_intercept = intercept_1;
        } else {
            if (intercept_1.dist(predict_pos) < intercept_2.dist(predict_pos)) {
                best_intercept = intercept_1;
            } else {
                best_intercept = intercept_2;
            }
        }
        //	cout<<"   predictOpponentReachStep_direct_kick5"<<endl;
        dash_dist = iner_pos.dist(best_intercept) - safe_dist_thr;


        int n_dash = ptype->cyclesToReachDistance(dash_dist);

        if (n_turn > 0) {
            view_cycle = 1;
        }
        if (vel().r() < 0.15)
            view_cycle = 1;
        else {		//63540798
            Vector2D opp_vel = predict_vel;
            opp_vel.rotate((opp_pos - ball_pos).th());
            if (opp_vel.x < 0.1)
                view_cycle = 1;
        }
        if (n_dash == 0)
            view_cycle = 0;
//        dlog.addText(Logger::PASS, "bestinter %.1f,%.1f, pp %.1f %.f, pv %.1f,%.1f, ip %.1f, %.1f, dd %.1f, ndash %d, vc %d, nt %d, nd %d"
//                     , best_intercept.x, best_intercept.y
//                     ,opp_pos.x, opp_pos.y
//                     ,predict_vel.x, predict_vel.y
//                     ,iner_pos.x, iner_pos.y
//                     , dash_dist, n_dash, view_cycle, n_turn, n_dash);
        int n_step = n_turn + n_dash + view_cycle;

        if(n_step < reach_step_with_out_action){
            dash_cycle = n_dash;
            turn_cycle = n_turn;
            return n_step;
        }else{
            dash_cycle = reach_step_with_out_action;
            turn_cycle = 0;
            view_cycle = 0;
            return reach_step_with_out_action;
        }
    }
    int predict_player_turn_cycle_direct_kick(const rcsc::PlayerType * ptype,
                                              const rcsc::AngleDeg & player_body,
                                              const double & player_speed,
                                              const double & target_dist,
                                              const rcsc::AngleDeg & target_angle,
                                              const double & dist_thr,
                                              const rcsc::Vector2D player_pos,
                                              const rcsc::Vector2D player_vel,
                                              const rcsc::Vector2D target_pos,
                                              bool & use_back_dash,
                                              double & next_body_angle) const {
        const ServerParam & SP = ServerParam::i();

        int n_turn = 0;

        double angle_diff = (target_angle - player_body).abs();
//        dlog.addText(Logger::DRIBBLE,"ta:%.1f,pb:%.1f,ad:%.1f",target_angle.degree(),player_body.degree(),angle_diff);
        if (angle_diff > 180){
            angle_diff = 360 - angle_diff;
        }
        if (use_back_dash && target_dist < 5.0 // Magic Number
                && angle_diff > 90.0 && SP.minDashPower() < -SP.maxDashPower() + 1.0) {
            angle_diff = std::fabs(angle_diff - 180.0);  // assume backward dash
            if(angle_diff > 180)
                angle_diff = 360 - angle_diff;
            use_back_dash = true;
        } else{
            if(angle_diff > 180)
                angle_diff = 360 - angle_diff;
            use_back_dash = false;
        }
//        dlog.addText(Logger::DRIBBLE,"ta:%.1f,pb:%.1f,ad:%.1f",target_angle.degree(),player_body.degree(),angle_diff);
        double speed = player_speed;
        Vector2D next_pos = player_pos;
        Vector2D next_vel = player_vel;
//        dlog.addText(Logger::DRIBBLE,"%.1f,%.1f",angle_diff,Line2D(next_pos, next_body_angle).dist(target_pos));
        while (Line2D(next_pos, next_body_angle).dist(target_pos) > dist_thr + 0.15) {
//            dlog.addText(Logger::DRIBBLE,"%.1f,%.1f",angle_diff,Line2D(next_pos, next_body_angle).dist(target_pos));

            next_pos += next_vel;
            next_vel *= ptype->playerDecay();
            angle_diff -= ptype->effectiveTurn(SP.maxMoment(), speed);
            speed *= ptype->playerDecay();
            ++n_turn;
            if (angle_diff <= 0) {
                next_body_angle = target_angle.degree();
                break;
            } else {
                next_body_angle = target_angle.degree() + angle_diff;
            }
        }

        return n_turn;
    }
    int cycles_to_cut_ball_direct_tackle(const WorldModel & wm,
                                         const Vector2D & ball_pos,
                                         const int cycle,
                                         int & dash_cycle,
                                         int & turn_cycle,
                                         int & view_cycle,
                                         Vector2D predict_pos,
                                         Vector2D predict_vel,
                                         AngleDeg predict_body) const {
        view_cycle = 0;
        static const Rect2D penalty_area(
                    Vector2D(ServerParam::i().theirPenaltyAreaLineX(),
                             -ServerParam::i().penaltyAreaHalfWidth()),
                    Size2D(ServerParam::i().penaltyAreaLength(),
                           ServerParam::i().penaltyAreaWidth()));

        const ServerParam & SP = ServerParam::i();

        const PlayerType * ptype = playerTypePtr();

        double control_area = (
                    goalie() && penalty_area.contains(ball_pos) ?
                        SP.catchableArea() : ptype->kickableArea());
        Vector2D opp_pos = predict_pos;
        const Vector2D inertia_pos = ptype->inertiaPoint(opp_pos,predict_vel,cycle);
        const double target_dist = inertia_pos.dist(ball_pos);

        if (target_dist
                > ptype->realSpeedMax() * (double) (cycle + posCount())
                + SP.tackleDist()) {
//            dlog.addText(Logger::DRIBBLE,"pos(%.2f,%.2f),tardist:%.2f, high dist",opp_pos.x,opp_pos.y,target_dist);
            turn_cycle = 3;
            dash_cycle = ptype->cyclesToReachDistance(target_dist) + 2;
            return dash_cycle + turn_cycle;
        }

        if (target_dist - control_area < 0.001) {
//            dlog.addText(Logger::DRIBBLE,"opppos(%.2f,%.2f),tardist:%.2f, low dist",opp_pos.x,opp_pos.y,target_dist);
            dash_cycle = 0;
            turn_cycle = 0;
            return 0;
        }

        double dash_dist = target_dist;

        bool back_dash = false;

        bool should_turn = true;
        Line2D opp_body_line = Line2D(inertia_pos,predict_body);
        if(opp_body_line.dist(ball_pos) < 1){
            double dif = ((ball_pos-inertia_pos).th() - AngleDeg(predict_body)).abs();
            if(dif > 180)
                dif = 360 - dif;
            if(dif < 90)
                should_turn = false;
        }
        double opp_ball_angle = (ball_pos - inertia_pos).th().degree();
        Vector2D tmp1 = inertia_pos
                + Vector2D::polar2vector(ptype->kickableArea(),
                                         opp_ball_angle + 90);
        Vector2D tmp2 = inertia_pos
                + Vector2D::polar2vector(ptype->kickableArea(),
                                         opp_ball_angle - 90);
        double tmp1_angle = (ball_pos - tmp1).th().degree();
        double tmp1_angle_dif = tmp1_angle - body().degree();
        if (tmp1_angle_dif < 0)
            tmp1_angle_dif *= (-1.0);
        if (tmp1_angle_dif > 180)
            tmp1_angle_dif = 360 - tmp1_angle_dif;
        double tmp2_angle = (ball_pos - tmp2).th().degree();
        double tmp2_angle_dif = tmp2_angle - body().degree();
        if (tmp2_angle_dif < 0)
            tmp2_angle_dif *= (-1.0);
        if (tmp2_angle_dif > 180)
            tmp2_angle_dif = 360 - tmp2_angle_dif;

        double body_target;
        if (tmp1_angle_dif < tmp2_angle_dif)
            body_target = tmp1_angle;
        else
            body_target = tmp2_angle;
        double next_body_angle = body().degree();
        int n_turn = (should_turn?predict_player_turn_cycle_direct_tackle(ptype, predict_body,
                                                                          predict_vel.r(), target_dist, body_target, control_area, opp_pos,
                                                                          predict_vel, ball_pos, back_dash, next_body_angle):0);

//        dlog.addText(Logger::DRIBBLE,"opppos(%.2f,%.2f),tardist:%.2f, targangle:%.2f, nturn:%d",opp_pos.x,opp_pos.y,target_dist,body_target,n_turn);
        Line2D player_move_line = Line2D(inertia_pos, next_body_angle);
        Vector2D ball_on_line = player_move_line.projection(ball_pos);
        dash_dist = inertia_pos.dist(ball_on_line)
                - ServerParam::i().tackleDist();

        int n_dash = ptype->cyclesToReachDistance(dash_dist);
//        dlog.addText(Logger::DRIBBLE,"opppos(%.2f,%.2f),tardist:%.2f, targangle:%.2f, nturn:%d, dash_dist:%.2f,ndash:%d",opp_pos.x,opp_pos.y,target_dist,body_target,n_turn,dash_dist,n_dash);
        view_cycle = 0;
        if (n_turn > 0) {
            view_cycle = 1;
        }
        if (vel().r() < 0.15)
            view_cycle = 1;
        else {    //63540798
            Vector2D opp_vel = vel();
            opp_vel.rotate((inertia_pos - ball_pos).th());
            if (opp_vel.x < 0.1)
                view_cycle = 1;
        }
        int n_step = n_turn + n_dash + view_cycle;

        dash_cycle = n_dash;
        turn_cycle = n_turn;
        return n_step;
    }

    int predict_player_turn_cycle_direct_tackle(const rcsc::PlayerType * ptype,
                                                const rcsc::AngleDeg & player_body, const double & player_speed,
                                                const double & target_dist, const rcsc::AngleDeg & target_angle,
                                                const double & dist_thr, const rcsc::Vector2D player_pos,
                                                const rcsc::Vector2D player_vel, const rcsc::Vector2D target_pos,
                                                bool & use_back_dash, double & next_body_angle) const {
        const ServerParam & SP = ServerParam::i();

        int n_turn = 0;

        double angle_diff = (target_angle - player_body).abs();
//        dlog.addText(Logger::DRIBBLE,"ta:%.1f,pb:%.1f,ad:%.1f",target_angle.degree(),player_body.degree(),angle_diff);
        if (angle_diff > 180){
            angle_diff = 360 - angle_diff;
        }

        double speed = player_speed;
        Vector2D next_pos = player_pos;
        Vector2D next_vel = player_vel;
//        dlog.addText(Logger::DRIBBLE,"%.1f,%.1f",angle_diff,Line2D(next_pos, next_body_angle).dist(target_pos));
        while (Line2D(next_pos, next_body_angle).dist(target_pos) > dist_thr) {
//            dlog.addText(Logger::DRIBBLE,"%.1f,%.1f",angle_diff,Line2D(next_pos, next_body_angle).dist(target_pos));

            next_pos += next_vel;
            next_vel *= ptype->playerDecay();
            angle_diff -= ptype->effectiveTurn(SP.maxMoment(), speed);
            speed *= ptype->playerDecay();
            ++n_turn;
            if (angle_diff <= 0) {
                next_body_angle = target_angle.degree();
                break;
            } else {
                next_body_angle = target_angle.degree() + angle_diff;
            }
        }

        return n_turn;

    }
    int cycles_to_cut_ball_omni_kick(const WorldModel & wm,
                                     const Vector2D & ball_pos,
                                     const int cycle,
                                     int & dash_cycle,
                                     int & turn_cycle,
                                     int & view_cycle,
                                     Vector2D predict_pos,
                                     Vector2D predict_vel,
                                     AngleDeg predict_body,
                                     double safe_dist_thr) const{
        static const Rect2D penalty_area(
                Vector2D(ServerParam::i().theirPenaltyAreaLineX(),
                         -ServerParam::i().penaltyAreaHalfWidth()),
                Size2D(ServerParam::i().penaltyAreaLength(),
                       ServerParam::i().penaltyAreaWidth()));

        const ServerParam & SP = ServerParam::i();

        const PlayerType * ptype = playerTypePtr();

        double control_area = (
                goalie() && penalty_area.contains(ball_pos) ?
                SP.catchableArea() : ptype->kickableArea());

        int final_step = 1000;
        for(double dir = -180; dir < 180; dir += 30.0){
            double dash_dir_deg = AngleDeg(dir).abs();
            dash_dir_deg /= 10.0;
            int dash_dir_step = static_cast<int>(std::round(dash_dir_deg));
            AngleDeg dash_angle = predict_body + AngleDeg(dir);
            for (int c = 1; c <= 2 && c <= cycle; c++){
                double move = ptype->dashDistanceTable()[dash_dir_step][c - 1];
                Vector2D opp_pos = predict_pos;
                Vector2D inertia_pos = ptype->inertiaPoint(opp_pos,predict_vel,c);
                inertia_pos += (Vector2D::polar2vector(move, dash_angle));
                if(inertia_pos.dist(ball_pos) < control_area){
                    int n_step = 1 + 0 + c;
                    if (c == 1){
                        view_cycle = 1;
                        turn_cycle = 0;
                        dash_cycle = c;
                        return n_step;
                    }
                    if (n_step < final_step){
                        view_cycle = 1;
                        turn_cycle = 0;
                        dash_cycle = c;
                        final_step = n_step;
                    }
                }
            }
        }
        return final_step;
    }
//    int cycles_to_cut_ball_omni_kick(const WorldModel & wm,
//                                       const Vector2D & ball_pos,
//                                       const int cycle,
//                                       int & dash_cycle,
//                                       int & turn_cycle,
//                                       int & view_cycle,
//                                       Vector2D predict_pos,
//                                       Vector2D predict_vel,
//                                       AngleDeg predict_body,
//                                       double safe_dist_thr) const{
//        static const Rect2D penalty_area(
//                Vector2D(ServerParam::i().theirPenaltyAreaLineX(),
//                         -ServerParam::i().penaltyAreaHalfWidth()),
//                Size2D(ServerParam::i().penaltyAreaLength(),
//                       ServerParam::i().penaltyAreaWidth()));
//
//        const ServerParam & SP = ServerParam::i();
//
//        const PlayerType * ptype = playerTypePtr();
//
//        double control_area = (
//                goalie() && penalty_area.contains(ball_pos) ?
//                SP.catchableArea() : ptype->kickableArea());
//
//        for(double dir = -180; dir < 180; dir+=45){
//            AngleDeg dash_angle = predict_body + AngleDeg(dir);
//            double dash_rate = ptype->dashPowerRate() * SP.dashDirRate( dir );
//            Vector2D accel = Vector2D::polar2vector(100 * dash_rate, dash_angle);
//            for (int c = 1; c <= 2 && c < cycle; c++){
//                Vector2D opp_pos = predict_pos;
//                Vector2D inertia_pos = ptype->inertiaPoint(opp_pos,predict_vel,c);
//                inertia_pos += (accel*c);
//                if(inertia_pos.dist(ball_pos) < control_area){
//                    view_cycle = 1;
//                    turn_cycle = 0;
//                    dash_cycle = c;
//                    return turn_cycle + dash_cycle + view_cycle;
//                }
//            }
//        }
//        return 1000;
//    }
};

}

#endif
