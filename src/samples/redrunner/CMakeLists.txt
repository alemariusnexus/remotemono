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
PROJECT(remotemono-test C CXX)


SET(SRCS "")

FIND_PATH(BLACKBONE_INCLUDE_DIRS BlackBone/Process/Process.h PATH_SUFFIXES BlackBone)
FIND_LIBRARY(BLACKBONE_LIBRARIES NAMES BlackBone libBlackBone)

FIND_LIBRARY(DIAGUIDS_LIBRARIES NAMES diaguids libdiaguids)


SET(SRCS
	main.cpp
	)


SET(LIBRARIES "")
SET(INCLUDES "")

# RemoteMono headers
SET(INCLUDES ${INCLUDES} "${CMAKE_CURRENT_SOURCE_DIR}/../..")

SET(LIBRARIES ${LIBRARIES} "${BLACKBONE_LIBRARIES}" "${DIAGUIDS_LIBRARIES}")
SET(INCLUDES ${INCLUDES} "${BLACKBONE_INCLUDE_DIRS}")


INCLUDE_DIRECTORIES(${INCLUDES})

ADD_EXECUTABLE(redrunner-sample ${SRCS})

SET_PROPERTY(TARGET redrunner-sample PROPERTY CXX_STANDARD 17)
TARGET_LINK_LIBRARIES(redrunner-sample ${LIBRARIES})
TARGET_PRECOMPILE_HEADERS(redrunner-sample PUBLIC pch.h)

IF(MSVC)
	TARGET_COMPILE_OPTIONS(redrunner-sample PRIVATE /bigobj)
ENDIF()

INSTALL(TARGETS redrunner-sample DESTINATION bin)
