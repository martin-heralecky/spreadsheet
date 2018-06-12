#include "UI.h"

#include <cassert>
#include <fstream>

#include "exception/InvalidArgumentException.h"
#include "exception/IOException.h"
#include "exception/UnknownCommandException.h"
#include "exception/ViewportTooSmallException.h"

using namespace std;

void UI::start()
{
    UI ui;
    ui.init(make_shared<Sheet>());

    /* run returns 1 if it needs to re-run (i.e. terminal size changed) */
    while (ui.run());
}

UI::UI()
{
    initscr();
    start_color();

    init_color(COLOR_GREY, 250, 250, 250);

    init_pair(BLACK_BLACK, COLOR_BLACK, COLOR_BLACK);
    init_pair(WHITE_BLACK, COLOR_WHITE, COLOR_BLACK);
    init_pair(GREY_BLACK, COLOR_GREY, COLOR_BLACK);
    init_pair(RED_BLACK, COLOR_RED, COLOR_BLACK);
    init_pair(GREEN_BLACK, COLOR_GREEN, COLOR_BLACK);
    init_pair(BLUE_BLACK, COLOR_BLUE, COLOR_BLACK);
    init_pair(CYAN_BLACK, COLOR_CYAN, COLOR_BLACK);
    init_pair(YELLOW_BLACK, COLOR_YELLOW, COLOR_BLACK);

    ACTIVE_CELL_COLOR = COLOR_PAIR(WHITE_BLACK);
    INACTIVE_CELL_COLOR = COLOR_PAIR(GREY_BLACK);
}

void UI::init(const string &filename)
{
    // todo

    // init(sheet);
}

void UI::init(shared_ptr<Sheet> sheet)
{
    m_CellWidth = 16;
    m_ViewportShift = Address(1, 1);
    m_ActiveCellAddr = Address(1, 1);
    m_Mode = WorkingMode::BROWSE;
    m_Sheet = sheet;
}

UI::~UI()
{
    endwin();
}

int UI::run()
{
    clear();

    curs_set(0);
    cbreak();
    noecho();
    keypad(stdscr, true);

    /* 11 chars for header (incl. border) */
    m_ViewportCols = (getmaxx(stdscr) - 11) / (m_CellWidth + 1);

    /* 1 line for prompt, 2 lines for header (incl. border) */
    m_ViewportRows = (getmaxy(stdscr) - 1 - 2) / 2;

    if (m_ViewportCols <= 0 || m_ViewportRows <= 0) {
        throw ViewportTooSmallException();
        // todo: handle this
    }

    m_VerticalHeaderCellWidth = getmaxx(stdscr) - m_ViewportCols * (m_CellWidth + 1) - 1;

    createPromptForm();

    drawHeadersGrid();
    drawGrid();
    drawHeaderLabels();

    printAllCells();

    m_Sheet->attachCellContentChangedEvent([&](const CellBase &cell) {
        printCell(cell);
    });

    moveActiveCell(m_ActiveCellAddr);

    updatePrompt();

    bool exit = false, reset = false;
    int c;
    while (!exit && (c = getch())) {
        if (c == KEY_RESIZE) {
            reset = true;
            break;
        }

        /* for debug */
        if (c == KEY_F(10)) {
            moveActiveCell(Address(2147483640, 2147483640));
        }

        switch (m_Mode) {
        case WorkingMode::BROWSE:
            switch (c) {
            case KEY_UP:
                if (m_ActiveCellAddr.row() > 1)
                    moveActiveCell(Address(m_ActiveCellAddr.col(), m_ActiveCellAddr.row() - 1));
                break;
            case KEY_LEFT:
                if (m_ActiveCellAddr.col() > 1)
                    moveActiveCell(Address(m_ActiveCellAddr.col() - 1, m_ActiveCellAddr.row()));
                break;
            case KEY_DOWN:
                if (m_ActiveCellAddr.row() < Address::MAX_ROW)
                    moveActiveCell(Address(m_ActiveCellAddr.col(), m_ActiveCellAddr.row() + 1));
                break;
            case KEY_RIGHT:
                if (m_ActiveCellAddr.col() < Address::MAX_COL)
                    moveActiveCell(Address(m_ActiveCellAddr.col() + 1, m_ActiveCellAddr.row()));
                break;

            case KEY_PPAGE:
                moveActiveCell(
                    Address(
                        m_ActiveCellAddr.col(),
                        max(1, m_ActiveCellAddr.row() - m_ViewportRows)));
                break;
            case KEY_NPAGE:
                moveActiveCell(
                    Address(
                        m_ActiveCellAddr.col(),
                        min(Address::MAX_ROW, m_ActiveCellAddr.row() + m_ViewportRows)));
                break;

            case 'I':
                runSafe([&]() {
                    m_Sheet->setCellType<int>(m_ActiveCellAddr);
                    updatePrompt();
                });
                break;
            case 'D':
                runSafe([&]() {
                    m_Sheet->setCellType<double>(m_ActiveCellAddr);
                    updatePrompt();
                });
                break;
            case 'S':
                runSafe([&]() {
                    m_Sheet->setCellType<string>(m_ActiveCellAddr);
                    updatePrompt();
                });
                break;

            case KEY_DC:
                runSafe([&]() {
                    m_Sheet->setCellContent(m_ActiveCellAddr, "");
                    updatePrompt();
                });
                break;

            case '\n': /* enter */
                m_Mode = WorkingMode::EDIT;
                curs_set(1);
                break;

            case ':':
                m_Mode = WorkingMode::CONTROL;
                set_field_buffer(m_PromptField[0], 0, "");
                curs_set(1);
                break;
            }
            break;

        case WorkingMode::EDIT:
        case WorkingMode::CONTROL:
            switch (c) {
            case KEY_LEFT:
                form_driver(m_PromptForm, REQ_PREV_CHAR);
                break;
            case KEY_RIGHT:
                form_driver(m_PromptForm, REQ_NEXT_CHAR);
                break;

            case KEY_HOME:
                form_driver(m_PromptForm, REQ_BEG_FIELD);
                break;
            case KEY_END:
                form_driver(m_PromptForm, REQ_END_FIELD);
                break;

            case KEY_DC:
                form_driver(m_PromptForm, REQ_DEL_CHAR);
                break;
            case KEY_BACKSPACE:
                form_driver(m_PromptForm, REQ_DEL_PREV);
                break;
            }

            switch (m_Mode) {
            case WorkingMode::EDIT:
                if (c == '\n') { /* enter */
                    curs_set(0);

                    form_driver(m_PromptForm, REQ_VALIDATION);

                    runSafe([&]() {
                        m_Sheet->setCellContent(
                            m_ActiveCellAddr,
                            Utils::trimRight(field_buffer(m_PromptField[0], 0)));
                        updatePrompt();
                    });

                    m_Mode = WorkingMode::BROWSE;
                } else {
                    form_driver(m_PromptForm, c);
                }
                break;

            case WorkingMode::CONTROL:
                if (c == '\n') { /* enter */
                    curs_set(0);

                    form_driver(m_PromptForm, REQ_VALIDATION);

                    string command = Utils::trim(field_buffer(m_PromptField[0], 0));

                    runSafe([&]() {
                        if (command.length() == 0)
                            throw UnknownCommandException();

                        string cmdName, arg;

                        size_t spacePos = command.find(' ');
                        if (spacePos == string::npos) {
                            cmdName = command;
                        } else {
                            cmdName = command.substr(0, spacePos);
                            arg = Utils::trim(command.substr(spacePos + 1));
                        }

                        if (cmdName == "write" || cmdName == "w") {
                            ofstream file(arg);
                            if (!file.good())
                                throw IOException();

                            m_Sheet->serialize(file);
                            file.close();

                            printSuccess("Written.");
                        } else if (cmdName == "load" || cmdName == "l") {
                            ifstream file(arg);
                            if (!file.good())
                                throw IOException();

                            shared_ptr<Sheet> sheet = Sheet::deserialize(file);
                            file.close();

                            init(sheet);
                            reset = true;
                            exit = true;
                        } else if (cmdName == "quit" || cmdName == "q") {
                            exit = true;
                        } else
                            throw UnknownCommandException();
                    });

                    m_Mode = WorkingMode::BROWSE;
                } else {
                    form_driver(m_PromptForm, c);
                }
                break;

            case WorkingMode::BROWSE:
                assert(false);
                break;
            }
            break;
        }
    }

    m_Sheet->attachCellContentChangedEvent(nullptr);

    deletePromptForm();

    return reset;
}

void UI::moveActiveCell(Address addr)
{
    if (addr.col() > Address::MAX_COL || addr.row() > Address::MAX_ROW) {
        throw InvalidArgumentException();
    }

    // todo: don't redraw cell border and header borders if not actually moving

    unhighlightCell(m_ActiveCellAddr - m_ViewportShift);

    m_ActiveCellAddr = addr;

    int newShiftCol = m_ViewportShift.col();
    int newShiftRow = m_ViewportShift.row();

    if (m_ActiveCellAddr.col() > m_ViewportShift.col() - 1 + m_ViewportCols) {
        /* cell right of viewport */
        newShiftCol = m_ActiveCellAddr.col() - m_ViewportCols + 1;
    } else if (m_ActiveCellAddr.col() < m_ViewportShift.col()) {
        /* cell left of viewport */
        newShiftCol = m_ActiveCellAddr.col();
    }

    if (m_ActiveCellAddr.row() > m_ViewportShift.row() - 1 + m_ViewportRows) {
        /* cell below viewport */
        newShiftRow = m_ActiveCellAddr.row() - m_ViewportRows + 1;
    } else if (m_ActiveCellAddr.row() < m_ViewportShift.row()) {
        /* cell above viewport */
        newShiftRow = m_ActiveCellAddr.row();
    }

    /* apply new viewport shift, redraw header labels and rewrite grid content */
    if (newShiftCol != m_ViewportShift.col() || newShiftRow != m_ViewportShift.row()) {
        m_ViewportShift = Address(newShiftCol, newShiftRow);
        drawHeaderLabels();
        printAllCells();
    }

    highlightCell(m_ActiveCellAddr - m_ViewportShift);

    updatePrompt();

    refresh();
}

void UI::printAllCells()
{
    for (int row = m_ViewportShift.row(); row < m_ViewportShift.row() + m_ViewportRows; ++row) {
        for (int col = m_ViewportShift.col(); col < m_ViewportShift.col() + m_ViewportCols; ++col) {
            shared_ptr<const CellBase> cell = m_Sheet->getCell(Address(col, row));
            printCell(*cell);
        }
    }
}

void UI::printCell(const CellBase &cell)
{
    Address viewportEnd(
        m_ViewportShift.col() + m_ViewportCols - 1,
        m_ViewportShift.row() + m_ViewportRows - 1);

    /* cell out of viewport */
    if (cell.getAddr().col() < m_ViewportShift.col() ||
        cell.getAddr().row() < m_ViewportShift.row() ||
        cell.getAddr().col() > viewportEnd.col() ||
        cell.getAddr().row() > viewportEnd.row()) {
        return;
    }

    /* cell too small */
    if (m_CellWidth <= 0) {
        return;
    }

    Address relAddr = cell.getAddr() - m_ViewportShift;

    // todo: align different cell-types differently
    string cellContent;

    try {
        cellContent = Utils::strPadRight(cell.getContentText().substr(0, m_CellWidth), m_CellWidth);
    } catch (...) {
        cellContent = Utils::strPadCenter("[-error-]", m_CellWidth);
    }

    mvprintw(
        2 + (relAddr.row() - 1) * 2,
        m_VerticalHeaderCellWidth + 1 + (relAddr.col() - 1) * (m_CellWidth + 1),
        cellContent.c_str());

    refresh();
}

void UI::createPromptForm()
{
    /* 21 chars in left side of prompt, up to 10 chars in right side of prompt */
    m_PromptField[0] = new_field(1, getmaxx(stdscr) - 21 - 10, getmaxy(stdscr) - 1, 21, 0, 0);
    m_PromptField[1] = nullptr;

    field_opts_off(m_PromptField[0], O_STATIC);

    m_PromptForm = new_form(m_PromptField);
    post_form(m_PromptForm);
}

void UI::deletePromptForm()
{
    unpost_form(m_PromptForm);
    free_form(m_PromptForm);
    free_field(m_PromptField[0]);
}

void UI::updatePrompt()
{
    int promptRow = getmaxy(stdscr) - 1;

    /* clear the prompt first */
    move(promptRow, 0);
    clrtoeol();

    shared_ptr<const CellBase> activeCell = m_Sheet->getCell(m_ActiveCellAddr);

    /* left side */
    string prompt_left;
    prompt_left += " ";
    prompt_left += Utils::strPadLeft(m_ActiveCellAddr, 17);
    prompt_left += " : ";

    mvprintw(promptRow, 0, prompt_left.c_str());

    /* right side */
    string prompt_right;
    prompt_right += activeCell->getType();
    prompt_right += " ";

    mvprintw(promptRow, getmaxx(stdscr) - prompt_right.length(), prompt_right.c_str());

    /* active cell value */
    set_field_buffer(
        m_PromptField[0],
        0,
        activeCell->getContentSource().c_str());

    refresh();
}

void UI::runSafe(const function<void()> &fun)
{
    try {
        fun();
    } catch (const Exception &ex) {
        printError("error");
    }
}

void UI::printError(const string &text)
{
    set_field_buffer(m_PromptField[0], 0, text.c_str());
    set_field_fore(m_PromptField[0], COLOR_PAIR(RED_BLACK));
    refresh();

    set_field_fore(m_PromptField[0], COLOR_PAIR(WHITE_BLACK));
}

void UI::printSuccess(const string &text)
{
    set_field_buffer(m_PromptField[0], 0, text.c_str());
    set_field_fore(m_PromptField[0], COLOR_PAIR(GREEN_BLACK));
    refresh();

    set_field_fore(m_PromptField[0], COLOR_PAIR(WHITE_BLACK));
}

void UI::drawHeadersGrid()
{
    attron(INACTIVE_CELL_COLOR);

    /* horizontal */
    mvhline(1, 0, ACS_HLINE, m_VerticalHeaderCellWidth + 1 + m_ViewportCols * (m_CellWidth + 1));
    for (int col = 0; col < m_ViewportCols; ++col) {
        move(0, m_VerticalHeaderCellWidth + (col + 1) * (m_CellWidth + 1));
        addch(ACS_VLINE);

        move(1, m_VerticalHeaderCellWidth + (col + 1) * (m_CellWidth + 1));
        addch(ACS_BTEE);
    }

    /* vertical */
    mvvline(0, m_VerticalHeaderCellWidth, ACS_VLINE, 2 + m_ViewportRows * 2);
    for (int row = 0; row < m_ViewportRows; ++row) {
        mvhline(3 + row * 2, 0, ACS_HLINE, m_VerticalHeaderCellWidth);
        move(3 + row * 2, m_VerticalHeaderCellWidth);
        addch(ACS_RTEE);
    }

    move(1, m_VerticalHeaderCellWidth);
    addch(ACS_PLUS);

    attroff(INACTIVE_CELL_COLOR);
}

void UI::drawGrid()
{
    attron(INACTIVE_CELL_COLOR);

    /* print horizontal lines */
    for (int row = 0; row < m_ViewportRows; ++row) {
        mvhline(
            3 + row * 2,
            m_VerticalHeaderCellWidth + 1,
            ACS_HLINE,
            m_ViewportCols * (m_CellWidth + 1));
    }

    /* print vertical lines */
    for (int col = 0; col < m_ViewportCols; ++col) {
        mvvline(
            2,
            m_VerticalHeaderCellWidth + (col + 1) * (m_CellWidth + 1),
            ACS_VLINE,
            m_ViewportRows * 2);
    }

    /* clear line intersections */
    for (int row = 0; row < m_ViewportRows; ++row) {
        for (int col = 0; col < m_ViewportCols; ++col) {
            move(3 + row * 2, m_VerticalHeaderCellWidth + (col + 1) * (m_CellWidth + 1));
            addch(' ');
        }
    }

    attroff(INACTIVE_CELL_COLOR);
}

void UI::drawHeaderLabels()
{
    Address cell_cursor = Address(1, 1);
    Address labels_cursor = m_ViewportShift;

    string label;

    bool proceed;
    do {
        proceed = false;

        if (cell_cursor.col() - 1 < m_ViewportCols) {
            label = Utils::strPadCenter(labels_cursor.colName(), m_CellWidth);
            mvprintw(
                0,
                m_VerticalHeaderCellWidth + 1 + (cell_cursor.col() - 1) * (m_CellWidth + 1),
                label.c_str());

            proceed = true;
        }

        if (cell_cursor.row() - 1 < m_ViewportRows) {
            label = Utils::strPadCenter(to_string(labels_cursor.row()), m_VerticalHeaderCellWidth);
            mvprintw(2 + (cell_cursor.row() - 1) * 2, 0, label.c_str());

            proceed = true;
        }

        cell_cursor = Address(cell_cursor.col() + 1, cell_cursor.row() + 1);

        if (labels_cursor.col() < Address::MAX_COL) {
            labels_cursor = Address(labels_cursor.col() + 1, labels_cursor.row());
        }

        if (labels_cursor.row() < Address::MAX_ROW) {
            labels_cursor = Address(labels_cursor.col(), labels_cursor.row() + 1);
        }
    } while (proceed);
}

void UI::highlightCell(Address addr)
{
    attron(ACTIVE_CELL_COLOR);

    drawCellBorder(addr, CellState::ACTIVE);
    drawHeaderCellsBorder(addr, CellState::ACTIVE);

    attroff(ACTIVE_CELL_COLOR);
}

void UI::unhighlightCell(Address addr)
{
    attron(INACTIVE_CELL_COLOR);

    drawCellBorder(addr, CellState::INACTIVE);
    drawHeaderCellsBorder(addr, CellState::INACTIVE);

    attroff(INACTIVE_CELL_COLOR);
}

void UI::drawCellBorder(Address cell, CellState state)
{
    /* cell out of viewport */
    if (cell.col() > m_ViewportCols || cell.row() > m_ViewportRows) {
        return;
    }

    int topleft_col = m_VerticalHeaderCellWidth + (cell.col() - 1) * (m_CellWidth + 1);
    int topleft_row = 1 + (cell.row() - 1) * 2;

    /* corners */
    if (state == CellState::ACTIVE) {
        move(topleft_row, topleft_col);
        addch(ACS_ULCORNER);

        move(topleft_row, topleft_col + m_CellWidth + 1);
        addch(ACS_URCORNER);

        move(topleft_row + 2, topleft_col);
        addch(ACS_LLCORNER);

        move(topleft_row + 2, topleft_col + m_CellWidth + 1);
        addch(ACS_LRCORNER);
    } else {
        move(topleft_row, topleft_col);
        if (cell.col() == 1 && cell.row() == 1) {
            addch(ACS_PLUS);
        } else if (cell.col() == 1) {
            addch(ACS_RTEE);
        } else if (cell.row() == 1) {
            addch(ACS_BTEE);
        } else {
            addch(' ');
        }

        move(topleft_row, topleft_col + m_CellWidth + 1);
        if (cell.row() == 1) {
            addch(ACS_BTEE);
        } else {
            addch(' ');
        }

        move(topleft_row + 2, topleft_col);
        if (cell.col() == 1) {
            addch(ACS_RTEE);
        } else {
            addch(' ');
        }

        move(topleft_row + 2, topleft_col + m_CellWidth + 1);
        if (cell.row() == 1) {
            addch(ACS_BTEE);
        } else {
            addch(' ');
        }
    }

    /* left & right */
    move(topleft_row + 1, topleft_col);
    addch(ACS_VLINE);

    move(topleft_row + 1, topleft_col + m_CellWidth + 1);
    addch(ACS_VLINE);

    /* top & bottom */
    if (m_CellWidth > 0) {
        mvhline(topleft_row, topleft_col + 1, ACS_HLINE, m_CellWidth);
        mvhline(topleft_row + 2, topleft_col + 1, ACS_HLINE, m_CellWidth);
    }
}

void UI::drawHeaderCellsBorder(Address addr, CellState state)
{
    int header_col, header_row;

    /* horizontal header */
    header_col = m_VerticalHeaderCellWidth + (addr.col() - 1) * (m_CellWidth + 1);
    header_row = 0;

    move(header_row, header_col);
    addch(ACS_VLINE);

    move(header_row, header_col + m_CellWidth + 1);
    addch(ACS_VLINE);

    move(header_row + 1, header_col);
    if (state == CellState::ACTIVE) {
        if (addr.row() > 1) {
            addch(ACS_LLCORNER);
        } else {
            addch(ACS_LTEE);
        }
    } else {
        addch(ACS_BTEE);
    }

    move(header_row + 1, header_col + m_CellWidth + 1);
    if (state == CellState::ACTIVE) {
        if (addr.row() > 1) {
            addch(ACS_LRCORNER);
        } else {
            addch(ACS_RTEE);
        }
    } else {
        addch(ACS_BTEE);
    }

    mvhline(header_row + 1, header_col + 1, ACS_HLINE, m_CellWidth);

    /* vertical header */
    header_col = 0;
    header_row = 1 + (addr.row() - 1) * 2;

    mvhline(header_row, header_col, ACS_HLINE, m_VerticalHeaderCellWidth);
    mvhline(header_row + 2, header_col, ACS_HLINE, m_VerticalHeaderCellWidth);

    move(header_row, header_col + m_VerticalHeaderCellWidth);
    if (state == CellState::ACTIVE) {
        if (addr.col() > 1) {
            addch(ACS_URCORNER);
        } else {
            addch(ACS_TTEE);
        }
    } else {
        addch(ACS_RTEE);
    }

    move(header_row + 2, header_col + m_VerticalHeaderCellWidth);
    if (state == CellState::ACTIVE) {
        if (addr.col() > 1) {
            addch(ACS_LRCORNER);
        } else {
            addch(ACS_BTEE);
        }
    } else {
        addch(ACS_RTEE);
    }

    move(header_row + 1, header_col + m_VerticalHeaderCellWidth);
    addch(ACS_VLINE);

    if ((state == CellState::ACTIVE && addr.col() == 1 && addr.row() == 1) ||
        (state == CellState::INACTIVE && (addr.col() == 1 || addr.row() == 1))) {
        move(1, m_VerticalHeaderCellWidth);
        addch(ACS_PLUS);
    }
}
