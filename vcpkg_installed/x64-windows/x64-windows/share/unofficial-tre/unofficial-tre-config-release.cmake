#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "unofficial::tre::tre" for configuration "Release"
set_property(TARGET unofficial::tre::tre APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(unofficial::tre::tre PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/tre.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/tre.dll"
  )

list(APPEND _cmake_import_check_targets unofficial::tre::tre )
list(APPEND _cmake_import_check_files_for_unofficial::tre::tre "${_IMPORT_PREFIX}/lib/tre.lib" "${_IMPORT_PREFIX}/bin/tre.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
