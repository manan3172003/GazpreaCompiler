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

compile:
	 ./bin/gazc runtime-tests/$(file).in runtime-tests/$(file).ll

run-llvm:
	lli runtime-tests/$(file).ll

test: build
	cd tests/ && dragon-runner GazpreaCompileConfig.json -v && cd -

file:
	touch include/$(path).h
	touch src/$(path).cpp

run: build compile run-llvm