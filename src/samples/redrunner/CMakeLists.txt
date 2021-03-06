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
SET(CMAKE_USER_MAKE_RULES_OVERRIDE  "${CMAKE_CURRENT_SOURCE_DIR}/../../InitFlags.cmake")
PROJECT(remotemono-test C CXX)


SET(SRCS "")


SET(SRCS
	main.cpp
	)

CONFIGURE_FILE(config.cmake.h "${CMAKE_CURRENT_BINARY_DIR}/includes/samples/redrunner/config_generated.h")


SET(LIBRARIES "")
SET(INCLUDES "")

# RemoteMono headers
SET(INCLUDES ${INCLUDES} "${CMAKE_CURRENT_BINARY_DIR}/includes" "${CMAKE_CURRENT_SOURCE_DIR}/../..")

IF(REMOTEMONO_BACKEND_ENABLE_BLACKBONE)
	LIST(APPEND LIBRARIES BlackBone::BlackBone)
ENDIF()


INCLUDE_DIRECTORIES(${INCLUDES})

ADD_EXECUTABLE(redrunner-sample ${SRCS})

SET_PROPERTY(TARGET redrunner-sample PROPERTY CXX_STANDARD 17)
TARGET_LINK_LIBRARIES(redrunner-sample ${LIBRARIES})
TARGET_PRECOMPILE_HEADERS(redrunner-sample PUBLIC pch.h)

IF(MSVC)
	TARGET_COMPILE_OPTIONS(redrunner-sample PRIVATE /bigobj)
ELSEIF(MINGW)
	TARGET_COMPILE_OPTIONS(redrunner-sample PRIVATE -Wa,-mbig-obj)
ENDIF()

INSTALL(TARGETS redrunner-sample DESTINATION bin)
