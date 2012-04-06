/* dcpu16asm.c - Assembler for the DCPU-16 CPU
   version 4.1, 5 April 2012

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
#include <string.h>

int line_num;
int exit_app;
FILE* output;
int current_address;

struct label
{
	char name[255];
	unsigned int address;
} labels[256];

int label_count;

struct labelref
{
	char name[255];
	unsigned int address;
} labelrefs[256];

int labelref_count;

unsigned char get_register_id(char reg)
{
	if (reg == 'A') {
		return 0x00;
	} else if (reg == 'B') {
		return 0x01;
	} else if (reg == 'C') {
		return 0x02;
	} else if (reg == 'X') {
		return 0x03;
	} else if (reg == 'Y') {
		return 0x04;
	} else if (reg == 'Z') {
		return 0x05;
	} else if (reg == 'I') {
		return 0x06;
	} else if (reg == 'J') {
		return 0x07;
	}
}

/*
 * This removes whitespace and comments. It also converts all text to uppercase.
 */
void clean_line(char* cleaned_line, const char* line)
{
	/* Loop through chars */
	int char_num = 0;
	int clean_char_num = 0;
	while (line[char_num] != 0 && line[char_num] != '\n' && line[char_num] != ';') {
		/* Look for labels */
		if(line[char_num] == ':') {
			int label_char_num = 0;
			char_num++;
			while ((line[char_num] >= '0' && line[char_num] <= '9')
				|| (line[char_num] >= 'a' && line[char_num] <= 'z')
				|| (line[char_num] >= 'A' && line[char_num] <= 'Z')) {
					
				if (line[char_num] >= 'a' && line[char_num] <= 'z')
					labels[label_count].name[label_char_num] = toupper(line[char_num]);
				else
					labels[label_count].name[label_char_num] = line[char_num];
				labels[label_count].address = current_address;
				char_num++;
				label_char_num++;
			}
			label_count++;
		}
		
		/* Filter out non characters */
		if (line[char_num] >= '!' && line[char_num] <= '~')
		{
			char current_char = line[char_num];
			
			/* Convert to upper case */
			if (current_char >= 'a' && current_char <= 'z')
				current_char = toupper(current_char);
				
			/* Place in cleaned line */
			cleaned_line[clean_char_num] = current_char;
			clean_char_num++;
		}
		
		/* Increment char number */
		char_num++;
	}
	
	/* Null terminate cleaned line */
	cleaned_line[clean_char_num] = 0;
}

/*
 * This takes a parameter string. eg, "A", "0x1234", "[0x1234]" and converts it into a 6 bit value
 */
unsigned char decode_parameter(char* param, int* extra_word_needed, unsigned short* extra_word_value)
{
	/* Check for square brackets */
	int square_brackets = 0;
	int first_sqbracket = 0, last_sqbracket = 0;
	if (param[0] == '[') {
		first_sqbracket = 1;
		param++;
	}
	if (param[strlen(param) - 1] == ']')  {
		last_sqbracket = 1;
		param[strlen(param) - 1] = 0;
	}
	
	/* Check for errors */
	if (first_sqbracket == 1) {
		square_brackets = 1;
		if (last_sqbracket != 1) {
			printf("Missing last square bracket on line %d\n", line_num);
			exit_app = 1;
			return 0;
		}
	} else {
		if (last_sqbracket == 1) {
			printf("Missing first square bracket on line %d\n", line_num);
			exit_app = 1;
			return 0;
		}
	}
	
	/* Check if this is a hex literal */
	if (param[0] == '0' && param[1] == 'X') {
		/* Decode hex */
		unsigned short value = 0;
		param = param + 2; /* Remove 0x */
		int digit_count = strlen(param);
		int digit_num = 0;
		int reg = -1;
		for (digit_num = 0; digit_num < digit_count; digit_num++) {
			/* Get Digit value */
			int digit_val = -1;
			char current_digit = param[digit_num];
			if (current_digit >= '0' && current_digit <= '9')
				digit_val = current_digit - '0';
			if (current_digit >= 'A' && current_digit <= 'F')
				digit_val = current_digit - 'A' + 10;
				
			if (current_digit == '+' && square_brackets == 1) {
				reg = get_register_id(param[digit_num + 1]);
				digit_num++;
			} else {
				/* Check for errors */
				if (digit_val == -1) {
					printf("invalid literal on line %d\n", line_num);
					exit_app = 1;
					return 0;
				}
				
				/* Merge into final number */
				value = (value << 4) + digit_val;
			}
		}
		
		if (value <= 0x1f && square_brackets == 0)
			return value + 0x20;
			
		*extra_word_needed = 1;
		*extra_word_value = value;
		
		if (square_brackets == 1) {
			if (reg != -1) {
				return 0x10 + reg;
			} else {
				
				return 0x1e;
			}
		} else {
			return 0x1f;
		}
	}
	
	/* Check if this is a decimal literal */
	if (param[0] >= '0' && param[0] <= '9') {
		/* Decode decimal */
		unsigned short value = 0;
		int digit_count = strlen(param);
		int digit_num = 0;
		int reg = -1;
		for (digit_num = 0; digit_num < digit_count; digit_num++) {
			/* Get Digit value */
			int digit_val = -1;
			char current_digit = param[digit_num];
			if (current_digit >= '0' && current_digit <= '9')
				digit_val = current_digit - '0';
				
			if (current_digit == '+' && square_brackets == 1) {
				reg = get_register_id(param[digit_num + 1]);
				digit_num++;
			} else {
				/* Check for errors */
				if (digit_val == -1) {
					printf("invalid literal on line %d\n", line_num);
					exit_app = 1;
					return 0;
				}
				
				/* Merge into final number */
				value = (value * 10) + digit_val;
			}
		}
		if (value <= 0x1f && square_brackets == 0)
			return value + 0x20;
			
		*extra_word_needed = 1;
		*extra_word_value = value;
		
		if (square_brackets == 1) {
			if (reg != -1) {
				return 0x10 + reg;
			} else {
				return 0x1e;
			}
		} else {
			return 0x1f;
		}
	}
	
	/* Check if this is a register */
	if (param[1] == 0) { /* This is a quick way to check that this is 1 character long */
		unsigned char reg = get_register_id(param[0]);
		if (square_brackets == 1)
			reg += 0x08;
		return reg;
	}
	
	/* Check if this is a word */
	if (strncmp("POP", param, 3) == 0) {
		return 0x18;
	} else if (strncmp("PEEK", param, 4) == 0) {
		return 0x19;
	} else if (strncmp("PUSH", param, 4) == 0) {
		return 0x1a;
	} else if (strncmp("SP", param, 2) == 0) {
		return 0x1b;
	} else if (strncmp("PC", param, 2) == 0) {
		return 0x1c;
	} else if (strncmp("O", param, 1) == 0) {
		return 0x1d;
	}
	
	/* Must be a label, store a labelref */
	strncpy(labelrefs[labelref_count].name, param, 255);
	labelrefs[labelref_count].address = current_address;
	labelref_count++;
	*extra_word_needed = 1; /* Allocate blank extra word, this will be where the pointer to the label will be stored at link stage */
	*extra_word_value = 0; 
	return 0x1f;
}

void process_line(char* uncleaned_line)
{
	/* Clean line */
	char line[256];
	clean_line(line, uncleaned_line);
	
	/* Check if this is a blank line */
	if (strlen(line) == 0)
		return;
		
	/* Work out instruction */
	int basic_opcode = 0;
	int non_basic_opcode = 0;
	
	/* Basic instructions */
	if (strncmp("SET", line, 3) == 0 ) {
		basic_opcode = 0x1;
	} else if (strncmp("ADD", line, 3) == 0) {
		basic_opcode = 0x2;
	} else if (strncmp("SUB", line, 3) == 0) {
		basic_opcode = 0x3;
	} else if (strncmp("MUL", line, 3) == 0) {
		basic_opcode = 0x4;
	} else if (strncmp("DIV", line, 3) == 0) {
		basic_opcode = 0x5;
	} else if (strncmp("MOD", line, 3) == 0) {
		basic_opcode = 0x6;
	} else if (strncmp("SHL", line, 3) == 0) {
		basic_opcode = 0x7;
	} else if (strncmp("SHR", line, 3) == 0) {
		basic_opcode = 0x8;
	} else if (strncmp("AND", line, 3) == 0) {
		basic_opcode = 0x9;
	} else if (strncmp("BOR", line, 3) == 0) {
		basic_opcode = 0xA;
	} else if (strncmp("XOR", line, 3) == 0) {
		basic_opcode = 0xB;
	} else if (strncmp("IFE", line, 3) == 0) {
		basic_opcode = 0xC;
	} else if (strncmp("IFN", line, 3) == 0) {
		basic_opcode = 0xD;
	} else if (strncmp("IFG", line, 3) == 0) {
		basic_opcode = 0xE;
	} else if (strncmp("IFB", line, 3) == 0) {
		basic_opcode = 0xF;
	}
	
	/* Non basic instructions */
	if (basic_opcode == 0) {
		if (strncmp("JSR", line, 3) == 0) {
			non_basic_opcode = 0x1;
		} else {
			printf("Unrecognised instruction on line %d\n", line_num);
			exit_app = 1;
			return;
		}
	}
	
	/* Decode basic instructions */
	if (basic_opcode != 0) {
		char* parameters = line + 3; /* Cut off first 3 characters */
		unsigned char parametera = 0, parameterb = 0;
		
		/* Find comma */
		int char_num = 0;
		while (parameters[char_num] != ','
			&& parameters[char_num] != '\n'
			&& parameters[char_num] != 0 && char_num < 256)
			char_num++;
			
		if (parameters[char_num] == ',') {
			/* Split parameter string to A and B */
			parameters[char_num] = 0;
			char* parama = parameters;
			char* paramb = parameters + char_num + 1;
			
			/* Increment address for the start word */
			current_address++;
			
			/* Parameter A */
			int parametera_extra_word_needed = 0;
			unsigned short parametera_extra_word_value = 0;
			parametera = decode_parameter(parama, &parametera_extra_word_needed, &parametera_extra_word_value);
			if(exit_app == 1)
				return;
			if (parametera_extra_word_needed == 1)
				current_address++;
				
			/* Parameter B */
			int parameterb_extra_word_needed = 0;
			unsigned short parameterb_extra_word_value = 0;
			parameterb = decode_parameter(paramb, &parameterb_extra_word_needed, &parameterb_extra_word_value);
			if(exit_app == 1)
				return;
			if (parameterb_extra_word_needed == 1)
				current_address++;
				
			/* Put everything together */
			unsigned short first_word = ((parameterb & 0x3F) << 10) | ((parametera & 0x3F) << 4) | (basic_opcode & 0xF);
			
			/* Print to screen */
			printf("%04X ", first_word);
			if(parametera_extra_word_needed == 1)
				printf("%04X ", parametera_extra_word_value);
			if(parameterb_extra_word_needed == 1)
				printf("%04X ", parameterb_extra_word_value);
			printf("\n");
			
			/* Write to file */
			fwrite(&first_word, 1, 2, output);
			if(parametera_extra_word_needed == 1)
				fwrite(&parametera_extra_word_value, 1, 2, output);
			if(parameterb_extra_word_needed == 1)
				fwrite(&parameterb_extra_word_value, 1, 2, output);
		} else {
			printf("Missing comma on line %d\n", line_num);
			exit_app = 1;
			return;
		}
	}
	
	/* Non basic instructions */
	if (non_basic_opcode == 0x1) { /* JSR */
		char* param = line + 3; /* Cut off first 3 characters */
		unsigned char parameter = 0;
		
		/* Increment address for the start word */
		current_address++;
		
		/* Decode parameter */
		int parameter_extra_word_needed = 0;
		unsigned short parameter_extra_word_value = 0;
		parameter = decode_parameter(param, &parameter_extra_word_needed, &parameter_extra_word_value);
		if(exit_app == 1)
			return;
		if (parameter_extra_word_needed == 1)
			current_address++;
			
		/* Put everything together */
		unsigned short first_word = ((parameter & 0x3F) << 10) | ((non_basic_opcode & 0x3F) << 4) | (basic_opcode & 0xF);
		
		/* Print to screen */
		printf("%04X ", first_word);
		if(parameter_extra_word_needed == 1)
			printf("%04X ", parameter_extra_word_value);
		printf("\n");
		
		/* Write to file */
		fwrite(&first_word, 1, 2, output);
		if(parameter_extra_word_needed == 1)
			fwrite(&parameter_extra_word_value, 1, 2, output);
	}
	
}

int main(int argc, char* argv[])
{
	/* Process arguements */
	if (argc > 3 || argc < 2) {
		printf("useage: %s input [output]\n", argv[0]);
		return 0;
	}
	
	/* Open input file */
	FILE* input = fopen(argv[1], "r");
	if (input == 0) {
		printf("failed to open input file\n");
		return 0;
	}
	
	/* Open output file */
	output = 0;
	if (argc == 2)
		output = fopen("out.bin", "wb");
	else
		output = fopen(argv[2], "wb");
	if (output == 0) {
		printf("failed to open output file\n");
		return 0;
	}
	
	/* Read lines */
	char line[256];
	line_num = 1;
	exit_app = 0;
	current_address = 0;
	while (fgets(line, 256, input) != 0) {
		process_line(line);
		if(exit_app)
			return 0;
		line_num++;
	}
	
	int label_num = 0;
	for (label_num = 0; label_num < label_count; label_num++) {
		printf("LABEL: %s (%04X)\n", labels[label_num].name, labels[label_num].address);
	}
	
	/* Link */
	int labelref_num = 0;
	for (labelref_num = 0; labelref_num < labelref_count; labelref_num++) {
		int label_num = 0;
		for (label_num = 0; label_num < label_count; label_num++) {
			if (strcmp(labels[label_num].name, labelrefs[labelref_num].name) == 0) {
				/* Seek to position of label ref */
				fseek(output, labelrefs[labelref_num].address * 2, SEEK_SET);
				fwrite(&labels[label_num].address, 1, 2, output);
				printf("LINKED: %s (%04X)\n", labels[label_num].name, labelrefs[labelref_num].address);
			}
		}
	}
}
