#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_SAMPLES 256

int main() {
    int fifo_samples_waiting = 0; // Simulated value, replace with actual FIFO status
    int fifo_data[MAX_SAMPLES];   // Simulated FIFO data, replace with actual data source

    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket == -1) {
        perror("Error creating UDP socket");
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // Change to the target IP address
    server_addr.sin_port = htons(25344); // Change to the target port

    while (1) {
        // Simulate reading from FIFO and updating its status
        fifo_samples_waiting = rand() % (MAX_SAMPLES + 1);

        // Do whatever else needs to be done

        if (fifo_samples_waiting > MAX_SAMPLES) {
            int num_samples_to_read = MAX_SAMPLES;
            for (int i = 0; i < num_samples_to_read; ++i) {
                // Simulate reading from FIFO
                fifo_data[i] = rand(); // Replace with actual FIFO reading
            }

            // Construct UDP packet and send
            sendto(udp_socket, fifo_data, sizeof(int) * num_samples_to_read, 0,
                   (struct sockaddr*)&server_addr, sizeof(server_addr));

            // Update FIFO status
            fifo_samples_waiting -= num_samples_to_read;
        }

        // Add delay or sleep if needed
        usleep(100000); // Sleep for 100ms (adjust as necessary)
    }

    close(udp_socket);

    return 0;
}
