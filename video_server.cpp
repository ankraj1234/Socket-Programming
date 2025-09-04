#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <atomic>
#include <map>
#include <random>
#include <algorithm>

using namespace std;

// Message structure
struct Message {
    int type;
    int length;
    char content[1024];
};

// Client info structure
struct ClientInfo {
    int socket_fd;
    string resolution;
    string mode;
    chrono::high_resolution_clock::time_point arrival_time;
    int client_id;
    bool active;
    
    ClientInfo() : active(true) {}
};

// Video stream parameters
map<string, int> video_bandwidth = {
    {"480p", 2000000},   // 2 Mbps
    {"720p", 5000000},   // 5 Mbps
    {"1080p", 8000000}   // 8 Mbps
};

map<string, int> video_size = {
    {"480p", 50000000},   // 50 MB
    {"720p", 120000000},  // 120 MB
    {"1080p", 200000000}  // 200 MB
};

class VideoStreamingServer {
private:
    int server_port;
    string scheduling_policy;
    int server_socket;
    queue<ClientInfo> request_queue;
    mutex queue_mutex;
    condition_variable queue_cv;
    atomic<bool> running;
    atomic<int> client_counter;
    vector<thread> worker_threads;
    
    // Round Robin specific
    vector<ClientInfo> rr_clients;
    mutex rr_mutex;
    atomic<int> rr_current_index;
    thread rr_thread;

public:
    VideoStreamingServer(int port, string policy) 
        : server_port(port), scheduling_policy(policy), running(true), 
          client_counter(0), rr_current_index(0) {}

    bool initialize() {
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0) {
            cerr << "Error creating socket" << endl;
            return false;
        }

        int opt = 1;
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        // Increased socket timeouts to prevent connection issues
        struct timeval timeout;
        timeout.tv_sec = 600;  // 10 minutes timeout (initially 30seconds)
        timeout.tv_usec = 0;
        setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(server_socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(server_port);

        if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            cerr << "Error binding socket" << endl;
            return false;
        }

        if (listen(server_socket, 20) < 0) { 
            cerr << "Error listening on socket" << endl;
            return false;
        }

        cout << "Server initialized on port " << server_port << endl;
        cout << "Scheduling policy: " << scheduling_policy << endl;
        return true;
    }

    void start() {
        if (scheduling_policy == "FCFS") {
            // More workers for FCFS to handle clients concurrently
            int num_workers = 4; 
            for (int i = 0; i < num_workers; i++) {
                worker_threads.emplace_back(&VideoStreamingServer::fcfs_worker_thread, this);
            }
            cout << "Started " << num_workers << " FCFS worker threads" << endl;
        } 
        else { // Round Robin
            rr_thread = thread(&VideoStreamingServer::round_robin_scheduler, this);
            cout << "Started Round Robin scheduler" << endl;
        }

        // Main accept loop
        while (running) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
            
            if (client_socket < 0) {
                if (running) {
                    cerr << "Error accepting client connection" << endl;
                    continue; // Continue accepting other clients instead of shutting down
                } else {
                    break;
                }
            }

            cout << "New client connected: " << inet_ntoa(client_addr.sin_addr) << endl;
            
            // Handle connection phase in separate thread to avoid blocking
            thread(&VideoStreamingServer::handle_connection_phase, this, client_socket).detach();
        }
    }

    void handle_connection_phase(int client_socket) {
        // Set socket options for this client
        int keepalive = 1;
        setsockopt(client_socket, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive));
        
        // Set longer timeouts for send/receive to prevent disconnections
        struct timeval timeout;
        timeout.tv_sec = 600;  // 10 minutes timeout
        timeout.tv_usec = 0;
        setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(client_socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
        
        Message request, response;
        
        // Receive Type 1 Request Message
        int recv_result = recv(client_socket, &request, sizeof(Message), 0);
        if (recv_result <= 0) {
            cout << "Failed to receive request from client: " << (recv_result == 0 ? "Connection closed" : strerror(errno)) << endl;
            close(client_socket);
            return;
        }

        if (request.type != 1) {
            cout << "Invalid request type: " << request.type << endl;
            close(client_socket);
            return;
        }

        // Parse request content (resolution:mode)
        string content(request.content);
        size_t colon_pos = content.find(':');
        if (colon_pos == string::npos) {
            cout << "Invalid request format: " << content << endl;
            close(client_socket);
            return;
        }
        
        string resolution = content.substr(0, colon_pos);
        string mode = content.substr(colon_pos + 1);

        // Validate resolution
        if (video_bandwidth.find(resolution) == video_bandwidth.end()) {
            cout << "Unsupported resolution: " << resolution << endl;
            close(client_socket);
            return;
        }

        cout << "Client " << (client_counter + 1) << " requests: " << resolution << " in " << mode << " mode" << endl;

        // Send Type 2 Response Message
        response.type = 2;
        string response_content = resolution + ":" + to_string(video_bandwidth[resolution]);
        strcpy(response.content, response_content.c_str());
        response.length = response_content.length();

        int send_result = send(client_socket, &response, sizeof(Message), 0);
        if (send_result <= 0) {
            cout << "Failed to send response to client: " << strerror(errno) << endl;
            close(client_socket);
            return;
        }
        
        cout << "Sent video info to client " << (client_counter + 1) << endl;

        // Create client info
        ClientInfo client_info;
        client_info.socket_fd = client_socket;
        client_info.resolution = resolution;
        client_info.mode = mode;
        client_info.arrival_time = chrono::high_resolution_clock::now();
        client_info.client_id = ++client_counter;
        client_info.active = true;

        // Add to appropriate queue based on scheduling policy
        if (scheduling_policy == "FCFS") {
            {
                lock_guard<mutex> lock(queue_mutex);
                request_queue.push(client_info);
            }
            queue_cv.notify_one();
            cout << "Added client " << client_info.client_id << " to FCFS queue" << endl;
        } else { // Round Robin
            {
                lock_guard<mutex> lock(rr_mutex);
                rr_clients.push_back(client_info);
            }
            cout << "Added client " << client_info.client_id << " to RR queue" << endl;
        }
    }

    void fcfs_worker_thread() {
        while (running) {
            ClientInfo client;
            
            {
                unique_lock<mutex> lock(queue_mutex);
                queue_cv.wait(lock, [this] { return !request_queue.empty() || !running; });
                
                if (!running) break;
                
                client = request_queue.front();
                request_queue.pop();
            }

            cout << "FCFS Worker picked up client " << client.client_id << endl;
            handle_streaming_phase(client);
        }
    }

    void round_robin_scheduler() {
        const int time_quantum_ms = 1000; // 1 second time quantum
        
        while (running) {
            vector<ClientInfo> active_clients;
            
            {
                lock_guard<mutex> lock(rr_mutex);
                // Copy active clients
                for (const auto& client : rr_clients) {
                    if (client.active) {
                        active_clients.push_back(client);
                    }
                }
            }
            
            if (active_clients.empty()) {
                this_thread::sleep_for(chrono::milliseconds(100));
                continue;
            }
            
            // Process each client for time quantum
            for (auto& client : active_clients) {
                if (!running) break;
                
                cout << "RR: Processing client " << client.client_id << " for " << time_quantum_ms << "ms" << endl;
                
                // Stream for time quantum
                bool completed = handle_streaming_phase_quantum(client, time_quantum_ms);
                
                if (completed) {
                    // Mark client as inactive and remove from RR queue
                    {
                        lock_guard<mutex> lock(rr_mutex);
                        for (auto& rr_client : rr_clients) {
                            if (rr_client.client_id == client.client_id) {
                                rr_client.active = false;
                                break;
                            }
                        }
                    }
                    cout << "Client " << client.client_id << " streaming completed" << endl;
                }
            }
            
            // Clean up inactive clients
            {
                lock_guard<mutex> lock(rr_mutex);
                rr_clients.erase(
                    remove_if(rr_clients.begin(), rr_clients.end(), 
                              [](const ClientInfo& c) { return !c.active; }),
                    rr_clients.end()
                );
            }
        }
    }

    bool handle_streaming_phase_quantum(ClientInfo& client, int quantum_ms) {
        static map<int, int> client_progress; // Track progress for each client
        static mutex progress_mutex;
        
        int total_data = video_size[client.resolution];
        int chunk_size = 1024; // 1KB chunks
        int total_chunks = total_data / chunk_size;
        
        // Get current progress
        int chunks_sent;
        {
            lock_guard<mutex> lock(progress_mutex);
            chunks_sent = client_progress[client.client_id];
        }
        
        auto start_time = chrono::high_resolution_clock::now();
        auto quantum_end = start_time + chrono::milliseconds(quantum_ms);
        
        // Simulate packet loss for UDP
        random_device rd;
        mt19937 gen(rd());
        uniform_real_distribution<> dis(0.0, 1.0);
        double packet_loss_rate = (client.mode == "UDP") ? 0.05 : 0.0;
        
        char data_buffer[chunk_size];
        memset(data_buffer, 'V', chunk_size);
        
        // Send chunks within time quantum
        while (chunks_sent < total_chunks && chrono::high_resolution_clock::now() < quantum_end) {
            // Simulate packet loss for UDP
            if (client.mode == "UDP" && dis(gen) < packet_loss_rate) {
                chunks_sent++; // Count as attempted
                continue;
            }
            
            int bytes_sent = send(client.socket_fd, data_buffer, chunk_size, 0);
            if (bytes_sent > 0) {
                chunks_sent++;
            } 
            else if (bytes_sent < 0) {
                cout << "Send error for client " << client.client_id << ": " << strerror(errno) << endl;
                close(client.socket_fd);
                {
                    lock_guard<mutex> lock(progress_mutex);
                    client_progress.erase(client.client_id);
                }
                return true; // Consider as completed due to error
            }
            
            // Simulate streaming delay
            this_thread::sleep_for(chrono::microseconds(100));
        }
        
        // Update progress
        {
            lock_guard<mutex> lock(progress_mutex);
            client_progress[client.client_id] = chunks_sent;
        }
        
        // Check if completed
        if (chunks_sent >= total_chunks) {
            auto end_time = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - client.arrival_time);
            
            // Calculate performance metrics
            double actual_data_sent = chunks_sent * chunk_size;
            double throughput_mbps = (actual_data_sent * 8.0) / (duration.count() * 1000.0);
            double packet_loss_percent = ((double)(total_chunks - chunks_sent) / total_chunks) * 100.0;
            
            cout << "=== RR Streaming completed for client " << client.client_id << " ===" << endl;
            cout << "Duration: " << duration.count() << " ms" << endl;
            cout << "Throughput: " << throughput_mbps << " Mbps" << endl;
            cout << "Packet loss: " << packet_loss_percent << "%" << endl;
            cout << "Protocol: " << client.mode << endl;
            cout << "Resolution: " << client.resolution << endl;
            cout << "=============================================" << endl;
            
            close(client.socket_fd);
            {
                lock_guard<mutex> lock(progress_mutex);
                client_progress.erase(client.client_id);
            }
            return true;
        }
        
        return false;
    }

    void handle_streaming_phase(ClientInfo& client) {
        cout << "=== Starting streaming for client " << client.client_id 
             << " (" << client.resolution << ", " << client.mode << ") ===" << endl;

        auto start_time = chrono::high_resolution_clock::now();
        int total_data = video_size[client.resolution];
        int chunk_size = 1024; // 1KB chunks
        int chunks_sent = 0;
        int total_chunks = total_data / chunk_size;

        // Simulate packet loss for UDP
        random_device rd;
        mt19937 gen(rd());
        uniform_real_distribution<> dis(0.0, 1.0);
        double packet_loss_rate = (client.mode == "UDP") ? 0.05 : 0.0; // 5% loss for UDP

        char data_buffer[chunk_size];
        memset(data_buffer, 'V', chunk_size); // Fill with 'V' for video data

        cout << "Sending " << total_chunks << " chunks to client " << client.client_id << endl;

        for (int i = 0; i < total_chunks; i++) {
            if (client.mode == "UDP" && dis(gen) < packet_loss_rate) {
                // Simulating Packet lost
                continue;
            }

            int bytes_sent = send(client.socket_fd, data_buffer, chunk_size, 0);
            if (bytes_sent > 0) {
                chunks_sent++;
                
                // Print progress every 10000 chunks(debugging purposes)
                // if (chunks_sent % 10000 == 0) {
                //     double progress = (double)chunks_sent / total_chunks * 100.0;
                //     cout << "Client " << client.client_id << " progress: " << progress << "%" << endl;
                // }
            } 
            else if (bytes_sent < 0) {
                cout << "Send error for client " << client.client_id << ": " << strerror(errno) << endl;
                break; 
            }

            // Simulate streaming delay
            this_thread::sleep_for(chrono::microseconds(50)); // Reduced delay for faster streaming
        }

        auto end_time = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
        
        // Calculate performance metrics
        double actual_data_sent = chunks_sent * chunk_size;
        double throughput_mbps = (actual_data_sent * 8.0) / (duration.count() * 1000.0); // Mbps
        double packet_loss_percent = ((double)(total_chunks - chunks_sent) / total_chunks) * 100.0;

        cout << "=== FCFS Streaming completed for client " << client.client_id << " ===" << endl;
        cout << "Duration: " << duration.count() << " ms" << endl;
        cout << "Chunks sent: " << chunks_sent << "/" << total_chunks << endl;
        cout << "Throughput: " << throughput_mbps << " Mbps" << endl;
        cout << "Packet loss: " << packet_loss_percent << "%" << endl;
        cout << "Protocol: " << client.mode << endl;
        cout << "Resolution: " << client.resolution << endl;
        cout << "=============================================" << endl;

        close(client.socket_fd);
    }

    void stop() {
        cout << "Stopping server..." << endl;
        running = false;
        close(server_socket);
        queue_cv.notify_all();
        
        for (auto& t : worker_threads) {
            if (t.joinable()) {
                t.join();
            }
        }
        
        if (rr_thread.joinable()) {
            rr_thread.join();
        }
        cout << "Server stopped." << endl;
    }
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cout << "Usage: " << argv[0] << " <Server Port> <Scheduling Policy: FCFS/RR>" << endl;
        return -1;
    }

    int port = atoi(argv[1]);
    string policy = string(argv[2]);

    if (policy != "FCFS" && policy != "RR") {
        cout << "Invalid scheduling policy. Use FCFS or RR" << endl;
        return -1;
    }

    VideoStreamingServer server(port, policy);
    
    if (!server.initialize()) {
        return -1;
    }

    cout << "Video Streaming Server starting..." << endl;
    server.start();

    return 0;
}