#####
# A CMake build file for the ImageAlter service, a service that can
# manipulate images.
# 
# Copyright 2009, Yahoo!
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
# 
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in
#     the documentation and/or other materials provided with the
#     distribution.
# 
#  3. Neither the name of Yahoo! nor the names of its
#     contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
# 
# Original Author: Lloyd Hilaiel
#####

cmake_minimum_required(VERSION 2.6)

INCLUDE(BuildConfigs.cmake)

SET (serviceName ImageAlter)

PROJECT(${serviceName}Service)

SET (outputDir "${CMAKE_CURRENT_BINARY_DIR}/${serviceName}")

# default to a debug build.  This gets us symbols by default under
# the cmake makefile generator
IF (NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "Release")
ENDIF ()

STRING(TOLOWER ${CMAKE_BUILD_TYPE} buildTypeLC)


SET(ServiceTools ${CMAKE_CURRENT_SOURCE_DIR}/../external/bp-service-tools)

INCLUDE_DIRECTORIES(
  "${CMAKE_CURRENT_SOURCE_DIR}/../external/built/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/../external/built/include/GraphicsMagick"
  "${ServiceTools}/CppTools/external/bpsdk/include"
  "${ServiceTools}/CppTools/src"
)

LINK_DIRECTORIES(
  "${CMAKE_CURRENT_SOURCE_DIR}/../external/built/lib"
  "${CMAKE_CURRENT_SOURCE_DIR}/../external/built/lib/${buildTypeLC}"
)

SET(EXT_SRCS
  ${ServiceTools}/CppTools/src/bptypeutil.cpp
  ${ServiceTools}/CppTools/src/bpurlutil.cpp
  ${ServiceTools}/CppTools/src/bpservicedescription.cpp
  ${ServiceTools}/CppTools/src/bpserviceversion.cpp
)

SET(SRCS service.cpp Transformations.hh ImageProcessor.cpp util/fileutil.cpp)
SET(HDRS Transformations.cpp ImageProcessor.hh util/bpsync.hh util/bpthread.hh)

# add required OS libs here
SET(OSLIBS)

IF (WIN32)
   FILE(GLOB OS_SRCS "util/*_Windows.cpp")
   # these are separate libraries on winbloze
   SET(OSLIBS GraphicsMagickCoders_s GraphicsMagickFilters_s)
ELSE()
   FILE(GLOB OS_SRCS "util/*_Darwin.cpp" "util/*_UNIX.cpp")
ENDIF ()

ADD_LIBRARY(${serviceName} MODULE ${EXT_SRCS} ${SRCS} ${OS_SRCS} ${HDRS})

TARGET_LINK_LIBRARIES(${serviceName}
                      GraphicsMagick_s png_s jpeg_s zlib_s ${OSLIBS})

# create service directory
FILE(MAKE_DIRECTORY ${outputDir})

# copy in manifest
GET_TARGET_PROPERTY(loc ${serviceName} LOCATION)
GET_FILENAME_COMPONENT(ServiceLibrary "${loc}" NAME)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/manifest.json"
               "${outputDir}/manifest.json")  
ADD_CUSTOM_COMMAND(TARGET ${serviceName} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_if_different
                           \"${loc}\" \"${outputDir}\")

# copy in magic.mgk - an xml lookup table that allows graphics magick
# to quickly identify files based on content, and is required for GM
# to work
SET(pathToMagicMgk 
   "${CMAKE_CURRENT_SOURCE_DIR}/../external/built/share/GraphicsMagick-1.3.7/config/magic.mgk")
IF (NOT EXISTS ${pathToMagicMgk})
  MESSAGE(FATAL_ERROR "Can't find 'magick.mgk' file at '${pathToMagicMgk}'")
ENDIF ()

ADD_CUSTOM_COMMAND(
  TARGET ${serviceName} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different \"${pathToMagicMgk}\" \"${outputDir}\")

IF (APPLE)
  ADD_CUSTOM_COMMAND(TARGET ${serviceName} POST_BUILD
                     COMMAND strip -x \"${outputDir}/${ServiceLibrary}\")
ENDIF (APPLE)
