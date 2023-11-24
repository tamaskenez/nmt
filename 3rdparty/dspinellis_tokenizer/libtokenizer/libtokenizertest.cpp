#include "libtokenizer.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>

int main(int argc, char* argv[]) {
    if (argc == 2) {
        namespace fs = std::filesystem;
        auto size = fs::file_size(fs::path(argv[1]));
        std::ifstream f(argv[1]);
        std::vector<char> v(size);
        f.read(v.data(), size);
        auto x = libtokenizer::process_cpp_with_option_B(std::string_view(v.data(), v.size()));
        printf("Done\n");
    } else {
        auto x = libtokenizer::process_cpp_with_option_B(
            "int a = \t123 * 42; // what\n\n\nnextLine(\"some string\\n\");");
        printf("Done\n");
    }
    return EXIT_SUCCESS;
}
