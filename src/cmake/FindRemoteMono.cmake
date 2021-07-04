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

SET(_RemoteMonoNameCands remotemono)
FOREACH(_namecand ${_RemoteMonoNameCands})
    IF(EXISTS "${CMAKE_SOURCE_DIR}/${_namecand}/src/CMakeLists.txt")
        SET(_RemoteMonoBundledDir "${CMAKE_SOURCE_DIR}/${_namecand}/src")
        BREAK()
    ENDIF()
ENDFOREACH()

IF(_RemoteMonoBundledDir)

    IF(NOT TARGET RemoteMono::RemoteMono)
        MESSAGE(STATUS "Building bundled RemoteMono: ${_RemoteMonoBundledDir}")
        ADD_SUBDIRECTORY("${_RemoteMonoBundledDir}" "${CMAKE_BINARY_DIR}/remotemono")

        SET(RemoteMono_INCLUDE_DIR "${_RemoteMonoBundledDir}")
        SET(RemoteMono_CMAKE_MODULE_DIR "${_RemoteMonoBundledDir}/cmake")

        INCLUDE(FindPackageHandleStandardArgs)
        FIND_PACKAGE_HANDLE_STANDARD_ARGS(RemoteMono
                REQUIRED_VARS RemoteMono_INCLUDE_DIR RemoteMono_CMAKE_MODULE_DIR
                )

        IF(RemoteMono_FOUND)
            SET(RemoteMono_INCLUDE_DIRS "${RemoteMono_INCLUDE_DIR}")

            ADD_LIBRARY(RemoteMono::RemoteMono INTERFACE IMPORTED GLOBAL)
            SET_TARGET_PROPERTIES(RemoteMono::RemoteMono PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES "${RemoteMono_INCLUDE_DIR}"
                    )
        ENDIF()
    ENDIF()

ELSE()

    FIND_PATH(RemoteMono_INCLUDE_DIR
            remotemono/RMonoAPI.h
            )
    FIND_PATH(RemoteMono_CMAKE_MODULE_DIR
            FindBlackBone.cmake
            HINTS "${RemoteMono_INCLUDE_DIR}/cmake"
            )

    MARK_AS_ADVANCED(RemoteMono_INCLUDE_DIR RemoteMono_CMAKE_MODULE_DIR)

    INCLUDE(FindPackageHandleStandardArgs)
    FIND_PACKAGE_HANDLE_STANDARD_ARGS(RemoteMono
            REQUIRED_VARS RemoteMono_INCLUDE_DIR RemoteMono_CMAKE_MODULE_DIR
            )

    IF(RemoteMono_FOUND)
        SET(RemoteMono_INCLUDE_DIRS "${RemoteMono_INCLUDE_DIR}")

        ADD_LIBRARY(RemoteMono::RemoteMono INTERFACE IMPORTED)
        SET_TARGET_PROPERTIES(RemoteMono::RemoteMono PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${RemoteMono_INCLUDE_DIR}"
                )
    ENDIF()

ENDIF()
