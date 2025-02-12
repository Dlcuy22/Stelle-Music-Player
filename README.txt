how to compile 
make sure you have gcc, cmake, libsdl2, sdl2_mixer and sdl2_ttf installed

THIS CODEBASE ONLY FOR WINDOWS BECAUSE ITS RELIES ON WINDOWS API
Linux build is soon



first you need to install the gcc toolchain via msys-mingw64 
then you need to install cmake and the sdl2 packages via pacman


then go to root directory of the repo and run this command

Then Compile:
 mkdir build 
 cd build 
 cmake .. -G "MinGW Makefiles" && mingw32-make

The build binary will be at ./build 
Stelle.exe

thats it
 