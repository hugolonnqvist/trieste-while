# While-Trieste

This project implements a compiler for the While language in Trieste. 

The main focus is on dataflow analysis and a framework for this is developed. The implemented analyses are zero analysis, constant propagation and liveness analysis. These are used in optimisation rewrite passes, such as constant folding and dead code elimination. 

## Compilation and running the executables
The project is compiled by running:

```
make
```

This creates two executable files `./build/while` and `./build/while_trieste`.

The first one is used if you want to run static analysis or evaluating the program.
The second one only runs the parsing passes and the normalisation pass.

## Benchmarking
Its possible to run a benchmarking script, executing the analyses on randomized programs.
To execute it run:
```
./stats
```
In the script both the number of runs and lines of code generated can be specified.
