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

/**
 * @file
 * a java-style generic doubly linked list
 */

#ifndef TLC_UTIL_CONTAINER_LINKEDLIST_H_
#define TLC_UTIL_CONTAINER_LINKEDLIST_H_

#include <stddef.h>

/** a doubly linked list node */
typedef struct ListNode {
  struct ListNode *next;
  struct ListNode *prev;
  void *data;
} ListNode;

/**
 * a doubly linked list
 *
 * uses dummy nodes at head and tail
 */
typedef struct {
  struct ListNode *head;
  struct ListNode *tail;
} LinkedList;

/**
 * ctor
 */
void linkedListInit(LinkedList *l);
/**
 * insertions
 */
void insertNodeAfter(ListNode *n, void *data);
void insertNodeBefore(ListNode *n, void *data);
void insertNodeEnd(LinkedList *l, void *data);
/**
 * deletion
 *
 * @returns data of deleted node
 */
void *removeNode(ListNode *n);
/**
 * sizeof
 *
 * @returns number of non-dummy nodes
 */
size_t linkedListLength(LinkedList *l);
/**
 * dtor
 */
void linkedListUninit(LinkedList *l, void (*dtor)(void *));

#endif  // TLC_UTIL_CONTAINER_LINKEDLIST_H_