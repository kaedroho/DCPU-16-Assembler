/* dcpu16asm.c - Emulator for the DCPU-16 CPU
   version 1, 6 April 2012

   Copyright (C) 2012 Karl Hobley

   Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation
   files (the "Software"), to deal in the Software without
   restriction, including without limitation the rights to use, copy,
   modify, merge, publish, distribute, sublicense, and/or sell copies
   of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Karl Hobley <turbodog10@yahoo.co.uk>
*/

#include <stdio.h>
#include <memory.h>

struct dcpu16
{
	unsigned short ram[0x100000];
	unsigned short a, b, c, x, y, z, i, j;
	unsigned short pc;
	unsigned short sp;
	unsigned short o;
} cpu;

unsigned short* decode_parameter(unsigned char paramvalue, unsigned short* literal)
{
	/* Check if this is a register */
	if (paramvalue < 0x08) {
		unsigned short* registers = &cpu.a;
		return &registers[paramvalue];
	}
	
	/* Check if this is a register pointer */
	if (paramvalue < 0x10) {
		unsigned short* registers = &cpu.a;
		return &cpu.ram[registers[paramvalue]];
	}
	
	/* Check if this is a register pointer with added word value */
	if (paramvalue < 0x18) {
		unsigned short* registers = &cpu.a;
		unsigned short word = cpu.ram[cpu.pc++];
		return &cpu.ram[registers[paramvalue] + word];
	}
	
	/* POP */
	if (paramvalue == 0x18) {
		/*unsigned short* registers = &cpu.a;
		unsigned short word = cpu.ram[cpu.pc++];
		return &cpu.ram[registers[paramvalue] + word];
		*/
	}
	
	/* PEEK */
	if (paramvalue == 0x19) {
		/*unsigned short* registers = &cpu.a;
		unsigned short word = cpu.ram[cpu.pc++];
		return &cpu.ram[registers[paramvalue] + word];
		*/
	}
	
	/* PUSH */
	if (paramvalue == 0x1a) {
		/*unsigned short* registers = &cpu.a;
		unsigned short word = cpu.ram[cpu.pc++];
		return &cpu.ram[registers[paramvalue] + word];
		*/
	}
	
	/* SP */
	if (paramvalue == 0x1b) {
		return &cpu.sp;
	}
	
	/* PC */
	if (paramvalue == 0x1c) {
		return &cpu.pc;
	}
	
	/* O */
	if (paramvalue == 0x1d) {
		return &cpu.o;
	}
	
	/* Check if this is a word pointer */
	if (paramvalue < 0x1e) {
		unsigned short word = cpu.ram[cpu.pc++];
		return &cpu.ram[word];
	}
	
	/* Check if this is a word literal */
	if (paramvalue < 0x1f) {
		return &cpu.ram[cpu.pc++];
	}
	
	/* This must be a literal */
	*literal = paramvalue - 0x20;
	return literal;
}

void run_instruction()
{
	/* Get first word */
	unsigned short first_word = cpu.ram[cpu.pc++];
	
	/* Decode operation */
	unsigned char opcode = first_word & 0xF;
	unsigned char parama = (first_word >> 4) & 0x3F;
	unsigned char paramb = (first_word >> 10) & 0x3F;
	
	if (opcode == 0x0) { /* Non basic instruction */
	
	} else {
		/* Decode parameters */
		unsigned short parama_literal = 0; /* These are here just incase the parameter is a literal */
		unsigned short paramb_literal = 0; /* It will need a different place to store these */
		unsigned short* parama_value = decode_parameter(parama, &parama_literal);
		unsigned short* paramb_value = decode_parameter(paramb, &paramb_literal);
		
		/* Decode operation */
		if (opcode == 0x1) { /* SET */
			*parama_value = *paramb_value;
		} else if (opcode == 0x2) { /* ADD */
		
		} else if (opcode == 0x3) { /* SUB */
		
		} else if (opcode == 0x4) { /* MUL */
		
		} else if (opcode == 0x5) { /* DIV */
		
		} else if (opcode == 0x6) { /* MOD */
		
		} else if (opcode == 0x7) { /* SHL */
		
		} else if (opcode == 0x8) { /* SHR */
		
		} else if (opcode == 0x9) { /* AND */
		
		} else if (opcode == 0xA) { /* BOR */
		
		} else if (opcode == 0xB) { /* XOR */
		
		} else if (opcode == 0xC) { /* IFE */
		
		} else if (opcode == 0xD) { /* IFN */
		
		} else if (opcode == 0xE) { /* IFG */
		
		} else if (opcode == 0xF) { /* IFB */
		
		}
	}
}

int main(int argc, char* argv[])
{
	/* Process arguements */
	if (argc > 2 || argc < 2) {
		printf("useage: %s input\n", argv[0]);
		return 0;
	}
	
	/* Open input file */
	FILE* input = fopen(argv[1], "r");
	if (input == 0) {
		printf("failed to open input file\n");
		return 0;
	}
	
	/* Initialise CPU */
	memset(&cpu, 0, sizeof(struct dcpu16));
	
	/* Read words into RAM */
	fread(cpu.ram, 2, 0x100000, input);
	
	/* Run */
	for (;;)
		run_instruction();
}
