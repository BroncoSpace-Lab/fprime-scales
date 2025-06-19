set(SOURCE_FILES
  "${CMAKE_CURRENT_LIST_DIR}/DataTransferRequest.fpp"
  "${CMAKE_CURRENT_LIST_DIR}/PowerLevel.fpp"
  "${CMAKE_CURRENT_LIST_DIR}/PowerReading.fpp"
  "${CMAKE_CURRENT_LIST_DIR}/PowerState.fpp"
  "${CMAKE_CURRENT_LIST_DIR}/PowerThermalStatus.fpp"
  "${CMAKE_CURRENT_LIST_DIR}/StorageData.fpp"
  "${CMAKE_CURRENT_LIST_DIR}/StorageStatus.fpp"
  "${CMAKE_CURRENT_LIST_DIR}/SystemState.fpp"
  "${CMAKE_CURRENT_LIST_DIR}/SystemStateData.fpp"
  "${CMAKE_CURRENT_LIST_DIR}/ThermalReading.fpp"
)

register_fprime_module()