/*
 * File:   CommonAntik.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on September 23, 2017, 2:33 PM
 *
 * Copyright 2017.
 *
 */

#ifndef COMMON_ANTIK_HPP
#define COMMON_ANTIK_HPP

//
// C++ STL
//

#include <string>
#include <vector>

namespace Antik {
    
        //
        // Container for list of file paths
        //
        
        using FileList = std::vector<std::string>;
        
        //
        // Server path separator
        //
        
        const char kServerPathSep { '/' };
        
} // namespace Antik

#endif /* COMMON_HPP */

