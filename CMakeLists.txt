project(Thirdeye)

# We probably support older versions than this.
cmake_minimum_required(VERSION 2.6)

# Macros
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/)
include (ThirdeyeMacros)

# Set a default build type if none was specified
IF(NOT DEFINED CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
  MESSAGE(STATUS "Setting build type to 'Debug' as none was specified.")
  SET(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  SET_PROPERTY(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
ENDIF()
SET(CMAKE_CONFIGURATION_TYPES "${CMAKE_BUILD_TYPE}")
MESSAGE(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")

# Platform specific defines
IF(UNIX)
    # allow 'large' files in 32 bit builds
    ADD_DEFINITIONS(
        -D_LARGEFILE_SOURCE
        -D_FILE_OFFSET_BITS=64
        -D_LARGE_FILES
        )
ENDIF()

# Version
set (THIRDEYE_VERSION_MAJOR 0)
set (THIRDEYE_VERSION_MINOR 86)
set (THIRDEYE_VERSION_RELEASE 0)

set (THIRDEYE_VERSION "${THIRDEYE_VERSION_MAJOR}.${THIRDEYE_VERSION_MINOR}.${THIRDEYE_VERSION_RELEASE}")

# Doxygen main page
configure_file ("${Thirdeye_SOURCE_DIR}/docs/mainpage.hpp.cmake" "${Thirdeye_SOURCE_DIR}/docs/mainpage.hpp")

# Are we building a debug build?
option(DEBUG "Use debug build options" FALSE)

# Are we running unit tests?
option(UNIT_TESTS "Run unit tests" FALSE)

# Are we building statically?
option(BOOST_STATIC "Link static build of Boost into the binaries" FALSE)
option(SDL2_STATIC "Link static build of SDL into the binaries" FALSE)

# Apps and tools
option(BUILD_DAESOP "Build Daesop" ON)
option(BUILD_LAUNCHER "Build Thirdeye Launcher" OFF)
option(BUILD_ARC "Build ARC" OFF)

# Source directory: libs
set(LIBDIR ${CMAKE_SOURCE_DIR}/libs)

# Platform specific variables
## Location of game data files
if(DPKG_PROGRAM)
    set(GAME_DATA_FILES "/usr/share/games/thirdeye/data/" CACHE PATH "Location of Thirdeye data files")
else()
    if (APPLE)
        set(GAME_DATA_FILES "./data" CACHE PATH "Location of Thirdeye data files")
    else()
        set(GAME_DATA_FILES "data" CACHE PATH "Location of Thirdeye data files")
    endif(APPLE)
endif(DPKG_PROGRAM)

if (WIN32)
    option(USE_DEBUG_CONSOLE "whether debug goes to console or to Visual Studio output" ON)
endif()

IF(WIN32)
    ADD_DEFINITIONS(
        -DWIN32_LEAN_AND_MEAN
        -D_CRT_SECURE_NO_WARNINGS
        )
ENDIF()

if (WIN32)
    set(PLATFORM_INCLUDE_DIR "platform")
    add_definitions(-DBOOST_ALL_NO_LIB)
else (WIN32)
    set(PLATFORM_INCLUDE_DIR "")
    find_path (UUID_INCLUDE_DIR uuid/uuid.h)
    include_directories(${UUID_INCLUDE_DIR})
endif (WIN32)

if (MSVC10)
    set(PLATFORM_INCLUDE_DIR "")
endif()

## Fix for not visible pthreads functions for linker with glibc 2.15
if (UNIX AND NOT APPLE)
    find_package (Threads)
endif()

## find boost without components so we can use Boost_VERSION
find_package(Boost REQUIRED)
set(BOOST_COMPONENTS system filesystem program_options thread)
find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})

include_directories("."
    ${PLATFORM_INCLUDE_DIR}
    ${SDL2_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
    ${UUID_INCLUDE_DIR}
    ${OPENAL_INCLUDE_DIR}
    ${LIBDIR}
)

link_directories(${Boost_LIBRARY_DIRS} ${Boost_LIBRARY_DIRS} )

if (UNIT_TESTS)
    # Locate GTest
    find_package(GTest REQUIRED)
    include_directories(${GTEST_INCLUDE_DIRS})
endif()

# Find SDL2
Find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})

# Find OpenAL
find_package(OpenAL REQUIRED)
include_directories(${OPENAL_INCLUDE_DIR})

# Find WildMIDI
Find_package(WILDMIDI REQUIRED)
include_directories(${WILDMIDI_INCLUDE_DIR})

if (APPLE)
    configure_file(${Thirdeye_SOURCE_DIR}/files/mac/Info.plist
        "${APP_BUNDLE_DIR}/Contents/Info.plist")

    configure_file(${Thirdeye_SOURCE_DIR}/files/mac/THIRDEYE.icns
        "${APP_BUNDLE_DIR}/Contents/Resources/THIRDEYE.icns" COPYONLY)
endif (APPLE)

# Set up DEBUG define
set_directory_properties(PROPERTIES COMPILE_DEFINITIONS_DEBUG DEBUG=1)

# Specify build paths
if (APPLE)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${APP_BUNDLE_DIR}/Contents/MacOS")
else (APPLE)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${Thirdeye_BINARY_DIR}")
endif (APPLE)

if (NOT WIN32 AND NOT APPLE)
    configure_file(${Thirdeye_SOURCE_DIR}/files/thirdeye.desktop
        "${Thirdeye_BINARY_DIR}/thirdeye.desktop")
endif()

# Compiler settings
if (CMAKE_COMPILER_IS_GNUCCXX)
    add_definitions (-Wall -Wextra -Wno-unused-parameter -Wno-reorder -std=c++11 -pedantic -Wno-long-long)

    # Set our debugging standards
    if(DEBUG)
        add_definitions (-ggdb3 -O0)
    endif(DEBUG)

    execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion
                OUTPUT_VARIABLE GCC_VERSION)
    if ("${GCC_VERSION}" VERSION_GREATER 4.6 OR "${GCC_VERSION}" VERSION_EQUAL 4.6)
        add_definitions (-Wno-unused-but-set-parameter)
    endif("${GCC_VERSION}" VERSION_GREATER 4.6 OR "${GCC_VERSION}" VERSION_EQUAL 4.6)
endif (CMAKE_COMPILER_IS_GNUCCXX)

if(DPKG_PROGRAM)
    SET(CMAKE_INSTALL_PREFIX "/usr")

    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.git")
        exec_program("git" ${CMAKE_CURRENT_SOURCE_DIR} ARGS "describe" OUTPUT_VARIABLE GIT_VERSION )
        STRING(REGEX REPLACE "THIRDEYE-" "" VERSION_STRING "${GIT_VERSION}")
        exec_program("git" ARGS "config --get user.name" OUTPUT_VARIABLE GIT_NAME )
        exec_program("git" ARGS "config --get user.email" OUTPUT_VARIABLE GIT_EMAIL)
        set(PACKAGE_MAINTAINER "${GIT_NAME} <${GIT_EMAIL}>")
    else()
        set(VERSION_STRING "${THIRDEYE_VERSION}")
        set(PACKAGE_MAINTAINER "unknown")
    endif()

    #Install icon and desktop file
    INSTALL(FILES "${Thirdeye_BINARY_DIR}/thirdeye.desktop" DESTINATION "share/applications/" PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ COMPONENT "thirdeye")
    INSTALL(FILES "${Thirdeye_SOURCE_DIR}/apps/launcher/resources/images/thirdeye.png" DESTINATION "share/pixmaps/" PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ COMPONENT "thirdeye")

    #Install resources
    INSTALL(DIRECTORY "${Thirdeye_BINARY_DIR}/resources" DESTINATION "share/games/thirdeye/" FILE_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ COMPONENT "Resources")
    INSTALL(DIRECTORY DESTINATION "share/games/thirdeye/data/" COMPONENT "Resources")

    SET(CPACK_GENERATOR "DEB")
    SET(CPACK_PACKAGE_NAME "Thirdeye")
    SET(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://www.mindwerks.net")
    SET(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "${PACKAGE_MAINTAINER}")
    SET(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Thirdeye is an attempt at recreating the engine for the popular role-playing games produced by Strategic Simulations, Inc. You need to own and install the original games for Thirdeye to work.")
    SET(CPACK_DEBIAN_PACKAGE_NAME "Thirdeye")
    SET(CPACK_DEBIAN_PACKAGE_VERSION "${VERSION_STRING}")
    SET(CPACK_PACKAGE_EXECUTABLES "thirdeye;Thirdeye thirdeyelauncher;ThirdeyeLauncher daesop;Daesop;runtests;RunTests")
    SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.11.2), libstdc++6 (>= 4.4.5), libuuid1 (>= 2.17.2)")
    SET(CPACK_DEBIAN_PACKAGE_SECTION "Games")
    string(TOLOWER "${CPACK_PACKAGE_NAME}" CPACK_PACKAGE_NAME_LOWERCASE)
    execute_process(
        COMMAND ${DPKG_PROGRAM} --print-architecture
        OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME_LOWERCASE}_${CPACK_DEBIAN_PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")


    include(CPack)
endif(DPKG_PROGRAM)

if(WIN32)
    FILE(GLOB dll_files "${Thirdeye_BINARY_DIR}/Release/*.dll")
    INSTALL(FILES ${dll_files} DESTINATION ".")
    INSTALL(FILES
        "${Thirdeye_SOURCE_DIR}/README.md"
        "${Thirdeye_SOURCE_DIR}/GPL3.txt"
        "${Thirdeye_BINARY_DIR}/Release/thirdeyelauncher.exe"
        "${Thirdeye_BINARY_DIR}/Release/thirdeye.exe"
        "${Thirdeye_BINARY_DIR}/Release/daesop.exe"
        "${Thirdeye_BINARY_DIR}/Release/runtests.exe"
        DESTINATION ".")
    INSTALL(DIRECTORY "${Thirdeye_BINARY_DIR}/resources" DESTINATION ".")

    SET(CPACK_GENERATOR "NSIS")
    SET(CPACK_PACKAGE_NAME "Thirdeye")
    SET(CPACK_PACKAGE_VENDOR "Mindwerks.net")
    SET(CPACK_PACKAGE_VERSION ${THIRDEYE_VERSION})
    SET(CPACK_PACKAGE_VERSION_MAJOR ${THIRDEYE_VERSION_MAJOR})
    SET(CPACK_PACKAGE_VERSION_MINOR ${THIRDEYE_VERSION_MINOR})
    SET(CPACK_PACKAGE_VERSION_PATCH ${THIRDEYE_VERSION_RELEASE})
    SET(CPACK_PACKAGE_EXECUTABLES "thirdeye;Thirdeye;thirdeyelauncher;Thirdeye Launcher;daesop;Daesop;runtests;RunTests")
    SET(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '\$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Readme.lnk' '\$INSTDIR\\\\README.md'")
    SET(CPACK_NSIS_DELETE_ICONS_EXTRA "
        !insertmacro MUI_STARTMENU_GETFOLDER Application $MUI_TEMP
        Delete \\\"$SMPROGRAMS\\\\$MUI_TEMP\\\\Readme.lnk\\\"
        ")
    SET(CPACK_RESOURCE_FILE_README "${Thirdeye_SOURCE_DIR}/README.md")
    SET(CPACK_PACKAGE_DESCRIPTION_FILE "${Thirdeye_SOURCE_DIR}/README.md")
    SET(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
    SET(CPACK_NSIS_DISPLAY_NAME "Thirdeye ${THIRDEYE_VERSION}")
    SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\www.mindwerks.net")
    SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.mindwerks.net")
    SET(CPACK_NSIS_INSTALLED_ICON_NAME "thirdeyelauncher.exe")
    SET(CPACK_NSIS_MUI_ICON "${Thirdeye_SOURCE_DIR}/apps/launcher/resources/images/thirdeye.ico")
    SET(CPACK_NSIS_MUI_UNIICON "${Thirdeye_SOURCE_DIR}/apps/launcher/resources/images/thirdeye.ico")
    SET(CPACK_PACKAGE_ICON "${Thirdeye_SOURCE_DIR}\\\\files\\\\thirdeye.bmp")

    SET(VCREDIST32 "${Thirdeye_BINARY_DIR}/vcredist_x86.exe")
    if(EXISTS ${VCREDIST32})
        INSTALL(FILES ${VCREDIST32} DESTINATION "redist")
        SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "ExecWait '\\\"$INSTDIR\\\\redist\\\\vcredist_x86.exe\\\" /q'" )
    endif(EXISTS ${VCREDIST32})

    SET(VCREDIST64 "${Thirdeye_BINARY_DIR}/vcredist_x64.exe")
    if(EXISTS ${VCREDIST64})
        INSTALL(FILES ${VCREDIST64} DESTINATION "redist")
        SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "ExecWait '\\\"$INSTDIR\\\\redist\\\\vcredist_x64.exe\\\" /q'" )
    endif(EXISTS ${VCREDIST64})

    if(CMAKE_CL_64)
        SET(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
    endif()

    include(CPack)
endif(WIN32)

# Components
add_subdirectory (components)

# Apps and tools
add_subdirectory( apps/thirdeye )
if (BUILD_LAUNCHER)
   add_subdirectory( apps/launcher )
endif()
if (BUILD_DAESOP)
   add_subdirectory( apps/daesop )
endif()
if (BUILD_ARC)
   add_subdirectory( apps/arc )
endif()
if (UNIT_TESTS)
   add_subdirectory( apps/tests )
endif()

if (WIN32)
  if (MSVC)
    if (USE_DEBUG_CONSOLE)
      set_target_properties(thirdeye PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
      set_target_properties(thirdeye PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
      set_target_properties(thirdeye PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE")
    else()
      # Turn off debug console, debug output will be written to visual studio output instead
      set_target_properties(thirdeye PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:WINDOWS")
      set_target_properties(thirdeye PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS")
    endif()

    # Release builds use the debug console
    set_target_properties(thirdeye PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:CONSOLE")
    set_target_properties(thirdeye PROPERTIES COMPILE_DEFINITIONS_RELEASE "_CONSOLE")
    set_target_properties(thirdeye PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:CONSOLE")

    # Play a bit with the warning levels

    set(WARNINGS "/Wall") # Since windows can only disable specific warnings, not enable them

    set(WARNINGS_DISABLE
        # Warnings that aren't enabled normally and don't need to be enabled
        # They're unneeded and sometimes completely retarded warnings that /Wall enables
        # Not going to bother commenting them as they tend to warn on every standard library files
        4061 4263 4264 4266 4350 4371 4514 4548 4571 4610 4619 4623 4625 4626 4628 4640 4668 4710 4711 4820 4826 4917 4946

        # Warnings that are thrown on standard libraries and not thirdeye
        4347 # Non-template function with same name and parameter count as template function
        4365 # Variable signed/unsigned mismatch
        4510 4512 # Unable to generate copy constructor/assignment operator as it's not public in the base
        4706 # Assignment in conditional expression
        4738 # Storing 32-bit float result in memory, possible loss of performance
        4986 # Undocumented warning that occurs in the crtdbg.h file
        4996 # Function was declared deprecated

		# cause by ogre extensivly
		4193 # #pragma warning(pop) : no matching '#pragma warning(push)'
		4251 # class 'XXXX' needs to have dll-interface to be used by clients of class 'YYYY'
		4275 # non dll-interface struct 'XXXX' used as base for dll-interface class 'YYYY'

        # thirdeye specific warnings
        4099 # Type mismatch, declared class or struct is defined with other type
        4100 # Unreferenced formal parameter (-Wunused-parameter)
        4127 # Conditional expression is constant
        4242 # Storing value in a variable of a smaller type, possible loss of data
        4244 # Storing value of one type in variable of another (size_t in int, for example)
        4305 # Truncating value (double to float, for example)
        4309 # Variable overflow, trying to store 128 in a signed char for example
        4355 # Using 'this' in member initialization list
        4701 # Potentially uninitialized local variable used
		4800 # Boolean optimization warning, e.g. myBool = (myInt != 0) instead of myBool = myInt
        )

    foreach(d ${WARNINGS_DISABLE})
        set(WARNINGS "${WARNINGS} /wd${d}")
    endforeach(d)

    set_target_properties(components PROPERTIES COMPILE_FLAGS ${WARNINGS})
    if (BUILD_LAUNCHER)
		set_target_properties(thirdeyelauncher PROPERTIES COMPILE_FLAGS ${WARNINGS})
	endif (BUILD_LAUNCHER)
    if (BUILD_DAESOP)
        set_target_properties(daesop PROPERTIES COMPILE_FLAGS ${WARNINGS})
    endif (BUILD_DAESOP)
    if (UNIT_TESTS)
        set_target_properties(runtests PROPERTIES COMPILE_FLAGS ${WARNINGS})
    endif(UNIT_TESTS)
    set_target_properties(thirdeye PROPERTIES COMPILE_FLAGS ${WARNINGS})
  endif(MSVC)

  # Same for MinGW
  if (MINGW)
    if (USE_DEBUG_CONSOLE)
      set_target_properties(thirdeye PROPERTIES LINK_FLAGS_DEBUG "-Wl,-subsystem,console")
      set_target_properties(thirdeye PROPERTIES LINK_FLAGS_RELWITHDEBINFO "-Wl,-subsystem,console")
      set_target_properties(thirdeye PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE")
    else(USE_DEBUG_CONSOLE)
      set_target_properties(thirdeye PROPERTIES LINK_FLAGS_DEBUG "-Wl,-subsystem,windows")
      set_target_properties(thirdeye PROPERTIES LINK_FLAGS_RELWITHDEBINFO "-Wl,-subsystem,windows")
    endif(USE_DEBUG_CONSOLE)

    set_target_properties(thirdeye PROPERTIES LINK_FLAGS_RELEASE "-Wl,-subsystem,console")
    set_target_properties(thirdeye PROPERTIES LINK_FLAGS_MINSIZEREL "-Wl,-subsystem,console")
    set_target_properties(thirdeye PROPERTIES COMPILE_DEFINITIONS_RELEASE "_CONSOLE")
  endif(MINGW)

  # TODO: At some point release builds should not use the console but rather write to a log file
  #set_target_properties(thirdeye PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
  #set_target_properties(thirdeye PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS")
endif()

# Apple bundling
if (APPLE)
    set(INSTALL_SUBDIR THIRDEYE)

    install(DIRECTORY "${APP_BUNDLE_DIR}" USE_SOURCE_PERMISSIONS DESTINATION "${INSTALL_SUBDIR}" COMPONENT Runtime)
    install(DIRECTORY "${Thirdeye_BINARY_DIR}/resources" DESTINATION "${INSTALL_SUBDIR}" COMPONENT Runtime)

    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_PACKAGE_VERSION ${THIRDEYE_VERSION})
    set(CPACK_PACKAGE_VERSION_MAJOR ${THIRDEYE_VERSION_MAJOR})
    set(CPACK_PACKAGE_VERSION_MINOR ${THIRDEYE_VERSION_MINOR})
    set(CPACK_PACKAGE_VERSION_PATCH ${THIRDEYE_VERSION_RELEASE})
    set(APPS "\${CMAKE_INSTALL_PREFIX}/${INSTALL_SUBDIR}/${APP_BUNDLE_NAME}")
    set(PLUGINS "")
    set(ABSOLUTE_PLUGINS "")

    foreach (PLUGIN ${USED_OGRE_PLUGINS})
        get_filename_component(PLUGIN_ABS ${PLUGIN} REALPATH)
        set(ABSOLUTE_PLUGINS ${PLUGIN_ABS} ${ABSOLUTE_PLUGINS})
    endforeach ()

    set(PLUGIN_INSTALL_BASE "\${CMAKE_INSTALL_PREFIX}/${INSTALL_SUBDIR}/${APP_BUNDLE_NAME}/Contents/Plugins")
    install(FILES ${ABSOLUTE_PLUGINS} DESTINATION "${INSTALL_SUBDIR}/${APP_BUNDLE_NAME}/Contents/Plugins" COMPONENT Runtime)
    foreach (PLUGIN ${ABSOLUTE_PLUGINS})
        get_filename_component(PLUGIN_RELATIVE ${PLUGIN} NAME)
        set(PLUGINS ${PLUGINS} "${PLUGIN_INSTALL_BASE}/${PLUGIN_RELATIVE}")
    endforeach ()

    #For now, search unresolved dependencies only in default system paths, so if you put unresolveable (i.e. with @executable_path in id name) lib or framework somewhere else, it would fail
    set(DIRS "")

    # Overriding item resolving during installation, it needed if
    # some library already has been "fixed up", i.e. its id name contains @executable_path,
    # but library is not embedded in bundle. For example, it's Ogre.framework from Ogre SDK.
    # Current implementation of GetPrerequsities/BundleUtilities doesn't handle that case.
    #
    # Current limitations:
    #   1. Handles only frameworks, not simple libs
    INSTALL(CODE "
        set(CMAKE_FIND_LIBRARY_PREFIXES ${CMAKE_FIND_LIBRARY_PREFIXES})
        set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
        set(CMAKE_SYSTEM_FRAMEWORK_PATH ${CMAKE_SYSTEM_FRAMEWORK_PATH})

        set(THIRDEYE_RESOLVED_ITEMS \"\")

        function(gp_resolve_item_override context item exepath dirs resolved_item_var resolved_var)
            if(item MATCHES \"@executable_path\" AND NOT \${\${resolved_var}})
                if (item MATCHES \"Frameworks\") # if it is a framework
                    # get last segment of path
                    get_filename_component(fname \"\${item}\" NAME_WE)
                    find_library(ri NAMES \${fname} PATHS \${exepath} \${dirs} \${CMAKE_SYSTEM_FRAMEWORK_PATH})
                    if (ri)
                        string(REGEX REPLACE \"^.*/Frameworks/.*\\\\.framework\" \"\" item_part \${item})
                        set(ri \"\${ri}\${item_part}\")
                        set(\${resolved_item_var} \${ri} PARENT_SCOPE)
                        set(\${resolved_var} 1 PARENT_SCOPE)
                    endif()
                else()
                    # code path for standard (non-framework) libs (ogre & qt pugins)
                    get_filename_component(fname \"\${item}\" NAME_WE)
                    string(REGEX REPLACE \"^lib\" \"\" fname \${fname})
                    find_library(ri NAMES \${fname} PATHS \${exepath} \${dirs} /usr/lib /usr/local/lib)
                    if (ri)
                        set(\${resolved_item_var} \${ri} PARENT_SCOPE)
                        set(\${resolved_var} 1 PARENT_SCOPE)
                    endif ()
                endif()
            endif()
        endfunction(gp_resolve_item_override)

        cmake_policy(SET CMP0009 OLD)
        set(BU_CHMOD_BUNDLE_ITEMS ON)
        include(BundleUtilities)
        fixup_bundle(\"${APPS}\" \"${PLUGINS}\" \"${DIRS}\")
        " COMPONENT Runtime)
        include(CPack)
endif (APPLE)

if (NOT WIN32 AND NOT DPKG_PROGRAM AND NOT APPLE)
    ## Non Debian based Linux building
    # paths
    set(BINDIR "${CMAKE_INSTALL_PREFIX}/usr/bin" CACHE PATH "Where to install binaries")
    set(DATAROOTDIR "${CMAKE_INSTALL_PREFIX}/share" CACHE PATH "Sets the root of data directories to a non-default location")
    set(DATADIR "${DATAROOTDIR}/thirdeye" CACHE PATH "Sets the thirdeye data directories to a non-default location")
    set(DOCDIR "${DATAROOTDIR}/doc/thirdeye" CACHE PATH "Sets the doc directory to a non-default location.")
    set(MANDIR "${DATAROOTDIR}/man" CACHE PATH "Where to install manpages")
    set(SYSCONFDIR "${CMAKE_INSTALL_PREFIX}/etc/thirdeye" CACHE PATH "Set config dir")
    set(ICONDIR "${DATAROOTDIR}/pixmaps" CACHE PATH "Set icon dir")

    # Install binaries
    INSTALL(PROGRAMS "${Thirdeye_BINARY_DIR}/thirdeye" DESTINATION "${BINDIR}" )
    IF(BUILD_LAUNCHER)
        INSTALL(PROGRAMS "${Thirdeye_BINARY_DIR}/thirdeyelauncher" DESTINATION "${BINDIR}" )
    ENDIF(BUILD_LAUNCHER)
    IF(BUILD_DAESOP)
        INSTALL(PROGRAMS "${Thirdeye_BINARY_DIR}/daesop" DESTINATION "${BINDIR}" )
    ENDIF(BUILD_DAESOP)  
    if(UNIT_TESTS)
        INSTALL(PROGRAMS "${Thirdeye_BINARY_DIR}/runtests" DESTINATION "${BINDIR}" )
    ENDIF(UNIT_TESTS)  
    IF(BUILD_ARC)
        INSTALL(PROGRAMS "${Thirdeye_BINARY_DIR}/arc" DESTINATION "${BINDIR}" )
    ENDIF(BUILD_ARC) 

    # Install icon and .desktop
    INSTALL(FILES "${Thirdeye_SOURCE_DIR}/apps/launcher/resources/images/thirdeye.png" DESTINATION "${ICONDIR}")
    INSTALL(FILES "${Thirdeye_BINARY_DIR}/thirdeye.desktop" DESTINATION "${DATAROOTDIR}/applications")

    # Install resources
    INSTALL(DIRECTORY "${Thirdeye_BINARY_DIR}/resources" DESTINATION "${DATADIR}" )
    IF(BUILD_LAUNCHER)
        INSTALL(FILES "${Thirdeye_BINARY_DIR}/launcher.qss" DESTINATION "${DATADIR}/resources" )
    ENDIF(BUILD_LAUNCHER)
endif(NOT WIN32 AND NOT DPKG_PROGRAM AND NOT APPLE)
