#include "Sheet.h"

using namespace std;

namespace Formula
{
    template<>
    int Add<int>::evaluate(const Sheet &sheet)
    {
        return this->m_Arg1->evaluate(sheet) + this->m_Arg2->evaluate(sheet);
    }

    template<>
    double Add<double>::evaluate(const Sheet &sheet)
    {
        return this->m_Arg1->evaluate(sheet) + this->m_Arg2->evaluate(sheet);
    }

    template<>
    string Add<string>::evaluate(const Sheet &sheet)
    {
        return this->m_Arg1->evaluate(sheet) + this->m_Arg2->evaluate(sheet);
    }
}
