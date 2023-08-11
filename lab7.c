#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <math.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>



#define FIFO_DATA_OFFSET 0
#define FIFO_DATA_COUNTER_OFFSET 1
#define FIFO_DATA_OFFSET 0
#define RADIO_TUNER_FAKE_ADC_PINC_OFFSET 0
#define RADIO_TUNER_TUNER_PINC_OFFSET 1
#define RADIO_TUNER_CONTROL_REG_OFFSET 2
#define RADIO_TUNER_TIMER_REG_OFFSET 3
#define FIFO_PERIPH_ADDRESS 0x43c10000
#define RADIO_PERIPH_ADDRESS 0x43c00000

// Global variables to store the tuner pinc values
int fake_adc_value = 30001000;
int tuner_value = 30000000;
char dest_ip[16];  // Assuming IPv4 address length
int num_packets = 10;  // Default number of packets



// Mutex for protecting shared variables
pthread_mutex_t value_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function to get a pointer to a physical address using /dev/mem
volatile unsigned int *get_a_pointer(unsigned int phys_addr) {
    int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    void *map_base = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, phys_addr);
    volatile unsigned int *radio_base = (volatile unsigned int *)map_base;
    return radio_base;
}
void radioTuner_tuneRadio(volatile unsigned int *ptrToRadio, float tune_frequency)
{
	float pinc = (-1.0*tune_frequency)*(float)(1<<27)/125.0e6;
	*(ptrToRadio+RADIO_TUNER_TUNER_PINC_OFFSET)=(int)pinc;
}

void radioTuner_setAdcFreq(volatile unsigned int* ptrToRadio, float freq)
{
	float pinc = freq*(float)(1<<27)/125.0e6;
	*(ptrToRadio+RADIO_TUNER_FAKE_ADC_PINC_OFFSET) = (int)pinc;
}

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
            // Calculate cosine (real part) and sine (imaginary part) values
            double angle = 2 * M_PI * counter / 256;
            int16_t i_value = htons((int16_t)(100 * cos(angle)));  // Signed 16-bit I value
            int16_t q_value = htons((int16_t)(100 * sin(angle)));  // Signed 16-bit Q value
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

// Thread function to handle user input
void *user_input_thread(void *arg) {
    char input;
    int ip_set = 0;  // Flag to track if the IP address has been set

    while (1) {
        printf("Press 'm' to show the menu.\n");
        scanf(" %c", &input);

        if (input == 'm') {
            printf("------------------------------------------------------\n");
            printf(" -- Press t to tune radio to a new frequency. \n");
            printf(" -- Press f to set the fake ADC to a new frequency. \n");
            printf(" -- Press 'U/u' to increase frequency by 1000/100 Hz. \n");
            printf(" -- Press 'D/d' to decrease frequency by 1000/100 Hz. \n");
            printf(" -- Press 'i' to set the destination IP address. \n");
            printf(" -- Press 'm' to show the menu again. \n");
            printf("------------------------------------------------------\n");
        }else if (input == 'i') {
            printf("Enter destination IP address: ");
            scanf("%15s", dest_ip);
            ip_set = 1;  // Set the flag to indicate IP address is set
        } else if (!ip_set) {
            printf("Please set the destination IP address first using 'i'\n");
        } else {
            pthread_mutex_lock(&value_mutex);
            if (input == 'f') {
                printf("Enter new ADC value: ");
                int new_value;
                scanf("%d", &new_value);
                fake_adc_value = new_value;
            } else if (input == 't') {
                printf("Enter new tuner value: ");
                int new_value;
                scanf("%d", &new_value);
                tuner_value = new_value;
            } else if (input == 'u') {
                fake_adc_value += 100;
                printf("Fake ADC value increased by 100\n");
            } else if (input == 'U') {
                fake_adc_value += 1000;
                printf("Fake ADC value increased by 1000\n");
            } else if (input == 'd') {
                fake_adc_value -= 100;
                printf("Fake ADC value decreased by 100\n");
            } else if (input == 'D') {
                fake_adc_value -= 1000;
                printf("Fake ADC value decreased by 1000\n");
            }
            printf("// --- Current ADC value: %d ---//\n", fake_adc_value);
            pthread_mutex_unlock(&value_mutex);
        }
    }

    return NULL;
}


int main(int argc, char *argv[]) {

    volatile unsigned int *my_periph = get_a_pointer(FIFO_PERIPH_ADDRESS);
    volatile unsigned int *my_radio_periph = get_a_pointer(RADIO_PERIPH_ADDRESS);
     // Write values to memory addresses
    // *(my_radio_periph +RADIO_TUNER_FAKE_ADC_PINC_OFFSET) = 30001000;
    // *(my_radio_periph + RADIO_TUNER_TUNER_PINC_OFFSET) = 30000000;
     // Create a thread for user input
    pthread_t input_thread;
    pthread_create(&input_thread, NULL, user_input_thread, NULL);

    while (1) {
        // Acquire the mutex to read the shared values
        pthread_mutex_lock(&value_mutex);
        *(my_radio_periph + RADIO_TUNER_FAKE_ADC_PINC_OFFSET) = fake_adc_value;
        *(my_radio_periph + RADIO_TUNER_TUNER_PINC_OFFSET) = tuner_value;
        pthread_mutex_unlock(&value_mutex);

         // Call send_udp_packets with the entered destination IP and number of packets
        //send_udp_packets(dest_ip, num_packets);

        // Sleep for a short interval to avoid excessive CPU usage
        usleep(100000);  // Sleep for 100 ms
    }

    
    




    // if (argc != 3) {
    //     fprintf(stderr, "Usage: %s <destination_ip> <num_packets>\n", argv[0]);
    //     exit(EXIT_FAILURE);
    // }

    // const char *dest_ip = argv[1];
    // int num_packets = atoi(argv[2]);

    // send_udp_packets(dest_ip, num_packets);




    return 0;
}
