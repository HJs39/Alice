#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "scn::scn" for configuration "Debug"
set_property(TARGET scn::scn APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(scn::scn PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/lib/scn.lib"
  )

list(APPEND _cmake_import_check_targets scn::scn )
list(APPEND _cmake_import_check_files_for_scn::scn "${_IMPORT_PREFIX}/debug/lib/scn.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
