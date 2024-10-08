cmake_minimum_required(VERSION 3.5)
project(cdlv VERSION 1.75.4)
set(CMAKE_C_COMPILER gcc)
set(CMAKE_C_STANDARD 11)
    
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    add_compile_options(-O1 -g -Wall -Werror -fsanitize=undefined -fsanitize=address -fno-omit-frame-pointer)
else()
    add_compile_options(-O2 -Wall -Werror)
endif()

option(CDLV_FFMPEG "Include FFmpeg" ON)
option(CDLV_MONGOOSE "Include mongoose" ON)
option(CDLV_SAMPLE "Build sample" ON)
option(CDLV_MDLV_SERVER "Build MDLV server" ON)

if(CDLV_FFMPEG)
    add_compile_definitions(CDLV_FFMPEG)
endif()

if(CDLV_MONGOOSE AND CDLV_MDLV_SERVER)
    add_library(${PROJECT_NAME}
        cdlv.c
        util.c
        parse.c
        resource.c
        scene.c
        mdlv.c
	scl/scl.c
        mongoose/mongoose.c
	cJSON/cJSON.c
    )
else()
    add_library(${PROJECT_NAME}
        cdlv.c
        util.c
        parse.c
        resource.c
        scene.c
	scl/scl.c
    )
endif()

if(CDLV_SAMPLE)
    find_package(SDL2 REQUIRED)
    find_package(SDL2_image REQUIRED)
    find_package(SDL2_ttf REQUIRED)

    set(EXEC_NAME cdlv-sample)
    
    add_executable(${EXEC_NAME}
        sample.c
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
    	    avcodec
    	    avformat
    	    avutil
    	    swscale
	    -fsanitize=address
	    -fsanitize=undefined
    	    ${PROJECT_NAME}
    	)
    else()
	target_link_libraries(${EXEC_NAME}
    	    SDL2::SDL2
    	    SDL2_image
    	    SDL2_ttf
    	    avcodec
    	    avformat
    	    avutil
    	    swscale
    	    ${PROJECT_NAME}
    	)
    endif()
endif()

if(CDLV_MDLV_SERVER)
    set(MDLV_EXEC_NAME mdlv-server)
    
    add_executable(${MDLV_EXEC_NAME}
        server.c
    )
        
    if(CMAKE_BUILD_TYPE MATCHES "Debug")
	target_link_libraries(${MDLV_EXEC_NAME}
    	    avcodec
    	    avformat
    	    avutil
    	    swscale
	    -fsanitize=address
	    -fsanitize=undefined
    	    ${PROJECT_NAME}
    	)
    else()
	if(CDLV_FFMPEG)
	    target_link_libraries(${MDLV_EXEC_NAME}
		avcodec
		avformat
		avutil
		swscale
		${PROJECT_NAME}
	    )
	else()
	    target_link_libraries(${MDLV_EXEC_NAME}
		${PROJECT_NAME}
	    )
	endif()
    endif()
endif()
