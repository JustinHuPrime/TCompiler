// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Test modules for constant parsing

#include "ast/ast.h"

#include "unitTests/tests.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

void constExpParseIntTest(TestStatus *status) {
  char *string;
  Node *node;

  // positive zero
  string = strcpy(malloc(3), "+0");
  node = constZeroIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [zero] parsing '+0' produces type signed "
       "byte.",
       node->data.constExp.type == CT_BYTE);
  test(status,
       "[ast] [constantExp] [int] [zero] parsing '+0' produces value 0.",
       node->data.constExp.value.byteVal == 0);
  nodeDestroy(node);

  // unsigned zero
  string = strcpy(malloc(2), "0");
  node = constZeroIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [zero] parsing '0' produces type unsigned "
       "byte.",
       node->data.constExp.type == CT_UBYTE);
  test(status, "[ast] [constantExp] [int] [zero] parsing '0' produces value 0.",
       node->data.constExp.value.ubyteVal == 0);
  nodeDestroy(node);

  // negative zero
  string = strcpy(malloc(3), "-0");
  node = constZeroIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [zero] parsing '-0' produces type signed "
       "byte.",
       node->data.constExp.type == CT_BYTE);
  test(status,
       "[ast] [constantExp] [int] [zero] parsing '-0' produces value 0.",
       node->data.constExp.value.byteVal == 0);
  nodeDestroy(node);

  // -byte
  string = strcpy(malloc(5), "-128");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [type] parsing '-128' produces type signed "
       "byte.",
       node->data.constExp.type == CT_BYTE);
  test(status, "[ast] [constantExp] [int] parsing '-128' produces value -128.",
       node->data.constExp.value.byteVal == -128);
  nodeDestroy(node);

  // ubyte
  string = strcpy(malloc(4), "213");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [type] parsing '213' produces type unsigned "
       "byte.",
       node->data.constExp.type == CT_UBYTE);
  test(status,
       "[ast] [constantExp] [int] [type] parsing '213' produces value 213.",
       node->data.constExp.value.ubyteVal == 213);
  nodeDestroy(node);

  // +byte
  string = strcpy(malloc(5), "+104");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [type] parsing '+104' produces type signed "
       "byte.",
       node->data.constExp.type == CT_BYTE);
  test(status,
       "[ast] [constantExp] [int] [type] parsing '+104' produces value +104.",
       node->data.constExp.value.byteVal == 104);
  nodeDestroy(node);

  // -short
  string = strcpy(malloc(5), "-200");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [type] parsing '-200' produces type signed "
       "short.",
       node->data.constExp.type == CT_SHORT);
  test(status,
       "[ast] [constantExp] [int] [type] parsing '-200' produces value -200.",
       node->data.constExp.value.shortVal == -200);
  nodeDestroy(node);

  // ushort
  string = strcpy(malloc(4), "256");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [type] parsing '256' produces type unsigned "
       "short.",
       node->data.constExp.type == CT_USHORT);
  test(status,
       "[ast] [constantExp] [int] [type] parsing '256' produces value 256.",
       node->data.constExp.value.ushortVal == 256);
  nodeDestroy(node);

  // +short
  string = strcpy(malloc(5), "+257");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [type] parsing '+257' produces type signed "
       "short.",
       node->data.constExp.type == CT_SHORT);
  test(status,
       "[ast] [constantExp] [int] [type] parsing '+257' produces value +257.",
       node->data.constExp.value.shortVal == 257);
  nodeDestroy(node);

  // -int
  string = strcpy(malloc(7), "-40000");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [type] parsing '-40000' produces type signed "
       "int.",
       node->data.constExp.type == CT_INT);
  test(status,
       "[ast] [constantExp] [int] [type] parsing '-40000' produces value "
       "-40000.",
       node->data.constExp.value.intVal == -40000);
  nodeDestroy(node);

  // uint
  string = strcpy(malloc(6), "70000");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(
      status,
      "[ast] [constantExp] [int] [type] parsing '70000' produces type unsigned "
      "int.",
      node->data.constExp.type == CT_UINT);
  test(status,
       "[ast] [constantExp] [int] [type] parsing '70000' produces value 70000.",
       node->data.constExp.value.uintVal == 70000);
  nodeDestroy(node);

  // +int
  string = strcpy(malloc(7), "+40000");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [type] parsing '+40000' produces type signed "
       "int.",
       node->data.constExp.type == CT_INT);
  test(status,
       "[ast] [constantExp] [int] [type] parsing '+40000' produces value "
       "+40000.",
       node->data.constExp.value.intVal == 40000);
  nodeDestroy(node);

  // -long
  string = strcpy(malloc(12), "-5000000000");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [type] parsing '-5000000000' produces type "
       "signed long.",
       node->data.constExp.type == CT_LONG);
  test(status,
       "[ast] [constantExp] [int] [type] parsing '-5000000000' produces value "
       "-5000000000.",
       node->data.constExp.value.longVal == -5000000000);
  nodeDestroy(node);

  // uint
  string = strcpy(malloc(20), "9223372036854775807");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [type] parsing '9223372036854775807' "
       "produces type unsigned long.",
       node->data.constExp.type == CT_ULONG);
  test(status,
       "[ast] [constantExp] [int] [type] parsing '9223372036854775807' "
       "produces value 9223372036854775807.",
       node->data.constExp.value.ulongVal == 9223372036854775807);
  nodeDestroy(node);

  // +long
  string = strcpy(malloc(12), "+5000000001");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [type] parsing '+5000000001' produces type "
       "signed long.",
       node->data.constExp.type == CT_LONG);
  test(status,
       "[ast] [constantExp] [int] [type] parsing '+5000000001' produces value "
       "+5000000001.",
       node->data.constExp.value.longVal == 5000000001);
  nodeDestroy(node);

  // signed ranges
  // -error, -long, -int, -byte, +byte, +int, +long, +error
  string = strcpy(malloc(21), "-9223372036854775809");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '-9223372036854775809' "
       "produces range error.",
       node->data.constExp.type == CT_RANGE_ERROR);
  nodeDestroy(node);

  string = strcpy(malloc(21), "-9223372036854775808");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '-9223372036854775808' "
       "produces type long.",
       node->data.constExp.type == CT_LONG);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '-9223372036854775808' "
       "produces value -9223372036854775808.",
       node->data.constExp.value.longVal == (-9223372036854775807 - 1));
  nodeDestroy(node);

  string = strcpy(malloc(12), "-2147483649");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '-2147483649' produces type "
       "long.",
       node->data.constExp.type == CT_LONG);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '-2147483649' produces value "
       "-2147483649.",
       node->data.constExp.value.longVal == -2147483649);
  nodeDestroy(node);

  string = strcpy(malloc(12), "-2147483648");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '-2147483648' produces type "
       "int.",
       node->data.constExp.type == CT_INT);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '-2147483648' produces value "
       "-2147483648.",
       node->data.constExp.value.intVal == -2147483648);
  nodeDestroy(node);

  string = strcpy(malloc(7), "-32769");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '-32769' produces type int.",
       node->data.constExp.type == CT_INT);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '-32769' produces value "
       "-32769.",
       node->data.constExp.value.intVal == -32769);
  nodeDestroy(node);

  string = strcpy(malloc(5), "-129");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '-129' produces type short.",
       node->data.constExp.type == CT_SHORT);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '-129' produces value -129.",
       node->data.constExp.value.shortVal == -129);
  nodeDestroy(node);

  string = strcpy(malloc(5), "-128");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '-128' produces type byte.",
       node->data.constExp.type == CT_BYTE);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '-128' produces value -128.",
       node->data.constExp.value.byteVal == -128);
  nodeDestroy(node);

  string = strcpy(malloc(5), "+127");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '+127' produces type byte.",
       node->data.constExp.type == CT_BYTE);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '+127' produces value 127.",
       node->data.constExp.value.byteVal == 127);
  nodeDestroy(node);

  string = strcpy(malloc(5), "+128");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '+128' produces type short.",
       node->data.constExp.type == CT_SHORT);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '+128' produces value 128.",
       node->data.constExp.value.shortVal == 128);
  nodeDestroy(node);

  string = strcpy(malloc(7), "+32768");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '+32768' produces type int.",
       node->data.constExp.type == CT_INT);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '+32768' produces value "
       "32768.",
       node->data.constExp.value.intVal == 32768);
  nodeDestroy(node);

  string = strcpy(malloc(12), "+2147483647");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '+2147483647' produces type "
       "int.",
       node->data.constExp.type == CT_INT);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '+2147483647' produces value "
       "2147483647.",
       node->data.constExp.value.intVal == 2147483647);
  nodeDestroy(node);

  string = strcpy(malloc(12), "+2147483648");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '+2147483648' produces type "
       "long.",
       node->data.constExp.type == CT_LONG);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '+2147483648' produces value "
       "2147483648.",
       node->data.constExp.value.longVal == 2147483648);
  nodeDestroy(node);

  string = strcpy(malloc(21), "+9223372036854775807");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '+9223372036854775807' "
       "produces type long.",
       node->data.constExp.type == CT_LONG);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '+9223372036854775807' "
       "produces value 9223372036854775807.",
       node->data.constExp.value.longVal == 9223372036854775807);
  nodeDestroy(node);

  string = strcpy(malloc(21), "+9223372036854775808");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '+9223372036854775808' "
       "produces range error.",
       node->data.constExp.type == CT_RANGE_ERROR);
  nodeDestroy(node);

  // unsigned ranges
  string = strcpy(malloc(4), "255");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '255' produces type unsigned "
       "byte.",
       node->data.constExp.type == CT_UBYTE);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '255' produces value 255.",
       node->data.constExp.value.ubyteVal == 255u);
  nodeDestroy(node);

  string = strcpy(malloc(4), "256");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '256' produces type unsigned "
       "short.",
       node->data.constExp.type == CT_USHORT);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '256' produces value 256.",
       node->data.constExp.value.ushortVal == 256u);
  nodeDestroy(node);

  string = strcpy(malloc(6), "65536");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '65536' produces type "
       "unsigned int.",
       node->data.constExp.type == CT_UINT);
  test(
      status,
      "[ast] [constantExp] [int] [range] parsing '65536' produces value 65536.",
      node->data.constExp.value.uintVal == 65536u);
  nodeDestroy(node);

  string = strcpy(malloc(11), "4294967295");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '4294967295' produces type "
       "unsigned int.",
       node->data.constExp.type == CT_UINT);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '4294967295' produces value "
       "4294967295.",
       node->data.constExp.value.uintVal == 4294967295u);
  nodeDestroy(node);

  string = strcpy(malloc(11), "4294967296");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '4294967296' produces type "
       "unsigned long.",
       node->data.constExp.type == CT_ULONG);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '4294967296' produces value "
       "4294967296.",
       node->data.constExp.value.ulongVal == 4294967296u);
  nodeDestroy(node);

  string = strcpy(malloc(21), "18446744073709551615");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '18446744073709551615' "
       "produces type unsigned long.",
       node->data.constExp.type == CT_ULONG);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '18446744073709551615' "
       "produces value 18446744073709551615.",
       node->data.constExp.value.ulongVal == 18446744073709551615u);
  nodeDestroy(node);

  string = strcpy(malloc(21), "18446744073709551617");
  node = constDecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [range] parsing '18446744073709551617' "
       "produces range error.",
       node->data.constExp.type == CT_RANGE_ERROR);
  nodeDestroy(node);

  // bases and leading zeroes

  // binary
  string = strcpy(malloc(19), "0b0111101010110111");
  node = constBinaryIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [base] parsing '0b0111101010110111' produces "
       "type unsigned short.",
       node->data.constExp.type == CT_USHORT);
  test(status,
       "[ast] [constantExp] [int] [base] parsing '0b0111101010110111' produces "
       "value 31415.",
       node->data.constExp.value.ushortVal == 31415);
  nodeDestroy(node);

  // octal
  string = strcpy(malloc(8), "0075267");
  node = constOctalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [base] parsing '0075267' produces type "
       "unsigned int.",
       node->data.constExp.type == CT_USHORT);
  test(status,
       "[ast] [constantExp] [int] [base] parsing '0075267' produces value "
       "31415.",
       node->data.constExp.value.ushortVal == 31415);
  nodeDestroy(node);

  // hex
  string = strcpy(malloc(9), "0x007AB7");
  node = constHexadecimalIntExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [int] [base] parsing '0x007AB7' produces type "
       "unsigned short.",
       node->data.constExp.type == CT_USHORT);
  test(status,
       "[ast] [constantExp] [int] [base] parsing '0x007AB7' produces value "
       "31415.",
       node->data.constExp.value.ushortVal == 31415);
  nodeDestroy(node);
}
void constExpParseFloatTest(TestStatus *status) {
  char *string;
  Node *node;

  // signedness
  string = strcpy(malloc(4), "1.0");
  node = constFloatExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [float] [signs] parsing '1.0' produces type float.",
       node->data.constExp.type == CT_FLOAT);
  test(status,
       "[ast] [constantExp] [float] [signs] parsing '1.0' produces value +1e0.",
       node->data.constExp.value.floatBits == 0x3F800000);
  nodeDestroy(node);

  string = strcpy(malloc(5), "-1.0");
  node = constFloatExpNodeCreate(0, 0, string);
  test(
      status,
      "[ast] [constantExp] [float] [signs] parsing '-1.0' produces type float.",
      node->data.constExp.type == CT_FLOAT);
  test(
      status,
      "[ast] [constantExp] [float] [signs] parsing '-1.0' produces value -1e0.",
      node->data.constExp.value.floatBits == 0xBF800000);
  nodeDestroy(node);

  string = strcpy(malloc(5), "+1.0");
  node = constFloatExpNodeCreate(0, 0, string);
  test(
      status,
      "[ast] [constantExp] [float] [signs] parsing '+1.0' produces type float.",
      node->data.constExp.type == CT_FLOAT);
  test(
      status,
      "[ast] [constantExp] [float] [signs] parsing '+1.0' produces value +1e0.",
      node->data.constExp.value.floatBits == 0x3F800000);
  nodeDestroy(node);

  // float vs double
  string = strcpy(malloc(4), "1.1");
  node = constFloatExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [float] [type] parsing '1.1' produces type double.",
       node->data.constExp.type == CT_DOUBLE);
  test(
      status,
      "[ast] [constantExp] [float] [type] parsing '1.1' produces value +1.1e0.",
      node->data.constExp.value.doubleBits == 0x3FF199999999999A);
  nodeDestroy(node);

  string = strcpy(malloc(4), "1.5");
  node = constFloatExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [float] [type] parsing '1.5' produces type float.",
       node->data.constExp.type == CT_FLOAT);
  test(
      status,
      "[ast] [constantExp] [float] [type] parsing '1.5' produces value +1.5e0.",
      node->data.constExp.value.floatBits == 0x3FC00000);
  nodeDestroy(node);

  // positive and negative zero
  string = strcpy(malloc(5), "+0.0");
  node = constFloatExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [float] [zero] parsing '+0.0' produces type float.",
       node->data.constExp.type == CT_FLOAT);
  test(status,
       "[ast] [constantExp] [float] [zero] parsing '+0.0' produces value +0e0.",
       node->data.constExp.value.floatBits == 0x00000000);
  nodeDestroy(node);

  string = strcpy(malloc(5), "-0.0");
  node = constFloatExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [float] [zero] parsing '-0.0' produces type float.",
       node->data.constExp.type == CT_FLOAT);
  test(status,
       "[ast] [constantExp] [float] [zero] parsing '-0.0' produces value -0e0.",
       node->data.constExp.value.floatBits == 0x80000000);
  nodeDestroy(node);

  // accuracy of conversion
  string =
      strcpy(malloc(50), "3.14159265358979323846264338327950288419716939937");
  node = constFloatExpNodeCreate(0, 0, string);
  test(
      status,
      "[ast] [constantExp] [float] [accuracy] parsing pi produces type double.",
      node->data.constExp.type == CT_DOUBLE);
  test(status,
       "[ast] [constantExp] [float] [accuracy] parsing pi produces value "
       "+1.5707963267948966 * 2^1",
       node->data.constExp.value.doubleBits == 0x400921FB54442D18);
  nodeDestroy(node);
}
void constExpParseStringTest(TestStatus *status) {
  char *string;
  Node *node;

  // simple case
  string = strcpy(malloc(13), "abcd1234!@#$");
  node = constStringExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [string] [type] parsing \"abcd1234!@#$\" produces "
       "type string.",
       node->data.constExp.type == CT_STRING);
  test(
      status,
      "[ast] [constantExp] [string] [simple] parsing \"abcd1234!@#$\" "
      "produces value \"abcd1234!@#$\".",
      strcmp((char *)node->data.constExp.value.stringVal, "abcd1234!@#$") == 0);
  nodeDestroy(node);

  // escape sequence
  string = strcpy(malloc(17), "\\\"\\n\\r\\t\\0\\\\\\x0f");
  node = constStringExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [string] [type] parsing "
       "\"\\\"\\n\\r\\t\\0\\\\\\x0f\" "
       "produces type string.",
       node->data.constExp.type == CT_STRING);
  test(status,
       "[ast] [constantExp] [string] [escape] parsing "
       "\"\\\"\\n\\r\\t\\0\\\\\\x0f\" produces escaped string.",
       strcmp((char *)node->data.constExp.value.stringVal,
              "\"\n\r\t\0\\\x0f") == 0);
  nodeDestroy(node);
}
void constExpParseCharTest(TestStatus *status) {
  char *string;
  Node *node;

  // simple case
  string = strcpy(malloc(2), "a");
  node = constCharExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [char] [type] parsing 'a' produces type char.",
       node->data.constExp.type == CT_CHAR);
  test(status,
       "[ast] [constantExp] [char] [simple] parsing 'a' produces value 'a'.",
       node->data.constExp.value.charVal == 'a');
  nodeDestroy(node);

  // escaped quote
  string = strcpy(malloc(3), "\\'");
  node = constCharExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [char] [type] parsing '\\'' produces type char.",
       node->data.constExp.type == CT_CHAR);
  test(status,
       "[ast] [constantExp] [char] [escape] parsing '\\'' produces single "
       "quote.",
       node->data.constExp.value.charVal == '\'');
  nodeDestroy(node);

  // newline
  string = strcpy(malloc(3), "\\n");
  node = constCharExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [char] [type] parsing '\\n' produces type char.",
       node->data.constExp.type == CT_CHAR);
  test(status,
       "[ast] [constantExp] [char] [escape] parsing '\\n' produces newline.",
       node->data.constExp.value.charVal == '\n');
  nodeDestroy(node);

  // carriage return
  string = strcpy(malloc(3), "\\r");
  node = constCharExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [char] [type] parsing '\\r' produces type char.",
       node->data.constExp.type == CT_CHAR);
  test(status,
       "[ast] [constantExp] [char] [escape] parsing '\\r' produces carriage "
       "return.",
       node->data.constExp.value.charVal == '\r');
  nodeDestroy(node);

  // tab
  string = strcpy(malloc(3), "\\t");
  node = constCharExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [char] [type] parsing '\\t' produces type char.",
       node->data.constExp.type == CT_CHAR);
  test(status,
       "[ast] [constantExp] [char] [escape] parsing '\\t' produces tab.",
       node->data.constExp.value.charVal == '\t');
  nodeDestroy(node);

  // null
  string = strcpy(malloc(3), "\\0");
  node = constCharExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [char] [type] parsing '\\0' produces type char.",
       node->data.constExp.type == CT_CHAR);
  test(status,
       "[ast] [constantExp] [char] [escape] parsing '\\0' produces null.",
       node->data.constExp.value.charVal == '\0');
  nodeDestroy(node);

  // backslash
  string = strcpy(malloc(3), "\\\\");
  node = constCharExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [char] [type] parsing '\\\\' produces type char.",
       node->data.constExp.type == CT_CHAR);
  test(status,
       "[ast] [constantExp] [char] [escape] parsing '\\\\' produces '\\'.",
       node->data.constExp.value.charVal == '\\');
  nodeDestroy(node);

  // hex
  string = strcpy(malloc(5), "\\x0f");
  node = constCharExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [char] [type] parsing '\\x0f' produces type char.",
       node->data.constExp.type == CT_CHAR);
  test(status,
       "[ast] [constantExp] [char] [escape] parsing '\\x0f' produces shift in.",
       node->data.constExp.value.charVal == '\x0F');
  nodeDestroy(node);
}
void constExpParseWStringTest(TestStatus *status) {
  char *string;
  Node *node;

  // simple case
  string = strcpy(malloc(13), "abcd1234!@#$");
  node = constWStringExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [wstring] [type] parsing \"abcd1234!@#$\"w "
       "produces "
       "type wstring.",
       node->data.constExp.type == CT_WSTRING);
  test(status,
       "[ast] [constantExp] [wstring] [simple] parsing \"abcd1234!@#$\" "
       "produces value \"abcd1234!@#$\".",
       wcscmp((wchar_t *)node->data.constExp.value.stringVal,
              L"abcd1234!@#$") == 0);
  nodeDestroy(node);

  // escape sequence
  string = strcpy(malloc(27), "\\\"\\n\\r\\t\\0\\\\\\x0f\\u0001F34c");
  node = constWStringExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [wstring] [type] parsing "
       "\"\\\"\\n\\r\\t\\0\\\\\\x0f\\u0001F34c\"w produces type wstring.",
       node->data.constExp.type == CT_WSTRING);
  test(status,
       "[ast] [constantExp] [wstring] [escape] parsing "
       "\"\\\"\\n\\r\\t\\0\\\\\\x0f\\u0001F34c\"w produces escaped string.",
       wcscmp((wchar_t *)node->data.constExp.value.stringVal,
              L"\"\n\r\t\0\\\x0f\U0001F34c") == 0);
  nodeDestroy(node);
}
void constExpParseWCharTest(TestStatus *status) {
  char *string;
  Node *node;

  // simple case
  string = strcpy(malloc(2), "a");
  node = constWCharExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [wchar] [type] parsing 'a'w produces type wchar.",
       node->data.constExp.type == CT_WCHAR);
  test(status,
       "[ast] [constantExp] [wchar] [simple] parsing 'a'w produces value 'a'.",
       node->data.constExp.value.wcharVal == U'a');
  nodeDestroy(node);

  // escaped quote
  string = strcpy(malloc(3), "\\'");
  node = constWCharExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [wchar] [type] parsing '\\''w produces type wchar.",
       node->data.constExp.type == CT_WCHAR);
  test(status,
       "[ast] [constantExp] [wchar] [escape] parsing '\\''w produces single "
       "quote.",
       node->data.constExp.value.wcharVal == U'\'');
  nodeDestroy(node);

  // newline
  string = strcpy(malloc(3), "\\n");
  node = constWCharExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [wchar] [type] parsing '\\n'w produces type wchar.",
       node->data.constExp.type == CT_WCHAR);
  test(status,
       "[ast] [constantExp] [wchar] [escape] parsing '\\n'w produces newline.",
       node->data.constExp.value.wcharVal == U'\n');
  nodeDestroy(node);

  // carriage return
  string = strcpy(malloc(3), "\\r");
  node = constWCharExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [wchar] [type] parsing '\\r'w produces type wchar.",
       node->data.constExp.type == CT_WCHAR);
  test(status,
       "[ast] [constantExp] [wchar] [escape] parsing '\\r'w produces carriage "
       "return.",
       node->data.constExp.value.wcharVal == U'\r');
  nodeDestroy(node);

  // tab
  string = strcpy(malloc(3), "\\t");
  node = constWCharExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [wchar] [type] parsing '\\t'w produces type wchar.",
       node->data.constExp.type == CT_WCHAR);
  test(status,
       "[ast] [constantExp] [wchar] [escape] parsing '\\t'w produces tab.",
       node->data.constExp.value.wcharVal == U'\t');
  nodeDestroy(node);

  // null
  string = strcpy(malloc(3), "\\0");
  node = constWCharExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [wchar] [type] parsing '\\0'w produces type wchar.",
       node->data.constExp.type == CT_WCHAR);
  test(status,
       "[ast] [constantExp] [wchar] [escape] parsing '\\0'w produces null.",
       node->data.constExp.value.wcharVal == U'\0');
  nodeDestroy(node);

  // backslash
  string = strcpy(malloc(3), "\\\\");
  node = constWCharExpNodeCreate(0, 0, string);
  test(
      status,
      "[ast] [constantExp] [wchar] [type] parsing '\\\\'w produces type wchar.",
      node->data.constExp.type == CT_WCHAR);
  test(status,
       "[ast] [constantExp] [wchar] [escape] parsing '\\\\'w produces '\\'.",
       node->data.constExp.value.wcharVal == U'\\');
  nodeDestroy(node);

  // hex
  string = strcpy(malloc(5), "\\x0f");
  node = constWCharExpNodeCreate(0, 0, string);
  test(status,
       "[ast] [constantExp] [wchar] [type] parsing '\\x0f'w produces type "
       "wchar.",
       node->data.constExp.type == CT_WCHAR);
  test(status,
       "[ast] [constantExp] [wchar] [escape] parsing '\\x0f'w produces shift "
       "in.",
       node->data.constExp.value.wcharVal == U'\x0F');
  nodeDestroy(node);

  // long hex
  string = strcpy(malloc(11), "\\u0001F34c");
  node = constWCharExpNodeCreate(0, 0, string);
  test(
      status,
      "[ast] [constantExp] [wchar] [type] parsing '\\u0001F34c'w produces type "
      "wchar.",
      node->data.constExp.type == CT_WCHAR);
  test(status,
       "[ast] [constantExp] [wchar] [escape] parsing '\\u0001F34c'w produces "
       "emoji banana.",
       node->data.constExp.value.wcharVal == U'\U0001f34c');
  nodeDestroy(node);
}