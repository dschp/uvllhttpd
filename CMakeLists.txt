cmake_minimum_required(VERSION 3.13)

project(uvllhttpd
   VERSION 1.0
   LANGUAGES C
   )

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -Wformat=2 -Wno-unused-parameter -Wshadow -Wwrite-strings -Wstrict-prototypes -Wold-style-definition -Wredundant-decls -Wnested-externs -Wmissing-include-dirs -Wjump-misses-init -Wlogical-op")

add_executable(httpd-example example.c)
target_link_libraries(httpd-example
   uvllhttpd
   uv llhttp)

add_library(uvllhttpd STATIC uvllhttpd.c)

add_library(cgreen-uvllhttpd SHARED
   uvllhttpd.cgreen.c
   uvllhttpd.c
   )
target_link_libraries(cgreen-uvllhttpd
   llhttp
   uvllhttpd-realuv
   )

add_library(uvllhttpd-realuv SHARED uvllhttpd.realuv.c)
target_link_libraries(uvllhttpd-realuv uv)
