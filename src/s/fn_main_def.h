
// -i db.tsv
// -o <output-dir>

int main(int argc, char* argv[]) {
    google::InitGoogleLogging(argv[0]);
    // Read args.
    auto args = ReadArgs(argc, argv);
    fmt::print("-i: {}\n", args.i);
    fmt::print("-o: {}\n", args.o);
    // Read db.
    // Generate source files.
    return EXIT_SUCCESS;
}
