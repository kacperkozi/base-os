BUILD_DIR ?= build

all:
	cmake -B $(BUILD_DIR) .
	cmake --build $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean