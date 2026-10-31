#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "getopt::getopt_shared" for configuration "Debug"
set_property(TARGET getopt::getopt_shared APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(getopt::getopt_shared PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/debug/lib/getopt.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/bin/getopt.dll"
  )

list(APPEND _cmake_import_check_targets getopt::getopt_shared )
list(APPEND _cmake_import_check_files_for_getopt::getopt_shared "${_IMPORT_PREFIX}/debug/lib/getopt.lib" "${_IMPORT_PREFIX}/debug/bin/getopt.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
