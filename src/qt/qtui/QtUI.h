#pragma once

#include "appcommon/UI.h"

#include <memory>

std::unique_ptr<UI> makeQtUI(int& argc, char* argv[]);
