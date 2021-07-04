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

FIND_PATH(Mono_ROOT_DIR
        bin/mono.exe
        PATH_SUFFIXES Mono
        )
FIND_PATH(Mono_INCLUDE_DIR
        mono/metadata/class.h
        HINTS "${Mono_ROOT_DIR}/include/mono-2.0"
        )
FIND_LIBRARY(Mono_LIBRARY
        NAMES mono mono-2.0-sgen
        HINTS "${Mono_ROOT_DIR}/lib"
        )
FIND_FILE(Mono_SHARED_LIBRARY
        NAMES mono.dll mono-2.0-sgen.dll
        HINTS "${Mono_ROOT_DIR}/bin"
        )
FIND_PROGRAM(Mono_MCS_EXECUTABLE
        NAMES mcs.bat mcs
        HINTS "${Mono_ROOT_DIR}/bin"
        )
FIND_FILE(Mono_MSCORLIB_LIBRARY
        NAMES mscorlib.dll
        HINTS "${Mono_ROOT_DIR}/lib/mono/4.5"
        )

MARK_AS_ADVANCED(Mono_ROOT_DIR Mono_INCLUDE_DIR Mono_LIBRARY Mono_MCS_EXECUTABLE Mono_MSCORLIB_LIBRARY)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Mono
        REQUIRED_VARS Mono_INCLUDE_DIR Mono_LIBRARY Mono_MCS_EXECUTABLE Mono_MSCORLIB_LIBRARY
        )

IF(Mono_FOUND)
    SET(Mono_LIBRARIES "${Mono_LIBRARY}")
    SET(Mono_INCLUDE_DIRS "${Mono_INCLUDE_DIR}")

    ADD_LIBRARY(Mono::Mono UNKNOWN IMPORTED)
    SET_TARGET_PROPERTIES(Mono::Mono PROPERTIES
            IMPORTED_LOCATION "${Mono_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${Mono_INCLUDE_DIR}"
            )

    ADD_EXECUTABLE(Mono::MCS IMPORTED)
    SET_TARGET_PROPERTIES(Mono::MCS PROPERTIES
            IMPORTED_LOCATION "${Mono_MCS_EXECUTABLE}"
            )
ENDIF()
