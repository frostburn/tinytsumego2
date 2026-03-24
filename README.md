# tinytsumego2

Algorithms for solving tiny go problems.

Successor to [tinytsumego](https://github.com/frostburn/tinytsumego).

## Prerequisites
Pull the JKISS submodule:
```bash
git submodule init
git submodule update
```

Install GCC:
```bash
sudo apt install build-essential
```

Install CMake:
```bash
sudo apt install cmake
```

## Compilation

```bash
mkdir build; cd build
cmake ..
make
make test
```


## Linting and formatting

This project uses `clang-format` for C source and header formatting. The `jkiss` submodule is third-party code and is intentionally excluded from linting commands.

Install it (Ubuntu/Debian):
```bash
sudo apt install clang-format
```

Check formatting from the `build` directory after configuring CMake:
```bash
cmake ..
make lint
```

`make lint` checks C and header files in this repository and excludes both the `jkiss` submodule and your build tree.

If you want to apply formatting in-place from the `build` directory, run:
```bash
make format
```

## Running the server
To interface with [vue-tsumego](https://github.com/frostburn/vue-tsumego) the Python HTTP bridge needs some pre-solved tsumego to serve.

To generate the public collection under `/tmp/` run this in the `build` directory:
```bash
./bin/generate_collections /tmp/
```

Start the server in development mode:
```bash
cd ../python
python3 main.py /tmp/ --dev
```

Copy the `.bin` files for safe-keeping to skip generating in the future.
