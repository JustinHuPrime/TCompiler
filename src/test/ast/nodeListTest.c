// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Tests for lists of nodes and lists of pairs of nodes

#include "ast/ast.h"

#include "tests.h"

void nodeListTest(TestStatus *status) {
  NodeList *list;
  Node *a = constTrueNodeCreate(0, 0);
  Node *b = constFalseNodeCreate(0, 0);

  list = nodeListCreate();
  test(status, "[ast] [nodeList] [constructor] list created has size zero",
       list->size == 0);
  test(status, "[ast] [nodeList] [constructor] list created has capacity one",
       list->capacity == 1);
  test(status,
       "[ast] [nodeList] [constructor] list created does not have null pointer "
       "to elements",
       list->elements != NULL);

  nodeListInsert(list, a);
  test(status, "[ast] [nodeList] [insert] insert increases size",
       list->size == 1);
  test(
      status,
      "[ast] [nodeList] [insert] insert into non-full does not change capacity",
      list->capacity == 1);
  test(status,
       "[ast] [nodeList] [insert] insert has the correct element in the list",
       list->elements[0] == a);

  nodeListInsert(list, b);
  test(status, "[ast] [nodeList] [insert] insert increases size",
       list->size == 2);
  test(status, "[ast] [nodeList] [insert] insert into full increases capacity",
       list->capacity == 2);
  test(status,
       "[ast] [nodeList] [insert] insert has the correct element in the list",
       list->elements[1] == b);
  test(status,
       "[ast] [nodeList] [insert] insert does not change the old elements",
       list->elements[0] == a);

  nodeListDestroy(list);
}
void nodeListPairTest(TestStatus *status) {
  NodePairList *list;
  Node *a1 = constTrueNodeCreate(0, 0);
  Node *a2 = constTrueNodeCreate(0, 0);
  Node *b1 = constFalseNodeCreate(0, 0);
  Node *b2 = constFalseNodeCreate(0, 0);

  list = nodePairListCreate();
  test(status, "[ast] [nodePairList] [constructor] list created has size zero",
       list->size == 0);
  test(status,
       "[ast] [nodePairList] [constructor] list created has capacity one",
       list->capacity == 1);
  test(status,
       "[ast] [nodePairList] [constructor] list created does not have null "
       "pointer to firstElements",
       list->firstElements != NULL);

  nodePairListInsert(list, a1, a2);
  test(status, "[ast] [nodePairList] [insert] insert increases size",
       list->size == 1);
  test(status,
       "[ast] [nodePairList] [insert] insert into non-full does not change "
       "capacity",
       list->capacity == 1);
  test(status,
       "[ast] [nodePairList] [insert] insert has the correct element.first in "
       "the list",
       list->firstElements[0] == a1);
  test(status,
       "[ast] [nodePairList] [insert] insert has the correct element.second in "
       "the list",
       list->secondElements[0] == a2);

  nodePairListInsert(list, b1, b2);
  test(status, "[ast] [nodePairList] [insert] insert increases size",
       list->size == 2);
  test(status,
       "[ast] [nodePairList] [insert] insert into full increases capacity",
       list->capacity == 2);
  test(status,
       "[ast] [nodePairList] [insert] insert has the correct element.first in "
       "the list",
       list->firstElements[1] == b1);
  test(status,
       "[ast] [nodePairList] [insert] insert has the correct element.second in "
       "the list",
       list->secondElements[1] == b2);
  test(status,
       "[ast] [nodePairList] [insert] insert does not change the old "
       "element.first",
       list->firstElements[0] == a1);
  test(status,
       "[ast] [nodePairList] [insert] insert does not change the old "
       "element.second",
       list->secondElements[0] == a2);

  nodePairListDestroy(list);
}
