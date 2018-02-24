cmake_minimum_required(VERSION 3.3)
cmake_policy(SET CMP0054 NEW)

# [Autorevision.cmake]
#
# CMake version of `autorevision` bash script:
# Copyright (c) 2012 - 2013 dak180 and contributors. See
# http://opensource.org/licenses/mit-license.php or the included
# COPYING.md for licence terms.
#
# autorevision.CMake:
# Copyright © 2018 pastdue ( https://github.com/past-due/ ) and contributors
# License: MIT License ( https://opensource.org/licenses/MIT )
#
#
# To call, define the following variables as appropriate:
# OUTPUT_TYPE <h | sh>
# OUTPUT_FILE <file>
# CACHEFILE <file>
# CACHEFORCE
# SKIPUPDATECACHE
# VAROUT (output all variable names and values to stdout; quiets other output)
#
# To ensure that this is run at *build* time, this script should be run in a custom command / target
#

if(DEFINED VAROUT)
	set(LOGGING_QUIET ON)
else()
	set(LOGGING_QUIET OFF)
endif()

if(NOT LOGGING_QUIET)
#	message( STATUS "Autorevision.cmake" )
#	message( STATUS "OUTPUT_TYPE=${OUTPUT_TYPE}" )
#	message( STATUS "OUTPUT_FILE=${OUTPUT_FILE}" )
#	message( STATUS "CACHEFILE=${CACHEFILE}" )
#	message( STATUS "CACHEFORCE=${CACHEFORCE}" )
#	message( STATUS "SKIPUPDATECACHE=${SKIPUPDATECACHE}" )
endif()

function(copy_if_different inputfile outputfile)
	if(EXISTS "${outputfile}")
		file(SHA256 "${outputfile}" old_output_hash)
		file(TIMESTAMP "${outputfile}" old_output_timestamp)
	endif()
	execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different "${inputfile}" "${outputfile}" OUTPUT_QUIET ERROR_QUIET)
	if(EXISTS "${outputfile}")
		file(SHA256 "${outputfile}" new_output_hash)
		file(TIMESTAMP "${outputfile}" new_output_timestamp)
	endif()

	if((NOT "${old_output_hash}" STREQUAL "${new_output_hash}") OR (NOT "${old_output_timestamp}" STREQUAL "${new_output_timestamp}"))
		# Files are different - copied input to output!
		set(DID_COPY_WAS_DIFFERENT ON PARENT_SCOPE)
	else()
		unset(DID_COPY_WAS_DIFFERENT PARENT_SCOPE)
	endif()
endfunction()

macro(_hOutput _outputFile _quiet)
	set(_hContents)
	string(CONCAT _hContents ${_hContents} "/* Generated by autorevision.cmake - do not hand-hack! */\n")
	string(CONCAT _hContents ${_hContents} "#ifndef AUTOREVISION_H\n")
	string(CONCAT _hContents ${_hContents} "#define AUTOREVISION_H\n")
	string(CONCAT _hContents ${_hContents} "\n")
	string(CONCAT _hContents ${_hContents} "#define VCS_TYPE		\"${VCS_TYPE}\"\n")
	string(CONCAT _hContents ${_hContents} "#define VCS_BASENAME	\"${VCS_BASENAME}\"\n")
	string(CONCAT _hContents ${_hContents} "#define VCS_BRANCH		\"${VCS_BRANCH}\"\n")
	string(CONCAT _hContents ${_hContents} "#define VCS_TAG			\"${VCS_TAG}\"\n")
	string(CONCAT _hContents ${_hContents} "#define VCS_EXTRA       \"${VCS_EXTRA}\"\n")
	string(CONCAT _hContents ${_hContents} "\n")
	string(CONCAT _hContents ${_hContents} "#define VCS_FULL_HASH		\"${VCS_FULL_HASH}\"\n")
	string(CONCAT _hContents ${_hContents} "#define VCS_SHORT_HASH		\"${VCS_SHORT_HASH}\"\n")
	string(CONCAT _hContents ${_hContents} "\n")
	string(CONCAT _hContents ${_hContents} "#define VCS_WC_MODIFIED		${VCS_WC_MODIFIED}\n")
	string(CONCAT _hContents ${_hContents} "\n")
	string(CONCAT _hContents ${_hContents} "#define VCS_COMMIT_COUNT	${VCS_COMMIT_COUNT}\n")
	string(CONCAT _hContents ${_hContents} "#define VCS_MOST_RECENT_TAGGED_VERSION	\"${VCS_MOST_RECENT_TAGGED_VERSION}\"\n")
	string(CONCAT _hContents ${_hContents} "#define VCS_COMMIT_COUNT_SINCE_MOST_RECENT_TAGGED_VERSION	${VCS_COMMIT_COUNT_SINCE_MOST_RECENT_TAGGED_VERSION}\n")
	string(CONCAT _hContents ${_hContents} "#define VCS_COMMIT_COUNT_ON_MASTER_UNTIL_BRANCH	${VCS_COMMIT_COUNT_ON_MASTER_UNTIL_BRANCH}\n")
	string(CONCAT _hContents ${_hContents} "#define VCS_BRANCH_COMMIT_COUNT	${VCS_BRANCH_COMMIT_COUNT}\n")
	string(CONCAT _hContents ${_hContents} "\n")
	string(CONCAT _hContents ${_hContents} "#endif\n")
	string(CONCAT _hContents ${_hContents} "\n")
	string(CONCAT _hContents ${_hContents} "/* end */\n")

	file(WRITE "${_outputFile}.new" ${_hContents})
	copy_if_different("${_outputFile}.new" "${_outputFile}")
	file(REMOVE "${_outputFile}.new")

	if(DID_COPY_WAS_DIFFERENT AND NOT _quiet)
		message( STATUS "Output H format to: ${_outputFile}" )
		message( STATUS "${_hContents}" )
	endif()
	unset(DID_COPY_WAS_DIFFERENT)
	unset(_hContents)
endmacro()

macro(_shOutput _outputFile _quiet)
	set(_hContents)
	string(CONCAT _hContents ${_hContents} "# Generated by autorevision.cmake - do not hand-hack!\n")
	string(CONCAT _hContents ${_hContents} "\n")
	string(CONCAT _hContents ${_hContents} "VCS_TYPE=\"${VCS_TYPE}\"\n")
	string(CONCAT _hContents ${_hContents} "VCS_BASENAME=\"${VCS_BASENAME}\"\n")
	string(CONCAT _hContents ${_hContents} "VCS_BRANCH=\"${VCS_BRANCH}\"\n")
	string(CONCAT _hContents ${_hContents} "VCS_TAG=\"${VCS_TAG}\"\n")
	string(CONCAT _hContents ${_hContents} "VCS_EXTRA=\"${VCS_EXTRA}\"\n")
	string(CONCAT _hContents ${_hContents} "\n")
	string(CONCAT _hContents ${_hContents} "VCS_FULL_HASH=\"${VCS_FULL_HASH}\"\n")
	string(CONCAT _hContents ${_hContents} "VCS_SHORT_HASH=\"${VCS_SHORT_HASH}\"\n")
	string(CONCAT _hContents ${_hContents} "\n")
	string(CONCAT _hContents ${_hContents} "VCS_WC_MODIFIED=${VCS_WC_MODIFIED}\n")
	string(CONCAT _hContents ${_hContents} "\n")
	string(CONCAT _hContents ${_hContents} "VCS_COMMIT_COUNT=${VCS_COMMIT_COUNT}\n")
	string(CONCAT _hContents ${_hContents} "VCS_MOST_RECENT_TAGGED_VERSION=\"${VCS_MOST_RECENT_TAGGED_VERSION}\"\n")
	string(CONCAT _hContents ${_hContents} "VCS_COMMIT_COUNT_SINCE_MOST_RECENT_TAGGED_VERSION=${VCS_COMMIT_COUNT_SINCE_MOST_RECENT_TAGGED_VERSION}\n")
	string(CONCAT _hContents ${_hContents} "VCS_COMMIT_COUNT_ON_MASTER_UNTIL_BRANCH=${VCS_COMMIT_COUNT_ON_MASTER_UNTIL_BRANCH}\n")
	string(CONCAT _hContents ${_hContents} "VCS_BRANCH_COMMIT_COUNT=${VCS_BRANCH_COMMIT_COUNT}\n")
	string(CONCAT _hContents ${_hContents} "\n")
	string(CONCAT _hContents ${_hContents} "# end\n")

	file(WRITE "${_outputFile}.new" ${_hContents})
	copy_if_different("${_outputFile}.new" "${_outputFile}")
	file(REMOVE "${_outputFile}.new")

	if(DID_COPY_WAS_DIFFERENT AND NOT _quiet)
		message( STATUS "Output SH format to: ${_outputFile}" )
	endif()
	unset(DID_COPY_WAS_DIFFERENT)
	unset(_hContents)
endmacro()

macro(_importCache _cacheFile _quiet)
	file(STRINGS "${_cacheFile}" _cacheFileContents REGEX "^[^#].*")
	foreach(_cacheFileLine ${_cacheFileContents})
		STRING(REGEX MATCH "([^=]+)=\"?([^\"]*)\"?" _line_Matched "${_cacheFileLine}")
		if(DEFINED CMAKE_MATCH_1 AND NOT "${CMAKE_MATCH_1}" STREQUAL "")
			if(DEFINED CMAKE_MATCH_2)
				# message( STATUS "set(${CMAKE_MATCH_1} \"${CMAKE_MATCH_2}\")" )
				set(${CMAKE_MATCH_1} "${CMAKE_MATCH_2}")
			else()
				# message( STATUS "set(${CMAKE_MATCH_1} \"\")" )
				set(${CMAKE_MATCH_1} "")
			endif()
		endif()
	endforeach()
endmacro()

function(extractVersionNumberFromGitTag _gitTag)
	set(version_tag ${_gitTag})

	# Remove "v/" or "v" prefix (as in "v3.2.2"), if present
	STRING(REGEX REPLACE "^v/" "" version_tag ${version_tag})
	STRING(REGEX REPLACE "^v" "" version_tag ${version_tag})

	# Remove _beta* or _rc* suffix, if present
	STRING(REGEX REPLACE "_beta.*$" "" version_tag ${version_tag})
	STRING(REGEX REPLACE "_rc.*$" "" version_tag ${version_tag})

	# Extract up to a 3-component version # from the beginning of the tag (i.e. 3.2.2)
	STRING(REGEX MATCH "^[0-9]+(.[0-9]+)?(.[0-9]+)?" version_tag ${version_tag})
	if(version_tag)
		set(DID_EXTRACT_VERSION ON PARENT_SCOPE)
		set(EXTRACTED_VERSION ${version_tag} PARENT_SCOPE)
	else()
		set(DID_EXTRACT_VERSION OFF PARENT_SCOPE)
		unset(EXTRACTED_VERSION PARENT_SCOPE)
	endif()
endfunction()

macro(_gitRepo)

	execute_process( COMMAND ${GIT_EXECUTABLE} rev-parse --show-toplevel
					 OUTPUT_VARIABLE _repo_top_directory
					 OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
	if(NOT _repo_top_directory)
		message( FATAL_ERROR "Failed to get top level repo path." )
		return()
	endif()

	set(VCS_TYPE "git")
	# VCS_BASENAME="$(basename "${PWD}")")
	get_filename_component(VCS_BASENAME "${_repo_top_directory}" NAME)

	# Check if working copy is clean, however, we will ignore any and all po files
	# when we determine if modifications were done.
	# git update-index --assume-unchanged po/*.po
	execute_process( COMMAND ${GIT_EXECUTABLE} update-index --assume-unchanged po/*.po
					 WORKING_DIRECTORY "${_repo_top_directory}" OUTPUT_QUIET ERROR_QUIET)
	# test -z "$(git status --untracked-files=no --porcelain)"
	execute_process( COMMAND ${GIT_EXECUTABLE} status --untracked-files=no --porcelain
					 WORKING_DIRECTORY "${_repo_top_directory}"
					 OUTPUT_VARIABLE _git_status OUTPUT_STRIP_TRAILING_WHITESPACE)
	string(LENGTH "${_git_status}" _git_status_length)
	if(_git_status_length GREATER 0)
		set(VCS_WC_MODIFIED 1)
	else()
		set(VCS_WC_MODIFIED 0)
	endif()
	unset(_git_status_length)
	unset(_git_status)
	# now, reset index back to normal
	# git update-index --no-assume-unchanged po/*.po
	execute_process( COMMAND ${GIT_EXECUTABLE} update-index --no-assume-unchanged po/*.po
					 WORKING_DIRECTORY "${_repo_top_directory}" OUTPUT_QUIET ERROR_QUIET)

	# The full revision hash
	# VCS_FULL_HASH="$(git rev-parse HEAD)"
	execute_process( COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
					 WORKING_DIRECTORY "${_repo_top_directory}"
					 OUTPUT_VARIABLE VCS_FULL_HASH
					 OUTPUT_STRIP_TRAILING_WHITESPACE )

	# The short hash
	# VCS_SHORT_HASH="$(echo "${VCS_FULL_HASH}" | cut -b 1-7)"
	string(SUBSTRING ${VCS_FULL_HASH} 0 7 VCS_SHORT_HASH)

	# Current branch (if we are on a branch...)
	# VCS_BRANCH="$(git symbolic-ref --short -q HEAD)"
	execute_process( COMMAND ${GIT_EXECUTABLE} symbolic-ref --short -q HEAD
					 WORKING_DIRECTORY "${_repo_top_directory}"
					 OUTPUT_VARIABLE VCS_BRANCH
					 OUTPUT_STRIP_TRAILING_WHITESPACE )

	# Check if we are on a tag
	# VCS_TAG="$(git describe --exact-match 2> /dev/null)"
	execute_process( COMMAND ${GIT_EXECUTABLE} describe --exact-match
					 WORKING_DIRECTORY "${_repo_top_directory}"
					 OUTPUT_VARIABLE VCS_TAG
					 OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)

	# get some extra data in case we are not on a branch or a tag...
	# VCS_EXTRA="$(git describe 2> /dev/null)"
	execute_process( COMMAND ${GIT_EXECUTABLE} describe
					 WORKING_DIRECTORY "${_repo_top_directory}"
					 OUTPUT_VARIABLE VCS_EXTRA
					 OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)

	# get the # of commits in the current history
	# IMPORTANT: This value is incorrect if operating from a shallow clone
	# VCS_COMMIT_COUNT="$(git rev-list --count HEAD 2> /dev/null)"
	execute_process( COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD
					 WORKING_DIRECTORY "${_repo_top_directory}"
					 OUTPUT_VARIABLE VCS_COMMIT_COUNT
					 OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)

	set(tag_skip 0)
	while(tag_skip LESS 5000)
		# revision="$(git rev-list --tags --skip=${tag_skip} --max-count=1 2> /dev/null)"
		execute_process( COMMAND ${GIT_EXECUTABLE} rev-list --tags --skip=${tag_skip} --max-count=1
						 WORKING_DIRECTORY "${_repo_top_directory}"
						 RESULT_VARIABLE exstatus
						 OUTPUT_VARIABLE revision
						 OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
		if((NOT exstatus EQUAL 0) OR ($revision STREQUAL ""))
			break()
		endif()
		# current_tag="$(git describe --abbrev=0 --tags "${revision}" 2> /dev/null)"
		execute_process( COMMAND ${GIT_EXECUTABLE} describe --abbrev=0 --tags "${revision}"
						 WORKING_DIRECTORY "${_repo_top_directory}"
						 RESULT_VARIABLE exstatus
						 OUTPUT_VARIABLE current_tag
						 OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
		if((NOT exstatus EQUAL 0) OR ($current_tag STREQUAL ""))
			break()
		endif()
		extractVersionNumberFromGitTag("${current_tag}")
		if(DID_EXTRACT_VERSION)
			# Found a tag that looks like a version number
			set(VCS_MOST_RECENT_TAGGED_VERSION "${current_tag}")
			break()
		endif()
		MATH(EXPR tag_skip "${tag_skip}+1")
	endwhile()

	if(tag_skip EQUAL 5000)
		message( WARNING "Something went wrong trying to obtain the most recent tagged version from Git." )
	endif()
	unset(tag_skip)

	if(VCS_MOST_RECENT_TAGGED_VERSION)
		# VCS_COMMIT_COUNT_SINCE_MOST_RECENT_TAGGED_VERSION="$(git rev-list --count ${VCS_MOST_RECENT_TAGGED_VERSION}.. 2> /dev/null)"
		execute_process( COMMAND ${GIT_EXECUTABLE} rev-list --count ${VCS_MOST_RECENT_TAGGED_VERSION}..
						 WORKING_DIRECTORY "${_repo_top_directory}"
						 OUTPUT_VARIABLE VCS_COMMIT_COUNT_SINCE_MOST_RECENT_TAGGED_VERSION
						 OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
	endif()

	# get the commit count on this branch *since* the branch from master
	# VCS_BRANCH_COMMIT_COUNT="$(git rev-list --count master.. 2> /dev/null)"
	execute_process( COMMAND ${GIT_EXECUTABLE} rev-list --count master..
						 WORKING_DIRECTORY "${_repo_top_directory}"
						 OUTPUT_VARIABLE VCS_BRANCH_COMMIT_COUNT
						 OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)

	# get the commit count on master until the branch
	# first_new_commit_on_branch_since_master="$(git rev-list master.. | tail -n 1)"
	execute_process( COMMAND ${GIT_EXECUTABLE} rev-list master..
						 WORKING_DIRECTORY "${_repo_top_directory}"
						 RESULT_VARIABLE exstatus
						 OUTPUT_VARIABLE _commits_since_master_branch
						 OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
	if(exstatus EQUAL 0)
		STRING(REGEX REPLACE "\n" ";" _commits_since_master_branch "${_commits_since_master_branch}")
		if(_commits_since_master_branch)
			list(GET _commits_since_master_branch -1 first_new_commit_on_branch_since_master)
		else()
			set(first_new_commit_on_branch_since_master)
		endif()
		if(("${first_new_commit_on_branch_since_master}" STREQUAL "") AND (DEFINED VCS_BRANCH_COMMIT_COUNT) AND (VCS_BRANCH_COMMIT_COUNT EQUAL 0))
			# The call succeeded, but git returned nothing
			# The number of commits since master is 0, so set VCS_COMMIT_COUNT_ON_MASTER_UNTIL_BRANCH
			# to be equal to VCS_COMMIT_COUNT
			set(VCS_COMMIT_COUNT_ON_MASTER_UNTIL_BRANCH "${VCS_COMMIT_COUNT}")
		else()
			# VCS_COMMIT_COUNT_ON_MASTER_UNTIL_BRANCH="$(git rev-list --count ${first_new_commit_on_branch_since_master}^ 2> /dev/null)"
			execute_process( COMMAND ${GIT_EXECUTABLE} rev-list --count ${first_new_commit_on_branch_since_master}^
							 WORKING_DIRECTORY "${_repo_top_directory}"
							 OUTPUT_VARIABLE VCS_COMMIT_COUNT_ON_MASTER_UNTIL_BRANCH
							 OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
		endif()
	endif()

	# cleanup
	unset(first_new_commit_on_branch_since_master)
	unset(exstatus)
	unset(_commits_since_master_branch)
	unset(_repo_top_directory)
endmacro()

if(LOGGING_QUIET)
	find_package(Git QUIET)
else()
	find_package(Git)
endif()
if(Git_FOUND)
	execute_process( COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
					 OUTPUT_VARIABLE _test_isCurrentDirectoryInGitRepo_Output
					 OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
	string(LENGTH "${_test_isCurrentDirectoryInGitRepo_Output}" _test_isCurrentDirectoryInGitRepo_Output_Len)
	if(_test_isCurrentDirectoryInGitRepo_Output_Len GREATER 0)
		set(_currentDirectoryIsInGitRepo ON)
	endif()
endif()


# Detect and collect repo data.
if(CACHEFORCE)
	if(EXISTS ${CACHEFILE})
		# When requested only read from the cache to populate our symbols.
		_importCache("${CACHEFILE}" ${LOGGING_QUIET})
		if(NOT LOGGING_QUIET)
			message( STATUS "Imported revision data from cache file (forced)" )
		endif()
	else()
		message( FATAL_ERROR "Option CACHEFORCE declared, but the specified CACHEFILE does not exist at: ${CACHEFILE}" )
	endif()
elseif(Git_FOUND AND _currentDirectoryIsInGitRepo)
	_gitRepo()
	if(NOT LOGGING_QUIET)
		message( STATUS "Gathered revision data from Git" )
	endif()
elseif(EXISTS ${CACHEFILE})
	# We are not in a repo; try to use a previously generated cache to populate our symbols.
	_importCache("${CACHEFILE}" ${LOGGING_QUIET})
	# Do not overwrite the cache if we know we are not going to write anything new.
	set(SKIPUPDATECACHE ON)
	if(NOT LOGGING_QUIET)
		message( STATUS "Imported revision data from cache file" )
	endif()
else()
	message( FATAL_ERROR "No repo, cache, or other data source detected." )
endif()

# VAROUT output is handled here
if(DEFINED VAROUT)
	message( STATUS "VCS_TYPE=${VCS_TYPE}" )
	message( STATUS "VCS_BASENAME=${VCS_BASENAME}" )
	message( STATUS "VCS_BRANCH=${VCS_BRANCH}" )
	message( STATUS "VCS_TAG=${VCS_TAG}" )
	message( STATUS "VCS_EXTRA=${VCS_EXTRA}" )
	message( STATUS "VCS_FULL_HASH=${VCS_FULL_HASH}" )
	message( STATUS "VCS_SHORT_HASH=${VCS_SHORT_HASH}" )
	message( STATUS "VCS_WC_MODIFIED=${VCS_WC_MODIFIED}" )
	message( STATUS "VCS_COMMIT_COUNT=${VCS_COMMIT_COUNT}" )
	message( STATUS "VCS_MOST_RECENT_TAGGED_VERSION=${VCS_MOST_RECENT_TAGGED_VERSION}" )
	message( STATUS "VCS_COMMIT_COUNT_SINCE_MOST_RECENT_TAGGED_VERSION=${VCS_COMMIT_COUNT_SINCE_MOST_RECENT_TAGGED_VERSION}" )
	message( STATUS "VCS_COMMIT_COUNT_ON_MASTER_UNTIL_BRANCH=${VCS_COMMIT_COUNT_ON_MASTER_UNTIL_BRANCH}" )
	message( STATUS "VCS_BRANCH_COMMIT_COUNT=${VCS_BRANCH_COMMIT_COUNT}" )
endif()

# Detect requested output type and use it.
if(DEFINED OUTPUT_TYPE)
	if("${OUTPUT_TYPE}" STREQUAL "h")
		_hOutput("${OUTPUT_FILE}" ${LOGGING_QUIET})
	elseif("${OUTPUT_TYPE}" STREQUAL "sh")
		_shOutput("${OUTPUT_FILE}" ${LOGGING_QUIET})
	else()
		message( FATAL_ERROR "Not a valid OUTPUT_TYPE (${OUTPUT_TYPE})." )
	endif()
endif()

# If requested, make a cache file.
if(DEFINED CACHEFILE AND NOT CACHEFORCE AND NOT SKIPUPDATECACHE)
	_shOutput("${CACHEFILE}" ${LOGGING_QUIET})
endif()
