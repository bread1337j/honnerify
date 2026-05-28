#ifndef LINKED_LIST_H
  #define LINKED_LIST_H
  typedef struct Node {

    struct Node* prev;
    void* entry;
    struct Node* next;

  } Node;

  Node* createNode(void* entry);
  void connectNodes(Node* prev, Node* next);

  typedef struct LinkedList {

    Node* front;
    Node* end;

  } LinkedList;

  LinkedList* LinkedList_new();
  Node* LinkedList_insertFront(LinkedList* list, void* entry);
  Node* LinkedList_insertEnd(LinkedList* list, void* entry);

  void LinkedList_deleteNode(LinkedList* list, Node* node);

  void LinkedList_free(LinkedList* list);
#endif
