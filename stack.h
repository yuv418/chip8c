#pragma once

#include <stdint.h>
typedef struct stack_t {
  uint16_t data;
  struct stack_t *next;
} stack;

uint16_t pop(struct stack_t **head);
struct stack_t *push(struct stack_t *head, uint16_t data);
