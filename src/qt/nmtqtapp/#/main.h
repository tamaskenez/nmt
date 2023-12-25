// #fn
int main(int argc, char* argv[]) {
    auto ui = makeQtUI(argc, argv);
    auto appOr = NmtApp::make(argc, argv);
    if (!appOr) {
        for (auto& m : appOr.error()) {
            fmt::print(stderr, "Error: {}\n", m);
        }
        return EXIT_FAILURE;
    }
    auto& app = *appOr;
    return app->run(std::move(ui));
}
// #defneeds: "QtUI.h", "NmtApp.h", <utility>, <cassert>, <fmt/core.h>
