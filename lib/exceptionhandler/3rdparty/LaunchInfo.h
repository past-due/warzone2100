//
// LaunchInfo
// Version: 1.0
//
// Copyright (c) 2020 past-due
//
// https://github.com/past-due/launchinfo
//
// Distributed under the MIT License.
// See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT
//

#pragma once

#ifndef __INCLUDED_LAUNCH_INFO_H__
#define __INCLUDED_LAUNCH_INFO_H__

#include <string>

#if defined(_WIN32)
# include <cstdint> // for uint32_t
#elif defined(__APPLE__)
# include <unistd.h> // for pid_t
#endif

class LaunchInfo
{
public:
#if defined(_WIN32)
	typedef uint32_t pid_type;
#elif defined(__APPLE__)
	typedef pid_t pid_type;
#else
	typedef int pid_type;
#endif

	struct ProcessDetails
	{
		pid_type pid = 0;
		std::string imageFileName;
	};

public:

	// should be called once, at process startup
	static void initializeProcess(int argc, const char * const *argv);

	static pid_type getParentPID();
	static const std::string& getParentImageName();

private:
	static LaunchInfo& getInstance();
	void _initializeProcess(int argc, const char * const *argv);
private:
	ProcessDetails parentProcess;
};

#endif // __INCLUDED_LAUNCH_INFO_H__
