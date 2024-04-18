
#ifndef FOCUS_MOVE_TO_POINT_H
#define FOCUS_MOVE_TO_POINT_H

#include <rcsc/player/soccer_action.h>
#include <rcsc/geom/vector_2d.h>

namespace rcsc {

/*!
  \class Focus_ChangeToPoint
*/
class Focus_MoveToPoint
     {
private:
    //! target point
    const Vector2D M_point;
public:
    /*!
      \brief constructor
      \param point target point
     */
    explicit
    Focus_MoveToPoint( const Vector2D & point )
        : M_point( point )
      { }

    /*!
      \brief execute action
      \param agent pointer to the agent itself
      \return true if action is performed
    */
    bool execute( PlayerAgent * agent );

    /*!
      \brief create cloned object
      \return pointer to the cloned object
     */
    // FocusAction * clone() const
    //   {
    //       return new Focus_MoveToPoint( M_point );
    //   }
};

}

#endif