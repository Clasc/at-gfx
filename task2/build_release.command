mkdir -p ./binaries/osx_release
pushd ./binaries/osx_release
clang++ -ObjC++ -O3 ./../../source/main.cpp -I ./../../external -I ./../../external/SDL -std=gnu++11 -Wno-c++11-compat-deprecated-writable-strings -Werror -Wno-c++11-extensions -Wno-writable-strings -Wswitch -F/Library/Frameworks -framework SDL2 -framework OpenGL -framework Cocoa -lGLEW -o rrt_release
popd