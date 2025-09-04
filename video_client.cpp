#include <iostream>
#include <string>
#include <cstring>
#include <chrono>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>

using namespace std;

// Message structure
struct Message {
    int type;
    int length;
    char content[1024];
};

class VideoStreamingClient {
private:
    string server_ip;
    int server_port;
    string mode;
    string resolution;

public:
    VideoStreamingClient(string ip, int port, string streaming_mode, string video_resolution)
        : server_ip(ip), server_port(port), mode(streaming_mode), resolution(video_resolution) {}

    bool connect_and_stream() {
        // Phase 1: Connection Phase (TCP)
        cout << "Starting connection phase..." << endl;
        
        int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (tcp_socket < 0) {
            cerr << "Error creating TCP socket" << endl;
            return false;
        }

        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port);
        inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);

        if (connect(tcp_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            cerr << "Error connecting to server" << endl;
            close(tcp_socket);
            return false;
        }

        cout << "Connected to server " << server_ip << ":" << server_port << endl;

        // Send Type 1 Request Message
        Message request;
        request.type = 1;
        string request_content = resolution + ":" + mode;
        strcpy(request.content, request_content.c_str());
        request.length = request_content.length();

        if (send(tcp_socket, &request, sizeof(Message), 0) < 0) {
            cerr << "Error sending request" << endl;
            close(tcp_socket);
            return false;
        }

        cout << "Sent request: " << resolution << " in " << mode << " mode" << endl;

        // Receive Type 2 Response Message
        Message response;
        if (recv(tcp_socket, &response, sizeof(Message), 0) <= 0) {
            cerr << "Error receiving response" << endl;
            close(tcp_socket);
            return false;
        }

        if (response.type != 2) {
            cout << "Invalid response type" << endl;
            close(tcp_socket);
            return false;
        }

        string response_content(response.content);
        cout << "Received video info: " << response_content << endl;

        // Parse bandwidth info
        size_t colon_pos = response_content.find(':');
        string bandwidth_str = response_content.substr(colon_pos + 1);
        int estimated_bandwidth = stoi(bandwidth_str);
        
        cout << "Estimated bandwidth: " << estimated_bandwidth << " bps" << endl;
        cout << "Connection phase completed. Starting streaming phase..." << endl;

        // Phase 2: Video Streaming Phase
        return stream_video(tcp_socket, estimated_bandwidth);
    }

private:
    bool stream_video(int socket_fd, int estimated_bandwidth) {
        auto start_time = chrono::high_resolution_clock::now();
        auto first_packet_time = chrono::high_resolution_clock::time_point::min();
        
        cout << "Starting video streaming in " << mode << " mode..." << endl;
        cout << "Resolution: " << resolution << endl;
        
        char buffer[1024];
        int total_received = 0;
        int packets_received = 0;
        
        // Set socket timeout for receiving
        struct timeval timeout;
        timeout.tv_sec = 30;  // 30 seconds timeout
        timeout.tv_usec = 0;
        setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        
        while (true) {
            int bytes_received = recv(socket_fd, buffer, sizeof(buffer), 0);
            
            if (bytes_received <= 0) {
                // End of stream or error
                break;
            }
            
            // Record first packet time for latency calculation
            if (first_packet_time == chrono::high_resolution_clock::time_point::min()) {
                first_packet_time = chrono::high_resolution_clock::now();
            }
            
            total_received += bytes_received;
            packets_received++;
            
            // Simulate video playback processing
            if (packets_received % 1000 == 0) {
                cout << "Received " << packets_received << " packets, " 
                     << total_received << " bytes" << endl;
            }
            
            // Small delay to simulate real-time processing
            this_thread::sleep_for(chrono::microseconds(50));
        }
        
        auto end_time = chrono::high_resolution_clock::now();
        
        // Calculate performance metrics
        auto total_duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
        auto latency = chrono::duration_cast<chrono::milliseconds>(first_packet_time - start_time);
        
        double throughput_mbps = (total_received * 8.0) / (total_duration.count() * 1000.0); // Mbps
        
        // Display performance results
        cout << "\n========== PERFORMANCE RESULTS ==========" << endl;
        cout << "Protocol: " << mode << endl;
        cout << "Resolution: " << resolution << endl;
        cout << "Total Duration: " << total_duration.count() << " ms" << endl;
        cout << "Latency (first packet): " << latency.count() << " ms" << endl;
        cout << "Total Data Received: " << total_received << " bytes" << endl;
        cout << "Packets Received: " << packets_received << endl;
        cout << "Throughput: " << throughput_mbps << " Mbps" << endl;
        cout << "Expected Bandwidth: " << estimated_bandwidth / 1000000.0 << " Mbps" << endl;
        
        // Calculate efficiency
        double efficiency = (throughput_mbps / (estimated_bandwidth / 1000000.0)) * 100.0;
        cout << "Bandwidth Efficiency: " << efficiency << "%" << endl;
        
        // Estimate packet loss for UDP
        if (mode == "UDP") {
            // Rough estimation based on expected vs actual data
            int expected_packets = estimated_bandwidth / 8192; // Rough estimate
            double estimated_loss = 0.0;
            if (expected_packets > packets_received) {
                estimated_loss = ((double)(expected_packets - packets_received) / expected_packets) * 100.0;
            }
            cout << "Estimated Packet Loss: " << estimated_loss << "%" << endl;
        }
        
        close(socket_fd);
        return true;
    }
};

// Function to display usage information
void print_usage(const char* program_name) {
    cout << "Video Streaming Client" << endl;
    cout << "Usage: " << program_name << " <Server IP> <Server Port> <Mode: TCP/UDP> <Resolution: 480p/720p/1080p>" << endl;
    cout << "\nExamples:" << endl;
    cout << "  " << program_name << " 127.0.0.1 8080 TCP 1080p" << endl;
    cout << "  " << program_name << " 192.168.1.100 8080 UDP 720p" << endl;
    cout << "\nModes:" << endl;
    cout << "  TCP - Reliable streaming with guaranteed delivery" << endl;
    cout << "  UDP - Real-time streaming with possible packet loss" << endl;
    cout << "\nResolutions:" << endl;
    cout << "  480p - Low quality (2 Mbps)" << endl;
    cout << "  720p - Medium quality (5 Mbps)" << endl;
    cout << "  1080p - High quality (8 Mbps)" << endl;
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        print_usage(argv[0]);
        return -1;
    }

    string server_ip = argv[1];
    int server_port = atoi(argv[2]);
    string mode = argv[3];
    string resolution = argv[4];

    // Validate input parameters
    if (mode != "TCP" && mode != "UDP") {
        cout << "Error: Invalid mode. Use TCP or UDP" << endl;
        return -1;
    }

    if (resolution != "480p" && resolution != "720p" && resolution != "1080p") {
        cout << "Error: Invalid resolution. Use 480p, 720p, or 1080p" << endl;
        return -1;
    }

    if (server_port <= 0 || server_port > 65535) {
        cout << "Error: Invalid port number. Use 1-65535" << endl;
        return -1;
    }

    cout << "Video Streaming Client" << endl;
    cout << "Server: " << server_ip << ":" << server_port << endl;
    cout << "Mode: " << mode << endl;
    cout << "Resolution: " << resolution << endl;
    cout << "------------------------" << endl;

    VideoStreamingClient client(server_ip, server_port, mode, resolution);
    
    if (!client.connect_and_stream()) {
        cout << "Failed to stream video" << endl;
        return -1;
    }

    cout << "Video streaming completed successfully!" << endl;
    return 0;
}