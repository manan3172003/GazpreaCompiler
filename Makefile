.PHONY: build format format-kunal kunal emit-llvm compile run-llvm test run
build: format
	cd build/ && cmake .. && make -j 8 && cd -

format:
	find src include -type f \( -name "*.cpp" -o -name "*.h" \) -exec clang-format -i {} +

format-kunal:
	find src include -type f \( -name "*.cpp" -o -name "*.h" \) -exec clang-format-21 -i {} +

kunal: format-kunal
	cd build/ && cmake .. && make -j 8 && cd -

emit-llvm:
	clang++ -O0 -S -emit-llvm runtime-tests/$(filename) -o runtime-tests/$(filename).ll

compile: build
	 ./bin/gazc runtime-tests/$(file).in runtime-tests/$(file).ll

run-llvm:
	lli --dlopen=bin/libgazrt.dylib runtime-tests/$(file).ll

run-llvm-lab:
	LD_LIBRARY_PATH=./bin ./runtime-tests/$(file)

compile-llvm: compile
	llc -filetype=obj -relocation-model=pic runtime-tests/$(file).ll -o runtime-tests/$(file).o
	clang runtime-tests/$(file).o -o runtime-tests/$(file) -L./bin -lgazrt -lm

valgrind: compile-llvm
	LD_LIBRARY_PATH=./bin valgrind --leak-check=full --error-exitcode=111 runtime-tests/$(file)

test: build
	cd tests/ && dragon-runner GazpreaCompileConfig.json -v && cd -

test-lab: build
	cd tests/ && dragon-runner memcheck LabMachineConfig.json -v && cd -

file:
	touch include/$(path).h
	touch src/$(path).cpp

run: build compile run-llvm
run-lab: build compile-llvm run-llvm-lab