/****************************************************************
* Project & Solution File Parsers
****************************************************************/
#include "fs.hpp"
#include "macros.hpp"
#include "opt-util.hpp"
#include "project_raw.hpp"
#include "string-util.hpp"
#include "xml-utils.hpp"

#include <algorithm>
#include <sstream>

using namespace std;

namespace project {

namespace impl {

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

vector<string> cl_compiles( pugi::xml_document const& doc ) {
    return xml::attr( doc, xpaths::cl_compiles, {}, false );
}

vector<string> cl_includes( pugi::xml_document const& doc ) {
    return xml::attr( doc, xpaths::cl_includes, {}, false );
}

vector<string> search_paths( pugi::xml_document const& doc,
                             string_view               platform ) {
    xml::XPathVars vars{ { "platform", string( platform ) } };
    // For a given platform, we must have precisely one resultant
    // (possibly  semicolon  separated)  list  of  search   paths.
    auto path = xml::text(
            doc, xpaths::search_paths, vars, true, true );
    if( !path )
        return {};
    auto res = util::split_strip( *path, ';' );
    return util::to_strings( res );
}

string int_dir( pugi::xml_document const& doc,
                string_view               platform ) {
    xml::XPathVars vars{ { "platform", string( platform ) } };
    auto dir = xml::text(
                   doc, xpaths::int_dir, vars, false, true );
    ASSERT( dir, "failed to find precisely one value for "
                 "IntDir for the " << platform << " platform. "
                 "There may be zero, or too many." );
    return *dir;
}

string out_dir( pugi::xml_document const& doc,
                string_view               platform ) {
    xml::XPathVars vars{ { "platform", string( platform ) } };
    auto dir = xml::text(
                   doc, xpaths::out_dir, vars, false, true );
    ASSERT( dir, "failed to find precisely one value for "
                 "OutDir for the " << platform << " platform. "
                 "There may be zero, or too many." );
    return *dir;
}

string project_name( pugi::xml_document const& doc ) {
    auto name = xml::text(
                    doc, xpaths::project_name, {}, false, true );
    ASSERT( name, "failed to find precisely one value for "
                  "ProjectName. There may be zero, or too many." );
    return move( *name );
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

} // impl

ProjectRaw::ProjectRaw( ProjectAttributes&& pa )
  : attr( move( pa ) )
{ }

ProjectRaw ProjectRaw::read( fs::path const& file,
                             string_view     platform ) {

    pugi::xml_document doc;

    // path may be relative to bin folder
    xml::parse( doc, file );

    auto fwd     = L( fs::path( util::fwd_slashes( _ ) ) );
    auto fwd_vec = L( util::to_paths( util::fwd_slashes( _ ) ) );
    auto is_var  = L( util::contains( _, "%(" ) );

    auto search_paths = impl::search_paths( doc, platform );
    util::remove_if( search_paths, is_var );

    return ProjectRaw( {
        move( fwd_vec( impl::cl_includes ( doc           ) ) ),
        move( fwd_vec( impl::cl_compiles ( doc           ) ) ),
        move( fwd_vec( search_paths                        ) ),
        move(     fwd( impl::int_dir     ( doc, platform ) ) ),
        move(     fwd( impl::out_dir     ( doc, platform ) ) ),
        move(          impl::project_name( doc           ) ),
        move(          impl::target_name ( doc, platform ) ),
        move(          impl::target_ext  ( doc, platform ) ),
        move(          impl::uuid        ( doc           ) )
    } );
}

string ProjectRaw::to_string() const {

    ostringstream oss;

    auto print = [&]( auto const& s ) {
            oss << "  | \"" << util::to_string( s )
                << "\"" << endl;
    };

    auto print_path = [&]( auto const& p ) {
            oss << "  | " << p << endl;
    };

    auto print_path_list = [&]( auto const& v ) {
        for( auto const& s : v )
            print_path( s );
    };

    oss << "AdditionaIncludeDirectories: " << endl;
    print_path_list( attr.search_paths );
    oss << "ClCompile: " << endl;
    print_path_list( attr.cl_compiles );
    oss << "ClInclude: " << endl;
    print_path_list( attr.cl_includes );
    oss << "IntDir: " << endl;
    print_path( attr.int_dir );
    oss << "OutDir: " << endl;
    print_path( attr.out_dir );
    oss << "ProjectName: " << endl;
    print( attr.project_name );
    oss << "TargetName: " << endl;
    print( attr.target_name );
    oss << "TargetExt: " << endl;
    print( attr.target_ext );
    oss << "UUID: " << endl;
    print( attr.uuid );
    oss << "tlog name: " << endl;
    print( tlog_name() );

    return oss.str();
}

string ProjectRaw::tlog_name() const {
    string res;
    if( attr.project_name.size() <= 16 )
        res = attr.project_name;
    else
        res = attr.project_name.substr( 0, 8 ) + "." +
              attr.uuid.substr( 0, 8 );
    return res + ".tlog";
}

ostream& operator<<( ostream& out, ProjectRaw const& p ) {
    return (out << p.to_string());
}

} // namespace project
