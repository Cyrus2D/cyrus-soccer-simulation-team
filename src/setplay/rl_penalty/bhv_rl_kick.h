#ifndef BHV_RL_KICK_H
#define BHV_RL_KICK_H

#include <rcsc/player/soccer_action.h>
#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/world_model.h>
#include "ONNX_model.h"
class BhvRlKick
    : public rcsc::SoccerBehavior {
        public:
            bool execute( rcsc::PlayerAgent * agent );
            BhvRlKick(){}
        private:
            std::vector<float> world_model_to_input_tensor(const rcsc::WorldModel &wm);
            void set_penalty_neck_action(rcsc::PlayerAgent * agent);



};

#endif