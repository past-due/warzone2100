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

#include "LaunchInfo.h"


// should be called once, at process startup
void LaunchInfo::initializeProcess(int argc, const char * const *argv)
{
	return getInstance()._initializeProcess(argc, argv);
}

LaunchInfo::pid_type LaunchInfo::getParentPID()
{
	return getInstance().parentProcess.pid;
}

const std::string& LaunchInfo::getParentImageName()
{
	return getInstance().parentProcess.imageFileName;
}

LaunchInfo& LaunchInfo::getInstance()
{
	static LaunchInfo info;
	return info;
}

#if defined(_WIN32)
# define WIN32_LEAN_AND_MEAN
# undef NOMINMAX
# define NOMINMAX 1
# include <windows.h>
# include <tlhelp32.h>
# include <vector>
struct ProcessEnumInfo
{
	DWORD pid = 0;
	std::string szExeFile;
};
ProcessEnumInfo GetParentProcessInfo(DWORD pid)
{
	ProcessEnumInfo parentProcessInfo;

	// Take a snapshot of all processes in the system.
	HANDLE hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	if( hProcessSnap == INVALID_HANDLE_VALUE )
	{
		// CreateToolhelp32Snapshot failed
		return parentProcessInfo;
	}

	// Set the size of the structure before using it.
	PROCESSENTRY32W pe32;
	pe32.dwSize = sizeof( PROCESSENTRY32W );

	// Retrieve information about the first process,
	// and exit if unsuccessful
	if( !Process32FirstW( hProcessSnap, &pe32 ) )
	{
		// Process32FirstW failed
		CloseHandle( hProcessSnap );          // clean the snapshot object
		return parentProcessInfo;
	}

	do
	{
		if ( pe32.th32ProcessID == pid )
		{
			parentProcessInfo.pid = pe32.th32ParentProcessID;
			break;
		}
	} while( Process32NextW( hProcessSnap, &pe32 ) );

	if (parentProcessInfo.pid != 0)
	{
		// Loop again to find the parent process entry
		if( !Process32FirstW( hProcessSnap, &pe32 ) )
		{
			// Unexpectedly failed calling Process32FirstW again
			CloseHandle( hProcessSnap );          // clean the snapshot object
			return parentProcessInfo;
		}
		do
		{
			if ( pe32.th32ProcessID == parentProcessInfo.pid )
			{
				// found parent process entry
				// convert UTF-16 szExeFile to UTF-8
				std::vector<char> utf8Buffer;
				int utf8Len = WideCharToMultiByte(CP_UTF8, 0, pe32.szExeFile, -1, NULL, 0, NULL, NULL);
				if ( utf8Len <= 0 )
				{
					// Encoding conversion error
					break;
				}
				utf8Buffer.resize(utf8Len, 0);
				if ( WideCharToMultiByte(CP_UTF8, 0, pe32.szExeFile, -1, &utf8Buffer[0], utf8Len, NULL, NULL) == 0 )
				{
					// Encoding conversion error
					break;
				}
				parentProcessInfo.szExeFile = std::string(utf8Buffer.begin(), utf8Buffer.end());
				break;
			}
		} while( Process32NextW( hProcessSnap, &pe32 ) );
	}

	CloseHandle( hProcessSnap );
	return parentProcessInfo;
}

typedef BOOL (WINAPI *QueryFullProcessImageNameWFunc)(
  HANDLE hProcess,
  DWORD  dwFlags,
  LPWSTR lpExeName,
  PDWORD lpdwSize
);

#if !defined(PROCESS_NAME_NATIVE)
# define PROCESS_NAME_NATIVE	0x00000001
#endif

LaunchInfo::ProcessDetails GetParentProcessDetails(DWORD pid)
{
	LaunchInfo::ProcessDetails parentProcess;

	ProcessEnumInfo parentProcessInfo = GetParentProcessInfo(pid);
	if ( parentProcessInfo.pid == 0 )
	{
		// Failed to get parent pid
		return parentProcess;
	}
	parentProcess.pid = parentProcessInfo.pid;
	parentProcess.imageFileName = parentProcessInfo.szExeFile; // default to parentProcessInfo.szExeFile

	HANDLE hParent = OpenProcess( PROCESS_QUERY_LIMITED_INFORMATION, FALSE, parentProcessInfo.pid );
	if ( hParent == NULL )
	{
		// OpenProcess failed
		// fall-back to the parentProcessInfo.szExeFile
		return parentProcess;
	}

	FILETIME ftCreation, ftExit, ftKernel, ftUser;
	if ( GetProcessTimes(hParent, &ftCreation, &ftExit, &ftKernel, &ftUser) != 0 )
	{
		// compare with the input pid's creation time, to try to catch case where the
		// parent exits and its pid is reused before this code is executed
		HANDLE hProcess = OpenProcess( PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid );
		if ( hProcess != NULL )
		{
			FILETIME ftChildCreation, ftChildExit, ftChildKernel, ftChildUser;
			if ( GetProcessTimes(hParent, &ftChildCreation, &ftChildExit, &ftChildKernel, &ftChildUser) != 0 )
			{
				if ( CompareFileTime(&ftCreation, &ftChildCreation) >= 1 )
				{
					// "parent" pid's process was created after the child - detected pid reuse
					CloseHandle(hProcess);
					CloseHandle(hParent);
					// clear any parent process data
					parentProcess.pid = 0;
					parentProcess.imageFileName.clear();
					return parentProcess;
				}
			}
			else
			{
				// calling GetProcessTimes on the input pid unexpectedly failed
				// ignore this, for now
			}

			CloseHandle(hProcess);
		}
		else
		{
			// OpenProcess unexpectedly failed for the input pid
			// ignore this, for now
		}
	}

	// Get the QueryFullProcessImageNameW function
	QueryFullProcessImageNameWFunc _QueryFullProcessImageNameW = reinterpret_cast<QueryFullProcessImageNameWFunc>(reinterpret_cast<void*>(GetProcAddress(GetModuleHandleW(L"kernel32"), "QueryFullProcessImageNameW")));
	if (!_QueryFullProcessImageNameW)
	{
		// QueryFullProcessImageNameW not found
		CloseHandle(hParent);
		// fall-back to the parentProcessInfo.szExeFile
		return parentProcess;
	}

	// Get the parent's image file name
	const DWORD MAX_EXTENDED_WIN32_PATH = 32767;
	DWORD bufferSize = MAX_PATH + 1;
	std::vector<wchar_t> buffer;
	buffer.resize(bufferSize, static_cast<wchar_t>(0));
	BOOL bQueryResult = FALSE;
	DWORD dwError = 0;
	while ( ((bQueryResult = _QueryFullProcessImageNameW(hParent, 0, &buffer[0], &bufferSize)) == 0) && ((dwError = GetLastError()) == ERROR_INSUFFICIENT_BUFFER) && (bufferSize < MAX_EXTENDED_WIN32_PATH))
	{
		bufferSize *= 2;
		buffer.resize(bufferSize, static_cast<wchar_t>(0));
	}

	if ( bQueryResult == 0 )
	{
		// QueryFullProcessImageNameW failed in an unrecoverable way (see: dwError)
		CloseHandle(hParent);
		// fall-back to the parentProcessInfo.szExeFile
		return parentProcess;
	}

	// attempt to convert the image file name to a long path
	std::vector<wchar_t> buffer_longPath;
	buffer_longPath.resize(bufferSize, static_cast<wchar_t>(0));
	DWORD dwLongPathLen = 0;
	while (((dwLongPathLen = GetLongPathNameW(buffer.data(), &buffer_longPath[0], bufferSize)) > bufferSize) && (dwLongPathLen <= MAX_EXTENDED_WIN32_PATH + 1))
	{
		// increase buffer size
		bufferSize = dwLongPathLen;
		buffer_longPath.resize(bufferSize, static_cast<wchar_t>(0));
	}
	if ( dwLongPathLen > 0 )
	{
		// succeeded at retrieving a long path - swap buffers
		buffer.swap(buffer_longPath);
	}
	else
	{
		// GetLongPathNameW failed - just use the original path
		// no-op
	}
	buffer_longPath.clear();

	// convert the UTF-16 buffer to UTF-8
	std::vector<char> utf8Buffer;
	int utf8Len = WideCharToMultiByte(CP_UTF8, 0, buffer.data(), -1, NULL, 0, NULL, NULL);
	if ( utf8Len <= 0 )
	{
		// Encoding conversion error
		CloseHandle(hParent);
		// fall-back to the parentProcessInfo.szExeFile
		return parentProcess;
	}
	utf8Buffer.resize(utf8Len, 0);
	if ( WideCharToMultiByte(CP_UTF8, 0, buffer.data(), -1, &utf8Buffer[0], utf8Len, NULL, NULL) == 0 )
	{
		// Encoding conversion error
		CloseHandle(hParent);
		// fall-back to the parentProcessInfo.szExeFile
		return parentProcess;
	}

	CloseHandle(hParent);
	parentProcess.imageFileName = std::string(utf8Buffer.begin(), utf8Buffer.end());
	return parentProcess;
}

LaunchInfo::ProcessDetails GetCurrentProcessParentDetails()
{
	return GetParentProcessDetails(GetCurrentProcessId());
}

#elif defined(__APPLE__)

# include <libproc.h>
# include <unistd.h>
# include <errno.h>

std::string GetPathFromProcessID(pid_t pid)
{
	// NOTE: This uses proc_pidpath, which is technically private API
	char pathbuf[PROC_PIDPATHINFO_MAXSIZE];
	int ret = proc_pidpath(pid, pathbuf, sizeof(pathbuf));
	if ( ret <= 0 )
	{
		// proc_pidpath failed
		// error details can be retrieved via `strerror(errno)`
	}
	else
	{
		return std::string(pathbuf);
	}
	return "";
}

LaunchInfo::ProcessDetails GetCurrentProcessParentDetails()
{
	LaunchInfo::ProcessDetails parentProcess;
	parentProcess.pid = getppid();
	parentProcess.imageFileName = GetPathFromProcessID(parentProcess.pid);
	return parentProcess;
}

#else

// Not yet implemented

LaunchInfo::ProcessDetails GetCurrentProcessParentDetails()
{
	LaunchInfo::ProcessDetails parentProcess;
	// not yet implemented
	return parentProcess;
}

#endif


void LaunchInfo::_initializeProcess(int argc, const char * const *argv)
{
	parentProcess = GetCurrentProcessParentDetails();
}
