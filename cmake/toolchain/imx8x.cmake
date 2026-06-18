set(CMAKE_SYSTEM_NAME Linux)
set(FPRIME_PLATFORM Linux)

# Cross-compile checks should not try to build/link runnable target executables.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

if(DEFINED ENV{IMX_C_COMPILER})
    message("The IMX8X C compiler path is set to $ENV{IMX_C_COMPILER}")
else()
    message(SEND_ERROR "Environment variable IMX_C_COMPILER is not set. Please set it to the path of your IMX C cross-compiler toolchain.")
endif()

if(DEFINED ENV{IMX_CXX_COMPILER})
    message("The IMX8X CXX compiler path is set to $ENV{IMX_CXX_COMPILER}")
else()
    message(SEND_ERROR "Environment variable IMX_CXX_COMPILER is not set. Please set it to the path of your IMX C++ cross-compiler toolchain.")
endif()

if(DEFINED ENV{IMX_ROOT_PATH})
    message("The IMX8X root path is set to $ENV{IMX_ROOT_PATH}")
else()
    message(SEND_ERROR "Environment variable IMX_ROOT_PATH is not set. Please set it to the root path of your IMX SDK.")
endif()

set(CMAKE_C_COMPILER "$ENV{IMX_C_COMPILER}")
set(CMAKE_CXX_COMPILER "$ENV{IMX_CXX_COMPILER}")

set(CMAKE_SYSROOT "/opt/ampliphy-vendor/5.0.4-devel/sysroots/cortexa35-phytec-linux")

add_compile_options(
    -O
    -mcpu=cortex-a35+crc+crypto
    -mbranch-protection=standard
    -fstack-protector-strong
    -O2
    -D_FORTIFY_SOURCE=2
    -Wformat
    -Wformat-security
    -Werror=format-security
)

set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

set(CMAKE_FIND_ROOT_PATH "$ENV{IMX_ROOT_PATH}")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)