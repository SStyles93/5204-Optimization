cmake_minimum_required (VERSION 3.8)

project ("OptimisationProject")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STRANDARD_REQUIRED ON) 

find_package(GTest CONFIG REQUIRED)
find_package(benchmark CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(PNG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(fmt CONFIG REQUIRED)

find_package(OpenMP REQUIRED)

#TRACY
option(ENABLE_PROFILER "Enable Tracy Profiling" OFF)
if(ENABLE_PROFILER)
add_subdirectory(tracy)
endif()


#PROGRAM FILES
file(GLOB_RECURSE SRC_FILES src/*.cpp include/*.hpp)
add_executable(Rasterizer ${SRC_FILES})
target_link_libraries(Rasterizer PUBLIC benchmark::benchmark benchmark::benchmark_main)
target_link_libraries(Rasterizer PUBLIC glm::glm)
target_link_libraries(Rasterizer PUBLIC PNG::PNG)
target_link_libraries(Rasterizer PUBLIC tinyobjloader::tinyobjloader)
target_link_libraries(Rasterizer PUBLIC OpenMP::OpenMP_CXX)
target_link_libraries(Rasterizer PUBLIC fmt::fmt)
if(ENABLE_PROFILER)
target_link_libraries(Rasterizer PUBLIC tracy)
target_compile_definitions(Rasterizer PUBLIC TRACY_ENABLE)
endif()
target_include_directories(Rasterizer PUBLIC ${Stb_INCLUDE_DIR})
target_include_directories(Rasterizer PUBLIC include/)

#TESTS
add_executable(Tests tests/tests.cpp src/Camera.cpp include/Camera.hpp src/Rasterizer.cpp include/Rasterizer.hpp src/Scene.cpp include/Scene.hpp)
target_link_libraries(Tests PUBLIC GTest::gtest GTest::gtest_main)# GTest::gmock GTest::gmock_main)
target_link_libraries(Tests PUBLIC benchmark::benchmark benchmark::benchmark_main)
target_link_libraries(Tests PUBLIC glm::glm)
target_link_libraries(Tests PUBLIC PNG::PNG)
target_link_libraries(Tests PUBLIC tinyobjloader::tinyobjloader)
target_include_directories(Tests PUBLIC ${Stb_INCLUDE_DIR})
target_include_directories(Tests PUBLIC include/)

#ASSETS
file(GLOB_RECURSE ASSET_FILES assets/*)
foreach(ASSET_FILE ${ASSET_FILES})
file(COPY ${ASSET_FILE} DESTINATION ../assets)
endforeach()