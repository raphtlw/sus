cmake_minimum_required(VERSION 3.0.0)
project(sus VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 14)

include(CTest)
enable_testing()

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
find_package(X11 REQUIRED)

include_directories(/usr/include/glib-2.0/)
include_directories(/usr/lib/glib-2.0/include/)
include_directories(${GTK3_INCLUDE_DIRS})
include_directories(${X11_INCLUDE_DIR})
link_directories(${GTK3_LIBRARY_DIRS})
link_directories(/usr/include/glib-2.0/)
link_libraries(${X11_LIBRARIES})
add_subdirectory(lib/c-vector/)
add_definitions(${GTK3_CFLAGS_OTHER})

add_executable(${PROJECT_NAME} main.c)
target_link_libraries(${PROJECT_NAME} ${GTK3_LIBRARIES})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
