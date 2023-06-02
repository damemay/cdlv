cmake_minimum_required(VERSION 3.2)

option(VITA "Build for vita" OFF)
option(MINGW "Build with mingw64" OFF)

if(VITA)

    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")

project(cdlv)
  include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "cdlv")
set(VITA_TITLEID  "VSDK07777")
set(VITA_VERSION  "01.00")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c17")

set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d ATTRIBUTE2=12")

include_directories(
  ../common
)

add_executable(${PROJECT_NAME}
  src/main.c
  src/cdlv_base.c
  src/cdlv_main.c
  src/cdlv_parse.c
  src/cdlv_menu.c
)

target_link_libraries(${PROJECT_NAME}
  SDL2_ttf
  SDL2_image
  SDL2
  SDL2main
  mathneon
  freetype
  vorbisfile
  vorbis
  mikmod
  modplug
  ogg
  mpg123
  FLAC
  webp
  jpeg
  png
  pthread
  vitashark
  SceShaccCgExt
  SceDisplay_stub
  SceCtrl_stub
  SceAudio_stub
  SceAudioIn_stub
  SceSysmodule_stub
  SceMotion_stub
  SceGxm_stub
  SceCommonDialog_stub
  SceTouch_stub
  SceHid_stub
  SceLibKernel_stub
  SceShaccCg_stub
  SceIme_stub
  SceAppMgr_stub
  SceKernelDmacMgr_stub
  taihen_stub
  m
  z
  c
)

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME})
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE sce_sys/icon0.png sce_sys/icon0.png
  FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
  FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
  FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
  FILE res res
)

else()

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

if(MINGW)
add_executable(${PROJECT_NAME} WIN32
    src/main.c
    src/base.c
    src/parse.c
    src/cdlv.c
    src/menu.c
)
else()
add_executable(${PROJECT_NAME}
    src/main.c
    src/cdlv_base.c
    src/cdlv_parse.c
    src/cdlv_main.c
    src/cdlv_menu.c
    src/cdlv_text.c
)
endif()

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
elseif(MINGW)
    target_link_libraries(${PROJECT_NAME}
        mingw32
        SDL2main
        SDL2
        SDL2_image
        SDL2_ttf
    )
else()
    target_link_libraries(${PROJECT_NAME}
        SDL2::SDL2
        SDL2_image
        SDL2_ttf
    )
endif()
endif()
