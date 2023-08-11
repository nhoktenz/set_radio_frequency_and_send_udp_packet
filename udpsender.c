#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

void send_udp_packets(const char *dest_ip, int num_packets) {
    const char *UDP_IP = dest_ip;
    const int UDP_PORT = 25344;
    const int PACKET_SIZE = 1026;

    // Create a UDP socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket creation");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(UDP_PORT);
    if (inet_pton(AF_INET, UDP_IP, &dest_addr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }

    for (int counter = 0; counter < num_packets; ++counter) {
        // Create a 16-bit unsigned counter value (little endian)
        uint16_t counter_bytes = htons(counter);

        // Create sample data (16-bit signed I and Q values) for each packet
        char sample_data[PACKET_SIZE - 2];  // 2 bytes for the counter
        for (int i = 0; i < 256; ++i) {
            // Create fake I and Q values (you can modify this as needed)
            int16_t i_value = htons(100);  // Signed 16-bit I value
            int16_t q_value = htons(-100); // Signed 16-bit Q value
            memcpy(sample_data + 4 * i, &i_value, 2);
            memcpy(sample_data + 4 * i + 2, &q_value, 2);
        }

        // Create the complete packet by concatenating counter and sample data
        char packet[PACKET_SIZE];
        memcpy(packet, &counter_bytes, 2);
        memcpy(packet + 2, sample_data, sizeof(sample_data));

        // Send the packet
        sendto(sock, packet, sizeof(packet), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    }

    //close(sock);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <destination_ip> <num_packets>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *dest_ip = argv[1];
    int num_packets = atoi(argv[2]);

    send_udp_packets(dest_ip, num_packets);

    return 0;
}
