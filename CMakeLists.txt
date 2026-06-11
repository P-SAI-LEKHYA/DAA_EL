cmake_minimum_required(VERSION 3.12)
project(DNA_Alignment_Visualizer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    main.cpp
    renderer.cpp
)

# Header files for IDE visualization
set(HEADERS
    affine.hpp
    confidence.hpp
    config.hpp
    dp_matrix.hpp
    exporter.hpp
    greedy.hpp
    nw.hpp
    race_mode.hpp
    renderer.hpp
    scoring.hpp
    sequence.hpp
    sw.hpp
)

add_executable(DAA_EL ${SOURCES} ${HEADERS})

# Global definitions to fix Windows-specific issues
target_compile_definitions(DAA_EL PRIVATE NOMINMAX _CRT_SECURE_NO_WARNINGS)

# On Windows MSVC, set compiler warning levels
if(MSVC)
    target_compile_options(DAA_EL PRIVATE /W4)
else()
    target_compile_options(DAA_EL PRIVATE -Wall -Wextra -pedantic)
endif()
