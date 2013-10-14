# From lighttpd2
#
# Copyright (c) 2004-2008 Jan Kneschke
# Copyright (c) 2008-2010 Stefan Bühler
# Copyright (c) 2008-2010 Thomas Porzelt
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

IF(NOT RAGEL_EXECUTABLE)
	MESSAGE(STATUS "Looking for ragel")
	FIND_PROGRAM(RAGEL_EXECUTABLE ragel)
	IF(RAGEL_EXECUTABLE)
		EXECUTE_PROCESS(COMMAND "${RAGEL_EXECUTABLE}" -v OUTPUT_VARIABLE _version)
		STRING(REGEX MATCH "[0-9.]+" RAGEL_VERSION ${_version})
		SET(RAGEL_FOUND TRUE)
	ENDIF(RAGEL_EXECUTABLE)
ELSE(NOT RAGEL_EXECUTABLE)
	EXECUTE_PROCESS(COMMAND "${RAGEL_EXECUTABLE}" -v OUTPUT_VARIABLE _version)
	STRING(REGEX MATCH "[0-9.]+" RAGEL_VERSION ${_version})
	SET(RAGEL_FOUND TRUE)
ENDIF(NOT RAGEL_EXECUTABLE)

IF(RAGEL_FOUND)
	IF (NOT Ragel_FIND_QUIETLY)
		MESSAGE(STATUS "Found ragel: ${RAGEL_EXECUTABLE} (${RAGEL_VERSION})")
	ENDIF (NOT Ragel_FIND_QUIETLY)

	IF(NOT RAGEL_FLAGS)
		SET(RAGEL_FLAGS "-T1")
	ENDIF(NOT RAGEL_FLAGS)

	MACRO(RAGEL_PARSER SRCFILE)
		GET_FILENAME_COMPONENT(SRCPATH "${SRCFILE}" PATH)
		GET_FILENAME_COMPONENT(SRCBASE "${SRCFILE}" NAME_WE)
		SET(OUTFILE "${CMAKE_CURRENT_BINARY_DIR}/${SRCPATH}/${SRCBASE}.cc")
		FILE(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${SRCPATH}")
		SET(INFILE "${CMAKE_CURRENT_SOURCE_DIR}/${SRCFILE}")
		SET(_flags ${ARGV1})
		IF(NOT _flags)
			SET(_flags ${RAGEL_FLAGS})
		ENDIF(NOT _flags)
		ADD_CUSTOM_COMMAND(OUTPUT ${OUTFILE}
			COMMAND "${RAGEL_EXECUTABLE}"
			ARGS -C ${_flags} -o "${OUTFILE}" "${INFILE}"
			DEPENDS "${INFILE}"
			COMMENT "Generating ${SRCBASE}.cc from ${SRCFILE}"
		)
	ENDMACRO(RAGEL_PARSER)

ELSE(RAGEL_FOUND)

	IF(Ragel_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "Could not find ragel")
	ENDIF(Ragel_FIND_REQUIRED)
ENDIF(RAGEL_FOUND)
