cmake_minimum_required(VERSION 3.2)
project(cdlv)

if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH
        /usr/local/Cellar/sdl2/2.26.5/lib/cmake/SDL2
        /usr/local/Cellar/sdl2_image/2.6.3/lib/cmake/SDL2_image
        /usr/local/Cellar/sdl2_ttf/2.20.2/lib/cmake/SDL2_ttf
    )
endif()

find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_ttf REQUIRED)

set(CMAKE_C_COMPILER clang)
set(CMAKE_C_STANDARD 17)

add_compile_options(-g)
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    add_compile_options(-O1 -g -fsanitize=address -fno-omit-frame-pointer)
endif()

add_executable(${PROJECT_NAME}
    src/main.c
    src/base.c
    src/parse.c
    src/cdlv.c
    src/menu.c
)

include_directories(${PROJECT_NAME}
    ${SDL2_INCLUDE_DIRS}
    ${SDL2IMAGE_INCLUDE_DIRS}
    ${SDL2TTF_INCLUDE_DIRS}
)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_link_libraries(${PROJECT_NAME}
        SDL2::SDL2
        SDL2_image
        SDL2_ttf
        -fsanitize=address
    )
else()
    target_link_libraries(${PROJECT_NAME}
        SDL2::SDL2
        SDL2_image
        SDL2_ttf
    )
endif()
