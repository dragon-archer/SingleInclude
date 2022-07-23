# SingleInclude

As the name suggest, this is a small program to generate a single include file for C/C++

## Usage

Simply download the code and run `cmake`

## Note

This program depends on either `<format>` from std or an external `fmtlib` with a `fmt/format.h` in include path.

Since some compilers (i.e. `GCC`) do not provide `<format>` now, you may need to download [`fmtlib`](https://github.com/fmtlib/fmt) and add it to include path

## License

This project is published under MIT License

Copyright(C) 2022 dragon-archer
