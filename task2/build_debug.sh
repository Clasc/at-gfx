# sudo apt-get update
# sudo apt install g++
# sudo apt install mesa-common-dev
# sudo apt install libglu1-mesa-dev freeglut3-dev
# sudo apt install libsdl2-dev
# sudo apt install libglew-dev
mkdir -p ./binaries/linux_debug
pushd ./binaries/linux_debug
g++ ./../../source/main.cpp -I./../../external -I./../../external/SDL -std=gnu++11 -Wno-write-strings -Werror -Wswitch -lSDL2 -lGL -lGLEW -o rrt_debug
popd