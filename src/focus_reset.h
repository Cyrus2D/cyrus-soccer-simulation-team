
#ifndef FOCUS_RESET_H
#define FOCUS_RESET_H

#include <rcsc/player/soccer_action.h>
#include <rcsc/geom/vector_2d.h>

namespace rcsc {

/*!
  \class Focus_Reset
  \brief reset the focus point to the center of self.
*/
class Focus_Reset {
public:
    /*!
      \brief constructor
     */
    Focus_Reset()
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
    //       return new Focus_Reset();
    //   }
};

}

#endif