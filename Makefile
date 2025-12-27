clean:
	rm -rf build

build:
	cmake -S . -B build -G Ninja
	cmake --build build

run:
	./build/task-manager
