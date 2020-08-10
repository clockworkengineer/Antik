#include "HOST.hpp"
/*
 * File:   UTCPath.cpp
 * 
 * Author: Robert Tizzard
 *
 * Created on October 24, 2016, 2:34 PM
 * 
 * Description: Google unit tests for class CPath.
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

// CPath class

#include "CPath.hpp" 

using namespace Antik::File;

// =======================
// UNIT TEST FIXTURE CLASS
// =======================

class UTCPath : public ::testing::Test {
protected:

    // Empty constructor

    UTCPath() {
    }

    // Empty destructor

    ~UTCPath() override{
    }

    // Keep initialisation and cleanup code to SetUp() and TearDown() methods

    void SetUp() override {

    }

    void TearDown() override {

    }

    static const std::string testPath1;
    static const std::string testPath2;
    static const std::string testFileName;
    static const std::string testFileBaseName;
    static const std::string testFileExtension;
    
};

const std::string UTCPath::testPath1("/home/user1/test/temp.txt");
const std::string UTCPath::testPath2("/home/user1/test");
const std::string UTCPath::testFileName("temp.txt");
const std::string UTCPath::testFileBaseName("temp");
const std::string UTCPath::testFileExtension(".txt");

// =================
// FIXTURE CONSTANTS
// =================

// ===============
// FIXTURE METHODS
// ===============

// =====================
// TASK CLASS UNIT TESTS
// =====================

//
// Path creation
//

TEST_F(UTCPath, PathCreation) {

    CPath path{ this->testPath1};

    ASSERT_STREQ(this->testPath1.c_str(), path.toString().c_str());

}

//
// Empty path creation
//

TEST_F(UTCPath, EmptyPathCreation) {

    CPath path{ ""};

    ASSERT_STREQ("", path.toString().c_str());
    ASSERT_STREQ(CPath::currentPath().c_str(), path.absolutePath().c_str());
    ASSERT_STREQ("", path.baseName().c_str());
    ASSERT_STREQ("", path.extension().c_str());
    ASSERT_STREQ("", path.fileName().c_str());
    ASSERT_STREQ("", path.parentPath().toString().c_str());
    ASSERT_STREQ("", path.extension().c_str());

}

//
// Parent path
//

TEST_F(UTCPath, ParentPath) {

    CPath path{ this->testPath1};

    ASSERT_STREQ(this->testPath2.c_str(), path.parentPath().toString().c_str());

}

//
// Filename
//

TEST_F(UTCPath, FileName) {

    CPath path{ this->testPath1};

    ASSERT_STREQ(this->testFileName.c_str(), path.fileName().c_str());

}

//
// BaseName
//

TEST_F(UTCPath, BaseName) {

    CPath path{ this->testPath1};

    ASSERT_STREQ(this->testFileBaseName.c_str(), path.baseName().c_str());

}

//
// Extension
//

TEST_F(UTCPath, Extension) {

    CPath path{ this->testPath1};

    ASSERT_STREQ(this->testFileExtension.c_str(), path.extension().c_str());

}

//
// Extension
//

TEST_F(UTCPath, ReplaceExtension) {

    CPath path{ this->testPath1};
    
    path.replaceExtension(".mp4");

    ASSERT_STREQ(".mp4", path.extension().c_str());

}

//
// Extension
//

TEST_F(UTCPath, Join) {

    CPath path{ this->testPath2};
    
    path.join("fileend.tmp");

    ASSERT_STREQ((this->testPath2+"/fileend.tmp").c_str(), path.toString().c_str());

}

//
// Extension
//

TEST_F(UTCPath, AbsolutePath) {

    CPath path{ "./test"};

    ASSERT_STREQ((CPath::currentPath()+"/test").c_str(), path.absolutePath().c_str());

}
