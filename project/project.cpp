/****************************************************************
* Project & Solution File Parsers
****************************************************************/
#include "macros.hpp"
#include "opt-util.hpp"
#include "project.hpp"
#include "string-util.hpp"
#include "xml-utils.hpp"

#include <algorithm>
#include <sstream>

using namespace std;

namespace project {

namespace xpaths {

char const* cl_compiles = "/Project/ItemGroup/ClCompile/@Include";
char const* cl_includes = "/Project/ItemGroup/ClInclude/@Include";

char const* search_paths =
    " /descendant-or-self::node()[              "
    "     contains(@Condition,$platform)        "
    " ]                                         "
    " /descendant-or-self::node()[              "
    "     name()='AdditionalIncludeDirectories' "
    " ]                                         ";

char const* int_dir =
    " /descendant-or-self::node()[              "
    "     contains(@Condition,$platform)        "
    " ]                                         "
    " /descendant-or-self::node()[              "
    "     name()='IntDir'                       "
    " ]                                         ";

char const* out_dir =
    " /descendant-or-self::node()[              "
    "     contains(@Condition,$platform)        "
    " ]                                         "
    " /descendant-or-self::node()[              "
    "     name()='OutDir'                       "
    " ]                                         ";

char const* project_name = "//ProjectName";
char const* project_guid = "//ProjectGuid";

char const* target_name =
    " /descendant-or-self::node()[              "
    "     contains(@Condition,$platform)        "
    " ]                                         "
    " /descendant-or-self::node()[              "
    "     name()='TargetName'                   "
    " ]                                         ";

char const* target_ext =
    " /descendant-or-self::node()[              "
    "     contains(@Condition,$platform)        "
    " ]                                         "
    " /descendant-or-self::node()[              "
    "     name()='TargetExt'                    "
    " ]                                         ";

}

vector<fs::path> cl_compiles( pugi::xml_document const& doc ) {
    auto res( xml::attr(
                    doc, xpaths::cl_compiles, {}, false ) );
    return util::to_paths( res );
}

vector<fs::path> cl_includes( pugi::xml_document const& doc ) {
    auto res( xml::attr(
                    doc, xpaths::cl_includes, {}, false ) );
    return util::to_paths( res );
}

vector<fs::path> search_paths( pugi::xml_document const& doc,
                               string_view               platform ) {
    xml::XPathVars vars{ { "platform", string( platform ) } };
    // For a given platform, we must have precisely one resultant
    // (possibly  semicolon  separated)  list  of  search   paths.
    auto path = xml::text(
            doc, xpaths::search_paths, vars, true, true );
    ASSERT( path, "failed to find precisely one value for "
                  "search paths for the " << platform << " "
                  "platform.  There may be zero, or too many." );
    auto res_win = util::split_strip( *path, ';' );
    vector<fs::path> res( res_win.size() );
    transform( begin( res_win ), end( res_win ), begin( res ),
               util::to_path );
    return move( res );
}

fs::path int_dir( pugi::xml_document const& doc,
                  string_view               platform ) {
    xml::XPathVars vars{ { "platform", string( platform ) } };
    auto dir( xml::text(
                doc, xpaths::int_dir, vars, false, true ) );
    ASSERT( dir, "failed to find precisely one value for "
                 "IntDir for the " << platform << " platform. "
                 "There may be zero, or too many." );
    return fs::path( *dir );
}

fs::path out_dir( pugi::xml_document const& doc,
                  string_view               platform ) {
    xml::XPathVars vars{ { "platform", string( platform ) } };
    auto dir( xml::text(
                doc, xpaths::out_dir, vars, false, true ) );
    ASSERT( dir, "failed to find precisely one value for "
                 "OutDir for the " << platform << " platform. "
                 "There may be zero, or too many." );
    return fs::path( *dir );
}

string project_name( pugi::xml_document const& doc ) {
    auto name( xml::text(
                doc, xpaths::project_name, {}, false, true ) );
    ASSERT( name, "failed to find precisely one value for "
                  "ProjectName. There may be zero, or too many." );
    return fs::path( *name );
}

optional<string> target_name( pugi::xml_document const& doc,
                              string_view               platform ) {
    xml::XPathVars vars{ { "platform", string( platform ) } };
    return xml::text(
            doc, xpaths::target_name, vars, true, true );
}

optional<string> target_ext( pugi::xml_document const& doc,
                             string_view               platform ) {
    xml::XPathVars vars{ { "platform", string( platform ) } };
    return xml::text(
                doc, xpaths::target_ext, vars, true, true );
}

string uuid( pugi::xml_document const& doc ) {
    auto res( xml::text(
                doc, xpaths::project_guid, {}, false, true ) );
    ASSERT( res, "failed to find precisely one value for "
                 "ProjectGuid. There may be zero, or too many." );
    string_view sv( *res );
    ASSERT( sv[0] == '{' && sv[sv.size()-1] == '}',
                 "failed to find uuid surrounded in { }." );
    // Remove { } from around the UUID.
    return string( sv.substr( 1, sv.size()-2 ) );
}

Project::Project( vector<fs::path>&& cl_includes,
                  vector<fs::path>&& cl_compiles,
                  vector<fs::path>&& search_paths,
                  fs::path&&         int_dir,
                  fs::path&&         out_dir,
                  string&&           project_name,
                  optional<string>&& target_name,
                  optional<string>&& target_ext,
                  string&&           uuid )

  : cl_includes  ( move( cl_includes  ) ),
    cl_compiles  ( move( cl_compiles  ) ),
    search_paths ( move( search_paths ) ),
    int_dir      ( move( int_dir      ) ),
    out_dir      ( move( out_dir      ) ),
    project_name ( move( project_name ) ),
    target_name  ( move( target_name  ) ),
    target_ext   ( move( target_ext   ) ),
    uuid         ( move( uuid         ) )
{ }

auto read( fs::path file, string_view platform ) -> Project {

    pugi::xml_document doc;

    // path may be relative to bin folder
    xml::parse( doc, file );

    return Project(
        cl_includes  ( doc           ),
        cl_compiles  ( doc           ),
        search_paths ( doc, platform ),
        int_dir      ( doc, platform ),
        out_dir      ( doc, platform ),
        project_name ( doc           ),
        target_name  ( doc, platform ),
        target_ext   ( doc, platform ),
        uuid         ( doc           )
    );
}

string Project::to_string() const {

    ostringstream oss;

    auto print = [&oss]( auto const& s ) {
            oss << "  | \"" << util::to_string( s ) << "\"" << endl;
    };

    auto print_list = [&]( auto const& v ) {
        for( auto const& s : v )
            print( s );
    };

    oss << "AdditionaIncludeDirectories: " << endl;
    print_list( search_paths );
    oss << "ClCompile: " << endl;
    print_list( cl_compiles );
    oss << "ClInclude: " << endl;
    print_list( cl_includes );
    oss << "IntDir: " << endl;
    print( string( int_dir ) );
    oss << "OutDir: " << endl;
    print( string( out_dir ) );
    oss << "ProjectName: " << endl;
    print( project_name );
    oss << "TargetName: " << endl;
    print( target_name );
    oss << "TargetExt: " << endl;
    print( target_ext );
    oss << "UUID: " << endl;
    print( uuid );

    return oss.str();
}

ostream& operator<<( ostream& out, Project const& p ) {
    return (out << p.to_string());
}

} // namespace project
