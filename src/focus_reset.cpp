
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "focus_reset.h"

#include <rcsc/player/player_agent.h>
#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>

namespace rcsc {

/*-------------------------------------------------------------------*/
/*!

*/
bool
Focus_Reset::execute( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    const double moment_dist = -wm.self().focusDist();
    const AngleDeg moment_dir = -wm.self().focusDir();

    agent->doChangeFocus( moment_dist, moment_dir );
    return true;
}

}