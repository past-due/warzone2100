/*
	This file is part of Warzone 2100.
	Copyright (C) 2022  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

// A WzMap::IOProvider implementation that uses libzip (https://libzip.org/) to support loading from zip archives
// You must link to libzip (and any of its required dependencies)

#include "ZipIOProvider.h"

#include <memory>
#include <unordered_set>

#include <zip.h> // from libzip

#include "../src/map_internal.h"

class WrappedZipArchive
{
public:
	WrappedZipArchive(zip_t* pZip)
	: pZip(pZip)
	{ }
	~WrappedZipArchive()
	{
		if (readOnly)
		{
			zip_discard(pZip);
		}
		else
		{
			zip_close(pZip);
		}
	}
	zip_t* handle() const
	{
		return pZip;
	}
private:
	zip_t* pZip = nullptr;
	bool readOnly = false;
};

class WzMapBinaryZipIOStream : public WzMap::BinaryIOStream
{
private:
	WzMapBinaryZipIOStream(std::shared_ptr<WrappedZipArchive> zipArchive, WzMap::BinaryIOStream::OpenMode mode)
	: m_zipArchive(zipArchive)
	, m_mode(mode)
	{}
public:
	static std::unique_ptr<WzMapBinaryZipIOStream> openForReading(zip_uint64_t zipArchiveIndex, std::shared_ptr<WrappedZipArchive> zipArchive)
	{
		if (zipArchive == nullptr)
		{
			return nullptr;
		}
		auto result = std::unique_ptr<WzMapBinaryZipIOStream>(new WzMapBinaryZipIOStream(zipArchive, WzMap::BinaryIOStream::OpenMode::READ));
		result->m_pReadHandle = zip_fopen_index(zipArchive->handle(), zipArchiveIndex, 0);
		return result;
	}

	static std::unique_ptr<WzMapBinaryZipIOStream> openForWriting(const std::string& filename, std::shared_ptr<WrappedZipArchive> zipArchive)
	{
		if (filename.empty() || zipArchive == nullptr)
		{
			return nullptr;
		}
		auto result = std::unique_ptr<WzMapBinaryZipIOStream>(new WzMapBinaryZipIOStream(zipArchive, WzMap::BinaryIOStream::OpenMode::WRITE));
		result->m_filename = filename;
		return result;
	}

public:
	virtual ~WzMapBinaryZipIOStream()
	{
		close();
	};

	virtual optional<size_t> readBytes(void *buffer, size_t len) override
	{
		if (!m_pReadHandle) { return nullopt; }
		auto result = zip_fread(m_pReadHandle, buffer, len);
		if (result < 0)
		{
			// failed
			return nullopt;
		}
		return static_cast<size_t>(result);
	}

	virtual optional<size_t> writeBytes(const void *buffer, size_t len) override
	{
		// Write to writeBuffer
		const char* begin = (const char*)buffer;
		const char* end = begin + len;
		m_writeBuffer.insert(m_writeBuffer.end(), begin, end);
		return len;
	}

	virtual bool close() override
	{
		// If reading
		if (m_pReadHandle != nullptr)
		{
			zip_fclose(m_pReadHandle);
			m_pReadHandle = nullptr;
			m_filename.clear();
		}
		// If writing
		else if (!m_writeBuffer.empty() && !m_filename.empty())
		{
			zip_source_t *s = zip_source_buffer(m_zipArchive->handle(), m_writeBuffer.data(), m_writeBuffer.size(), 0);
			if (s == NULL)
			{
				// Failed to create a source buffer from the write buffer?
				m_writeBuffer.clear();
				m_filename.clear();
				return false;
			}
			zip_int64_t result = zip_file_add(m_zipArchive->handle(), m_filename.c_str(), s, ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8);
			zip_source_free(s);
			m_writeBuffer.clear();
			m_filename.clear();
			if (result < 0)
			{
				// Failed to write file
				return false;
			}
		}
		return true;
	}

	virtual bool endOfStream() override
	{
		if (m_mode != WzMap::BinaryIOStream::OpenMode::READ)
		{
			return false;
		}
		if (!m_pReadHandle) { return false; }
		// get current position in file
		auto position = zip_ftell(m_pReadHandle);
		if (position < 0)
		{
			return false;
		}
		// attempt to read a byte
		uint8_t tmpByte;
		if (!readULE8(&tmpByte))
		{
			// read failed - assume end of stream
			return true;
		}
		// restore original position
		zip_fseek(m_pReadHandle, 0, position);
		return false;
	}
private:
	std::shared_ptr<WrappedZipArchive> m_zipArchive;
	std::string m_filename;
	optional<WzMap::BinaryIOStream::OpenMode> m_mode;
	zip_file_t *m_pReadHandle = nullptr;
	std::vector<unsigned char> m_writeBuffer;
};

static zip_int64_t wz_zip_name_locate_impl(zip_t *archive, const char *fname, zip_flags_t flags, bool useWindowsPathWorkaroundIfNeeded)
{
	auto zipLocateResult = zip_name_locate(archive, fname, flags);
	if (zipLocateResult < 0)
	{
		// Failed to find a file with this name
		if (useWindowsPathWorkaroundIfNeeded && archive != NULL && fname != NULL)
		{
			// Replace all '/' in the input fname with '\\' and try again
			std::string fNameAdjusted = fname;
			std::replace(fNameAdjusted.begin(), fNameAdjusted.end(), '/', '\\');
			zipLocateResult = zip_name_locate(archive, fNameAdjusted.c_str(), flags);
		}
	}
	return zipLocateResult;
}

#define malformed_windows_path_separators_workaround \
	((m_foundMalformedWindowsPathSeparators.has_value()) ? m_foundMalformedWindowsPathSeparators.value() : determineIfMalformedWindowsPathSeparatorWorkaround())

#define wz_zip_name_locate(archive, fname, flags) \
	wz_zip_name_locate_impl(archive, fname, flags, malformed_windows_path_separators_workaround)

std::shared_ptr<WzMapZipIO> WzMapZipIO::openZipArchiveFS(const char* fileSystemPath, bool extraConsistencyChecks, bool readOnly)
{
	if (fileSystemPath == nullptr) { return nullptr; }
	struct zip_error error;
	zip_error_init(&error);
#if defined(_WIN32)
	// Special win32 handling (convert path from UTF-8 to UTF-16 and use the wide-char win32 source functions)
	std::vector<wchar_t> wFileSystemPathStr;
	if (!win_utf8ToUtf16(fileSystemPath, wFileSystemPathStr))
	{
		return nullptr;
	}
	zip_source_t* s = zip_source_win32w_create(wFileSystemPathStr.c_str(), 0, -1, &error);
#else
	zip_source_t* s = zip_source_file_create(fileSystemPath, 0, -1, &error);
#endif
	if (s == NULL)
	{
		// Failed to create source / open file
		return nullptr;
	}
	int flags = 0;
	if (extraConsistencyChecks)
	{
		flags |= ZIP_CHECKCONS;
	}
	if (readOnly)
	{
		flags |= ZIP_RDONLY;
	}
	zip_t* pZip = zip_open_from_source(s, flags, &error);
	if (pZip == NULL)
	{
		// Failed to open from source
		zip_source_free(s);
		return nullptr;
	}
	auto result = std::shared_ptr<WzMapZipIO>(new WzMapZipIO());
	result->m_zipArchive = std::make_shared<WrappedZipArchive>(pZip);
	return result;
}

WzMapZipIO::~WzMapZipIO()
{ }

std::unique_ptr<WzMap::BinaryIOStream> WzMapZipIO::openBinaryStream(const std::string& filename, WzMap::BinaryIOStream::OpenMode mode)
{
	std::unique_ptr<WzMap::BinaryIOStream> pStream;
	switch (mode)
	{
		case WzMap::BinaryIOStream::OpenMode::READ:
		{
			auto zipLocateResult = wz_zip_name_locate(m_zipArchive->handle(), filename.c_str(), ZIP_FL_ENC_GUESS);
			if (zipLocateResult < 0)
			{
				// Failed to find a file with this name
				return nullptr;
			}
			zip_uint64_t zipFileIndex = static_cast<zip_uint64_t>(zipLocateResult);
			pStream = WzMapBinaryZipIOStream::openForReading(zipFileIndex, m_zipArchive);
			break;
		}
		case WzMap::BinaryIOStream::OpenMode::WRITE:
			pStream = WzMapBinaryZipIOStream::openForWriting(filename, m_zipArchive);
			break;
	}
	return pStream;
}

bool WzMapZipIO::loadFullFile(const std::string& filename, std::vector<char>& fileData)
{
	auto zipLocateResult = wz_zip_name_locate(m_zipArchive->handle(), filename.c_str(), ZIP_FL_ENC_GUESS);
	if (zipLocateResult < 0)
	{
		// Failed to find a file with this name
		return false;
	}
	zip_uint64_t zipFileIndex = static_cast<zip_uint64_t>(zipLocateResult);
	// Get the expected length of the file
	struct zip_stat st;
	zip_stat_init(&st);
	if (zip_stat_index(m_zipArchive->handle(), zipFileIndex, 0, &st) != 0)
	{
		// zip_stat failed for file??
		return false;
	}
	if (!(st.valid & ZIP_STAT_SIZE))
	{
		// couldn't get the file size??
		return false;
	}
	if (std::numeric_limits<uint32_t>::max() < st.size)
	{
		// size is too big!
		return false;
	}
	auto readStream = WzMapBinaryZipIOStream::openForReading(zipFileIndex, m_zipArchive);
	if (!readStream)
	{
		return false;
	}
	// read the entire file
	fileData.clear();
	fileData.resize(static_cast<size_t>(st.size));
	auto result = readStream->readBytes(fileData.data(), fileData.size());
	if (!result.has_value())
	{
		// read failed
		return false;
	}
	if (result.value() != fileData.size())
	{
		// read was short
		fileData.clear();
		return false;
	}
	readStream->close();
	return true;
}

bool WzMapZipIO::writeFullFile(const std::string& filename, const char *ppFileData, uint32_t fileSize)
{
	zip_source_t *s = zip_source_buffer(m_zipArchive->handle(), ppFileData, fileSize, 0);
	if (s == NULL)
	{
		// Failed to create a source buffer from the write buffer?
		return false;
	}
	zip_int64_t result = zip_file_add(m_zipArchive->handle(), filename.c_str(), s, ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8);
	zip_source_free(s);
	if (result < 0)
	{
		// Failed to write file
		return false;
	}
	m_cachedDirectoriesList.clear(); // for now, just clear so it's re-generated if enumerateFolders is called
	return true;
}

const char* WzMapZipIO::pathSeparator() const
{
	return "/";
}

bool WzMapZipIO::enumerateFiles(const std::string& basePath, const std::function<bool (const char* file)>& enumFunc)
{
	if (!enumFunc)
	{
		return false;
	}
	zip_int64_t result = zip_get_num_entries(m_zipArchive->handle(), 0);
	if (result < 0)
	{
		return false;
	}
	for (zip_uint64_t idx = 0; idx < static_cast<zip_uint64_t>(result); idx++)
	{
		const char *name = zip_get_name(m_zipArchive->handle(), idx, ZIP_FL_ENC_GUESS);
		if (name == NULL)
		{
			continue;
		}
		if (!basePath.empty() && strncmp(basePath.c_str(), name, basePath.size()) != 0)
		{
			continue;
		}
		size_t nameLen = strlen(name);
		if (nameLen == 0)
		{
			continue;
		}

		if (malformed_windows_path_separators_workaround)
		{
			zip_uint8_t opsys = ZIP_OPSYS_UNIX;
			if (zip_file_get_external_attributes(m_zipArchive->handle(), idx, 0, &opsys, NULL) == 0)
			{
				if (opsys == ZIP_OPSYS_DOS && strchr(name, '\\') != nullptr)
				{
					std::string nameStr = name;
					std::replace(nameStr.begin(), nameStr.end(), '\\', '/');

					// filter out entries that end with "/" (these are dedicated directory entries)
					if (nameStr.back() == '/')
					{
						continue;
					}
					if (!enumFunc(nameStr.c_str()))
					{
						break;
					}
					continue;
				}
			}
		}

		// filter out entries that end with "/" (these are dedicated directory entries)
		if (*(name + (nameLen - 1)) == '/')
		{
			continue;
		}
		if (!enumFunc(name))
		{
			break;
		}
	}
	return true;
}

bool WzMapZipIO::enumerateFolders(const std::string& basePath, const std::function<bool (const char* file)>& enumFunc)
{
	if (!enumFunc)
	{
		return false;
	}
	
	if (m_cachedDirectoriesList.empty())
	{
		// Valid ZIP files may or may not contain dedicated directory entries (which end in "/")
		// So the only way to be sure is to enumerate everything and build a listing of all possible directories

		std::unordered_set<std::string> foundDirectoriesSet;
		zip_int64_t result = zip_get_num_entries(m_zipArchive->handle(), 0);
		if (result < 0)
		{
			return false;
		}
		for (zip_uint64_t idx = 0; idx < static_cast<zip_uint64_t>(result); idx++)
		{
			const char *name = zip_get_name(m_zipArchive->handle(), idx, ZIP_FL_ENC_GUESS);
			if (name == NULL)
			{
				continue;
			}
			std::string nameStr = name;
			if (nameStr.empty())
			{
				continue;
			}
			// support non-conforming Zip files that use Windows path separators (certain old compressor tools on Windows)
			if (malformed_windows_path_separators_workaround)
			{
				zip_uint8_t opsys = ZIP_OPSYS_UNIX;
				if (zip_file_get_external_attributes(m_zipArchive->handle(), idx, 0, &opsys, NULL) == 0)
				{
					if (opsys == ZIP_OPSYS_DOS && nameStr.find('\\') != std::string::npos)
					{
						std::replace(nameStr.begin(), nameStr.end(), '\\', '/');
					}
				}
			}

			// entries that end with "/" are dedicated directory entries

			size_t directoryDepth = 0;
			while (!nameStr.empty())
			{
				if (nameStr.back() != '/')
				{
					// otherwise, trim everything after the last "/" (i.e. trim the filename / basename)
					// to get the parent directory path
					auto lastSlashPos = nameStr.rfind('/');
					if (lastSlashPos == std::string::npos)
					{
						// no slash found
						break;
					}
					nameStr = nameStr.substr(0, lastSlashPos + 1);
				}

				auto setInsertResult = foundDirectoriesSet.insert(nameStr);
				if (setInsertResult.second)
				{
					m_cachedDirectoriesList.push_back(nameStr);
				}
				directoryDepth++;

				// remove any trailing "/"
				size_t numTrailingSlash = 0;
				for (auto it_r = nameStr.rbegin(); it_r != nameStr.rend(); it_r++)
				{
					if (*it_r != '/')
					{
						break;
					}
					numTrailingSlash++;
				}
				nameStr.resize(nameStr.size() - numTrailingSlash);
			}
		}

		// sort the directory list
		std::sort(m_cachedDirectoriesList.begin(), m_cachedDirectoriesList.end());
	}

	std::string basePathToSearch = basePath;
	bool emptyBasePath = basePathToSearch.empty();
	if (!emptyBasePath && basePathToSearch.back() != '/')
	{
		basePathToSearch += '/';
	}
	if (basePathToSearch == "/")
	{
		basePathToSearch = "";
		emptyBasePath = true;
	}
	for (const auto& dirPath : m_cachedDirectoriesList)
	{
		if (!emptyBasePath && strncmp(basePathToSearch.c_str(), dirPath.c_str(), basePathToSearch.size()) != 0)
		{
			continue;
		}
		// exclude exact match of basePathToSearch
		if (dirPath.length() == basePathToSearch.length())
		{
			continue;
		}
		// remove prefix from dirPath
		std::string relativeDirPath = dirPath.substr(basePathToSearch.length());
		if (!enumFunc(relativeDirPath.c_str()))
		{
			break;
		}
	}
	return true;
}

bool WzMapZipIO::determineIfMalformedWindowsPathSeparatorWorkaround()
{
	zip_int64_t result = zip_get_num_entries(m_zipArchive->handle(), 0);
	if (result < 0) { return false; }
	for (zip_uint64_t idx = 0; idx < static_cast<zip_uint64_t>(result); idx++)
	{
		const char *name = zip_get_name(m_zipArchive->handle(), idx, ZIP_FL_ENC_GUESS);
		if (name == NULL)
		{
			continue;
		}
		zip_uint8_t opsys;
		if (zip_file_get_external_attributes(m_zipArchive->handle(), idx, 0, &opsys, NULL) == 0)
		{
			if (opsys == ZIP_OPSYS_DOS)
			{
				if (strchr(name, '\\') != nullptr)
				{
					m_foundMalformedWindowsPathSeparators = true;
					return true;
				}
				else if (strchr(name, '/') != nullptr)
				{
					m_foundMalformedWindowsPathSeparators = false;
					return false;
				}
				continue;
			}
			else
			{
				m_foundMalformedWindowsPathSeparators = false;
				return false;
			}
		}
	}

	m_foundMalformedWindowsPathSeparators = false;
	return false;
}
