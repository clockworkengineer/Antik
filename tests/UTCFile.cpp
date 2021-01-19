/*
 * File:   UTCFile.cpp
 * 
 * Author: Robert Tizzard
 *
 * Created on October 24, 2016, 2:34 PM
 * 
 * Description: Google unit tests for class CFile.
 *
 * Copyright 2021.
 *
 */
// =============
// INCLUDE FILES
// =============
// Google test
#include "gtest/gtest.h"
// C++ STL
#include <stdexcept>
#include <fstream>
// CTask class
#include "CFile.hpp"
using namespace Antik::File;
// =======================
// UNIT TEST FIXTURE CLASS
// =======================
class UTCFile : public ::testing::Test
{
protected:
    // Empty constructor
    UTCFile()
    {
    }
    // Empty destructor
    ~UTCFile() override
    {
    }
    // Keep initialization and cleanup code to SetUp() and TearDown() methods
    void SetUp() override
    {
    }
    void TearDown() override
    {
    }
    void createFile(const CPath &filePath); // Create a test file.
    static const std::string kTestPathName1;
    static const std::string kTestPathName2;
    static const std::string kTestPathName3;
    static const std::string kTestPathName4;
};
// =================
// FIXTURE CONSTANTS
// =================
const std::string UTCFile::kTestPathName1{"/tmp/test1.txt"};
const std::string UTCFile::kTestPathName2{"/tmp/test2.txt"};
const std::string UTCFile::kTestPathName3{"/tmp/test"};
const std::string UTCFile::kTestPathName4{"/tmp/test/test1.txt"};
// ===============
// FIXTURE METHODS
// ===============
//
// Create a file for test purposes.
//
void UTCFile::createFile(const CPath &filePath)
{
    std::ofstream outfile(filePath.toString());
    outfile << "TEST TEXT" << std::endl;
    outfile.close();
}
// =====================
// TASK CLASS UNIT TESTS
// =====================
//
// Check to see file does not exist.
//
TEST_F(UTCFile, FileDoesNotExist)
{
    CPath filePath{kTestPathName1};
    EXPECT_FALSE(CFile::exists(filePath));
}
//
// Check to see file does exist.
//
TEST_F(UTCFile, FileExists)
{
    CPath filePath{kTestPathName1};
    createFile(filePath);
    EXPECT_TRUE(CFile::exists(filePath));
    CFile::remove(filePath);
}
//
// Check to see path is a file.
//
TEST_F(UTCFile, CheckIfPathIsNormalFile)
{
    CPath filePath{kTestPathName1};
    createFile(filePath);
    EXPECT_TRUE(CFile::isFile(filePath));
    CFile::remove(filePath);
}
//
// Check to see path is not a file.
//
TEST_F(UTCFile, CheckIfPathIsNotAFile)
{
    CPath filePath{kTestPathName3};
    CFile::createDirectory(filePath);
    EXPECT_FALSE(CFile::isFile(filePath));
    CFile::remove(filePath);
}
//
// Check to see path is not a directory.
//
TEST_F(UTCFile, CheckIfPathIsNotADirectory)
{
    CPath filePath{kTestPathName1};
    createFile(filePath);
    EXPECT_FALSE(CFile::isDirectory(filePath));
    CFile::remove(filePath);
}
//
// Check to see path is a directory
//
TEST_F(UTCFile, CheckIfPathIsADirectory)
{
    CPath filePath{kTestPathName3};
    CFile::createDirectory(filePath);
    EXPECT_TRUE(CFile::isDirectory(filePath));
    CFile::remove(filePath);
}
//
// Create an directory with invalid name (empty)
//
TEST_F(UTCFile, CreatDirectoryWithEmptyName)
{
    CPath filePath{""};
    EXPECT_THROW(CFile::createDirectory(filePath), CFile::Exception);
    CFile::remove(filePath);
}
//
// Create an directory and check that successful.
//
TEST_F(UTCFile, CreatDirectoryAndCheckForSuccess)
{
    CPath filePath{kTestPathName3};
    CFile::createDirectory(filePath);
    EXPECT_TRUE(CFile::isDirectory(filePath));
    CFile::remove(filePath);
}
//
// Remove a normal file.
//
TEST_F(UTCFile, RemoveANormalFile)
{
    CPath filePath{kTestPathName1};
    createFile(filePath);
    EXPECT_TRUE(CFile::exists(filePath) && CFile::isFile(filePath));
    CFile::remove(filePath);
    EXPECT_FALSE(CFile::exists(filePath));
}
//
// Remove a directory file.
//
TEST_F(UTCFile, RemoveADirectory)
{
    CPath filePath{kTestPathName3};
    CFile::createDirectory(filePath);
    EXPECT_TRUE(CFile::exists(filePath) && CFile::isDirectory(filePath));
    CFile::remove(filePath);
    EXPECT_FALSE(CFile::exists(filePath));
}
//
// Remove a non-empty directory file.
//
TEST_F(UTCFile, RemoveANonEmptyDirectory)
{
    CPath filePath{kTestPathName4};
    CFile::createDirectory(filePath.parentPath());
    createFile(filePath);
    EXPECT_TRUE(CFile::exists(filePath) && CFile::isFile(filePath));
    EXPECT_THROW(CFile::remove(filePath.parentPath()), CFile::Exception);
    CFile::remove(filePath);
    CFile::remove(filePath.parentPath());
}
//
// Copy a file.
//
TEST_F(UTCFile, CopyFile)
{
    CPath filePath{kTestPathName1};
    createFile(filePath);
    CFile::copy(filePath, kTestPathName2);
    EXPECT_TRUE(CFile::exists(kTestPathName2));
    CFile::remove(filePath);
    CFile::remove(kTestPathName2);
}
//
// Copy file.
//
TEST_F(UTCFile, CopyNonExistantFile)
{
    CPath filePath{kTestPathName1};
    EXPECT_THROW(CFile::copy(filePath, kTestPathName2), CFile::Exception);
}
//
// Copy file to an existing file.
//
TEST_F(UTCFile, CopyToExistingFile)
{
    CPath filePath{kTestPathName1};
    createFile(filePath);
    createFile(kTestPathName2);
    EXPECT_THROW(CFile::copy(filePath, kTestPathName2), CFile::Exception);
    CFile::remove(filePath);
    CFile::remove(kTestPathName2);
}
//
// Rename a file.
//
TEST_F(UTCFile, RenameFile)
{
    CPath filePath{kTestPathName1};
    createFile(filePath);
    CFile::rename(filePath, kTestPathName2);
    EXPECT_TRUE(CFile::exists(kTestPathName2));
    EXPECT_FALSE(CFile::exists(kTestPathName1));
    CFile::remove(kTestPathName2);
}
//
// Rename non-existant file.
//
TEST_F(UTCFile, RenameNonExistantFile)
{
    CPath filePath{kTestPathName1};
    EXPECT_THROW(CFile::rename(filePath, kTestPathName2), CFile::Exception);
}
//
// Rename file to an existing file.
//
TEST_F(UTCFile, RenameToExistingFile)
{
    CPath filePath{kTestPathName1};
    createFile(filePath);
    createFile(kTestPathName2);
    CFile::rename(filePath, kTestPathName2);
    EXPECT_FALSE(CFile::exists(kTestPathName1));
    EXPECT_TRUE(CFile::exists(kTestPathName2));
    CFile::remove(kTestPathName2);
}
//
// Remove file.
//
TEST_F(UTCFile, RemoveFile)
{
    CPath filePath{kTestPathName1};
    createFile(filePath);
    CFile::remove(filePath);
    EXPECT_FALSE(CFile::exists(kTestPathName1));
}