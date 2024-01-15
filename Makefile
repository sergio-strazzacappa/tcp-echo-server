TARGET_EXEC := client server
BUILD_DIR   := ./build
SRC_DIR     := ./src

$(BUILD_DIR)/$(TARGET_EXEC): $(SRC_DIR)/client.cpp $(SRC_DIR)/server.cpp
	mkdir -p $(BUILD_DIR)
	g++ $(SRC_DIR)/server.cpp -o $(BUILD_DIR)/server
	g++ $(SRC_DIR)/client.cpp -o $(BUILD_DIR)/client

clean:
	rm -rf $(BUILD_DIR)
