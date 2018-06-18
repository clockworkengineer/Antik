#include "HOST.hpp"
/*
 * File:   CTaskTests.cpp
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

// Test Action function data

struct TestActFnData {
    int fnCalledCount; // How many times action function called
};


class CTaskTests : public ::testing::Test {
protected:
    
    // Empty constructor

    CTaskTests() {
    }

    // Empty destructor
    
    virtual ~CTaskTests() {
    }
    
    // Keep initialization and cleanup code to SetUp() and TearDown() methods

    virtual void SetUp() {

        // Create function data (wrap in void shared pointer for passing to task).

        fnData.reset(new TestActFnData{0});
        funcData = static_cast<TestActFnData *> (fnData.get());

        taskOptions.reset(new CTask::TaskOptions{0});

        // Create watch folder.

        if (!fs::exists(CTaskTests::kWatchFolder)) {
            fs::create_directory(CTaskTests::kWatchFolder);
        }

        // Create destination folder.

        if (!fs::exists(CTaskTests::kDestinationFolder)) {
            fs::create_directory(CTaskTests::kDestinationFolder);
        }

    }

    virtual void TearDown() {

        // Remove watch folder.

        if (fs::exists(CTaskTests::kWatchFolder)) {
            fs::remove(CTaskTests::kWatchFolder);
        }

        // Remove destination folder.

        if (fs::exists(CTaskTests::kDestinationFolder)) {
            fs::remove(CTaskTests::kDestinationFolder);
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

    CTask::TaskActionFcn taskActFcn; // Task Action Function Data
    std::shared_ptr<CTask::TaskOptions> taskOptions; // Task options

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

const std::string CTaskTests::kWatchFolder("/tmp/watch/");
const std::string CTaskTests::kDestinationFolder("/tmp/destination/");

const std::string CTaskTests::kParamAssertion1("Assertion*"); // NEED TO MODIFY FOR SPECIFIC ASSERTS
const std::string CTaskTests::kParamAssertion2("Assertion*");
const std::string CTaskTests::kParamAssertion3("Assertion*");
const std::string CTaskTests::kParamAssertion4("Assertion*");
const std::string CTaskTests::kParamAssertion5("Assertion*");

// ===============
// FIXTURE METHODS
// ===============

//
// Create a file for test purposes.
//

void CTaskTests::createFile(std::string fileName) {

    std::ofstream outfile(fileName);
    outfile << "TEST TEXT" << std::endl;
    outfile.close();

}

//
// Create fileCount files and check that action function called for each
//

void CTaskTests::createFiles(int fileCount) {

    taskName = "Test";
    watchFolder = kWatchFolder;
    watchDepth = -1;

    // Simple test action function that just increases call count

    taskActFcn = [] ( const std::string &filenamePath, const std::shared_ptr<void> fnData) -> bool {
        TestActFnData *funcData = static_cast<TestActFnData *> (fnData.get());
        funcData->fnCalledCount++;
        return true;
    };

    // Set any task options required by test

    (taskOptions)->killCount = fileCount;

    // Create task object
    
    CTask task{taskName, watchFolder, taskActFcn, fnData, watchDepth, taskOptions};

    // Create task object thread and start to watch

    std::unique_ptr<std::thread> taskThread;

    taskThread.reset(new std::thread(&CTask::monitor, &task));
     
    filePath = CTaskTests::kWatchFolder;

    for (auto cnt01 = 0; cnt01 < fileCount; cnt01++) {
        std::string file = (boost::format("temp%1%.txt") % cnt01).str();
        createFile(filePath + file);
    }

    // Thread should die after killCount files created

    taskThread->join();

    EXPECT_EQ(fileCount, funcData->fnCalledCount);

    for (auto cnt01 = 0; cnt01 < fileCount; cnt01++) {
        std::string file = (boost::format("temp%1%.txt") % cnt01).str();
        fs::remove(filePath + file);
    }

}

//
// Re-throw any exception passed.
//

void CTaskTests::generateException(std::exception_ptr e) {

    if (e) {
        std::rethrow_exception(e);
    }
    
}

// =====================
// TASK CLASS UNIT TESTS
// =====================

//
// Task Name length == 0 ASSERT
//

TEST_F(CTaskTests, AssertParam1) {

    EXPECT_DEATH(CTask task(taskName, watchFolder, taskActFcn, fnData, watchDepth), CTaskTests::kParamAssertion1);

}

//
// Watch Folder Name lengh == 0 ASSERT
//

TEST_F(CTaskTests, AssertParam2) {

    taskName = "Test";

    EXPECT_DEATH(CTask task(taskName, watchFolder, taskActFcn, fnData, watchDepth), CTaskTests::kParamAssertion2);

}

//
// Action Function Pointer == NULL ASSERT
//

TEST_F(CTaskTests, AssertParam3) {

    taskName = "Test";
    watchFolder = kWatchFolder;
    watchDepth = -1;

    EXPECT_DEATH(CTask task(taskName, watchFolder, nullptr, fnData, watchDepth), CTaskTests::kParamAssertion3);

}

//
// Action Function Data Pointer == NULL ASSERT
//

TEST_F(CTaskTests, AssertParam4) {

    taskName = "Test";
    watchFolder = kWatchFolder;
    watchDepth = -1;

    EXPECT_DEATH(CTask task(taskName, watchFolder, taskActFcn, nullptr, watchDepth), CTaskTests::kParamAssertion4);

}

//
// Watch Depth < -1 ASSERT
//

TEST_F(CTaskTests, AssertParam5) {

    taskName = "Test";
    watchFolder = kWatchFolder;
    watchDepth = -99;

    EXPECT_DEATH(CTask task(taskName, watchFolder, taskActFcn, fnData, watchDepth), CTaskTests::kParamAssertion5);

}

//
// Create 1 file in watcher folder
//

TEST_F(CTaskTests, CreateFile1) {

    createFiles(1);

}

//
// Create 10 files in watcher folder
//

TEST_F(CTaskTests, CreateFile10) {

    createFiles(10);

}

//
// Create 50 files in watcher folder
//

TEST_F(CTaskTests, CreateFile50) {

    createFiles(50);

}

//
// Create 100 files in watcher folder
//

TEST_F(CTaskTests, CreateFile100) {

    createFiles(100);

}

//
// Create 250 files in watcher folder
//

TEST_F(CTaskTests, CreateFile250) {

    createFiles(250);

}

//
// Create 500 files in watcher folder
//

TEST_F(CTaskTests, CreateFile500) {

    createFiles(500);

}

//
// Watch folder does not exist exception.
//

TEST_F(CTaskTests, NoWatchFolder) {

    taskName = "Test";
    watchFolder = "/tmp/tnothere";
    watchDepth = -1;

    // Simple test action function that does nothing

    taskActFcn = [] (const std::string& filenamePath, const std::shared_ptr<void> fnData) -> bool {
        TestActFnData *funcData = static_cast<TestActFnData *> (fnData.get());
        return true;
    };

    // Create task object

    EXPECT_THROW(CTask task(taskName, watchFolder, taskActFcn, fnData, watchDepth, taskOptions), std::system_error);

}

//
// Task action throw exception capture.
//

TEST_F(CTaskTests, ActionFunctionException) {

    taskName = "Test";
    watchFolder = kWatchFolder;
    fileName = "tmp.txt";
    watchDepth = -1;

    // Simple test action function that just throws an exception

    taskActFcn = [] (const std::string& filenamePath, const std::shared_ptr<void> fnData) -> bool {
        TestActFnData *funcData = static_cast<TestActFnData *> (fnData.get());
        throw std::logic_error("Just an example.");
        return true;
    };

    // Set any task options required by test

    (taskOptions)->killCount = 1;
    
    // Create task object

    CTask task{taskName, watchFolder, taskActFcn, fnData, watchDepth, taskOptions};

    // Create task object thread and start to watch

    std::unique_ptr<std::thread> taskThread;

    taskThread.reset(new std::thread(&CTask::monitor, &task));
    
    // Create one file to trigger action function

    createFile(watchFolder + fileName);

    // Thread should die after killCount files created

    taskThread->join();

    EXPECT_THROW(generateException(task.getThrownException()), std::logic_error);

    if (fs::exists(watchFolder + fileName)) {
        fs::remove(watchFolder + fileName);
    }

}

// =====================
// RUN GOOGLE UNIT TESTS
// =====================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}