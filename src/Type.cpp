#include "Type.h"

#include "Utils.h"

#include <regex>

using namespace std;

template<>
const string Type<int>::name = "int";

template<>
const string Type<double>::name = "double";

template<>
const string Type<string>::name = "string";

template<>
const int Type<int>::defaultValue = 0;

template<>
const double Type<double>::defaultValue = 0;

template<>
const string Type<string>::defaultValue = "";

template<>
string Type<int>::toString(const int &val, bool isLiteral)
{
    return to_string(val);
}

template<>
string Type<double>::toString(const double &val, bool isLiteral)
{
    return to_string(val);
}

template<>
string Type<string>::toString(const string &val, bool isLiteral)
{
    if (isLiteral) {
        return string("\"") + Utils::escapeString(val) + "\"";
    }

    return string(val);
}

/**
 * Converts given string to integer. Whitespace returns 0.
 *
 * @param isLiteral unused
 */
template<>
int Type<int>::fromString(const string &val, bool isLiteral)
{
    if (regex_match(val, regex("^\\s*$"))) {
        return 0;
    }

    try {
        return stoi(val);
    } catch (...) {
        throw InvalidTypeException();
    }
}

/**
 * Converts given string to double. Whitespace returns 0.
 *
 * @param isLiteral unused
 */
template<>
double Type<double>::fromString(const string &val, bool isLiteral)
{
    if (regex_match(val, regex("^\\s*$"))) {
        return 0.0;
    }

    try {
        return stod(val);
    } catch (...) {
        throw InvalidTypeException();
    }
}

template<>
string Type<string>::fromString(const string &val, bool isLiteral)
{
    if (isLiteral) {
        if (val.length() < 2 || val[0] != '"' || val[val.length() - 1] != '"') {
            throw InvalidTypeException();
        }

        return Utils::unescapeString(val.substr(1, val.length() - 2));
    }

    return string(val);
}
