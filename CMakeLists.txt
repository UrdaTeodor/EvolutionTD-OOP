cmake_minimum_required(VERSION 3.26)

# NOTE: update executable name in .github/workflows/cmake.yml:25 when changing executable name in this file
# for now, the project name is used as the executable name
set(MAIN_PROJECT_NAME "oop")
set(MAIN_EXECUTABLE_NAME "${MAIN_PROJECT_NAME}")


project(${MAIN_PROJECT_NAME})
# set(CMAKE_PROJECT_VERSION_MAJOR 0)
# set(CMAKE_PROJECT_VERSION_MINOR 0)
# set(CMAKE_PROJECT_VERSION_PATCH 1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/Options.cmake)
include(cmake/CompilerFlags.cmake)
include(cmake/CopyHelper.cmake)

###############################################################################

# external dependencies with FetchContent
include(FetchContent)

set(FETCHCONTENT_QUIET OFF CACHE INTERNAL "Override verbose output" FORCE)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE INTERNAL "Override update strategy" FORCE)

# SFML 2.6.x pentru rendering + audio (v0.4: SFX + muzica generata procedural).
set(SFML_BUILD_AUDIO   ON  CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
# Static linking ca sa nu avem nevoie sa copiem DLL-uri.
set(BUILD_SHARED_LIBS  OFF CACHE BOOL "" FORCE)

FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        2.6.1
    GIT_SHALLOW    1
    GIT_PROGRESS   ON
    SYSTEM
)

FetchContent_MakeAvailable(SFML)

# nlohmann/json 3.11.3 (header-only) pentru parsare fisiere de date JSON (T3).
# JSON_BuildTests off ca sa nu compileze test suite-ul lor.
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    1
    GIT_PROGRESS   ON
    SYSTEM
)

FetchContent_MakeAvailable(json)

###############################################################################

# external dependencies with find_package

# find_package(Threads REQUIRED)

###############################################################################

# Logica de joc pura (fara SFML) - refolosita de executabilul principal si de
# balance_sim (simulator headless pentru tunarea dificultatii).
set(GAME_CORE_SOURCES
    src/Enemy.cpp
    src/Tower.cpp
    src/ProjectileTower.cpp
    src/AntivirusTower.cpp
    src/AdblockerTower.cpp
    src/HoneypotTower.cpp
    src/FirewallTower.cpp
    src/BytecoinMinerTower.cpp
    src/Wave.cpp
    src/Game.cpp
    src/Evolution.cpp
    src/StatEvolution.cpp
    src/AbilityEvolution.cpp
    src/MythicEvolution.cpp
    src/EvolutionFactory.cpp
    src/DataRegistry.cpp
    src/Director.cpp
)

# NOTE: update executable name in .github/workflows/cmake.yml:25 when changing name here
add_executable(${MAIN_EXECUTABLE_NAME}
    main.cpp
)
target_sources(${MAIN_EXECUTABLE_NAME}
    PRIVATE
        ${GAME_CORE_SOURCES}
        src/SceneManager.cpp
        src/AudioManager.cpp
        src/MainMenuScene.cpp
        src/GameScene.cpp
        src/Sprites.cpp
        src/BoardRenderer.cpp
        src/EffectsLayer.cpp
        src/HudPanel.cpp
        src/ShopPanel.cpp
        src/SaveManager.cpp
        src/HighScores.cpp
        src/PauseScene.cpp
        src/GameOverScene.cpp
)

# Simulator de balans headless (nu intra in build-ul default / CI).
add_executable(balance_sim EXCLUDE_FROM_ALL
    tools/balance_sim.cpp
    ${GAME_CORE_SOURCES}
)
target_include_directories(balance_sim PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(balance_sim PRIVATE nlohmann_json::nlohmann_json)

# NOTE: Add all defined targets (e.g. executables, libraries, etc. )
# NOTE: RUN_SANITIZERS is optional, if it's not present it will default to true
set_compiler_flags(RUN_SANITIZERS ${USE_ASAN} TARGET_NAMES ${MAIN_EXECUTABLE_NAME})
# set_compiler_flags(TARGET_NAMES ${MAIN_EXECUTABLE_NAME} ${FOO} ${BAR})
# where ${FOO} and ${BAR} represent additional executables or libraries
# you want to compile with the set compiler flags

###############################################################################

# use SYSTEM so cppcheck and clang-tidy do not report warnings from these directories
target_include_directories(${MAIN_EXECUTABLE_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)
target_include_directories(${MAIN_EXECUTABLE_NAME} SYSTEM PRIVATE
    ext
)
# target_include_directories(${MAIN_EXECUTABLE_NAME} SYSTEM PRIVATE ${<SomeLib>_SOURCE_DIR}/include)
# target_link_directories(${MAIN_EXECUTABLE_NAME} PRIVATE ${<SomeLib>_BINARY_DIR}/lib)
# target_link_libraries(${MAIN_EXECUTABLE_NAME} <SomeLib>)

###############################################################################

# copy binaries to "bin" folder; these are uploaded as artifacts on each release
# DESTINATION_DIR is set as "bin" in cmake/Options.cmake:6
install(TARGETS ${MAIN_EXECUTABLE_NAME} DESTINATION ${DESTINATION_DIR})
if(APPLE)
    install(FILES launcher.command DESTINATION ${DESTINATION_DIR})
endif()


copy_files(FILES tastatura.txt DIRECTORY assets data COPY_TO_DESTINATION TARGET_NAME ${MAIN_EXECUTABLE_NAME})


target_link_libraries(${MAIN_EXECUTABLE_NAME} PRIVATE sfml-graphics sfml-window sfml-audio sfml-system nlohmann_json::nlohmann_json)

# SFML audio pe Windows are nevoie de openal32.dll langa executabil.
if(WIN32)
    add_custom_command(TARGET ${MAIN_EXECUTABLE_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${sfml_SOURCE_DIR}/extlibs/bin/x64/openal32.dll"
            "$<TARGET_FILE_DIR:${MAIN_EXECUTABLE_NAME}>")
    install(FILES "${sfml_SOURCE_DIR}/extlibs/bin/x64/openal32.dll"
            DESTINATION ${DESTINATION_DIR})
endif()
