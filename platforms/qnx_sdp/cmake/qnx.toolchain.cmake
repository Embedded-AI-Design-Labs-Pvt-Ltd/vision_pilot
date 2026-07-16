# QNX SDP toolchain file for Vision Pilot (cross-compile from Linux)

# Usage:
#   export QNX_HOST=/opt/qnx800/host/linux/x86_64
#   export QNX_TARGET=/opt/qnx800/target/qnx
#   cmake -DCMAKE_TOOLCHAIN_FILE=.../qnx.toolchain.cmake ...

set(CMAKE_SYSTEM_NAME QNX)
set(CMAKE_SYSTEM_VERSION 1)

if(NOT DEFINED ENV{QNX_HOST} OR NOT DEFINED ENV{QNX_TARGET})
  message(FATAL_ERROR "QNX_HOST and QNX_TARGET must be set (source qnxsdp-env.sh)")
endif()

set(QNX_HOST "$ENV{QNX_HOST}")
set(QNX_TARGET "$ENV{QNX_TARGET}")

# Prefer aarch64 automotive targets; override with -DQNX_PROCESSOR=
if(NOT DEFINED QNX_PROCESSOR)
  set(QNX_PROCESSOR aarch64le)
endif()

set(CMAKE_C_COMPILER "${QNX_HOST}/usr/bin/nto${QNX_PROCESSOR}-gcc")
set(CMAKE_CXX_COMPILER "${QNX_HOST}/usr/bin/nto${QNX_PROCESSOR}-g++")
set(CMAKE_AR "${QNX_HOST}/usr/bin/nto${QNX_PROCESSOR}-ar" CACHE FILEPATH "" FORCE)
set(CMAKE_RANLIB "${QNX_HOST}/usr/bin/nto${QNX_PROCESSOR}-ranlib" CACHE FILEPATH "" FORCE)

set(CMAKE_FIND_ROOT_PATH "${QNX_TARGET}/${QNX_PROCESSOR}" "${QNX_TARGET}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

add_compile_definitions(_QNX_SOURCE)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
