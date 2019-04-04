#include "ast/constExpParseTest.h"

#include "ast/ast.h"

#include <stdlib.h>
#include <string.h>

#include <stdio.h>

void constExpParseTest(TestStatus *status) {
  char *string;
  Node *node;

  // INT

  // positive zero
  string = strcpy(malloc(3), "+0");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(status,
       "[ast] [constantExp] [int] parsing '+0' produces type signed byte.",
       node->data.constExp.type == CTYPE_BYTE);
  test(status, "[ast] [constantExp] [int] parsing '+0' produces value 0.",
       node->data.constExp.value.byteVal == 0);
  nodeDestroy(node);

  // unsigned zero
  string = strcpy(malloc(2), "0");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(status,
       "[ast] [constantExp] [int] parsing '0' produces type unsigned byte.",
       node->data.constExp.type == CTYPE_UBYTE);
  test(status, "[ast] [constantExp] [int] parsing '0' produces value 0.",
       node->data.constExp.value.ubyteVal == 0);
  nodeDestroy(node);

  // negative zero
  string = strcpy(malloc(3), "-0");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(status,
       "[ast] [constantExp] [int] parsing '-0' produces type signed byte.",
       node->data.constExp.type == CTYPE_BYTE);
  test(status, "[ast] [constantExp] [int] parsing '-0' produces value 0.",
       node->data.constExp.value.byteVal == 0);
  nodeDestroy(node);

  // -byte
  string = strcpy(malloc(5), "-128");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(status,
       "[ast] [constantExp] [int] parsing '-128' produces type signed byte.",
       node->data.constExp.type == CTYPE_BYTE);
  test(status, "[ast] [constantExp] [int] parsing '-128' produces value -128.",
       node->data.constExp.value.byteVal == -128);
  nodeDestroy(node);

  // ubyte
  string = strcpy(malloc(4), "213");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(status,
       "[ast] [constantExp] [int] parsing '213' produces type unsigned byte.",
       node->data.constExp.type == CTYPE_UBYTE);
  test(status, "[ast] [constantExp] [int] parsing '213' produces value 213.",
       node->data.constExp.value.ubyteVal == 213);
  nodeDestroy(node);

  // +byte
  string = strcpy(malloc(5), "+104");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(status,
       "[ast] [constantExp] [int] parsing '+104' produces type signed byte.",
       node->data.constExp.type == CTYPE_BYTE);
  test(status, "[ast] [constantExp] [int] parsing '+104' produces value +104.",
       node->data.constExp.value.byteVal == 104);
  nodeDestroy(node);

  // -int
  string = strcpy(malloc(5), "-200");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(status,
       "[ast] [constantExp] [int] parsing '-200' produces type signed int.",
       node->data.constExp.type == CTYPE_INT);
  test(status, "[ast] [constantExp] [int] parsing '-200' produces value -200.",
       node->data.constExp.value.intVal == -200);
  nodeDestroy(node);

  // uint
  string = strcpy(malloc(4), "256");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(status,
       "[ast] [constantExp] [int] parsing '256' produces type unsigned int.",
       node->data.constExp.type == CTYPE_UINT);
  test(status, "[ast] [constantExp] [int] parsing '256' produces value 256.",
       node->data.constExp.value.uintVal == 256);
  nodeDestroy(node);

  // +int
  string = strcpy(malloc(5), "+257");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(status,
       "[ast] [constantExp] [int] parsing '+257' produces type signed int.",
       node->data.constExp.type == CTYPE_INT);
  test(status, "[ast] [constantExp] [int] parsing '+257' produces value +257.",
       node->data.constExp.value.intVal == 257);
  nodeDestroy(node);

  // -long
  string = strcpy(malloc(12), "-5000000000");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(status,
       "[ast] [constantExp] [int] parsing '-5000000000' produces type signed "
       "long.",
       node->data.constExp.type == CTYPE_LONG);
  test(status,
       "[ast] [constantExp] [int] parsing '-5000000000' produces value "
       "-5000000000.",
       node->data.constExp.value.longVal == -5000000000);
  nodeDestroy(node);

  // uint
  string = strcpy(malloc(20), "9223372036854775807");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(status,
       "[ast] [constantExp] [int] parsing '9223372036854775807' produces type "
       "unsigned long.",
       node->data.constExp.type == CTYPE_ULONG);
  test(status,
       "[ast] [constantExp] [int] parsing '9223372036854775807' produces value "
       "9223372036854775807.",
       node->data.constExp.value.ulongVal == 9223372036854775807);
  nodeDestroy(node);

  // +long
  string = strcpy(malloc(12), "+5000000001");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(status,
       "[ast] [constantExp] [int] parsing '+5000000001' produces type signed "
       "int.",
       node->data.constExp.type == CTYPE_LONG);
  test(status,
       "[ast] [constantExp] [int] parsing '+5000000001' produces value "
       "+5000000001.",
       node->data.constExp.value.longVal == 5000000001);
  nodeDestroy(node);

  // signed ranges
  // -error, -long, -int, -byte, +byte, +int, +long, +error
  string = strcpy(malloc(21), "-9223372036854775809");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(
      status,
      "[ast] [constantExp] [int] parsing '-9223372036854775809' produces range "
      "error.",
      node->data.constExp.type == CTYPE_RANGE_ERROR);
  nodeDestroy(node);

  string = strcpy(malloc(21), "-9223372036854775808");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(status,
       "[ast] [constantExp] [int] parsing '-9223372036854775808' produces type "
       "long.",
       node->data.constExp.type == CTYPE_LONG);
  test(
      status,
      "[ast] [constantExp] [int] parsing '-9223372036854775808' produces value "
      "-9223372036854775808.",
      node->data.constExp.value.longVal == (-9223372036854775807 - 1));
  nodeDestroy(node);

  string = strcpy(malloc(12), "-2147483649");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(status,
       "[ast] [constantExp] [int] parsing '-2147483649' produces type long.",
       node->data.constExp.type == CTYPE_LONG);
  test(status,
       "[ast] [constantExp] [int] parsing '-2147483649' produces value "
       "-2147483649.",
       node->data.constExp.value.longVal == -2147483649);
  nodeDestroy(node);

  string = strcpy(malloc(12), "-2147483648");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(status,
       "[ast] [constantExp] [int] parsing '-2147483648' produces type int.",
       node->data.constExp.type == CTYPE_INT);
  test(status,
       "[ast] [constantExp] [int] parsing '-2147483648' produces value "
       "-2147483648.",
       node->data.constExp.value.intVal == -2147483648);
  nodeDestroy(node);

  string = strcpy(malloc(5), "-129");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(status, "[ast] [constantExp] [int] parsing '-129' produces type int.",
       node->data.constExp.type == CTYPE_INT);
  test(status, "[ast] [constantExp] [int] parsing '-129' produces value -129.",
       node->data.constExp.value.intVal == -129);
  nodeDestroy(node);

  string = strcpy(malloc(5), "-128");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(status, "[ast] [constantExp] [int] parsing '-128' produces type byte.",
       node->data.constExp.type == CTYPE_BYTE);
  test(status, "[ast] [constantExp] [int] parsing '-128' produces value -128.",
       node->data.constExp.value.byteVal == -128);
  nodeDestroy(node);

  string = strcpy(malloc(5), "+127");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(status, "[ast] [constantExp] [int] parsing '+127' produces type byte.",
       node->data.constExp.type == CTYPE_BYTE);
  test(status, "[ast] [constantExp] [int] parsing '+127' produces value 127.",
       node->data.constExp.value.byteVal == 127);
  nodeDestroy(node);

  string = strcpy(malloc(5), "+128");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(status, "[ast] [constantExp] [int] parsing '+128' produces type int.",
       node->data.constExp.type == CTYPE_INT);
  test(status, "[ast] [constantExp] [int] parsing '+128' produces value 128.",
       node->data.constExp.value.intVal == 128);
  nodeDestroy(node);

  string = strcpy(malloc(12), "+2147483647");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(status,
       "[ast] [constantExp] [int] parsing '+2147483647' produces type int.",
       node->data.constExp.type == CTYPE_INT);
  test(status,
       "[ast] [constantExp] [int] parsing '+2147483647' produces value "
       "2147483647.",
       node->data.constExp.value.intVal == 2147483647);
  nodeDestroy(node);

  string = strcpy(malloc(12), "+2147483648");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(status,
       "[ast] [constantExp] [int] parsing '+2147483648' produces type long.",
       node->data.constExp.type == CTYPE_LONG);
  test(status,
       "[ast] [constantExp] [int] parsing '+2147483648' produces value "
       "2147483648.",
       node->data.constExp.value.longVal == 2147483648);
  nodeDestroy(node);

  string = strcpy(malloc(21), "+9223372036854775807");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(status,
       "[ast] [constantExp] [int] parsing '+9223372036854775807' produces type "
       "long.",
       node->data.constExp.type == CTYPE_LONG);
  test(
      status,
      "[ast] [constantExp] [int] parsing '+9223372036854775807' produces value "
      "9223372036854775807.",
      node->data.constExp.value.longVal == 9223372036854775807);
  nodeDestroy(node);

  string = strcpy(malloc(21), "+9223372036854775808");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(
      status,
      "[ast] [constantExp] [int] parsing '+9223372036854775808' produces range "
      "error.",
      node->data.constExp.type == CTYPE_RANGE_ERROR);
  nodeDestroy(node);

  // unsigned ranges
  string = strcpy(malloc(4), "255");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(status,
       "[ast] [constantExp] [int] parsing '255' produces type unsigned byte.",
       node->data.constExp.type == CTYPE_UBYTE);
  test(status, "[ast] [constantExp] [int] parsing '255' produces value 255.",
       node->data.constExp.value.ubyteVal == 255u);
  nodeDestroy(node);

  string = strcpy(malloc(4), "256");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(status,
       "[ast] [constantExp] [int] parsing '256' produces type unsigned int.",
       node->data.constExp.type == CTYPE_UINT);
  test(status, "[ast] [constantExp] [int] parsing '256' produces value 256.",
       node->data.constExp.value.uintVal == 256u);
  nodeDestroy(node);

  string = strcpy(malloc(11), "4294967295");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(status,
       "[ast] [constantExp] [int] parsing '4294967295' produces type unsigned "
       "int.",
       node->data.constExp.type == CTYPE_UINT);
  test(status,
       "[ast] [constantExp] [int] parsing '4294967295' produces value "
       "4294967295.",
       node->data.constExp.value.uintVal == 4294967295u);
  nodeDestroy(node);

  string = strcpy(malloc(11), "4294967296");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(status,
       "[ast] [constantExp] [int] parsing '4294967296' produces type unsigned "
       "long.",
       node->data.constExp.type == CTYPE_ULONG);
  test(status,
       "[ast] [constantExp] [int] parsing '4294967296' produces value "
       "4294967296.",
       node->data.constExp.value.ulongVal == 4294967296u);
  nodeDestroy(node);

  string = strcpy(malloc(21), "18446744073709551615");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(status,
       "[ast] [constantExp] [int] parsing '18446744073709551615' produces type "
       "unsigned long.",
       node->data.constExp.type == CTYPE_ULONG);
  test(
      status,
      "[ast] [constantExp] [int] parsing '18446744073709551615' produces value "
      "18446744073709551615.",
      node->data.constExp.value.ulongVal == 18446744073709551615u);
  nodeDestroy(node);

  string = strcpy(malloc(21), "18446744073709551617");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(
      status,
      "[ast] [constantExp] [int] parsing '18446744073709551617' produces range "
      "error.",
      node->data.constExp.type == CTYPE_RANGE_ERROR);
  nodeDestroy(node);

  // bases and leading zeroes

  // binary
  string = strcpy(malloc(19), "0b0111101010110111");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(status,
       "[ast] [constantExp] [int] parsing '0b0111101010110111' produces type "
       "unsigned int.",
       node->data.constExp.type == CTYPE_UINT);
  test(status,
       "[ast] [constantExp] [int] parsing '0b0111101010110111' produces value "
       "31415.",
       node->data.constExp.value.uintVal == 31415);
  nodeDestroy(node);

  // octal
  string = strcpy(malloc(8), "0075267");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(
      status,
      "[ast] [constantExp] [int] parsing '0075267' produces type unsigned int.",
      node->data.constExp.type == CTYPE_UINT);
  test(status,
       "[ast] [constantExp] [int] parsing '0075267' produces value 31415.",
       node->data.constExp.value.uintVal == 31415);
  nodeDestroy(node);

  // hex
  string = strcpy(malloc(9), "0x007AB7");
  node = constExpNodeCreate(0, 0, TYPEHINT_INT, string);
  test(status,
       "[ast] [constantExp] [int] parsing '0x007AB7' produces type unsigned "
       "int.",
       node->data.constExp.type == CTYPE_UINT);
  test(status,
       "[ast] [constantExp] [int] parsing '0x007AB7' produces value 31415.",
       node->data.constExp.value.uintVal == 31415);
  nodeDestroy(node);

  // FLOAT

  // signedness
  string = strcpy(malloc(4), "1.0");
  node = constExpNodeCreate(0, 0, TYPEHINT_FLOAT, string);
  test(status, "[ast] [constantExp] [float] parsing '1.0' produces type float.",
       node->data.constExp.type == CTYPE_FLOAT);
  test(status, "[ast] [constantExp] [float] parsing '1.0' produces value +1e0.",
       node->data.constExp.value.floatBits == 0x3F800000);
  nodeDestroy(node);

  string = strcpy(malloc(5), "-1.0");
  node = constExpNodeCreate(0, 0, TYPEHINT_FLOAT, string);
  test(status,
       "[ast] [constantExp] [float] parsing '-1.0' produces type float.",
       node->data.constExp.type == CTYPE_FLOAT);
  test(status,
       "[ast] [constantExp] [float] parsing '-1.0' produces value -1e0.",
       node->data.constExp.value.floatBits == 0xBF800000);
  nodeDestroy(node);

  string = strcpy(malloc(5), "+1.0");
  node = constExpNodeCreate(0, 0, TYPEHINT_FLOAT, string);
  test(status,
       "[ast] [constantExp] [float] parsing '+1.0' produces type float.",
       node->data.constExp.type == CTYPE_FLOAT);
  test(status,
       "[ast] [constantExp] [float] parsing '+1.0' produces value +1e0.",
       node->data.constExp.value.floatBits == 0x3F800000);
  nodeDestroy(node);

  // float vs double
  string = strcpy(malloc(4), "1.1");
  node = constExpNodeCreate(0, 0, TYPEHINT_FLOAT, string);
  test(status,
       "[ast] [constantExp] [float] parsing '1.1' produces type double.",
       node->data.constExp.type == CTYPE_DOUBLE);
  test(status,
       "[ast] [constantExp] [float] parsing '1.1' produces value +1.1e0.",
       node->data.constExp.value.doubleBits == 0x3FF199999999999A);
  nodeDestroy(node);

  string = strcpy(malloc(4), "1.5");
  node = constExpNodeCreate(0, 0, TYPEHINT_FLOAT, string);
  test(status, "[ast] [constantExp] [float] parsing '1.5' produces type float.",
       node->data.constExp.type == CTYPE_FLOAT);
  test(status,
       "[ast] [constantExp] [float] parsing '1.5' produces value +1.5e0.",
       node->data.constExp.value.floatBits == 0x3FC00000);
  nodeDestroy(node);

  // positive and negative zero
  string = strcpy(malloc(5), "+0.0");
  node = constExpNodeCreate(0, 0, TYPEHINT_FLOAT, string);
  test(status,
       "[ast] [constantExp] [float] parsing '+0.0' produces type float.",
       node->data.constExp.type == CTYPE_FLOAT);
  test(status,
       "[ast] [constantExp] [float] parsing '+0.0' produces value +0e0.",
       node->data.constExp.value.floatBits == 0x00000000);
  nodeDestroy(node);

  string = strcpy(malloc(5), "-0.0");
  node = constExpNodeCreate(0, 0, TYPEHINT_FLOAT, string);
  test(status,
       "[ast] [constantExp] [float] parsing '-0.0' produces type float.",
       node->data.constExp.type == CTYPE_FLOAT);
  test(status,
       "[ast] [constantExp] [float] parsing '-0.0' produces value -0e0.",
       node->data.constExp.value.floatBits == 0x80000000);
  nodeDestroy(node);

  // STRING
  // CHAR
  // WSTRING
  // WCHAR
}