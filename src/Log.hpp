#pragma once

#include "Common.hpp"

namespace Log {
    void Info(const char *fstr, ...);
    void Warn(const char *fstr, ...);
    void Error(const char *fstr, ...);
}