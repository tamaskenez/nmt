// #fn
int main(int argc, char* argv[]) {
    auto ui = QtUI_make(argc, argv);
    auto app = NmtApp::make(argc, argv);

    return app->run(std::move(ui));
}
// #defneeds: "QtUI.h", "NmtApp.h", <utility>, <cassert>
