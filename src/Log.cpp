#include "Log.hpp"

const char *Red    = "\e[1;31m";
const char *Green  = "\e[1;32m";
const char *Yellow = "\e[1;33m";
const char *White  = "\e[0m";

// const char *Red    = "";
// const char *Green  = "";
// const char *Yellow = "";
// const char *White  = "";

namespace Log {
    void Info(const char *fstr, ...) {
        va_list args;
        va_start(args, fstr);
        printf("%s", Green);
        vprintf(fstr, args);
        printf("%s", White);
        va_end(args);
    }

    void Warn(const char *fstr, ...) {
        va_list args;
        va_start(args, fstr);
        printf("%s", Yellow);
        vprintf(fstr, args);
        printf("%s", White);
        va_end(args);
    }

    void Error(const char *fstr, ...) {
        va_list args;
        va_start(args, fstr);
        printf("%s", Red);
        vprintf(fstr, args);
        printf("%s", White);
        va_end(args);
    }
}