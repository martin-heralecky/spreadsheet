#ifndef SPREADSHEET_TYPE_H
#define SPREADSHEET_TYPE_H

#include <string>

#include "exception/InvalidTypeException.h"

using namespace std;

/**
 * Provides additional features to type T.
 */
template<typename T>
class Type
{
public:
    /**
     * Name of type T.
     */
    static const string name;

    /**
     * Default value of type T.
     */
    static const T defaultValue;

    /**
     * Converts T to string.
     *
     * @param isLiteral Whether the given value should be treated as literal (i.e. surround string
     *                  with double quotes if T is string)
     *
     * @throws InvalidTypeException
     */
    static string toString(const T &val, bool isLiteral = false)
    {
        throw InvalidTypeException();
    }

    /**
     * Converts given string to type T.
     *
     * @param isLiteral Whether the given string should be treated as literal (i.e. strip
     *                  surrounding double quotes if T is string)
     *
     * @throws InvalidTypeException
     */
    static T fromString(const string &val, bool isLiteral = false)
    {
        throw InvalidTypeException();
    }
};

#endif /* SPREADSHEET_TYPE_H */
