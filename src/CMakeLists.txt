cmake_minimum_required(VERSION 3.10)
project(airplay2-lite)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

# Compiler flags for optimization
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -ffunction-sections -fdata-sections")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")

# Check for MIPS architecture
if(CMAKE_SYSTEM_PROCESSOR MATCHES "mips")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mips32r2 -mtune=24kc")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mips32r2 -mtune=24kc")
endif()

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(AVAHI REQUIRED avahi-client avahi-common)
pkg_check_modules(ALSA REQUIRED alsa)
pkg_check_modules(OPENSSL REQUIRED openssl)
pkg_check_modules(DAEMON REQUIRED libdaemon)

# Include directories
include_directories(${AVAHI_INCLUDE_DIRS})
include_directories(${ALSA_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIRS})
include_directories(${DAEMON_INCLUDE_DIRS})

# Source files
set(SOURCES
    main.c
    airplay_server.c
    audio_output.c
    volume_control.c
    playback_control.c
    multiroom.c
    crypto_utils.c
    network_utils.c
)

# Create executable
add_executable(airplay2-lite ${SOURCES})

# Link libraries
target_link_libraries(airplay2-lite
    ${AVAHI_LIBRARIES}
    ${ALSA_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${DAEMON_LIBRARIES}
    -lm
    -lpthread
)

# Compiler definitions
target_compile_definitions(airplay2-lite PRIVATE
    -DWITH_AVAHI=1
    -DWITH_ALSA=1
    -DWITH_OPENSSL=1
)

# Install target
install(TARGETS airplay2-lite DESTINATION bin)
