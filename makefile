# Make
MAKEFLAGS += --no-print-directory

# Compiler
CC	:= clang
CXX	:= clang++

# Project
PROJECT	!= grep "^project" CMakeLists.txt | cut -c9- | cut -d " " -f1
VERSION	!= grep "^project" CMakeLists.txt | cut -c9- | cut -d " " -f3

# Sources
SOURCES	!= find src -type f

# Targets
all: debug

run: debug
	build/llvm/debug/$(PROJECT)

test: debug
	@cd build/llvm/debug && ctest

debug: build/llvm/debug/CMakeCache.txt $(SOURCES)
	@cmake --build build/llvm/debug

release: build/llvm/release/CMakeCache.txt $(SOURCES)
	@cmake --build build/llvm/release

build/llvm/debug/CMakeCache.txt: CMakeLists.txt build/llvm/debug
	@cd build/llvm/debug && CC="$(CC)" CXX="$(CXX)" cmake -GNinja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX:PATH=../../.. ../../..

build/llvm/release/CMakeCache.txt: CMakeLists.txt build/llvm/release
	@cd build/llvm/release && CC="$(CC)" CXX="$(CXX)" cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=../../.. ../../..

build/llvm/debug:
	@mkdir -p build/llvm/debug

build/llvm/release:
	@mkdir -p build/llvm/release

install: release
	@cmake --build build/llvm/release --target install

clean:
	@rm -rf build/llvm

.PHONY: all run test debug release install clean
