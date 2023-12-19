// #memfn
STATIC std::unique_ptr<NmtApp> NmtApp::make(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    return std::make_unique<NmtAppImpl>();
}
// #needs: <memory>
// #defneeds: NmtAppImpl
// #visibility: public
