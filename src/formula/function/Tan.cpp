#include "Sheet.h"

#include <cmath>

using namespace std;

namespace Formula
{
    template<>
    int Tan<int>::evaluate(const Sheet &sheet)
    {
        return round(tan(this->m_Arg->evaluate(sheet)));
    }

    template<>
    double Tan<double>::evaluate(const Sheet &sheet)
    {
        return tan(this->m_Arg->evaluate(sheet));
    }
}
