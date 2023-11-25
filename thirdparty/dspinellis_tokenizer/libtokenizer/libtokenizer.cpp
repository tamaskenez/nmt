#include "libtokenizer.h"

#include "injector.h"

#include "../tokenizer/src/CppTokenizer.h"

#include <sstream>
#include <strstream>

// TODO in CharSource.h this line: `} while (c < 0 || c > 127);` should be patched
// to include non-ASCII characters, otherwise non-ASCII characters offset the character counting.
namespace libtokenizer {
class CppTokenizer : public ::CppTokenizer {
   public:
    CppTokenizer(CharSource& s, std::vector<std::string> opt = {})
        : ::CppTokenizer(s, "<N/A>", opt) {}
    int get_src_nchar() const {
        return src.get_nchar();
    }
};
std::expected<Result, std::string> process_cpp_with_option_B(std::string_view in_sv) {
    Result result;
    try {
        const bool all_contents = false;
        const bool symbolic_output = false;
        const bool compress_ids = false;
        const bool show_file_name = false;
        const std::vector<std::string> processing_opt;
        const char separator = 0;

        std::strstreambuf in_ssbuf(in_sv.data(), in_sv.size());
        std::istream in(&in_ssbuf);
        CharSource cs(
            in);  // TokenBase copies, does not hold reference to this, but who knows, let's
        // leave this here instead of passing it in as rvalue.
        auto t = std::make_unique<CppTokenizer>(cs, processing_opt);

        t->set_separator(separator ? separator : ' ');
        t->set_all_contents(all_contents);

        injector::cout.StartCapture(
            in_sv,
            [&t]() -> int {
                return t->get_src_nchar();
            },
            &result);
        t->type_code_tokenize();
    } catch (std::exception& e) {
        return std::unexpected(e.what());
    }
    return result;
}
}  // namespace libtokenizer
