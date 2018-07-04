#include "HOST.hpp"
/*
 * File:   CAppriseTests.cpp
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

// Google test

#include "gtest/gtest.h"

// C++ STL

#include <stdexcept>
#include <fstream>
#include <thread>
#include <sstream>

// CApprise class

#include "CApprise.hpp"

// Used Antik classes

#include "CFile.hpp"
#include "CPath.hpp"

using namespace Antik::File;

// =======================
// UNIT TEST FIXTURE CLASS
// =======================

class CAppriseTests : public ::testing::Test {
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

    CAppriseTests() {
    }

    // Empty destructor

    virtual ~CAppriseTests() {
    }

    // Keep initialization and cleanup code to SetUp() and TearDown() methods

    virtual void SetUp() {

        // Create watch folder.

        if (!CFile::exists(CAppriseTests::kWatchFolder)) {
            CFile::createDirectory(CAppriseTests::kWatchFolder);
        }

        // Create destination folder.

        if (!CFile::exists(CAppriseTests::kDestinationFolder)) {
            CFile::createDirectory(CAppriseTests::kDestinationFolder);
        }

    }

    virtual void TearDown() {

        // Remove watch folder.

        if (CFile::exists(CAppriseTests::kWatchFolder)) {
            CFile::remove(CAppriseTests::kWatchFolder);
        }

        // Remove destination folder.

        if (CFile::exists(CAppriseTests::kDestinationFolder)) {
            CFile::remove(CAppriseTests::kDestinationFolder);
        }

    }

    void createFile(const std::string &fileName);   // Create a test file.
    void createRemoveFiles(int fileCount);          // Create fileCount files and check action function call count
    void createChanges(int updateCount);            // Perform updateCount changes to a file and verify event count
    void generateException(std::exception_ptr e);   // Generate an exception stored in CFileApprise class
    
    // Collect loopCount CFileApprise events
    
    void gatherEvents(CApprise& watcher , EventCounts& evtTotals, int loopCount);

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

const std::string CAppriseTests::kWatchFolder("/tmp/watch/");
const std::string CAppriseTests::kDestinationFolder("/tmp/destination/");

const std::string CAppriseTests::kParamAssertion1("Assertion*"); // NEED TO MODIFY FOR SPECIFIC ASSERTS
const std::string CAppriseTests::kParamAssertion2("Assertion*");

// ===============
// FIXTURE METHODS
// ===============

//
// Create a file for test purposes.
//

void CAppriseTests::createFile(const std::string &fileName) {

    std::ofstream outfile(fileName);
    outfile << "TEST TEXT" << std::endl;
    outfile.close();

}

//
// Loop gathering events and updating totals. We are expecting loopCount events so the only reason for
// the loop not terminating is a major bug.
//

void CAppriseTests::gatherEvents(CApprise& watcher , EventCounts& evtTotals, int loopCount ){
  

    while (watcher.stillWatching() && (loopCount--)) {

        CApprise::Event evt;

        watcher.getEvent(evt);

        if ((evt.id == CApprise::Event_add) && !evt.message.empty()) {
            evtTotals.add++;
        } else if ((evt.id == CApprise::Event_addir) && !evt.message.empty()) {
            evtTotals.addir++;
        } else if ((evt.id == CApprise::Event_unlinkdir) && !evt.message.empty()) {
            evtTotals.unlinkdir++;
        } else if ((evt.id == CApprise::Event_unlink) && !evt.message.empty()) {
            evtTotals.unlink++;
        } else if ((evt.id == CApprise::Event_change) && !evt.message.empty()) {
            evtTotals.change++;
        } else if ((evt.id == CApprise::Event_error) && !evt.message.empty()) {
            evtTotals.error++;
        } 
        
    }
    
}

//
// Generate updateCount changes on a file and verify.
//

void CAppriseTests::createChanges(int updateCount) {

    // Initialise event counts
    
    EventCounts evtTotals { 0, 0, 0, 0, 0, 0};
    
    watchFolder = kWatchFolder;
    watchDepth = -1;
    filePath = watchFolder;
    fileName = "tmp.txt";

    // Create the file
    
    createFile(filePath+fileName);

    // Create CFileApprise object and start watching

    CApprise watcher{watchFolder, watchDepth};

    watcher.startWatching();

    filePath = CAppriseTests::kWatchFolder;

    // Perform updates. Given the nature of modify events (i.e  the number and frequency not being 
    // predictable then perform one up and close file per expected event.
    
    for (auto cnt01 = 0; cnt01 < updateCount; cnt01++) {
            std::ofstream fileToUpdate;
            fileToUpdate.open(filePath+fileName, std::ios::out | std::ios::app);
            fileToUpdate << "Writing this to a file.\n";           
            fileToUpdate.close();
    }
    
    // Loop getting change events

    gatherEvents(watcher, evtTotals, updateCount);
    
    // Check events generated
    
    EXPECT_EQ(0, evtTotals.add);
    EXPECT_EQ(0, evtTotals.addir);
    EXPECT_EQ(0, evtTotals.unlinkdir);
    EXPECT_EQ(0, evtTotals.unlink);
    EXPECT_EQ(updateCount, evtTotals.change);
    EXPECT_EQ(0, evtTotals.error);

    // Remove file
    
    CFile::remove(filePath+fileName);
        
    // Stop watching
    
    watcher.stopWatching();

}

//
// Create fileCount files and then remove. Count add/unlink events and verify.
//

void CAppriseTests::createRemoveFiles(int fileCount) {

    // Initialise event counts
    
    EventCounts evtTotals { 0, 0, 0, 0, 0, 0};
    
    watchFolder = kWatchFolder;
    watchDepth = -1;

    // Create CFileApprise object and start watching

    CApprise watcher{watchFolder, watchDepth};

    watcher.startWatching();

    filePath = CAppriseTests::kWatchFolder;

    // Create fileCount files
    
    for (auto cnt01 = 0; cnt01 < fileCount; cnt01++) {
        std::stringstream file;
        file << "temp" << cnt01 << ".txt" ;
        createFile(filePath + file.str());
    }
    
    // Loop getting add events

    gatherEvents(watcher, evtTotals, fileCount);
    
    // Check events generated
    
    EXPECT_EQ(fileCount, evtTotals.add);
    EXPECT_EQ(0, evtTotals.addir);
    EXPECT_EQ(0, evtTotals.unlinkdir);
    EXPECT_EQ(0, evtTotals.unlink);
    EXPECT_EQ(0, evtTotals.change);
    EXPECT_EQ(0, evtTotals.error);

    // Remove files
    
    for (auto cnt01 = 0; cnt01 < fileCount; cnt01++) {
        std::stringstream file;
        file << "temp" << cnt01 << ".txt" ;
        CFile::remove(filePath + file.str());
    }
    
     // Loop getting unlink events

    evtTotals = { 0, 0, 0, 0, 0, 0};
    gatherEvents(watcher, evtTotals, fileCount);
       
    // Stop watcher
    
    watcher.stopWatching();

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

void CAppriseTests::generateException(std::exception_ptr e) {

    if (e) {
        std::rethrow_exception(e);
    }

}

// =============================
// CFILEAPPRISE CLASS UNIT TESTS
// =============================

//
// Watch Depth < -1 ASSERT
//

TEST_F(CAppriseTests,AssertParam2) {

    watchFolder = kWatchFolder;
    watchDepth = -99;

    EXPECT_DEATH(CApprise watcher(watchFolder, watchDepth), CAppriseTests::kParamAssertion2);

}

//
// Create 1 file in watcher folder
//

TEST_F(CAppriseTests,CreateFile1) {

    createRemoveFiles(1);

}

//
// Create 10 files in watcher folder
//

TEST_F(CAppriseTests, CreateFile10) {

    createRemoveFiles(10);

}

//
// Create 50 files in watcher folder
//

TEST_F(CAppriseTests, CreateFile50) {

    createRemoveFiles(50);

}

//
// Create 100 files in watcher folder
//

TEST_F(CAppriseTests,CreateFile100) {

    createRemoveFiles(100);

}

//
// Create 250 files in watcher folder
//

TEST_F(CAppriseTests, CreateFile250) {

    createRemoveFiles(250);

}

//
// Create 500 files in watcher folder
//

TEST_F(CAppriseTests, CreateFile500) {

    createRemoveFiles(500);

}

//
// Modify file one time
//

TEST_F(CAppriseTests, UpdateFile1) {

    createChanges(1);

}

//
// Modify file 10 times
//

TEST_F(CAppriseTests, UpdateFile10) {

    createChanges(10);

}

//
// Modify file 50 times
//

TEST_F(CAppriseTests, UpdateFile50) {

    createChanges(50);

}

//
// Modify file 100 times
//

TEST_F(CAppriseTests, UpdateFile100) {

    createChanges(100);

}

//
// Modify file 250 times
//

TEST_F(CAppriseTests, UpdateFile250) {

    createChanges(250);

}

//
// Modify file 500 times
//

TEST_F(CAppriseTests, UpdateFile500) {

    createChanges(500);

}

// =====================
// RUN GOOGLE UNIT TESTS
// =====================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}