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
#include "CMailIMAPBodyStruct.hpp"

// ====================
// CLASS IMPLEMENTATION
// ====================

//
// C++ STL definitions
//

#include <iostream>
#include <sstream>

// ===========================
// PRIVATE TYPES AND CONSTANTS
// ===========================

//
// BODYSTRUCTURE constants
//

const std::string CMailIMAPBodyStruct::kNILStr("NIL");
const std::string CMailIMAPBodyStruct::kTEXTStr("TEXT");
const std::string CMailIMAPBodyStruct::kATTACHMENTStr("ATTACHMENT");
const std::string CMailIMAPBodyStruct::kINLINEStr("INLINE");
const std::string CMailIMAPBodyStruct::kCREATIONDATEStr("CREATION-DATE");
const std::string CMailIMAPBodyStruct::kFILENAMEStr("FILENAME");
const std::string CMailIMAPBodyStruct::kMODIFICATIONDATEStr("MODIFICATION-DATE");
const std::string CMailIMAPBodyStruct::kSIZEStr("SIZE");
 

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
// Perform case-insensitive string compare (return true strings are equal, false otherwise)
//

inline bool CMailIMAPBodyStruct::stringEqual(const std::string& lineStr, const std::string& compareStr) {

    int cnt01 = 0;
    if (lineStr.length() < compareStr.length()) return (false);
    for (auto &c : compareStr) if (std::toupper(c) != std::toupper(lineStr[cnt01++])) return (false);
    return (true);

}

//
// Convert any lowercase characters in string to upper.
//

inline std::string CMailIMAPBodyStruct::stringToUpper(const std::string& lineStr) {

    std::string upperCase(lineStr);
    for (auto &c : upperCase) c = std::toupper(c);
    return (upperCase);

}

//
// Extract list  from command response line. Note: only check until 
// the end of line; the first character in linsStr is the start 
// of the list ie. a '('.
//

inline std::string CMailIMAPBodyStruct::extractList(const std::string& lineStr) {

    int bracketCount = 0, currentIndex = 0, lineLength = lineStr.length();

    do {
        if (lineStr[currentIndex] == '(') bracketCount++;
        if (lineStr[currentIndex] == ')') bracketCount--;
        currentIndex++;
    } while (bracketCount && (--lineLength > 0));

    return (lineStr.substr(0, currentIndex));

}

//
// Extract the contents between two delimeters in command response line.
//

inline std::string CMailIMAPBodyStruct::extractBetween(const std::string& lineStr, const char first, const char last) {
    int firstDel = lineStr.find_first_of(first);
    int lastDel = lineStr.find_first_of(last, firstDel);
    return (lineStr.substr(firstDel + 1, (lastDel - firstDel - 1)));
}

//
// Extract string between two occurrences of a delimeter character (ie. number and spaces).
//

inline std::string CMailIMAPBodyStruct::extractBetweenDelimeter(const std::string& lineStr, const char delimeter) {

    int firstDel = lineStr.find_first_of(delimeter) + 1;
    int lastDel = lineStr.find_first_of(delimeter, firstDel);
    return (lineStr.substr(firstDel, lastDel - firstDel));

}

//
// Parse and extract next element in IMAP body structure
//

void CMailIMAPBodyStruct::parseNext(std::string& part, std::string& value) {

    if (part.empty()) {
        return;
    } else if (part[0] == '\"') {
        value = extractBetweenDelimeter(part, '\"');
        part = part.substr(value.length() + 3);
    } else if (part[0] == '(') {
        value = extractList(part);
        part = part.substr(value.length() + 1);
    } else if (std::isdigit(part[0])) {
        value = part.substr(0, part.find_first_of(' '));
        part = part.substr(part.find_first_of(' ') + 1);
    } else if (stringEqual(part, kNILStr)) {
        value = kNILStr;
        part = part.substr(value.length() + 1);
    } else {
        throw CMailIMAPBodyStruct::Exception("error while parsing body structure ["+part+"]");
    }

}

//
// Extract body details
//

void CMailIMAPBodyStruct::parseBodyPart(BodyPart& bodyPart) {

    std::string part{bodyPart.part.substr(1)};
 
    bodyPart.parsedPart.reset(new BodyPartParsed());

    parseNext(part, bodyPart.parsedPart->type);
    parseNext(part, bodyPart.parsedPart->subtype);
    parseNext(part, bodyPart.parsedPart->parameterList);
    parseNext(part, bodyPart.parsedPart->id);
    parseNext(part, bodyPart.parsedPart->description);
    parseNext(part, bodyPart.parsedPart->encoding);
    parseNext(part, bodyPart.parsedPart->size);

    if (stringEqual(bodyPart.parsedPart->type, kTEXTStr)) {
        parseNext(part, bodyPart.parsedPart->textLines);
    }

    parseNext(part, bodyPart.parsedPart->md5);
    parseNext(part, bodyPart.parsedPart->disposition);
    parseNext(part, bodyPart.parsedPart->language);
    parseNext(part, bodyPart.parsedPart->location);

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
 
void CMailIMAPBodyStruct::createBodyStructTree(std::unique_ptr<BodyNode>& bodyNode, const std::string& bodyPart) {

    uint32_t partNo = 1;
    std::string bodyStructure(bodyPart.substr(1));
    std::vector<std::string> bodyParts;

    while (bodyStructure[0] == '(') {
        bodyParts.push_back(extractList(bodyStructure));
        bodyStructure = bodyStructure.substr(bodyParts.back().length());
    }
    
    bodyStructure.pop_back();
    bodyStructure = bodyStructure.substr(1);
    bodyParts.push_back(bodyStructure);

    for (auto part : bodyParts) {
        if (part[1] == '\"') {
            if (!bodyNode->partLevel.empty()) {
                bodyNode->bodyParts.push_back({bodyNode->partLevel + "." + std::to_string(partNo), part, nullptr, nullptr});
            } else {
                bodyNode->bodyParts.push_back({bodyNode->partLevel + std::to_string(partNo), part, nullptr, nullptr});
            }
        } else if (part[1] == '(') {
            bodyNode->bodyParts.push_back({"", "", nullptr});
            bodyNode->bodyParts.back().child.reset(new BodyNode());
            bodyNode->bodyParts.back().child->partLevel = bodyNode->partLevel + std::to_string(partNo);
            createBodyStructTree(bodyNode->bodyParts.back().child, part);
        } else {
            bodyNode->extended = part;
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
    std::string disposition{bodyPart.parsedPart->disposition};
    
    if (!stringEqual(disposition, kNILStr)) {
        disposition = disposition.substr(1);
        while (!disposition.empty()) {
            std::string item, value;
            parseNext(disposition, item);
            parseNext(disposition, value);
            dispositionMap.insert({stringToUpper(item), value});
        }
        std::string attachmentLabel;
        auto findAttachment = dispositionMap.find(kATTACHMENTStr);
        auto findInline = dispositionMap.find(kINLINEStr);
        if (findAttachment != dispositionMap.end()) {
            attachmentLabel = kATTACHMENTStr;
        } else if (findInline != dispositionMap.end()) {
            attachmentLabel = kINLINEStr;
        }
        if (!attachmentLabel.empty()) {
            disposition = dispositionMap[attachmentLabel];
            if (!stringEqual(disposition, kNILStr)) {
                dispositionMap.clear();
                disposition = disposition.substr(1);
                while (!disposition.empty()) {
                    std::string item, value;
                    parseNext(disposition, item);
                    parseNext(disposition, value);
                    dispositionMap.insert({stringToUpper(item), value});
                }
                Attachment fileAttachment;
                fileAttachment.creationDate = dispositionMap[kCREATIONDATEStr];
                fileAttachment.fileName = dispositionMap[kFILENAMEStr];
                fileAttachment.modifiactionDate = dispositionMap[kMODIFICATIONDATEStr];
                fileAttachment.size = dispositionMap[kSIZEStr];
                fileAttachment.partNo = bodyPart.partNo;
                attachments->attachmentsList.push_back(fileAttachment);
            }
        }
    }

}

//
// Construct body structure tree
//

void CMailIMAPBodyStruct::consructBodyStructTree(std::unique_ptr<BodyNode>& bodyNode, const std::string& bodyPart) {

    createBodyStructTree(bodyNode, bodyPart);
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