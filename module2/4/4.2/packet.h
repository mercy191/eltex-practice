#pragma once

#include <stdio.h>
#include <stdlib.h>

typedef enum {
    VOICE,
    VIDEO,
    WEB,
    OTHER
} TrafficType;

typedef struct Packet
{
    unsigned int packed_ID;
    unsigned int priority;
    TrafficType traffic_type;
} Packet;

Packet* createPacket(unsigned int packet_ID, unsigned int priority, TrafficType traffic_type);

void removePacket(Packet* packet);

Packet* createPacket(unsigned int packet_ID, unsigned int priority, TrafficType traffic_type) 
{
    Packet* new_packet = (Packet*)malloc(sizeof(Packet));
    if (new_packet == NULL)
        return NULL;

    new_packet->packed_ID = packet_ID;
    new_packet->priority = priority;
    new_packet->traffic_type = traffic_type;

    return new_packet;
}

void removePacket(Packet* packet) 
{
    if (packet == NULL)
        return;

    free(packet);
}