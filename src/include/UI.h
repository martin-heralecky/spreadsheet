#ifndef SPREADSHEET_UI_H
#define SPREADSHEET_UI_H

#include <memory>

#include <form.h>
#include <ncurses.h>

#include "Sheet.h"

/* thus we can't use magenta anywhere */
#define COLOR_GREY COLOR_MAGENTA

using namespace std;

/**
 * Manages all user input (calls Sheet's methods) and output (attaches hooks to Sheet's events).
 *
 * In order to run correctly, the program should be executed in 256-colors terminal. This can be
 * achieved by: $ TERM=xterm-256color
 *
 * It is never granted that the cursor will stay at its location. Anytime it is to be used,
 * we should set its location first, explicitly.
 *
 * No draw*() method calls refresh(). Needs to be called explicitly.
 *
 * COMMANDS:
 *     write <filename> - saves the sheet to the file
 *     w - alias for write
 *
 *     load <filename> - loads the sheet from the file
 *     l - alias for load
 *
 *     quit - ends the UI
 *     q - alias for quit
 */
class UI
{
    /**
     * Mode the UI can be in.
     */
    enum WorkingMode
    {
        BROWSE, /** Moving between cells, showing content. */
        EDIT,   /** Inserting/modifying cell's content. */
        CONTROL /** Executing commands. */
    };

    enum CellState
    {
        ACTIVE, INACTIVE
    };

    /* color-pair constants */
    const short BLACK_BLACK = 16;
    const short WHITE_BLACK = 17;
    const short GREY_BLACK = 18;
    const short RED_BLACK = 19;
    const short GREEN_BLACK = 20;
    const short BLUE_BLACK = 21;
    const short CYAN_BLACK = 22;
    const short YELLOW_BLACK = 23;

    int ACTIVE_CELL_COLOR;
    int INACTIVE_CELL_COLOR;

    /**
     * Not including border.
     */
    int m_CellWidth;

    /**
     * Is automatically counted so it always fills remaining space.
     */
    int m_VerticalHeaderCellWidth;

    /**
     * Number of columns currently shown in viewport.
     */
    int m_ViewportCols;

    /**
     * Number of rows currently shown in viewport.
     */
    int m_ViewportRows;

    /**
     * Absolute address of the top-left cell.
     */
    Address m_ViewportShift = Address(1, 1);

    /**
     * Absolute address of the currently active cell.
     */
    Address m_ActiveCellAddr = Address(1, 1);

    /**
     * Mode the UI currently is in.
     */
    WorkingMode m_Mode;

    FORM *m_PromptForm;
    FIELD *m_PromptField[2];

    /**
     * The Sheet the UI works with.
     */
    shared_ptr<Sheet> m_Sheet;

    /**
     * Initializes ncurses and colors.
     */
    UI();

    /**
     * Initializes members, loads a sheet from file, and starts the UI.
     */
    void init(const string &filename);

    /**
     * Initializes members, loads the given sheet, and starts the UI.
     */
    void init(shared_ptr<Sheet> sheet);

    /**
     * Initializes viewport, starts handling all user input.
     *
     * @return 0 for exit, 1 for re-init.
     *
     * @throws ViewportTooSmallException
     */
    int run();

    /**
     * Sets active cell's address and highlights it. Shifts the viewport, if necessary.
     *
     * @throws InvalidArgumentException Address out of range.
     */
    void moveActiveCell(Address addr);

    /**
     * Prints contents of all cells in the viewport.
     */
    void printAllCells();

    /**
     * Prints the content of given cell at its address (according to current viewport). Trims
     * if necessary. Does nothing if the cell's address is out of viewport.
     */
    void printCell(const CellBase &cell);

    /**
     * Creates the form and the field for prompt. Does not call refresh().
     */
    void createPromptForm();

    /**
     * Deletes the prompt form and frees its resources.
     */
    void deletePromptForm();

    /**
     * Prints the prompt according to the current state.
     */
    void updatePrompt();

    /**
     * Executes the function and displays an error if it throws an Exception.
     */
    void runSafe(const function<void()> &fun);

    /**
     * Prints the given text as an error message.
     */
    void printError(const string &text);

    /**
     * Prints the given text as a success message.
     */
    void printSuccess(const string &text);

    /**
     * Draws grid lines for the horizontal and vertical headers.
     */
    void drawHeadersGrid();

    /**
     * Draws grid lines (fills the viewport).
     */
    void drawGrid();

    /**
     * Draws horizontal and vertical header labels according to current viewport shift.
     */
    void drawHeaderLabels();

    /**
     * Highlights the specified cell (its border and associated borders of both horizontal
     * and vertical header). Does not call refresh().
     *
     * @param addr Cell address relative to viewport.
     */
    void highlightCell(Address addr);

    /**
     * Disables highlight of the specified cell (its border and associated borders of both
     * horizontal and vertical header). Does not call refresh().
     *
     * @param addr Cell address relative to viewport.
     */
    void unhighlightCell(Address addr);

    /**
     * Draws border around the specified cell, either marking it as active or inactive. Uses
     * existing color scheme. Does nothing if the cell is out of viewport.
     *
     * @param cell Cell address relative to viewport.
     */
    void drawCellBorder(Address cell, CellState state);

    /**
     * Draws border around the header cells (both horizontal and vertical) that correspond
     * to the specified cell-address, either marking it as active or inactive. Uses existing color
     * scheme.
     *
     * @param addr Cell address relative to viewport.
     */
    void drawHeaderCellsBorder(Address addr, CellState state);

public:
    UI(const UI &) = delete;
    UI(UI &&) = delete;

    /**
     * Starts the UI.
     */
    static void start();

    /**
     * Terminates ncurses.
     */
    ~UI();
};

#endif /* SPREADSHEET_UI_H */
