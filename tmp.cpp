// kind: fn|enum|struct|class|memfn
// namespace:
// subdir:
// visibility: public, target, subdir,

namespace Entity {

struct Base {
    std::string name;
    std::vector<std::string> needs;
    std::filesystem::path path;
};

struct Function : Base {
    std::string functionDeclaration;  // Fully defined.
    std::vector<std::string> defNeeds;
    bool inline_;
};

struct Enum : Base {
    std::vector<std::string> oedNeeds;  // For the opaqueEnumDeclaration.
    std::string opaqueEnumDeclaration;  // To use without referring to the values.
};

struct StructClass : Base {
    std::vector<std::string> fdNeeds;  // Forward-declaration needs.
    std::string forwardDeclaration;    // To use as a pointer.
};
using V = std::variant<Function, Enum, StructClass>;
}  // namespace Entity

struct Target {
    std::filesystem::path sourceRoot;       // Input source files
    std::filesystem::path boilerplateRoot;  // For generated files
    std::unordered_map<std::string, Entity> entities;
};

// #fdneeds:
// #oedneeds:
// #needs:
// #defneeds:
// #namespace:
// #visibility: public, target

class X {
   public:
   private:
#include NMT_GENERATED_DECLARATIONS
};

class X;
enum A;
class X {};
void f();
enum A {..} extern int x;

void f() {
    ..
}
int x = ...;