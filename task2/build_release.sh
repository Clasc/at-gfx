# sudo apt-get update
# sudo apt install g++
# sudo apt install mesa-common-dev
# sudo apt install libglu1-mesa-dev freeglut3-dev
# sudo apt install libsdl2-dev
# sudo apt install libglew-dev
mkdir -p ./binaries/linux_release
pushd ./binaries/linux_release
g++ ./../../source/main.cpp -O3 -I./../../external -I./../../external/SDL -std=gnu++11 -Wno-write-strings -Werror -Wswitch -lSDL2 -lGL -lGLEW -o rrt_release
popd