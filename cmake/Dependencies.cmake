# Required for static linking and exported to PBSAM.cmake
set(PBSAM_LINKER_LIBS "")

# ---[ BLAS
if(NOT APPLE)
  set(BLAS "Atlas" CACHE STRING "Selected BLAS library")
  set_property(CACHE BLAS PROPERTY STRINGS "Atlas;Open;MKL")

  if(BLAS STREQUAL "Atlas" OR BLAS STREQUAL "atlas")
    find_package(Atlas REQUIRED)
    include_directories(SYSTEM ${Atlas_INCLUDE_DIR})
    list(APPEND PBSAM_LINKER_LIBS ${Atlas_LIBRARIES})
  elseif(BLAS STREQUAL "Open" OR BLAS STREQUAL "open")
    find_package(OpenBLAS REQUIRED)
    include_directories(SYSTEM ${OpenBLAS_INCLUDE_DIR})
    list(APPEND PBSAM_LINKER_LIBS ${OpenBLAS_LIB})
  elseif(BLAS STREQUAL "MKL" OR BLAS STREQUAL "mkl")
    find_package(MKL REQUIRED)
    include_directories(SYSTEM ${MKL_INCLUDE_DIR})
    list(APPEND PBSAM_LINKER_LIBS ${MKL_LIBRARIES})
    add_definitions(-DUSE_MKL)
  endif()
elseif(APPLE)
  find_package(vecLib REQUIRED)
  include_directories(SYSTEM ${vecLib_INCLUDE_DIR})
  list(APPEND PBSAM_LINKER_LIBS ${vecLib_LINKER_LIBS})
  add_definitions(-D__MACOS)
  add_definitions(-D__LAU)
endif()
