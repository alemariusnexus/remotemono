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
PROJECT(remotemono C CXX)

INCLUDE(CPack)

SET(REMOTEMONO_TEST_BUILD OFF CACHE BOOL "Whether you want to build the unit tests")
SET(REMOTEMONO_SAMPLES_BUILD OFF CACHE BOOL "Whether you want to build the example projects")
SET(REMOTEMONO_DOCS_BUILD OFF CACHE BOOL "Whether you want to build the documentation")

IF(REMOTEMONO_TEST_BUILD)
    ADD_SUBDIRECTORY(remotemono-test)
ENDIF(REMOTEMONO_TEST_BUILD)

IF(REMOTEMONO_SAMPLES_BUILD)
    ADD_SUBDIRECTORY(samples)
ENDIF(REMOTEMONO_SAMPLES_BUILD)

IF(REMOTEMONO_DOCS_BUILD)
	FIND_PACKAGE(Doxygen REQUIRED dot)
	
	SET(DOXYGEN_RECURSIVE FALSE)
	SET(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/doc")
	
	DOXYGEN_ADD_DOCS (
		doxygen
		"${CMAKE_CURRENT_SOURCE_DIR}/remotemono"
		"${CMAKE_CURRENT_SOURCE_DIR}/remotemono/impl"
		"${CMAKE_CURRENT_SOURCE_DIR}/../README.md"
		COMMENT "Generating Doxygen documentation"
		)
ENDIF()


INSTALL (
	DIRECTORY remotemono
	TYPE INCLUDE
	FILES_MATCHING
		PATTERN *.h
		PATTERN *.hpp
	)

IF(REMOTEMONO_DOCS_BUILD)
	INSTALL (
		DIRECTORY "${DOXYGEN_OUTPUT_DIRECTORY}/"
		TYPE DOC
		)
ENDIF()
