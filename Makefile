.PHONY: cmake build verbose test clean
default: build

cmake:
	mkdir -p build
	cd build; CXX=clang++ CC=clang cmake ..

build: cmake
	cd build; make -j2

verbose: cmake
	cd build; VERBOSE=1 make -j2

test: build
	build/statorange_unit_tests --reporter=spec

run: build
	build/statorange

install: test
	cd build; make install

clean:
	rm -rf build
