// -*-c++-*-

/*!
  \file body_turn_to_tackle.h
  \brief turn to tackle the ball
*/


/////////////////////////////////////////////////////////////////////

#ifndef BODY_TURN_TO_TACKLE_H
#define BODY_TURN_TO_TACKLE_H

#include <rcsc/player/soccer_action.h>

/*!
  \class Body_TurnToTackle
  \brief turn to tackle the ball
*/
class Body_TurnToTackle : public rcsc::BodyAction {

public:
    /*!
      \brief constructor
     */
    Body_TurnToTackle() = default;

    /*!
      \brief do clear ball
      \param agent agent pointer to the agent itself
      \return true with action, false if can't turn to tackle
     */
    bool execute( rcsc::PlayerAgent * agent ) override;
};

#endif
