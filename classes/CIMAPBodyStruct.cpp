#include "HOST.hpp"
/*
 * File:   CIMAPBodyStruct.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on January 24, 2017, 2:33 PM
 *
 * Copyright 2016.
 *
 */

//
// Class: CIMAPBodyStruct
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

#include "CIMAP.hpp"
#include "CIMAPParse.hpp"
#include "CIMAPBodyStruct.hpp"

// ====================
// CLASS IMPLEMENTATION
// ====================

//
// C++ STL
//

#include <iostream>
#include <sstream>

// =========
// NAMESPACE
// =========

namespace Antik::IMAP
{

// ===========================
// PRIVATE TYPES AND CONSTANTS
// ===========================

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

void CIMAPBodyStruct::parseNext(std::string &part, std::string &value)
{

    if (part.empty())
    {
        return;
    }
    else if (part[0] == '\"')
    {
        value = CIMAPParse::stringBetween(part, '\"', '\"');
        part = part.substr(value.length() + 3);
    }
    else if (part[0] == '(')
    {
        value = CIMAPParse::stringList(part);
        part = part.substr(value.length() + 1);
    }
    else if (std::isdigit(part[0]))
    {
        value = part.substr(0, part.find_first_of(' '));
        part = part.substr(part.find_first_of(' ') + 1);
    }
    else if (CIMAPParse::stringStartsWith(part, kNIL))
    {
        value = kNIL;
        part = part.substr(value.length() + 1);
    }
    else
    {
        throw Exception("error while parsing body structure [" + part + "]");
    }
}

//
// Extract body details
//

void CIMAPBodyStruct::parseBodyPart(BodyPart &bodyPart)
{

    std::string part{bodyPart.part.substr(1)};

    bodyPart.parsedPart.reset(new BodyPartParsed());

    parseNext(part, bodyPart.parsedPart->type);
    parseNext(part, bodyPart.parsedPart->subtype);
    parseNext(part, bodyPart.parsedPart->parameterList);
    parseNext(part, bodyPart.parsedPart->id);
    parseNext(part, bodyPart.parsedPart->description);
    parseNext(part, bodyPart.parsedPart->encoding);
    parseNext(part, bodyPart.parsedPart->size);

    if (CIMAPParse::CIMAPParse::stringStartsWith(bodyPart.parsedPart->type, kTEXT))
    {
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

void CIMAPBodyStruct::parseBodyStructTree(std::unique_ptr<BodyNode> &bodyNode)
{

    for (auto &bodyPart : bodyNode->bodyParts)
    {
        if (bodyPart.child)
        {
            parseBodyStructTree(bodyPart.child);
        }
        else
        {
            parseBodyPart(bodyPart);
        }
    }
}

//
// Create body structure tree from body part list
//

void CIMAPBodyStruct::createBodyStructTree(std::unique_ptr<BodyNode> &bodyNode, const std::string &bodyPart)
{

    std::uint32_t partNo{1};
    std::string bodyucture(bodyPart.substr(1));
    std::vector<std::string> bodyParts;

    while (bodyucture[0] == '(')
    {
        bodyParts.push_back(CIMAPParse::stringList(bodyucture));
        bodyucture = bodyucture.substr(bodyParts.back().length());
    }

    bodyucture.pop_back();
    bodyucture = bodyucture.substr(1);
    bodyParts.push_back(bodyucture);

    for (auto part : bodyParts)
    {
        if (part[1] == '\"')
        {
            if (!bodyNode->partLevel.empty())
            {
                bodyNode->bodyParts.push_back({bodyNode->partLevel + "." + std::to_string(partNo), part, nullptr, nullptr});
            }
            else
            {
                bodyNode->bodyParts.push_back({bodyNode->partLevel + std::to_string(partNo), part, nullptr, nullptr});
            }
        }
        else if (part[1] == '(')
        {
            bodyNode->bodyParts.push_back({"", "", nullptr, nullptr});
            bodyNode->bodyParts.back().child.reset(new BodyNode());
            bodyNode->bodyParts.back().child->partLevel = bodyNode->partLevel + std::to_string(partNo);
            createBodyStructTree(bodyNode->bodyParts.back().child, part);
        }
        else
        {
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

void CIMAPBodyStruct::attachmentFn(std::unique_ptr<BodyNode> &/*bodyNode*/, BodyPart &bodyPart, std::shared_ptr<void> &attachmentData)
{

    AttachmentData *attachments{static_cast<AttachmentData *>(attachmentData.get())};
    std::unordered_map<std::string, std::string> dispositionMap;
    std::string disposition{bodyPart.parsedPart->disposition};

    if (!CIMAPParse::stringStartsWith(disposition, kNIL))
    {
        disposition = disposition.substr(1);
        while (!disposition.empty())
        {
            std::string item, value;
            parseNext(disposition, item);
            parseNext(disposition, value);
            dispositionMap.insert({CIMAPParse::stringToUpper(item), value});
        }
        std::string attachmentLabel;
        auto findAttachment = dispositionMap.find(kATTACHMENT);
        auto findInline = dispositionMap.find(kINLINE);
        if (findAttachment != dispositionMap.end())
        {
            attachmentLabel = kATTACHMENT;
        }
        else if (findInline != dispositionMap.end())
        {
            attachmentLabel = kINLINE;
        }
        if (!attachmentLabel.empty())
        {
            disposition = dispositionMap[attachmentLabel];
            if (!CIMAPParse::stringStartsWith(disposition, kNIL))
            {
                dispositionMap.clear();
                disposition = disposition.substr(1);
                while (!disposition.empty())
                {
                    std::string item, value;
                    parseNext(disposition, item);
                    parseNext(disposition, value);
                    dispositionMap.insert({CIMAPParse::stringToUpper(item), value});
                }
                Attachment fileAttachment;
                fileAttachment.creationDate = dispositionMap[kCREATIONDATE];
                fileAttachment.fileName = dispositionMap[kFILENAME];
                fileAttachment.modifiactionDate = dispositionMap[kMODIFICATIONDATE];
                fileAttachment.size = dispositionMap[kSIZE];
                fileAttachment.partNo = bodyPart.partNo;
                fileAttachment.encoding = bodyPart.parsedPart->encoding;
                attachments->attachmentsList.push_back(fileAttachment);
            }
        }
    }
}

//
// Construct body structure tree
//

void CIMAPBodyStruct::consructBodyStructTree(std::unique_ptr<BodyNode> &bodyNode, const std::string &bodyPart)
{

    createBodyStructTree(bodyNode, bodyPart);
    parseBodyStructTree(bodyNode);
}

//
// Walk body structure tree calling user supplied function for each body part.
//

void CIMAPBodyStruct::walkBodyStructTree(std::unique_ptr<BodyNode> &bodyNode, BodyPartFn walkFn, std::shared_ptr<void> &walkData)
{

    for (auto &bodyPart : bodyNode->bodyParts)
    {
        if (bodyPart.child)
        {
            walkBodyStructTree(bodyPart.child, walkFn, walkData);
        }
        else
        {
            walkFn(bodyNode, bodyPart, walkData);
        }
    }
}

} // namespace Antik::IMAP
