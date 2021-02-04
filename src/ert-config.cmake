include(${CMAKE_CURRENT_LIST_DIR}/ert-targets.cmake)

add_library(openenclave::ertmeshpremain STATIC IMPORTED)
set_target_properties(
  openenclave::ertmeshpremain
  PROPERTIES IMPORTED_LOCATION
             ${CMAKE_CURRENT_LIST_DIR}/../enclave/libertmeshpremain.a)
