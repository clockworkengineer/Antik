#include "HOST.hpp"
/*
 * File:   CFileAppriseTests.cpp
 * 
 * Author: Robert Tizzard
 *
 * Created on October 24, 2016, 2:34 PM
 * 
 * Description: Google unit tests for classCFileApprise.
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
#include <fstream>

// CFileApprise class definitions

#include "CFileApprise.hpp" 

// Boost file system and format libraries definitions

#include <boost/filesystem.hpp> 
#include <boost/format.hpp>

namespace fs = boost::filesystem;

// =======================
// UNIT TEST FIXTURE CLASS
// =======================

class CFileAppriseTests : public ::testing::Test {
protected:

    // Event counts
    
    struct EventCounts {
        int add;          // File added to watched folder hierarchy
        int change;       // File changed
        int unlink;       // File deleted from watched folder hierarchy
        int addir;        // Directory added to watched folder hierarchy
        int unlinkdir;    // Directory deleted from watched folder hierarchy
        int error;        // Exception error
    };

    // Empty constructor

    CFileAppriseTests() {
    }

    // Empty destructor

    virtual ~CFileAppriseTests() {
    }

    // Keep initialization and cleanup code to SetUp() and TearDown() methods

    virtual void SetUp() {

        // Create watch folder.

        if (!fs::exists(CFileAppriseTests::kWatchFolder)) {
            fs::create_directory(CFileAppriseTests::kWatchFolder);
        }

        // Create destination folder.

        if (!fs::exists(CFileAppriseTests::kDestinationFolder)) {
            fs::create_directory(CFileAppriseTests::kDestinationFolder);
        }

    }

    virtual void TearDown() {

        // Remove watch folder.

        if (fs::exists(CFileAppriseTests::kWatchFolder)) {
            fs::remove(CFileAppriseTests::kWatchFolder);
        }

        // Remove destination folder.

        if (fs::exists(CFileAppriseTests::kDestinationFolder)) {
            fs::remove(CFileAppriseTests::kDestinationFolder);
        }

    }

    void createFile(std::string fileName);          // Create a test file.
    void createRemoveFiles(int fileCount);          // Create fileCount files and check action function call count
    void createChanges(int updateCount);            // Perform updateCount changes to a file and verify event count
    void generateException(std::exception_ptr e);   // Generate an exception stored in CFileApprise class
    
    // Collect loopCount CFileApprise events
    
    void gatherEvents(CFileApprise& watcher , EventCounts& evtTotals, int loopCount);

    std::string filePath = "";          // Test file path
    std::string fileName = "";          // Test file name
    int watchDepth = -1;                // Folder Watch depth
    std::string watchFolder = "";       // Watch Folder

    static const std::string kWatchFolder;          // Test Watch Folder
    static const std::string kDestinationFolder;    // Test Destination folder

    static const std::string kParamAssertion1;  // Missing parameter 1 Assert REGEX
    static const std::string kParamAssertion2;  // Missing parameter 2 Assert REGEX
 
};

// =================
// FIXTURE CONSTANTS
// =================

const std::string CFileAppriseTests::kWatchFolder("/tmp/watch/");
const std::string CFileAppriseTests::kDestinationFolder("/tmp/destination/");

const std::string CFileAppriseTests::kParamAssertion1("Assertion*"); // NEED TO MODIFY FOR SPECIFIC ASSERTS
const std::string CFileAppriseTests::kParamAssertion2("Assertion*");

// ===============
// FIXTURE METHODS
// ===============

//
// Create a file for test purposes.
//

void CFileAppriseTests::createFile(std::string fileName) {

    std::ofstream outfile(fileName);
    outfile << "TEST TEXT" << std::endl;
    outfile.close();

}

//
// Loop gathering events and updating totals. We are expecting loopCount events so the only reason for
// the loop not terminating is a major bug.
//

void CFileAppriseTests::gatherEvents(CFileApprise& watcher , EventCounts& evtTotals, int loopCount ){
  

    while (watcher.stillWatching() && (loopCount--)) {

        CFileApprise::Event evt;

        watcher.getEvent(evt);

        if ((evt.id == CFileApprise::Event_add) && !evt.message.empty()) {
            evtTotals.add++;
        } else if ((evt.id == CFileApprise::Event_addir) && !evt.message.empty()) {
            evtTotals.addir++;
        } else if ((evt.id == CFileApprise::Event_unlinkdir) && !evt.message.empty()) {
            evtTotals.unlinkdir++;
        } else if ((evt.id == CFileApprise::Event_unlink) && !evt.message.empty()) {
            evtTotals.unlink++;
        } else if ((evt.id == CFileApprise::Event_change) && !evt.message.empty()) {
            evtTotals.change++;
        } else if ((evt.id == CFileApprise::Event_error) && !evt.message.empty()) {
            evtTotals.error++;
        } 
        
    }
    
}

//
// Generate updateCount changes on a file and verify.
//

void CFileAppriseTests::createChanges(int updateCount) {

    // Initialise event counts
    
    EventCounts evtTotals { 0, 0, 0, 0, 0, 0};
    
    this->watchFolder = kWatchFolder;
    this->watchDepth = -1;
    this->filePath = this->watchFolder;
    this->fileName = "tmp.txt";

    // Create the file
    
    this->createFile(this->filePath+this->fileName);
       
    // Setup CFileApprise options

    std::shared_ptr<CFileApprise::Options> watchOptions;
    watchOptions.reset(new CFileApprise::Options{0, false, CLogger::noOp, CLogger::noOp});

    // Create CFileApprise object

    CFileApprise watcher{this->watchFolder, this->watchDepth, watchOptions};

    // Create CFileApprise object thread and start to watch

    std::unique_ptr<std::thread> watcherThread;
    watcherThread.reset(new std::thread(&CFileApprise::watch, &watcher));

    this->filePath = CFileAppriseTests::kWatchFolder;

    // Perform updates. Given the nature of modify events (i.e  the number and frequency not being 
    // predictable then perform one up and close file per expected event.
    
    for (auto cnt01 = 0; cnt01 < updateCount; cnt01++) {
            std::ofstream fileToUpdate;
            fileToUpdate.open(this->filePath+this->fileName, std::ios::out | std::ios::app);
            fileToUpdate << "Writing this to a file.\n";           
            fileToUpdate.close();
    }
    
    // Loop getting change events

    this->gatherEvents(watcher, evtTotals, updateCount);
    
    // Check events generated
    
    EXPECT_EQ(0, evtTotals.add);
    EXPECT_EQ(0, evtTotals.addir);
    EXPECT_EQ(0, evtTotals.unlinkdir);
    EXPECT_EQ(0, evtTotals.unlink);
    EXPECT_EQ(updateCount, evtTotals.change);
    EXPECT_EQ(0, evtTotals.error);

    // Remove file
    
    fs::remove(this->filePath+this->fileName);
        
    // Stop watcher
    
    watcher.stop();
    watcherThread->join();

}

//
// Create fileCount files and then remove. Count add/unlink events and verify.
//

void CFileAppriseTests::createRemoveFiles(int fileCount) {

    // Initialise event counts
    
    EventCounts evtTotals { 0, 0, 0, 0, 0, 0};
    
    this->watchFolder = kWatchFolder;
    this->watchDepth = -1;

    // Setup CFileApprise options

    std::shared_ptr<CFileApprise::Options> watchOptions;
    watchOptions.reset(new CFileApprise::Options{0, false, CLogger::noOp, CLogger::noOp});

    // Create CFileApprise object

    CFileApprise watcher{this->watchFolder, this->watchDepth, watchOptions};

    // Create CFileApprise object thread and start to watch

    std::unique_ptr<std::thread> watcherThread;
    watcherThread.reset(new std::thread(&CFileApprise::watch, &watcher));

    this->filePath = CFileAppriseTests::kWatchFolder;

    // Create fileCount files
    
    for (auto cnt01 = 0; cnt01 < fileCount; cnt01++) {
        std::string file = (boost::format("temp%1%.txt") % cnt01).str();
        this->createFile(this->filePath + file);
    }
    
    // Loop getting add events

    this->gatherEvents(watcher, evtTotals, fileCount);
    
    // Check events generated
    
    EXPECT_EQ(fileCount, evtTotals.add);
    EXPECT_EQ(0, evtTotals.addir);
    EXPECT_EQ(0, evtTotals.unlinkdir);
    EXPECT_EQ(0, evtTotals.unlink);
    EXPECT_EQ(0, evtTotals.change);
    EXPECT_EQ(0, evtTotals.error);

    // Remove files
    
    for (auto cnt01 = 0; cnt01 < fileCount; cnt01++) {
        std::string file = (boost::format("temp%1%.txt") % cnt01).str();
        fs::remove(this->filePath + file);
    }
    
     // Loop getting unlink events

    evtTotals = { 0, 0, 0, 0, 0, 0};
    this->gatherEvents(watcher, evtTotals, fileCount);
       
    // Stop watcher
    
    watcher.stop();
    watcherThread->join();

    // Check events generated
    
    EXPECT_EQ(0, evtTotals.add);
    EXPECT_EQ(0, evtTotals.addir);
    EXPECT_EQ(0, evtTotals.unlinkdir);
    EXPECT_EQ(fileCount, evtTotals.unlink);
    EXPECT_EQ(0, evtTotals.change);
    EXPECT_EQ(0, evtTotals.error);

}

//
// Re-throw any exception passed.
//

void CFileAppriseTests::generateException(std::exception_ptr e) {

    if (e) {
        std::rethrow_exception(e);
    }

}

// =============================
// CFILEAPPRISE CLASS UNIT TESTS
// =============================

//
// Watch Folder Name lengh == 0 ASSERT
//

TEST_F(CFileAppriseTests,CFileAppriseAssertParam1) {

    //  this->taskName = "Test";

    EXPECT_DEATH(CFileApprise watcher(this->watchFolder, this->watchDepth), CFileAppriseTests::kParamAssertion1);

}

//
// Watch Depth < -1 ASSERT
//

TEST_F(CFileAppriseTests,CFileAppriseAssertParam2) {

    //   this->taskName = "Test";
    this->watchFolder = kWatchFolder;
    this->watchDepth = -99;

    EXPECT_DEATH(CFileApprise watcher(this->watchFolder, this->watchDepth), CFileAppriseTests::kParamAssertion2);

}

//
// Create 1 file in watcher folder
//

TEST_F(CFileAppriseTests,CFileAppriseCreateFile1) {

    this->createRemoveFiles(1);

}

//
// Create 10 files in watcher folder
//

TEST_F(CFileAppriseTests,CFileAppriseCreateFile10) {

    this->createRemoveFiles(10);

}

//
// Create 50 files in watcher folder
//

TEST_F(CFileAppriseTests,CFileAppriseCreateFile50) {

    this->createRemoveFiles(50);

}

//
// Create 100 files in watcher folder
//

TEST_F(CFileAppriseTests,CFileAppriseCreateFile100) {

    this->createRemoveFiles(100);

}

//
// Create 250 files in watcher folder
//

TEST_F(CFileAppriseTests,CFileAppriseCreateFile250) {

    this->createRemoveFiles(250);

}

//
// Create 500 files in watcher folder
//

TEST_F(CFileAppriseTests,CFileAppriseCreateFile500) {

    this->createRemoveFiles(500);

}

//
// Modify file one time
//

TEST_F(CFileAppriseTests,CFileAppriseUpdateFile1) {

    this->createChanges(1);

}

//
// Modify file 10 times
//

TEST_F(CFileAppriseTests,CFileAppriseUpdateFile10) {

    this->createChanges(10);

}

//
// Modify file 50 times
//

TEST_F(CFileAppriseTests,CFileAppriseUpdateFile50) {

    this->createChanges(50);

}

//
// Modify file 100 times
//

TEST_F(CFileAppriseTests,CFileAppriseUpdateFile100) {

    this->createChanges(100);

}

//
// Modify file 250 times
//

TEST_F(CFileAppriseTests,CFileAppriseUpdateFile250) {

    this->createChanges(250);

}

//
// Modify file 500 times
//

TEST_F(CFileAppriseTests,CFileAppriseUpdateFile500) {

    this->createChanges(500);

}

// =====================
// RUN GOOGLE UNIT TESTS
// =====================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}