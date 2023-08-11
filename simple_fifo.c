#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>

#define FIFO_DATA_OFFSET 0
#define FIFO_DATA_COUNTER_OFFSET 1
#define FIFO_DATA_OFFSET 0
#define RADIO_TUNER_FAKE_ADC_PINC_OFFSET 0
#define RADIO_TUNER_TUNER_PINC_OFFSET 1
#define RADIO_TUNER_CONTROL_REG_OFFSET 2
#define RADIO_TUNER_TIMER_REG_OFFSET 3
#define FIFO_PERIPH_ADDRESS 0x43c10000
#define RADIO_PERIPH_ADDRESS 0x43c00000

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

int main() {
    volatile unsigned int *my_periph = get_a_pointer(FIFO_PERIPH_ADDRESS);
    volatile unsigned int *my_radio_periph = get_a_pointer(RADIO_PERIPH_ADDRESS);
     // Write values to memory addresses
    *(my_radio_periph +RADIO_TUNER_FAKE_ADC_PINC_OFFSET) = 30001000;
    *(my_radio_periph + RADIO_TUNER_TUNER_PINC_OFFSET) = 30000000;

    int total_samples_to_read = 480000;
    int samples_read = 0;
     printf("Playing 30MHz tune...\n");
    printf("Hello, I am going to read 10 seconds worth of data now...\n");

    while (samples_read < total_samples_to_read) {
        // Read FIFO count
        unsigned int fifo_count = *(my_periph + FIFO_DATA_COUNTER_OFFSET);

        if (fifo_count >= 1) {
            // Read FIFO data
            uint32_t data = *(my_periph + FIFO_DATA_OFFSET);
            samples_read++;
        }
    }

    printf("Finished!\n");


    return 0;
}
