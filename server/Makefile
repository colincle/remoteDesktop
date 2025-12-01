BUILD_DIR = build
SERVER = $(BUILD_DIR)/Server

.PHONY: all re clean run

all:
	./build.sh

re: clean all

clean:
	rm -rf $(BUILD_DIR)

run:
	@if [ ! -f $(SERVER) ]; then ./build.sh; fi
	./$(SERVER)
