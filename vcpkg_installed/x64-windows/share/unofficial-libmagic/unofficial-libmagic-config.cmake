
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was unofficial-libmagic-config.cmake.in                            ########

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

include(CMakeFindDependencyMacro)

if("dynamic" STREQUAL "static")
    if(WIN32)
        find_dependency(unofficial-tre)
    endif()

    if(0)
        find_dependency(ZLIB)
    endif()

    if(0)
        find_dependency(BZip2)
    endif()

    if(0)
        find_dependency(LibLZMA)
    endif()

    if(0)
        find_dependency(zstd)
    endif()
endif()

# Compute the installation prefix relative to this file.
get_filename_component(_IMPORT_PREFIX "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
if(_IMPORT_PREFIX STREQUAL "/")
    set(_IMPORT_PREFIX "")
endif()

if("dynamic" STREQUAL "static")
    add_library(unofficial::libmagic::libmagic STATIC IMPORTED)
else()
    add_library(unofficial::libmagic::libmagic SHARED IMPORTED)
endif()

set_target_properties(unofficial::libmagic::libmagic PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${_IMPORT_PREFIX}/include"
)

if("dynamic" STREQUAL "static")
    set_target_properties(unofficial::libmagic::libmagic PROPERTIES
        INTERFACE_LINK_LIBRARIES
            "\$<LINK_ONLY:$<$<BOOL:${WIN32}>:unofficial::tre::tre;shlwapi>>"
            "\$<LINK_ONLY:$<0:ZLIB::ZLIB>>"
            "\$<LINK_ONLY:$<0:BZip2::BZip2>>"
            "\$<LINK_ONLY:$<0:LibLZMA::LibLZMA>>"
            "\$<LINK_ONLY:$<0:zstd::libzstd>"
    )
endif()

macro(add_library_config config prefix)
    set_property(TARGET unofficial::libmagic::libmagic APPEND PROPERTY IMPORTED_CONFIGURATIONS ${config})
    if("dynamic" STREQUAL "static")
        set_target_properties(unofficial::libmagic::libmagic PROPERTIES
            IMPORTED_LOCATION_${config} "${_IMPORT_PREFIX}/${prefix}lib/magic.lib"
            IMPORTED_LINK_INTERFACE_LANGUAGES_${config} "C"
        )
    else()
        if(WIN32)
            set(library_dir "${_IMPORT_PREFIX}/${prefix}bin/")
            set(soversion_suffix "-1")
            set_target_properties(unofficial::libmagic::libmagic PROPERTIES
                IMPORTED_IMPLIB_${config} "${_IMPORT_PREFIX}/${prefix}/lib/magic.lib"
            )
        else()
            set(library_dir "${_IMPORT_PREFIX}/${prefix}lib/")
        endif()
        set_target_properties(unofficial::libmagic::libmagic PROPERTIES
            IMPORTED_LOCATION_${config} "${library_dir}magic${soversion_suffix}.dll"
        )
        unset(soversion_suffix)
        unset(library_dir)
    endif()
endmacro()

if("" STREQUAL "" OR "" STREQUAL "debug")
    add_library_config(DEBUG "debug/")
endif()

if("" STREQUAL "" OR "" STREQUAL "release")
    add_library_config(RELEASE "")
endif()

set_and_check(unofficial-libmagic_DICTIONARY "${_IMPORT_PREFIX}/share/libmagic/misc/magic.mgc")

unset(_IMPORT_PREFIX)

check_required_components(unofficial-libmagic)
