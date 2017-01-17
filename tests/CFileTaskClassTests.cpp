#include "HOST.hpp"
/*
 * File:   CFileTaskTests.cpp
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

// CFileTask class definitions

#include "CFileTask.hpp" 

// Boost file system and format libraries definitions

#include <boost/filesystem.hpp> 
#include <boost/format.hpp>

namespace fs = boost::filesystem;

// =======================
// UNIT TEST FIXTURE CLASS
// =======================

// Test Action function data

struct TestActFnData {
    int fnCalledCount; // How many times action function called
};


class CFileTaskTests : public ::testing::Test {
protected:
    
    // Empty constructor

    CFileTaskTests() {
    }

    // Empty destructor
    
    virtual ~CFileTaskTests() {
    }
    
    // Keep initialization and cleanup code to SetUp() and TearDown() methods

    virtual void SetUp() {

        // Create function data (wrap in void shared pointer for passing to task).

        fnData.reset(new TestActFnData{0});
        funcData = static_cast<TestActFnData *> (fnData.get());

        this->taskOptions.reset(new CFileTask::TaskOptions{0, nullptr, nullptr});

        // Create watch folder.

        if (!fs::exists(CFileTaskTests::kWatchFolder)) {
            fs::create_directory(CFileTaskTests::kWatchFolder);
        }

        // Create destination folder.

        if (!fs::exists(CFileTaskTests::kDestinationFolder)) {
            fs::create_directory(CFileTaskTests::kDestinationFolder);
        }

    }

    virtual void TearDown() {

        // Remove watch folder.

        if (fs::exists(CFileTaskTests::kWatchFolder)) {
            fs::remove(CFileTaskTests::kWatchFolder);
        }

        // Remove destination folder.

        if (fs::exists(CFileTaskTests::kDestinationFolder)) {
            fs::remove(CFileTaskTests::kDestinationFolder);
        }

    }

    void createFile(std::string fileName);           // Create a test file.
    void createFiles(int fileCount);                 // Create fileCount files and check action function call count
    void generateException(std::exception_ptr e);

    std::shared_ptr<void> fnData;   // Action function data shared pointer wrapper
    TestActFnData *funcData;        // Action function data 

    std::string filePath = "";      // Test file path
    std::string fileName = "";      // Test file name
    int watchDepth = -1;            // Folder Watch depth
    std::string taskName = "";      // Task Name
    std::string watchFolder = "";   // Watch Folder

    CFileTask::TaskActionFcn taskActFcn; // Task Action Function Data
    std::shared_ptr<CFileTask::TaskOptions> taskOptions; // Task options

    static const std::string kWatchFolder; // Test Watch Folder
    static const std::string kDestinationFolder; // Test Destination folder

    static const std::string kParamAssertion1; // Missing parameter 1 Assert REGEX
    static const std::string kParamAssertion2; // Missing parameter 2 Assert REGEX
    static const std::string kParamAssertion3; // Missing parameter 3 Assert REGEX
    static const std::string kParamAssertion4; // Missing parameter 4 Assert REGEX
    static const std::string kParamAssertion5; // Missing parameter 5 Assert REGEX

};

// =================
// FIXTURE CONSTANTS
// =================

const std::string CFileTaskTests::kWatchFolder("/tmp/watch/");
const std::string CFileTaskTests::kDestinationFolder("/tmp/destination/");

const std::string CFileTaskTests::kParamAssertion1("Assertion*"); // NEED TO MODIFY FOR SPECIFIC ASSERTS
const std::string CFileTaskTests::kParamAssertion2("Assertion*");
const std::string CFileTaskTests::kParamAssertion3("Assertion*");
const std::string CFileTaskTests::kParamAssertion4("Assertion*");
const std::string CFileTaskTests::kParamAssertion5("Assertion*");

// ===============
// FIXTURE METHODS
// ===============

//
// Create a file for test purposes.
//

void CFileTaskTests::createFile(std::string fileName) {

    std::ofstream outfile(fileName);
    outfile << "TEST TEXT" << std::endl;
    outfile.close();

}

//
// Create fileCount files and check that action function called for each
//

void CFileTaskTests::createFiles(int fileCount) {

    this->taskName = "Test";
    this->watchFolder = kWatchFolder;
    this->watchDepth = -1;

    // Simple test action function that just increases call count

    this->taskActFcn = [] ( const std::string &filenamePath, const std::shared_ptr<void> fnData) -> bool {
        TestActFnData *funcData = static_cast<TestActFnData *> (fnData.get());
        funcData->fnCalledCount++;
        return true;
    };

    // Set any task options required by test

    (this->taskOptions)->killCount = fileCount;

    // Create task object
    
    CFileTask task{this->taskName, this->watchFolder, this->taskActFcn, this->fnData, this->watchDepth, this->taskOptions};

    // Create task object thread and start to watch

    std::unique_ptr<std::thread> taskThread;

    taskThread.reset(new std::thread(&CFileTask::monitor, &task));
     
    this->filePath = CFileTaskTests::kWatchFolder;

    for (auto cnt01 = 0; cnt01 < fileCount; cnt01++) {
        std::string file = (boost::format("temp%1%.txt") % cnt01).str();
        this->createFile(this->filePath + file);
    }

    // Thread should die after killCount files created

    taskThread->join();

    EXPECT_EQ(fileCount, funcData->fnCalledCount);

    for (auto cnt01 = 0; cnt01 < fileCount; cnt01++) {
        std::string file = (boost::format("temp%1%.txt") % cnt01).str();
        fs::remove(this->filePath + file);
    }

}

//
// Re-throw any exception passed.
//

void CFileTaskTests::generateException(std::exception_ptr e) {

    if (e) {
        std::rethrow_exception(e);
    }
    
}

// =====================
// TASK CLASS UNIT TESTS
// =====================

//
// Task Name lengh == 0 ASSERT
//

TEST_F(CFileTaskTests, CFileTaskAssertParam1) {

    EXPECT_DEATH(CFileTask task(this->taskName, this->watchFolder, this->taskActFcn, this->fnData, this->watchDepth), CFileTaskTests::kParamAssertion1);

}

//
// Watch Folder Name lengh == 0 ASSERT
//

TEST_F(CFileTaskTests, CFileTaskAssertParam2) {

    this->taskName = "Test";

    EXPECT_DEATH(CFileTask task(this->taskName, this->watchFolder, this->taskActFcn, this->fnData, this->watchDepth), CFileTaskTests::kParamAssertion2);

}

//
// Action Function Pointer == NULL ASSERT
//

TEST_F(CFileTaskTests, CFileTaskAssertParam3) {

    this->taskName = "Test";
    this->watchFolder = kWatchFolder;
    this->watchDepth = -1;

    EXPECT_DEATH(CFileTask task(this->taskName, this->watchFolder, nullptr, this->fnData, this->watchDepth), CFileTaskTests::kParamAssertion3);

}

//
// Action Function Data Pointer == NULL ASSERT
//

TEST_F(CFileTaskTests, CFileTaskAssertParam4) {

    this->taskName = "Test";
    this->watchFolder = kWatchFolder;
    this->watchDepth = -1;

    EXPECT_DEATH(CFileTask task(this->taskName, this->watchFolder, this->taskActFcn, nullptr, this->watchDepth), CFileTaskTests::kParamAssertion4);

}

//
// Watch Depth < -1 ASSERT
//

TEST_F(CFileTaskTests, CFileTaskAssertParam5) {

    this->taskName = "Test";
    this->watchFolder = kWatchFolder;
    this->watchDepth = -99;

    EXPECT_DEATH(CFileTask task(this->taskName, this->watchFolder, this->taskActFcn, this->fnData, this->watchDepth), CFileTaskTests::kParamAssertion5);

}

//
// Create 1 file in watcher folder
//

TEST_F(CFileTaskTests, CFileTaskCreateFile1) {

    this->createFiles(1);

}

//
// Create 10 files in watcher folder
//

TEST_F(CFileTaskTests, CFileTaskCreateFile10) {

    this->createFiles(10);

}

//
// Create 50 files in watcher folder
//

TEST_F(CFileTaskTests, CFileTaskCreateFile50) {

    this->createFiles(50);

}

//
// Create 100 files in watcher folder
//

TEST_F(CFileTaskTests, CFileTaskCreateFile100) {

    this->createFiles(100);

}

//
// Create 250 files in watcher folder
//

TEST_F(CFileTaskTests, CFileTaskCreateFile250) {

    this->createFiles(250);

}

//
// Create 500 files in watcher folder
//

TEST_F(CFileTaskTests, CFileTaskCreateFile500) {

    this->createFiles(500);

}

//
// Watch folder does not exist exception.
//

TEST_F(CFileTaskTests, CFileTaskNoWatchFolder) {

    this->taskName = "Test";
    this->watchFolder = "/tmp/tnothere";
    this->watchDepth = -1;

    // Simple test action function that does nothing

    this->taskActFcn = [] (const std::string& filenamePath, const std::shared_ptr<void> fnData) -> bool {
        TestActFnData *funcData = static_cast<TestActFnData *> (fnData.get());
        return true;
    };

    // Create task object

    EXPECT_THROW(CFileTask task(this->taskName, this->watchFolder, this->taskActFcn, this->fnData, this->watchDepth, this->taskOptions), std::system_error);

}

//
// Task action throw exception capture.
//

TEST_F(CFileTaskTests, CFileTaskTaskActionFunctionException) {

    this->taskName = "Test";
    this->watchFolder = kWatchFolder;
    this->fileName = "tmp.txt";
    this->watchDepth = -1;

    // Simple test action function that just throws an exception

    this->taskActFcn = [] (const std::string& filenamePath, const std::shared_ptr<void> fnData) -> bool {
        TestActFnData *funcData = static_cast<TestActFnData *> (fnData.get());
        throw std::logic_error("Just an example.");
        return true;
    };

    // Set any task options required by test

    (this->taskOptions)->killCount = 1;
    
    // Create task object

    CFileTask task{this->taskName, this->watchFolder, this->taskActFcn, this->fnData, this->watchDepth, this->taskOptions};

    // Create task object thread and start to watch

    std::unique_ptr<std::thread> taskThread;

    taskThread.reset(new std::thread(&CFileTask::monitor, &task));
    
    // Create one file to trigger action function

    this->createFile(this->watchFolder + this->fileName);

    // Thread should die after killCount files created

    taskThread->join();

    EXPECT_THROW(this->generateException(task.getThrownException()), std::logic_error);

    if (fs::exists(this->watchFolder + this->fileName)) {
        fs::remove(this->watchFolder + this->fileName);
    }

}

// =====================
// RUN GOOGLE UNIT TESTS
// =====================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}