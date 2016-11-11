# Nanocube2

This is a clean-room, *COMPLETELY WORK-IN-PROGRESS* reimplementation
of the [nanocubes](http://nanocubes.net) data structure. Less template
magic, more simplicity, a few new tricks up its sleeve.

## Dependencies

* Boost
* CMake

## Compiling

Run the following commands at the root level of the project dir:

```
mkdir build
cd build
cmake ..
make
cd ..
```
The executable will be in `./bin`

## Running a nanocube

```
cd bin
./ncserver
```

By default, a nanocube server will serve on port `8000`.

A simple web ui will be served at: `http://localhost:8000/`

The query json api is: `http://localhost:8000/query`
