#include "CellBase.h"

#include <istream>

#include "Sheet.h"

#include "exception/InvalidInputException.h"

using namespace std;

CellBase::CellBase(const Sheet &sheet, const Address &addr)
    : m_Sheet(sheet),
      m_Addr(addr)
{}

Address CellBase::getAddr() const
{
    return m_Addr;
}

const vector<Address> &CellBase::getDependencies() const
{
    return m_Dependencies;
}

shared_ptr<CellBase> CellBase::deserialize(istream &is, const Sheet &sheet)
{
    string s, type, content;
    Address addr(1, 1);
    bool addrInitialized = false;

    is >> skipws;
    Utils::assertInput(is, '{');

    for (int i = 0; i < 3; ++i) {
        is >> skipws;
        Utils::assertInput(is, '"');

        is >> noskipws;
        getline(is, s, '"');

        is >> skipws;
        Utils::assertInput(is, ':');

        if (s == "addr") {
            addr = Address::deserialize(is);
            addrInitialized = true;
        } else {
            Utils::assertInput(is, '"');

            is >> noskipws;
            if (s == "type") {
                getline(is, type, '"');
            } else if (s == "content") {
                content = Utils::unescapeString(Utils::readString(is));
            }
        }

        if (i < 2) {
            is >> skipws;
            Utils::assertInput(is, ',');
        }
    }

    Utils::assertInput(is, '}');

    if (!addrInitialized) {
        throw InvalidInputException();
    }

    if (type == "int") {
        return make_shared<Cell<int>>(sheet, addr, content);
    } else if (type == "double") {
        return make_shared<Cell<double>>(sheet, addr, content);
    } else if (type == "string") {
        return make_shared<Cell<string>>(sheet, addr, content);
    }

    throw InvalidInputException();
}
