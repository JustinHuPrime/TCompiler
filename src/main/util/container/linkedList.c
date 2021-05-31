// Copyright 2021 Justin Hu
//
// This file is part of the T Language Compiler.
//
// The T Language Compiler is free software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
//
// The T Language Compiler is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
// Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// the T Language Compiler. If not see <https://www.gnu.org/licenses/>.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "util/container/linkedList.h"

#include <stdlib.h>

void linkedListInit(LinkedList *l) {
  l->head = malloc(sizeof(ListNode));
  l->tail = malloc(sizeof(ListNode));
  l->head->next = l->tail;
  l->head->prev = NULL;
  l->head->data = NULL;
  l->tail->next = NULL;
  l->tail->prev = l->head;
  l->tail->data = NULL;
}
void insertNodeAfter(ListNode *n, void *data) {
  ListNode *newNode = malloc(sizeof(ListNode));
  newNode->data = data;
  newNode->prev = n;
  newNode->next = n->next;
  newNode->next->prev = newNode->prev->next = newNode;
}
void insertNodeBefore(ListNode *n, void *data) {
  ListNode *newNode = malloc(sizeof(ListNode));
  newNode->data = data;
  newNode->prev = n->prev;
  newNode->next = n;
  newNode->next->prev = newNode->prev->next = newNode;
}
void *removeNode(ListNode *n) {
  n->next->prev = n->prev;
  n->prev->next = n->next;
  void *retval = n->data;
  free(n);
  return retval;
}
void linkedListUninit(LinkedList *l, void (*dtor)(void *)) {
  while (l->head->next != l->tail) {
    dtor(removeNode(l->head->next));
  }
  free(l->head);
  free(l->tail);
}