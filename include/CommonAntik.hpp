#ifndef COMMON_ANTIK_HPP
#define COMMON_ANTIK_HPP
//
// C++ STL
//
#include <string>
#include <vector>
namespace Antik
{
    //
    // Container for list of file paths
    //
    using FileList = std::vector<std::string>;
    //
    // Server path separator
    //
    const char kServerPathSep{'/'};
} // namespace Antik
#endif /* COMMON_HPP */
