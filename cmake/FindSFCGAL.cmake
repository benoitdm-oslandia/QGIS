# Find SFCGAL
# ~~~~~~~~~
# Copyright (c) 2024, De Mezzo Benoit <benoit dot de dot mezzo at oslandia dot com>
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
#
# Once run this will define: 
# 
# SFCGAL_FOUND       = system has SFCGAL lib
#
# SFCGAL_LIBRARY     = full path to the library
#
# SFCGAL_INCLUDE_DIR      = where to find headers

find_package(SFCGAL CONFIG)
if(NOT SFCGAL_FOUND)
  # Fallback logic for SFCGAL < 3.5, as soon as we switch to SFCGAL>=3.5 this file (Find_SFCGAL.cmake) can be deleted
  INCLUDE (${CMAKE_SOURCE_DIR}/cmake/MacPlistMacros.cmake)

  IF(WIN32)
  
    IF (MINGW)
      FIND_PATH(SFCGAL_INCLUDE_DIR sfcgal.h /usr/local/include /usr/include c:/msys/local/include PATH_SUFFIXES sfcgal)
      FIND_LIBRARY(SFCGAL_LIBRARY NAMES sfcgal PATHS /usr/local/lib /usr/lib c:/msys/local/lib)
    ENDIF (MINGW)
  
    IF (MSVC)
      FIND_PATH(SFCGAL_INCLUDE_DIR sfcgal.h "$ENV{LIB_DIR}/include/sfcgal" $ENV{INCLUDE})
      FIND_LIBRARY(SFCGAL_LIBRARY NAMES sfcgal sfcgal_i PATHS
  	    "$ENV{LIB_DIR}/lib" $ENV{LIB} /usr/lib c:/msys/local/lib)
      IF (SFCGAL_LIBRARY)
        SET (
           SFCGAL_LIBRARY;odbc32;odbccp32
           CACHE STRING INTERNAL)
      ENDIF (SFCGAL_LIBRARY)
    ENDIF (MSVC)
  
  ELSEIF(APPLE AND QGIS_MAC_DEPS_DIR)
  
      FIND_PATH(SFCGAL_INCLUDE_DIR sfcgal.h "$ENV{LIB_DIR}/include")
      FIND_LIBRARY(SFCGAL_LIBRARY NAMES sfcgal PATHS "$ENV{LIB_DIR}/lib")
  
  ELSE(WIN32)
  
    IF(UNIX) 
  
      # try to use framework on mac
      # want clean framework path, not unix compatibility path
      IF (APPLE)
        IF (CMAKE_FIND_FRAMEWORK MATCHES "FIRST"
            OR CMAKE_FRAMEWORK_PATH MATCHES "ONLY"
            OR NOT CMAKE_FIND_FRAMEWORK)
          SET (CMAKE_FIND_FRAMEWORK_save ${CMAKE_FIND_FRAMEWORK} CACHE STRING "" FORCE)
          SET (CMAKE_FIND_FRAMEWORK "ONLY" CACHE STRING "" FORCE)
          FIND_LIBRARY(SFCGAL_LIBRARY SFCGAL)
          IF (SFCGAL_LIBRARY)
            # they're all the same in a framework
            SET (SFCGAL_INCLUDE_DIR ${SFCGAL_LIBRARY}/Headers CACHE PATH "Path to a file.")
            # set SFCGAL_CONFIG to make later test happy, not used here, may not exist
            SET (SFCGAL_CONFIG ${SFCGAL_LIBRARY}/unix/bin/sfcgal-config CACHE FILEPATH "Path to a program.")
            # version in info.plist
            GET_VERSION_PLIST (${SFCGAL_LIBRARY}/Resources/Info.plist SFCGAL_VERSION)
            IF (NOT SFCGAL_VERSION)
              MESSAGE (FATAL_ERROR "Could not determine SFCGAL version from framework.")
            ENDIF (NOT SFCGAL_VERSION)
            STRING(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\1" SFCGAL_VERSION_MAJOR "${SFCGAL_VERSION}")
            STRING(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\2" SFCGAL_VERSION_MINOR "${SFCGAL_VERSION}")
          IF (SFCGAL_VERSION_MAJOR LESS 3)
              MESSAGE (FATAL_ERROR "SFCGAL version is too old (${SFCGAL_VERSION}). Use 3.2 or higher.")
          ENDIF (SFCGAL_VERSION_MAJOR LESS 3)
            IF ( (SFCGAL_VERSION_MAJOR EQUAL 3) AND (SFCGAL_VERSION_MINOR LESS 2) )
              MESSAGE (FATAL_ERROR "SFCGAL version is too old (${SFCGAL_VERSION}). Use 3.2 or higher.")
            ENDIF( (SFCGAL_VERSION_MAJOR EQUAL 3) AND (SFCGAL_VERSION_MINOR LESS 2) )
  
          ENDIF (SFCGAL_LIBRARY)
          SET (CMAKE_FIND_FRAMEWORK ${CMAKE_FIND_FRAMEWORK_save} CACHE STRING "" FORCE)
        ENDIF ()
      ENDIF (APPLE)
  
      IF(CYGWIN)
        FIND_LIBRARY(SFCGAL_LIBRARY NAMES sfcgal PATHS /usr/lib /usr/local/lib)
      ENDIF(CYGWIN)
  
      IF (NOT SFCGAL_INCLUDE_DIR OR NOT SFCGAL_LIBRARY OR NOT SFCGAL_CONFIG)
        # didn't find OS X framework, and was not set by user
        SET(SFCGAL_CONFIG_PREFER_PATH "$ENV{SFCGAL_HOME}/bin" CACHE STRING "preferred path to SFCGAL (sfcgal-config)")
        FIND_PROGRAM(SFCGAL_CONFIG sfcgal-config
            ${SFCGAL_CONFIG_PREFER_PATH}
            $ENV{LIB_DIR}/bin
            /usr/local/bin/
            /usr/bin/
            )
        # MESSAGE("DBG SFCGAL_CONFIG ${SFCGAL_CONFIG}")
      
        IF (SFCGAL_CONFIG)
  
          ## extract sfcgal version
          EXEC_PROGRAM(${SFCGAL_CONFIG}
              ARGS --version
              OUTPUT_VARIABLE SFCGAL_VERSION )
          STRING(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\1" SFCGAL_VERSION_MAJOR "${SFCGAL_VERSION}")
          STRING(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\2" SFCGAL_VERSION_MINOR "${SFCGAL_VERSION}")
          STRING(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\3" SFCGAL_VERSION_MICRO "${SFCGAL_VERSION}")
    
#          MESSAGE("DBG SFCGAL_VERSION ${SFCGAL_VERSION}")
#          MESSAGE("DBG SFCGAL_VERSION_MAJOR ${SFCGAL_VERSION_MAJOR}")
#          MESSAGE("DBG SFCGAL_VERSION_MINOR ${SFCGAL_VERSION_MINOR}")
    
          # check for sfcgal version
          # version 1.2.5 is known NOT to be supported (missing CPL_STDCALL macro)
          # According to INSTALL, 2.1+ is required
        IF (SFCGAL_VERSION_MAJOR LESS 1)
          MESSAGE (FATAL_ERROR "SFCGAL version is too old (${SFCGAL_VERSION}). Use 1.0 or higher.")
        ENDIF (SFCGAL_VERSION_MAJOR LESS 1)
        #IF ( (SFCGAL_VERSION_MAJOR EQUAL 2) AND (SFCGAL_VERSION_MINOR LESS 1) )
        #  MESSAGE (FATAL_ERROR "SFCGAL version is too old (${SFCGAL_VERSION}). Use 2.1 or higher.")
        #ENDIF( (SFCGAL_VERSION_MAJOR EQUAL 2) AND (SFCGAL_VERSION_MINOR LESS 1) )
          IF ( (SFCGAL_VERSION_MAJOR EQUAL 1) AND (SFCGAL_VERSION_MINOR EQUAL 4) AND (SFCGAL_VERSION_MICRO LESS 1) )
            MESSAGE (FATAL_ERROR "SFCGAL version is too old (${SFCGAL_VERSION}). Use 1.4.1 or higher.")
          ENDIF( (SFCGAL_VERSION_MAJOR EQUAL 1) AND (SFCGAL_VERSION_MINOR EQUAL 4) AND (SFCGAL_VERSION_MICRO LESS 1) )
  
          # set INCLUDE_DIR to prefix+include
          EXEC_PROGRAM(${SFCGAL_CONFIG}
              ARGS --prefix
              OUTPUT_VARIABLE SFCGAL_PREFIX)
#          MESSAGE("DBG  SFCGAL_PREFIX=${SFCGAL_PREFIX}")
          #SET(SFCGAL_INCLUDE_DIR ${SFCGAL_PREFIX}/include CACHE STRING INTERNAL)
          FIND_PATH(SFCGAL_INCLUDE_DIR
              capi/sfcgal_c.h
              ${SFCGAL_PREFIX}/include/SFCGAL
              ${SFCGAL_PREFIX}/include
              /usr/local/include 
              /usr/include 
              )
  
          ## extract link dirs for rpath  
          EXEC_PROGRAM(${SFCGAL_CONFIG}
              ARGS --libs
              OUTPUT_VARIABLE SFCGAL_CONFIG_LIBS )
  
          ## split off the link dirs (for rpath)
          ## use regular expression to match wildcard equivalent "-L*<endchar>"
          ## with <endchar> is a space or a semicolon
          STRING(REGEX MATCHALL "[-][L]([^ ;])+" 
              SFCGAL_LINK_DIRECTORIES_WITH_PREFIX
              "${SFCGAL_CONFIG_LIBS}" )
          #      MESSAGE("DBG  SFCGAL_LINK_DIRECTORIES_WITH_PREFIX=${SFCGAL_LINK_DIRECTORIES_WITH_PREFIX}")
  
          ## remove prefix -L because we need the pure directory for LINK_DIRECTORIES
        
          IF (SFCGAL_LINK_DIRECTORIES_WITH_PREFIX)
            STRING(REGEX REPLACE "[-][L]" "" SFCGAL_LINK_DIRECTORIES ${SFCGAL_LINK_DIRECTORIES_WITH_PREFIX} )
          ENDIF (SFCGAL_LINK_DIRECTORIES_WITH_PREFIX)
  
          ## split off the name
          ## use regular expression to match wildcard equivalent "-l*<endchar>"
          ## with <endchar> is a space or a semicolon
          STRING(REGEX MATCHALL "[-][l]([^ ;])+" 
              SFCGAL_LIB_NAME_WITH_PREFIX
              "${SFCGAL_CONFIG_LIBS}" )
          #      MESSAGE("DBG  SFCGAL_LIB_NAME_WITH_PREFIX=${SFCGAL_LIB_NAME_WITH_PREFIX}")
  
  
          ## remove prefix -l because we need the pure name
        
          IF (SFCGAL_LIB_NAME_WITH_PREFIX)
            STRING(REGEX REPLACE "[-][l]" "" SFCGAL_LIB_NAME ${SFCGAL_LIB_NAME_WITH_PREFIX} )
          ENDIF (SFCGAL_LIB_NAME_WITH_PREFIX)
  
          IF (APPLE)
            IF (NOT SFCGAL_LIBRARY)
              # work around empty SFCGAL_LIBRARY left by framework check
              # while still preserving user setting if given
              # ***FIXME*** need to improve framework check so below not needed
              SET(SFCGAL_LIBRARY ${SFCGAL_LINK_DIRECTORIES}/lib${SFCGAL_LIB_NAME}.dylib CACHE STRING INTERNAL FORCE)
            ENDIF (NOT SFCGAL_LIBRARY)
          ELSE (APPLE)
            FIND_LIBRARY(SFCGAL_LIBRARY NAMES ${SFCGAL_LIB_NAME} SFCGAL PATHS ${SFCGAL_LINK_DIRECTORIES}/lib ${SFCGAL_LINK_DIRECTORIES})
          ENDIF (APPLE)
        
        ELSE(SFCGAL_CONFIG)
          MESSAGE("FindSFCGAL.cmake: sfcgal-config not found. Please set it manually. SFCGAL_CONFIG=${SFCGAL_CONFIG}")
        ENDIF(SFCGAL_CONFIG)
      ENDIF (NOT SFCGAL_INCLUDE_DIR OR NOT SFCGAL_LIBRARY OR NOT SFCGAL_CONFIG)
    ENDIF(UNIX)
  ENDIF(WIN32)
  
  
  IF (SFCGAL_INCLUDE_DIR AND SFCGAL_LIBRARY)
     SET(SFCGAL_FOUND TRUE)
  ENDIF (SFCGAL_INCLUDE_DIR AND SFCGAL_LIBRARY)
  
  IF (SFCGAL_FOUND)
     IF (NOT SFCGAL_FIND_QUIETLY)
        ## extract sfcgal version
        EXEC_PROGRAM(${SFCGAL_CONFIG}
            ARGS --version
            OUTPUT_VARIABLE SFCGAL_VERSION )
        STRING(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\1" SFCGAL_VERSION_MAJOR "${SFCGAL_VERSION}")
        STRING(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\2" SFCGAL_VERSION_MINOR "${SFCGAL_VERSION}")
        STRING(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\3" SFCGAL_VERSION_MICRO "${SFCGAL_VERSION}")
        MESSAGE(STATUS "Found SFCGAL: ${SFCGAL_LIBRARY} (${SFCGAL_VERSION})")
     ENDIF (NOT SFCGAL_FIND_QUIETLY)
     add_library(SFCGAL::SFCGAL UNKNOWN IMPORTED)
     target_link_libraries(SFCGAL::SFCGAL INTERFACE ${SFCGAL_LIBRARY})
     target_include_directories(SFCGAL::SFCGAL INTERFACE ${SFCGAL_INCLUDE_DIR})
     set_target_properties(SFCGAL::SFCGAL PROPERTIES IMPORTED_LOCATION ${SFCGAL_LIBRARY})
  ELSE (SFCGAL_FOUND)
  
     MESSAGE(SFCGAL_INCLUDE_DIR=${SFCGAL_INCLUDE_DIR})
     MESSAGE(SFCGAL_LIBRARY=${SFCGAL_LIBRARY})
     MESSAGE(FATAL_ERROR "Could not find SFCGAL")
  
  ENDIF (SFCGAL_FOUND)
endif()
