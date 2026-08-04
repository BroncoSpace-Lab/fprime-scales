set(SOURCE_FILES
  "${CMAKE_CURRENT_LIST_DIR}/HubComAdapter.fpp"
  "${CMAKE_CURRENT_LIST_DIR}/HubComAdapter.cpp"
)

register_fprime_module()

### Unit Tests ###
set(UT_SOURCE_FILES
  "${CMAKE_CURRENT_LIST_DIR}/HubComAdapter.fpp"
  "${CMAKE_CURRENT_LIST_DIR}/test/ut/HubComAdapterTestMain.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/test/ut/HubComAdapterTester.cpp"
)
set(UT_AUTO_HELPERS ON)
register_fprime_ut()
