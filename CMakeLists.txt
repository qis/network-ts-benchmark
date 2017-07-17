cmake_minimum_required(VERSION 3.8.2 FATAL_ERROR)
project(ntsb VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  set(CMAKE_CXX_FLAGS "/permissive- /std:c++latest ${CMAKE_CXX_FLAGS} /utf-8")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /manifestuac:NO /ignore:4099 /ignore:4098")
  add_definitions(-D_UNICODE -DUNICODDE -DWIN32_LEAN_AND_MEAN -DNOMINMAX)
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -D_ATL_SECURE_NO_DEPRECATE)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
  add_definitions(-DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00)
else()
  set(CMAKE_CXX_FLAGS "-O3 ${CMAKE_CXX_FLAGS} -pthread -fno-omit-frame-pointer -fcolor-diagnostics")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra -Werror -Wno-unused -Wno-unused-parameter")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER build)

find_package(asio REQUIRED PATHS third_party/asio)
find_package(compat REQUIRED PATHS third_party/compat)

file(GLOB_RECURSE sources src/*.h src/*.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src PREFIX src FILES ${sources})

add_executable(${PROJECT_NAME}_asio ${sources})
target_link_libraries(${PROJECT_NAME}_asio PRIVATE asio compat)
target_compile_definitions(${PROJECT_NAME}_asio PRIVATE USE_ASIO=1)
target_include_directories(${PROJECT_NAME}_asio PRIVATE ${CMAKE_CURRENT_BINARY_DIR} PUBLIC src)
set_target_properties(${PROJECT_NAME}_asio PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS ${PROJECT_NAME}_asio DESTINATION bin)

add_executable(${PROJECT_NAME}_net ${sources})
target_link_libraries(${PROJECT_NAME}_net PRIVATE compat)
target_compile_definitions(${PROJECT_NAME}_net PRIVATE USE_NET=1)
target_include_directories(${PROJECT_NAME}_net PRIVATE ${CMAKE_CURRENT_BINARY_DIR} PUBLIC src)
set_target_properties(${PROJECT_NAME}_net PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS ${PROJECT_NAME}_net DESTINATION bin)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME}_asio)
