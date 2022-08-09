cmake_minimum_required(VERSION 2.8)
project(ssr_acq)

######################### configures here to different types of executable ##########################
INCLUDE_DIRECTORIES("${PROJECT_BINARY_DIR}")
INCLUDE_DIRECTORIES("${PROJECT_SOURCE_DIR}")
set(CMAKE_CXX_STANDARD 11)


############### LINK COMMANDS ################
FILE(GLOB SOURCE_RTKLIB ${PROJECT_SOURCE_DIR}/src/Rtklib/*.cpp)
set(SOURCE_ALL main.cpp ${SOURCE_RTKLIB} )


set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall  -ggdb")
add_executable(ssr_acq ${SOURCE_ALL})
target_link_libraries(ssr_acq ${bbo_libs})
target_link_libraries(ssr_acq pthread)
