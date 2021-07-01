# Copyright 2020 David "Alemarius Nexus" Lerch
# 
# This file is part of RemoteMono.
# 
# RemoteMono is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# RemoteMono is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public License
# along with RemoteMono.  If not, see <https://www.gnu.org/licenses/>. 

CMAKE_MINIMUM_REQUIRED(VERSION 3.13)
SET(CMAKE_USER_MAKE_RULES_OVERRIDE  "${CMAKE_CURRENT_SOURCE_DIR}/../InitFlags.cmake")
PROJECT(remotemono-test C CXX)

SET(REMOTEMONO_BACKEND_ENABLE_BLACKBONE ON CACHE BOOL "Whether to enable the BlackBone backend for RemoteMono")


ADD_SUBDIRECTORY(target)

LIST(TRANSFORM TARGET_SRCS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/target/")
LIST(TRANSFORM TARGET_CS_SRCS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/target/")
LIST(TRANSFORM TARGET_DUMMY_CS_SRCS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/target/")


SET(SRCS "")

FIND_PACKAGE(GTest REQUIRED)

IF(REMOTEMONO_BACKEND_ENABLE_BLACKBONE)
	FIND_PATH(BLACKBONE_INCLUDE_DIRS BlackBone/Process/Process.h PATH_SUFFIXES BlackBone)
	FIND_LIBRARY(BLACKBONE_LIBRARIES NAMES BlackBone libBlackBone)

	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		FIND_LIBRARY(DIAGUIDS_LIBRARIES NAMES diaguids libdiaguids HINTS "${BLACKBONE_INCLUDE_DIRS}/3rd_party/DIA/lib/amd64")
	else()
		FIND_LIBRARY(DIAGUIDS_LIBRARIES NAMES diaguids libdiaguids HINTS "${BLACKBONE_INCLUDE_DIRS}/3rd_party/DIA/lib")
	endif()
ENDIF()

FIND_PATH(MONO_ROOT bin/mono.exe PATH_SUFFIXES Mono)

FIND_PATH(MONO_INCLUDE_DIRS mono/metadata/class.h HINTS "${MONO_ROOT}/include/mono-2.0")
FIND_LIBRARY(MONO_LIBRARIES NAMES mono mono-2.0-sgen HINTS "${MONO_ROOT}/lib")
FIND_PROGRAM(MONO_MCS_EXECUTABLE NAMES mcs.bat mcs HINTS "${MONO_ROOT}/bin")


SET(SRCS
	main.cpp
	System.cpp
	TestBackend.cpp
	TestEnvException.cpp
	
	backend/BlackBoneTestBackend.cpp
	
	tests/IPCVector.cpp
	tests/RMonoHandle.cpp
	tests/MonoAPIDomain.cpp
	tests/MonoAPIAssembly.cpp
	tests/MonoAPIImage.cpp
	tests/MonoAPIMetadata.cpp
	tests/MonoAPIClass.cpp
	tests/MonoAPIField.cpp
	tests/MonoAPIMethod.cpp
	tests/MonoAPIProperty.cpp
	tests/MonoAPIObject.cpp
	tests/MonoAPIString.cpp
	tests/MonoAPIArray.cpp
	tests/MonoAPIMisc.cpp
	tests/RMonoHelpers.cpp
	)

FILE(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/includes")
FILE(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/includes/remotemono-test")
CONFIGURE_FILE(config.cmake.h "${CMAKE_CURRENT_BINARY_DIR}/includes/remotemono-test/config_generated.h")


SET(LIBRARIES "")
SET(INCLUDES "")

SET(INCLUDES "${CMAKE_CURRENT_BINARY_DIR}/includes" "${CMAKE_CURRENT_SOURCE_DIR}")

# RemoteMono headers
SET(INCLUDES ${INCLUDES} "${CMAKE_CURRENT_SOURCE_DIR}/..")

SET(LIBRARIES ${LIBRARIES} ${GTEST_LIBRARIES})
SET(INCLUDES ${INCLUDES} ${GTEST_INCLUDE_DIRS})

IF(REMOTEMONO_BACKEND_ENABLE_BLACKBONE)
	SET(LIBRARIES ${LIBRARIES} "${BLACKBONE_LIBRARIES}" "${DIAGUIDS_LIBRARIES}")
	SET(INCLUDES ${INCLUDES} "${BLACKBONE_INCLUDE_DIRS}")

	IF(NOT MSVC)
		STRING(APPEND CMAKE_CXX_STANDARD_LIBRARIES " -lshlwapi")
	ENDIF()
ENDIF()



IF(CMAKE_GENERATOR MATCHES "Visual Studio")
	# TODO: Choose proper configuration subdirectory
	SET(TARGET_MONO_ASSEMBLY "${CMAKE_CURRENT_BINARY_DIR}/Debug/remotemono-test-target-mono.dll")
	SET(TARGET_DUMMY_MONO_ASSEMBLY "${CMAKE_CURRENT_BINARY_DIR}/Debug/remotemono-test-target-dummy-mono.dll")
ELSE()
	SET(TARGET_MONO_ASSEMBLY "${CMAKE_CURRENT_BINARY_DIR}/remotemono-test-target-mono.dll")
	SET(TARGET_DUMMY_MONO_ASSEMBLY "${CMAKE_CURRENT_BINARY_DIR}/remotemono-test-target-dummy-mono.dll")
ENDIF()


ADD_CUSTOM_COMMAND (
	OUTPUT "${TARGET_MONO_ASSEMBLY}"
	COMMAND "${MONO_MCS_EXECUTABLE}" -nowarn:0414,0649 -target:library "-out:${TARGET_MONO_ASSEMBLY}" ${TARGET_CS_SRCS}
	DEPENDS ${TARGET_CS_SRCS}
	COMMENT "Building test target Mono assembly ..."
	VERBATIM
	)
ADD_CUSTOM_TARGET (
	remotemono-test-target-mono
	DEPENDS "${TARGET_MONO_ASSEMBLY}"
	)

ADD_CUSTOM_COMMAND (
	OUTPUT "${TARGET_DUMMY_MONO_ASSEMBLY}"
	COMMAND "${MONO_MCS_EXECUTABLE}" -target:library "-out:${TARGET_DUMMY_MONO_ASSEMBLY}" ${TARGET_DUMMY_CS_SRCS}
	DEPENDS ${TARGET_DUMMY_CS_SRCS}
	COMMENT "Building test target dummy Mono assembly ..."
	VERBATIM
	)
ADD_CUSTOM_TARGET (
	remotemono-test-target-dummy-mono
	DEPENDS "${TARGET_DUMMY_MONO_ASSEMBLY}"
	)


INCLUDE_DIRECTORIES(${MONO_INCLUDE_DIRS})

ADD_EXECUTABLE(remotemono-test-target ${TARGET_SRCS})
TARGET_LINK_LIBRARIES(remotemono-test-target ${MONO_LIBRARIES})
ADD_DEPENDENCIES (
	remotemono-test-target
	
	remotemono-test-target-mono
	remotemono-test-target-dummy-mono
	)



INCLUDE_DIRECTORIES(${INCLUDES})

ADD_EXECUTABLE(remotemono-test ${SRCS})

SET_PROPERTY(TARGET remotemono-test PROPERTY CXX_STANDARD 17)
TARGET_LINK_LIBRARIES(remotemono-test ${LIBRARIES})
TARGET_PRECOMPILE_HEADERS(remotemono-test PUBLIC pch.h)

IF(MSVC)
	TARGET_COMPILE_OPTIONS(remotemono-test PRIVATE /bigobj)
ELSEIF(MINGW)
	TARGET_COMPILE_OPTIONS(remotemono-test PRIVATE -Wa,-mbig-obj)
ENDIF()


INSTALL (
	TARGETS remotemono-test remotemono-test-target #remotemono-test-target-mono remotemono-test-target-dummy-mono
	DESTINATION bin
	)
INSTALL (
	PROGRAMS "${TARGET_MONO_ASSEMBLY}" "${TARGET_DUMMY_MONO_ASSEMBLY}"
	TYPE BIN
	)
