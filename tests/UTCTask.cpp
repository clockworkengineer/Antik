/*
 * File:   UTCTask.cpp
 * 
 * Author: Robert Tizzard
 *
 * Created on October 24, 2016, 2:34 PM
 * 
 * Description: Google unit tests for class CTask.
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
#include <sstream>

// CTask class

#include "CTask.hpp"

// Used Antik classes

#include "CFile.hpp"
#include "CPath.hpp"

using namespace Antik::File;

// =======================
// UNIT TEST FIXTURE CLASS
// =======================

class TestAction1 : public CTask::IAction {
public:

    explicit TestAction1(const std::string &taskName) : name{taskName}
    {
    }

    void init(void) override {
    };

    void term(void) override {
    };

    bool process([[maybe_unused]] const std::string &file) override {
        fileCount++;
        return true;
    }

    virtual ~TestAction1() {

    };

    int fileCount{ 0};
    
protected:
    std::string name; // Action name
};

class TestAction2: public CTask::IAction {
public:

    explicit TestAction2(const std::string &taskName) : name{taskName}
    {
    }

    void init(void) override {};
    void term(void) override {} ;
    bool process([[maybe_unused]] const std::string &file) override {
         throw std::logic_error("Just an example.");
         return true;
    }
    virtual ~TestAction2() {
    };
protected:
    std::string name; // Action name
};

class UTCTask : public ::testing::Test {
protected:
    
    // Empty constructor

    UTCTask() {
    }

    // Empty destructor
    
    ~UTCTask() override {
    }
    
    // Keep initialization and cleanup code to SetUp() and TearDown() methods

    void SetUp() override{

        // Create test actions
        
        testTaskAction1 = std::make_shared<TestAction1>("Test1");
        testTaskAction2 = std::make_shared<TestAction2>("Test2");
        
        // Create watch folder.

        if (!CFile::exists(UTCTask::kWatchFolder)) {
            CFile::createDirectory(UTCTask::kWatchFolder);
        }

        // Create destination folder.

        if (!CFile::exists(UTCTask::kDestinationFolder)) {
            CFile::createDirectory(UTCTask::kDestinationFolder);
        }

    }

    void TearDown() override {

        // Remove watch folder.

        if (CFile::exists(UTCTask::kWatchFolder)) {
            CFile::remove(UTCTask::kWatchFolder);
        }

        // Remove destination folder.

        if (CFile::exists(UTCTask::kDestinationFolder)) {
            CFile::remove(UTCTask::kDestinationFolder);
        }

    }

    void createFile(std::string fileName);           // Create a test file.
    void createFiles(int fileCount);                 // Create fileCount files and check action function call count
    void generateException(const std::exception_ptr &e);

    std::string filePath = "";      // Test file path
    std::string fileName = "";      // Test file name
    int watchDepth = -1;            // Folder Watch depth
    std::string taskName = "";      // Task Name
    std::string watchFolder = "";   // Watch Folder

    std::shared_ptr<TestAction1> testTaskAction1; // Test Action 1
    std::shared_ptr<TestAction2> testTaskAction2; // Test Action 2

    static const std::string kWatchFolder;       // Test Watch Folder
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

const std::string UTCTask::kWatchFolder("/tmp/watch/");
const std::string UTCTask::kDestinationFolder("/tmp/destination/");

const std::string UTCTask::kParamAssertion1("Assertion*"); // NEED TO MODIFY FOR SPECIFIC ASSERTS
const std::string UTCTask::kParamAssertion2("Assertion*");
const std::string UTCTask::kParamAssertion3("Assertion*");
const std::string UTCTask::kParamAssertion4("Assertion*");
const std::string UTCTask::kParamAssertion5("Assertion*");

// ===============
// FIXTURE METHODS
// ===============

//
// Create a file for test purposes.
//

void UTCTask::createFile(std::string fileName) {

    std::ofstream outfile(fileName);
    outfile << "TEST TEXT" << std::endl;
    outfile.close();

}

//
// Create fileCount files and check that action function called for each
//

void UTCTask::createFiles(int fileCount) {

    taskName = "Test";
    watchFolder = kWatchFolder;
    watchDepth = -1;

    // Create task object
    
    CTask task{watchFolder, testTaskAction1,  watchDepth, fileCount};
           
    // Create task object thread and start to watch

    std::unique_ptr<std::thread> taskThread;

    taskThread = std::make_unique<std::thread>(&CTask::monitor, &task);
     
    filePath = UTCTask::kWatchFolder;

    for (auto cnt01 = 0; cnt01 < fileCount; cnt01++) {
        std::stringstream file;
        file << "temp" << cnt01 << ".txt" ;
        createFile(filePath + file.str());
    }

    // Thread should die after killCount files created

    taskThread->join();

    EXPECT_EQ(fileCount, testTaskAction1->fileCount);

    for (auto cnt01 = 0; cnt01 < fileCount; cnt01++) {
        std::stringstream file;
        file << "temp" << cnt01 << ".txt" ;
        CFile::remove(filePath + file.str());
    }

}

//
// Re-throw any exception passed.
//

void UTCTask::generateException(const std::exception_ptr &e) {

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

TEST_F(UTCTask, AssertParam1) {

    EXPECT_DEATH(CTask task(watchFolder, testTaskAction1, 0, watchDepth), UTCTask::kParamAssertion1);

}

//
// Watch Folder Name lengh == 0 ASSERT
//

TEST_F(UTCTask, AssertParam2) {

    taskName = "Test";

    EXPECT_DEATH(CTask task(watchFolder, testTaskAction1,  watchDepth, 0), UTCTask::kParamAssertion2);

}

//
// Action Function Pointer == NULL ASSERT
//

TEST_F(UTCTask, AssertParam3) {

    taskName = "Test";
    watchFolder = kWatchFolder;
    watchDepth = -1;

    EXPECT_DEATH(CTask task(watchFolder, nullptr, watchDepth, 0), UTCTask::kParamAssertion3);

}

//
// Action Function Data Pointer == NULL ASSERT
//

TEST_F(UTCTask, AssertParam4) {

    taskName = "Test";
    watchFolder = kWatchFolder;
    watchDepth = -1;

    EXPECT_DEATH(CTask task(watchFolder, nullptr, watchDepth, 0), UTCTask::kParamAssertion4);

}

//
// Watch Depth < -1 ASSERT
//

TEST_F(UTCTask, AssertParam5) {

    taskName = "Test";
    watchFolder = kWatchFolder;
    watchDepth = -99;

    EXPECT_DEATH(CTask task(watchFolder, testTaskAction1, watchDepth, 0), UTCTask::kParamAssertion5);

}

//
// Create 1 file in watcher folder
//

TEST_F(UTCTask, CreateFile1) {

    createFiles(1);

}

//
// Create 10 files in watcher folder
//

TEST_F(UTCTask, CreateFile10) {

    createFiles(10);

}

//
// Create 50 files in watcher folder
//

TEST_F(UTCTask, CreateFile50) {

    createFiles(50);

}

//
// Create 100 files in watcher folder
//

TEST_F(UTCTask, CreateFile100) {

    createFiles(100);

}

//
// Create 250 files in watcher folder
//

TEST_F(UTCTask, CreateFile250) {

    createFiles(250);

}

//
// Create 500 files in watcher folder
//

TEST_F(UTCTask, CreateFile500) {

    createFiles(500);

}

//
// Watch folder does not exist exception.
//

TEST_F(UTCTask, NoWatchFolder) {

    taskName = "Test";
    watchFolder = "/tmp/tnothere";
    watchDepth = -1;

    // Create task object

    EXPECT_THROW(CTask task(watchFolder, testTaskAction1, watchDepth, 0), CApprise::Exception);

}

//
// Task action throw exception capture.
//

TEST_F(UTCTask, ActionFunctionException) {

    taskName = "Test";
    watchFolder = kWatchFolder;
    fileName = "tmp.txt";
    watchDepth = -1;
    
    // Create task object

    CTask task{watchFolder, testTaskAction2, watchDepth, 0};

    // Create task object thread and start to watch

    std::unique_ptr<std::thread> taskThread;

    taskThread = std::make_unique<std::thread>(&CTask::monitor, &task);
    
    // Create one file to trigger action function

    createFile(watchFolder + fileName);

    // Thread should die after killCount files created

    taskThread->join();

    EXPECT_THROW(generateException(task.getThrownException()), std::logic_error);

    if (CFile::exists(watchFolder + fileName)) {
        CFile::remove(watchFolder + fileName);
    }

}