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
        Fw_StringFormat_snprintf
        Os_Cpu_Stub
        Os_Memory_Stub
        Fw_StringScan_sscanf

    BASE_CONFIG
)

target_compile_definitions(PlatformImx8x INTERFACE -DTGT_OS_TYPE_LINUX)

# Your custom PlatformTypes.hpp location, if you still need it.
# Leave this enabled only if lib/fprime-scales/cmake/platform/types/PlatformTypes.hpp exists
# and you intentionally want it instead of the standard Unix platform types.
target_include_directories(PlatformImx8x INTERFACE "${CMAKE_CURRENT_LIST_DIR}/types")