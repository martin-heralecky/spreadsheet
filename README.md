# spreadsheet

This is a school project. It is not intended for regular use as there are lots of unaddressed issues and todos.

## Intro

spreadsheet is a very simple implementation of a table editor - spreadsheet - which runs in terminal. It supports features like cell data types, nested formulas, saving to and loading from a file. The project is written in C++ with ncurses library.

<p align="center">
    <img src="preview.gif" alt="preview">
</p>

## Compiling the source
```shell
make
```

## Control
`up`, `down`, `left`, `right`, `pg-up`, `pg-down` to move cell-cursor.

`enter` to start editing the selected cell. `enter` again to confirm and leave.

`:l <filename>` or `:load <filename>` to load sheet from file.

`:w <filename>` or `:write <filename>` to save the sheet to file.

`:q` or `:quit` to exit.

## License
MIT - feel free to use this code in any way as long as you keep the copyright notice.
