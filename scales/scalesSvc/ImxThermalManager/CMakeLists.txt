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

register_fprime_module(
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/ImxThermalManager.fpp"
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/ImxThermalManager.cpp"
  DEPENDS
    Os_File
)


### Unit Tests ###
register_fprime_ut(
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/ImxThermalManager.fpp"
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/ImxThermalManagerTestMain.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/ImxThermalManagerTester.cpp"
  DEPENDS
    STest
  CHOOSES_IMPLEMENTATIONS
    Os_File_Posix
  UT_AUTO_HELPERS
)
