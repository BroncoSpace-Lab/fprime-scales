set(SOURCE_FILES
  "${CMAKE_CURRENT_LIST_DIR}/PowerPorts.fpp"
  "${CMAKE_CURRENT_LIST_DIR}/SystemStateData.fpp"
  "${CMAKE_CURRENT_LIST_DIR}/ThermalStatePorts.fpp"
)

set(MOD_DEPS
  scales_Types
)

register_fprime_module()