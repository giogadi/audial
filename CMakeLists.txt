cmake_minimum_required(VERSION 3.10)

project(Audial)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

if(MSVC)
  add_compile_options(/W2)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
  add_compile_options(-Wall)
endif()

# STK
add_subdirectory(src/stk)

add_executable(game
    src/features.h

    src/stb_image.cpp src/imgui/imgui.cpp src/imgui/imgui_demo.cpp src/imgui/imgui_draw.cpp
    src/imgui/imgui_tables.cpp src/imgui/imgui_widgets.cpp src/imgui/backends/imgui_impl_glfw.cpp
    src/imgui/backends/imgui_impl_opengl3.cpp
    src/glad/src/glad.cpp

    src/constants.h

    src/editor_id.cpp src/editor_id.h
    src/util.h
    src/string_util.h
    src/math_util.h
    src/logger.h

    src/imgui_util.cpp src/imgui_util.h
    src/imgui_vector_util.h

    src/midi_util.h    

    src/viewport.cpp src/viewport.h

    src/matrix.cpp src/matrix.h
    src/quaternion.cpp src/quaternion.h
    src/transform.cpp src/transform.h
    src/aabb.cpp src/aabb.h

    src/game_manager.h

    src/filter.cpp

    src/serial.cpp src/serial.h
    src/serial_vector_util.h
    src/rng.cpp src/rng.h
    src/game.cpp
    src/audio_util.cpp src/audio_util.h
    src/audio.cpp src/audio.h
    src/audio_event_imgui.cpp src/audio_event_imgui.h
    src/sound_bank.cpp src/sound_bank.h
    src/editor.cpp src/editor.h
    src/version_id_list_simple.cpp src/version_id_list_simple.h
    src/entity_picking.cpp src/entity_picking.h
    src/shader.cpp src/shader.h
    src/synth.cpp src/synth.h
    src/mesh.cpp src/mesh.h
    src/synth_patch.cpp src/synth_patch.h
    src/synth_patch_bank.cpp src/synth_patch_bank.h
    src/beat_clock.cpp src/beat_clock.h
    src/input_manager.cpp src/input_manager.h
    src/beat_time_event.cpp src/beat_time_event.h
    src/synth_imgui.cpp src/synth_imgui.h
    src/new_entity_id.h
    src/new_entity.cpp src/new_entity.h
    src/renderer.cpp src/renderer.h
    src/camera_util.h
    src/stb_truetype.cpp src/stb_truetype.h
    src/geometry.cpp src/geometry.h

    src/string_ci.cpp src/string_ci.h

    src/waypoint_follower.cpp src/waypoint_follower.h
    src/random_wander.cpp src/random_wander.h    

    src/properties/Test.cpp
    src/properties/MidiNoteAndName.cpp src/properties/MidiNoteAndName.h
    src/properties/ChangeStepSequencerSeqAction.cpp src/properties/ChangeStepSequencerSeqAction.h
    src/properties/SpawnAutomatorSeqAction.cpp src/properties/SpawnAutomatorSeqAction.h
    src/properties/NoteOnOffSeqAction.cpp src/properties/NoteOnOffSeqAction.h
    src/properties/CameraControlSeqAction.cpp src/properties/CameraControlSeqAction.h
    src/properties/ChangeStepSeqMaxVoices.cpp src/properties/ChangeStepSeqMaxVoices.h
    src/properties/VfxEntity.cpp src/properties/VfxEntity.h
    src/properties/VfxPulseSeqAction.cpp src/properties/VfxPulseSeqAction.h
    src/properties/SetAllStepsSeqAction.cpp src/properties/SetAllStepsSeqAction.h

    src/seq_action.cpp src/seq_action.h
    src/actions.cpp src/actions.h
    src/seq_actions/camera_control.cpp src/seq_actions/camera_control.h
    src/seq_actions/change_patch.cpp src/seq_actions/change_patch.h
    src/seq_actions/vfx_pulse.cpp src/seq_actions/vfx_pulse.h
    src/seq_actions/set_enemy_hittable.cpp src/seq_actions/set_enemy_hittable.h
   
    src/entities/test.cpp src/entities/test.h 
    src/entities/light.cpp src/entities/light.h
    src/entities/camera.cpp src/entities/camera.h    
    src/entities/sequencer.cpp src/entities/sequencer.h
    src/entities/typing_enemy.cpp src/entities/typing_enemy.h
    src/entities/step_sequencer.cpp src/entities/step_sequencer.h
    src/entities/action_sequencer.cpp src/entities/action_sequencer.h
    src/entities/param_automator.cpp src/entities/param_automator.h
    src/entities/flow_player.cpp src/entities/flow_player.h
    src/entities/flow_wall.cpp src/entities/flow_wall.h
    src/entities/flow_pickup.cpp src/entities/flow_pickup.h
    src/entities/flow_trigger.cpp src/entities/flow_trigger.h
    src/entities/int_variable.cpp src/entities/int_variable.h
    src/entities/vfx.cpp src/entities/vfx.h
    src/entities/mech.cpp src/entities/mech.h
    src/entities/resource.cpp src/entities/resource.h
    src/entities/viz.cpp src/entities/viz.h
    
    src/enums/audio_EventType.cpp src/enums/audio_EventType.h
    src/enums/audio_SynthParamType.cpp src/enums/audio_SynthParamType.h
    src/enums/CollisionLayer.cpp src/enums/CollisionLayer.h
    src/enums/ScriptActionType.cpp src/enums/ScriptActionType.h
    src/enums/synth_Waveform.cpp src/enums/synth_Waveform.h
    src/enums/ColorPreset.cpp src/enums/ColorPreset.h
    src/enums/Direction.cpp src/enums/Direction.h
    src/enums/WaypointFollowerMode.cpp src/enums/WaypointFollowerMode.h
    src/enums/SeqActionType.cpp src/enums/SeqActionType.h
    src/enums/TypingEnemyType.cpp src/enums/TypingEnemyType.h
    src/enums/HitResponseType.cpp src/enums/HitResponseType.h
    src/enums/MechType.cpp src/enums/MechType.h
)

## GLFW ##
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(src/glfw)
target_link_libraries(game glfw)
target_include_directories(game PUBLIC src/glfw/include)

## PORTAUDIO ##
add_subdirectory(src/portaudio EXCLUDE_FROM_ALL)
target_link_libraries(game PortAudio)
target_include_directories(game PUBLIC src/portaudio/include)

target_include_directories(game PUBLIC
    ./ ./src src/glfw/include src/glad/include src/imgui)

if(MSVC)
else()
    target_include_directories(game PUBLIC src/fftw/)
    target_link_libraries(game ${CMAKE_SOURCE_DIR}/src/fftw/libfftw3.a)
endif()


add_executable(font_tool EXCLUDE_FROM_ALL src/font_tool.cpp)

##
## TEST SHIT ##
##

add_executable(ptree_test EXCLUDE_FROM_ALL src/ptree_test.cpp)
target_include_directories(ptree_test PUBLIC
    ./src)

add_executable(ptree_pimpl_test EXCLUDE_FROM_ALL src/ptree_pimpl_test.cpp)
target_include_directories(ptree_pimpl_test PUBLIC
    ./src)

add_executable(version_id_list_test EXCLUDE_FROM_ALL
    src/version_id_list_test.cpp src/version_id_list_simple.cpp)
target_include_directories(version_id_list_test PUBLIC
    ./src)

add_executable(ecs_test EXCLUDE_FROM_ALL
    src/ecs_test.cpp)
target_include_directories(version_id_list_test PUBLIC
    ./src)

add_executable(new_entity_test EXCLUDE_FROM_ALL
    src/new_entity_test.cpp src/new_entity.cpp src/serial.cpp)
target_include_directories(new_entity_test PUBLIC
    ./src)

add_executable(synth_test EXCLUDE_FROM_ALL
    src/synth_test.cpp src/glad/src/glad.cpp
    src/imgui/imgui.cpp src/imgui/imgui_demo.cpp src/imgui/imgui_draw.cpp
    src/imgui/imgui_tables.cpp src/imgui/imgui_widgets.cpp src/imgui/backends/imgui_impl_glfw.cpp
    src/imgui/backends/imgui_impl_opengl3.cpp
    src/audio_util.cpp src/audio.cpp src/audio_event_imgui.cpp
    src/synth_patch.cpp
    src/synth_patch_bank.cpp
    src/sound_bank.cpp src/synth.cpp
    src/serial.cpp
    src/synth_imgui.cpp
    src/enums/audio_EventType.cpp src/enums/audio_SynthParamType.cpp src/enums/synth_Waveform.cpp)
target_link_libraries(synth_test glfw)
target_include_directories(synth_test PUBLIC
    src/glfw/include src/glad/include)
target_link_libraries(synth_test PortAudio)
target_include_directories(synth_test PUBLIC src/portaudio/include)
target_link_libraries(synth_test stk)
target_include_directories(synth_test PUBLIC src/ src/imgui/ ./)

add_executable(stk_test EXCLUDE_FROM_ALL
    src/stk_test.cpp src/glad/src/glad.cpp)
target_link_libraries(stk_test glfw)
target_include_directories(stk_test PUBLIC
    src/glfw/include src/glad/include)
target_link_libraries(stk_test PortAudio)
target_include_directories(stk_test PUBLIC src/portaudio/include)
target_link_libraries(stk_test stk)
target_include_directories(stk_test PUBLIC src/)

add_executable(matrix_test EXCLUDE_FROM_ALL
    src/matrix_test.cpp src/matrix.cpp src/serial.cpp)                          
target_include_directories(matrix_test PUBLIC src/)

add_executable(bser_test EXCLUDE_FROM_ALL
    src/bser_test.c)

add_executable(synth_render EXCLUDE_FROM_ALL
    src/synth_render.cpp
    src/imgui/imgui.cpp src/imgui/imgui_draw.cpp
    src/imgui/imgui_tables.cpp src/imgui/imgui_widgets.cpp
    src/synth.cpp
    src/synth_patch.cpp
    src/synth_patch_bank.cpp
    src/serial.cpp
    src/enums/audio_EventType.cpp
    src/enums/audio_SynthParamType.cpp
    src/enums/synth_Waveform.cpp)
target_include_directories(synth_render PUBLIC src/ ./)

add_executable(synth_simd_test EXCLUDE_FROM_ALL
    src/synth_simd_test.c)
target_include_directories(synth_simd_test PUBLIC src/ ./)
target_compile_options(synth_simd_test PRIVATE "-mavx")

if (MSVC)
else()
    add_executable(fft_test EXCLUDE_FROM_ALL
        src/fft_test.c)
    target_include_directories(fft_test PUBLIC src/ ./ src/fftw/)
    target_link_libraries(fft_test ${CMAKE_SOURCE_DIR}/src/fftw/libfftw3.a)
endif()

