all: build/while

build/while: build
	cd build; ninja install

build:
	mkdir -p build; cd build; cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=c++ -DCMAKE_CXX_STANDARD=20

fuzz:
	./build/while test -f


.PHONY: clean all build/while test
