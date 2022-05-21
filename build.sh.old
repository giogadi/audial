
MAKE_ARGS=""
SILENT_ARG="-s"
while getopts 'dv' OPTION; do
    case "$OPTION" in
        d) 
            MAKE_ARGS="${MAKE_ARGS} debug OBJDIR=intermediates-dbg GAMENAME=game-dbg"
            ;;
        v)
            SILENT_ARG=""
            ;;
        ?)
            echo "Unrecognized option ${OPTION}"
            exit 1
            ;;
    esac
done

make -j4 $SILENT_ARG $MAKE_ARGS

# (
#     set -x ; 
#     clang++ -std=c++17 \
#     $BUILD_FLAGS \
#     -I src/glfw/include \
#     -I src/glad/include \
#     -L src/glfw/lib-x86_64 \
#     -l portaudio \
#     -lglfw -framework Cocoa -framework OpenGL -framework IOKit \
#     src/game.cpp src/audio.cpp src/synth.cpp src/glad/src/glad.cpp src/shader.cpp src/stb_image.cpp \
#     -o game.out
# )