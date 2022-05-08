BUILD_FLAGS=""
while getopts 'dpir' OPTION; do
    case "$OPTION" in
        d) 
            BUILD_FLAGS="${BUILD_FLAGS}-g -O0"
            ;;
        r)
            BUILD_FLAGS="${BUILD_FLAGS} -O3"
            ;;
        ?)
            echo "Unrecognized option ${OPTION}"
            exit 1
            ;;
    esac
done

(
    set -x ; 
    clang++ -std=c++17 \
    $BUILD_FLAGS \
    -I src/glfw/include \
    -I src/glad/include \
    -L src/glfw/lib-x86_64 \
    -l portaudio \
    -lglfw -framework Cocoa -framework OpenGL -framework IOKit \
    src/game.cpp src/audio.cpp src/synth.cpp src/glad/src/glad.cpp src/shader.cpp src/stb_image.cpp \
    -o game.out
)