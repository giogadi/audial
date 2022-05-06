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
    -I glfw/include \
    -I glad/include \
    -L glfw/lib-x86_64 \
    -l portaudio \
    -lglfw -framework Cocoa -framework OpenGL -framework IOKit \
    game.cpp audio.cpp synth.cpp glad/src/glad.cpp \
    -o game.out
)