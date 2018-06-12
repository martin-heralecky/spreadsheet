#include "Sheet.h"

#include <cmath>

using namespace std;

namespace Formula
{
    template<>
    int Abs<int>::evaluate(const Sheet &sheet)
    {
        return abs(this->m_Arg->evaluate(sheet));
    }

    template<>
    double Abs<double>::evaluate(const Sheet &sheet)
    {
        return abs(this->m_Arg->evaluate(sheet));
    }
}
