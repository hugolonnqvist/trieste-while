CC=c++
MIN_CMAKE_STANDARD=3.5

all: build/while

run:
	./build/while build ./examples/$(program).while && cat ./$(program).trieste

build/while: build
	cd build; ninja install

build:
	mkdir -p build; cd build; cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=$(CC) -DCMAKE_CXX_STANDARD=20 -DCMAKE_POLICY_VERSION_MINIMUM=$(MIN_CMAKE_STANDARD) -DCMAKE_EXPORT_COMPILE_COMMANDS=1

fuzz:
	./build/while test -f

clean:
	rm flamegraph.svg gmon.out out.perf folded.perf perf.data
	rm *.trieste

.PHONY: clean all build/while test
