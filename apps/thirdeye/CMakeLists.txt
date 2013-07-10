
# config file
configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.cmake" "${CMAKE_CURRENT_SOURCE_DIR}/config.hpp")

# local files
set(GAME
    main.cpp
    engine.cpp
    resource.cpp
)

set(GAME_HEADER
    engine.hpp
    config.hpp
    resource.hpp
)
source_group(game FILES ${GAME} ${GAME_HEADER})

# Main executable
add_executable(thirdeye
    ${THIRDEYE_LIBS} ${THIRDEYE_LIBS_HEADER}
    ${THIRDEYE_FILES}
    ${GAME} ${GAME_HEADER}
    ${APPLE_BUNDLE_RESOURCES}
)

target_link_libraries(thirdeye
    ${Boost_LIBRARIES}
    ${SDL2_LIBRARY}
    ${SDL2MAIN_LIBRARY} 
    -lGL
    components
)

# Fix for not visible pthreads functions for linker with glibc 2.15
if (UNIX AND NOT APPLE)
target_link_libraries(thirdeye ${CMAKE_THREAD_LIBS_INIT})
endif()

if(APPLE)
    find_library(CARBON_FRAMEWORK Carbon)
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(IOKIT_FRAMEWORK IOKit)
    target_link_libraries(thirdeye ${CARBON_FRAMEWORK} ${COCOA_FRAMEWORK} ${IOKIT_FRAMEWORK})

    if (FFMPEG_FOUND)
        find_library(COREVIDEO_FRAMEWORK CoreVideo)
        find_library(VDA_FRAMEWORK VideoDecodeAcceleration)
        target_link_libraries(thirdeye ${COREVIDEO_FRAMEWORK} ${VDA_FRAMEWORK})
    endif()
endif(APPLE)

if(DPKG_PROGRAM)
    INSTALL(TARGETS thirdeye RUNTIME DESTINATION games COMPONENT thirdeye)
endif(DPKG_PROGRAM)

if (BUILD_WITH_CODE_COVERAGE)
  add_definitions (--coverage)
  target_link_libraries(thirdeye gcov)
endif()