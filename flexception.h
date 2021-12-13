#pragma once

#include <sstream>
#include <stdexcept>
#include <string>

// http://stackoverflow.com/questions/348833/how-to-know-the-exact-line-of-code-where-where-an-exception-has-been-caused
class fl_exception : public std::runtime_error {
    std::string msg;

    public:
    fl_exception(const std::string &arg, const char *file, int line) : std::runtime_error(arg) {
        std::ostringstream o;
        o << file << ":" << line << ": " << arg;
        msg = o.str();
    }
    ~fl_exception() throw() {
    }
    const char *what() const throw() {
        return msg.c_str();
    }
};

#define Error(arg) throw fl_exception(arg, __FILE__, __LINE__);
