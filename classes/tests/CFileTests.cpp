#include "HOST.hpp"
/*
 * File:   CFileTests.cpp
 * 
 * Author: Robert Tizzard
 *
 * Created on October 24, 2016, 2:34 PM
 * 
 * Description: Google unit tests for class CFile.
 *
 * Copyright 2016.
 *
 */

// =============
// INCLUDE FILES
// =============

// Google test

#include "gtest/gtest.h"

// C++ STL

#include <stdexcept>

// CTask class

#include "CFile.hpp" 

using namespace Antik::File;

// =======================
// UNIT TEST FIXTURE CLASS
// =======================

class CFileTests : public ::testing::Test {
protected:
    
    // Empty constructor

    CFileTests() {
    }

    // Empty destructor
    
    ~CFileTests() override{
    }
    
    // Keep initialization and cleanup code to SetUp() and TearDown() methods

    void SetUp() override {

    }

    void TearDown() override {

    }
    
    void createFile(const CPath &filePath);   // Create a test file.

    static const std::string kTestPathName1;
    static const std::string kTestPathName2;
    static const std::string kTestPathName3;
    static const std::string kTestPathName4;

        
};

// =================
// FIXTURE CONSTANTS
// =================

const std::string CFileTests::kTestPathName1 { "/tmp/test1.txt"};
const std::string CFileTests::kTestPathName2 { "/tmp/test2.txt"};
const std::string CFileTests::kTestPathName3 { "/tmp/test"};
const std::string CFileTests::kTestPathName4 { "/tmp/test/test1.txt"};

   
// ===============
// FIXTURE METHODS
// ===============

//
// Create a file for test purposes.
//

void CFileTests::createFile(const CPath &filePath) {

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

TEST_F(CFileTests, FileDoesNotExist) {

    CPath filePath { kTestPathName1 };
    
    EXPECT_FALSE(CFile::exists(filePath));
    
}

//
// Check to see file does exist.
//

TEST_F(CFileTests, FileExists) {

    CPath filePath { kTestPathName1 };
    
    createFile(filePath);
    
    EXPECT_TRUE(CFile::exists(filePath));
    
    CFile::remove(filePath);
    
}

//
// Check to see path is a file.
//

TEST_F(CFileTests, CheckIfPathIsNormalFile) {

    CPath filePath { kTestPathName1 };
    
    createFile(filePath);
    
    EXPECT_TRUE(CFile::isFile(filePath));
    
    CFile::remove(filePath);
    
}

//
// Check to see path is not a file.
//

TEST_F(CFileTests, CheckIfPathIsNotAFile) {

    CPath filePath { kTestPathName3 };
    
    CFile::createDirectory(filePath);
    
    EXPECT_FALSE(CFile::isFile(filePath));
    
    CFile::remove(filePath);
    
}

//
// Check to see path is not a directory.
//

TEST_F(CFileTests, CheckIfPathIsNotADirectory) {

    CPath filePath { kTestPathName1 };
    
    createFile(filePath);
    
    EXPECT_FALSE(CFile::isDirectory(filePath));
    
    CFile::remove(filePath);
    
}

//
// Check to see path is a directory
//

TEST_F(CFileTests, CheckIfPathIsADirectory) {

    CPath filePath { kTestPathName3 };
    
    CFile::createDirectory(filePath);
    
    EXPECT_TRUE(CFile::isDirectory(filePath));
    
    CFile::remove(filePath);
    
}

//
// Create an directory with invalid name (empty)
//

TEST_F(CFileTests, CreatDirectoryWithEmptyName) {

    CPath filePath {""};
    
    EXPECT_THROW(CFile::createDirectory(filePath), CFile::Exception);
    
    CFile::remove(filePath);
    
}

//
// Create an directory and check that successful.
//

TEST_F(CFileTests, CreatDirectoryAndCheckForSuccess) {

    CPath filePath {kTestPathName3};
    
    CFile::createDirectory(filePath);
    
    EXPECT_TRUE(CFile::isDirectory(filePath));
    
    CFile::remove(filePath);
    
}

//
// Remove a normal file.
//

TEST_F(CFileTests, RemoveANormalFile) {

    CPath filePath {kTestPathName1};
    
    createFile(filePath);
    
    EXPECT_TRUE(CFile::exists(filePath)&&CFile::isFile(filePath));
    
    CFile::remove(filePath);
    
    EXPECT_FALSE(CFile::exists(filePath));
    
}

//
// Remove a directory file.
//

TEST_F(CFileTests, RemoveADirectory) {

    CPath filePath {kTestPathName3};
    
    CFile::createDirectory(filePath);
    
    EXPECT_TRUE(CFile::exists(filePath)&&CFile::isDirectory(filePath));
    
    CFile::remove(filePath);
    
    EXPECT_FALSE(CFile::exists(filePath));
    
}

//
// Remove a non-empty directory file.
//

TEST_F(CFileTests, RemoveANonEmptyDirectory) {

    CPath filePath {kTestPathName4};
    
    CFile::createDirectory(filePath.parentPath());
    createFile(filePath);
    
    EXPECT_TRUE(CFile::exists(filePath)&&CFile::isFile(filePath));
    
    EXPECT_THROW(CFile::remove(filePath.parentPath()), CFile::Exception);
    
    CFile::remove(filePath);
    CFile::remove(filePath.parentPath());
    
}

//
// Copy a file.
//

TEST_F(CFileTests, CopyFile) {

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

TEST_F(CFileTests, CopyNonExistantFile) {

    CPath filePath{kTestPathName1};

    EXPECT_THROW(CFile::copy(filePath, kTestPathName2), CFile::Exception);
    
}

//
// Copy file to an existing file.
//

TEST_F(CFileTests, CopyToExistingFile) {

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

TEST_F(CFileTests, RenameFile) {

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

TEST_F(CFileTests, RenameNonExistantFile) {

    CPath filePath{kTestPathName1};

    EXPECT_THROW(CFile::rename(filePath, kTestPathName2), CFile::Exception);
    
}

//
// Rename file to an existing file.
//

TEST_F(CFileTests, RenameToExistingFile) {

    CPath filePath{kTestPathName1};

    createFile(filePath);
    createFile(kTestPathName2);
    
    CFile::rename(filePath, kTestPathName2);
 
    EXPECT_FALSE(CFile::exists(kTestPathName1));
    EXPECT_TRUE(CFile::exists(kTestPathName2));
    
    CFile::remove(kTestPathName2);
    
}

//
// Remove file file.
//

TEST_F(CFileTests, RemoveFile) {

    CPath filePath{kTestPathName1};

    createFile(filePath);
    
    CFile::remove(filePath);
 
    EXPECT_FALSE(CFile::exists(kTestPathName1));
    
}

// =====================
// RUN GOOGLE UNIT TESTS
// =====================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}