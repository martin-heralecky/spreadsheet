#ifndef SPREADSHEET_SERIALIZABLE_H
#define SPREADSHEET_SERIALIZABLE_H

#include <ostream>

using namespace std;

/**
 * Interface that declares the serialize method.
 */
class Serializable
{
public:
    /**
     * Serializes this object to given output stream.
     */
    virtual void serialize(ostream &os) const = 0;
};

#endif /* SPREADSHEET_SERIALIZABLE_H */
