#ifndef SPREADSHEET_CELL_BASE_H
#define SPREADSHEET_CELL_BASE_H

#include <memory>
#include <string>
#include <vector>

#include "Address.h"
#include "Serializable.h"

using namespace std;

class Sheet;

/**
 * Base class of every cell.
 *
 * Immutable.
 */
class CellBase : public Serializable
{
protected:
    /**
     * The sheet this cell belongs to.
     */
    const Sheet &m_Sheet;

    /**
     * The address of this cell within the sheet.
     */
    const Address m_Addr;

    /**
     * Addresses of cells this cell depends on.
     */
    vector<Address> m_Dependencies;

public:
    /**
     * Initializes sheet and address.
     */
    CellBase(const Sheet &sheet, const Address &addr);

    /**
     * @return Type name of the cell.
     */
    virtual string getType() const = 0;

    /**
     * @return The address of this cell.
     */
    Address getAddr() const;

    /**
     * @return Addresses of cells this cell depends on.
     */
    const vector<Address> &getDependencies() const;

    /**
     * @return Evaluated cell's content converted to string.
     *
     * @throws InvalidTypeException
     */
    virtual string getContentText() const = 0;

    /**
     * @return Cell's content's source.
     */
    virtual string getContentSource() const = 0;

    /**
     * Creates a new cell of the same type, copying the sheet ref and address.
     *
     * @param content The content of the new cell.
     */
    virtual shared_ptr<CellBase> create(const string &content) = 0;

    /**
     * Creates a new CellBase from given input stream in JSON.
     *
     * @param sheet Sheet this cell belongs to.
     *
     * @throws InvalidInputException
     */
    static shared_ptr<CellBase> deserialize(istream &is, const Sheet &sheet);
};

#endif /* SPREADSHEET_CELL_BASE_H */
