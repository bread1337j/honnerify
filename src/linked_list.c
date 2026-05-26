#include "linked_list.h"

#include <stdlib.h>

Node* createNode(void* entry) {
  Node* newNode = (Node*) malloc(sizeof(Node));
  newNode->entry = entry;
  newNode->prev = NULL;
  newNode->next = NULL;
  return newNode;
}

void connectNodes(Node* prev, Node* next) {
  if (prev) {
    prev->next = next;
  }
  if (next) {
    next->prev = prev;
  }
}



LinkedList* LinkedList_new() {
  LinkedList* newList = (LinkedList*) malloc(sizeof(LinkedList));
  newList->front = NULL;
  newList->end = NULL;
  return newList;
}

Node* LinkedList_insertFront(LinkedList* list, void* entry) {
  Node* newFront = createNode(entry);
  Node* oldFront = list->front;
  list->front = newFront;
  if (!oldFront) {
    list->end = newFront;
  }

  connectNodes(newFront, oldFront);
  return newFront;
}

Node* LinkedList_insertEnd(LinkedList* list, void* entry) {
  Node* newEnd = createNode(entry);
  Node* oldEnd = list->end;
  list->end = newEnd;
  if (!oldEnd) {
    list->front = newEnd;
  }

  connectNodes(oldEnd, newEnd);
  return newEnd;
}

void LinkedList_deleteNode(LinkedList* list, Node* node) {
  Node* prev = node->prev;
  Node* next = node->next;

  connectNodes(prev, next);

  if (node == list->front) {
    list->front = next;
  }

  if (node == list->end) {
    list->end = prev;
  }

  free(node);
}

void LinkedList_free(LinkedList* list) {
  Node* cur = list->front;
  Node* next = NULL;

  while (cur) {
    next = cur->next;
    free(cur);
    cur = next;
  }

  free(list);
}
