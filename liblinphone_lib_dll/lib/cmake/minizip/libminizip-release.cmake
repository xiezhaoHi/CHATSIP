#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "MINIZIP::libminizip" for configuration "Release"
set_property(TARGET MINIZIP::libminizip APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(MINIZIP::libminizip PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "ZLIB::ZLIB"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libminizip.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS MINIZIP::libminizip )
list(APPEND _IMPORT_CHECK_FILES_FOR_MINIZIP::libminizip "${_IMPORT_PREFIX}/lib/libminizip.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
