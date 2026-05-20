cmake_minimum_required(VERSION 3.10)

project(moonbeam)

# Source files
set(MOONBEAM_SOURCES
    src/moonbeam.cpp
)

# Header files
set(MOONBEAM_HEADERS
    include/moonbeam.hpp
)

# Add static library
add_library(moonbeam STATIC
    ${MOONBEAM_SOURCES}
    ${MOONBEAM_HEADERS}
)

# Include directories
target_include_directories(moonbeam PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# Platform specific configurations
if(WIN32)
    # Windows requires linking against WinSock / curl
    target_link_libraries(moonbeam PRIVATE ws2_32)
elseif(PLATFORM_PSV)
    # PS Vita specific macros or stubs
    target_compile_definitions(moonbeam PRIVATE __vita__ __PSV__)
endif()

# Enable C++17 standard
target_compile_features(moonbeam PUBLIC cxx_std_17)

# Copy lang directory to the main project's resources directory so libromfs embeds it
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/lang" DESTINATION "${CMAKE_SOURCE_DIR}/resources/moonbeam")

