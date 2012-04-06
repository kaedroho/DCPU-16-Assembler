/* dcpu16asm.c - Emulator for the DCPU-16 CPU
   version 2, 6 April 2012

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
	int skip_next_instruction;
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
		return &cpu.ram[cpu.sp++];
	}
	
	/* PEEK */
	if (paramvalue == 0x19) {
		return &cpu.ram[cpu.sp];
	}
	
	/* PUSH */
	if (paramvalue == 0x1a) {
		return &cpu.ram[--cpu.sp];
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
	if (paramvalue == 0x1e) {
		unsigned short word = cpu.ram[cpu.pc++];
		return &cpu.ram[word];
	}
	
	/* Check if this is a word literal */
	if (paramvalue == 0x1f) {
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
	printf("OPCODE: %04X (%u)\n", first_word, opcode);
	
	if (opcode == 0x0) { /* Non basic instruction */
		/* Decode parameter */
		unsigned short param_literal = 0;
		unsigned short* param_value = decode_parameter(paramb, &param_literal);
		
		/* Decode operation */
		if (cpu.skip_next_instruction == 0) {
			if (parama == 0x1) { /* JSR */
				cpu.ram[--cpu.sp] = cpu.pc;
				cpu.pc = *param_value;
			}
		} else {
			cpu.skip_next_instruction = 0;
		}
	} else {
		/* Decode parameters */
		unsigned short parama_literal = 0; /* These are here just incase the parameter is a short literal */
		unsigned short paramb_literal = 0; /* It will need a different place to store short literals */
		unsigned short* parama_value = decode_parameter(parama, &parama_literal);
		unsigned short* paramb_value = decode_parameter(paramb, &paramb_literal);
		
		/* Decode operation */
		if (cpu.skip_next_instruction == 0) {
			if (opcode == 0x1) { /* SET */
				*parama_value = *paramb_value;
			} else if (opcode == 0x2) { /* ADD */
				unsigned int value = *parama_value + *paramb_value;
				if (value > 0xFFFF) {
					cpu.o = 0x0001;
				}
				*parama_value = value & 0xFFFF;
			} else if (opcode == 0x3) { /* SUB */
				int value = *parama_value - *paramb_value;
				if (value < 0) {
					cpu.o = 0xFFFF;
					*parama_value = -value;
				} else{
					*parama_value = value;
				}
			} else if (opcode == 0x4) { /* MUL */
				unsigned int value = *parama_value * *paramb_value;
				cpu.o = (value >> 16) & 0xFFFF;
				*parama_value = value & 0xFFFF;
			} else if (opcode == 0x5) { /* DIV */
				if (*paramb_value == 0) {
					cpu.o = 0;
					*parama_value = 0;
				} else {
					cpu.o = ((*parama_value << 16) / *paramb_value) & 0xFFFF;
					*parama_value = *parama_value / *paramb_value;
				}
			} else if (opcode == 0x6) { /* MOD */
				if (*paramb_value == 0) {
					*parama_value = 0;
				} else {
					*parama_value = *parama_value % *paramb_value;
				}
			} else if (opcode == 0x7) { /* SHL */
				cpu.o = (*parama_value << (*paramb_value - 16)) & 0xFFFF;
				*parama_value = *parama_value << *paramb_value;
			} else if (opcode == 0x8) { /* SHR */
				cpu.o = (*parama_value >> (*paramb_value - 16)) & 0xFFFF;
				*parama_value = *parama_value >> *paramb_value;
			} else if (opcode == 0x9) { /* AND */
				*parama_value = *parama_value & *paramb_value;
			} else if (opcode == 0xA) { /* BOR */
				*parama_value = *parama_value | *paramb_value;
			} else if (opcode == 0xB) { /* XOR */
				*parama_value = *parama_value ^ *paramb_value;
			} else if (opcode == 0xC) { /* IFE */
				if (*parama_value != *paramb_value)
					cpu.skip_next_instruction = 1;
			} else if (opcode == 0xD) { /* IFN */
				if (*parama_value == *paramb_value)
					cpu.skip_next_instruction = 1;
			} else if (opcode == 0xE) { /* IFG */
				if (*parama_value <= *paramb_value)
					cpu.skip_next_instruction = 1;
			} else if (opcode == 0xF) { /* IFB */
				if ((*parama_value & *paramb_value) == 0)
					cpu.skip_next_instruction = 1;
			}
		} else {
			cpu.skip_next_instruction = 0;
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
	cpu.sp = 0xFFFF;
	
	/* Read words into RAM */
	fread(cpu.ram, 2, 0x100000, input);
	
	/* Run */
	for (;;) {
		printf("\nA: %04X, B: %04X, C: %04X, X: %04X, Y: %04X, Z: %04X, I: %04X, J: %04X, PC: %04X, SP: %04X, O: %04X\n", cpu.a, cpu.b, cpu.c, cpu.x, cpu.y, cpu.z, cpu.i, cpu.j, cpu.pc, cpu.sp, cpu.o);
		run_instruction();
	}

}
