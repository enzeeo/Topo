cmake_minimum_required(VERSION 3.16)
project(MarketStrainIndex VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------

# Eigen (header-only linear algebra)
find_package(Eigen3 REQUIRED)

# Arrow (Parquet I/O)
find_package(Arrow REQUIRED)
find_package(Parquet REQUIRED)

# OpenMP (optional parallelization - not required)
find_package(OpenMP QUIET)

# -----------------------------------------------------------------------------
# Library: msi_core
# -----------------------------------------------------------------------------

add_library(msi_core STATIC
    src/io_parquet.cpp
    src/returns.cpp
    src/correlation.cpp
    src/graph.cpp
    src/diffusion.cpp
    src/topology.cpp
    src/strain.cpp
)

target_include_directories(msi_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/hera/include
)

target_link_libraries(msi_core PUBLIC
    Eigen3::Eigen
    Arrow::arrow_shared
    Parquet::parquet_shared
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(msi_core PUBLIC OpenMP::OpenMP_CXX)
    message(STATUS "OpenMP found - parallel optimizations enabled")
else()
    message(STATUS "OpenMP not found - building without parallelization (this is OK)")
endif()

# -----------------------------------------------------------------------------
# Executable: daily_run
# -----------------------------------------------------------------------------

add_executable(daily_run apps/daily_run.cpp)

target_link_libraries(daily_run PRIVATE msi_core)

# -----------------------------------------------------------------------------
# Test Executables
# -----------------------------------------------------------------------------

# Test 1: io_parquet
add_executable(test_io_parquet apps/test_io_parquet.cpp)
target_link_libraries(test_io_parquet PRIVATE msi_core)

# Test 2: returns
add_executable(test_returns apps/test_returns.cpp)
target_link_libraries(test_returns PRIVATE msi_core)

# Test 3: correlation
add_executable(test_correlation apps/test_correlation.cpp)
target_link_libraries(test_correlation PRIVATE msi_core)

# Test 4: graph / laplacian / GTV 
add_executable(test_graph apps/test_graph.cpp)
target_link_libraries(test_graph PRIVATE msi_core)

# Test 5: diffusion
add_executable(test_diffusion apps/test_diffusion.cpp)
target_link_libraries(test_diffusion PRIVATE msi_core)

# Test 6: topology
add_executable(test_topology apps/test_topology.cpp)
target_link_libraries(test_topology PRIVATE msi_core)

# -----------------------------------------------------------------------------
# Install
# -----------------------------------------------------------------------------

install(TARGETS daily_run DESTINATION bin)
