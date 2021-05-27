/*
 * @author: Mohsen SAdeghipour
 * @email: mohsen.sadeghipour@icloud.com
 * @year: 2018
 */

#ifndef STAMINA_WITH_POINTTO
#define STAMINA_WITH_POINTTO

#include "player_agent.h"

namespace rcsc {

class StaminaWithPointto {

    static bool s_enable;

public:
    static void setEnable(bool val);
    static void setStaminaFromVisionData(double& desStamina, double angle_in_degree);
    static void doPointtoForStamina(PlayerAgent* agent);

};

}

#endif // STAMINA_WITH_POINTTO
