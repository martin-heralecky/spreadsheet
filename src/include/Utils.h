#ifndef SPREADSHEET_UTILS_H
#define SPREADSHEET_UTILS_H

#include <istream>
#include <string>

using namespace std;

class Utils
{
public:
    /**
     * Generates a new string, padded-left with the specified character.
     */
    static string strPadLeft(const string &str, size_t len, char c = ' ');

    /**
     * Generates a new string, padded-right with the specified character.
     */
    static string strPadRight(const string &str, size_t len, char c = ' ');

    /**
     * Generates a new string, padded-around with the specified character.
     */
    static string strPadCenter(const string &str, size_t len, char c = ' ');

    /**
     * Creates a new string trimmed on both sides of specified characters.
     */
    static string trim(const string &str, const string &trim_chars = " ");

    /**
     * Creates a new string right-trimmed of specified characters.
     */
    static string trimRight(const string &str, const string &trim_chars = " ");

    /**
     * Creates a new string, modifying all characters to lower case.
     */
    static string toLower(const string &str);

    /**
     * Escapes given string to be a JSON string.
     */
    static string escapeString(const string &str);

    /**
     * Decodes given JSON string.
     *
     * @param str Pure string, NOT surrounded by double quotes.
     *
     * @throws InvalidInputException If the last character is an escape character.
     */
    static string unescapeString(const string &str);

    /**
     * Reads given stream as a string that ends with unescaped double quotes (which are also
     * extracted from the stream but not returned).
     *
     * EXAMPLE:
     *     this is a \"large\" string with \\ backslash"and here is past its end
     *     ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
     *
     * @throws InvalidInputException If EOF before closing double quotes found.
     */
    static string readString(istream &is);

    /**
     * Reads given stream and throws exception if input doesn't match. Uses pre-set skipws flag.
     *
     * @throws InvalidInputException
     */
    static void assertInput(istream &is, char input);

    /**
     * Calls given function and checks for what it throws.
     *
     * @tparam T Type that should be thrown by the function.
     * @return True if the function throws object of type T. False otherwise.
     */
    template<typename T>
    static bool throws(void (*f)())
    {
        try {
            f();
        } catch (T ex) {
            return true;
        } catch (...) {
            return false;
        }

        return false;
    }
};

#endif /* SPREADSHEET_UTILS_H */
