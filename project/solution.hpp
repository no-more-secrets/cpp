/****************************************************************
* Solution File Parsing and Processing
****************************************************************/
#pragma once

#include "project.hpp"
#include "types.hpp"

#include <experimental/filesystem>
#include <map>
#include <vector>

namespace fs = std::experimental::filesystem;

namespace project {

/****************************************************************
* SolutionFile
****************************************************************/
struct SolutionFile {

    SolutionFile( PathVec&& projects );

    // Reads  in  the  solution  file and parses it for any lines
    // that declare projects, and then  extracts the project file
    // location (vcxproj) for each project.
    static SolutionFile read( fs::path const& file );

    // Holds only paths to projects, and those paths will be  rel-
    // ative to the folder containing the solutiont file.
    PathVec projects;
};

// Mainly for debugging.
std::ostream& operator<<( std::ostream&       out,
                          SolutionFile const& s );

/****************************************************************
* Solution
****************************************************************/
struct Solution {

    Solution( std::map<fs::path, Project>&& projects );

    // Read in a solution file, parse  it  to  get  the  list  of
    // projects that it contains, then  read in each project file
    // and parse it completely.
    static Solution read( fs::path const&  file,
                          std::string_view platform,
                          fs::path const&  base = "" );

    // Holds path to vcxproj file and full project data.
    std::map<fs::path, Project> projects;
};

// Mainly for debugging.
std::ostream& operator<<( std::ostream&   out,
                          Solution const& s );

} // namespace project
