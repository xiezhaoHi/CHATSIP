#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "MINIZIP::libminizip" for configuration "Debug"
set_property(TARGET MINIZIP::libminizip APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(MINIZIP::libminizip PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C"
  IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG "ZLIB::ZLIB"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libminizipd.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS MINIZIP::libminizip )
list(APPEND _IMPORT_CHECK_FILES_FOR_MINIZIP::libminizip "${_IMPORT_PREFIX}/lib/libminizipd.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
