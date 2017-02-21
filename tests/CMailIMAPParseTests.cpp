#include "HOST.hpp"
/*
 * File:   CMailIMAPParseTests.cpp
 * 
 * Author: Robert Tizzard
 *
 * Created on October 24, 2016, 2:34 PM
 * 
 * Description: Google unit tests for class CMailIMAPParse.
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

// CMailIMAPParse class definitions

#include "CMailIMAP.hpp" 
#include "CMailIMAPParse.hpp"


// =======================
// UNIT TEST FIXTURE CLASS
// =======================

class CMailIMAPParseTests : public ::testing::Test {
protected:

    // Empty constructor

    CMailIMAPParseTests() {
    }

    // Empty destructor

    virtual ~CMailIMAPParseTests() {
    }

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

TEST_F(CMailIMAPParseTests, SELECTValid) {

    std::vector<std::string> selectResponseStr = {
        { "A000001 SELECT INBOX"},
        { "* 1 EXISTS"},
        { "* 0 RECENT"},
        { "* FLAGS (\\Seen \\Answered \\Flagged \\Deleted \\Draft $MDNSent)"},
        { "* OK [PERMANENTFLAGS (\\Seen \\Answered \\Flagged \\Deleted \\Draft $MDNSent)] Permanent flags"},
        { "* OK [UIDVALIDITY 14] UIDVALIDITY value"},
        { "* OK [UIDNEXT 4554] The next unique identifier value"},
        { "A000001 OK [READ-WRITE] SELECT completed."}
    };
    
    std::string commandResponseStr;
 
    for (auto str : selectResponseStr) {
        commandResponseStr.append( str + CMailIMAP::kEOLStr);
    }
    
    CMailIMAPParse::BASERESPONSE parsedResponse(CMailIMAPParse::parseResponse(commandResponseStr));
    CMailIMAPParse::SelectResponse *ptr = static_cast<CMailIMAPParse::SelectResponse *> (parsedResponse.get());
  
    EXPECT_TRUE(ptr->status==CMailIMAPParse::RespCode::OK);
    EXPECT_TRUE(ptr->responseMap.find("EXISTS")!=ptr->responseMap.end());
    EXPECT_TRUE(ptr->responseMap.find("RECENT")!=ptr->responseMap.end());
    EXPECT_TRUE(ptr->responseMap.find("FLAGS")!=ptr->responseMap.end());
    EXPECT_TRUE(ptr->responseMap.find("PERMANENTFLAGS")!=ptr->responseMap.end());
    EXPECT_TRUE(ptr->responseMap.find("UIDVALIDITY")!=ptr->responseMap.end());
    EXPECT_TRUE(ptr->responseMap.find("UIDNEXT")!=ptr->responseMap.end());

    ASSERT_STREQ("INBOX", CMailIMAPParse::stringToUpper(ptr->mailBoxNameStr).c_str());
    ASSERT_STREQ( "READ-WRITE", ptr->mailBoxAccessStr.c_str());
    ASSERT_STREQ("1", ptr->responseMap["EXISTS"].c_str());
    ASSERT_STREQ("0", ptr->responseMap["RECENT"].c_str());    
    ASSERT_STREQ("(\\Seen \\Answered \\Flagged \\Deleted \\Draft $MDNSent)", ptr->responseMap["FLAGS"].c_str());
    ASSERT_STREQ( "(\\Seen \\Answered \\Flagged \\Deleted \\Draft $MDNSent)", ptr->responseMap["PERMANENTFLAGS"].c_str());
    ASSERT_STREQ("14", ptr->responseMap["UIDVALIDITY"].c_str());    
    ASSERT_STREQ("4554", ptr->responseMap["UIDNEXT"].c_str());    

}

TEST_F(CMailIMAPParseTests, SELECTInvalidMailBox) {

    std::vector<std::string> selectResponseStr = {
        { "A000002 SELECT NOTHERE"},
        { "A000002 NO NOTHERE doesn't exist."}
    };
    
    std::string commandResponseStr;
 
    for (auto str : selectResponseStr) {
        commandResponseStr.append( str + CMailIMAP::kEOLStr);
    }
    
    CMailIMAPParse::BASERESPONSE parsedResponse(CMailIMAPParse::parseResponse(commandResponseStr));
    CMailIMAPParse::SelectResponse *ptr = static_cast<CMailIMAPParse::SelectResponse *> (parsedResponse.get());
    
    EXPECT_TRUE(ptr->status==CMailIMAPParse::RespCode::NO);
    ASSERT_STREQ("A000002 NO NOTHERE doesn't exist.", ptr->errorMessageStr.c_str());

}

TEST_F(CMailIMAPParseTests, EXAMINEValid) {

    std::vector<std::string> examineResponseStr = {
        {"A000002 EXAMINE INBOX"},
        { "* 11 EXISTS"},
        { "* 0 RECENT"},
        { "* FLAGS (\\Seen \\Answered \\Flagged \\Deleted \\Draft $MDNSent)"},
        { "* OK [PERMANENTFLAGS ()] Permanent flags"},
        { "* OK [UNSEEN 1] Is the first unseen message"},
        { "* OK [UIDVALIDITY 18] UIDVALIDITY value"},
        { "* OK [UIDNEXT 4584] The next unique identifier value"},
        { "A000002 OK [READ-ONLY] EXAMINE completed."}
      };
    
    std::string commandResponseStr;
 
    for (auto str : examineResponseStr) {
        commandResponseStr.append( str + CMailIMAP::kEOLStr);
    }
    
    CMailIMAPParse::BASERESPONSE parsedResponse(CMailIMAPParse::parseResponse(commandResponseStr));
    CMailIMAPParse::ExamineResponse *ptr = static_cast<CMailIMAPParse::ExamineResponse *> (parsedResponse.get());
  
    EXPECT_TRUE(ptr->status==CMailIMAPParse::RespCode::OK);
    EXPECT_TRUE(ptr->responseMap.find("EXISTS")!=ptr->responseMap.end());
    EXPECT_TRUE(ptr->responseMap.find("RECENT")!=ptr->responseMap.end());
    EXPECT_TRUE(ptr->responseMap.find("FLAGS")!=ptr->responseMap.end());
    EXPECT_TRUE(ptr->responseMap.find("PERMANENTFLAGS")!=ptr->responseMap.end());
    EXPECT_TRUE(ptr->responseMap.find("UIDVALIDITY")!=ptr->responseMap.end());
    EXPECT_TRUE(ptr->responseMap.find("UIDNEXT")!=ptr->responseMap.end());

    ASSERT_STREQ("INBOX", CMailIMAPParse::stringToUpper(ptr->mailBoxNameStr).c_str());
    ASSERT_STREQ( "READ-ONLY", ptr->mailBoxAccessStr.c_str());
    ASSERT_STREQ("11", ptr->responseMap["EXISTS"].c_str());
    ASSERT_STREQ("0", ptr->responseMap["RECENT"].c_str());    
    ASSERT_STREQ("(\\Seen \\Answered \\Flagged \\Deleted \\Draft $MDNSent)", ptr->responseMap["FLAGS"].c_str());
    ASSERT_STREQ( "()", ptr->responseMap["PERMANENTFLAGS"].c_str());
    ASSERT_STREQ("18", ptr->responseMap["UIDVALIDITY"].c_str());    
    ASSERT_STREQ("4584", ptr->responseMap["UIDNEXT"].c_str());    

}

TEST_F(CMailIMAPParseTests, EXAMINEInvalidMailBox) {

    std::vector<std::string> examineResponseStr = {
        { "A000002 EXAMINE NOTTHERE"},
        { "A000002 NO NOTHERE doesn't exist."}
    };
    
    std::string commandResponseStr;
 
    for (auto str : examineResponseStr) {
        commandResponseStr.append( str + CMailIMAP::kEOLStr);
    }
    
    CMailIMAPParse::BASERESPONSE parsedResponse(CMailIMAPParse::parseResponse(commandResponseStr));
    CMailIMAPParse::ExamineResponse *ptr = static_cast<CMailIMAPParse::ExamineResponse *> (parsedResponse.get());
    
    EXPECT_TRUE(ptr->status==CMailIMAPParse::RespCode::NO);
    ASSERT_STREQ("A000002 NO NOTHERE doesn't exist.", ptr->errorMessageStr.c_str());

}

TEST_F(CMailIMAPParseTests, STATUSValid) {

    std::vector<std::string> statusResponseStr = {
       { "A000003 STATUS INBOX (UIDNEXT MESSAGES RECENT UIDVALIDITY UNSEEN)" },
       { "* STATUS Inbox (UIDNEXT 4584 MESSAGES 11 RECENT 0 UIDVALIDITY 14 UNSEEN 2)" }, 
       { "A000003 OK STATUS completed."}
    };
    
    std::string commandResponseStr;
 
    for (auto str : statusResponseStr) {
        commandResponseStr.append( str + CMailIMAP::kEOLStr);
    }
    
    CMailIMAPParse::BASERESPONSE parsedResponse(CMailIMAPParse::parseResponse(commandResponseStr));
    CMailIMAPParse::StatusResponse *ptr = static_cast<CMailIMAPParse::StatusResponse *> (parsedResponse.get());

    EXPECT_TRUE(ptr->status == CMailIMAPParse::RespCode::OK);
    EXPECT_TRUE(ptr->responseMap.find("UIDNEXT") != ptr->responseMap.end());
    EXPECT_TRUE(ptr->responseMap.find("MESSAGES") != ptr->responseMap.end());
    EXPECT_TRUE(ptr->responseMap.find("RECENT") != ptr->responseMap.end());
    EXPECT_TRUE(ptr->responseMap.find("UIDVALIDITY") != ptr->responseMap.end());
    EXPECT_TRUE(ptr->responseMap.find("UNSEEN") != ptr->responseMap.end());

    ASSERT_STREQ("INBOX", CMailIMAPParse::stringToUpper(ptr->mailBoxNameStr).c_str());
    ASSERT_STREQ("4584", ptr->responseMap["UIDNEXT"].c_str());
    ASSERT_STREQ("11", ptr->responseMap["MESSAGES"].c_str());
    ASSERT_STREQ("0", ptr->responseMap["RECENT"].c_str());
    ASSERT_STREQ("14", ptr->responseMap["UIDVALIDITY"].c_str());
    ASSERT_STREQ("2", ptr->responseMap["UNSEEN"].c_str());

}

TEST_F(CMailIMAPParseTests, STATUSInvalidMailBox) {

    std::vector<std::string> statusResponseStr = {
        { "A000002 STATUS NOTTHERE (UIDNEXT MESSAGES RECENT UIDVALIDITY UNSEEN)"},
        { "A000002 NO NOTHERE doesn't exist."}
    };
    
    std::string commandResponseStr;
 
    for (auto str : statusResponseStr) {
        commandResponseStr.append( str + CMailIMAP::kEOLStr);
    }
    
    CMailIMAPParse::BASERESPONSE parsedResponse(CMailIMAPParse::parseResponse(commandResponseStr));
    CMailIMAPParse::StatusResponse *ptr = static_cast<CMailIMAPParse::StatusResponse *> (parsedResponse.get());
    
    EXPECT_TRUE(ptr->status==CMailIMAPParse::RespCode::NO);
    ASSERT_STREQ("A000002 NO NOTHERE doesn't exist.", ptr->errorMessageStr.c_str());

}

TEST_F(CMailIMAPParseTests, LISTValid) {

    std::vector<std::string> listResponseStr = { 
        { "A000002 LIST \"\" *"},
        { "* LIST (\\HasNoChildren) \"/\" \"DDNS\""},
        { "* LIST (\\HasNoChildren) \"/\" \"EDO\""},
        { "* LIST (\\HasNoChildren) \"/\" \"INBOX\""},
        { "* LIST (\\HasNoChildren) \"/\" \"Microsoft\""},
        { "* LIST (\\HasNoChildren) \"/\" \"Personal\""},
        { "* LIST (\\HasNoChildren) \"/\" \"Receipts\""},
        { "* LIST (\\HasNoChildren) \"/\" \"Sent\""},
        { "* LIST (\\HasNoChildren) \"/\" \"Trash\""},
        { "* LIST (\\HasNoChildren) \"/\" \"Travel\""},
        { "* LIST (\\HasNoChildren) \"/\" \"Work\""},
        { "* LIST (\\HasChildren \\Noselect) \"/\" \"[Google Mail]\""},
        { "* LIST (\\All \\HasNoChildren) \"/\" \"[Google Mail]/All Mail\""},
        { "* LIST (\\Drafts \\HasNoChildren) \"/\" \"[Google Mail]/Drafts\""},
        { "* LIST (\\HasNoChildren \\Important) \"/\" \"[Google Mail]/Important\""},
        { "* LIST (\\HasNoChildren \\Sent) \"/\" \"[Google Mail]/Sent Mail\""},
        { "* LIST (\\HasNoChildren \\Junk) \"/\" \"[Google Mail]/Spam\""},
        { "* LIST (\\Flagged \\HasNoChildren) \"/\" \"[Google Mail]/Starred\""},
        { "* LIST (\\HasNoChildren \\Trash) \"/\" \"[Google Mail]/Trash\""},
        { "A000002 OK Success"}
    };

    std::string commandResponseStr;
 
    for (auto str : listResponseStr) {
        commandResponseStr.append( str + CMailIMAP::kEOLStr);
    }
    
    CMailIMAPParse::BASERESPONSE parsedResponse(CMailIMAPParse::parseResponse(commandResponseStr));
    CMailIMAPParse::ListResponse *ptr = static_cast<CMailIMAPParse::ListResponse *> (parsedResponse.get());
    
    EXPECT_TRUE(ptr->status==CMailIMAPParse::RespCode::OK);

}

TEST_F(CMailIMAPParseTests, SEARCHValid) {

    std::vector<std::string> searchResponseStr = {
        { "A000002 SEARCH 1:*"},
        { "* SEARCH 1 2 3 4 5 6 7 8 9 10"},
        { "A000002 OK SEARCH completed (Success)" }
    };
    
    std::string commandResponseStr;
 
    for (auto str : searchResponseStr) {
        commandResponseStr.append( str + CMailIMAP::kEOLStr);
    }
    
    CMailIMAPParse::BASERESPONSE parsedResponse(CMailIMAPParse::parseResponse(commandResponseStr));
    CMailIMAPParse::SearchResponse *ptr = static_cast<CMailIMAPParse::SearchResponse *> (parsedResponse.get());
    
    EXPECT_TRUE(ptr->status==CMailIMAPParse::RespCode::OK);

    EXPECT_EQ(1, ptr->indexes[0]);
    EXPECT_EQ(2, ptr->indexes[1]);
    EXPECT_EQ(3, ptr->indexes[2]);
    EXPECT_EQ(4, ptr->indexes[3]);
    EXPECT_EQ(5, ptr->indexes[4]);
    EXPECT_EQ(6, ptr->indexes[5]);
    EXPECT_EQ(7, ptr->indexes[6]);
    EXPECT_EQ(8, ptr->indexes[7]);
    EXPECT_EQ(9, ptr->indexes[8]);
    EXPECT_EQ(10, ptr->indexes[9]);

}

TEST_F(CMailIMAPParseTests, UIDSEARCHValid) {

    std::vector<std::string> searchResponseStr = {
        { "A000002 UID SEARCH 1:*"},
        { "* SEARCH 998 999 1000 1003 1009 1010 1011 1012 1013 1014"},
        { "A000002 OK SEARCH completed (Success)" }
    };
    
    std::string commandResponseStr;
 
    for (auto str : searchResponseStr) {
        commandResponseStr.append( str + CMailIMAP::kEOLStr);
    }
    
    CMailIMAPParse::BASERESPONSE parsedResponse(CMailIMAPParse::parseResponse(commandResponseStr));
    CMailIMAPParse::SearchResponse *ptr = static_cast<CMailIMAPParse::SearchResponse *> (parsedResponse.get());
    
    EXPECT_TRUE(ptr->status==CMailIMAPParse::RespCode::OK);
    
    EXPECT_EQ(998, ptr->indexes[0]);
    EXPECT_EQ(999, ptr->indexes[1]);
    EXPECT_EQ(1000, ptr->indexes[2]);
    EXPECT_EQ(1003, ptr->indexes[3]);
    EXPECT_EQ(1009, ptr->indexes[4]);
    EXPECT_EQ(1010, ptr->indexes[5]);
    EXPECT_EQ(1011, ptr->indexes[6]);
    EXPECT_EQ(1012, ptr->indexes[7]);
    EXPECT_EQ(1013, ptr->indexes[8]);
    EXPECT_EQ(1014, ptr->indexes[9]);


}

TEST_F(CMailIMAPParseTests, LSUBValid) {

    std::vector<std::string> LSubResponseStr = { 
        { "A000002 LSUB \"\" *"},
        { "* LSUB (\\HasNoChildren) \"/\" \"DDNS\""},
        { "* LSUB (\\HasNoChildren) \"/\" \"EDO\""},
        { "* LSUB (\\HasNoChildren) \"/\" \"INBOX\""},
        { "* LSUB (\\HasNoChildren) \"/\" \"Microsoft\""},
        { "* LSUB (\\HasNoChildren) \"/\" \"Personal\""},
        { "* LSUB (\\HasNoChildren) \"/\" \"Receipts\""},
        { "* LSUB (\\HasNoChildren) \"/\" \"Sent\""},
        { "* LSUB (\\HasNoChildren) \"/\" \"Trash\""},
        { "* LSUB (\\HasNoChildren) \"/\" \"Travel\""},
        { "* LSUB (\\HasNoChildren) \"/\" \"Work\""},
        { "* LSUB (\\HasChildren \\Noselect) \"/\" \"[Google Mail]\""},
        { "* LSUB (\\All \\HasNoChildren) \"/\" \"[Google Mail]/All Mail\""},
        { "* LSUB (\\Drafts \\HasNoChildren) \"/\" \"[Google Mail]/Drafts\""},
        { "* LSUB (\\HasNoChildren \\Important) \"/\" \"[Google Mail]/Important\""},
        { "* LSUB (\\HasNoChildren \\Sent) \"/\" \"[Google Mail]/Sent Mail\""},
        { "* LSUB (\\HasNoChildren \\Junk) \"/\" \"[Google Mail]/Spam\""},
        { "* LSUB (\\Flagged \\HasNoChildren) \"/\" \"[Google Mail]/Starred\""},
        { "* LSUB (\\HasNoChildren \\Trash) \"/\" \"[Google Mail]/Trash\""},
        { "A000002 OK Success"}
    };

    std::string commandResponseStr;
 
    for (auto str : LSubResponseStr) {
        commandResponseStr.append( str + CMailIMAP::kEOLStr);
    }
    
    CMailIMAPParse::BASERESPONSE parsedResponse(CMailIMAPParse::parseResponse(commandResponseStr));
    CMailIMAPParse::LSubResponse *ptr = static_cast<CMailIMAPParse::LSubResponse *> (parsedResponse.get());
    
    EXPECT_TRUE(ptr->status==CMailIMAPParse::RespCode::OK);

}

TEST_F(CMailIMAPParseTests, EXPUNGEValid) {

    std::vector<std::string> ExpungeResponseStr = { 
        { "A000002 EXPUNGE" },
        { "A000002 OK Success" }
   
    };

    std::string commandResponseStr;
 
    for (auto str : ExpungeResponseStr) {
        commandResponseStr.append( str + CMailIMAP::kEOLStr);
    }
    
    CMailIMAPParse::BASERESPONSE parsedResponse(CMailIMAPParse::parseResponse(commandResponseStr));
    CMailIMAPParse::ExpungeResponse *ptr = static_cast<CMailIMAPParse::ExpungeResponse *> (parsedResponse.get());
    
    EXPECT_TRUE(ptr->status==CMailIMAPParse::RespCode::OK);

}

TEST_F(CMailIMAPParseTests, STOREValid) {

    std::vector<std::string> StoreResponseStr = { 
       { "A000008 STORE 1:* +FLAGS (\\Deleted)"},
       { "* 1 FETCH (FLAGS (\\Seen \\Deleted))"},
       { "* 2 FETCH (FLAGS (\\Seen \\Deleted))"},
       { "* 3 FETCH (FLAGS (\\Seen \\Deleted))"},
       { "* 4 FETCH (FLAGS (\\Seen \\Deleted))"},
       { "* 5 FETCH (FLAGS (\\Deleted))"},
       { "* 6 FETCH (FLAGS (\\Deleted))"},
       { "* 7 FETCH (FLAGS (\\Deleted))"},
       { "* 8 FETCH (FLAGS (\\Deleted))"},
       { "* 9 FETCH (FLAGS (\\Deleted))"},
       { "* 10 FETCH (FLAGS (\\Deleted))"},
       { "A000008 OK Success"}  
    };

    std::string commandResponseStr;
 
    for (auto str : StoreResponseStr) {
        commandResponseStr.append( str + CMailIMAP::kEOLStr);
    }
    
    CMailIMAPParse::BASERESPONSE parsedResponse(CMailIMAPParse::parseResponse(commandResponseStr));
    CMailIMAPParse::StoreResponse *ptr = static_cast<CMailIMAPParse::StoreResponse *> (parsedResponse.get());
    
    EXPECT_TRUE(ptr->status==CMailIMAPParse::RespCode::OK);

    EXPECT_EQ(1, ptr->storeList[0].index);
    EXPECT_EQ(2, ptr->storeList[1].index);
    EXPECT_EQ(3, ptr->storeList[2].index);
    EXPECT_EQ(4, ptr->storeList[3].index);
    EXPECT_EQ(5, ptr->storeList[4].index);
    EXPECT_EQ(6, ptr->storeList[5].index);
    EXPECT_EQ(7, ptr->storeList[6].index);
    EXPECT_EQ(8, ptr->storeList[7].index);
    EXPECT_EQ(9, ptr->storeList[8].index);
    EXPECT_EQ(10, ptr->storeList[9].index);

    ASSERT_STREQ("(\\Seen \\Deleted)", ptr->storeList[0].flagsListStr.c_str());
    ASSERT_STREQ("(\\Seen \\Deleted)", ptr->storeList[1].flagsListStr.c_str());
    ASSERT_STREQ("(\\Seen \\Deleted)", ptr->storeList[2].flagsListStr.c_str());
    ASSERT_STREQ("(\\Seen \\Deleted)", ptr->storeList[3].flagsListStr.c_str());
    ASSERT_STREQ("(\\Deleted)", ptr->storeList[4].flagsListStr.c_str());
    ASSERT_STREQ("(\\Deleted)", ptr->storeList[5].flagsListStr.c_str());
    ASSERT_STREQ("(\\Deleted)", ptr->storeList[6].flagsListStr.c_str());
    ASSERT_STREQ("(\\Deleted)", ptr->storeList[7].flagsListStr.c_str());
    ASSERT_STREQ("(\\Deleted)", ptr->storeList[8].flagsListStr.c_str());
    ASSERT_STREQ("(\\Deleted)", ptr->storeList[9].flagsListStr.c_str());


}

TEST_F(CMailIMAPParseTests, CAPABILITYValid) {

    std::vector<std::string> capabilityResponseStr = { 
       { "A000002 CAPABILITY"},
       { "* CAPABILITY IMAP4rev1 UNSELECT IDLE NAMESPACE QUOTA ID XLIST CHILDREN X-GM-EXT-1 "
         "UIDPLUS COMPRESS=DEFLATE ENABLE MOVE CONDSTORE ESEARCH UTF8=ACCEPT LIST-EXTENDED "
         "LIST-STATUS LITERAL- APPENDLIMIT=35651584 SPECIAL-USE" },
       { "A000002 OK Success"}
    };

    std::string commandResponseStr;
 
    for (auto str : capabilityResponseStr) {
        commandResponseStr.append( str + CMailIMAP::kEOLStr);
    }
    
    CMailIMAPParse::BASERESPONSE parsedResponse(CMailIMAPParse::parseResponse(commandResponseStr));
    CMailIMAPParse::CapabilityResponse *ptr = static_cast<CMailIMAPParse::CapabilityResponse *> (parsedResponse.get());
    
    EXPECT_TRUE(ptr->status==CMailIMAPParse::RespCode::OK);
    ASSERT_STREQ("IMAP4rev1 UNSELECT IDLE NAMESPACE QUOTA ID XLIST CHILDREN X-GM-EXT-1 "
         "UIDPLUS COMPRESS=DEFLATE ENABLE MOVE CONDSTORE ESEARCH UTF8=ACCEPT LIST-EXTENDED "
         "LIST-STATUS LITERAL- APPENDLIMIT=35651584 SPECIAL-USE", ptr->capabilitiesStr.c_str());

}

TEST_F(CMailIMAPParseTests, NOOPValid) {

    std::vector<std::string> noOpResponseStr = { 
       { "A000002 NOOP" },
       { "* 8 EXISTS" },
       { "A000002 OK Success" }
    };

    std::string commandResponseStr;
 
    for (auto str : noOpResponseStr) {
        commandResponseStr.append( str + CMailIMAP::kEOLStr);
    }
    
    CMailIMAPParse::BASERESPONSE parsedResponse(CMailIMAPParse::parseResponse(commandResponseStr));
    CMailIMAPParse::NoOpResponse *ptr = static_cast<CMailIMAPParse::NoOpResponse *> (parsedResponse.get());
    
    EXPECT_TRUE(ptr->status==CMailIMAPParse::RespCode::OK);
    
    EXPECT_EQ(1, ptr->rawResponse.size());

    if (ptr->rawResponse.size() == 1) {
        ASSERT_STREQ("* 8 EXISTS",ptr->rawResponse[0].c_str());
    }
}
TEST_F(CMailIMAPParseTests, IDLEValid) {

    std::vector<std::string> idleResponseStr = { 
       { "A000002 IDLE" },
       { "* 1 EXISTS" },
       { "A000002 OK IDLE terminated (Success)" }
    };

    std::string commandResponseStr;
 
    for (auto str : idleResponseStr) {
        commandResponseStr.append( str + CMailIMAP::kEOLStr);
    }
    
    CMailIMAPParse::BASERESPONSE parsedResponse(CMailIMAPParse::parseResponse(commandResponseStr));
    CMailIMAPParse::IdleResponse *ptr = static_cast<CMailIMAPParse::IdleResponse *> (parsedResponse.get());
    
    EXPECT_TRUE(ptr->status==CMailIMAPParse::RespCode::OK);

    EXPECT_EQ(1, ptr->rawResponse.size());

    if (ptr->rawResponse.size() == 1) {
        ASSERT_STREQ("* 1 EXISTS", ptr->rawResponse[0].c_str());
    }
    
}

TEST_F(CMailIMAPParseTests, LOGOUTValid) {

    std::vector<std::string> logOutResponseStr = { 
       { "A000003 LOGOUT" },
       { "* BYE LOGOUT Requested" },
       { "A000003 OK 73 good day (Success)" }
    };

    std::string commandResponseStr;
 
    for (auto str : logOutResponseStr) {
        commandResponseStr.append( str + CMailIMAP::kEOLStr);
    }

    CMailIMAPParse::BASERESPONSE parsedResponse(CMailIMAPParse::parseResponse(commandResponseStr));
    CMailIMAPParse::LogOutResponse *ptr = static_cast<CMailIMAPParse::LogOutResponse *> (parsedResponse.get());

    EXPECT_TRUE(ptr->status == CMailIMAPParse::RespCode::OK);

    EXPECT_EQ(1, ptr->rawResponse.size());

    if (ptr->rawResponse.size() == 1) {
        ASSERT_STREQ("* BYE LOGOUT Requested", ptr->rawResponse[0].c_str());
    }
    
}

TEST_F(CMailIMAPParseTests, FETCHValid) {

    std::vector<std::string> fetchResponseStr = { 
       { "A000004 FETCH 1 (BODYSTRUCTURE FLAGS UID)" },
       { "* 1 FETCH (UID 1015 FLAGS () BODYSTRUCTURE ((\"TEXT\" \"PLAIN\" (\"CHARSET\" \"iso-8859-1\") NIL NIL \"QUOTED-PRINTABLE\" 355 20 NIL NIL NIL)(\"TEXT\" \"HTML\" (\"CHARSET\" \"iso-8859-1\") NIL NIL \"QUOTED-PRINTABLE\" 1667 54 NIL NIL NIL) \"ALTERNATIVE\" (\"BOUNDARY\" \"_000_DB4PR08MB0174985090CE13C6BC7D7237E6510DB4PR08MB0174eurp_\") NIL NIL))"},
       { "A000004 OK Success" }
    };

    std::string commandResponseStr;
 
    for (auto str : fetchResponseStr) {
        commandResponseStr.append( str + CMailIMAP::kEOLStr);
    }
    
    CMailIMAPParse::BASERESPONSE parsedResponse(CMailIMAPParse::parseResponse(commandResponseStr));
    CMailIMAPParse::FetchResponse *ptr = static_cast<CMailIMAPParse::FetchResponse *> (parsedResponse.get());
    
    EXPECT_TRUE(ptr->status==CMailIMAPParse::RespCode::OK);

    EXPECT_EQ(1, ptr->fetchList.size());

    if (ptr->fetchList.size() == 1) {
        EXPECT_EQ(1, ptr->fetchList[0].index);
        EXPECT_TRUE(ptr->fetchList[0].responseMap.find("UID") != ptr->fetchList[0].responseMap.end());
        EXPECT_TRUE(ptr->fetchList[0].responseMap.find("FLAGS") != ptr->fetchList[0].responseMap.end());
        EXPECT_TRUE(ptr->fetchList[0].responseMap.find("BODYSTRUCTURE") != ptr->fetchList[0].responseMap.end());

        ASSERT_STREQ("1015", ptr->fetchList[0].responseMap["UID"].c_str());
        ASSERT_STREQ("()", ptr->fetchList[0].responseMap["FLAGS"].c_str());
        ASSERT_STREQ("((\"TEXT\" \"PLAIN\" (\"CHARSET\" \"iso-8859-1\") NIL NIL \"QUOTED-PRINTABLE\" 355 20 NIL NIL NIL)(\"TEXT\" \"HTML\" (\"CHARSET\" \"iso-8859-1\") NIL NIL \"QUOTED-PRINTABLE\" 1667 54 NIL NIL NIL) \"ALTERNATIVE\" (\"BOUNDARY\" \"_000_DB4PR08MB0174985090CE13C6BC7D7237E6510DB4PR08MB0174eurp_\") NIL NIL)", ptr->fetchList[0].responseMap["BODYSTRUCTURE"].c_str());
    }
    
}

// =====================
// RUN GOOGLE UNIT TESTS
// =====================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}