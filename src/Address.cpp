#include "Address.h"

#include <cmath>
#include <algorithm>
#include <istream>

#include "Utils.h"

#include "exception/InvalidArgumentException.h"
#include "exception/InvalidInputException.h"

using namespace std;

const int Address::MAX_ROW;
const int Address::MAX_COL;

Address::Address(int col, int row)
    : m_Col(col),
      m_Row(row)
{
    if (col <= 0 || row <= 0) {
        throw InvalidArgumentException();
    }
}

Address::Address(const string &addr)
{
    size_t rowDigits_pos = addr.find_first_of("0123456789");
    if (rowDigits_pos < 1 || rowDigits_pos == string::npos) {
        throw InvalidArgumentException();
    }

    m_Col = 0;
    for (int i = rowDigits_pos - 1; i >= 0; --i) {
        char c = tolower(addr[i]);
        if (c < 'a' || c > 'z') {
            throw InvalidArgumentException();
        }

        double addition = (c - 'a' + 1) * pow(26, rowDigits_pos - i - 1);
        if (m_Col + addition > Address::MAX_COL) {
            throw InvalidArgumentException();
        }

        m_Col += addition;
    }

    double desiredRow = stoul(addr.substr(rowDigits_pos));
    if (desiredRow < 1 || desiredRow > Address::MAX_ROW) {
        throw InvalidArgumentException();
    }

    m_Row = desiredRow;
}

Address::Address(const char *addr)
    : Address(string(addr))
{}

int Address::col() const
{
    return m_Col;
}

int Address::row() const
{
    return m_Row;
}

string Address::colName() const
{
    return colName(col() - 1);
}

Address::operator string() const
{
    return colName(col() - 1) + to_string(row());
}

bool Address::operator==(const Address &rhs) const
{
    return m_Col == rhs.m_Col && m_Row == rhs.m_Row;
}

bool Address::operator!=(const Address &rhs) const
{
    return !(rhs == *this);
}

bool Address::operator<(const Address &rhs) const
{
    if (m_Col < rhs.m_Col)
        return true;

    if (rhs.m_Col < m_Col)
        return false;

    return m_Row < rhs.m_Row;
}

bool Address::operator>(const Address &rhs) const
{
    return rhs < *this;
}

bool Address::operator<=(const Address &rhs) const
{
    return !(rhs < *this);
}

bool Address::operator>=(const Address &rhs) const
{
    return !(*this < rhs);
}

string Address::colName(int col)
{
    if (col < 26) {
        return string(1, 'A' + col);
    }

    return colName(col / 26 - 1) + (char) ('A' + (col % 26));
}

Address Address::operator-(const Address &rhs) const
{
    return Address(m_Col - rhs.m_Col + 1, m_Row - rhs.m_Row + 1);
}

void Address::serialize(ostream &os) const
{
    os << '"' << (string) *this << '"';
    os.flush();
}

Address Address::deserialize(istream &is)
{
    string s;

    is >> skipws;
    Utils::assertInput(is, '"');
    is >> noskipws;

    getline(is, s, '"');

    if (is.eof())
        throw InvalidInputException();

    return Address(s);
}
