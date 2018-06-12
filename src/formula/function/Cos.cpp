#include "Sheet.h"

#include <cmath>

using namespace std;

namespace Formula
{
    template<>
    int Cos<int>::evaluate(const Sheet &sheet)
    {
        return round(cos(this->m_Arg->evaluate(sheet)));
    }

    template<>
    double Cos<double>::evaluate(const Sheet &sheet)
    {
        return cos(this->m_Arg->evaluate(sheet));
    }
}
