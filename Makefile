.PHONY: cmake build verbose test clean
default: build

cmake:
	mkdir -p build
	cd build; CXX=clang++ CC=clang cmake ..

cmake_debug:
	mkdir -p build
	cd build; CXX=clang++ CC=clang cmake .. -DCMAKE_BUILD_TYPE=Debug

build: cmake
	cd build; make -j8

build_debug: cmake_debug
	cd build; make -j8

verbose: cmake
	cd build; VERBOSE=1 make -j8

verbose_debug: cmake_debug
	cd build; VERBOSE=1 make -j8

test: build
	build/statorange_unit_tests --reporter=spec

test_debug: build_debug
	build/statorange_unit_tests --reporter=spec

run: build
	build/statorange

run_debug: build_debug
	cgdb build/statorange

install:
	cd build; make -j8
	cd build; sudo make install

clean:
	rm -rf build
