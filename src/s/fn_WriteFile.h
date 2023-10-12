// needs: <filesystem>, <string_view>
[[nodiscard]] bool WriteFile(const std::filesystem::path& p, std::string_view content)
// needs: <fstream>
{
    std::ofstream f(p);
    if (!f.is_open()) {
        return false;
    }
    f.write(content.data(), std::streamsize(content.size()));
    if (!f.good()) {
        return false;
    }
    f.flush();
    return f.good();
}
