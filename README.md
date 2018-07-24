# crosswords-generator
Crosswords generator based on Gecode Constraint Programming framework.

## Dependencies
This generator relies on constraint programming and the source is built on top of Gecode 6.0.1 [1].
Cloning Gecode repo and running `make configure && make install` with adequate privileges should suffice to successfully compile this project.

[1](https://github.com/Gecode/gecode)

## Compiling
If it's your first time compiling this project, you'll need to `make build`.

To get the binary, simply `make`.

## Running
### Dictionary files
Crosswords-generator needs a word collection, stored as a newline-separated plain text file named `dict` in the same directory as the binary.
One can also impose some words to appear in the grid by creating a file named `mandatory` that would contain the list of mandatory words, using the same format.

### And voila!
To run the program, `./crosswords`

It should take up to a few minutes to get a solution. Search is random-based with a seed that depends on the clock. Within a single run, you'll get very similar grids, so if you want completely different solutions, you might want to exit the program and run it again.

## Runtime requirements
Without altering the source file, the algorithm should run on four threads.
Depending on your word collection, hardware requirements may vary. For instance, for 200k words you would need 4GB ram.

## Theory and internal structures
TODO!
