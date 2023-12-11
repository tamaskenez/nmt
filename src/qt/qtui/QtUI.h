#pragma once

#include "UI.h"

#include <memory>

std::unique_ptr<UI> QtUI_make(int& argc, char* argv[]);
