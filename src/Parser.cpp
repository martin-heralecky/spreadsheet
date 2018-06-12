#include "Sheet.h"

#include <regex>

#include "exception/InvalidInputException.h"

using namespace std;

namespace Formula
{
    bool Parser::isLink(const string &source)
    {
        return regex_match(source, regex("^[a-zA-Z]+[1-9][0-9]*$"));
    }

    template<>
    bool Parser::isLiteral<int>(const string &source)
    {
        return regex_match(source, regex("^[0-9]+$"));
    }

    template<>
    bool Parser::isLiteral<double>(const string &source)
    {
        /* int to double implicit conversion */
        return regex_match(source, regex("^[0-9]*\\.[0-9]+$")) || isLiteral<int>(source);
    }

    template<>
    bool Parser::isLiteral<string>(const string &source)
    {
        return regex_match(source, regex("^\"([^\"\\\\]|\\\\.)*\"$"));
    }

    vector<string> Parser::splitLogical(const string &source, const string &separators)
    {
        vector<string> res;
        istringstream source_iss(source);
        char c;

        string cur = "";

        auto pushCur = [&]() {
            if (cur.size() >= 2 && cur[0] == '(' && cur[cur.size() - 1] == ')') {
                cur = cur.substr(1, cur.size() - 2);
            }

            if (cur.size() > 0) {
                res.push_back(cur);
                cur.clear();
            }
        };

        int logicalLevel = 0; /* how deep in nested parentheses we are */

        source_iss >> c;
        while (!source_iss.eof()) {
            if (separators.find(c) != string::npos && logicalLevel == 0) {
                /* we've found separator and we're at root level */
                pushCur();
                res.push_back(string(1, c));

                source_iss >> c;
                continue;
            } else if (c == '"') {
                if (!cur.empty()) {
                    throw IncorrectFormulaSyntaxException();
                }

                try {
                    cur = string(1, '"') + Utils::readString(source_iss) + '"';
                    source_iss >> noskipws;
                    pushCur();

                    source_iss >> c;
                    continue;
                } catch (const InvalidInputException &ex) {
                    throw IncorrectFormulaSyntaxException();
                }
            } else if (c == '(') {
                ++logicalLevel;
            } else if (c == ')') {
                --logicalLevel;
            }

            cur += c;
            source_iss >> c;
        }

        pushCur();

        return res;
    }
}
