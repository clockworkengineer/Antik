#include "HOST.hpp"
/*
 * File:   CMailIMAPBodyStruct.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on January 24, 2017, 2:33 PM
 *
 * Copyright 2016.
 *
 */

//
// Class: CMailIMAPBodyStruct
// 
// Description:  Class to create a tree representation of an IMAP body structure
// and allow it to be traversed in-order and calling a user provided function
// for each body part to do things such as search and store attachment data.
//
// Dependencies:   C11++     - Language standard features used.
//

// =================
// CLASS DEFINITIONS
// =================

#include "CMailIMAP.hpp"
#include "CMailIMAPParse.hpp"
#include "CMailIMAPBodyStruct.hpp"

// ====================
// CLASS IMPLEMENTATION
// ====================

//
// C++ STL definitions
//

#include <iostream>
#include <sstream>

// =========
// NAMESPACE
// =========

namespace Antik {

    // ===========================
    // PRIVATE TYPES AND CONSTANTS
    // ===========================

    //
    // BODYSTRUCTURE constants
    //

    const char *CMailIMAPBodyStruct::kNILStr = "NIL";
    const char *CMailIMAPBodyStruct::kTEXTStr = "TEXT";
    const char *CMailIMAPBodyStruct::kATTACHMENTStr = "ATTACHMENT";
    const char *CMailIMAPBodyStruct::kINLINEStr = "INLINE";
    const char *CMailIMAPBodyStruct::kCREATIONDATEStr = "CREATION-DATE";
    const char *CMailIMAPBodyStruct::kFILENAMEStr = "FILENAME";
    const char *CMailIMAPBodyStruct::kMODIFICATIONDATEStr = "MODIFICATION-DATE";
    const char *CMailIMAPBodyStruct::kSIZEStr = "SIZE";


    // ==========================
    // PUBLIC TYPES AND CONSTANTS
    // ==========================

    // ========================
    // PRIVATE STATIC VARIABLES
    // ========================

    // =======================
    // PUBLIC STATIC VARIABLES
    // =======================

    // ===============
    // PRIVATE METHODS
    // ===============

    //
    // Parse and extract next element in IMAP body structure
    //

    void CMailIMAPBodyStruct::parseNext(std::string& partStr, std::string& valueStr) {

        if (partStr.empty()) {
            return;
        } else if (partStr[0] == '\"') {
            valueStr = CMailIMAPParse::stringBetween(partStr, '\"', '\"');
            partStr = partStr.substr(valueStr.length() + 3);
        } else if (partStr[0] == '(') {
            valueStr = CMailIMAPParse::stringList(partStr);
            partStr = partStr.substr(valueStr.length() + 1);
        } else if (std::isdigit(partStr[0])) {
            valueStr = partStr.substr(0, partStr.find_first_of(' '));
            partStr = partStr.substr(partStr.find_first_of(' ') + 1);
        } else if (CMailIMAPParse::stringEqual(partStr, kNILStr)) {
            valueStr = kNILStr;
            partStr = partStr.substr(valueStr.length() + 1);
        } else {
            throw CMailIMAPBodyStruct::Exception("error while parsing body structure [" + partStr + "]");
        }

    }

    //
    // Extract body details
    //

    void CMailIMAPBodyStruct::parseBodyPart(BodyPart& bodyPart) {

        std::string partStr{bodyPart.partStr.substr(1)};

        bodyPart.parsedPart.reset(new BodyPartParsed());

        parseNext(partStr, bodyPart.parsedPart->typeStr);
        parseNext(partStr, bodyPart.parsedPart->subtypeStr);
        parseNext(partStr, bodyPart.parsedPart->parameterListStr);
        parseNext(partStr, bodyPart.parsedPart->idStr);
        parseNext(partStr, bodyPart.parsedPart->descriptionStr);
        parseNext(partStr, bodyPart.parsedPart->encodingStr);
        parseNext(partStr, bodyPart.parsedPart->sizeStr);

        if (CMailIMAPParse::CMailIMAPParse::stringEqual(bodyPart.parsedPart->typeStr, kTEXTStr)) {
            parseNext(partStr, bodyPart.parsedPart->textLinesStr);
        }

        parseNext(partStr, bodyPart.parsedPart->md5Str);
        parseNext(partStr, bodyPart.parsedPart->dispositionStr);
        parseNext(partStr, bodyPart.parsedPart->languageStr);
        parseNext(partStr, bodyPart.parsedPart->locationStr);

    }

    //
    // Parse basic body structure filling in data
    //

    void CMailIMAPBodyStruct::parseBodyStructTree(std::unique_ptr<BodyNode>& bodyNode) {

        for (auto &bodyPart : bodyNode->bodyParts) {
            if (bodyPart.child) {
                parseBodyStructTree(bodyPart.child);
            } else {
                parseBodyPart(bodyPart);
            }
        }

    }

    //
    // Create body structure tree from body part list
    //

    void CMailIMAPBodyStruct::createBodyStructTree(std::unique_ptr<BodyNode>& bodyNode, const std::string& bodyPartStr) {

        uint32_t partNo = 1;
        std::string bodyStructureStr(bodyPartStr.substr(1));
        std::vector<std::string> bodyParts;

        while (bodyStructureStr[0] == '(') {
            bodyParts.push_back(CMailIMAPParse::stringList(bodyStructureStr));
            bodyStructureStr = bodyStructureStr.substr(bodyParts.back().length());
        }

        bodyStructureStr.pop_back();
        bodyStructureStr = bodyStructureStr.substr(1);
        bodyParts.push_back(bodyStructureStr);

        for (auto partStr : bodyParts) {
            if (partStr[1] == '\"') {
                if (!bodyNode->partLevelStr.empty()) {
                    bodyNode->bodyParts.push_back({bodyNode->partLevelStr + "." + std::to_string(partNo), partStr, nullptr, nullptr});
                } else {
                    bodyNode->bodyParts.push_back({bodyNode->partLevelStr + std::to_string(partNo), partStr, nullptr, nullptr});
                }
            } else if (partStr[1] == '(') {
                bodyNode->bodyParts.push_back({"", "", nullptr});
                bodyNode->bodyParts.back().child.reset(new BodyNode());
                bodyNode->bodyParts.back().child->partLevelStr = bodyNode->partLevelStr + std::to_string(partNo);
                createBodyStructTree(bodyNode->bodyParts.back().child, partStr);
            } else {
                bodyNode->extendedStr = partStr;
            }
            partNo++;
        }

    }

    // ==============
    // PUBLIC METHODS
    // ==============

    //
    // If a body structure part contains file attachment details then extract them.
    //

    void CMailIMAPBodyStruct::attachmentFn(std::unique_ptr<BodyNode>& bodyNode, BodyPart& bodyPart, std::shared_ptr<void>& attachmentData) {

        AttachmentData *attachments = static_cast<AttachmentData *> (attachmentData.get());
        std::unordered_map<std::string, std::string> dispositionMap;
        std::string dispositionStr{bodyPart.parsedPart->dispositionStr};

        if (!CMailIMAPParse::stringEqual(dispositionStr, kNILStr)) {
            dispositionStr = dispositionStr.substr(1);
            while (!dispositionStr.empty()) {
                std::string itemStr, valueStr;
                parseNext(dispositionStr, itemStr);
                parseNext(dispositionStr, valueStr);
                dispositionMap.insert({CMailIMAPParse::stringToUpper(itemStr), valueStr});
            }
            std::string attachmentLabelStr;
            auto findAttachment = dispositionMap.find(kATTACHMENTStr);
            auto findInline = dispositionMap.find(kINLINEStr);
            if (findAttachment != dispositionMap.end()) {
                attachmentLabelStr = kATTACHMENTStr;
            } else if (findInline != dispositionMap.end()) {
                attachmentLabelStr = kINLINEStr;
            }
            if (!attachmentLabelStr.empty()) {
                dispositionStr = dispositionMap[attachmentLabelStr];
                if (!CMailIMAPParse::stringEqual(dispositionStr, kNILStr)) {
                    dispositionMap.clear();
                    dispositionStr = dispositionStr.substr(1);
                    while (!dispositionStr.empty()) {
                        std::string itemStr, valueStr;
                        parseNext(dispositionStr, itemStr);
                        parseNext(dispositionStr, valueStr);
                        dispositionMap.insert({CMailIMAPParse::stringToUpper(itemStr), valueStr});
                    }
                    Attachment fileAttachment;
                    fileAttachment.creationDateStr = dispositionMap[kCREATIONDATEStr];
                    fileAttachment.fileNameStr = dispositionMap[kFILENAMEStr];
                    fileAttachment.modifiactionDateStr = dispositionMap[kMODIFICATIONDATEStr];
                    fileAttachment.sizeStr = dispositionMap[kSIZEStr];
                    fileAttachment.partNoStr = bodyPart.partNoStr;
                    fileAttachment.encodingStr = bodyPart.parsedPart->encodingStr;
                    attachments->attachmentsList.push_back(fileAttachment);
                }
            }
        }

    }

    //
    // Construct body structure tree
    //

    void CMailIMAPBodyStruct::consructBodyStructTree(std::unique_ptr<BodyNode>& bodyNode, const std::string& bodyPartStr) {

        createBodyStructTree(bodyNode, bodyPartStr);
        parseBodyStructTree(bodyNode);

    }

    //
    // Walk body structure tree calling user supplied function for each body part.
    //

    void CMailIMAPBodyStruct::walkBodyStructTree(std::unique_ptr<BodyNode>& bodyNode, BodyPartFn walkFn, std::shared_ptr<void>& walkData) {

        for (auto &bodyPart : bodyNode->bodyParts) {
            if (bodyPart.child) {
                walkBodyStructTree(bodyPart.child, walkFn, walkData);
            } else {
                walkFn(bodyNode, bodyPart, walkData);
            }
        }

    }

    //
    // Main CMailIMAPBodyStruct object constructor. 
    //

    CMailIMAPBodyStruct::CMailIMAPBodyStruct() {

    }

    //
    // CMailIMAPBodyStruct Destructor
    //

    CMailIMAPBodyStruct::~CMailIMAPBodyStruct() {

    }

} // namespace Antik