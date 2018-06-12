#include "Sheet.h"

#include <algorithm>

using namespace std;

size_t hash<Address>::operator()(const Address &addr) const
{
    return hash<string>()(addr);
}

void Sheet::createDependencies(shared_ptr<const CellBase> cell)
{
    for (const Address &depAddr : cell->getDependencies()) {
        m_Dependencies[depAddr].insert(cell);
    }
}

void Sheet::deleteDependencies(shared_ptr<const CellBase> cell)
{
    for (const Address &depAddr : cell->getDependencies()) {
        unordered_set<shared_ptr<const CellBase>> &deps = m_Dependencies[depAddr];
        unordered_set<shared_ptr<const CellBase>>::iterator it = deps.find(cell);
        if (it != deps.end()) {
            deps.erase(it);
        }
    }
}

void Sheet::distributeContentChangedEvent(
    shared_ptr<const CellBase> cell,
    unordered_set<shared_ptr<const CellBase>> processedCells)
{
    if (!m_CellContentChanged || processedCells.find(cell) != processedCells.end()) {
        return;
    }

    m_CellContentChanged(*cell);

    unordered_map<Address, unordered_set<shared_ptr<const CellBase>>>::const_iterator it
        = m_Dependencies.find(cell->getAddr());

    /* no dependents */
    if (it == m_Dependencies.end()) {
        return;
    }

    unordered_set<shared_ptr<const CellBase>> newProcessedCells;
    newProcessedCells.insert(processedCells.begin(), processedCells.end());
    newProcessedCells.insert(cell);

    for (shared_ptr<const CellBase> dependent : it->second) {
        distributeContentChangedEvent(dependent, newProcessedCells);
    }
}

void Sheet::attachCellContentChangedEvent(
    const function<void(const CellBase &)> &cellContentChanged)
{
    m_CellContentChanged = cellContentChanged;
}

shared_ptr<const CellBase> Sheet::getCell(const Address &addr) const
{
    unordered_map<Address, shared_ptr<CellBase>>::const_iterator it = m_Cells.find(addr);

    /* cell doesn't exist */
    if (it == m_Cells.end()) {
        return make_shared<const Cell<string>>(*this, addr, "");
    }

    return it->second;
}

void Sheet::setCellContent(const Address &addr, const string &text)
{
    unordered_map<Address, shared_ptr<CellBase>>::iterator it = m_Cells.find(addr);

    shared_ptr<CellBase> cell;

    /* cell doesn't exist yet */
    if (it == m_Cells.end()) {
        if (text.empty()) {
            return;
        }

        cell = make_shared<Cell<string>>(*this, addr, text);
        m_Cells[addr] = cell;

        createDependencies(cell);

        distributeContentChangedEvent(cell);
    } else {
        cell = it->second;

        deleteDependencies(cell);

        cell = cell->create(text);

        /* delete empty string cell */
        if (cell->getContentSource().empty() &&
            dynamic_cast<Cell<string> *>(cell.get()) != nullptr) {
            m_Cells.erase(it);
        } else {
            m_Cells[addr] = cell;
            createDependencies(cell);
        }

        /* if the cell is of string type, it is now removed from m_Cells, but "cell" still holds
         * the last reference, so we can use it to trigger the content-changed event */
        distributeContentChangedEvent(cell);
    }

    /**
     * We could trigger events only for cells whose content actually changed. But since there is
     * no "last content" stored anywhere, we'd have to evaluate all contents before making
     * the change to the current cell and then comparing with the new contents. Problem is that
     * this would be generally even worse because we'd have to reevaluate contents of all
     * dependent (directly or indirectly) cells, no matter whether they're currently in the
     * viewport of UI or not.
     */
}

void Sheet::serialize(ostream &os) const
{
    os << "[";

    for (unordered_map<Address, shared_ptr<CellBase>>::const_iterator it = m_Cells.begin();
        it != m_Cells.end();
        ++it) {
        it->second->serialize(os);

        if (next(it) != m_Cells.end()) {
            os << ",";
        }
    }

    os << "]";

    os.flush();
}

shared_ptr<Sheet> Sheet::deserialize(istream &is)
{
    shared_ptr<Sheet> sheet = make_shared<Sheet>();
    char c;

    is >> skipws;
    Utils::assertInput(is, '[');

    is >> c;
    if (c == ']') {
        return sheet;
    } else {
        is.seekg(-1, is.cur);
    }

    do {
        shared_ptr<CellBase> cell = CellBase::deserialize(is, *sheet);
        sheet->m_Cells[cell->getAddr()] = cell;
        sheet->createDependencies(cell);

        is >> skipws;
        is >> c;
    } while (c == ',');
    is.seekg(-1, is.cur);

    Utils::assertInput(is, ']');

    return sheet;
}
