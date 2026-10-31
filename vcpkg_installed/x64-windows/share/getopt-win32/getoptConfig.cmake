
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was getoptConfig.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

# Include the targets file to define getopt::getopt_shared and getopt::getopt_static
include("${CMAKE_CURRENT_LIST_DIR}/getoptTargets.cmake")

# Initialize list of available components based on installed targets
set(_getopt_available_components "")
if(TARGET getopt::getopt_shared)
    list(APPEND _getopt_available_components shared)
    set(getopt_shared_FOUND TRUE)  # Set for available components
endif()
if(TARGET getopt::getopt_static)
    list(APPEND _getopt_available_components static)
    set(getopt_static_FOUND TRUE)  # Set for available components
endif()

# Check requested components
foreach(comp IN LISTS getopt_FIND_COMPONENTS)
    if(NOT comp IN_LIST _getopt_available_components)
        set(getopt_FOUND FALSE)
        set(getopt_NOT_FOUND_MESSAGE "Component '${comp}' is not available in getopt. Valid components are: ${_getopt_available_components}")
        return()
    endif()
endforeach()

# Ensure all required components are found
check_required_components(getopt)
