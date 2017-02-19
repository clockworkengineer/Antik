/*
 * File:   CMailIMAPBodyStruct.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on Januray 24, 2017, 2:33 PM
 *
 * Copyright 2016.
 
 */

#ifndef CMAILIMAPBODYSTRUCT_HPP
#define CMAILIMAPBODYSTRUCT_HPP

//
// C++ STL definitions
//

#include <vector>
#include <memory>
#include <unordered_map>
#include <stdexcept>

// ================
// CLASS DEFINITION
// ================

class CMailIMAPBodyStruct {
public:

    // ==========================
    // PUBLIC TYPES AND CONSTANTS
    // ==========================

    //
    // Class exception
    //

    struct Exception : public std::runtime_error {

        Exception(std::string const& message)
        : std::runtime_error("CMailIMAPBodyStruct Failure: " + message) {
        }

    };

    //
    // Parsed body part contents
    //
    
    struct BodyPartParsed {
        std::string type;           // Body type
        std::string subtype;        // Body subtype
        std::string parameterList;  // Body parameter list
        std::string id;             // Body id
        std::string description;    // Body Description
        std::string encoding;       // Body encoding
        std::string size;           // Body size
        std::string textLines;      // Body ("TEXT") extended no of text lines
        std::string md5;            // Body MD5 value
        std::string disposition;    // Body disposition list
        std::string language;       // Body language
        std::string location;       // Body location
        std::string extended;       // Body extended data (should be empty)

    };

    //
    // Body structure tree
    //
    
    struct BodyPart;
    
    struct BodyNode {
        std::string             partLevel;      // Body part level
        std::vector<BodyPart>   bodyParts;      // Vector of body parts and child nodes
        std::string             extended;       // Multi-part extended data for level
    };

    struct BodyPart {
        std::string                     partNo;      // Body part no (ie. 1 or 1.2..)                 
        std::string                     part;        // Body part contents
        std::unique_ptr<BodyPartParsed> parsedPart;  // Parsed body part data
        std::unique_ptr<BodyNode>       child;       // Pointer to lower level node in tree
    };

    //
    // Body attachment details
    //
    
    struct Attachment {
        std::string index;
        std::string partNo;
        std::string creationDate;
        std::string fileName;
        std::string modifiactionDate;
        std::string size;
    };

    struct AttachmentData {
        std::vector<Attachment> attachmentsList;
    };


    typedef std::function<void (std::unique_ptr<BodyNode>&, BodyPart&, std::shared_ptr<void>&) > BodyPartFn;

    //
    // BODYSTRUCTURE constants
    //

    static const std::string kNILStr;
    static const std::string kTEXTStr;
    static const std::string kATTACHMENTStr;
    static const std::string kINLINEStr;
    static const std::string kCREATIONDATEStr;
    static const std::string kFILENAMEStr;
    static const std::string kMODIFICATIONDATEStr;
    static const std::string kSIZEStr;
    
    // ============
    // CONSTRUCTORS
    // ============

    // ==========
    // DESTRUCTOR
    // ==========

    // ==============
    // PUBLIC METHODS
    // ==============
    
    //
    // Construct body structure tree
    //
    
    static void consructBodyStructTree(std::unique_ptr<BodyNode>& bodyNode, const std::string& bodyPart);
        
    //
    // Walk body structure tree calling use supplied function for each body part.
    //
    
    static void walkBodyStructTree(std::unique_ptr<BodyNode>& bodyNode, BodyPartFn walkFn, std::shared_ptr<void>&  walkData);

    //
    // Walk function to extract file attachments.
    //
    
    static void attachmentFn(std::unique_ptr<BodyNode>& bodyNode, BodyPart& bodyPart, std::shared_ptr<void>& attachmentData);

    // ================
    // PUBLIC VARIABLES
    // ================

private:

    // ===========================
    // PRIVATE TYPES AND CONSTANTS
    // ===========================


    // ===========================================
    // DISABLED CONSTRUCTORS/DESTRUCTORS/OPERATORS
    // ===========================================

    CMailIMAPBodyStruct() = delete;
    CMailIMAPBodyStruct(const CMailIMAPBodyStruct & orig) = delete;
    CMailIMAPBodyStruct(const CMailIMAPBodyStruct && orig) = delete;
    virtual ~CMailIMAPBodyStruct() = delete;
    CMailIMAPBodyStruct& operator=(CMailIMAPBodyStruct other) = delete;

    // ===============
    // PRIVATE METHODS
    // ===============
    
    //
    // Utility methods (duplicates of CMailIMAPParse methods.NEED TO REFACTOR)
    //
  
    static bool stringEqual(const std::string& lineStr, const std::string& compareStr);
    static std::string stringToUpper(const std::string& lineStr);
    static std::string extractList(const std::string& lineStr);
    static std::string extractBetween(const std::string& lineStr, const char first, const char last);
    static std::string extractBetweenDelimeter(const std::string& lineStr, const char delimeter);
    
    //
    // Parse body structure tree filling in body part data
    //
    
    static void parseNext(std::string& part, std::string& value);
    static void parseBodyPart(BodyPart& bodyPart);
    static void parseBodyStructTree(std::unique_ptr<BodyNode>& bodyNode);
    
    //
    // Create body structure tree from body part list
    //
    
    static void createBodyStructTree(std::unique_ptr<BodyNode>& bodyNode, const std::string& bodyPart);

    // =================
    // PRIVATE VARIABLES
    // =================

};
#endif /* CMAILIMAPBODYSTRUCT_HPP */

