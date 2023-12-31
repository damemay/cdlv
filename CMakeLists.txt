cmake_minimum_required(VERSION 3.5)

option(VITA "Build for vita" OFF)
option(ZDLV_ONLY "Build only ZDLV" OFF)

if(VITA)

    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")

    project(cdlv)
      include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
    
    set(VITA_APP_NAME "cdlv")
    set(VITA_TITLEID  "CDLV07777")
    set(VITA_VERSION  "01.00")
    
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c17 -O2")
    
    set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d ATTRIBUTE2=12")
    
    include_directories(
      ../common
    )
    
    add_executable(${PROJECT_NAME}
      menu/main.c
      cdlv_base.c
      cdlv_main.c
      cdlv_parse.c
      menu/cdlv_menu.c
      cdlv_text.c
      cdlv_render.c
      cdlv_choice.c
      cdlv_util.c
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
    if(NOT ZDLV_ONLY)
    if(APPLE)
        list(APPEND CMAKE_PREFIX_PATH
            /usr/local/Cellar/sdl2/2.26.5/lib/cmake/SDL2
            /usr/local/Cellar/sdl2_image/2.6.3/lib/cmake/SDL2_image
            /usr/local/Cellar/sdl2_ttf/2.20.2/lib/cmake/SDL2_ttf
        )
    endif()
    
    if(MINGW)
	    list(APPEND CMAKE_PREFIX_PATH
		    /c/msys64/mingw64/lib/cmake/SDL2/
		    /c/msys64/mingw64/lib/cmake/SDL2_image/
		    /c/msys64/mingw64/lib/cmake/SDL2_ttf/
	    )
    endif()
    find_package(SDL2 REQUIRED)
    find_package(SDL2_image REQUIRED)
    find_package(SDL2_ttf REQUIRED)
    endif()
    
    set(CMAKE_C_COMPILER gcc)
    set(CMAKE_C_STANDARD 17)
    
    add_compile_options(-O2)
    if(CMAKE_BUILD_TYPE MATCHES "Debug")
        add_compile_options(-O1 -g -fsanitize=address -fno-omit-frame-pointer)
    endif()
    
    if(NOT ZDLV_ONLY)
    add_library(${PROJECT_NAME} STATIC
        cdlv_base.c
        cdlv_parse.c
        cdlv_main.c
        cdlv_text.c
        cdlv_render.c
        cdlv_choice.c
        cdlv_util.c
    )
    endif()

    if(NOT ZDLV_ONLY)
    set(EXEC_NAME cdlv-menu)
    endif()
    set(ZDLV_NAME zdlv-packer)
    set(SERV_NAME zdlv-server)

    if(MINGW)
	    add_executable(${EXEC_NAME} WIN32
		    menu/main.c
		    menu/cdlv_menu.c
                    menu/cdlv_sdl.c
                zocket/zocket.c
            )
    else()
        if(NOT ZDLV_ONLY)
	    add_executable(${EXEC_NAME}
		menu/main.c
		menu/cdlv_menu.c
                menu/cdlv_sdl.c
                zocket/zocket.c
	    )
        endif()
            add_executable(${ZDLV_NAME}
                zdlv.c
                zocket/zocket.c
            )
            add_executable(${SERV_NAME}
                zocket/zocket.c
                zdlv_s.c
            )
    endif()
    
    if(MINGW)
	    set(SDL2_INCLUDE_DIRS "/c/msys64/mingw64/include/SDL2")
    endif()
    if(NOT ZDLV_ONLY)
    include_directories(${EXEC_NAME}
        ${SDL2_INCLUDE_DIRS}
        ${SDL2IMAGE_INCLUDE_DIRS}
        ${SDL2TTF_INCLUDE_DIRS}
    )
    include_directories(${ZDLV_NAME}
        ${SDL2_INCLUDE_DIRS}
        ${SDL2IMAGE_INCLUDE_DIRS}
        ${SDL2TTF_INCLUDE_DIRS}
    )
    endif()
    
    if(CMAKE_BUILD_TYPE MATCHES "Debug")
        if(NOT ZDLV_ONLY)
        target_link_libraries(${EXEC_NAME}
            SDL2::SDL2
            SDL2_image
            SDL2_ttf
            zstd
            ${PROJECT_NAME}
            -fsanitize=address
        )
        endif()
        target_link_libraries(${ZDLV_NAME}
            SDL2::SDL2
            SDL2_image
            SDL2_ttf
            zstd
            ${PROJECT_NAME}
            -fsanitize=address
        )
        target_link_libraries(${SERV_NAME}
            SDL2::SDL2
            SDL2_image
            SDL2_ttf
            zstd
            ${PROJECT_NAME}
            -fsanitize=address
        )
    else()
	    if(MINGW)
            target_link_libraries(${EXEC_NAME}
		${PROJECT_NAME}
		SDL2_ttf
		SDL2_image
		SDL2
		SDL2main
                zstd
		mingw32
            )
            add_definitions(-DSDL_MAIN_HANDLED)
	else()
            if(NOT ZDLV_ONLY)
            target_link_libraries(${EXEC_NAME}
                SDL2::SDL2
                SDL2_image
                SDL2_ttf
                zstd
                ${PROJECT_NAME}
            )
            endif()
            target_link_libraries(${ZDLV_NAME}
                SDL2::SDL2
                SDL2_image
                SDL2_ttf
                zstd
                ${PROJECT_NAME}
            )
            target_link_libraries(${SERV_NAME}
                SDL2::SDL2
                SDL2_image
                SDL2_ttf
                zstd
                ${PROJECT_NAME}
            )
	endif()
    endif()
endif()
