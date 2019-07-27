#include "../include/cpprintf.hpp"
#include<variant>
#include "../include/Swan.hpp"
#include "../include/SwanBinding.hpp"
#include<boost/filesystem.hpp>
using namespace std;
namespace fs = boost::filesystem;

typedef variant<string,fs::path> stringOrPath;

struct FS {};

static fs::path toPath (const stringOrPath& a) {
struct ToPath {
fs::path operator() (const string& s) { return fs::path(s).lexically_normal().make_preferred(); }
fs::path operator() (const fs::path& p) { return p.lexically_normal().make_preferred(); }
};
return visit(ToPath(), a);
}

static fs::path pathAppend (const fs::path& p, const stringOrPath& a) {
struct Appender {
const fs::path& base;
Appender(const fs::path& b): base(b) {}
fs::path operator() (const fs::path& p) { return (fs::path(base)/=p).lexically_normal(); }
fs::path operator() (const string& p) { return (fs::path(base)/=p).lexically_normal(); }
};
return visit(Appender(p), a);
}

static fs::path pathConcat (const fs::path& p, const stringOrPath& a) {
struct Concatener {
const fs::path& base;
Concatener  (const fs::path& b): base(b) {}
fs::path operator() (const fs::path& p) { return (fs::path(base)+=p).lexically_normal(); }
fs::path operator() (const string& p) { return (fs::path(base)+=p).lexically_normal(); }
};
return visit(Concatener(p), a);
}

static fs::path pathRelativize (const fs::path& path, const stringOrPath& base) {
return path.lexically_relative(toPath(base)).lexically_normal().make_preferred();
}

static fs::path pathAbsolutize (const fs::path& path, const stringOrPath& base) {
return fs::absolute(path, toPath(base)).lexically_normal().make_preferred();
}

static string pathToString (const fs::path& path) {
return path.string<string>();
}

static long long pathFileSize (const fs::path& path) {
return fs::file_size(path);
}

static bool pathExists (const fs::path& path) {
return fs::exists(path);
}

static inline bool readpath (Swan::Fiber& f, int idx, fs::path& path) {
if (f.isString(idx)) path = fs::path(f.getString(idx));
else if (f.isUserObject<fs::path>(idx)) path = f.getUserObject<fs::path>(idx);
else return false;
return true;
}

static void pathCopy (Swan::Fiber& f) {
fs::path src, dst;
if (!readpath(f, 0, src) || !readpath(f, 1, dst)) return;
fs::copy_option opt = f.getOptionalBool(2, "overwrite", false)? fs::copy_option::overwrite_if_exists : fs::copy_option::fail_if_exists;
fs::copy_file(src, dst, opt);
}

static void pathDelete (Swan::Fiber& f) {
fs::path path;
if (!readpath(f, 0, path)) { f.setUndefined(0); return; }
bool recurse = f.getOptionalBool("recurse", false);
if (recurse) f.setNum(0, fs::remove_all(path));
else f.setBool(0, fs::remove(path));
}

static void pathRename (const fs::path& from, const stringOrPath& to) {
fs::rename(from, toPath(to));
}

static fs::path pathGetCurrent () {
return fs::current_path();
}

static fs::path pathSetCurrent (const stringOrPath& s) {
fs::current_path(toPath(s));
return fs::current_path();
}

static bool pathMkdirs (const fs::path& path) {
return fs::create_directories(path);
}

static bool pathIsFile (const fs::path& path) {
return fs::is_regular_file(path);
}

static bool pathIsDir (const fs::path& path) {
return fs::is_directory(path);
}

static fs::directory_iterator diritCreate (const stringOrPath& s) {
return fs::directory_iterator(toPath(s));
}

static void diritNext (Swan::Fiber& f) {
fs::directory_iterator& dirit = f.getUserObject<fs::directory_iterator>(0);
if (dirit==fs::directory_iterator()) f.setUndefined(0);
else {
f.setUserObject<fs::path>(0, dirit->path() );
++dirit;
}}

static fs::recursive_directory_iterator recdiritCreate (const stringOrPath& s) {
return fs::recursive_directory_iterator(toPath(s));
}

static void recdiritNext (Swan::Fiber& f) {
fs::recursive_directory_iterator& dirit = f.getUserObject<fs::recursive_directory_iterator>(0);
if (dirit==fs::recursive_directory_iterator()) f.setUndefined(0);
else {
f.setUserObject<fs::path>(0, dirit->path() );
++dirit;
}}

void swanLoadFS (Swan::Fiber& f) {
f.pushNewMap();

f.pushNewClass<fs::path>("Path");
f.registerStaticMethod("()", STATIC_METHOD(toPath));
f.registerStaticProperty("current", STATIC_METHOD(pathGetCurrent), STATIC_METHOD(pathSetCurrent));
f.registerMethod("toString", FUNCTION(pathToString));
f.registerMethod("parent", METHOD(fs::path, parent_path));
f.registerMethod("name", METHOD(fs::path, filename));
f.registerMethod("stem", METHOD(fs::path, stem));
f.registerMethod("extension", METHOD(fs::path, extension));
f.registerMethod("/", FUNCTION(pathAppend));
f.registerMethod("+", FUNCTION(pathConcat));
f.registerMethod("rel", FUNCTION(pathRelativize));
f.registerMethod("abs", FUNCTION(pathAbsolutize));
f.registerMethod("length", FUNCTION(pathFileSize));
f.registerMethod("exists", FUNCTION(pathExists));
f.registerMethod("copy", pathCopy);
f.registerMethod("mkdirs", FUNCTION(pathMkdirs));
f.registerMethod("delete", pathDelete);
f.registerMethod("rename", FUNCTION(pathRename));
f.registerMethod("isFile", FUNCTION(pathIsFile));
f.registerMethod("isDirectory", FUNCTION(pathIsDir));
f.storeIndex(-2, "Path");
f.pop();

f.loadGlobal("Iterable");
f.pushNewClass<fs::directory_iterator>("DirectoryIterator", 1);
f.registerStaticMethod("()", STATIC_METHOD(diritCreate));
f.registerMethod("next", diritNext);
f.storeIndex(-2, "DirectoryIterator");
f.pop();

f.loadGlobal("Iterable");
f.pushNewClass<fs::recursive_directory_iterator>("RecursiveDirectoryIterator", 1);
f.registerStaticMethod("()", STATIC_METHOD(recdiritCreate));
f.registerMethod("next", recdiritNext);
f.storeIndex(-2, "RecursiveDirectoryIterator");
f.pop();
}

