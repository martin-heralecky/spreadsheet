#ifndef SPREADSHEET_EXCEPTION_H
#define SPREADSHEET_EXCEPTION_H

#include <exception>
#include <string>

using namespace std;

class Exception : exception
{
protected:
    string message;

public:
    explicit Exception(const string &message = "")
        : message(message)
    {}
};

#endif /* SPREADSHEET_EXCEPTION_H */
