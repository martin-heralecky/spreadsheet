#ifndef SPREADSHEET_SHEET_H
#define SPREADSHEET_SHEET_H

#include <istream>
#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Address.h"
#include "CellBase.h"
#include "Serializable.h"
#include "Type.h"
#include "Utils.h"

#include "exception/DependencyLoopException.h"
#include "exception/InvalidTypeException.h"
#include "exception/IncorrectFormulaSyntaxException.h"

using namespace std;

namespace std
{
    template<>
    struct hash<Address>
    {
        size_t operator()(const Address &addr) const;
    };
}

/**
 * Represents data structure for cells in a sheet. Manages writing to cells as well as distributing
 * content-changed events. Alone does not handle any user input or provide any user output.
 *
 * DEPENDENCIES:
 *     Every cell has a container of its dependencies (addresses it depends on). Sheet has a map
 *     of dependencies which maps the address to cells that depend on that address. The cell's
 *     container is redundant but provides faster iteration through cell's dependencies.
 */
class Sheet : public Serializable
{
    friend class __Test;

    /**
     * NOT IMPLEMENTED
     *
     * Indicates whether the format of a new cell should be automatically detected or not (in which
     * case new cells are always Cell<string>).
     */
    /* bool m_AutoDetectCellType = false; */

    /**
     * All cells in this spreadsheet, indexed by their addresses. Contains only non-empty cells.
     */
    unordered_map<Address, shared_ptr<CellBase>> m_Cells;

    /**
     * All dependencies across cells in this spreadsheet.
     * Form: cell address -> cells that depend on that cell
     */
    unordered_map<Address, unordered_set<shared_ptr<const CellBase>>> m_Dependencies;

    /**
     * Event that is called whenever content of any cell is changed.
     */
    function<void(const CellBase &)> m_CellContentChanged;

    /**
     * Copies cell's dependencies from its inner container to m_Dependencies.
     */
    void createDependencies(shared_ptr<const CellBase> cell);

    /**
     * Deletes the cell's dependencies from m_Dependencies based on cell's inner dependencies
     * container.
     */
    void deleteDependencies(shared_ptr<const CellBase> cell);

    /**
     * Triggers the content-changed event for the specified cell and its dependents. Stops further
     * propagation if finds already notified cell.
     *
     * @param cell Cell, whose content changed.
     * @param processedCells Cells that have already been notified and should not be notified again.
     */
    void distributeContentChangedEvent(
        shared_ptr<const CellBase> cell,
        unordered_set<shared_ptr<const CellBase>> processedCells = unordered_set<shared_ptr<const CellBase>>());

public:
    Sheet()
    {}

    Sheet(const Sheet &) = delete;
    Sheet(Sheet &&) = delete;

    /**
     * Saves a function that will be called whenever content of any cell in the spreadsheet changes.
     */
    void attachCellContentChangedEvent(const function<void(const CellBase &)> &cellContentChanged);

    /**
     * Locates cell at the specified address.
     *
     * @return Cell, if found, empty Cell<string> otherwise.
     */
    shared_ptr<const CellBase> getCell(const Address &addr) const;

    /**
     * Assigns the specified text as content of the cell specified by its address. Updates its
     * dependencies. Triggers the content-changed event for this cell and all its dependents
     * recursively.
     *
     * @throws IncorrectFormulaSyntaxException
     * @throws InvalidTypeException
     */
    void setCellContent(const Address &addr, const string &text);

    /**
     * Changes type of the cell specified by its address.
     *
     * @tparam T The new type of the cell.
     *
     * @throws InvalidTypeException
     */
    template<typename T>
    void setCellType(const Address &addr);

    /**
     * Serializes the address to given output stream in JSON as object where keys are addresses
     * and values are cells.
     */
    void serialize(ostream &os) const override;

    /**
     * Creates a new Sheet from given input stream in JSON.
     *
     * @throws InvalidInputException
     */
    static shared_ptr<Sheet> deserialize(istream &is);
};

template<typename T>
class Cell;

namespace Formula
{
    /**
     * Abstract class for any function evaluating to a value of type T.
     *
     * @tparam T Type to which this function evaluates.
     */
    template<typename T>
    class Function
    {
    public:
        /**
         * Evaluates the function.
         *
         * @param sheet The Sheet this function works with.
         */
        virtual T evaluate(const Sheet &sheet) = 0;

        /**
         * Parses the function back to source text.
         */
        virtual string toSource() const = 0;
    };

    /**
     * An abstract function that takes 1 argument of type T and evaluates to a value of type T.
     *
     * @tparam T Both type of argument and the return type.
     */
    template<typename T>
    class UnaryFunction : public Function<T>
    {
    protected:
        unique_ptr<Function<T>> m_Arg;

    public:
        UnaryFunction() = delete;
        UnaryFunction(const UnaryFunction &) = delete;
        UnaryFunction(UnaryFunction &&) = delete;

        /**
         * Initializes the argument.
         */
        UnaryFunction(unique_ptr<Function<T>> arg)
            : m_Arg(move(arg))
        {}

        /**
         * Evaluates the function.
         *
         * @param sheet The Sheet this function works with.
         */
        virtual T evaluate(const Sheet &sheet)  = 0;

        virtual string toSource() const = 0;
    };

    /**
     * An abstract function that takes 2 arguments of type T and evaluates to a value of type T.
     *
     * @tparam T Both function arguments type and the return type.
     */
    template<typename T>
    class BinaryFunction : public Function<T>
    {
    protected:
        unique_ptr<Function<T>> m_Arg1;
        unique_ptr<Function<T>> m_Arg2;

    public:
        BinaryFunction() = delete;
        BinaryFunction(const BinaryFunction &) = delete;
        BinaryFunction(BinaryFunction &&) = delete;

        /**
         * Initializes the arguments.
         */
        BinaryFunction(unique_ptr<Function<T>> arg1, unique_ptr<Function<T>> arg2)
            : m_Arg1(move(arg1)),
              m_Arg2(move(arg2))
        {}

        /**
         * Evaluates the function.
         *
         * @param sheet The Sheet this function works with.
         */
        virtual T evaluate(const Sheet &sheet)  = 0;

        /**
         * Parses the function back to source text.
         */
        virtual string toSource() const = 0;
    };

    /**
     * Represents a literal value in the form of a function which evaluates to the given value.
     *
     * @tparam T Type of the literal.
     */
    template<typename T>
    class Literal : public Function<T>
    {
        const T m_Value;

    public:
        /**
         * Initializes the value.
         */
        Literal(const T &value)
            : m_Value(value)
        {}

        /**
         * Evaluates the literal.
         *
         * @param sheet The Sheet this function works with.
         * @return The value this literal was initialized with.
         */
        T evaluate(const Sheet &sheet) override
        {
            return m_Value;
        }

        string toSource() const override
        {
            return Type<T>::toString(m_Value, true);
        }

        string toSource(bool isLiteral) const
        {
            return Type<T>::toString(m_Value, isLiteral);
        }
    };

    /**
     * A function which evaluates to the value of linked cell.
     *
     * @tparam T Type to which this link evaluates.
     */
    template<typename T>
    class Link : public Function<T>
    {
        /**
         * Address of the linked cell.
         */
        const Address m_Addr;

        bool evaluating = false;

    public:
        Link() = delete;
        Link(const Link &) = delete;
        Link(Link &&) = delete;

        /**
         * Initializes the address.
         */
        Link(const Address &addr)
            : m_Addr(addr)
        {}

        /**
         * Evaluates to the value of linked cell.
         *
         * @param sheet The Sheet this function works with.
         *
         * @throws DependencyLoopException
         * @throws InvalidTypeException If linked cell's value is not of type T.
         */
        T evaluate(const Sheet &sheet) override
        {
            if (evaluating) {
                throw DependencyLoopException();
            }

            shared_ptr<const CellBase> linkedCellBase = sheet.getCell(m_Addr);

            const Cell<T> *linkedCell = dynamic_cast<const Cell<T> *>(linkedCellBase.get());

            if (linkedCell == nullptr) {
                throw InvalidTypeException();
            }

            evaluating = true;
            try {
                T res = linkedCell->getContent();
                evaluating = false;
                return res;
            } catch (...) {
                evaluating = false;
                throw;
            }
        }

        string toSource() const override
        {
            return m_Addr;
        }
    };

    /**
     * Function for addition.
     *
     * @tparam T Both function arguments type and the return type.
     */
    template<typename T>
    class Add : public BinaryFunction<T>
    {
    public:
        Add() = delete;
        Add(const Add &) = delete;
        Add(Add &&) = delete;

        /**
         * Initializes the arguments.
         */
        Add(unique_ptr<Function<T>> arg1, unique_ptr<Function<T>> arg2)
            : BinaryFunction<T>(move(arg1), move(arg2))
        {}

        /**
         * Evaluates the argument functions and returns their sum.
         *
         * @param sheet The Sheet this function works with.
         */
        T evaluate(const Sheet &sheet) override
        {
            throw InvalidTypeException();
        }

        string toSource() const override
        {
            return this->m_Arg1->toSource() + "+" + this->m_Arg2->toSource();
        }
    };

    /**
     * Function for subtraction.
     *
     * @tparam T Both function arguments type and the return type.
     */
    template<typename T>
    class Sub : public BinaryFunction<T>
    {
    public:
        Sub() = delete;
        Sub(const Sub &) = delete;
        Sub(Sub &&) = delete;

        /**
         * Initializes the arguments.
         */
        Sub(unique_ptr<Function<T>> minuend, unique_ptr<Function<T>> subtrahend)
            : BinaryFunction<T>(move(minuend), move(subtrahend))
        {}

        /**
         * Evaluates the argument functions and returns their difference.
         *
         * @param sheet The Sheet this function works with.
         */
        T evaluate(const Sheet &sheet) override
        {
            throw InvalidTypeException();
        }

        string toSource() const override
        {
            return this->m_Arg1->toSource() + "-" + this->m_Arg2->toSource();
        }
    };

    /**
     * Function for multiplication.
     *
     * @tparam T Both function arguments type and the return type.
     */
    template<typename T>
    class Mul : public BinaryFunction<T>
    {
    public:
        Mul() = delete;
        Mul(const Mul &) = delete;
        Mul(Mul &&) = delete;

        /**
         * Initializes the arguments.
         */
        Mul(unique_ptr<Function<T>> arg1, unique_ptr<Function<T>> arg2)
            : BinaryFunction<T>(move(arg1), move(arg2))
        {}

        /**
         * Evaluates the argument functions and returns their product.
         *
         * @param sheet The Sheet this function works with.
         */
        T evaluate(const Sheet &sheet) override
        {
            throw InvalidTypeException();
        }

        string toSource() const override
        {
            return this->m_Arg1->toSource() + "*" + this->m_Arg2->toSource();
        }
    };

    /**
     * Function for division.
     *
     * @tparam T Both function arguments type and the return type.
     */
    template<typename T>
    class Div : public BinaryFunction<T>
    {
    public:
        Div() = delete;
        Div(const Div &) = delete;
        Div(Div &&) = delete;

        /**
         * Initializes the arguments.
         */
        Div(unique_ptr<Function<T>> dividend, unique_ptr<Function<T>> divisor)
            : BinaryFunction<T>(move(dividend), move(divisor))
        {}

        /**
         * Evaluates the argument functions and returns their quotient.
         *
         * @param sheet The Sheet this function works with.
         */
        T evaluate(const Sheet &sheet) override
        {
            throw InvalidTypeException();
        }

        string toSource() const override
        {
            return this->m_Arg1->toSource() + "/" + this->m_Arg2->toSource();
        }
    };

    /**
     * Function for absolute value.
     */
    template<typename T>
    class Abs : public UnaryFunction<T>
    {
    public:
        Abs() = delete;
        Abs(const Abs &) = delete;
        Abs(Abs &&) = delete;

        /**
         * Initializes the argument.
         */
        Abs(unique_ptr<Function<T>> arg)
            : UnaryFunction<T>(move(arg))
        {}

        /**
         * Evaluates the argument function and returns its absolute value.
         *
         * @param sheet The Sheet this function works with.
         */
        T evaluate(const Sheet &sheet) override
        {
            throw InvalidTypeException();
        }

        string toSource() const override
        {
            return string("ABS(") + this->m_Arg->toSource() + ")";
        }
    };

    /**
     * Sine function.
     */
    template<typename T>
    class Sin : public UnaryFunction<T>
    {
    public:
        Sin() = delete;
        Sin(const Sin &) = delete;
        Sin(Sin &&) = delete;

        /**
         * Initializes the argument.
         */
        Sin(unique_ptr<Function<T>> arg)
            : UnaryFunction<T>(move(arg))
        {}

        /**
         * Evaluates the argument function and returns its sine value.
         *
         * @param sheet The Sheet this function works with.
         * @return Rounded if T too small.
         */
        T evaluate(const Sheet &sheet) override
        {
            throw InvalidTypeException();
        }

        string toSource() const override
        {
            return string("SIN(") + this->m_Arg->toSource() + ")";
        }
    };

    /**
     * Cosine function.
     */
    template<typename T>
    class Cos : public UnaryFunction<T>
    {
    public:
        Cos() = delete;
        Cos(const Cos &) = delete;
        Cos(Cos &&) = delete;

        /**
         * Initializes the argument.
         */
        Cos(unique_ptr<Function<T>> arg)
            : UnaryFunction<T>(move(arg))
        {}

        /**
         * Evaluates the argument function and returns its cosine value.
         *
         * @param sheet The Sheet this function works with.
         * @return Rounded if T too small.
         */
        T evaluate(const Sheet &sheet) override
        {
            throw InvalidTypeException();
        }

        string toSource() const override
        {
            return string("COS(") + this->m_Arg->toSource() + ")";
        }
    };

    /**
     * Tangent function.
     */
    template<typename T>
    class Tan : public UnaryFunction<T>
    {
    public:
        Tan() = delete;
        Tan(const Tan &) = delete;
        Tan(Tan &&) = delete;

        /**
         * Initializes the argument.
         */
        Tan(unique_ptr<Function<T>> arg)
            : UnaryFunction<T>(move(arg))
        {}

        /**
         * Evaluates the argument function and returns its tangent value.
         *
         * @param sheet The Sheet this function works with.
         * @return Rounded if T too small.
         */
        T evaluate(const Sheet &sheet) override
        {
            throw InvalidTypeException();
        }

        string toSource() const override
        {
            return string("TAN(") + this->m_Arg->toSource() + ")";
        }
    };

    class Parser
    {
        /**
         * Determines whether the given text matches the syntax of a link.
         */
        static bool isLink(const string &source);

        /**
         * Determines whether the given text matches the syntax of a literal of type T.
         *
         * @throws InvalidTypeException
         */
        template<typename T>
        static bool isLiteral(const string &source)
        {
            throw InvalidTypeException();
        }

        /**
         * Finds all logical sections in the given source text.
         *
         * Separators always include ^ and $.
         *
         * EXAMPLES:
         *     5+ABS(7)-ABS(1,ABS(2,3))
         *     returns { "5", "+", "ABS(7)", "-", "ABS(1,ABS(2,3))" }
         *
         *     1.5,ABS(2,3)
         *     returns { "1.5", ",", "ABS(2,3)" }
         *
         *     1+(2-3)
         *     returns { "1", "+", "2-3" }
         *
         *     1,ABS(9,8),3
         *     returns { "1", ",", "ABS(9,8)", ",", "3" }
         *
         *     1+"1+2","foo ABS(9,8)"
         *     returns { "1", "+", "\"1+2\"", ",", "\"foo ABS(9,8)\"" }
         *
         * @throws IncorrectFormulaSyntaxException
         */
        static vector<string> splitLogical(
            const string &source,
            const string &separators = ",+-*/");

        /**
         * Parses given formula sections to Functions structure. Fills given container with address
         * dependencies of the formula.
         *
         * @tparam T Return type of parsed Function.
         *
         * @throws IncorrectFormulaSyntaxException
         * @throws InvalidTypeException
         */
        template<typename T>
        static unique_ptr<Function<T>> parse(
            const vector<string> &sections,
            vector<Address> &dependencies)
        {
            if (sections.size() == 1) {
                /* literal */
                if (isLiteral<T>(sections[0])) {
                    return make_unique<Literal<T>>(Type<T>::fromString(sections[0], true));
                }

                /* link */
                if (isLink(sections[0])) {
                    Address addr = sections[0];
                    dependencies.push_back(addr);
                    return make_unique<Link<T>>(addr);
                }

                /* nested expression */
                const vector<string> newSections = splitLogical(sections[0]);
                if (sections != newSections) {
                    return parse<T>(newSections, dependencies);
                }

                /* function */
                size_t parenthesesPos = sections[0].find('(');
                if (parenthesesPos > 0 && parenthesesPos != string::npos &&
                    sections[0][sections[0].length() - 1] == ')') {
                    string identifier = Utils::toLower(sections[0].substr(0, parenthesesPos));

                    vector<string> arguments = splitLogical(
                        sections[0]
                            .substr(
                                parenthesesPos + 1,
                                sections[0].length() - (parenthesesPos + 1) - 1),
                        ",");

                    if (identifier == "abs" && arguments.size() == 1) {
                        return make_unique<Abs<T>>(
                            parse<T>(splitLogical(arguments[0]), dependencies));
                    }

                    if (identifier == "sin" && arguments.size() == 1) {
                        return make_unique<Sin<T>>(
                            parse<T>(splitLogical(arguments[0]), dependencies));
                    }

                    if (identifier == "cos" && arguments.size() == 1) {
                        return make_unique<Cos<T>>(
                            parse<T>(splitLogical(arguments[0]), dependencies));
                    }

                    if (identifier == "tan" && arguments.size() == 1) {
                        return make_unique<Tan<T>>(
                            parse<T>(splitLogical(arguments[0]), dependencies));
                    }
                }
            }

            /* operations */
            if (sections.size() >= 3 && sections[sections.size() - 2].length() == 1) {
                char op = sections[sections.size() - 2][0];

                vector<string> leftSections(sections.begin(), sections.end() - 2);

                unique_ptr<Function<T>> arg1 = parse<T>(
                    leftSections.size() > 1 ? leftSections : splitLogical(leftSections[0]),
                    dependencies);

                unique_ptr<Function<T>> arg2 = parse<T>(
                    splitLogical(sections[sections.size() - 1]),
                    dependencies);

                switch (op) {
                case '+':
                    return make_unique<Add<T>>(move(arg1), move(arg2));
                case '-':
                    return make_unique<Sub<T>>(move(arg1), move(arg2));
                case '*':
                    return make_unique<Mul<T>>(move(arg1), move(arg2));
                case '/':
                    return make_unique<Div<T>>(move(arg1), move(arg2));
                }
            }

            throw IncorrectFormulaSyntaxException();
        }

    public:
        /**
         * Parses given formula source to Functions structure. Fills given container with address
         * dependencies of the formula.
         *
         * SYNTAX:
         *     EXPRESSION:
         *         literal
         *         link
         *         function
         *         operation
         *         (<expr>)
         *
         *     LITERAL:
         *         int: [0-9]+
         *         double: [0-9]*\.[0-9]+
         *         string: enclosed in double quotes, backslash works as an escape character
         *             double quotes: \"
         *             backslash: \\
         *
         *     LINK: [a-zA-Z]+[1-9][0-9]*
         *
         *     FUNCTION: <identifier>(<expr>[, <expr>, ...])
         *         where <identifier>: [a-zA-Z][a-zA-Z0-9]*
         *         where <identifier> is case-insensitive
         *
         *         ABS(int)
         *         ABS(double)
         *         SIN(int) : rounded
         *         SIN(double)
         *         COS(int) : rounded
         *         COS(double)
         *         TAN(int) : rounded
         *         TAN(double)
         *
         *     OPERATION:
         *         <expr>+<expr>
         *         <expr>-<expr>
         *         <expr>*<expr>
         *         <expr>/<expr>
         *
         * @note All operations have the same priority and are processed from left to right.
         * @note Whitespaces are not allowed (except in string literals).
         *
         * @tparam T Return type of parsed Function.
         *
         * @throws IncorrectFormulaSyntaxException
         * @throws InvalidTypeException
         */
        template<typename T>
        static unique_ptr<Function<T>> parseSource(
            const string &source,
            vector<Address> &dependencies)
        {
            return parse<T>(splitLogical(source), dependencies);
        }
    };
}

/**
 * Generic class representing one cell in the sheet specified by its address.
 *
 * Immutable.
 *
 * Every content is worked with as a formula (Function). Even if it's general text or number,
 * it is parsed as a literal, which is a Function.
 */
template<typename T>
class Cell : public CellBase
{
    /**
     * Whether the content of the cell is formula or a literal.
     */
    bool m_IsFormula;

    /**
     * If the cell's content is a formula: Parsed formula.
     * If the cell's content is not a formula: Literal evaluating to static content.
     */
    unique_ptr<Formula::Function<T>> m_Formula;

public:
    Cell() = delete;
    Cell(const Cell<T> &) = delete;
    Cell(Cell<T> &&) = delete;

    /**
     * Initializes sheet, address and content, parsing its formula and creating dependencies.
     *
     * @throws IncorrectFormulaSyntaxException
     * @throws InvalidTypeException
     */
    Cell(const Sheet &sheet, const Address &addr, const string &content)
        : CellBase(sheet, addr)
    {
        if (content.length() > 0 && content[0] == '=') {
            m_IsFormula = true;
            m_Formula = Formula::Parser::parseSource<T>(content.substr(1), m_Dependencies);
        } else {
            m_IsFormula = false;
            m_Formula = make_unique<Formula::Literal<T>>(Type<T>::fromString(content));
        }
    }

    /**
     * Initializes sheet and address. Content is the type's default value.
     */
    Cell(const Sheet &sheet, const Address &addr)
        : CellBase(sheet, addr)
    {
        m_IsFormula = false;
        m_Formula = make_unique<Formula::Literal<T>>(Type<T>::defaultValue);
    }

    string getType() const override
    {
        return Type<T>::name;
    }

    /**
     * @return Content of the cell, evaluated.
     *
     * @throws InvalidTypeException If there is a Link in the formula and it doesn't evaluate
     *                              to type T.
     */
    T getContent() const
    {
        return m_Formula->evaluate(m_Sheet);
    }

    /**
     * @return Evaluated cell's content converted to string.
     *
     * @throws InvalidTypeException If there is a Link in the formula and it doesn't evaluate
     *                              to type T.
     */
    string getContentText() const override
    {
        return Type<T>::toString(getContent());
    }

    /**
     * @return Cell's content's source. This is either source of the formula or literal converted
     *         to string.
     */
    string getContentSource() const override
    {
        if (m_IsFormula) {
            /* formula */
            return string("=") + m_Formula->toSource();
        }

        /* literal - pass isLiteral(false) to indicate that we want pure value (i.e. not a string
         * surrounded by double quotes) */
        return static_cast<Formula::Literal<T> *>(m_Formula.get())->toSource(false);
    }

    /**
     * Serializes the cell to given output stream in JSON as object with cell type and its source
     * content.
     */
    void serialize(ostream &os) const override
    {
        os << "{";
        os << "\"type\":\"" << Utils::escapeString(Type<T>::name) << "\",";
        os << "\"addr\":";
        m_Addr.serialize(os);
        os << ",";
        os << "\"content\":\"" << Utils::escapeString(getContentSource()) << "\"";
        os << "}";

        os.flush();
    }

    shared_ptr<CellBase> create(const string &content) override
    {
        return make_shared<Cell<T>>(m_Sheet, m_Addr, content);
    }
};

template<typename T>
void Sheet::setCellType(const Address &addr)
{
    unordered_map<Address, shared_ptr<CellBase>>::iterator it = m_Cells.find(addr);

    shared_ptr<CellBase> cell;

    /* cell doesn't exist yet */
    if (it == m_Cells.end()) {
        /* don't create empty string cell */
        if (is_same<T, string>::value) {
            return;
        }

        cell = make_shared<Cell<T>>(*this, addr);
        m_Cells[addr] = cell;
    } else if (is_same<T, string>::value && it->second->getContentSource().empty()) {
        cell = it->second;
        deleteDependencies(cell);
        m_Cells.erase(it);
    } else {
        // todo: don't recreate cell if the type doesn't change

        cell = make_shared<Cell<T>>(*this, addr, it->second->getContentSource());

        deleteDependencies(it->second);
        m_Cells[addr] = cell;
        createDependencies(cell);
    }

    /* if T is string and cell's content is empty, it is now removed from m_Cells, but "cell" still
     * holds the last reference, so we can use it to trigger the content-changed event */
    distributeContentChangedEvent(cell);
}

#endif /* SPREADSHEET_SHEET_H */
