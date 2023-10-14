// needs: <filesystem>
bool fs_exists_total(const std::filesystem::path& p) {
    std::error_code ec;
    return std::filesystem::exists(p, ec);
}
