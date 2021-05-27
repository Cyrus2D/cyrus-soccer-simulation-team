#include "stamina_with_pointto.h"

#include <rcsc/geom.h>
//#include "../geom/vector_2d.h"
//#include "../geom/angle_deg.h"

bool rcsc::StaminaWithPointto::s_enable = true;

void rcsc::StaminaWithPointto::setStaminaFromVisionData(double &desStamina, double angle_in_degree)
{
    if( !s_enable || angle_in_degree > 0 || angle_in_degree < -180 )
        return;

    desStamina = (180 + angle_in_degree) / (180) * 8000;
}

void rcsc::StaminaWithPointto::doPointtoForStamina(rcsc::PlayerAgent *agent)
{
    if(!s_enable)
        return;

    Vector2D point = agent->world().self().pos()
                        + Vector2D::polar2vector(50, AngleDeg((agent->world().self().stamina() / 8000 * (180)) - 180));
    agent->doPointto(point.x, point.y);

}

void rcsc::StaminaWithPointto::setEnable(bool val)
{
    s_enable = val;
}
