cmake_minimum_required(VERSION 3.0)

project(alsong-lyrics-fetcher VERSION 1.0.0)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

set(CURL_LIBRARY "-lcurl") 
find_package(CURL REQUIRED) 

include_directories(include)
include_directories(${CURL_INCLUDE_DIR})
link_libraries(stdc++fs ${CURL_LIBRARIES})

set(SOURCES
  src/tinyxml2.cpp
  src/main.cpp
)

add_executable(
    ${PROJECT_NAME}
    ${SOURCES}
)


