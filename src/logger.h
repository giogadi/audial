#pragma once

#include <cstdio>
#include <cstdarg>

namespace logger {

    struct Logger {
        Logger();
        ~Logger();
        FILE* _outputFile = nullptr;
    };

    void Log(Logger& logger, char const* fmt, ...) {
        {
            va_list args;
            va_start(args, fmt);
            vfprintf(logger._outputFile, fmt, args);
            va_end(args);
        }

        {
            va_list args;
            va_start(args, fmt);
            vprintf(fmt, args);
            va_end(args);
        }
    }

    Logger::Logger() {
        _outputFile = fopen("log.txt", "w");
    }

    Logger::~Logger() {
        if (_outputFile != nullptr) {
            fclose(_outputFile);
        }
    }

};