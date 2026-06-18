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

####
# IMX8X platform file for F Prime 4.x
#
# Compiler, sysroot, CMAKE_SYSTEM_NAME, and FPRIME_PLATFORM are handled by:
#   lib/fprime-scales/cmake/toolchain/imx8x.cmake
####

# Target behaves like Linux/POSIX
set(FPRIME_USE_POSIX ON)
set(FPRIME_HAS_SOCKETS ON)

# Use threads unless explicitly building with the baremetal scheduler
if (NOT DEFINED FPRIME_USE_BAREMETAL_SCHEDULER)
    set(FPRIME_USE_BAREMETAL_SCHEDULER OFF)
endif()

if (NOT FPRIME_USE_BAREMETAL_SCHEDULER)
    message(STATUS "Requiring thread library")
    find_package(Threads REQUIRED)
endif()

# Pull in the standard Unix platform types/config support
add_fprime_subdirectory("${FPRIME_FRAMEWORK_PATH}/cmake/platform/unix/Platform/")

# F Prime 4.x platform configuration
register_fprime_config(
    PlatformImx8x

    INTERFACE

    CHOOSES_IMPLEMENTATIONS
        Os_File_Posix
        Os_Console_Posix
        Os_Task_Posix
        Os_Mutex_Posix
        Os_Generic_PriorityQueue
        Os_RawTime_Posix
        Os_Cpu_Stub
        Os_Memory_Stub

    BASE_CONFIG
)

target_compile_definitions(PlatformImx8x INTERFACE -DTGT_OS_TYPE_LINUX)

# Your custom PlatformTypes.hpp location, if you still need it.
# Leave this enabled only if lib/fprime-scales/cmake/platform/types/PlatformTypes.hpp exists
# and you intentionally want it instead of the standard Unix platform types.
target_include_directories(PlatformImx8x INTERFACE "${CMAKE_CURRENT_LIST_DIR}/types")