cmake_minimum_required(VERSION 3.13)
enable_language(CXX)

message(STATUS "Using CMake version ${CMAKE_VERSION}")

set(SRC_FILES
    calendar.cpp)

set(HDR_FILES
    calendar.h)

find_package(Protobuf REQUIRED)
find_package(gflags REQUIRED)

include(FindPkgConfig)
pkg_check_modules(CAIRO pangocairo REQUIRED)
pkg_check_modules(LIBRSVG2 librsvg-2.0 REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS config.proto)

add_executable(bible-reading-calendar "main_cms.cpp" ${SRC_FILES} ${HDR_FILES} ${PROTO_SRCS} ${PROTO_HDRS})
add_executable(cli "main_cli.cpp" ${SRC_FILES} ${HDR_FILES} ${PROTO_SRCS} ${PROTO_HDRS})

target_include_directories(bible-reading-calendar PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CAIRO_INCLUDE_DIRS} ${LIBRSVG2_INCLUDE_DIRS})
target_link_libraries(bible-reading-calendar ${Protobuf_LIBRARIES} ${gflags_LIBRARIES} ${CAIRO_LIBRARIES} ${LIBRSVG2_LIBRARIES} cppcms)

target_include_directories(cli PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CAIRO_INCLUDE_DIRS} ${LIBRSVG2_INCLUDE_DIRS})
target_link_libraries(cli ${Protobuf_LIBRARIES} ${gflags_LIBRARIES} ${CAIRO_LIBRARIES} ${LIBRSVG2_LIBRARIES} cppcms)

project(bible_reading_calendar VERSION 1.0)

install(TARGETS bible-reading-calendar DESTINATION bin)
