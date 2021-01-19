#ifndef CURL_HPP
#define CURL_HPP
//
// C++ STL
//
#include <stdexcept>
#include <string>
#include <vector>
#include <memory>
//
// libcurl
//
#include <curl/curl.h>
// =========
// NAMESPACE
// =========
namespace Antik::Network
{
    // ==========================
    // PUBLIC TYPES AND CONSTANTS
    // ==========================
    // ================
    // CLASS DEFINITION
    // ================
    class CCurl
    {
    public:
        // ==========================
        // PUBLIC TYPES AND CONSTANTS
        // ==========================
        //
        // Class exception
        //
        struct Exception : public std::runtime_error
        {
            Exception(std::string const &message)
                : std::runtime_error("CCurl Failure: " + message)
            {
            }
        };
        //
        // Curl return status code, get info, set option and string list.
        //
        using StatusCode = CURLcode;
        using Info = CURLINFO;
        using Option = CURLoption;
        using StringList = struct curl_slist *;
        // ============
        // CONSTRUCTORS
        // ============
        //
        // Main constructor.
        //
        CCurl(void);
        // ==========
        // DESTRUCTOR
        // ==========
        virtual ~CCurl();
        // ==============
        // PUBLIC METHODS
        // ==============
        //
        // Set extended error buffer size.
        //
        void setErrorBuffer(size_t errorBufferSize);
        //
        // Set connection option.
        //
        template <typename T>
        void setOption(const Option &option, T value);
        //
        // Get connection information.
        //
        template <typename T>
        T getInfo(const Info &info);
        //
        // Perform connection transfer.
        //
        void transfer();
        //
        // Perform connection reset.
        //
        void reset();
        //
        // Curl global closedown(cleanup).
        //
        static void globalCleanup()
        {
            curl_global_cleanup();
        }
        //
        // Append character string to string list.
        //
        static StringList stringListAppend(const StringList &inStringList, const char *string)
        {
            StringList outStringList = curl_slist_append(inStringList, string);
            if (outStringList == NULL)
            {
                throw Exception("Failed to append to string list.");
            }
            return (outStringList);
        }
        //
        // Free string list memory.
        //
        static void stringListFree(const StringList &stringList)
        {
            curl_slist_free_all(stringList);
        }
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
        CCurl(const CCurl &orig) = delete;
        CCurl(const CCurl &&orig) = delete;
        CCurl &operator=(CCurl other) = delete;
        // ===============
        // PRIVATE METHODS
        // ===============
        // =================
        // PRIVATE VARIABLES
        // =================
        std::string m_errorBuffer;       // Extended error buffer
        CURL *m_curlConnection;          // Curl connection
        static bool m_globalInitialised; // == true global intialisation performed.
    };
    //
    // Set connection options
    //
    template <typename T>
    void CCurl::setOption(const Option &option, T value)
    {
        auto code = curl_easy_setopt(m_curlConnection, option, value);
        if (code != CURLE_OK)
        {
            if (m_errorBuffer[0])
            {
                throw Exception("Failed to set option." + m_errorBuffer);
            }
            else
            {
                throw Exception(std::string("Failed to set option.") + curl_easy_strerror(code) + ".");
            }
        }
    }
    //
    // Get connection information.
    //
    template <typename T>
    T CCurl::getInfo(const Info &info)
    {
        T infoValue;
        auto code = curl_easy_getinfo(m_curlConnection, info, &infoValue);
        if (code != CURLE_OK)
        {
            if (m_errorBuffer[0])
            {
                throw Exception("Failed to get information." + m_errorBuffer);
            }
            else
            {
                throw Exception(std::string("Failed to get information.") + curl_easy_strerror(code) + ".");
            }
        }
        return (infoValue);
    }
} // namespace Antik::Network
#endif /* CURL_HPP */
