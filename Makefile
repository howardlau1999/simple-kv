CC := g++
BUILD_DIR := build
SRC_DIR := src
INCLUDE_DIR := include
FLAGS := -g
GTEST_DIR := googletest
GTEST_INCLUDE_DIR := $(GTEST_DIR)/googletest/include
GTEST_LIB_DIR := $(GTEST_DIR)/build/lib
GTEST_FLAGS := -lgtest_main -lgtest -lpthread
INCLUDE := -I$(INCLUDE_DIR) -I$(GTEST_INCLUDE_DIR)
TEST_DIR := tests
TEST_BUILD_DIR := build/tests

$(TEST_BUILD_DIR)/%.o: $(TEST_DIR)/%.cc
	@mkdir -p $(TEST_BUILD_DIR)
	$(CC) $(FLAGS) $(INCLUDE) -c -o $@ $<

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cc $(INCLUDE_DIR)/%.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(FLAGS) $(INCLUDE) -c -o $@ $<

all: server client

server: $(BUILD_DIR)/acceptor.o $(BUILD_DIR)/btree.o $(BUILD_DIR)/channel.o $(BUILD_DIR)/connection.o $(BUILD_DIR)/eventloop.o $(BUILD_DIR)/networking.o $(BUILD_DIR)/poller.o $(BUILD_DIR)/server.o $(BUILD_DIR)/socket.o
	@mkdir -p $(BUILD_DIR)
	$(CC) $(FLAGS) $(INCLUDE) -o build/server $^

client: $(BUILD_DIR)/acceptor.o $(BUILD_DIR)/btree.o $(BUILD_DIR)/channel.o $(BUILD_DIR)/connection.o $(BUILD_DIR)/eventloop.o $(BUILD_DIR)/networking.o $(BUILD_DIR)/poller.o $(BUILD_DIR)/client.o $(BUILD_DIR)/socket.o
	@mkdir -p $(BUILD_DIR)
	$(CC) $(FLAGS) $(INCLUDE) -o build/client $^

test: $(BUILD_DIR)/btree.o $(TEST_BUILD_DIR)/btree_test.o
	@mkdir -p $(BUILD_DIR)
	$(CC) $(FLAGS) $(INCLUDE) -L$(GTEST_LIB_DIR) $^ $(GTEST_FLAGS) -o build/test
	@build/test

clean:
	rm -rf $(BUILD_DIR)