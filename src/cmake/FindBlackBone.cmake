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

SET(_BlackBoneNameCands BlackBone Blackbone blackbone)
FOREACH(_namecand ${_BlackBoneNameCands})
    IF(EXISTS "${CMAKE_SOURCE_DIR}/${_namecand}/src/CMakeLists.txt")
        SET(_BlackBoneBundledDir "${CMAKE_SOURCE_DIR}/${_namecand}/src")
        BREAK()
    ENDIF()
ENDFOREACH()

IF(_BlackBoneBundledDir)

    IF(NOT TARGET BlackBone::BlackBone)
        MESSAGE(STATUS "Building bundled BlackBone: ${_BlackBoneBundledDir}")
        ADD_SUBDIRECTORY("${_BlackBoneBundledDir}" "${CMAKE_BINARY_DIR}/blackbone")

        SET(BlackBone_INCLUDE_DIR "${_BlackBoneBundledDir}")
        SET(BlackBone_LIBRARY BlackBone)

        IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
            SET(BlackBone_DIAGUIDS_LIBRARY "${BlackBone_INCLUDE_DIR}/3rd_party/DIA/lib/amd64/diaguids.lib")
        ELSE()
            SET(BlackBone_DIAGUIDS_LIBRARY "${BlackBone_INCLUDE_DIR}/3rd_party/DIA/lib/diaguids.lib")
        ENDIF()

        INCLUDE(FindPackageHandleStandardArgs)
        FIND_PACKAGE_HANDLE_STANDARD_ARGS(BlackBone
                REQUIRED_VARS BlackBone_INCLUDE_DIR BlackBone_LIBRARY BlackBone_DIAGUIDS_LIBRARY
                )

        IF(BlackBone_FOUND)
            SET(BlackBone_LIBRARIES "${BlackBone_LIBRARY}" "${BlackBone_DIAGUIDS_LIBRARY}")
            SET(BlackBone_INCLUDE_DIRS "${BlackBone_INCLUDE_DIR}")

            TARGET_INCLUDE_DIRECTORIES(BlackBone INTERFACE "${BlackBone_INCLUDE_DIR}")
            ADD_LIBRARY(BlackBone::BlackBoneLib ALIAS BlackBone)

            ADD_LIBRARY(BlackBone::DIAGUIDS UNKNOWN IMPORTED GLOBAL)
            SET_TARGET_PROPERTIES(BlackBone::DIAGUIDS PROPERTIES
                    IMPORTED_LOCATION "${BlackBone_DIAGUIDS_LIBRARY}"
                    )

            ADD_LIBRARY(BlackBone::BlackBone INTERFACE IMPORTED GLOBAL)
            SET_PROPERTY(TARGET BlackBone::BlackBone
                    PROPERTY INTERFACE_LINK_LIBRARIES BlackBone::BlackBoneLib BlackBone::DIAGUIDS
                    )
        ENDIF()
    ENDIF()

ELSE()

    FIND_PATH(BlackBone_INCLUDE_DIR
            BlackBone/Process/Process.h
            PATH_SUFFIXES BlackBone
            )
    FIND_LIBRARY(BlackBone_LIBRARY
            NAMES BlackBone libBlackBone
            )

    SET(_diaguids_hints "")
    IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
        LIST(APPEND _diaguids_hints "${BlackBone_INCLUDE_DIR}/3rd_party/DIA/lib/amd64")
    ELSE()
        LIST(APPEND _diaguids_hints "${BlackBone_INCLUDE_DIR}/3rd_party/DIA/lib")
    ENDIF()

    FIND_LIBRARY(BlackBone_DIAGUIDS_LIBRARY
            NAMES diaguids libdiaguids
            HINTS "${_diaguids_hints}"
            )

    MARK_AS_ADVANCED(BlackBone_INCLUDE_DIR BlackBone_LIBRARY BlackBone_DIAGUIDS_LIBRARY)

    INCLUDE(FindPackageHandleStandardArgs)
    FIND_PACKAGE_HANDLE_STANDARD_ARGS(BlackBone
            REQUIRED_VARS BlackBone_INCLUDE_DIR BlackBone_LIBRARY BlackBone_DIAGUIDS_LIBRARY
            )

    IF(BlackBone_FOUND)
        SET(BlackBone_LIBRARIES "${BlackBone_LIBRARY}" "${BlackBone_DIAGUIDS_LIBRARY}")
        SET(BlackBone_INCLUDE_DIRS "${BlackBone_INCLUDE_DIR}")

        ADD_LIBRARY(BlackBone::BlackBoneLib UNKNOWN IMPORTED)
        SET_TARGET_PROPERTIES(BlackBone::BlackBoneLib PROPERTIES
                IMPORTED_LOCATION "${BlackBone_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${BlackBone_INCLUDE_DIR}"
                )

        ADD_LIBRARY(BlackBone::DIAGUIDS UNKNOWN IMPORTED)
        SET_TARGET_PROPERTIES(BlackBone::DIAGUIDS PROPERTIES
                IMPORTED_LOCATION "${BlackBone_DIAGUIDS_LIBRARY}"
                )

        ADD_LIBRARY(BlackBone::BlackBone INTERFACE IMPORTED)
        SET_PROPERTY(TARGET BlackBone::BlackBone
                PROPERTY INTERFACE_LINK_LIBRARIES BlackBone::BlackBoneLib BlackBone::DIAGUIDS
                )
    ENDIF()

ENDIF()
