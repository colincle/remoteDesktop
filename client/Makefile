BUILD_DIR = build
CLIENT = $(BUILD_DIR)/Client

.PHONY: all re clean run

all:
	./build.sh

re: clean all

clean:
	rm -rf $(BUILD_DIR)

run:
	@if [ ! -f $(CLIENT) ]; then ./build.sh; fi
	./$(CLIENT)
