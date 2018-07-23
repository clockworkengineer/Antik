#include "HOST.hpp"
/*
 * File:   TCIMAPParse.cpp
 * 
 * Author: Robert Tizzard
 *
 * Created on October 24, 2016, 2:34 PM
 * 
 * Description: Google unit tests for class CIMAPParse.
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

// CIMAP/CIMAPParse class

#include "CIMAP.hpp" 
#include "CIMAPParse.hpp"

using namespace Antik::IMAP;

// =======================
// UNIT TEST FIXTURE CLASS
// =======================

class TCIMAPParse : public ::testing::Test {
protected:

    // Empty constructor

    TCIMAPParse() {
    }

    // Empty destructor

    ~TCIMAPParse() override {
    }

    void SetUp() override {
    }

    void TearDown() override {
    }
    
    static void checkListRespData(CIMAPParse::ListRespData &respData, std::uint8_t hierDel, const std::string &attributesStr, const std::string &mailBoxNameStr);


};

// =================
// FIXTURE CONSTANTS
// =================

// ===============
// FIXTURE METHODS
// ===============

void TCIMAPParse::checkListRespData(CIMAPParse::ListRespData &respData, std::uint8_t hierDel, const std::string &attributesStr, const std::string &mailBoxNameStr) {

    EXPECT_EQ(hierDel, respData.hierDel);
    ASSERT_STREQ(attributesStr.c_str(), respData.attributes.c_str());
    ASSERT_STREQ(mailBoxNameStr.c_str(), respData.mailBoxName.c_str());

}

// =====================
// TASK CLASS UNIT TESTS
// =====================

TEST_F(TCIMAPParse, SELECTValid) {

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
        commandResponseStr.append( str + kEOL);
    }
    
    CIMAPParse::COMMANDRESPONSE parsedResponse(CIMAPParse::parseResponse(commandResponseStr));
  
    EXPECT_TRUE(parsedResponse->status==CIMAPParse::RespCode::OK);
    EXPECT_EQ(8, parsedResponse->responseMap.size());
    EXPECT_TRUE(parsedResponse->responseMap.find("EXISTS")!=parsedResponse->responseMap.end());
    EXPECT_TRUE(parsedResponse->responseMap.find("RECENT")!=parsedResponse->responseMap.end());
    EXPECT_TRUE(parsedResponse->responseMap.find("FLAGS")!=parsedResponse->responseMap.end());
    EXPECT_TRUE(parsedResponse->responseMap.find("PERMANENTFLAGS")!=parsedResponse->responseMap.end());
    EXPECT_TRUE(parsedResponse->responseMap.find("UIDVALIDITY")!=parsedResponse->responseMap.end());
    EXPECT_TRUE(parsedResponse->responseMap.find("UIDNEXT")!=parsedResponse->responseMap.end());
    EXPECT_TRUE(parsedResponse->responseMap.find("MAILBOX-NAME")!=parsedResponse->responseMap.end());
    EXPECT_TRUE(parsedResponse->responseMap.find("MAILBOX-ACCESS")!=parsedResponse->responseMap.end());

    ASSERT_STREQ("INBOX", CIMAPParse::stringToUpper(parsedResponse->responseMap["MAILBOX-NAME"]).c_str());
    ASSERT_STREQ( "READ-WRITE", parsedResponse->responseMap["MAILBOX-ACCESS"].c_str());
    ASSERT_STREQ("1", parsedResponse->responseMap["EXISTS"].c_str());
    ASSERT_STREQ("0", parsedResponse->responseMap["RECENT"].c_str());    
    ASSERT_STREQ("(\\Seen \\Answered \\Flagged \\Deleted \\Draft $MDNSent)", parsedResponse->responseMap["FLAGS"].c_str());
    ASSERT_STREQ( "(\\Seen \\Answered \\Flagged \\Deleted \\Draft $MDNSent)", parsedResponse->responseMap["PERMANENTFLAGS"].c_str());
    ASSERT_STREQ("14", parsedResponse->responseMap["UIDVALIDITY"].c_str());    
    ASSERT_STREQ("4554", parsedResponse->responseMap["UIDNEXT"].c_str());    

    EXPECT_FALSE(parsedResponse->byeSent);
    
}

TEST_F(TCIMAPParse, SELECTInvalidMailBox) {

    std::vector<std::string> selectResponseStr = {
        { "A000002 SELECT NOTHERE"},
        { "A000002 NO NOTHERE doesn't exist."}
    };
    
    std::string commandResponseStr;
 
    for (auto str : selectResponseStr) {
        commandResponseStr.append( str + kEOL);
    }
    
    CIMAPParse::COMMANDRESPONSE parsedResponse(CIMAPParse::parseResponse(commandResponseStr));
    
    EXPECT_TRUE(parsedResponse->status==CIMAPParse::RespCode::NO);
    ASSERT_STREQ("A000002 NO NOTHERE doesn't exist.", parsedResponse->errorMessage.c_str());

    EXPECT_FALSE(parsedResponse->byeSent);
    
}

TEST_F(TCIMAPParse, EXAMINEValid) {

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
        commandResponseStr.append( str + kEOL);
    }
    
    CIMAPParse::COMMANDRESPONSE parsedResponse(CIMAPParse::parseResponse(commandResponseStr));
       
    EXPECT_TRUE(parsedResponse->status==CIMAPParse::RespCode::OK);
    EXPECT_EQ(9, parsedResponse->responseMap.size());
    EXPECT_TRUE(parsedResponse->responseMap.find("EXISTS")!=parsedResponse->responseMap.end());
    EXPECT_TRUE(parsedResponse->responseMap.find("RECENT")!=parsedResponse->responseMap.end());
    EXPECT_TRUE(parsedResponse->responseMap.find("FLAGS")!=parsedResponse->responseMap.end());
    EXPECT_TRUE(parsedResponse->responseMap.find("PERMANENTFLAGS")!=parsedResponse->responseMap.end());
    EXPECT_TRUE(parsedResponse->responseMap.find("UNSEEN")!=parsedResponse->responseMap.end());
    EXPECT_TRUE(parsedResponse->responseMap.find("UIDVALIDITY")!=parsedResponse->responseMap.end());
    EXPECT_TRUE(parsedResponse->responseMap.find("UIDNEXT")!=parsedResponse->responseMap.end());
    EXPECT_TRUE(parsedResponse->responseMap.find("MAILBOX-NAME")!=parsedResponse->responseMap.end());
    EXPECT_TRUE(parsedResponse->responseMap.find("MAILBOX-ACCESS")!=parsedResponse->responseMap.end());

    ASSERT_STREQ("INBOX", CIMAPParse::stringToUpper(parsedResponse->responseMap["MAILBOX-NAME"]).c_str());
    ASSERT_STREQ( "READ-ONLY", parsedResponse->responseMap["MAILBOX-ACCESS"].c_str());
    ASSERT_STREQ("11", parsedResponse->responseMap["EXISTS"].c_str());
    ASSERT_STREQ("0", parsedResponse->responseMap["RECENT"].c_str());    
    ASSERT_STREQ("(\\Seen \\Answered \\Flagged \\Deleted \\Draft $MDNSent)", parsedResponse->responseMap["FLAGS"].c_str());
    ASSERT_STREQ( "()", parsedResponse->responseMap["PERMANENTFLAGS"].c_str());
    ASSERT_STREQ("1", parsedResponse->responseMap["UNSEEN"].c_str());
    ASSERT_STREQ("18", parsedResponse->responseMap["UIDVALIDITY"].c_str());    
    ASSERT_STREQ("4584", parsedResponse->responseMap["UIDNEXT"].c_str());    

    EXPECT_FALSE(parsedResponse->byeSent);
       
}

TEST_F(TCIMAPParse, EXAMINEInvalidMailBox) {

    std::vector<std::string> examineResponseStr = {
        { "A000002 EXAMINE NOTTHERE"},
        { "A000002 NO NOTHERE doesn't exist."}
    };
    
    std::string commandResponseStr;
 
    for (auto str : examineResponseStr) {
        commandResponseStr.append( str + kEOL);
    }
    
    CIMAPParse::COMMANDRESPONSE parsedResponse(CIMAPParse::parseResponse(commandResponseStr));
    
    EXPECT_TRUE(parsedResponse->status==CIMAPParse::RespCode::NO);
    ASSERT_STREQ("A000002 NO NOTHERE doesn't exist.", parsedResponse->errorMessage.c_str());

    EXPECT_FALSE(parsedResponse->byeSent);
       
}

TEST_F(TCIMAPParse, STATUSValid) {

    std::vector<std::string> statusResponseStr = {
       { "A000003 STATUS INBOX (UIDNEXT MESSAGES RECENT UIDVALIDITY UNSEEN)" },
       { "* STATUS Inbox (UIDNEXT 4584 MESSAGES 11 RECENT 0 UIDVALIDITY 14 UNSEEN 2)" }, 
       { "A000003 OK STATUS completed."}
    };
    
    std::string commandResponseStr;
 
    for (auto str : statusResponseStr) {
        commandResponseStr.append( str + kEOL);
    }
    
    CIMAPParse::COMMANDRESPONSE parsedResponse(CIMAPParse::parseResponse(commandResponseStr));

    EXPECT_TRUE(parsedResponse->status == CIMAPParse::RespCode::OK);
    EXPECT_EQ(6, parsedResponse->responseMap.size());
    EXPECT_TRUE(parsedResponse->responseMap.find("UIDNEXT") != parsedResponse->responseMap.end());
    EXPECT_TRUE(parsedResponse->responseMap.find("MESSAGES") != parsedResponse->responseMap.end());
    EXPECT_TRUE(parsedResponse->responseMap.find("RECENT") != parsedResponse->responseMap.end());
    EXPECT_TRUE(parsedResponse->responseMap.find("UIDVALIDITY") != parsedResponse->responseMap.end());
    EXPECT_TRUE(parsedResponse->responseMap.find("UNSEEN") != parsedResponse->responseMap.end());
    EXPECT_TRUE(parsedResponse->responseMap.find("MAILBOX-NAME") != parsedResponse->responseMap.end());

    ASSERT_STREQ("INBOX", CIMAPParse::stringToUpper(parsedResponse->responseMap["MAILBOX-NAME"]).c_str());
    ASSERT_STREQ("4584", parsedResponse->responseMap["UIDNEXT"].c_str());
    ASSERT_STREQ("11", parsedResponse->responseMap["MESSAGES"].c_str());
    ASSERT_STREQ("0", parsedResponse->responseMap["RECENT"].c_str());
    ASSERT_STREQ("14", parsedResponse->responseMap["UIDVALIDITY"].c_str());
    ASSERT_STREQ("2", parsedResponse->responseMap["UNSEEN"].c_str());

    EXPECT_FALSE(parsedResponse->byeSent);

}

TEST_F(TCIMAPParse, STATUSInvalidMailBox) {

    std::vector<std::string> statusResponseStr = {
        { "A000002 STATUS NOTTHERE (UIDNEXT MESSAGES RECENT UIDVALIDITY UNSEEN)"},
        { "A000002 NO NOTHERE doesn't exist."}
    };
    
    std::string commandResponseStr;
 
    for (auto str : statusResponseStr) {
        commandResponseStr.append( str + kEOL);
    }
    
    CIMAPParse::COMMANDRESPONSE parsedResponse(CIMAPParse::parseResponse(commandResponseStr));
    
    EXPECT_TRUE(parsedResponse->status==CIMAPParse::RespCode::NO);
    ASSERT_STREQ("A000002 NO NOTHERE doesn't exist.", parsedResponse->errorMessage.c_str());

    EXPECT_FALSE(parsedResponse->byeSent);
       
}

TEST_F(TCIMAPParse, LISTValid) {

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
        commandResponseStr.append( str + kEOL);
    }
    
    CIMAPParse::COMMANDRESPONSE parsedResponse(CIMAPParse::parseResponse(commandResponseStr));
    
    EXPECT_TRUE(parsedResponse->status==CIMAPParse::RespCode::OK);
    EXPECT_EQ(18, parsedResponse->mailBoxList.size());

    if (parsedResponse->mailBoxList.size() == 18) {
        checkListRespData(parsedResponse->mailBoxList[0], '/', "(\\HasNoChildren)", "\"DDNS\"");
        checkListRespData(parsedResponse->mailBoxList[1], '/', "(\\HasNoChildren)", "\"EDO\"");
        checkListRespData(parsedResponse->mailBoxList[2], '/', "(\\HasNoChildren)", "\"INBOX\"");
        checkListRespData(parsedResponse->mailBoxList[3], '/', "(\\HasNoChildren)", "\"Microsoft\"");
        checkListRespData(parsedResponse->mailBoxList[4], '/', "(\\HasNoChildren)", "\"Personal\"");
        checkListRespData(parsedResponse->mailBoxList[5], '/', "(\\HasNoChildren)", "\"Receipts\"");
        checkListRespData(parsedResponse->mailBoxList[6], '/', "(\\HasNoChildren)", "\"Sent\"");
        checkListRespData(parsedResponse->mailBoxList[7], '/', "(\\HasNoChildren)", "\"Trash\"");
        checkListRespData(parsedResponse->mailBoxList[8], '/', "(\\HasNoChildren)", "\"Travel\"");
        checkListRespData(parsedResponse->mailBoxList[9], '/', "(\\HasNoChildren)", "\"Work\"");
        checkListRespData(parsedResponse->mailBoxList[10], '/', "(\\HasChildren \\Noselect)", "\"[Google Mail]\"");
        checkListRespData(parsedResponse->mailBoxList[11], '/', "(\\All \\HasNoChildren)", "\"[Google Mail]/All Mail\"");
        checkListRespData(parsedResponse->mailBoxList[12], '/', "(\\Drafts \\HasNoChildren)", "\"[Google Mail]/Drafts\"");
        checkListRespData(parsedResponse->mailBoxList[13], '/', "(\\HasNoChildren \\Important)", "\"[Google Mail]/Important\"");
        checkListRespData(parsedResponse->mailBoxList[14], '/', "(\\HasNoChildren \\Sent)", "\"[Google Mail]/Sent Mail\"");
        checkListRespData(parsedResponse->mailBoxList[15], '/', "(\\HasNoChildren \\Junk)", "\"[Google Mail]/Spam\"");
        checkListRespData(parsedResponse->mailBoxList[16], '/', "(\\Flagged \\HasNoChildren)", "\"[Google Mail]/Starred\"");
        checkListRespData(parsedResponse->mailBoxList[17], '/', "(\\HasNoChildren \\Trash)", "\"[Google Mail]/Trash\"");
    }
    
    EXPECT_FALSE(parsedResponse->byeSent);
       
}

TEST_F(TCIMAPParse, SEARCHValid) {

    std::vector<std::string> searchResponseStr = {
        { "A000002 SEARCH 1:*"},
        { "* SEARCH 1 2 3 4 5 6 7 8 9 10"},
        { "A000002 OK SEARCH completed (Success)" }
    };
    
    std::string commandResponseStr;
 
    for (auto str : searchResponseStr) {
        commandResponseStr.append( str + kEOL);
    }
    
    CIMAPParse::COMMANDRESPONSE parsedResponse(CIMAPParse::parseResponse(commandResponseStr));
    
    EXPECT_TRUE(parsedResponse->status==CIMAPParse::RespCode::OK);
    EXPECT_EQ (10, parsedResponse->indexes.size());

    if (parsedResponse->indexes.size() == 10) {
        EXPECT_EQ(1, parsedResponse->indexes[0]);
        EXPECT_EQ(2, parsedResponse->indexes[1]);
        EXPECT_EQ(3, parsedResponse->indexes[2]);
        EXPECT_EQ(4, parsedResponse->indexes[3]);
        EXPECT_EQ(5, parsedResponse->indexes[4]);
        EXPECT_EQ(6, parsedResponse->indexes[5]);
        EXPECT_EQ(7, parsedResponse->indexes[6]);
        EXPECT_EQ(8, parsedResponse->indexes[7]);
        EXPECT_EQ(9, parsedResponse->indexes[8]);
        EXPECT_EQ(10, parsedResponse->indexes[9]);
    }
    
    EXPECT_FALSE(parsedResponse->byeSent);
       
}

TEST_F(TCIMAPParse, UIDSEARCHValid) {

    std::vector<std::string> searchResponseStr = {
        { "A000002 UID SEARCH 1:*"},
        { "* SEARCH 998 999 1000 1003 1009 1010 1011 1012 1013 1014"},
        { "A000002 OK SEARCH completed (Success)" }
    };
    
    std::string commandResponseStr;
 
    for (auto str : searchResponseStr) {
        commandResponseStr.append( str + kEOL);
    }
    
    CIMAPParse::COMMANDRESPONSE parsedResponse(CIMAPParse::parseResponse(commandResponseStr));

    EXPECT_TRUE(parsedResponse->status == CIMAPParse::RespCode::OK);
    EXPECT_EQ(10, parsedResponse->indexes.size());

    if (parsedResponse->indexes.size() == 10) {
        EXPECT_EQ(998, parsedResponse->indexes[0]);
        EXPECT_EQ(999, parsedResponse->indexes[1]);
        EXPECT_EQ(1000, parsedResponse->indexes[2]);
        EXPECT_EQ(1003, parsedResponse->indexes[3]);
        EXPECT_EQ(1009, parsedResponse->indexes[4]);
        EXPECT_EQ(1010, parsedResponse->indexes[5]);
        EXPECT_EQ(1011, parsedResponse->indexes[6]);
        EXPECT_EQ(1012, parsedResponse->indexes[7]);
        EXPECT_EQ(1013, parsedResponse->indexes[8]);
        EXPECT_EQ(1014, parsedResponse->indexes[9]);
    }

    EXPECT_FALSE(parsedResponse->byeSent);
       
}

TEST_F(TCIMAPParse, LSUBValid) {

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
        commandResponseStr.append( str + kEOL);
    }
    
    CIMAPParse::COMMANDRESPONSE parsedResponse(CIMAPParse::parseResponse(commandResponseStr));
    
    EXPECT_TRUE(parsedResponse->status==CIMAPParse::RespCode::OK);
    EXPECT_EQ(18, parsedResponse->mailBoxList.size());

    if (parsedResponse->mailBoxList.size() == 18) {
        checkListRespData(parsedResponse->mailBoxList[0], '/', "(\\HasNoChildren)", "\"DDNS\"");
        checkListRespData(parsedResponse->mailBoxList[1], '/', "(\\HasNoChildren)", "\"EDO\"");
        checkListRespData(parsedResponse->mailBoxList[2], '/', "(\\HasNoChildren)", "\"INBOX\"");
        checkListRespData(parsedResponse->mailBoxList[3], '/', "(\\HasNoChildren)", "\"Microsoft\"");
        checkListRespData(parsedResponse->mailBoxList[4], '/', "(\\HasNoChildren)", "\"Personal\"");
        checkListRespData(parsedResponse->mailBoxList[5], '/', "(\\HasNoChildren)", "\"Receipts\"");
        checkListRespData(parsedResponse->mailBoxList[6], '/', "(\\HasNoChildren)", "\"Sent\"");
        checkListRespData(parsedResponse->mailBoxList[7], '/', "(\\HasNoChildren)", "\"Trash\"");
        checkListRespData(parsedResponse->mailBoxList[8], '/', "(\\HasNoChildren)", "\"Travel\"");
        checkListRespData(parsedResponse->mailBoxList[9], '/', "(\\HasNoChildren)", "\"Work\"");
        checkListRespData(parsedResponse->mailBoxList[10], '/', "(\\HasChildren \\Noselect)", "\"[Google Mail]\"");
        checkListRespData(parsedResponse->mailBoxList[11], '/', "(\\All \\HasNoChildren)", "\"[Google Mail]/All Mail\"");
        checkListRespData(parsedResponse->mailBoxList[12], '/', "(\\Drafts \\HasNoChildren)", "\"[Google Mail]/Drafts\"");
        checkListRespData(parsedResponse->mailBoxList[13], '/', "(\\HasNoChildren \\Important)", "\"[Google Mail]/Important\"");
        checkListRespData(parsedResponse->mailBoxList[14], '/', "(\\HasNoChildren \\Sent)", "\"[Google Mail]/Sent Mail\"");
        checkListRespData(parsedResponse->mailBoxList[15], '/', "(\\HasNoChildren \\Junk)", "\"[Google Mail]/Spam\"");
        checkListRespData(parsedResponse->mailBoxList[16], '/', "(\\Flagged \\HasNoChildren)", "\"[Google Mail]/Starred\"");
        checkListRespData(parsedResponse->mailBoxList[17], '/', "(\\HasNoChildren \\Trash)", "\"[Google Mail]/Trash\"");
    }
    
    EXPECT_FALSE(parsedResponse->byeSent);
       
}

TEST_F(TCIMAPParse, EXPUNGEValid) {

    std::vector<std::string> ExpungeResponseStr = { 
        { "A000002 EXPUNGE" },
        { "* 3 EXPUNGE" },
        { "* 3 EXPUNGE" },
        { "* 3 EXPUNGE" },
        { "* 8 EXPUNGE" },
        { "A000002 OK EXPUNGE Success" }
   
    };

    std::string commandResponseStr;
 
    for (auto str : ExpungeResponseStr) {
        commandResponseStr.append( str + kEOL);
    }
    
    CIMAPParse::COMMANDRESPONSE parsedResponse(CIMAPParse::parseResponse(commandResponseStr));
    
    EXPECT_TRUE(parsedResponse->status==CIMAPParse::RespCode::OK);
    
    ASSERT_STREQ("3 3 3 8", parsedResponse->responseMap["EXPUNGE"].c_str());
            
    EXPECT_FALSE(parsedResponse->byeSent);

}

TEST_F(TCIMAPParse, STOREValid) {

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
        commandResponseStr.append( str + kEOL);
    }
    
    CIMAPParse::COMMANDRESPONSE parsedResponse(CIMAPParse::parseResponse(commandResponseStr));
    
    EXPECT_TRUE(parsedResponse->status==CIMAPParse::RespCode::OK);
    EXPECT_EQ(10, parsedResponse->storeList.size());
    
    if (parsedResponse->storeList.size() == 10) {

        EXPECT_EQ(1, parsedResponse->storeList[0].index);
        EXPECT_EQ(2, parsedResponse->storeList[1].index);
        EXPECT_EQ(3, parsedResponse->storeList[2].index);
        EXPECT_EQ(4, parsedResponse->storeList[3].index);
        EXPECT_EQ(5, parsedResponse->storeList[4].index);
        EXPECT_EQ(6, parsedResponse->storeList[5].index);
        EXPECT_EQ(7, parsedResponse->storeList[6].index);
        EXPECT_EQ(8, parsedResponse->storeList[7].index);
        EXPECT_EQ(9, parsedResponse->storeList[8].index);
        EXPECT_EQ(10, parsedResponse->storeList[9].index);

        ASSERT_STREQ("(\\Seen \\Deleted)", parsedResponse->storeList[0].flagsList.c_str());
        ASSERT_STREQ("(\\Seen \\Deleted)", parsedResponse->storeList[1].flagsList.c_str());
        ASSERT_STREQ("(\\Seen \\Deleted)", parsedResponse->storeList[2].flagsList.c_str());
        ASSERT_STREQ("(\\Seen \\Deleted)", parsedResponse->storeList[3].flagsList.c_str());
        ASSERT_STREQ("(\\Deleted)", parsedResponse->storeList[4].flagsList.c_str());
        ASSERT_STREQ("(\\Deleted)", parsedResponse->storeList[5].flagsList.c_str());
        ASSERT_STREQ("(\\Deleted)", parsedResponse->storeList[6].flagsList.c_str());
        ASSERT_STREQ("(\\Deleted)", parsedResponse->storeList[7].flagsList.c_str());
        ASSERT_STREQ("(\\Deleted)", parsedResponse->storeList[8].flagsList.c_str());
        ASSERT_STREQ("(\\Deleted)", parsedResponse->storeList[9].flagsList.c_str());

    }

    EXPECT_FALSE(parsedResponse->byeSent);
       
}

TEST_F(TCIMAPParse, CAPABILITYValid) {

    std::vector<std::string> capabilityResponseStr = { 
       { "A000002 CAPABILITY"},
       { "* CAPABILITY IMAP4rev1 UNSELECT IDLE NAMESPACE QUOTA ID XLIST CHILDREN X-GM-EXT-1 "
         "UIDPLUS COMPRESS=DEFLATE ENABLE MOVE CONDSTORE ESEARCH UTF8=ACCEPT LIST-EXTENDED "
         "LIST-STATUS LITERAL- APPENDLIMIT=35651584 SPECIAL-USE" },
       { "A000002 OK Success"}
    };

    std::string commandResponseStr;
 
    for (auto str : capabilityResponseStr) {
        commandResponseStr.append( str + kEOL);
    }
    
    CIMAPParse::COMMANDRESPONSE parsedResponse(CIMAPParse::parseResponse(commandResponseStr));
    
    EXPECT_TRUE(parsedResponse->status==CIMAPParse::RespCode::OK);
    ASSERT_STREQ("IMAP4rev1 UNSELECT IDLE NAMESPACE QUOTA ID XLIST CHILDREN X-GM-EXT-1 "
         "UIDPLUS COMPRESS=DEFLATE ENABLE MOVE CONDSTORE ESEARCH UTF8=ACCEPT LIST-EXTENDED "
         "LIST-STATUS LITERAL- APPENDLIMIT=35651584 SPECIAL-USE", parsedResponse->responseMap["CAPABILITY"].c_str());
    
    EXPECT_FALSE(parsedResponse->byeSent);

}

TEST_F(TCIMAPParse, NOOPValid) {

    std::vector<std::string> noOpResponseStr = { 
       { "A000002 NOOP" },
       { "* 8 EXISTS" },
       { "A000002 OK Success" }
    };

    std::string commandResponseStr;
 
    for (auto str : noOpResponseStr) {
        commandResponseStr.append( str + kEOL);
    }
    
    CIMAPParse::COMMANDRESPONSE parsedResponse(CIMAPParse::parseResponse(commandResponseStr));
    
    EXPECT_TRUE(parsedResponse->status==CIMAPParse::RespCode::OK);
    EXPECT_EQ(1, parsedResponse->responseMap.size());
    EXPECT_TRUE(parsedResponse->responseMap.find("EXISTS") != parsedResponse->responseMap.end());
 
    if (parsedResponse->responseMap.size() == 1) {
        ASSERT_STREQ("8",parsedResponse->responseMap["EXISTS"].c_str());
    }
    
    EXPECT_FALSE(parsedResponse->byeSent);
    
}
TEST_F(TCIMAPParse, IDLEValid) {

    std::vector<std::string> idleResponseStr = { 
       { "A000002 IDLE" },
       { "* 1 EXISTS" },
       { "A000002 OK IDLE terminated (Success)" }
    };

    std::string commandResponseStr;
 
    for (auto str : idleResponseStr) {
        commandResponseStr.append( str + kEOL);
    }
    
    CIMAPParse::COMMANDRESPONSE parsedResponse(CIMAPParse::parseResponse(commandResponseStr));
    
    EXPECT_TRUE(parsedResponse->status==CIMAPParse::RespCode::OK);
    EXPECT_EQ(1, parsedResponse->responseMap.size());
    EXPECT_TRUE(parsedResponse->responseMap.find("EXISTS") != parsedResponse->responseMap.end());
 
    if (parsedResponse->responseMap.size() == 1) {
        ASSERT_STREQ("1",parsedResponse->responseMap["EXISTS"].c_str());
    }
   
    EXPECT_FALSE(parsedResponse->byeSent);
    
}

TEST_F(TCIMAPParse, LOGOUTValid) {

    std::vector<std::string> logOutResponseStr = { 
       { "A000003 LOGOUT" },
       { "* BYE LOGOUT Requested" },
       { "A000003 OK 73 good day (Success)" }
    };

    std::string commandResponseStr;
 
    for (auto str : logOutResponseStr) {
        commandResponseStr.append( str + kEOL);
    }

    CIMAPParse::COMMANDRESPONSE parsedResponse(CIMAPParse::parseResponse(commandResponseStr));

    EXPECT_TRUE(parsedResponse->status == CIMAPParse::RespCode::OK);
    
    EXPECT_TRUE(parsedResponse->byeSent);
    
}

TEST_F(TCIMAPParse, FETCHValid) {

    std::vector<std::string> fetchResponseStr = { 
       { "A000004 FETCH 1 (BODYSTRUCTURE FLAGS UID)" },
       { "* 1 FETCH (UID 1015 FLAGS () BODYSTRUCTURE ((\"TEXT\" \"PLAIN\" (\"CHARSET\" \"iso-8859-1\") NIL "
         "NIL \"QUOTED-PRINTABLE\" 355 20 NIL NIL NIL)(\"TEXT\" \"HTML\" (\"CHARSET\" \"iso-8859-1\") NIL "
         "NIL \"QUOTED-PRINTABLE\" 1667 54 NIL NIL NIL) \"ALTERNATIVE\" (\"BOUNDARY\" "
         "\"_000_DB4PR08MB0174985090CE13C6BC7D7237E6510DB4PR08MB0174eurp_\") NIL NIL))"},
       { "A000004 OK Success" }
    };

    std::string commandResponseStr;
 
    for (auto str : fetchResponseStr) {
        commandResponseStr.append( str + kEOL);
    }
    
    CIMAPParse::COMMANDRESPONSE parsedResponse(CIMAPParse::parseResponse(commandResponseStr));
    
    EXPECT_TRUE(parsedResponse->status==CIMAPParse::RespCode::OK);

    EXPECT_EQ(1, parsedResponse->fetchList.size());

    if (parsedResponse->fetchList.size() == 1) {
        EXPECT_EQ(1, parsedResponse->fetchList[0].index);
        EXPECT_EQ(3, parsedResponse->fetchList[0].responseMap.size());
        EXPECT_TRUE(parsedResponse->fetchList[0].responseMap.find("UID") != parsedResponse->fetchList[0].responseMap.end());
        EXPECT_TRUE(parsedResponse->fetchList[0].responseMap.find("FLAGS") != parsedResponse->fetchList[0].responseMap.end());
        EXPECT_TRUE(parsedResponse->fetchList[0].responseMap.find("BODYSTRUCTURE") != parsedResponse->fetchList[0].responseMap.end());

        ASSERT_STREQ("1015", parsedResponse->fetchList[0].responseMap["UID"].c_str());
        ASSERT_STREQ("()", parsedResponse->fetchList[0].responseMap["FLAGS"].c_str());
        ASSERT_STREQ("((\"TEXT\" \"PLAIN\" (\"CHARSET\" \"iso-8859-1\") NIL "
                    "NIL \"QUOTED-PRINTABLE\" 355 20 NIL NIL NIL)(\"TEXT\" \"HTML\" (\"CHARSET\" \"iso-8859-1\") NIL "
                    "NIL \"QUOTED-PRINTABLE\" 1667 54 NIL NIL NIL) \"ALTERNATIVE\" (\"BOUNDARY\" "
                    "\"_000_DB4PR08MB0174985090CE13C6BC7D7237E6510DB4PR08MB0174eurp_\") NIL NIL)", parsedResponse->fetchList[0].responseMap["BODYSTRUCTURE"].c_str());
    }
    
    EXPECT_FALSE(parsedResponse->byeSent);

}

TEST_F(TCIMAPParse, FETCHValidWithBYE) {

    std::vector<std::string> fetchResponseStr = { 
       { "A000004 FETCH 1 (BODYSTRUCTURE FLAGS UID)" },
       { "* 1 FETCH (UID 1015 FLAGS () BODYSTRUCTURE ((\"TEXT\" \"PLAIN\" (\"CHARSET\" \"iso-8859-1\") NIL "
         "NIL \"QUOTED-PRINTABLE\" 355 20 NIL NIL NIL)(\"TEXT\" \"HTML\" (\"CHARSET\" \"iso-8859-1\") NIL "
         "NIL \"QUOTED-PRINTABLE\" 1667 54 NIL NIL NIL) \"ALTERNATIVE\" (\"BOUNDARY\" "
         "\"_000_DB4PR08MB0174985090CE13C6BC7D7237E6510DB4PR08MB0174eurp_\") NIL NIL))"},
       { "* BYE Close down." },
       { "A000004 OK Success" }
    };

    std::string commandResponseStr;
 
    for (auto str : fetchResponseStr) {
        commandResponseStr.append( str + kEOL);
    }
    
    CIMAPParse::COMMANDRESPONSE parsedResponse(CIMAPParse::parseResponse(commandResponseStr));
    
    EXPECT_TRUE(parsedResponse->status==CIMAPParse::RespCode::OK);

    EXPECT_EQ(1, parsedResponse->fetchList.size());

    if (parsedResponse->fetchList.size() == 1) {
        EXPECT_EQ(1, parsedResponse->fetchList[0].index);
        EXPECT_EQ(3, parsedResponse->fetchList[0].responseMap.size());
        EXPECT_TRUE(parsedResponse->fetchList[0].responseMap.find("UID") != parsedResponse->fetchList[0].responseMap.end());
        EXPECT_TRUE(parsedResponse->fetchList[0].responseMap.find("FLAGS") != parsedResponse->fetchList[0].responseMap.end());
        EXPECT_TRUE(parsedResponse->fetchList[0].responseMap.find("BODYSTRUCTURE") != parsedResponse->fetchList[0].responseMap.end());

        ASSERT_STREQ("1015", parsedResponse->fetchList[0].responseMap["UID"].c_str());
        ASSERT_STREQ("()", parsedResponse->fetchList[0].responseMap["FLAGS"].c_str());
        ASSERT_STREQ("((\"TEXT\" \"PLAIN\" (\"CHARSET\" \"iso-8859-1\") NIL "
                    "NIL \"QUOTED-PRINTABLE\" 355 20 NIL NIL NIL)(\"TEXT\" \"HTML\" (\"CHARSET\" \"iso-8859-1\") NIL "
                    "NIL \"QUOTED-PRINTABLE\" 1667 54 NIL NIL NIL) \"ALTERNATIVE\" (\"BOUNDARY\" "
                    "\"_000_DB4PR08MB0174985090CE13C6BC7D7237E6510DB4PR08MB0174eurp_\") NIL NIL)", parsedResponse->fetchList[0].responseMap["BODYSTRUCTURE"].c_str());
    }
    
    EXPECT_TRUE(parsedResponse->byeSent);
    
}

// =====================
// RUN GOOGLE UNIT TESTS
// =====================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}