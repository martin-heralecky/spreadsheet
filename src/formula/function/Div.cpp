#include "Sheet.h"

using namespace std;

namespace Formula
{
    template<>
    int Div<int>::evaluate(const Sheet &sheet)
    {
        return this->m_Arg1->evaluate(sheet) / this->m_Arg2->evaluate(sheet);
    }

    template<>
    double Div<double>::evaluate(const Sheet &sheet)
    {
        return this->m_Arg1->evaluate(sheet) / this->m_Arg2->evaluate(sheet);
    }
}
