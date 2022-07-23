# SingleInclude

As the name suggest, this is a small program to generate a single include file for C/C++.

In addition, it can also generate the dependency of header files

## How to build

Simply download the code and run `cmake`

### Note

This program depends on either `<format>` from std or an external `fmtlib` with a `fmt/format.h` in include path.

Since some compilers (i.e. `GCC`) do not provide `<format>` now, you may need to download [`fmtlib`](https://github.com/fmtlib/fmt) and add it to include path or pass the path through `cmake` with `-DWITH_FMTLIB=/path/to/your/fmtlib`

## Usage

Run `singleinclude --help` for details

## License

This project is published under MIT License

Copyright(C) 2022 dragon-archer
