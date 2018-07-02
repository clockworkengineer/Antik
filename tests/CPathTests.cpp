#include "HOST.hpp"
/*
 * File:   CPathTests.cpp
 * 
 * Author: Robert Tizzard
 *
 * Created on October 24, 2016, 2:34 PM
 * 
 * Description: Google unit tests for class CPathTests.
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

// Boost file system and format libraries

#include <boost/filesystem.hpp> 
#include <boost/format.hpp>

namespace fs = boost::filesystem;

// =======================
// UNIT TEST FIXTURE CLASS
// =======================

class CPathTests : public ::testing::Test {
protected:
    
    // Empty constructor

    CPathTests() {
    }

    // Empty destructor
    
    virtual ~CPathTests() {
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

TEST_F(CPathTests, ActionFunctionException) {

}

// =====================
// RUN GOOGLE UNIT TESTS
// =====================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}