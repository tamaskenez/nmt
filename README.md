# NMT - No More Tears with #include boilerplate

MVP in progress of a supposedly easy to use C++ preprocessor + GUI app with the following benefits:

- No need to manually manage #include files
- Smoother, "one-click" developer experience: no need to deal with files (cpp/h), C++ languages entities are created "into the project", no need to make decisions about group and organizing them into source headers
- No need to duplicate function declarations (cpp + h)
- Faster builds during development - because we're rebuilding only what has been modified, in the strictest sense: development builds compile each atomic C++ language entity in a separate file
- Faster builds in CI - because the separate files can automatically be combined into unity builds, since dependency relations between C++ language entities are always known and enforced during development
- More reasonable compiler error messages during development since leaves in the dependency graphs are compiled first, in isolation. Unlike traditional builds where compiling a cpp file might fail because of errors in their headers and the interaction between them.
- Full compatibility with existing projects: they can be gradually converted or extended with nmt-style modules

The traditional way C++ projects and IDEs work is to store the project in raw C++ files which need to be preprocessed and compiled as "translation units" again and again, to extract information about the program, for example dependencies between language entities (functions, types).

The alternative would be storing the C++ project as a database of the language entities with dependency relations between entities enforced and maintained throughout the development cycle. That means, for example, that IDE knows about each function, what other entities the function declaration and definition need to compile them.

This project is a POC experiment, trying to achieve many of the benefits of the ideal solution (database of C++ entities stored in AST form with a super-helpful IDE) with minimal effort. It looks like this:

- We store (almost) each C++ language entity in a separate .h file. Each .h file describes (in most cases) exactly one language entity. These files don't contain an `#include` directives and namespace definitions. Instead, the dependencies, namespaces, visibility should be specified in with special keywords in C++ comments, like: `// #needs: <string>, <vector>, "foo.h", SomeClass, BarFactory` and `// #visibility: public` and `// #namespace: foo::bar`
- A GUI app helps creating/managing the language entities and writing the comments describing dependencies, namespaces and visibilty.
- As a pre-build step added by a simple CMake function the preprocessor creates the boilerplate and cpp files and adds them to the CMake target.

# Targets

- nmtlib: base library for parsing sources and generating boilerplate
- nmt: command line interface for nmtlib

- 