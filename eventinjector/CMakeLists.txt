cmake_minimum_required(VERSION 3.4.1)

set(INCLUDES  src/main/cpp)
add_library(
        event-injector
        SHARED
        src/main/cpp/native-lib.cpp
        src/main/cpp/EventInjector.cpp)

find_library(
        log-lib
        log)


target_link_libraries(
        event-injector
        ${log-lib})