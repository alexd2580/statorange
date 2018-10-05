.PHONY: cmake all verbose test clean

cmake:
	mkdir -p build
	cd build; CXX=clang++ CC=clang cmake ..

all: cmake
	cd build; make -j2

verbose: cmake
	cd build; VERBOSE=1 make -j2

test: all
	build/statorange_unit_tests --reporter=spec

clean:
	rm -rf build
