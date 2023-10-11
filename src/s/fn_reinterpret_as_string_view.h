// needs: <string_view>
std::string_view reinterpret_as_string_view(std::u8string_view sv) {
    return std::string_view(reinterpret_cast<const char*>(sv.data()), sv.size());
}
