cmake_minimum_required(VERSION 3.16)
project(shadnet-toolbox LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

if(MSVC)
    add_compile_options(/EHsc /utf-8)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Sql Widgets Network)

add_library(worlds_editor STATIC
    ../common/worlds/worldsconfig.cpp ../common/worlds/worldsconfig.h
    ../common/worlds/worldsstore.cpp ../common/worlds/worldsstore.h
    ../common/worlds/worldsbackend.cpp ../common/worlds/worldsbackend.h
    ../common/worlds/localworldsbackend.cpp ../common/worlds/localworldsbackend.h
    ../common/worlds/worldsdialog.cpp ../common/worlds/worldsdialog.h
)
target_include_directories(worlds_editor PUBLIC ../common/worlds)
target_link_libraries(worlds_editor PUBLIC Qt6::Core Qt6::Widgets Qt6::Network)

add_library(member_store STATIC
    member_store.cpp
    member_store.h
    ../src/database.cpp
    ../src/database.h
)
target_include_directories(member_store PUBLIC . ../src)
target_link_libraries(member_store PUBLIC Qt6::Core Qt6::Sql)

add_executable(shadnet-toolbox WIN32 main.cpp member_window.cpp member_window.h)
target_link_libraries(shadnet-toolbox PRIVATE member_store worlds_editor)

option(MEMBER_TOOL_BUILD_TESTS "Build the shadNet Toolbox tests" OFF)
if(MEMBER_TOOL_BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_executable(member_store_test tests/member_store_test.cpp)
    target_link_libraries(member_store_test PRIVATE member_store Qt6::Test)
    add_test(NAME member_store_test COMMAND member_store_test)

    add_executable(member_window_test tests/member_window_test.cpp member_window.cpp member_window.h)
    target_link_libraries(member_window_test PRIVATE member_store worlds_editor Qt6::Test)
    add_test(NAME member_window_test COMMAND member_window_test -platform offscreen)
    set_tests_properties(member_store_test member_window_test PROPERTIES TIMEOUT 30)
    set_tests_properties(member_window_test PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::QOffscreenIntegrationPlugin>")
    if(WIN32)
        set_property(TEST member_window_test APPEND PROPERTY ENVIRONMENT "QT_QPA_FONTDIR=$ENV{WINDIR}/Fonts")
    endif()
endif()

install(TARGETS shadnet-toolbox RUNTIME DESTINATION bin)
