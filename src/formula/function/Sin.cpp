#include "Sheet.h"

#include <cmath>

using namespace std;

namespace Formula
{
    template<>
    int Sin<int>::evaluate(const Sheet &sheet)
    {
        return round(sin(this->m_Arg->evaluate(sheet)));
    }

    template<>
    double Sin<double>::evaluate(const Sheet &sheet)
    {
        return sin(this->m_Arg->evaluate(sheet));
    }
}
