set(SOURCE_FILES
  "${CMAKE_CURRENT_LIST_DIR}/PowerPorts.fpp"
  "${CMAKE_CURRENT_LIST_DIR}/SystemStateData.fpp"
)

set(MOD_DEPS
  scales_Types
)

register_fprime_module()