clean:
	rm -rf build

build:
	cmake -S . -B build
	cmake --build build
	cd build && make

run:
	./build/task-manager
