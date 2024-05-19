// -*-c++-*-

/*!
  \file body_intercept_plan
  \brief intercept the ball
*/


/////////////////////////////////////////////////////////////////////

#ifndef BODY_INTERCEPT_PLAN_H
#define BODY_INTERCEPT_PLAN_H

#include <rcsc/player/soccer_action.h>

/*!
  \class Body_InterceptPlan
  \brief intercept the ball
*/
class Body_InterceptPlan : public rcsc::BodyAction {

    bool M_called_from_block = false;
public:
    /*!
      \brief constructor
     */
    explicit Body_InterceptPlan(bool called_from_block = false) : M_called_from_block(called_from_block) {
    }

    /*!
      \brief intercept the ball
      \param agent agent pointer to the agent itself
      \return true with action, false if can't intercept
     */
    bool execute( rcsc::PlayerAgent * agent ) override;
};

#endif
