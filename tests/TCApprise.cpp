#include "HOST.hpp"
/*
 * File:   TCApprise.cpp
 * 
 * Author: Robert Tizzard
 *
 * Created on October 24, 2016, 2:34 PM
 * 
 * Description: Google unit tests for class CApprise.
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
#include "CFileEventNotifier.hpp"

// Used Antik classes

#include "CFile.hpp"
#include "CPath.hpp"

using namespace Antik::File;

// =======================
// UNIT TEST FIXTURE CLASS
// =======================

class TCApprise : public ::testing::Test {
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

    TCApprise() {
    }

    // Empty destructor

    ~TCApprise() override {
    }

    // Keep initialization and cleanup code to SetUp() and TearDown() methods

    void SetUp() override {

        // Create watch folder.

        if (!CFile::exists(TCApprise::kWatchFolder)) {
            CFile::createDirectory(TCApprise::kWatchFolder);
        }

        // Create destination folder.

        if (!CFile::exists(TCApprise::kDestinationFolder)) {
            CFile::createDirectory(TCApprise::kDestinationFolder);
        }

    }

    void TearDown() override {

        // Remove watch folder.

        if (CFile::exists(TCApprise::kWatchFolder)) {
            CFile::remove(TCApprise::kWatchFolder);
        }

        // Remove destination folder.

        if (CFile::exists(TCApprise::kDestinationFolder)) {
            CFile::remove(TCApprise::kDestinationFolder);
        }

    }

    void createFile(const std::string &fileName);   // Create a test file.
    void createFiles(int fileCount);                // Create fileCount files and check action function call count
    void removeFiles(int fileCount);                // RemovefileCount files and check action function call count
    void updateFiles(int updateCount);              // Perform updateCount changes to a file and verify event count
    void createDirectories(int fileCount);          // Create fileCount directories and check action function call count
    void removeDirectories(int fileCount);          // Remove fileCount directories and check action function call count
    void generateException(std::exception_ptr &e);  // Generate an exception stored in CFileApprise class
    
    void gatherEvents(CApprise& watcher , EventCounts& evtTotals, int loopCount); // Collect loopCount CFileApprise events

    int watchDepth { -1 };                        // Folder Watch depth

    EventCounts evtTotals { 0, 0, 0, 0, 0, 0};  // Event totals

    static const std::string kWatchFolder;          // Test Watch Folder
    static const std::string kDestinationFolder;    // Test Destination folder

    static const std::string kParamAssertion2;  // Missing parameter 2 Assert REGEX
 
};

// =================
// FIXTURE CONSTANTS
// =================

const std::string TCApprise::kWatchFolder("/tmp/watch/");
const std::string TCApprise::kDestinationFolder("/tmp/destination/");
const std::string TCApprise::kParamAssertion2("Assertion*");

// ===============
// FIXTURE METHODS
// ===============

//
// Create a file for test purposes.
//

void TCApprise::createFile(const std::string &fileName) {

    std::ofstream outfile(fileName);
    outfile << "TEST TEXT" << std::endl;
    outfile.close();

}

//
// Loop gathering events and updating totals. We are expecting loopCount events so the only reason for
// the loop not terminating is a major bug.
//

void TCApprise::gatherEvents(CApprise& watcher , EventCounts& evtTotals, int loopCount ){
  

    while (watcher.stillWatching() && (loopCount--)) {

        IApprise::Event evt;

        watcher.getNextEvent(evt);

        if ((evt.id == IApprise::Event_add) && !evt.message.empty()) {
            evtTotals.add++;
        } else if ((evt.id == IApprise::Event_addir) && !evt.message.empty()) {
            evtTotals.addir++;
        } else if ((evt.id == IApprise::Event_unlinkdir) && !evt.message.empty()) {
            evtTotals.unlinkdir++;
        } else if ((evt.id == IApprise::Event_unlink) && !evt.message.empty()) {
            evtTotals.unlink++;
        } else if ((evt.id == IApprise::Event_change) && !evt.message.empty()) {
            evtTotals.change++;
        } else if ((evt.id == IApprise::Event_error) && !evt.message.empty()) {
            evtTotals.error++;
        } 
        
    }
    
}

//
// Generate updateCount changes on a file and verify.
//

void TCApprise::updateFiles(int updateCount) {

    // Create the file
    
    createFile(kWatchFolder+"tmp.txt");

    // Create CFileApprise object and start watching

    CApprise watcher{kWatchFolder, watchDepth};

    watcher.startWatching();

    // Perform updates. Given the nature of modify events (i.e  the number and frequency not being 
    // predictable then perform one up and close file per expected event.
    
    for (auto cnt01 = 0; cnt01 < updateCount; cnt01++) {
            std::ofstream fileToUpdate;
            fileToUpdate.open(kWatchFolder+"tmp.txt", std::ios::out | std::ios::app);
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
    
    CFile::remove(kWatchFolder+"tmp.txt");
        
    // Stop watching
    
    watcher.stopWatching();

}

//
// Create fileCount files.Count add events and verify.
//

void TCApprise::createFiles(int fileCount) {
    
    // Create CFileApprise object and start watching

    CApprise watcher{kWatchFolder, watchDepth};

    watcher.startWatching();

    // Create fileCount files
    
    for (auto cnt01 = 0; cnt01 < fileCount; cnt01++) {
        std::stringstream file;
        file << "temp" << cnt01 << ".txt" ;
        createFile(kWatchFolder + file.str());
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
        CFile::remove(kWatchFolder + file.str());
    }
    
    watcher.stopWatching();

}

//
// Create fileCount files and then remove. Count unlink events and verify.
//

void TCApprise::removeFiles(int fileCount) {

    // Create CFileApprise object and start watching

    CApprise watcher{kWatchFolder, watchDepth};

    // Create fileCount files
    
    for (auto cnt01 = 0; cnt01 < fileCount; cnt01++) {
        std::stringstream file;
        file << "temp" << cnt01 << ".txt" ;
        createFile(kWatchFolder + file.str());
    }
    
    watcher.startWatching();

    // Remove files
    
    for (auto cnt01 = 0; cnt01 < fileCount; cnt01++) {
        std::stringstream file;
        file << "temp" << cnt01 << ".txt" ;
        CFile::remove(kWatchFolder + file.str());
    }
    
     // Loop getting unlink events

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
// Create fileCount directroies.Count add events and verify.
//

void TCApprise::createDirectories(int fileCount) {

    // Create CFileApprise object and start watching

    CApprise watcher{kWatchFolder, watchDepth};

    watcher.startWatching();

    // Create fileCount files
    
    for (auto cnt01 = 0; cnt01 < fileCount; cnt01++) {
        std::stringstream file;
        file << "temp" << cnt01;
        CFile::createDirectory(kWatchFolder + file.str());
    }
    
    // Loop getting add events

    gatherEvents(watcher, evtTotals, fileCount);
    
    // Check events generated
    
    EXPECT_EQ(0, evtTotals.add);
    EXPECT_EQ(fileCount, evtTotals.addir);
    EXPECT_EQ(0, evtTotals.unlinkdir);
    EXPECT_EQ(0, evtTotals.unlink);
    EXPECT_EQ(0, evtTotals.change);
    EXPECT_EQ(0, evtTotals.error);

    watcher.stopWatching();
        
    // Remove files
    
    for (auto cnt01 = 0; cnt01 < fileCount; cnt01++) {
        std::stringstream file;
        file << "temp" << cnt01;
        CFile::remove(kWatchFolder + file.str());
    }

}

//
// Remove fileCount directroies.Count add events and verify.
//

void TCApprise::removeDirectories(int fileCount) {

    // Create CFileApprise object and start watching

    CApprise watcher{kWatchFolder, watchDepth};

    // Create fileCount files
    
    for (auto cnt01 = 0; cnt01 < fileCount; cnt01++) {
        std::stringstream file;
        file << "temp" << cnt01;
        CFile::createDirectory(kWatchFolder + file.str());
    }

    watcher.startWatching();
    
    // Remove files
    
    for (auto cnt01 = 0; cnt01 < fileCount; cnt01++) {
        std::stringstream file;
        file << "temp" << cnt01;
        CFile::remove(kWatchFolder + file.str());
    }

    // Loop getting add events

    gatherEvents(watcher, evtTotals, fileCount);
    
    // Check events generated
    
    EXPECT_EQ(0, evtTotals.add);
    EXPECT_EQ(0, evtTotals.addir);
    EXPECT_EQ(fileCount, evtTotals.unlinkdir);
    EXPECT_EQ(0, evtTotals.unlink);
    EXPECT_EQ(0, evtTotals.change);
    EXPECT_EQ(0, evtTotals.error);

    watcher.stopWatching();
    
}

//
// Re-throw any exception passed.
//

void TCApprise::generateException(std::exception_ptr &e) {

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

TEST_F(TCApprise,AssertParam2) {

    watchDepth = -99;

    EXPECT_DEATH(CApprise watcher(kWatchFolder, watchDepth), TCApprise::kParamAssertion2);

}

//
// Create 1 file in watcher folder
//

TEST_F(TCApprise,CreateFile1) {

    createFiles(1);

}

//
// Create 10 files in watcher folder
//

TEST_F(TCApprise, CreateFile10) {

    createFiles(10);

}

//
// Create 50 files in watcher folder
//

TEST_F(TCApprise, CreateFile50) {

    createFiles(50);

}

//
// Create 100 files in watcher folder
//

TEST_F(TCApprise,CreateFile100) {

    createFiles(100);

}

//
// Create 250 files in watcher folder
//

TEST_F(TCApprise, CreateFile250) {

    createFiles(250);

}

//
// Create 500 files in watcher folder
//

TEST_F(TCApprise, CreateFile500) {

    createFiles(500);

}

//
// Update file one time
//

TEST_F(TCApprise, UpdateFile1) {

    updateFiles(1);

}

//
// Update file 10 times
//

TEST_F(TCApprise, UpdateFile10) {

    updateFiles(10);

}

//
// Update file 50 times
//

TEST_F(TCApprise, UpdateFile50) {

    updateFiles(50);

}

//
// Update file 100 times
//

TEST_F(TCApprise, UpdateFile100) {

    updateFiles(100);

}

//
// Update file 250 times
//

TEST_F(TCApprise, UpdateFile250) {

    updateFiles(250);

}

//
// Update file 500 times
//

TEST_F(TCApprise, UpdateFile500) {

    updateFiles(500);

}

//
// Remove 1 file
//

TEST_F(TCApprise, RemoveFile1) {

    removeFiles(1);

}

//
// Remove 10 files
//

TEST_F(TCApprise, RemoveFile10) {

    removeFiles(10);

}

//
// Remove 50 files
//

TEST_F(TCApprise, RemoveFile50) {

    removeFiles(50);

}

//
// Remove 100 files
//

TEST_F(TCApprise, RemoveFile100) {

    removeFiles(100);

}

//
// Remove 250 files
//

TEST_F(TCApprise, RemoveFile250) {

    removeFiles(250);

}

//
// Remove 500 files
//

TEST_F(TCApprise, RemoveFile500) {

    removeFiles(500);

}

//
// Create one directory
//

TEST_F(TCApprise, CreateDirectory1) {

    createDirectories(1);

}

//
// Create 10 directories
//

TEST_F(TCApprise, CreateDirectory10) {

    createDirectories(10);

}

//
// Create 50 directories
//

TEST_F(TCApprise, CreateDirectory50) {

    createDirectories(50);

}

//
// Create 100 directories
//

TEST_F(TCApprise, CreateDirectory100) {

    createDirectories(100);

}

//
// Create 250 directories
//

TEST_F(TCApprise, CreateDirectory250) {

    createDirectories(250);

}

//
// Create 500 directories
//

TEST_F(TCApprise, CreateDirectory500) {

    createDirectories(500);

}

//
// Remove one directory
//

TEST_F(TCApprise, RemoveDirectory1) {

    removeDirectories(1);

}

//
// Remove 10 directories
//

TEST_F(TCApprise, RemoveDirectory10) {

    removeDirectories(10);

}

//
// Remove 50 directories
//

TEST_F(TCApprise, RemoveDirectory50) {

    removeDirectories(50);

}

//
// Remove 100 directories
//

TEST_F(TCApprise, RemoveDirectory100) {

    removeDirectories(100);

}

//
// Remove 250 directories
//

TEST_F(TCApprise, removeDirectory250) {

    removeDirectories(250);

}

//
// Remove 500 directories
//

TEST_F(TCApprise, removeDirectory500) {

    removeDirectories(500);

}

//
// Create watcher with non-existant folder.
//

TEST_F(TCApprise, NonExistantWatchFolder) {

    EXPECT_THROW(new CApprise(kWatchFolder+"x", watchDepth), CApprise::Exception);

}

//
// Add non-existant watch folder
//

TEST_F(TCApprise, AddNonExistantWatchFolder) {

    CApprise watcher;
        
    EXPECT_THROW(watcher.addWatch(kWatchFolder+"x"), CApprise::Exception);
       
}

//
// Remove non-existant watch folder
//

TEST_F(TCApprise, RemoveNonExistantWatchFolder) {

    CApprise watcher;
        
    EXPECT_THROW(watcher.removeWatch(kWatchFolder+"x"), CApprise::Exception);
       
}

// =====================
// RUN GOOGLE UNIT TESTS
// =====================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}