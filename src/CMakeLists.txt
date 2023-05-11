# player
add_executable(sample_player
  strategy.cpp
setplay/bhv_set_play_goal_kick.cpp
setplay/bhv_set_play.cpp
setplay/bhv_go_to_static_ball.cpp
setplay/bhv_penalty_kick.cpp
setplay/bhv_set_play_indirect_free_kick.cpp
setplay/bhv_set_play_free_kick.cpp
setplay/bhv_set_play_kick_off.cpp
setplay/bhv_their_goal_kick_move.cpp
setplay/bhv_custom_before_kick_off.cpp
setplay/bhv_set_play_kick_in.cpp
setplay/bhv_prepare_set_play_kick.cpp
setplay/intention_wait_after_set_play_kick.cpp
intention_receive.cpp
roles/soccer_role.cpp
roles/role_goalie.cpp
roles/role_player.cpp
roles/role_sample.cpp
sample_field_evaluator.cpp
sample_freeform_message_parser.cpp
sample_freeform_message.cpp
move_off/bhv_scape.cpp
move_off/bhv_unmark2019.cpp
move_off/bhv_offensive_move.cpp
move_off/bhv_scape_voronoi.cpp
move_off/bhv_unmark.cpp
main_player.cpp
data_extractor/DEState.cpp
data_extractor/offensive_data_extractor_v1.cpp
data_extractor/offensive_data_extractor.cpp
sample_communication.cpp
calculate_offensive_opponents.cpp
neck/view_tactical.cpp
neck/neck_offensive_intercept_neck.cpp
neck/neck_decision.cpp
neck/neck_default_intercept_neck.cpp
neck/next_pass_predictor.cpp
goalie/neck_goalie_turn_neck.cpp
goalie/bhv_goalie_basic_move.cpp
goalie/bhv_goalie_free_kick.cpp
goalie/bhv_goalie_chase_ball.cpp
bhv_basic_offensive_kick.cpp
sample_player.cpp
bhv_basic_move.cpp
sample_coach.cpp
setting.cpp
move_def/mark_position_finder.cpp
move_def/bhv_basic_tackle.cpp
move_def/bhv_mark_decision_greedy.cpp
move_def/bhv_block.cpp
move_def/bhv_mark_decisions.cpp
move_def/best_match_finder.cpp
move_def/bhv_mark_execute.cpp
move_def/cyrus_interceptable.cpp
sample_trainer.cpp
chain_action/self_pass_generator.cpp
chain_action/simple_pass_checker.cpp
chain_action/bhv_chain_action.cpp
chain_action/actgen_short_dribble.cpp
chain_action/DataExtractor.cpp
chain_action/strict_check_pass_generator.cpp
chain_action/actgen_shoot.cpp
chain_action/action_chain_graph.cpp
chain_action/cross_generator.cpp
chain_action/shoot_generator.cpp
chain_action/pass.cpp
chain_action/clear_ball.cpp
chain_action/field_analyzer.cpp
chain_action/short_dribble_generator.cpp
chain_action/actgen_direct_pass.cpp
chain_action/bhv_pass_kick_find_receiver.cpp
chain_action/clear_generator.cpp
chain_action/cooperative_action.cpp
chain_action/action_chain_holder.cpp
chain_action/predict_state.cpp
chain_action/body_force_shoot.cpp
chain_action/actgen_simple_dribble.cpp
chain_action/bhv_strict_check_shoot.cpp
chain_action/hold_ball.cpp
chain_action/actgen_cross.cpp
chain_action/dribble.cpp
chain_action/shoot.cpp
chain_action/bhv_normal_dribble.cpp
chain_action/neck_turn_to_receiver.cpp
chain_action/tackle_generator.cpp
chain_action/actgen_self_pass.cpp
chain_action/actgen_strict_check_pass.cpp
  )

target_include_directories(sample_player
  PRIVATE
  ${PROJECT_SOURCE_DIR}/src/
  ${PROJECT_SOURCE_DIR}/src/chain_action
  ${PROJECT_SOURCE_DIR}/src/move_def
  ${PROJECT_SOURCE_DIR}/src/goalie
  ${PROJECT_SOURCE_DIR}/src/neck
  ${PROJECT_SOURCE_DIR}/src/data_extractor
  ${PROJECT_SOURCE_DIR}/src/move_off
  ${PROJECT_SOURCE_DIR}/src/roles
  ${PROJECT_SOURCE_DIR}/src/setplay
  ${PROJECT_BINARY_DIR}
  PUBLIC
  ${Boost_INCLUDE_DIRS}
  ${LIBRCSC_INCLUDE_DIR}
  )

target_link_libraries(sample_player
  PUBLIC
  ${LIBRCSC_LIB}
  Boost::system
  ZLIB::ZLIB
  )

set_target_properties(sample_player
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/src
  )

# coach
add_executable(sample_coach
	  sample_freeform_message.cpp
	  sample_coach.cpp
	  main_coach.cpp
	  )

	target_include_directories(sample_coach
	  PRIVATE
	  ${PROJECT_SOURCE_DIR}/src/
	  ${PROJECT_BINARY_DIR}
	  PUBLIC
	  ${Boost_INCLUDE_DIRS}
	  ${LIBRCSC_INCLUDE_DIR}
	  )

	target_link_libraries(sample_coach
	  PUBLIC
	  ${LIBRCSC_LIB}
	  Boost::system
	  ZLIB::ZLIB
	  )

	set_target_properties(sample_coach
	  PROPERTIES
	  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/src
	  )

# trainer
#add_executable(sample_trainer
# sample_trainer.cpp
# main_trainer.cpp
# )

#target_include_directories(sample_trainer
# PRIVATE
# ${PROJECT_SOURCE_DIR}/src/
# ${PROJECT_BINARY_DIR}
# PUBLIC
# ${Boost_INCLUDE_DIRS}
# ${LIBRCSC_INCLUDE_DIR}
# )

#target_link_libraries(sample_trainer
# PUBLIC
# ${LIBRCSC_LIB}
# Boost::system
# ZLIB::ZLIB
# )

#set_target_properties(sample_trainer
# PROPERTIES
# RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/src
# )

# generate shell scripts and copy them to the binary direcotry as an executable script
configure_file(
  ${PROJECT_SOURCE_DIR}/src/start.sh.in
  ${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/start.sh
  @ONLY
  )
file(
  COPY ${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/start.sh
  DESTINATION ${PROJECT_BINARY_DIR}/src
  FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  )

configure_file(
  ${PROJECT_SOURCE_DIR}/src/keepaway.sh.in
  ${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/keepaway.sh
  @ONLY
  )
file(
  COPY ${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/keepaway.sh
  DESTINATION ${PROJECT_BINARY_DIR}/src
  FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  )

configure_file(
  ${PROJECT_SOURCE_DIR}/src/train.sh.in
  ${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/train.sh
  @ONLY
  )
file(
  COPY ${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/train.sh
  DESTINATION ${PROJECT_BINARY_DIR}/src
  FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  )

# copy other files to the binary direcotry
file(COPY
  formations-dt player.conf coach.conf start-debug.sh start-offline.sh auto.sh github.sh
  DESTINATION ${PROJECT_BINARY_DIR}/src/
  )

file(
  COPY data
  DESTINATION ${PROJECT_BINARY_DIR}/src
  )
