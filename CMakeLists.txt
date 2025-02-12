cmake_minimum_required(VERSION 3.10)
project(Stelle)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set SDL2 paths
set(SDL2_DIR "C:/msys64/mingw64") # CHANGE THIS BEFORE COMPIING BASED ON YOUR SDL2 PATH
set(SDL2_INCLUDE_DIRS "${SDL2_DIR}/include/SDL2")
set(SDL2_LIBRARIES "${SDL2_DIR}/lib")

# Add SDL2 include directories
include_directories(${SDL2_INCLUDE_DIRS})
link_directories(${SDL2_LIBRARIES})

# Set source files
set(SOURCE_FILES 
    src/main.cpp
    src/resources.rc
)

# Add executable
add_executable(${PROJECT_NAME} WIN32 ${SOURCE_FILES})

# Link SDL2, SDL2_mixer and SDL2_ttf
target_link_libraries(${PROJECT_NAME} 
    SDL2
    SDL2main
    SDL2_mixer
    SDL2_ttf
)
