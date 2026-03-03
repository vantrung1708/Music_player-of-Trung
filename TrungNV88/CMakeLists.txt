cmake_minimum_required(VERSION 3.15)
project(S32MediaHub LANGUAGES CXX)

option(BUILD_TESTS "Build unit tests" ON)
option(ENABLE_QT "Build Qt UI" ON)
option(ENABLE_HARDWARE "Enable hardware serial" OFF)
option(ENABLE_PLUGINS "Enable plugins" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(Threads REQUIRED)
find_package(SDL2 REQUIRED)
find_package(TagLib REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)
find_package(FFmpeg REQUIRED COMPONENTS AVCODEC AVFORMAT AVUTIL SWRESAMPLE SWSCALE)

# SDL2 target fallback
if (NOT TARGET SDL2::SDL2)
  if (SDL2_LIBRARIES AND SDL2_INCLUDE_DIRS)
    add_library(SDL2::SDL2 INTERFACE IMPORTED)
    set_target_properties(SDL2::SDL2 PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS}"
      INTERFACE_LINK_LIBRARIES "${SDL2_LIBRARIES}"
    )
  endif()
endif()

# TagLib target fallback
if (NOT TARGET TagLib::TagLib)
  if (TAGLIB_LIBRARIES AND TAGLIB_INCLUDE_DIRS)
    add_library(TagLib::TagLib INTERFACE IMPORTED)
    set_target_properties(TagLib::TagLib PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${TAGLIB_INCLUDE_DIRS}"
      INTERFACE_LINK_LIBRARIES "${TAGLIB_LIBRARIES}"
    )
  endif()
endif()

# spdlog target fallback
if (NOT TARGET spdlog::spdlog)
  if (TARGET spdlog::spdlog_header_only)
    add_library(spdlog::spdlog ALIAS spdlog::spdlog_header_only)
  endif()
endif()

add_library(s32_core
  src/playback/AudioEngine.cpp
  src/playback/VideoEngine.cpp
  src/model/LibraryScanner.cpp
  src/model/MetadataExtractor.cpp
  src/model/PlaylistManager.cpp
  src/model/MockSerialTransceiver.cpp
  src/model/SerialTransceiver.cpp
  src/utils/Config.cpp
)

target_include_directories(s32_core PUBLIC include)
target_link_libraries(s32_core
  PUBLIC
    Threads::Threads
    SDL2::SDL2
    TagLib::TagLib
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    ${FFMPEG_LIBRARIES}
)

target_include_directories(s32_core PRIVATE ${FFMPEG_INCLUDE_DIRS})

if (ENABLE_QT)
  find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

  add_executable(s32_app
    src/main.cpp
    src/controller/PlaybackController.cpp
    src/controller/LibraryController.cpp
    src/controller/HardwareController.cpp
    src/controller/SettingsController.cpp
    src/controller/VideoController.cpp
    src/view/VideoFrameProvider.cpp
    src/view/qml.qrc
  )

  target_link_libraries(s32_app
    PRIVATE
      s32_core
      Qt6::Core
      Qt6::Gui
      Qt6::Qml
      Qt6::Quick
  )

  target_include_directories(s32_app PRIVATE src)

  target_compile_definitions(s32_app PRIVATE S32_ENABLE_QT=1)
endif()

if (ENABLE_HARDWARE)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SERIALPORT libserialport)
  if (SERIALPORT_FOUND)
    target_include_directories(s32_core PRIVATE ${SERIALPORT_INCLUDE_DIRS})
    target_link_libraries(s32_core PUBLIC ${SERIALPORT_LIBRARIES})
    target_compile_definitions(s32_core PRIVATE S32_HAVE_SERIALPORT=1)
    if (TARGET s32_app)
      target_compile_definitions(s32_app PRIVATE S32_ENABLE_HARDWARE=1)
    endif()
  else()
    message(WARNING "libserialport not found; hardware disabled")
  endif()
endif()

if (BUILD_TESTS)
  find_package(GTest)
  if (GTest_FOUND AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/tests/playlist_tests.cpp")
    enable_testing()
    add_executable(s32_tests tests/playlist_tests.cpp)
    target_link_libraries(s32_tests PRIVATE s32_core GTest::gtest GTest::gtest_main)
    add_test(NAME s32_tests COMMAND s32_tests)
  else()
    message(STATUS "Tests skipped: GTest not found or tests/playlist_tests.cpp missing")
  endif()
endif()
