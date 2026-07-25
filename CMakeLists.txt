cmake_minimum_required(VERSION 3.16)
project(HarvestVale CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL3 and SDL3_image
find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)

# Add source files
add_executable(sdl3_game
    src/main.cpp
    src/Game.cpp
    src/World.cpp
    src/maps/Village.cpp
    src/maps/Farm.cpp
    src/Autotile.cpp
    src/Entities.cpp
    src/Assets.cpp
    src/Sprites.cpp
    src/Font.cpp
    src/Sfx.cpp
)

# Link libraries
target_link_libraries(sdl3_game PRIVATE SDL3::SDL3 SDL3_image::SDL3_image)

# For macOS, we need to handle the bundle
if(APPLE)
    set_target_properties(sdl3_game PROPERTIES
        MACOSX_BUNDLE TRUE
    )
    # Copy the asset pack into the bundle Resources
    add_custom_command(TARGET sdl3_game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/Farm RPG - Tiny Asset Pack - (All in One)"
        "$<TARGET_FILE_DIR:sdl3_game>/../Resources/Farm RPG - Tiny Asset Pack - (All in One)"
        VERBATIM
    )
endif()
