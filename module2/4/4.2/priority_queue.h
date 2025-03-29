#pragma once

#include <stdio.h>
#include <stdlib.h>
#include "packet.h"


typedef struct PQNode {
    Packet* packet;      
    struct PQNode* next;
} PQNode;

typedef struct {
    PQNode* head;
    unsigned int size;
} PriorityQueue;

PriorityQueue* createPQ();

void removePQ(PriorityQueue* pq);

PQNode* createPQNode();

void removePQNode(PQNode* node);

PQNode* pushPQ(PriorityQueue* pq, Packet* packet);

Packet* popPQ(PriorityQueue* pq);

int isEmpty(PriorityQueue* pq);


PriorityQueue* createPQ()
{
    PriorityQueue *pq = (PriorityQueue *)malloc(sizeof(PriorityQueue));
    if (pq == NULL)
        return NULL;

    pq->head = NULL;
    pq->size = 0;
    return pq;
}

void removePQ(PriorityQueue* pq)
{
    if (pq == NULL)
        return;

    PQNode *current = pq->head;
    PQNode *next_node;

    while (current != NULL)
    {
        next_node = current->next;
        removePQNode(current);
        current = next_node;
    }

    free(pq);
    return;
}

PQNode* createPQNode()
{
    PQNode* new_node = (PQNode*)malloc(sizeof(PQNode));
    if (new_node == NULL)
        return NULL;

    new_node->packet = NULL;
    new_node->next = NULL;
    return new_node;
}

void removePQNode(PQNode* node)
{
    if (node == NULL)
        return;

    removePacket(node->packet);
    free(node);
}

PQNode* pushPQ(PriorityQueue* pq, Packet* packet) 
{
    PQNode* new_node = createPQNode();
    if (new_node == NULL)
        return NULL;
    new_node->packet = packet;
    new_node->next = NULL;
    
    if (pq->head == NULL || pq->head->packet->priority > packet->priority) {
        new_node->next = pq->head;
        pq->head = new_node;
    } else {
        PQNode* current = pq->head;
        while (current->next != NULL && current->next->packet->priority <= packet->priority) {
            current = current->next;
        }
        new_node->next = current->next;
        current->next = new_node;
    }

    return new_node;
}

Packet* popPQ(PriorityQueue* pq) 
{
    if (isEmpty(pq)) {
        return NULL;
    }

    PQNode* temp = pq->head;
    Packet* packet = temp->packet;

    pq->head = pq->head->next;
    free(temp);
    return packet;
}

int isEmpty(PriorityQueue* pq)
{
    return (pq->head == NULL);
}