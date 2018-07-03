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

#include "CTask.hpp" 

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
    
    virtual ~CFileTests() {
    }
    
    // Keep initialization and cleanup code to SetUp() and TearDown() methods

    virtual void SetUp() {

    }

    virtual void TearDown() {

    }

};

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
// Task action throw exception capture.
//

TEST_F(CFileTests, Test1) {

}

// =====================
// RUN GOOGLE UNIT TESTS
// =====================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}