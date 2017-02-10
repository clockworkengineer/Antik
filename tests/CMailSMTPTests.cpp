#include "HOST.hpp"
/*
 * File:   CFileSMTPTests.cpp
 * 
 * Author: Robert Tizzard
 *
 * Created on October 24, 2016, 2:34 PM
 * 
 * Description: Google unit tests for class CFileTask.
 *
 * Copyright 2016.
 *
 */

// =============
// INCLUDE FILES
// =============

// Google test definitions

#include "gtest/gtest.h"

// C++ STL definitions

#include <stdexcept>

// CMailSMTP class definitions

#include "CMailSMTP.hpp" 

// =======================
// UNIT TEST FIXTURE CLASS
// =======================


class CFileSMTPTests : public ::testing::Test {
protected:
    
    // Empty constructor

    CFileSMTPTests() {
    }

    // Empty destructor
    
    virtual ~CFileSMTPTests() {
    }
    
    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
    
    CMailSMTP smtp;

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

TEST_F(CFileSMTPTests, CMailSMTPSetServerURL) {
    
    std::string serverURL;
    smtp.setServer("smtp://smtp.gmail.com:25");
    ASSERT_STREQ("smtp://smtp.gmail.com:25", smtp.getServer().c_str());
    
}

TEST_F(CFileSMTPTests, CMailSMTPSetUser) {
    
    smtp.setUserAndPassword("user01", "password01");
    ASSERT_STREQ("user01", smtp.getUser().c_str());
    
}

TEST_F(CFileSMTPTests, CMailSMTPSetFromAddress) {
    
    smtp.setFromAddress("<user01@gmail.com>");
    ASSERT_STREQ("<user01@gmail.com>", smtp.getFromAddress().c_str());
    
}

TEST_F(CFileSMTPTests, CMailSMTPSetToAddress) {
    
    smtp.setToAddress("<user02@gmail.com>");
    ASSERT_STREQ("<user02@gmail.com>", smtp.getToAddress().c_str());
    
}

TEST_F(CFileSMTPTests, CMailSMTPSetCCAddress) {
    
    smtp.setCCAddress("<user03@gmail.com>,<user04@gmail.com>,<user05@gmail.com>,<user06@gmail.com>");
    ASSERT_STREQ("<user03@gmail.com>,<user04@gmail.com>,<user05@gmail.com>,<user06@gmail.com>", smtp.getCCAddress().c_str());
    
}

TEST_F(CFileSMTPTests, CMailSMTPSetMailSubject) {
    
    smtp.setMailSubject("Message From The Grave");
    ASSERT_STREQ("Message From The Grave", smtp.getMailSubject().c_str());
    
}

TEST_F(CFileSMTPTests, CMailSMTPSetMailMessage) {
    
    smtp.setMailMessage({"Man is distinguished, not only by his reason, but by this singular passion from ",
        "other animals, which is a lust of the mind, that by a perseverance of delight ",
        "in the continued and indefatigable generation of knowledge, exceeds the short ",
        "vehemence of any carnal pleasure."});
    
    ASSERT_STREQ("Man is distinguished, not only by his reason, but by this singular passion from other animals,"
                 " which is a lust of the mind, that by a perseverance of delight in the continued and indefatigable"
                 " generation of knowledge, exceeds the short vehemence of any carnal pleasure.", smtp.getMailMessage().c_str());
    
}

// =====================
// RUN GOOGLE UNIT TESTS
// =====================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}