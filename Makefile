ARTIFACT=filehash

configure:
	export CXX=/usr/bin/clang++ && cmake -S . -B build

compile: configure
	cmake --build build
	cp build/compile_commands.json compile_commands.json

test: compile
	(cd build; ctest)

run: compile
	(cd build; ./${ARTIFACT})

format:
	clang-format --dry-run --Werror src/* test/*

tidy:
	clang-tidy -p ./compile_commands.json test/*.cpp

clean:
	rm -rf build
	rm -rf .cache
	rm -rf compile_commands.json
