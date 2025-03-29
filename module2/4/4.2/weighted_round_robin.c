#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "priority_queue.h"

#define MIN_PRIORITY 0
#define MAX_PRIORITY 255

#define NUM_QUEUES 4
#define NUM_PACKETS 10

int getQueueIndex(TrafficType traffic_type) {
    switch (traffic_type) {
        case VOICE: return 0;
        case VIDEO: return 1;
        case WEB:   return 2;
        case OTHER: return 3;
        default:    return 3;
    }
}

void dispatchPacket(PriorityQueue* queues[NUM_QUEUES], Packet* packet) {
    int index = getQueueIndex(packet->traffic_type);
    pushPQ(queues[index], packet);
}

void generateTrafiic(PriorityQueue* queues[NUM_QUEUES]) {
    int total_traffic = NUM_PACKETS;
    int packet_ID = 1;

    while (total_traffic > 0) {
        Packet* packet = createPacket(
            packet_ID, 
            rand() % MAX_PRIORITY + MIN_PRIORITY,
            rand() % (OTHER + 1) + VOICE);

        dispatchPacket(queues, packet);
        total_traffic--;
    }
}

void weightedRoundRobinProcess(PriorityQueue* queues[NUM_QUEUES]) {
    // Голос (VOICE) – 4, Видео (VIDEO) – 3, Веб (WEB) – 2, Остальное (OTHER) – 1.
    unsigned int weights[NUM_QUEUES] = {
        (unsigned int)(NUM_PACKETS * 0.4), 
        (unsigned int)(NUM_PACKETS * 0.3), 
        (unsigned int)(NUM_PACKETS * 0.2), 
        (unsigned int)(NUM_PACKETS * 0.1)
    };
    
    int cycle = 1;
    int totalProcessed;
    
    do {
        totalProcessed = 0;
        printf("Цикл %d:\n", cycle);
        
        for (int i = 0; i < NUM_QUEUES; i++) {
            int packet_count = 0;
            while (packet_count < weights[i] && !isEmpty(queues[i])) {
                Packet* packet = popPQ(queues[i]);
                if (packet != NULL) {
                    printf("Обработка пакета #%d из очереди %s (приоритет=%d)\n",
                        packet->packed_ID,
                        (i == 0 ? "VOICE" : i == 1 ? "VIDEO" : i == 2 ? "WEB" : "OTHER"),
                        packet->priority);
                    removePacket(packet);
                }                             
                packet_count++;
                totalProcessed++;
            }
        }
        cycle++;
    } while (totalProcessed > 0);
}

int main()
{
    srand((unsigned)time(NULL));

    PriorityQueue* queues[NUM_QUEUES];
    for (int i = 0; i < NUM_QUEUES; i++) {
        queues[i] = createPQ();
    }

    generateTrafiic(queues);

    weightedRoundRobinProcess(queues);
    
    for (int i = 0; i < NUM_QUEUES; i++) {
        removePQ(queues[i]);
    }
    return 0;
}