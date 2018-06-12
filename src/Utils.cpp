#include "Utils.h"

#include <algorithm>
#include <cmath>
#include <sstream>

#include "exception/InvalidInputException.h"

using namespace std;

string Utils::strPadLeft(const string &str, size_t len, char c)
{
    if (str.length() >= len) {
        return str;
    }

    return string(len - str.length(), c) + str;
}

string Utils::strPadRight(const string &str, size_t len, char c)
{
    if (str.length() >= len) {
        return str;
    }

    return str + string(len - str.length(), c);
}

string Utils::strPadCenter(const string &str, size_t len, char c)
{
    if (str.length() >= len) {
        return str;
    }

    return string(ceil((len - str.length()) / 2.0), c) +
        str +
        string(floor((len - str.length()) / 2.0), c);
}

string Utils::trim(const string &str, const string &trim_chars)
{
    size_t begin = str.find_first_not_of(trim_chars);
    size_t end = str.find_last_not_of(trim_chars);

    if (begin == string::npos) {
        return "";
    }

    return str.substr(begin, end - begin + 1);
}

string Utils::trimRight(const string &str, const string &trim_chars)
{
    size_t end = str.find_last_not_of(trim_chars);

    if (end == string::npos) {
        return "";
    }

    return str.substr(0, end + 1);
}

string Utils::toLower(const string &str)
{
    string s(str);

    transform(s.begin(), s.end(), s.begin(), ::tolower);

    return s;
}

string Utils::escapeString(const string &str)
{
    ostringstream oss;

    for (char c : str) {
        switch (c) {
            case '"':
                oss << "\\\"";
                break;
            case '\\':
                oss << "\\\\";
                break;
            default:
                oss << c;
        }
    }

    return oss.str();
}

string Utils::unescapeString(const string &str)
{
    istringstream iss(str);
    ostringstream res;
    char c;

    iss >> noskipws;

    iss >> c;
    while (!iss.eof()) {
        if (c == '\\') {
            iss >> c;

            if (iss.eof())
                throw InvalidInputException();
        }

        res << c;

        iss >> c;
    }

    return res.str();
}

string Utils::readString(istream &is)
{
    ostringstream oss;
    char c;

    is >> noskipws;
    while (!is.eof()) {
        is >> c;

        if (c == '\\') {
            oss << c;
            is >> c;
        } else if (c == '"') {
            return oss.str();
        }

        oss << c;
    }

    throw InvalidInputException();
}

void Utils::assertInput(istream &is, char input)
{
    char c;

    is >> c;
    if (is.eof() || c != input)
        throw InvalidInputException();
}
