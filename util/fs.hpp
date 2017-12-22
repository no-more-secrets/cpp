/****************************************************************
* filesystem related utilities
****************************************************************/
#pragma once

#include "types.hpp"

#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

namespace util {

// This enum will be used to select options of  case  sensitivity
// when comparing paths.
enum class CaseSensitive { DEFAULT, YES, NO };

// This will put the path (which  may not exist) into normal form
// and preserving absolute/relative nature.  Path must exist, and
// will resolve symlinks.
fs::path normpath( fs::path const& p );

// This  is  like normpath except that it makes the path absolute
// (relative to cwd) if it is not already). Path must  exist  and
// will resolve symlinks.
fs::path absnormpath( fs::path const& p );

/* Put  a  path into normal form without regard to whether or not
 * it  exists  (and  do so without touching the filesystem at all.
 * The C++17 filesystem library has this  method as a member func-
 * tion of the path class, but it has not yet been implemented in
 * gcc at the time of this writing.  Once gcc 8 is released, then
 * this  function  can  be  deleted and the usage of it can be re-
 * placed with p.lexicaly_normal(). */
fs::path lexically_normal( fs::path const& p );

/* Implemenation of the  lexically_relative  function.  Find  the
 * relative  path between the given path and base path without re-
 * gard  to  whether or not it exists (and do so without touching
 * the  filesystem  at  all; hence we also don't follow symlinks).
 * The C++17 filesystem library has this  method as a member func-
 * tion of the path class, but it has not yet been implemented in
 * gcc at the time of this writing.  Once gcc 8 is released, then
 * this function can probably be deleted and the usage of it  can
 * be replaced with p.lexicaly_relative(). */
fs::path lexically_relative( fs::path const& p,
                             fs::path const& base );

// This  is  a  simplified version of lexically_relative which as-
// sumes (assumptions are not verified for efficiency!) that both
// input  paths  are  either  absolute or both are relative, that
// both are in normal form, and that the base path has no  double
// dots. If you call this function  with  those  assumptions  vio-
// lated then it's not certain what you will  get.  NOTE:  perfor-
// mance of this has  not  actually  been  measured,  so it's not
// clear if it's really faster (it may well not be).
fs::path lexically_relative_fast( fs::path const& p,
                                  fs::path const& base );

// Flip any backslashes to forward slashes.
std::string fwd_slashes( std::string_view in );

// Flip any backslashes to forward slashes.
StrVec fwd_slashes( StrVec const& v );

// Constructs  a path from a pair of iterators to path components.
// Didn't see this available in  the  standard,  but  could  have
// missed it. The name is meant  for  it  to look like a construc-
// tor, even though it isn't.
fs::path path_( fs::path::const_iterator b,
                fs::path::const_iterator e );

// The purpose of this function  is  to  compare two paths lexico-
// graphically  with the option of case insensitivity. On Windows,
// the comparison will  default  to  being  case  insensitive. On
// Linux it will default to case-sensitive, but  in  either  case,
// this can be overridden.
bool path_equals( fs::path const& a,
                  fs::path const& b,
                  CaseSensitive   sen = CaseSensitive::DEFAULT );

} // namespace util

// It seems that, at the time of this writing, std::hash has  not
// been given a specialization for fs::path, so  we  will  inject
// one into the std namespace so that we can use it as the key in
// unordered maps. This is simple to do  because  the  filesystem
// library does already provide the hash_value method which  does
// the actual hashing of a path. Note that two paths' hashes will
// be  equal when equality is satisfied, so e.g. A//B is the same
// as A/B. If one is  added  in  the  future  then this may start
// causing compilation failures, in which  case it should just be
// deleted.
namespace std {

    template<> struct hash<fs::path> {
        auto operator()( fs::path const& p ) const noexcept {
            return fs::hash_value( p );
        }
    };

} // namespace std
