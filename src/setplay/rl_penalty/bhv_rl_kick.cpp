#include "bhv_rl_kick.h"

#include <rcsc/common/server_param.h>
#include <vector>
#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/player_agent.h>
#include "basic_actions/body_smart_kick.h"
#include "basic_actions/view_wide.h"
#include "basic_actions/neck_turn_to_point.h"
#include "basic_actions/neck_turn_to_ball.h"
#include "basic_actions/body_kick_one_step.h"
using namespace rcsc;

 std::vector<float> BhvRlKick::world_model_to_input_tensor(const rcsc::WorldModel &wm){
        std::vector<float> input_tensor_values;
        const ServerParam & SP = ServerParam::i();
        static const double hl = SP.pitchHalfLength();
        static const double hw = SP.pitchHalfWidth();
        double dist2goal = wm.self().pos().dist(Vector2D(hl,0))/120;
        double angle2goal = wm.self().body().degree()/90;
        input_tensor_values.push_back(dist2goal);
        input_tensor_values.push_back(angle2goal);
        input_tensor_values.push_back(wm.ball().pos().x/hl);
        input_tensor_values.push_back(wm.ball().pos().y/hw);
        auto opp = wm.getTheirGoalie();
        if(opp){
            input_tensor_values.push_back(opp->pos().x/hl);
            input_tensor_values.push_back(opp->pos().y/hw);
        }else{
            input_tensor_values.push_back(1);
            input_tensor_values.push_back(0);
        }
        double opp_relative_angle = (opp->angleFromBall()-opp->body()).degree()/180.0;
        input_tensor_values.push_back(opp_relative_angle);
        input_tensor_values.push_back(opp->distFromSelf()/100);
        input_tensor_values.push_back(opp->angleFromSelf().degree()/180.0);
        return input_tensor_values;
 }

static ONNX_Model model;
void BhvRlKick::set_penalty_neck_action(rcsc::PlayerAgent * agent){
    if ( agent->effector().queuedNextViewWidth().type() != ViewWidth::WIDE )
    {
        //agent->doChangeView( ViewWidth::WIDE );
        agent->setViewAction( new View_Wide() );
    }
    agent->setNeckAction(new Neck_TurnToPoint(Vector2D(52.5,0)));

    
}
bool BhvRlKick::execute( rcsc::PlayerAgent * agent ){
    const WorldModel &wm = agent->world();
    auto features = world_model_to_input_tensor(wm);
    auto output = model.run_inference(features);
    double kick_angle = output[0]*180;
    double kick_power = output[1]*ServerParam::i().ballSpeedMax();
    Vector2D target = wm.ball().pos() + Vector2D::from_polar(1,kick_angle);
    set_penalty_neck_action(agent);
    return Body_KickOneStep(target,kick_power,true).execute(agent);
    

}