#include "Sheet.h"

using namespace std;

namespace Formula
{
    template<>
    int Sub<int>::evaluate(const Sheet &sheet)
    {
        return this->m_Arg1->evaluate(sheet) - this->m_Arg2->evaluate(sheet);
    }

    template<>
    double Sub<double>::evaluate(const Sheet &sheet)
    {
        return this->m_Arg1->evaluate(sheet) - this->m_Arg2->evaluate(sheet);
    }
}
