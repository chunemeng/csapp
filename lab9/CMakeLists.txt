cmake_minimum_required(VERSION 3.20)
project(lab9)

set(CMAKE_CXX_STANDARD 11)

add_library(lab9lib memlib.h
        memlib.c
        ftimer.h
        ftimer.c
        fsecs.c
        fsecs.h
        fcyc.h
        fcyc.c
        config.h
        clock.h
        clock.c
        mm.h
        mm.c)
add_executable(lab9 mdriver.c)
target_link_libraries(lab9 lab9lib)
