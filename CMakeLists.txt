cmake_minimum_required(VERSION 3.5)
project(cdlv)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_ttf REQUIRED)
    
set(CMAKE_C_COMPILER gcc)
set(CMAKE_C_STANDARD 17)
    
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    add_compile_options(-O1 -g -fsanitize=address -fno-omit-frame-pointer)
else()
    add_compile_options(-O2)
endif()
    
add_library(${PROJECT_NAME} STATIC
    hashdict.c/hashdict.c
    base.c
    parse.c
    cdlv.c
    text.c
    render.c
    choice.c
    util.c
)

set(EXEC_NAME cdlv-menu)

add_executable(${EXEC_NAME}
    menu/main.c
    menu/cdlv_menu.c
    menu/cdlv_sdl.c
)
    
include_directories(${EXEC_NAME}
    ${SDL2_INCLUDE_DIRS}
    ${SDL2IMAGE_INCLUDE_DIRS}
    ${SDL2TTF_INCLUDE_DIRS}
)
    
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_link_libraries(${EXEC_NAME}
        SDL2::SDL2
        SDL2_image
        SDL2_ttf
        ${PROJECT_NAME}
        -fsanitize=address
    )
else()
    target_link_libraries(${EXEC_NAME}
        SDL2::SDL2
        SDL2_image
        SDL2_ttf
        ${PROJECT_NAME}
    )
endif()