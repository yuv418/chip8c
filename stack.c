#include "stack.h"

#include <stdlib.h>

uint16_t pop(struct stack_t **head) {
  if (*head != NULL) {
    struct stack_t *old_node = *head;
    uint16_t old_data = old_node->data;
    (*head) = old_node->next;
    return old_data;
  } else {
    // TODO: Shouldn't this be the base address,
    // 0x200?
    return 0;
  }
}
struct stack_t *push(struct stack_t *head, uint16_t data) {
  struct stack_t *new_node = malloc(sizeof(struct stack_t));
  new_node->data = data;
  new_node->next = head;
  return new_node;
}
