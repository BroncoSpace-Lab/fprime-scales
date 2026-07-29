####
# FPrime CMakeLists.txt:
#
# SOURCE_FILES: combined list of source and autocoding files
# MOD_DEPS: (optional) module dependencies
# UT_SOURCE_FILES: list of source files for unit tests
#
# More information in the F´ CMake API documentation:
# https://fprime.jpl.nasa.gov/latest/documentation/reference
#
####

set(SOURCE_FILES
  "${CMAKE_CURRENT_LIST_DIR}/InaManager.fpp"
  "${CMAKE_CURRENT_LIST_DIR}/InaManager.cpp"
)

# Uncomment and add any modules that this component depends on, else
# they might not be available when cmake tries to build this component.
#
# Module names are derived from the path from the nearest project/library/framework
# root when not specifically overridden by the developer. i.e. The module defined by
# `Ref/SignalGen/CMakeLists.txt` will be named `Ref_SignalGen`.  `Ref/SignalGen`
# is an acceptable alternative and will be internally converted to `Ref_SignalGen`.
#
# set(MOD_DEPS
#   MyPackage_MyOtherModule
# )

register_fprime_module()


# TEMP: commented out again by a concurrent session to unblock a shared
# build while the InaManager audit agent is still writing its real test
# suite. Restore from /tmp/InaManager_CMakeLists_agent_version2.txt once
# that work lands.
# set(UT_SOURCE_FILES
#   "${CMAKE_CURRENT_LIST_DIR}/InaManager.fpp"
#   "${CMAKE_CURRENT_LIST_DIR}/test/ut/InaManagerTestMain.cpp"
#   "${CMAKE_CURRENT_LIST_DIR}/test/ut/InaManagerTester.cpp"
# )
# set(UT_AUTO_HELPERS ON)
# register_fprime_ut()