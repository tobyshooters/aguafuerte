# fliptable

A key-value database stored as a single PNG image.

Keys are strings. Values are strings, integers, or images.
Text is rendered with a 3x6 bitmap font directly into RGB pixels.
The database file *is* the image; there is no separate format.

An SDL2 window displays the live database with a built-in REPL
rendered in the same bitmap font. The evaluator is a Forth-style
stack machine.

## Build

    ./install.sh    # install dependencies (SDL2, scrot, imagemagick)
    make
    make run

Requires SDL2 and a C99 compiler.

## Data model

Keys use dot-separated namespaces. The part after the last dot
is the local key; everything before it is the namespace.
Keys without a dot go into the `home` namespace.

    30 age SET                -> home.age = 30
    90 metrics.cpu SET        -> metrics.cpu = 90

Each namespace is a visual row in the image. The namespace
label column has uniform width across all rows.

Deleting a namespace deletes all sub-namespaces too:
`docs DEL` removes `docs`, `docs.math`, `docs.string`, etc.

## Persistence

Save writes the database image to `images/`. On load, the cell
structure (rows, keys, values, images) is reconstructed by
scanning the pixel data:

- Namespace labels are read from the left column
- Cell keys are decoded from the bitmap font
- Values are detected as text (gray pixels) or image data
- Image cells include a file path header above the pixel data

If an image cell's source file is missing from disk, it is
re-created from the pixels stored in the database image.

    db SAVE                   save to images/db.png
    mydb SAVE                 save to images/mydb.png
    db LOAD                   load and reconstruct cells

## Language

Forth-style postfix. Words are whitespace-separated.
Values are pushed onto a stack; commands pop their arguments.

    42                        push number
    "hello world"             push quoted string
    age                       push literal "age"
    @age                      fetch and execute stored value

Use `.` to print the top of the stack.

### Shorthand

`@key` fetches a key's value. For text values, it executes the
text as Forth code (like `GET EXEC`). For other types, it pushes
the value onto the stack (like `GET`).

    "dup *" sq SET
    4 @sq .                   -> 16
    "@sq @sq" quad SET
    3 @quad .                 -> 81

### Database

    name SAVE                 save db to images/name
    name LOAD                 load db from file

### Cells

    val key SET               set key to value
    key GET                   push value of key
    @key                      shorthand: fetch and execute
    key DEL                   delete key or namespace

### Images

    path READ                 load image or text file (.md, .txt)
    path READ key SET         store image under key
    SCREENSHOT                interactive screenshot (via scrot -s)
    img size RESIZE           resize image to size x size pixels

Image cells display three rows: key name, file path, and pixel
data. The file path enables re-linking to the source file for
operations like RESIZE.

### Arithmetic

    3 4 + .                   -> 7
    10 3 - .                  -> 7
    6 7 * .                   -> 42
    10 3 / .                  -> 3
    10 3 % .                  -> 1

### Strings

    a b CAT                   concatenate two values

### Stack

    DUP                       duplicate top
    DROP                      discard top
    SWAP                      swap top two

### Control

    EXEC                      pop text, execute as Forth
    .                         pop and print top of stack

### Procedures

    "2 *" double SET          store a procedure
    5 double GET EXEC .       -> 10
    5 @double .               -> 10 (shorthand)
    "@double @double" quad SET
    3 @quad .                 -> 81

### REPL

    quit / exit               close the program

The stack persists across lines, so `5` on one line
then `age SET` on the next works.

## Keyboard shortcuts

    Ctrl +/-                  scale images up/down (text stays fixed)
    Arrow keys / scroll       scroll the view vertically

## Files

    src/main.c      REPL + SDL2 window + keyboard input
    src/db.h/c      Database, cells, rows, PNG I/O, reconstruction
    src/eval.h/c    Forth-style stack evaluator
    src/glyph.h/c   3x6 bitmap font (read and write)
    deps/           stb_image headers
    install.sh      dependency installer
