/* 
 * CS:APP Data Lab 
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#include "btest.h"
#include <limits.h>

/*
 * Instructions to Students:
 *
 * STEP 1: Fill in the following struct with your identifying info.
 */
team_struct team =
{
   /* Team name: Replace with either:
      Your login ID if working as a one person team
      or, ID1+ID2 where ID1 is the login ID of the first team member
      and ID2 is the login ID of the second team member */
    "522031910118", 
   /* Student name 1: Replace with the full name of first team member */
   "TENG Haoyi",
   /* Login ID 1: Replace with the login ID of first team member */
   "522031910118",

   /* The following should only be changed if there are two team members */
   /* Student name 2: Full name of the second team member */
   "",
   /* Login ID 2: Login ID of the second team member */
   ""
};

#if 0
/*
 * STEP 2: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting an integer by more
     than the word size.

EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
     that you are allowed to use for your implementation of the function. 
     The max operator count is checked by dlc. Note that '=' is not 
     counted; you may use as many of these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.
#endif

/*
 * STEP 3: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the btest test harness to check that your solutions produce 
 *      the correct answers. Watch out for corner cases around Tmin and Tmax.
 */
/* Copyright (C) 1991-2020 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */
/* This header is separate from features.h so that the compiler can
   include it implicitly at the start of every compilation.  It must
   not itself include <features.h> or any other header that includes
   <features.h> because the implicit include comes before any feature
   test macros that may be defined in a source file before it first
   explicitly includes a system header.  GCC knows the name of this
   header in order to preinclude it.  */
/* glibc's intent is to support the IEC 559 math functionality, real
   and complex.  If the GCC (4.9 and later) predefined macros
   specifying compiler intent are available, use them to determine
   whether the overall intent is to support these features; otherwise,
   presume an older compiler has intent to support these features and
   define these macros by default.  */
/* wchar_t uses Unicode 10.0.0.  Version 10.0 of the Unicode Standard is
   synchronized with ISO/IEC 10646:2017, fifth edition, plus
   the following additions from Amendment 1 to the fifth edition:
   - 56 emoji characters
   - 285 hentaigana
   - 3 additional Zanabazar Square characters */

/*
 * bitCount - returns count of number of 1's in word
 *   Examples: bitCount(5) = 2, bitCount(7) = 3
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 40
 *   Rating: 4
 */
int bitCount(int x) {
  int mask = 0x11 | (0x11 << 8);
  int mask2 = (mask << 16) | mask;
  int s = x & mask2;
  s += (x >> 1) & mask2;
  s += (x >> 2) & mask2;
  s += (x >> 3) & mask2;
  s += s >> 16;
  mask = (0xf << 8) | 0xf;
  s = (s & mask) + ((s >> 4) & mask);
  return (s + (s >> 8)) & 0x3f;
}
/* 
 * copyLSB - set all bits of result to least significant bit of x
 *   Example: copyLSB(5) = 0xFFFFFFFF, copyLSB(6) = 0x00000000
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int copyLSB(int x) {
  int mask = 0x1 & x;
  mask <<= 31;
  mask >>= 31;  
  return mask;
}
/* 
 * evenBits - return word with all even-numbered bits set to 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 8
 *   Rating: 2
 */
int evenBits(void) {
  int mask = 0x55 | (0x55 << 8);
  int mask1 =  (mask << 16) | mask;
  return mask1;
}
/* 
 * fitsBits - return 1 if x can be represented as an 
 *  n-bit, two's complement integer.
 *   1 <= n <= 32
 *   Examples: fitsBits(5,3) = 0, fitsBits(-4,3) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 2
 */
int fitsBits(int x, int n) {
  	//int mask0 = 0x1 << 31;
	//return (((!(x & mask0) - 1) & !((~x + (1 & !!(mask0 ^ x))) >> (n - 1))) || !((x >> (n - 1)) >> 1));
	//int mask0 = 0x1 << 31;
        //return (((!(x & mask0) + ~0) & !((~x + (1 & !!(mask0 ^ x))) >> (n + ~0))) ^ (!((x >> (n + ~0)) >> 1)));
	int mask0 = x >> 31;
	int mask2 = n + ~0x0;
	int mask1 = (~mask0 & x) | (mask0 & ~x);
	//return (mask0 & !((~x) >> mask2)) | (~mask0 & !((x >> mask2) >> 1));i
	return !(mask1 >> (mask2));
}
/* 
 * getByte - Extract byte n from word x
 *   Bytes numbered from 0 (LSB) to 3 (MSB)
 *   Examples: getByte(0x12345678,1) = 0x56
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 6
 *   Rating: 2
 */
int getByte(int x, int n) {
	int mask0 = (0xf) | (0xf << 4);
	int mask2 = n << 3;
  return  (x >> mask2) & (mask0);
}
/* 
 * isGreater - if x > y  then return 1, else return 0 
 *   Example: isGreater(4,5) = 0, isGreater(5,4) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isGreater(int x, int y) {
	int mask = (x ^ y) >> 31;
	int mask1 = x >> 31;
	int mask2 = ~x + 1;
	int mask3 = 0x1 << 31;
    return (mask & !(mask1) ) | (~mask & !!((y + mask2) & mask3));
}
/* 
 * isNonNegative - return 1 if x >= 0, return 0 otherwise 
 *   Example: isNonNegative(-1) = 0.  isNonNegative(0) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 6
 *   Rating: 3
 */
int isNonNegative(int x) {
  	int mask = 0x1 << 31 & x;
	return !mask;
}
/* 
 * isNotEqual - return 0 if x == y, and 1 otherwise 
 *   Examples: isNotEqual(5,5) = 0, isNotEqual(4,5) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 6
 *   Rating: 2
 */
int isNotEqual(int x, int y) {	
  return !!(x ^ y);
}
/* 
 * leastBitPos - return a mask that marks the position of the
 *               least significant 1 bit. If x == 0, return 0
 *   Example: leastBitPos(96) = 0x20
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 6
 *   Rating: 4 
 */
int leastBitPos(int x) {

	return (x & (~x + 1));
}
/* 
 * logicalShift - shift x to the right by n, using a logical shift
 *   Can assume that 1 <= n <= 31
 *   Examples: logicalShift(0x87654321,4) = 0x08765432
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3 
 */
int logicalShift(int x, int n) {
	int mask = (0x1 << 31) & x;

  return (mask >> (n + ~0x0) ^ (x >> n));
}
/*
 * satAdd - adds two numbers but when positive overflow occurs, returns
 *          the maximum value, and when negative overflow occurs,
 *          it returns the minimum value.
 *   Examples: satAdd(0x40000000,0x40000000) = 0x7fffffff
 *             satAdd(0x80000000,0xffffffff) = 0x80000000
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 30
 *   Rating: 4
 */
int satAdd(int x, int y) {
   int sum = x + y;
   int mask0 = x >> 31;
   int mask1 = (sum ^ x) >> 31;
   int mask2 = (sum ^ y) >> 31;
   int mask3 = mask2 & mask1;
   int min = 0x1 << 31;
   int max = ~min;
   return (~mask3 & (sum)) | (mask3 & ((~mask0 & max) | (mask0 & min)));
}


/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
int howManyBits(int x) {
	int mask0 = ~0x0;
  	int mask = x >> 31;
	int mask1 = (~mask & x) | (mask & ~x);
	int nums =  0;
	int mask2 = mask1 >> 16;
	int flag2 = !mask2 + mask0;
	int mask3 = (flag2 &  mask2) | (mask1 & ~flag2);
	int mask4 = mask3 >> 8;
	int flag4 = !mask4 + mask0;
	int mask5 = (flag4 & mask4) | (~flag4 & mask3);
	int mask6 = mask5 >> 4;
	int flag6 = !mask6 + mask0;
	int mask7 = (flag6 & mask6) | (~flag6 & mask5);
	int mask8 = mask7 >> 2;
	int flag8 = !mask8 + mask0;
	int mask9 = (flag8 & mask8) | (~flag8 & mask7);
	int mask10 = mask9 >> 1;
	int flag10 = !mask10 + mask0;
	nums += (flag2 & 16);
	nums += (flag4 & 8);
	nums += (flag6 & 4);
	nums += (flag8 & 2);
   nums += (flag10 & 1);
   return nums + (1 & (!mask1 + mask0)) + 1;
   /*int bt17 = mask1 >> 16; //greater than 17 bits
	int bt25 = bt17  >> 8;  //greater than 25 bits
	int bt9 = mask1 >> 8;  //greater than 9 bits
	int bt29 = bt25 >> 4; //greater than 29 bits
	int bt5 = mask1  >> 4;
	int bt13 = bt9 >> 4;
	int bt7 = bt5 >> 2;
	int bt3 = mask1 >> 2;
	int bt2 = mask1 >> 1;
	int bt11 = bt9 >> 2;*/
	/*int mask2 = 0xff << 8;
	int mask3 = mask2 & mask1;
	int mask4 = (~mask2 & mask1) >> 16;
	int mask5 = 0xff;
	int flag0 = mask3 & mask5;
	int flag1 = (mask3 & ~mask5) >> 8;
	int flag2 = mask4 & mask5;
	int flag3 = (mask4 & ~mask5) >> 8;
	int mask6 = 0xf;
	int 
	int num = (flag4 & (((!flag3) - 1)& )) |	
	*/
	/*
	int b3 = !(mask1 >> 2) - 1;
	int b5 = !(mask1 >> 4) - 1;
	int b7 = !(mask1 >> 6) - 1;
	int b9 = !(mask1 >> 8) - 1;
	int b11 = !(mask1 >> 10) - 1;
	int b13 = !(mask1 >> 12) - 1;
	int b15 = !(mask1 >> 14) - 1;
	int b17 = !(mask1 >> 16) - 1;
	int b19 = !(mask1 >> 18) - 1;
	int b21 = !(mask1 >> 20) - 1;
	int b23 = !(mask1 >> 22) - 1;
	int b25 = !(mask1 >> 24) - 1;
	int b27 = !(mask1 >> 26) - 1;
	int b29 = !(mask1 >> 28) - 1;
	int b31 = !(mask1 >> 30) - 1;
	*/	

	return 2; 
}

/* 
 * logicalNeg - implement the ! operator, using all of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int logicalNeg(int x) {
  	int a = 0x1;	
  	int b = a << 31; //get the sign bit
	//avoid sar
  	return (((b ^ x) & ((~x + 1) ^ b)) >> 31) & a;
}

/* 
 * dividePower2 - Compute x/(2^n), for 0 <= n <= 30
 *  Round toward zero
 *   Examples: dividePower2(15,1) = 7, dividePower2(-33,4) = -2
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 */
int dividePower2(int x, int n) {
    	int mask = (0x1 << n) + ~0x0;
	int bias = (x >> 31) & mask;
	return (x + bias) >> n;
}

/* 
 * bang - Convert from two's complement to sign-magnitude 
 *   where the MSB is the sign bit
 *   You can assume that x > TMin
 *   Example: bang(-5) = 0x80000005.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 4
 */
int bang(int x) {
	int mask = 0x1 << 31;
   int mask1 = x >> 31;
   return ((mask1) & ((~x + 1) | mask)) | ((~mask1) & (x));
}

