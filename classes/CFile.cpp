//
// Class: CFile
//
// Description: File interrogation and manipulation. At present this is just a simple
// adapter class for any boost file system functionality that is required.
//
// Dependencies:   C17++ - Language standard features used.
//                 Boost - Filesystem.
//

// =================
// CLASS DEFINITIONS
// =================

#include "CFile.hpp"

// ====================
// CLASS IMPLEMENTATION
// ====================

//
// C++ STL
//

// =========
// NAMESPACE
// =========

namespace Antik::File
{

// ===========================
// PRIVATE TYPES AND CONSTANTS
// ===========================

// ==========================
// PUBLIC TYPES AND CONSTANTS
// ==========================

// ========================
// PRIVATE STATIC VARIABLES
// ========================

// =======================
// PUBLIC STATIC VARIABLES
// =======================

// ===============
// PRIVATE METHODS
// ===============

// ==============
// PUBLIC METHODS
// ==============

//
// Return true of a path exists.
//

bool CFile::exists(const CPath &filePath)
{
    try
    {
        return (std::filesystem::exists(filePath.toString()));
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        throw Exception(e.what());
    }
}

//
// Return true if a path is a regular file.
//

bool CFile::isFile(const CPath &filePath)
{
    try
    {
        return std::filesystem::is_regular_file(filePath.toString());
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        throw Exception(e.what());
    }
}

//
// Get path type and permissions.
//

CFile::Status CFile::fileStatus(const CPath &filePath)
{
    try
    {
        return (std::filesystem::status(filePath.toString()));
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        throw Exception(e.what());
    }
}

//
// Return true if a path is a directory.
//

bool CFile::isDirectory(const CPath &filePath)
{
    try
    {
        return (std::filesystem::is_directory(filePath.toString()));
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        throw Exception(e.what());
    }
}

//
// Create a directory (recursively if necessary) returning true on success.
//

bool CFile::createDirectory(const CPath &directoryPath)
{
    try
    {
        return (std::filesystem::create_directories(directoryPath.toString()));
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        throw Exception(e.what());
    }
}

//
// Remove a file.
//

void CFile::remove(const CPath &filePath)
{
    try
    {
        std::filesystem::remove(filePath.toString());
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        throw Exception(e.what());
    }
}

//
// Set a files permissions.
//

void CFile::setPermissions(const CPath &filePath, Permissions permissions)
{
    try
    {
        std::filesystem::permissions(filePath.toString(), permissions);
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        throw Exception(e.what());
    }
}

//
// Copy a file.
//

void CFile::copy(const CPath &sourcePath, const CPath &destinationPath)
{
    try
    {
        std::filesystem::copy_file(sourcePath.toString(), destinationPath.toString(),
                                     std::filesystem::copy_options::none);
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        throw Exception(e.what());
    }
}

//
// Rename a file.
//

void CFile::rename(const CPath &sourcePath, const CPath &destinationPath)
{
    try
    {
        std::filesystem::rename(sourcePath.toString(), destinationPath.toString());
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        throw Exception(e.what());
    }
}

//
// Produce a list of files in a directory structure (traversing all directories).
//

FileList CFile::directoryContentsList(const CPath &localDirectory)
{

    FileList fileList;

    try
    {

        for (auto &directoryEntry : std::filesystem::
                 recursive_directory_iterator{localDirectory.toString()})
        {
            fileList.emplace_back(directoryEntry.path().string());
        }
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        throw Exception(e.what());
    }

    return (fileList);
}

//
// Return time that a file was last written to.
//

CFile::Time CFile::lastWriteTime(const CPath &filePath)
{

    try
    {
        return (std::filesystem::last_write_time(filePath.toString()));
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        throw Exception(e.what());
    }
}

} // namespace Antik::File
