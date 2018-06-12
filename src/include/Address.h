#ifndef SPREADSHEET_ADDRESS_H
#define SPREADSHEET_ADDRESS_H

#include <string>
#include <climits>

#include "Serializable.h"

using namespace std;

/**
 * Represents column-row address of a cell within a sheet. Both column and row indexes start at 1.
 * When using string description, the format is [a-zA-Z]+[1-9][0-9]* where letters represent column
 * index and numbers represent row index. This struct is immutable.
 */
struct Address : public Serializable
{
private:
    /**
     * Column starting from 1.
     */
    int m_Col;

    /**
     * Row starting from 1.
     */
    int m_Row;

    /**
     * Generates string representation of given column index (which starts at 0).
     */
    static string colName(int col);

public:
    static const int MAX_COL = INT_MAX;
    static const int MAX_ROW = INT_MAX;

    Address() = delete;

    /**
     * Initializes column and row fields with given values.
     *
     * @throws InvalidArgumentException Arguments zero or negative.
     */
    Address(int col, int row);

    /**
     * Initializes column and row fields based on given string-address representation.
     *
     * @throws InvalidArgumentException Address malformed or out of range.
     */
    Address(const string &addr);

    /**
     * Initializes column and row fields based on given string-address representation.
     *
     * @throws InvalidArgumentException Address malformed or out of range.
     */
    Address(const char *addr);

    /**
     * @return Column index.
     */
    int col() const;

    /**
     * @return Row index.
     */
    int row() const;

    /**
     * @return String representation of the column value.
     */
    string colName() const;

    operator string() const;

    bool operator==(const Address &rhs) const;
    bool operator!=(const Address &rhs) const;

    bool operator<(const Address &rhs) const;
    bool operator>(const Address &rhs) const;
    bool operator<=(const Address &rhs) const;
    bool operator>=(const Address &rhs) const;

    /**
     * @throws InvalidArgumentException
     */
    Address operator-(const Address &rhs) const;

    /**
     * Serializes the address to given output stream in JSON as string.
     */
    void serialize(ostream &os) const override;

    /**
     * Creates a new Address from given input stream in JSON.
     *
     * @throws InvalidInputException
     */
    static Address deserialize(istream &is);

    // todo: with() ...
};

#endif /* SPREADSHEET_ADDRESS_H */
