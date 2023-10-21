int main(int argc, char* argv[])
// needs: "QtUI.h", "NmtApp.h", <utility>, <cassert>
{
    auto ui = QtUI_make(argc, argv);
    auto app = NmtApp::make(argc, argv);

    return app->run(std::move(ui));
}
