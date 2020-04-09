CC := g++
BUILD_DIR := build
SRC_DIR := src
INCLUDE_DIR := include
INCLUDE := -I$(INCLUDE_DIR)
FLAGS := -g
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cc $(INCLUDE_DIR)/%.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(FLAGS) $(INCLUDE) -c -o $@ $<

all: server client

server: $(BUILD_DIR)/acceptor.o $(BUILD_DIR)/btree.o $(BUILD_DIR)/channel.o $(BUILD_DIR)/connection.o $(BUILD_DIR)/eventloop.o $(BUILD_DIR)/networking.o $(BUILD_DIR)/poller.o $(BUILD_DIR)/server.o $(BUILD_DIR)/socket.o
	mkdir -p build
	$(CC) -g -I$(INCLUDE_DIR) -o build/server $^

client: $(BUILD_DIR)/acceptor.o $(BUILD_DIR)/btree.o $(BUILD_DIR)/channel.o $(BUILD_DIR)/connection.o $(BUILD_DIR)/eventloop.o $(BUILD_DIR)/networking.o $(BUILD_DIR)/poller.o $(BUILD_DIR)/client.o $(BUILD_DIR)/socket.o
	mkdir -p build
	$(CC) -g -I$(INCLUDE_DIR) -o build/client $^

clean:
	rm -rf $(BUILD_DIR)