cmake_minimum_required(VERSION 3.15)

project(XiaoSu)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

add_executable(XiaoSu src/main.cpp)

target_link_libraries(XiaoSu booster cppcms curl cpr wtdbo wtdbomysql)