#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#include "y64asm.h"

line_t* line_head = NULL;
line_t* line_tail = NULL;
int lineno = 0;

#define err_print(_s, _a ...) do { \
  if (lineno < 0) \
    fprintf(stderr, "[--]: "_s"\n", ## _a); \
  else \
    fprintf(stderr, "[L%d]: "_s"\n", lineno, ## _a); \
} while (0);

#define fun(statement) do { \
    statement;\
} while (0)\

int64_t vmaddr = 0;    /* vm addr */

/* register table */
const reg_t reg_table[REG_NONE] = {
	{ "%rax", REG_RAX, 4 },
	{ "%rcx", REG_RCX, 4 },
	{ "%rdx", REG_RDX, 4 },
	{ "%rbx", REG_RBX, 4 },
	{ "%rsp", REG_RSP, 4 },
	{ "%rbp", REG_RBP, 4 },
	{ "%rsi", REG_RSI, 4 },
	{ "%rdi", REG_RDI, 4 },
	{ "%r8", REG_R8, 3 },
	{ "%r9", REG_R9, 3 },
	{ "%r10", REG_R10, 4 },
	{ "%r11", REG_R11, 4 },
	{ "%r12", REG_R12, 4 },
	{ "%r13", REG_R13, 4 },
	{ "%r14", REG_R14, 4 }
};
const reg_t* find_register(char* name) {
	int i;
	for (i = 0; i < REG_NONE; i++)
		if (!strncmp(name, reg_table[i].name, reg_table[i].namelen))
			return &reg_table[i];
	return NULL;
}

/* instruction set */
instr_t instr_set[] = {
	{ "nop", 3, HPACK(I_NOP, F_NONE), 1 },
	{ "halt", 4, HPACK(I_HALT, F_NONE), 1 },
	{ "rrmovq", 6, HPACK(I_RRMOVQ, F_NONE), 2 },
	{ "cmovle", 6, HPACK(I_RRMOVQ, C_LE), 2 },
	{ "cmovl", 5, HPACK(I_RRMOVQ, C_L), 2 },
	{ "cmove", 5, HPACK(I_RRMOVQ, C_E), 2 },
	{ "cmovne", 6, HPACK(I_RRMOVQ, C_NE), 2 },
	{ "cmovge", 6, HPACK(I_RRMOVQ, C_GE), 2 },
	{ "cmovg", 5, HPACK(I_RRMOVQ, C_G), 2 },
	{ "irmovq", 6, HPACK(I_IRMOVQ, F_NONE), 10 },
	{ "rmmovq", 6, HPACK(I_RMMOVQ, F_NONE), 10 },
	{ "mrmovq", 6, HPACK(I_MRMOVQ, F_NONE), 10 },
	{ "addq", 4, HPACK(I_ALU, A_ADD), 2 },
	{ "subq", 4, HPACK(I_ALU, A_SUB), 2 },
	{ "andq", 4, HPACK(I_ALU, A_AND), 2 },
	{ "xorq", 4, HPACK(I_ALU, A_XOR), 2 },
	{ "jmp", 3, HPACK(I_JMP, C_YES), 9 },
	{ "jle", 3, HPACK(I_JMP, C_LE), 9 },
	{ "jl", 2, HPACK(I_JMP, C_L), 9 },
	{ "je", 2, HPACK(I_JMP, C_E), 9 },
	{ "jne", 3, HPACK(I_JMP, C_NE), 9 },
	{ "jge", 3, HPACK(I_JMP, C_GE), 9 },
	{ "jg", 2, HPACK(I_JMP, C_G), 9 },
	{ "call", 4, HPACK(I_CALL, F_NONE), 9 },
	{ "ret", 3, HPACK(I_RET, F_NONE), 1 },
	{ "pushq", 5, HPACK(I_PUSHQ, F_NONE), 2 },
	{ "popq", 4, HPACK(I_POPQ, F_NONE), 2 },
	{ ".byte", 5, HPACK(I_DIRECTIVE, D_DATA), 1 },
	{ ".word", 5, HPACK(I_DIRECTIVE, D_DATA), 2 },
	{ ".long", 5, HPACK(I_DIRECTIVE, D_DATA), 4 },
	{ ".quad", 5, HPACK(I_DIRECTIVE, D_DATA), 8 },
	{ ".pos", 4, HPACK(I_DIRECTIVE, D_POS), 0 },
	{ ".align", 6, HPACK(I_DIRECTIVE, D_ALIGN), 0 },
	{ NULL, 1, 0, 0 } //end
};

instr_t* find_instr(char* name) {
	int i;
	for (i = 0; instr_set[i].name; i++)
		if (strncmp(instr_set[i].name, name, instr_set[i].len) == 0)
			return &instr_set[i];
	return NULL;
}

/* symbol table (don't forget to init and finit it) */
symbol_t* symtab = NULL;

/*
 * find_symbol: scan table to find the symbol
 * args
 *     name: the name of symbol
 *
 * return
 *     symbol_t: the 'name' symbol
 *     NULL: not exist
 */
symbol_t* find_symbol(char* name) {
	symbol_t* next = symtab->next;
	while (next) {
		if (strcmp(next->name, name) == 0) {
			return next;
		}
		next = next->next;
	}
	return NULL;
}

/*
 * add_symbol: add a new symbol to the symbol table
 * args
 *     name: the name of symbol
 *
 * return
 *     0: success
 *     -1: error, the symbol has exist
 */
int add_symbol(char* name) {
	/* check duplicate */
	if (find_symbol(name)) {
		return -1;
	}
	/* create new symbol_t (don't forget to free it)*/
	symbol_t* new_sym = (symbol_t*)malloc(sizeof(symbol_t));
	new_sym->name = name;
	/* add the new symbol_t to symbol table */
	new_sym->addr = vmaddr;
	new_sym->next = symtab->next;
	symtab->next = new_sym;
	return 0;
}

/* relocation table (don't forget to init and finit it) */
reloc_t* reltab = NULL;

/*
 * add_reloc: add a new relocation to the relocation table
 * args
 *     name: the name of symbol
 */
void add_reloc(char* name, bin_t* bin) {
	/* create new reloc_t (don't forget to free it)*/
	reloc_t* new_rel = (reloc_t*)malloc(sizeof(reloc_t));
	new_rel->name = name;
	new_rel->y64bin = bin;
	/* add the new reloc_t to relocation table */
	new_rel->next = reltab->next;
	reltab->next = new_rel;
}


/* macro for parsing y64 assembly code */
#define IS_DIGIT(s) ((*(s)>='0' && *(s)<='9') || *(s)=='-' || *(s)=='+')
#define IS_LETTER(s) ((*(s)>='a' && *(s)<='z') || (*(s)>='A' && *(s)<='Z'))
#define IS_COMMENT(s) (*(s)=='#')
#define IS_REG(s) (*(s)=='%')
#define IS_IMM(s) (*(s)=='$')
#define IS_ERROR(p) ((p) == PARSE_ERR)

#define IS_BLANK(s) (*(s)==' ' || *(s)=='\t')
#define IS_END(s) (*(s)=='\0')

#define SKIP_BLANK(s) do {  \
  while(!IS_END(s) && IS_BLANK(s))  \
    (s)++;    \
} while(0);

/* return value from different parse_xxx function */
typedef enum {
	PARSE_ERR = -1, PARSE_REG, PARSE_DIGIT, PARSE_SYMBOL,
	PARSE_MEM, PARSE_DELIM, PARSE_INSTR, PARSE_LABEL
} parse_t;

/*
 * parse_instr: parse an expected data token (e.g., 'rrmovq')
 * args
 *     ptr: point to the start of string
 *     inst: point to the inst_t within instr_set
 *
 * return
 *     PARSE_INSTR: success, move 'ptr' to the first char after token,
 *                            and store the pointer of the instruction to 'inst'
 *     PARSE_ERR: error, the value of 'ptr' and 'inst' are undefined
 */
parse_t parse_instr(char** ptr, instr_t** inst) {
	/* skip the blank */
	char* instr_ptr = *ptr;
	SKIP_BLANK(instr_ptr);
	/* find_instr and check end */
	if (!IS_LETTER(instr_ptr) && *instr_ptr != '.') {
		return PARSE_ERR;
	}
	instr_t* inst_ptr;
	if ((inst_ptr = find_instr(instr_ptr))) {
		instr_ptr += inst_ptr->len;
	} else {
		return PARSE_ERR;
	}
	/* set 'ptr' and 'inst' */
	*ptr = instr_ptr;
	*inst = inst_ptr;
	return PARSE_INSTR;
}

parse_t parse_delim_print(char** ptr, char delim, int p) {
	/* skip the blank and check */
	char* delim_ptr = *ptr;
	SKIP_BLANK(delim_ptr);
	if (IS_END(delim_ptr) || delim_ptr[0] != delim) {
		if (p) {
			err_print("Invalid '%c'", delim);
		}
		return PARSE_ERR;
	}
	/* set 'ptr' */
	*ptr = ++delim_ptr;
	return PARSE_DELIM;
}

/*
 * parse_delim: parse an expected delimiter token (e.g., ',')
 * args
 *     ptr: point to the start of string
 *
 * return
 *     PARSE_DELIM: success, move 'ptr' to the first char after token
 *     PARSE_ERR: error, the value of 'ptr' and 'delim' are undefined
 */
parse_t parse_delim(char** ptr, char delim) {
	return parse_delim_print(ptr, delim, 1);
}



/*
 * parse_reg: parse an expected register token (e.g., '%rax')
 * args
 *     ptr: point to the start of string
 *     regid: point to the regid of register
 *
 * return
 *     PARSE_REG: success, move 'ptr' to the first char after token, 
 *                         and store the regid to 'regid'
 *     PARSE_ERR: error, the value of 'ptr' and 'regid' are undefined
 */
parse_t parse_reg(char** ptr, regid_t* regid) {
	/* skip the blank and check */
	char* reg_ptr = *ptr;
	SKIP_BLANK(reg_ptr);
	/* find register */
	if (!IS_REG(reg_ptr)) {
		err_print("Invalid %s", "REG");
		return PARSE_ERR;
	}
	const reg_t* regid_ptr = find_register(reg_ptr);
	if (!regid_ptr) {
		err_print("Invalid %s", "REG");
		return PARSE_ERR;
	}
	/* set 'ptr' and 'regid' */
	reg_ptr += regid_ptr->namelen;
	*ptr = reg_ptr;
	*regid = regid_ptr->id;
	return PARSE_REG;
}

/*
 * parse_symbol: parse an expected symbol token (e.g., 'Main')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *
 * return
 *     PARSE_SYMBOL: success, move 'ptr' to the first char after token,
 *                               and allocate and store name to 'name'
 *     PARSE_ERR: error, the value of 'ptr' and 'name' are undefined
 */
parse_t parse_symbol(char** ptr, char** name) {
	/* skip the blank and check */
	char* symbol_ptr = *ptr;
	SKIP_BLANK(symbol_ptr);
	/* allocate name and copy to it */
	if (IS_DIGIT(symbol_ptr)) {
		return PARSE_ERR;
	}
	char* tmp = symbol_ptr;
	while (IS_LETTER(tmp) || IS_DIGIT(tmp) || *tmp == '_') {
		++tmp;
	}
	int64_t len = tmp - symbol_ptr;
	if (len) {
		return PARSE_ERR;
	}
	/* set 'ptr' and 'name' */
	*name = malloc(len + 1);
	strncpy(*name, symbol_ptr, len);
	*name[len] = '\0';
	*ptr = tmp;
	return PARSE_SYMBOL;
}

/*
 * parse_digit: parse an expected digit token (e.g., '0x100')
 * args
 *     ptr: point to the start of string
 *     value: point to the value of digit
 *
 * return
 *     PARSE_DIGIT: success, move 'ptr' to the first char after token
 *                            and store the value of digit to 'value'
 *     PARSE_ERR: error, the value of 'ptr' and 'value' are undefined
 */
parse_t parse_digit(char** ptr, long* value) {
	/* skip the blank and check */
	char* digit_ptr = *ptr;
	SKIP_BLANK(digit_ptr);
	/* calculate the digit, (NOTE: see strtoll()) */
	if (!IS_DIGIT(digit_ptr)) {
		return PARSE_ERR;
	}
	char* end_ptr = digit_ptr;
	uint64_t digit = strtoull(digit_ptr, &end_ptr, 0);
	if (end_ptr == digit_ptr) {
		return PARSE_ERR;
	}
	/* set 'ptr' and 'value' */
	*value = digit;
	*ptr = end_ptr;
	return PARSE_DIGIT;
}

/*
 * parse_imm: parse an expected immediate token (e.g., '$0x100' or 'STACK')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *     value: point to the value of digit
 *
 * return
 *     PARSE_DIGIT: success, the immediate token is a digit,
 *                            move 'ptr' to the first char after token,
 *                            and store the value of digit to 'value'
 *     PARSE_SYMBOL: success, the immediate token is a symbol,
 *                            move 'ptr' to the first char after token,
 *                            and allocate and store name to 'name' 
 *     PARSE_ERR: error, the value of 'ptr', 'name' and 'value' are undefined
 */
parse_t parse_imm(char** ptr, char** name, long* value) {
	/* skip the blank and check */
	char* imm_ptr = *ptr;
	SKIP_BLANK(imm_ptr);
	/* if IS_IMM, then parse the digit */
	if (IS_IMM(imm_ptr)) {
		*ptr = ++imm_ptr;
		if (IS_ERROR(parse_digit(ptr, value))) {
			err_print("Invalid %s", "Immediate");
			return PARSE_ERR;
		}
		return PARSE_DIGIT;
	}
	/* if IS_LETTER, then parse the symbol */
	*ptr = imm_ptr;
	if (IS_LETTER(imm_ptr)) {
		while (IS_LETTER(imm_ptr) || IS_DIGIT(imm_ptr) || *imm_ptr == '_') {
			++imm_ptr;
		}
		size_t len = imm_ptr - *ptr;
		char* tmp = (char*)malloc(len + 1);
		strncpy(tmp, *ptr, len);
		tmp[len] = '\0';
		*name = tmp;
		*ptr = imm_ptr;
		return PARSE_SYMBOL;
	}
	/* set 'ptr' and 'name' or 'value' */

	return PARSE_ERR;
}

/*
 * parse_mem: parse an expected memory token (e.g., '8(%rbp)')
 * args
 *     ptr: point to the start of string
 *     value: point to the value of digit
 *     regid: point to the regid of register
 *
 * return
 *     PARSE_MEM: success, move 'ptr' to the first char after token,
 *                          and store the value of digit to 'value',
 *                          and store the regid to 'regid'
 *     PARSE_ERR: error, the value of 'ptr', 'value' and 'regid' are undefined
 */
parse_t parse_mem(char** ptr, long* value, regid_t* regid) {
	/* skip the blank and check */
	SKIP_BLANK(*ptr);
	/* calculate the digit and register, (ex: (%rbp) or 8(%rbp)) */
	if (IS_DIGIT(*ptr) && IS_ERROR(parse_digit(ptr, value))) {
		err_print("Invalid %s", "MEM");
		return PARSE_ERR;
	}
	if (IS_ERROR(parse_delim_print(ptr, '(', 0))
		|| IS_ERROR(parse_reg(ptr, regid))
		|| IS_ERROR(parse_delim_print(ptr, ')', 0))) {
		err_print("Invalid %s", "MEM");
		return PARSE_ERR;
	}
	/* set 'ptr', 'value' and 'regid' */
	return PARSE_MEM;
}

/*
 * parse_data: parse an expected data token (e.g., '0x100' or 'array')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *     value: point to the value of digit
 *
 * return
 *     PARSE_DIGIT: success, data token is a digit,
 *                            and move 'ptr' to the first char after token,
 *                            and store the value of digit to 'value'
 *     PARSE_SYMBOL: success, data token is a symbol,
 *                            and move 'ptr' to the first char after token,
 *                            and allocate and store name to 'name' 
 *     PARSE_ERR: error, the value of 'ptr', 'name' and 'value' are undefined
 */
parse_t parse_data(char** ptr, char** name, long* value) {
	/* skip the blank and check */
	SKIP_BLANK(*ptr);
	/* if IS_DIGIT, then parse the digit */
	if (IS_DIGIT(*ptr) && !IS_ERROR(parse_digit(ptr, value))) {
		return PARSE_DIGIT;
	}
	/* if IS_LETTER, then parse the symbol */
	char* tmp = *ptr;
	if (IS_LETTER(tmp)) {
		while (IS_LETTER(tmp) || IS_DIGIT(tmp) || *tmp == '_') {
			++tmp;
		}
		size_t len = tmp - *ptr;
		char* name_tmp = (char*)malloc(len + 1);
		strncpy(name_tmp, *ptr, len);
		name_tmp[len] = '\0';
		*name = name_tmp;
		*ptr = tmp;
		return PARSE_SYMBOL;
	}
	/* set 'ptr', 'name' and 'value' */

	return PARSE_ERR;
}

/*
 * parse_label: parse an expected label token (e.g., 'Loop:')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *
 * return
 *     PARSE_LABEL: success, move 'ptr' to the first char after token
 *                            and allocate and store name to 'name'
 *     PARSE_ERR: error, the value of 'ptr' is undefined
 */
parse_t parse_label(char** ptr, char** name) {
	/* skip the blank and check */
	char* label_ptr = *ptr;
	SKIP_BLANK(label_ptr);
	/* allocate name and copy to it */
	if (IS_END(label_ptr)) {
		return PARSE_ERR;
	}
	char* end_ptr;
	if (!(end_ptr = strchr(label_ptr, ':'))) {
		return PARSE_ERR;
	}
	/* set 'ptr' and 'name' */
	size_t len = end_ptr - label_ptr;
	char* tmp = malloc(len + 1);
	strncpy(tmp, label_ptr, end_ptr - label_ptr);
	tmp[len] = '\0';
	*name = tmp;
	*ptr = ++end_ptr;
	return PARSE_LABEL;
}

void storeValue(byte_t* ptr, unsigned long value, int bytes) {
	for (int i = 0; i < bytes; ++i) {
		ptr[i] = (value & 0xff);
		value >>= 8;
	}
}

/*
 * parse_line: parse a line of y64 code (e.g., 'Loop: mrmovq (%rcx), %rsi')
 * (you could combine above parse_xxx functions to do it)
 * args
 *     line: point to a line_t data with a line of y64 assembly code
 *
 * return
 *     TYPE_XXX: success, fill line_t with assembled y64 code
 *     TYPE_ERR: error, try to print err information
 */
type_t parse_line(line_t* line) {

/* when finish parse an instruction or lable, we still need to continue check 
* e.g., 
*  Loop: mrmovl (%rbp), %rcx
*           call SUM  #invoke SUM function */

	/* skip blank and check IS_END */
	char* ptr = line->y64asm;
	bin_t* bin = &line->y64bin;
	SKIP_BLANK(ptr);
	if (IS_END(ptr)) {
		return line->type;
	}
	line->type = TYPE_ERR;
	/* is a comment ? */
	if (IS_COMMENT(ptr)) {
		line->type = TYPE_COMM;
		return line->type;
	}
	char* name;
	if (!IS_LETTER(ptr) && (*ptr) != '.') {
		return line->type;
	}
	/* is a label ? */
	char* tmp = ptr;
	if (!IS_ERROR(parse_label(&tmp, &name))) {
		if (add_symbol(name)) {
			err_print("Dup symbol:%s", name);
			line->type = TYPE_ERR;
			return line->type;
		}
		ptr = tmp;
		SKIP_BLANK(ptr);
		if (IS_END(ptr) || IS_COMMENT(ptr)) {
			line->type = TYPE_INS;
			bin->addr = vmaddr;
			bin->bytes = 0;
			return line->type;
		}
	}
	/* is an instruction ? */
	instr_t* instr;
	if (IS_ERROR(parse_instr(&ptr, &instr))) {
		return line->type;
	}
	switch (HIGH(instr->code)) {
	case I_NOP:
	case I_RET:
	case I_HALT:
		break;
	case I_RRMOVQ:
	case I_ALU:
		fun(
			regid_t regid_f;
			regid_t regid_s;
			if (IS_ERROR(parse_reg(&ptr, &regid_f))
				|| IS_ERROR(parse_delim(&ptr, ','))
				|| IS_ERROR(parse_reg(&ptr, &regid_s))) {
				return line->type;
			} else {
				bin->codes[1] = HPACK(regid_f, regid_s);
			}
		);
		break;
	case I_RMMOVQ:
		fun(
			regid_t regid_f;
			long value = 0;
			regid_t regid_s;
			if (IS_ERROR(parse_reg(&ptr, &regid_f))
				|| IS_ERROR(parse_delim(&ptr, ','))
				|| IS_ERROR(parse_mem(&ptr, &value, &regid_s))) {
				return line->type;
			} else {
				bin->codes[1] = HPACK(regid_f, regid_s);
				storeValue(&bin->codes[2], value, 8);
			}
		);
		break;
	case I_MRMOVQ:
		fun(
			regid_t regid_f;
			long value = 0;
			regid_t regid_s;
			if (IS_ERROR(parse_mem(&ptr, &value, &regid_s))
				|| IS_ERROR(parse_delim(&ptr, ','))
				|| IS_ERROR(parse_reg(&ptr, &regid_f))) {
				return line->type;
			} else {
				bin->codes[1] = HPACK(regid_f, regid_s);
				storeValue(&bin->codes[2], value, 8);
			}
		);
		break;
	case I_IRMOVQ:
		fun(
			long value = 0;
			regid_t regid_s;
			parse_t type;
			if (IS_ERROR(type = parse_imm(&ptr, &name, &value))
				|| IS_ERROR(parse_delim(&ptr, ','))
				|| IS_ERROR(parse_reg(&ptr, &regid_s))) {
				return line->type;
			} else {
				bin->codes[1] = HPACK(15, regid_s);
				if (type == PARSE_SYMBOL) {
					add_reloc(name, bin);
					break;
				} else {
					storeValue(&bin->codes[2], value, 8);
				}
			}
		);
		break;
	case I_JMP:
	case I_CALL:
		fun(
			parse_t type;
			long value = 0;
			if (IS_ERROR(type = parse_imm(&ptr, &name, &value))) {
				err_print("Invalid %s", "DEST");
				return line->type;
			} else {
				if (type == PARSE_SYMBOL) {
					add_reloc(name, bin);
					break;
				} else {
					storeValue(&bin->codes[1], value, 8);
				}
			}
		);
		break;
	case I_POPQ:
	case I_PUSHQ:
		fun(
			regid_t regid_f;
			if (IS_ERROR(parse_reg(&ptr, &regid_f))) {
				return line->type;
			} else {
				bin->codes[1] = HPACK(regid_f, 15);
			}
		);
		break;
	case I_DIRECTIVE:
		switch (LOW(instr->code)) {
		case D_DATA:
			fun(
				long value = 0;
				parse_t type;
				if (IS_ERROR(type = parse_data(&ptr, &name, &value))) {
					return line->type;
				}
				if (type == PARSE_SYMBOL) {
					add_reloc(name, bin);
				} else {
					storeValue(&bin->codes[0], value, instr->bytes);
				}
			);
			break;
		case D_ALIGN:
			fun(
				long value = 0;
				if (IS_ERROR(parse_digit(&ptr, &value))) {
					return line->type;
				}
				vmaddr = (((vmaddr) + (value - 1)) / value) * value;
			);
			break;
		case D_POS:
			fun(
				long value = 0;
				if (IS_ERROR(parse_digit(&ptr, &value))) {
					return line->type;
				}
				vmaddr = value;
			);
			break;
		}
		break;
	default:
		return line->type;
	}
	/* set type and y64bin */
	line->type = TYPE_INS;
	if (HIGH(instr->code) != I_DIRECTIVE) {
		bin->codes[0] = instr->code;
	}
	/* update vmaddr */
	bin->addr = vmaddr;
	bin->bytes = instr->bytes;
	vmaddr += instr->bytes;
	/* parse the rest of instruction according to the itype */
	SKIP_BLANK(ptr);
	if (!IS_COMMENT(ptr) && !IS_END(ptr)) {
		line->type = TYPE_ERR;
		return line->type;
	}
	return line->type;
}

/*
 * assemble: assemble an y64 file (e.g., 'asum.ys')
 * args
 *     in: point to input file (an y64 assembly file)
 *
 * return
 *     0: success, assmble the y64 file to a list of line_t
 *     -1: error, try to print err information (e.g., instr type and line number)
 */
int assemble(FILE* in) {
	static char asm_buf[MAX_INSLEN]; /* the current line of asm code */
	line_t* line;
	int slen;
	char* y64asm;

	/* read y64 code line-by-line, and parse them to generate raw y64 binary code list */
	while (fgets(asm_buf, MAX_INSLEN, in) != NULL) {
		slen = strlen(asm_buf);
		while ((asm_buf[slen - 1] == '\n') || (asm_buf[slen - 1] == '\r')) {
			asm_buf[--slen] = '\0'; /* replace terminator */
		}

		/* store y64 assembly code */
		y64asm = (char*)malloc(sizeof(char) * (slen + 1)); // free in finit
		strcpy(y64asm, asm_buf);

		line = (line_t*)malloc(sizeof(line_t)); // free in finit
		memset(line, '\0', sizeof(line_t));

		line->type = TYPE_COMM;
		line->y64asm = y64asm;
		line->next = NULL;

		line_tail->next = line;
		line_tail = line;
		lineno++;

		if (parse_line(line) == TYPE_ERR) {
			return -1;
		}
	}

	lineno = -1;
	return 0;
}

/*
 * relocate: relocate the raw y64 binary code with symbol address
 *
 * return
 *     0: success
 *     -1: error, try to print err information (e.g., addr and symbol)
 */
int relocate(void) {
	reloc_t* rtmp = NULL;

	rtmp = reltab->next;
	while (rtmp) {
		/* find symbol */
		symbol_t* symbol = find_symbol(rtmp->name);
		if (symbol == NULL) {
			err_print("Unknown symbol:'%s'", rtmp->name);
			return -1;
		}
		/* relocate y64bin according itype */
		switch (HIGH(rtmp->y64bin->codes[0])) {
		case I_IRMOVQ:
			storeValue(&rtmp->y64bin->codes[2], symbol->addr, 8);
			break;
		case I_JMP:
		case I_CALL:
			storeValue(&rtmp->y64bin->codes[1], symbol->addr, 8);
			break;
		case 0:
			storeValue(&rtmp->y64bin->codes[0], symbol->addr, 8);
			break;
		default:
			return -1;
		}
		/* next */
		rtmp = rtmp->next;
	}
	return 0;
}

/*
 * binfile: generate the y64 binary file
 * args
 *     out: point to output file (an y64 binary file)
 *
 * return
 *     0: success
 *     -1: error
 */
int binfile(FILE* out) {
	/* prepare image with y64 binary code */
	line_t* ltmp = line_head->next;
	/* binary write y64 code to output file (NOTE: see fwrite()) */
	while (ltmp != NULL) {
		fseek(out, ltmp->y64bin.addr, SEEK_SET);
		fwrite(ltmp->y64bin.codes, sizeof(char), ltmp->y64bin.bytes, out);
		ltmp = ltmp->next;
	}
	return 0;
}

/* whether print the readable output to screen or not ? */
bool_t screen = FALSE;

static void hexstuff(char* dest, int value, int len) {
	int i;
	for (i = 0; i < len; i++) {
		char c;
		int h = (value >> 4 * i) & 0xF;
		c = h < 10 ? h + '0' : h - 10 + 'a';
		dest[len - i - 1] = c;
	}
}

void print_line(line_t* line) {
	char buf[64];

	/* line format: 0xHHH: cccccccccccc | <line> */
	if (line->type == TYPE_INS) {
		bin_t* y64bin = &line->y64bin;
		int i;

		strcpy(buf, "  0x000:                      | ");

		hexstuff(buf + 4, y64bin->addr, 3);
		if (y64bin->bytes > 0)
			for (i = 0; i < y64bin->bytes; i++)
				hexstuff(buf + 9 + 2 * i, y64bin->codes[i] & 0xFF, 2);
	} else {
		strcpy(buf, "                              | ");
	}

	printf("%s%s\n", buf, line->y64asm);
}

/* 
 * print_screen: dump readable binary and assembly code to screen
 * (e.g., Figure 4.8 in ICS book)
 */
void print_screen(void) {
	line_t* tmp = line_head->next;
	while (tmp != NULL) {
		print_line(tmp);
		tmp = tmp->next;
	}
}

/* init and finit */
void init(void) {
	reltab = (reloc_t*)malloc(sizeof(reloc_t)); // free in finit
	memset(reltab, 0, sizeof(reloc_t));

	symtab = (symbol_t*)malloc(sizeof(symbol_t)); // free in finit
	memset(symtab, 0, sizeof(symbol_t));

	line_head = (line_t*)malloc(sizeof(line_t)); // free in finit
	memset(line_head, 0, sizeof(line_t));
	line_tail = line_head;
	lineno = 0;
}

void finit(void) {
	reloc_t* rtmp = NULL;
	do {
		rtmp = reltab->next;
		if (reltab->name)
			free(reltab->name);
		free(reltab);
		reltab = rtmp;
	} while (reltab);

	symbol_t* stmp = NULL;
	do {
		stmp = symtab->next;
		if (symtab->name)
			free(symtab->name);
		free(symtab);
		symtab = stmp;
	} while (symtab);

	line_t* ltmp = NULL;
	do {
		ltmp = line_head->next;
		if (line_head->y64asm)
			free(line_head->y64asm);
		free(line_head);
		line_head = ltmp;
	} while (line_head);
}

static void usage(char* pname) {
	printf("Usage: %s [-v] file.ys\n", pname);
	printf("   -v print the readable output to screen\n");
	exit(0);
}

int main(int argc, char* argv[]) {
	int rootlen;
	char infname[512];
	char outfname[512];
	int nextarg = 1;
	FILE* in = NULL, * out = NULL;

	if (argc < 2)
		usage(argv[0]);

	if (argv[nextarg][0] == '-') {
		char flag = argv[nextarg][1];
		switch (flag) {
		case 'v':
			screen = TRUE;
			nextarg++;
			break;
		default:
			usage(argv[0]);
		}
	}

	/* parse input  name */
	rootlen = strlen(argv[nextarg]) - 3;
	/* only support the .ys file */
	if (strcmp(argv[nextarg] + rootlen, ".ys"))
		usage(argv[0]);

	if (rootlen > 500) {
		err_print("File name too long");
		exit(1);
	}


	/* init */
	init();


	/* assemble .ys file */
	strncpy(infname, argv[nextarg], rootlen);
	strcpy(infname + rootlen, ".ys");
	in = fopen(infname, "r");
	if (!in) {
		err_print("Can't open input file '%s'", infname);
		exit(1);
	}

	if (assemble(in) < 0) {
		err_print("Assemble y64 code error");
		fclose(in);
		exit(1);
	}
	fclose(in);


	/* relocate binary code */
	if (relocate() < 0) {
		err_print("Relocate binary code error");
		exit(1);
	}


	/* generate .bin file */
	strncpy(outfname, argv[nextarg], rootlen);
	strcpy(outfname + rootlen, ".bin");
	out = fopen(outfname, "wb");
	if (!out) {
		err_print("Can't open output file '%s'", outfname);
		exit(1);
	}

	if (binfile(out) < 0) {
		err_print("Generate binary file error");
		fclose(out);
		exit(1);
	}
	fclose(out);

	/* print to screen (.yo file) */
	if (screen)
		print_screen();

	/* finit */
	finit();
	return 0;
}


