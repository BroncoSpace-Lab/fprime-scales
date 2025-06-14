####
# Template toolchain.cmake.template: 
#
# This file acts as a template for the cmake toolchains. These toolchain files
# specify what tools to use when performing the build as part of CMake. This
# file can be used to quickly set one up.
#
# Follow all the steps in this template to create a toolchain file. Ensure
# to remove the template-failsafe (step 1) and fill in all <SOMETHING> tags.
#
# **Note:** this file should follow the standard CMake toolchain format. See:
# https://cmake.org/cmake/help/v3.12/manual/cmake-toolchains.7.html
#
# **Note:** If the user desires to set compile flags, or F prime specific build options, a platform
#           file should be constructed. See: [platform.md](platform.md)
#
# ### Filling In CMake Toolchain by Example ###
#
# CMake Toolchain files, at the most basic, define the system name and C and C++ compilers. In
# addition, a find path can be set to search for other utilities. This example will walk through
# setting these values using the appropriate variables. These can be specified using the following
# CMake setting flags:
#
# ```
# CMAKE_SYSTEM_NAME "RaspberryPI"
# # specify the cross compiler
# set(CMAKE_C_COMPILER "/opt/rpi/bin/arm-linux-gnueabihf-gcc")
# set(CMAKE_CXX_COMPILER "/opt/rpi/bin/arm-linux-gnueabihf-g++")
# # where is the target environment
# set(CMAKE_FIND_ROOT_PATH  "/opt/rpi")
# ```
#
# **Note:** if copying the template, delete the message with FATAL_ERROR line. This is a fail-safe
#           to prevent a raw-copy from being treated as a valid toolchain file. 
####

## STEP 1: DELETE the following fail-safe line


## STEP 2: Specify the target system's name. i.e. raspberry-pi-3
set(CMAKE_SYSTEM_NAME "imx8x")

# STEP 3: Specify the path to C and CXX cross compilers
set (CMAKE_C_COMPILER "/opt/ampliphy-vendor/5.0.4-devel/sysroots/x86_64-phytecsdk-linux/usr/bin/aarch64-phytec-linux/aarch64-phytec-linux-gcc")
set (CMAKE_CXX_COMPILER "/opt/ampliphy-vendor/5.0.4-devel/sysroots/x86_64-phytecsdk-linux/usr/bin/aarch64-phytec-linux/aarch64-phytec-linux-g++")
add_compile_options(-O -mcpu=cortex-a35+crc+crypto -mbranch-protection=standard -fstack-protector-strong  -O2 -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security -Werror=format-security --sysroot=/opt/ampliphy-vendor/5.0.4-devel/sysroots/cortexa35-phytec-linux)
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)


# STEP 4: Specify paths to root of toolchain package, for searching for
#         libraries, executables, etc.
set (CMAKE_FIND_ROOT_PATH "/opt/ampliphy-vendor")

# DO NOT EDIT: F prime searches the host for programs, not the cross
# compile toolchain
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# DO NOT EDIT: F prime searches for libs, includes, and packages in the
# toolchain when cross-compiling.
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)