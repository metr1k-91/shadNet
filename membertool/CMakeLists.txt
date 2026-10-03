cmake_minimum_required(VERSION 3.16)
project(shadnet-member-manager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

if(MSVC)
    add_compile_options(/EHsc /utf-8)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Sql Widgets)

add_library(member_store STATIC
    member_store.cpp
    member_store.h
    ../src/database.cpp
    ../src/database.h
)
target_include_directories(member_store PUBLIC . ../src)
target_link_libraries(member_store PUBLIC Qt6::Core Qt6::Sql)

add_executable(shadnet-member-manager WIN32 main.cpp member_window.cpp member_window.h)
target_link_libraries(shadnet-member-manager PRIVATE member_store Qt6::Widgets)

option(MEMBER_TOOL_BUILD_TESTS "Build the member manager tests" OFF)
if(MEMBER_TOOL_BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_executable(member_store_test tests/member_store_test.cpp)
    target_link_libraries(member_store_test PRIVATE member_store Qt6::Test)
    add_test(NAME member_store_test COMMAND member_store_test)

    add_executable(member_window_test tests/member_window_test.cpp member_window.cpp member_window.h)
    target_link_libraries(member_window_test PRIVATE member_store Qt6::Widgets Qt6::Test)
    add_test(NAME member_window_test COMMAND member_window_test -platform offscreen)
endif()

install(TARGETS shadnet-member-manager RUNTIME DESTINATION bin)
