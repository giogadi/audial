cmake_minimum_required(VERSION 3.10)

project(Audial)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# STK
add_subdirectory(src/stk)

add_executable(game
    src/stb_image.cpp src/imgui/imgui.cpp src/imgui/imgui_demo.cpp src/imgui/imgui_draw.cpp
    src/imgui/imgui_tables.cpp src/imgui/imgui_widgets.cpp src/imgui/backends/imgui_impl_glfw.cpp
    src/imgui/backends/imgui_impl_opengl3.cpp
    src/glad/src/glad.cpp
    src/serial.cpp
    src/audio_loader.cpp src/game.cpp src/audio_util.cpp src/audio.cpp src/audio_event_imgui.cpp
    src/sound_bank.cpp
    src/editor.cpp
    src/version_id_list_simple.cpp
    src/entity_picking.cpp src/entity_editing_context.cpp src/script_action.cpp
    src/shader.cpp src/synth.cpp  src/matrix.cpp src/mesh.cpp
    src/beat_clock.cpp src/input_manager.cpp src/beat_time_event.cpp
    src/component.cpp src/entity.cpp src/entity_manager.cpp
    src/new_entity.cpp    
    src/renderer.cpp src/collisions.cpp
    src/components/transform.cpp
    src/components/model_instance.cpp src/components/camera.cpp src/components/light.cpp
    src/components/rigid_body.cpp src/components/beep_on_hit.cpp src/components/player_controller.cpp
    src/components/player_orbit_controller.cpp src/components/camera_controller.cpp src/components/hit_counter.cpp
    src/components/orbitable.cpp src/components/events_on_hit.cpp src/components/activator.cpp src/components/damage.cpp
    src/components/waypoint_follow.cpp src/components/recorder_balloon.cpp src/components/area_recorder.cpp
    src/components/piano_planet.cpp
    src/entities/light.cpp
    src/entities/camera.cpp
    src/entities/enemy.cpp
    src/entities/player.cpp
    src/entities/sequencer.cpp
    src/enums/audio_EventType.cpp src/enums/audio_SynthParamType.cpp src/enums/CollisionLayer.cpp
    src/enums/ScriptActionType.cpp src/enums/synth_Waveform.cpp)

if(MSVC)
  target_compile_options(game PRIVATE /Wall)
else()
  target_compile_options(game PRIVATE -Wall)
endif()

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

add_executable(stk_test EXCLUDE_FROM_ALL
    src/stk_test.cpp src/glad/src/glad.cpp)
target_link_libraries(stk_test glfw)
target_include_directories(stk_test PUBLIC
    src/glfw/include src/glad/include)
target_link_libraries(stk_test PortAudio)
target_include_directories(stk_test PUBLIC src/portaudio/include)
target_link_libraries(stk_test stk)
target_include_directories(stk_test PUBLIC src/)
