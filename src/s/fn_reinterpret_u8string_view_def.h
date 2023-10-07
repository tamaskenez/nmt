std::u8string_view reinterpret_u8string_view(std::string_view sv) {
    return std::u8string_view(reinterpret_cast<const char8_t*>(sv.data()), sv.size());
}
