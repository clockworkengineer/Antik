#include "HOST.hpp"
/*
 * File:   TCTask.cpp
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

    bool process(const std::string &file) override {
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
    bool process(const std::string &file) override {
         throw std::logic_error("Just an example.");
         return true;
    }
    virtual ~TestAction2() {
    };
protected:
    std::string name; // Action name
};

class TCTask : public ::testing::Test {
protected:
    
    // Empty constructor

    TCTask() {
    }

    // Empty destructor
    
    ~TCTask() override {
    }
    
    // Keep initialization and cleanup code to SetUp() and TearDown() methods

    void SetUp() override{

        // Create test actions
        
        testTaskAction1.reset(new TestAction1("Test1"));
        testTaskAction2.reset(new TestAction2("Test2"));
        
        // Create watch folder.

        if (!CFile::exists(TCTask::kWatchFolder)) {
            CFile::createDirectory(TCTask::kWatchFolder);
        }

        // Create destination folder.

        if (!CFile::exists(TCTask::kDestinationFolder)) {
            CFile::createDirectory(TCTask::kDestinationFolder);
        }

    }

    void TearDown() override {

        // Remove watch folder.

        if (CFile::exists(TCTask::kWatchFolder)) {
            CFile::remove(TCTask::kWatchFolder);
        }

        // Remove destination folder.

        if (CFile::exists(TCTask::kDestinationFolder)) {
            CFile::remove(TCTask::kDestinationFolder);
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

const std::string TCTask::kWatchFolder("/tmp/watch/");
const std::string TCTask::kDestinationFolder("/tmp/destination/");

const std::string TCTask::kParamAssertion1("Assertion*"); // NEED TO MODIFY FOR SPECIFIC ASSERTS
const std::string TCTask::kParamAssertion2("Assertion*");
const std::string TCTask::kParamAssertion3("Assertion*");
const std::string TCTask::kParamAssertion4("Assertion*");
const std::string TCTask::kParamAssertion5("Assertion*");

// ===============
// FIXTURE METHODS
// ===============

//
// Create a file for test purposes.
//

void TCTask::createFile(std::string fileName) {

    std::ofstream outfile(fileName);
    outfile << "TEST TEXT" << std::endl;
    outfile.close();

}

//
// Create fileCount files and check that action function called for each
//

void TCTask::createFiles(int fileCount) {

    taskName = "Test";
    watchFolder = kWatchFolder;
    watchDepth = -1;

    // Create task object
    
    CTask task{watchFolder, testTaskAction1,  watchDepth, fileCount};
           
    // Create task object thread and start to watch

    std::unique_ptr<std::thread> taskThread;

    taskThread.reset(new std::thread(&CTask::monitor, &task));
     
    filePath = TCTask::kWatchFolder;

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

void TCTask::generateException(const std::exception_ptr &e) {

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

TEST_F(TCTask, AssertParam1) {

    EXPECT_DEATH(CTask task(watchFolder, testTaskAction1, 0, watchDepth), TCTask::kParamAssertion1);

}

//
// Watch Folder Name lengh == 0 ASSERT
//

TEST_F(TCTask, AssertParam2) {

    taskName = "Test";

    EXPECT_DEATH(CTask task(watchFolder, testTaskAction1,  watchDepth, 0), TCTask::kParamAssertion2);

}

//
// Action Function Pointer == NULL ASSERT
//

TEST_F(TCTask, AssertParam3) {

    taskName = "Test";
    watchFolder = kWatchFolder;
    watchDepth = -1;

    EXPECT_DEATH(CTask task(watchFolder, nullptr, watchDepth, 0), TCTask::kParamAssertion3);

}

//
// Action Function Data Pointer == NULL ASSERT
//

TEST_F(TCTask, AssertParam4) {

    taskName = "Test";
    watchFolder = kWatchFolder;
    watchDepth = -1;

    EXPECT_DEATH(CTask task(watchFolder, nullptr, watchDepth, 0), TCTask::kParamAssertion4);

}

//
// Watch Depth < -1 ASSERT
//

TEST_F(TCTask, AssertParam5) {

    taskName = "Test";
    watchFolder = kWatchFolder;
    watchDepth = -99;

    EXPECT_DEATH(CTask task(watchFolder, testTaskAction1, watchDepth, 0), TCTask::kParamAssertion5);

}

//
// Create 1 file in watcher folder
//

TEST_F(TCTask, CreateFile1) {

    createFiles(1);

}

//
// Create 10 files in watcher folder
//

TEST_F(TCTask, CreateFile10) {

    createFiles(10);

}

//
// Create 50 files in watcher folder
//

TEST_F(TCTask, CreateFile50) {

    createFiles(50);

}

//
// Create 100 files in watcher folder
//

TEST_F(TCTask, CreateFile100) {

    createFiles(100);

}

//
// Create 250 files in watcher folder
//

TEST_F(TCTask, CreateFile250) {

    createFiles(250);

}

//
// Create 500 files in watcher folder
//

TEST_F(TCTask, CreateFile500) {

    createFiles(500);

}

//
// Watch folder does not exist exception.
//

TEST_F(TCTask, NoWatchFolder) {

    taskName = "Test";
    watchFolder = "/tmp/tnothere";
    watchDepth = -1;

    // Create task object

    EXPECT_THROW(CTask task(watchFolder, testTaskAction1, watchDepth, 0), CApprise::Exception);

}

//
// Task action throw exception capture.
//

TEST_F(TCTask, ActionFunctionException) {

    taskName = "Test";
    watchFolder = kWatchFolder;
    fileName = "tmp.txt";
    watchDepth = -1;
    
    // Create task object

    CTask task{watchFolder, testTaskAction2, watchDepth, 0};

    // Create task object thread and start to watch

    std::unique_ptr<std::thread> taskThread;

    taskThread.reset(new std::thread(&CTask::monitor, &task));
    
    // Create one file to trigger action function

    createFile(watchFolder + fileName);

    // Thread should die after killCount files created

    taskThread->join();

    EXPECT_THROW(generateException(task.getThrownException()), std::logic_error);

    if (CFile::exists(watchFolder + fileName)) {
        CFile::remove(watchFolder + fileName);
    }

}

// =====================
// RUN GOOGLE UNIT TESTS
// =====================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}