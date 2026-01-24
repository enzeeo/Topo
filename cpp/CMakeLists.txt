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

# OpenMP (optional parallelization)
find_package(OpenMP)

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
)

target_link_libraries(msi_core PUBLIC
    Eigen3::Eigen
    Arrow::arrow_shared
    Parquet::parquet_shared
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(msi_core PUBLIC OpenMP::OpenMP_CXX)
endif()

# -----------------------------------------------------------------------------
# Executable: daily_run
# -----------------------------------------------------------------------------

add_executable(daily_run apps/daily_run.cpp)

target_link_libraries(daily_run PRIVATE msi_core)

# -----------------------------------------------------------------------------
# Install
# -----------------------------------------------------------------------------

install(TARGETS daily_run DESTINATION bin)
