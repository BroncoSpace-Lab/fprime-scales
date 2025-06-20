####
# platform.cmake.template:
#
# This file acts as a template for the fprime platform files used by the CMake system.
# These files specify build flags, compiler directives, and must specify an include
# directory for system includes like "PlatformTypes.hpp".
#
# Follow all the steps in this template to create a platform file. Ensure
# to remove the platform-failsafe (step 1) and fill in all <SOMETHING> tags.
#
# **Note:** If the user desires to set compiler paths, and other CMake toolchain settings, a
#           toolchain file should be constructed. See: [toolchain.md](toolchain.md)
#
# ### Platform File Loading ###
#
# The user rarely needs to specify a platform file directly. It will be specified based on the data
# in the chosen Toolchain file, or by the CMake system itself. However, if the user wants to control
# which platform file is used, the load is specified by the following rules:
#
# If the user specifies a CMake Toolchain file, then the platform file `${CMAKE_SYSTEM_NAME}.cmake`
# will be used. `${CMAKE_SYSTEM_NAME}` is set in the toolchain file and is typically set to a name like Linux, or Darwin
# but may be more specific if required.
#
# Otherwise, CMake sets the `${CMAKE_SYSTEM_NAME}` automatically to be that of the Host system, and that platform
# will be used. e.g. when building on Linux, the platform file "Linux.cmake" will be used.
#
# ### Filling In CMake Platform by Example ###
#
# F prime platform files are used to set F prime specific settings. This allows the user to control
# some aspects of the F prime build at the top-level. This means setting global include directories
# compiler definitions for the platform, threading libraries, etc. The bare-minimum platform file
# should specify an include directory for "PlatformTypes.hpp" and a threading library if using
# active components with OS supported threads. This can be done with the following lines:
#
# ```
# FIND_PACKAGE ( Threads REQUIRED )
# include_directories(SYSTEM "${FPRIME_FRAMEWORK_PATH}/Fw/Types/Linux")
# ```
#
# **Note:** much of this is done already in *-common.cmake for Linux. If using a linux-like system,
#           this can be included to save time.
#
# **Note:** if copying the template, delete the message with FATAL_ERROR line. This is a fail-safe
#           to prevent a raw-copy from being treated as a valid toolchain file.
####

## STEP 1: DELETE the following fail-safe line
# message(FATAL_ERROR "\n[F-PRIME] Platform must be filled before use.\n")

## STEP 2: Specify the OS type include directive i.e. LINUX or DARWIN
add_definitions(-DTGT_OS_TYPE_LINUX)

# STEP 3: Specify CMAKE C and CXX compile flags. DO NOT clear existing flags
set(CMAKE_C_FLAGS
  "${CMAKE_C_FLAGS} -mcpu=cortex-a35+crc+crypto -mbranch-protection=standard -fstack-protector-strong  -O2 -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security -Werror=format-security --sysroot=/opt/ampliphy-vendor/5.0.4-devel/sysroots/cortexa35-phytec-linux"
)
set(CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} -mcpu=cortex-a35+crc+crypto -mbranch-protection=standard -fstack-protector-strong  -O2 -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security -Werror=format-security --sysroot=/opt/ampliphy-vendor/5.0.4-devel/sysroots/cortexa35-phytec-linux"
)

# STEP 4: Specify that a thread package should be searched in the toolchain
#         directory. NOTE: when running without threads, remove this line.
#         Here there is a check for the using baremetal scheduler
if (NOT DEFINED FPRIME_USE_BAREMETAL_SCHEDULER)
   set(FPRIME_USE_BAREMETAL_SCHEDULER OFF)
   message(STATUS "Requiring thread library")
   FIND_PACKAGE ( Threads REQUIRED )
endif()

choose_fprime_implementation(Os/File Os/File/Posix)
choose_fprime_implementation(Os/Console Os/Console/Posix)
choose_fprime_implementation(Os/Task Os/Task/Posix)
choose_fprime_implementation(Os/Mutex Os/Mutex/Posix)
choose_fprime_implementation(Os/Queue Os/Generic/PriorityQueue)
choose_fprime_implementation(Os/RawTime Os/RawTime/Posix)

choose_fprime_implementation(Os/Cpu Os/Cpu/Stub)
choose_fprime_implementation(Os/Memory Os/Memory/Stub)

set(FPRIME_USE_POSIX ON)
# STEP 5: Specify a directory containing the "PlatformTypes.hpp" headers, as well
#         as other system headers. Other global headers can be placed here.
#         Note: Typically, the Linux directory is a good default, as it grabs
#         standard types from <cstdint>.
add_definitions(-DTGT_OS_TYPE_LINUX)
# include_directories(SYSTEM "${FPRIME_FRAMEWORK_PATH}/Fw/Types/Linux")
include_directories(SYSTEM "${CMAKE_CURRENT_LIST_DIR}/types")