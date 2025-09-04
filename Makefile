CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -pthread
TARGET_SERVER = video_server
TARGET_CLIENT = video_client
SOURCE_SERVER = video_server.cpp
SOURCE_CLIENT = video_client.cpp

# Default target
all: $(TARGET_SERVER) $(TARGET_CLIENT)

# Server compilation
$(TARGET_SERVER): $(SOURCE_SERVER)
	$(CXX) $(CXXFLAGS) -o $(TARGET_SERVER) $(SOURCE_SERVER)
	@echo "Server compiled successfully!"

# Client compilation
$(TARGET_CLIENT): $(SOURCE_CLIENT)
	$(CXX) $(CXXFLAGS) -o $(TARGET_CLIENT) $(SOURCE_CLIENT)
	@echo "Client compiled successfully!"

# Clean compiled files
clean:
	rm -f $(TARGET_SERVER) $(TARGET_CLIENT)
	@echo "Cleaned compiled files"

# Help target
help:
	@echo "Video Streaming Application - Build System"
	@echo "==========================================="
	@echo "Available targets:"
	@echo "  all          - Build both server and client"
	@echo "  video_server - Build server only"
	@echo "  video_client - Build client only"
	@echo "  clean        - Remove compiled files"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Running the applications:"
	@echo "  Server: ./video_server <port> <FCFS|RR>"
	@echo "  Client: ./video_client <server_ip> <port> <TCP|UDP> <480p|720p|1080p>"