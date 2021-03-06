# Required for static linking and exported to PBSAM.cmake
set(PBSAM_LINKER_LIBS "")

# ---[ BLAS
if(NOT APPLE)
  set(BLAS "Open" CACHE STRING "Selected BLAS library")
  set_property(CACHE BLAS PROPERTY STRINGS "Atlas;Open;MKL")

  if(BLAS STREQUAL "Open" OR BLAS STREQUAL "open")
    find_package(OpenBLAS)
    if( OpenBLAS_FOUND)
      include_directories(${OpenBLAS_INCLUDE_DIR})
      list(APPEND PBSAM_LINKER_LIBS ${OpenBLAS_LIB})
      add_definitions(-D__MKL)
      add_definitions(-D__LAU)
    endif()
  endif()
elseif(APPLE)
  find_package(vecLib)
  if(vecLib_FOUND)
    include_directories(${vecLib_INCLUDE_DIR})
    MESSAGE( STATUS "Dependencies: " ${vecLib_INCLUDE_DIR} )
    list(APPEND PBSAM_LINKER_LIBS ${vecLib_LINKER_LIBS})
    add_definitions(-D__MACOS)
    add_definitions(-D__LAU)
  endif()
# MESSAGE( STATUS "sys root " ${CMAKE_OSX_SYSROOT} )
endif()

MESSAGE( STATUS "linkers: " ${PBSAM_LINKER_LIBS} )
