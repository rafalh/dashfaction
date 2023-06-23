#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "subhook::subhook" for configuration "Release"
set_property(TARGET subhook::subhook APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(subhook::subhook PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/subhook.lib"
  )

list(APPEND _cmake_import_check_targets subhook::subhook )
list(APPEND _cmake_import_check_files_for_subhook::subhook "${_IMPORT_PREFIX}/lib/subhook.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
