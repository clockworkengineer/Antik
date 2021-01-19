#ifndef CZIPIO_HPP
#define CZIPIO_HPP
//
// C++ STL
//
#include <string>
#include <vector>
#include <stdexcept>
#include <fstream>
//
// Antik classes
//
#include "CommonAntik.hpp"
// =========
// NAMESPACE
// =========
namespace Antik::ZIP
{
    // ===========================
    // PRIVATE TYPES AND CONSTANTS
    // ===========================
    //
    // ZIP archive compression methods.
    //
    constexpr std::uint16_t kZIPCompressionStore{0};
    constexpr std::uint16_t kZIPCompressionDeflate{8};
    //
    // ZIP archive versions
    //
    constexpr std::uint8_t kZIPVersion10{0x0a};
    constexpr std::uint8_t kZIPVersion20{0x14};
    constexpr std::uint8_t kZIPVersion45{0x2d};
    //
    // ZIP archive creator
    //
    static const std::uint8_t kZIPCreatorUnix{0x03};
    // ================
    // CLASS DEFINITION
    // ================
    class CZIPIO
    {
    public:
        // ==========================
        // PUBLIC TYPES AND CONSTANTS
        // ==========================
        //
        // Although these are meant to be the specified sizes add a double check.
        //
        static_assert(sizeof(std::uint16_t) == 2, "Error : std::uint16_t needs to be 2 bytes.");
        static_assert(sizeof(std::uint32_t) == 4, "Error : std::uint32_t needs to be 4 bytes.");
        static_assert(sizeof(std::uint64_t) == 8, "Error : std::uint64_t needs to be 8 bytes.");
        //
        // Class exception
        //
        struct Exception : public std::runtime_error
        {
            explicit Exception(std::string const &message)
                : std::runtime_error("CFileZIPIO Failure: " + message)
            {
            }
        };
        //
        // Archive Local File Header record
        //
        struct LocalFileHeader
        {
            const std::uint32_t size{30};
            const std::uint32_t signature{0x04034b50};
            std::uint16_t creatorVersion{0};
            std::uint16_t bitFlag{0};
            std::uint16_t compression{0};
            std::uint16_t modificationTime{0};
            std::uint16_t modificationDate{0};
            std::uint32_t crc32{0};
            std::uint32_t compressedSize{0};
            std::uint32_t uncompressedSize{0};
            std::uint16_t fileNameLength{0};
            std::uint16_t extraFieldLength{0};
            std::string fileName;
            std::vector<std::uint8_t> extraField;
        };
        //
        // Archive Data Descriptor record.
        //
        struct DataDescriptor
        {
            const std::uint32_t size{12};
            const std::uint32_t signature{0x08074b50};
            std::uint32_t crc32{0};
            std::uint32_t compressedSize{0};
            std::uint32_t uncompressedSize{0};
        };
        //
        // Archive Central Directory File Header record.
        //
        struct CentralDirectoryFileHeader
        {
            const std::uint32_t size{46};
            const std::uint32_t signature{0x02014b50};
            std::uint16_t creatorVersion{(kZIPCreatorUnix << 8) | kZIPVersion20};
            std::uint16_t extractorVersion{kZIPVersion20};
            std::uint16_t bitFlag{0};
            std::uint16_t compression{kZIPCompressionDeflate};
            std::uint16_t modificationTime{0};
            std::uint16_t modificationDate{0};
            std::uint32_t crc32{0};
            std::uint32_t compressedSize{0};
            std::uint32_t uncompressedSize{0};
            std::uint16_t fileNameLength{0};
            std::uint16_t extraFieldLength{0};
            std::uint16_t fileCommentLength{0};
            std::uint16_t diskNoStart{0};
            std::uint16_t internalFileAttrib{0};
            std::uint32_t externalFileAttrib{0};
            std::uint32_t fileHeaderOffset{0};
            std::string fileName;
            std::vector<std::uint8_t> extraField;
            std::string fileComment;
        };
        //
        // Archive End Of Central Directory record.
        //
        struct EOCentralDirectoryRecord
        {
            const std::uint32_t size{22};
            const std::uint32_t signature{0x06054b50};
            std::uint16_t diskNumber{0};
            std::uint16_t startDiskNumber{0};
            std::uint16_t numberOfCentralDirRecords{0};
            std::uint16_t totalCentralDirRecords{0};
            std::uint32_t sizeOfCentralDirRecords{0};
            std::uint32_t offsetCentralDirRecords{0};
            std::uint16_t commentLength{0};
            std::string comment;
        };
        //
        // ZIP64 Archive End Of Central Directory record.
        //
        struct Zip64EOCentralDirectoryRecord
        {
            const std::uint32_t size{56};
            const std::uint32_t signature{0x06064b50};
            std::uint64_t totalRecordSize{0};
            std::uint16_t creatorVersion{(kZIPCreatorUnix << 8) | kZIPVersion45};
            std::uint16_t extractorVersion{kZIPVersion45};
            std::uint32_t diskNumber{0};
            std::uint32_t startDiskNumber{0};
            std::uint64_t numberOfCentralDirRecords{0};
            std::uint64_t totalCentralDirRecords{0};
            std::uint64_t sizeOfCentralDirRecords{0};
            std::uint64_t offsetCentralDirRecords{0};
            std::vector<std::uint8_t> extensibleDataSector;
        };
        //
        // ZIP64 Archive End Of Central Directory record locator.
        //
        struct Zip64EOCentDirRecordLocator
        {
            const std::uint32_t size{20};
            const std::uint32_t signature{0x07064b50};
            std::uint32_t startDiskNumber{0};
            std::uint64_t offset{0};
            std::uint32_t numberOfDisks{0};
        };
        //
        // ZIP64 Archive extended information field.
        //
        struct Zip64ExtendedInfoExtraField
        {
            const std::uint16_t signature{0x0001};
            std::uint16_t size{0};
            std::uint64_t originalSize{0};
            std::uint64_t compressedSize{0};
            std::uint64_t fileHeaderOffset{0};
            std::uint32_t diskNo{0};
        };
        // ============
        // CONSTRUCTORS
        // ============
        CZIPIO();
        // ==========
        // DESTRUCTOR
        // ==========
        virtual ~CZIPIO();
        // ==============
        // PUBLIC METHODS
        // ==============
        //
        // Return true if field all 1's
        //
        template <typename T>
        static bool fieldOverflow(const T &field);
        //
        // Return true if field value requires more bits
        //
        static bool fieldRequires64bits(std::uint64_t field)
        {
            return (field & 0xFFFFFFFF00000000);
        };
        static bool fieldRequires32bits(std::uint32_t field)
        {
            return (field & 0xFFFF0000);
        };
        //
        // Put ZIP record into byte array and write to ZIP archive.
        //
        void putZIPRecord(DataDescriptor &entry);
        void putZIPRecord(CentralDirectoryFileHeader &entry);
        void putZIPRecord(LocalFileHeader &entry);
        void putZIPRecord(EOCentralDirectoryRecord &entry);
        void putZIPRecord(Zip64EOCentralDirectoryRecord &entry);
        void putZIPRecord(Zip64EOCentDirRecordLocator &entry);
        //
        // Place ZIP64 extended information
        //
        static void putZip64ExtendedInfoExtraField(Zip64ExtendedInfoExtraField &extendedInfo, std::vector<std::uint8_t> &info);
        //
        // Read ZIP archive record into byte array and place into structure.
        //
        void getZIPRecord(DataDescriptor &entry);
        void getZIPRecord(CentralDirectoryFileHeader &entry);
        void getZIPRecord(LocalFileHeader &entry);
        void getZIPRecord(EOCentralDirectoryRecord &entry);
        void getZIPRecord(Zip64EOCentralDirectoryRecord &entry);
        void getZIPRecord(Zip64EOCentDirRecordLocator &entry);
        //
        // Extraxct ZIP64 extended information
        //
        static void getZip64ExtendedInfoExtraField(Zip64ExtendedInfoExtraField &zip64ExtendedInfo, std::vector<std::uint8_t> &info);
        //
        // ZIP Archive file I/O
        //
        void openZIPFile(const std::string &fileName, std::ios_base::openmode mode);
        void closeZIPFile(void);
        void positionInZIPFile(std::uint64_t offset);
        std::uint64_t currentPositionZIPFile(void);
        void writeZIPFile(std::vector<std::uint8_t> &buffer, std::uint64_t count);
        void readZIPFile(std::vector<std::uint8_t> &buffer, std::uint64_t count);
        std::uint64_t readCountZIPFile(void);
        bool errorInZIPFile(void);
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
        CZIPIO(const CZIPIO &orig) = delete;
        CZIPIO(const CZIPIO &&orig) = delete;
        CZIPIO &operator=(CZIPIO other) = delete;
        // ===============
        // PRIVATE METHODS
        // ===============
        //
        // Put/get ZIP record fields from byte array (buffer).
        //
        template <typename T>
        static void putField(T field, std::vector<std::uint8_t> &buffer);
        template <typename T>
        static std::uint8_t *getField(T &field, std::uint8_t *buffptr);
        //
        // Worker methods for put/get field.
        //
        static void readZIPRecord(std::fstream &zipFileStream, DataDescriptor &entry);
        static void readZIPRecord(std::fstream &zipFileStream, CentralDirectoryFileHeader &entry);
        static void readZIPRecord(std::fstream &zipFileStream, LocalFileHeader &entry);
        static void readZIPRecord(std::fstream &zipFileStream, EOCentralDirectoryRecord &entry);
        static void readZIPRecord(std::fstream &zipFileStream, Zip64EOCentralDirectoryRecord &entry);
        static void readZIPRecord(std::fstream &zipFileStream, Zip64EOCentDirRecordLocator &entry);
        static void writeZIPRecord(std::fstream &zipFileStream, DataDescriptor &entry);
        static void writeZIPRecord(std::fstream &zipFileStream, CentralDirectoryFileHeader &entry);
        static void writeZIPRecord(std::fstream &zipFileStream, LocalFileHeader &entry);
        static void writeZIPRecord(std::fstream &zipFileStream, EOCentralDirectoryRecord &entry);
        static void writeZIPRecord(std::fstream &zipFileStream, Zip64EOCentralDirectoryRecord &entry);
        static void writeZIPRecord(std::fstream &zipFileStream, Zip64EOCentDirRecordLocator &entry);
        // =================
        // PRIVATE VARIABLES
        // =================
        //
        // ZIP archive I/O stream
        //
        std::fstream m_zipFileStream;
    };
    //
    // Return true if field contains all 1s.
    //
    template <typename T>
    bool CZIPIO::fieldOverflow(const T &field)
    {
        return (field == static_cast<T>(~0));
    }
    //
    // Place a word into buffer.
    //
    template <typename T>
    void CZIPIO::putField(T field, std::vector<std::uint8_t> &buffer)
    {
        std::uint16_t size = sizeof(T);
        while (size--)
        {
            buffer.push_back(static_cast<std::uint8_t>(field & 0xFF));
            field >>= 8;
        }
    }
    //
    // Get word from buffer. Incrementing buffptr by the word size after.
    //
    template <typename T>
    std::uint8_t *CZIPIO::getField(T &field, std::uint8_t *buffptr)
    {
        std::uint16_t size = sizeof(T) - 1;
        field = buffptr[size];
        do
        {
            field <<= 8;
            field |= buffptr[size - 1];
        } while (--size);
        return (buffptr + sizeof(T));
    }
} // namespace Antik::ZIP
#endif /* CZIPIO_HPP */
