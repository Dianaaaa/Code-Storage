#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "y64asm.h"

line_t *line_head = NULL;
line_t *line_tail = NULL;
int lineno = 0;

#define err_print(_s, _a ...) do { \
  if (lineno < 0) \
    fprintf(stderr, "[--]: "_s"\n", ## _a); \
  else \
    fprintf(stderr, "[L%d]: "_s"\n", lineno, ## _a); \
} while (0);


int64_t vmaddr = 0;    /* vm addr */

/* register table */
const reg_t reg_table[REG_NONE] = {
    {"%rax", REG_RAX, 4},
    {"%rcx", REG_RCX, 4},
    {"%rdx", REG_RDX, 4},
    {"%rbx", REG_RBX, 4},
    {"%rsp", REG_RSP, 4},
    {"%rbp", REG_RBP, 4},
    {"%rsi", REG_RSI, 4},
    {"%rdi", REG_RDI, 4},
    {"%r8",  REG_R8,  3},
    {"%r9",  REG_R9,  3},
    {"%r10", REG_R10, 4},
    {"%r11", REG_R11, 4},
    {"%r12", REG_R12, 4},
    {"%r13", REG_R13, 4},
    {"%r14", REG_R14, 4}
};
const reg_t* find_register(char *name) 
{
    int i;
    for (i = 0; i < REG_NONE; i++)
        if (!strncmp(name, reg_table[i].name, reg_table[i].namelen))
            return &reg_table[i];
    return NULL;
}


/* instruction set */
instr_t instr_set[] = {
    {"nop", 3,   HPACK(I_NOP, F_NONE), 1 },
    {"halt", 4,  HPACK(I_HALT, F_NONE), 1 },
    {"rrmovq", 6,HPACK(I_RRMOVQ, F_NONE), 2 },
    {"cmovle", 6,HPACK(I_RRMOVQ, C_LE), 2 },
    {"cmovl", 5, HPACK(I_RRMOVQ, C_L), 2 },
    {"cmove", 5, HPACK(I_RRMOVQ, C_E), 2 },
    {"cmovne", 6,HPACK(I_RRMOVQ, C_NE), 2 },
    {"cmovge", 6,HPACK(I_RRMOVQ, C_GE), 2 },
    {"cmovg", 5, HPACK(I_RRMOVQ, C_G), 2 },
    {"irmovq", 6,HPACK(I_IRMOVQ, F_NONE), 10 },
    {"rmmovq", 6,HPACK(I_RMMOVQ, F_NONE), 10 },
    {"mrmovq", 6,HPACK(I_MRMOVQ, F_NONE), 10 },
    {"addq", 4,  HPACK(I_ALU, A_ADD), 2 },
    {"subq", 4,  HPACK(I_ALU, A_SUB), 2 },
    {"andq", 4,  HPACK(I_ALU, A_AND), 2 },
    {"xorq", 4,  HPACK(I_ALU, A_XOR), 2 },
    {"jmp", 3,   HPACK(I_JMP, C_YES), 9 },
    {"jle", 3,   HPACK(I_JMP, C_LE), 9 },
    {"jl", 2,    HPACK(I_JMP, C_L), 9 },
    {"je", 2,    HPACK(I_JMP, C_E), 9 },
    {"jne", 3,   HPACK(I_JMP, C_NE), 9 },
    {"jge", 3,   HPACK(I_JMP, C_GE), 9 },
    {"jg", 2,    HPACK(I_JMP, C_G), 9 },
    {"call", 4,  HPACK(I_CALL, F_NONE), 9 },
    {"ret", 3,   HPACK(I_RET, F_NONE), 1 },
    {"pushq", 5, HPACK(I_PUSHQ, F_NONE), 2 },
    {"popq", 4,  HPACK(I_POPQ, F_NONE),  2 },
    {".byte", 5, HPACK(I_DIRECTIVE, D_DATA), 1 },
    {".word", 5, HPACK(I_DIRECTIVE, D_DATA), 2 },
    {".long", 5, HPACK(I_DIRECTIVE, D_DATA), 4 },
    {".quad", 5, HPACK(I_DIRECTIVE, D_DATA), 8 },
    {".pos", 4,  HPACK(I_DIRECTIVE, D_POS), 0 },
    {".align", 6,HPACK(I_DIRECTIVE, D_ALIGN), 0 },
    {NULL, 1,    0   , 0 } //end
};

instr_t *find_instr(char *name)
{
    int i;
    for (i = 0; instr_set[i].name; i++)
	if (strncmp(instr_set[i].name, name, instr_set[i].len) == 0)
	    return &instr_set[i];
    return NULL;
}

/* symbol table (don't forget to init and finit it) */
symbol_t *symtab = NULL;

/*
 * find_symbol: scan table to find the symbol
 * args
 *     name: the name of symbol
 *
 * return
 *     symbol_t: the 'name' symbol
 *     NULL: not exist
 */
symbol_t *find_symbol(char *name)
{
    symbol_t *p = symtab->next;
    while(p)
    {
	if(!strcmp(p->name, name)) return p;
	p = p->next;
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
int add_symbol(char *name)
{
    /* check duplicate */
    if (find_symbol(name) != NULL) return -1;

    /* create new symbol_t (don't forget to free it)*/
    symbol_t *new_sb = (symbol_t *)malloc(sizeof(symbol_t));
    new_sb->name = name;
    new_sb->addr = vmaddr;

    //err_print("addr: %x", (int)(new_sb->addr));
    new_sb->next = NULL;

    /* add the new symbol_t to symbol table */
    new_sb->next = symtab->next;
    symtab->next = new_sb;

    return 0;
}

/* relocation table (don't forget to init and finit it) */
reloc_t *reltab = NULL;

/*
 * add_reloc: add a new relocation to the relocation table
 * args
 *     name: the name of symbol
 *
 * return
 *     0: success
 *     -1: error, the symbol has exist
 */
void add_reloc(char *name, bin_t *bin)
{
    /* create new reloc_t (don't forget to free it)*/
    reloc_t *new_rel = (reloc_t *)malloc(sizeof(reloc_t));
    new_rel->y64bin = bin;
    new_rel->name = name;
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

#define IS_BLANK(s) (*(s)==' ' || *(s)=='\t')
#define IS_END(s) (*(s)=='\0')

#define SKIP_BLANK(s) do {  \
  while(!IS_END(s) && IS_BLANK(s))  \
    (s)++;    \
} while(0);

/* return value from different parse_xxx function */
typedef enum { PARSE_ERR=-1, PARSE_REG, PARSE_DIGIT, PARSE_SYMBOL, 
    PARSE_MEM, PARSE_DELIM, PARSE_INSTR, PARSE_LABEL} parse_t;

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
parse_t parse_instr(char **ptr, instr_t **inst)
{
    char *p = *ptr;
    if (ptr == NULL || p == NULL) return PARSE_ERR;
    /* skip the blank */
    SKIP_BLANK(p);
    if (IS_END(p)) return PARSE_ERR;

    /* find_instr and check end */
    instr_t* ins = find_instr(p);
    if (ins == NULL) return PARSE_ERR;
    p += ins->len;
    

    /* set 'ptr' and 'inst' */
    *ptr = p;
    *inst = ins;
    return PARSE_INSTR;
    //return PARSE_ERR;
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
parse_t parse_delim(char **ptr, char delim)
{
    char *p = *ptr;
    if (ptr == NULL || p == NULL) return PARSE_ERR;
    /* skip the blank and check */
    SKIP_BLANK(p);
    if (IS_END(p)) return PARSE_ERR;
    if (*p != delim) return PARSE_ERR;
    p = p + 1;

    /* set 'ptr' */
    *ptr = p;
    return PARSE_DELIM;
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
parse_t parse_reg(char **ptr, regid_t *regid)
{
    char *p = *ptr;
    
    if (ptr == NULL || p == NULL) return PARSE_ERR;
    /* skip the blank and check */
    SKIP_BLANK(p);
    if (IS_END(p)) return PARSE_ERR;

    /* find register */
    if (IS_REG(p)) {
	const reg_t *reg = find_register(p);
	if (reg == NULL) return PARSE_ERR;
    	p += reg->namelen;

    /* set 'ptr' and 'regid' */
    	*ptr = p;
    	regid_t id = reg->id;
	*regid = id;
    	return PARSE_REG;
    } else return PARSE_ERR;

    //return PARSE_ERR;
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
parse_t parse_symbol(char **ptr, char **name)
{
    char *p = *ptr;
    if (ptr == NULL || p == NULL) return PARSE_ERR;
    /* skip the blank and check */
    SKIP_BLANK(p);
    if (IS_END(p) || !IS_LETTER(p)) return PARSE_ERR;

    /* allocate name and copy to it */
    int len = 0; 
    while (IS_LETTER(p+len) || IS_DIGIT(p+len)){
	len ++;
    }
    char *sym_name = (char *)malloc(len+1);
    for (int i = 0; i < len; i++) 
	sym_name[i] = p[i];
    sym_name[len] = '\0';
    p += len;

    /* set 'ptr' and 'name' */
    *ptr = p;
    *name = sym_name;

    return PARSE_SYMBOL;
    //return PARSE_ERR;
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
parse_t parse_digit(char **ptr, long *value)
{
    char *p = *ptr;
    if (ptr == NULL || p == NULL) return PARSE_ERR;
    /* skip the blank and check */
    SKIP_BLANK(p);
    if (IS_END(p)) return PARSE_ERR;
    if (!IS_DIGIT(p)) return PARSE_ERR;
    /* calculate the digit, (NOTE: see strtoll()) */
    long val;
    val = (long)strtoull(p, &p, 0);

    /* set 'ptr' and 'value' */
    *ptr = p;
    *value = val;
    return PARSE_DIGIT;
    //return PARSE_ERR;

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
parse_t parse_imm(char **ptr, char **name, long *value)
{
    char *p = *ptr;
    if (!ptr || !p) return PARSE_ERR;
    /* skip the blank and check */
    SKIP_BLANK(p);
    if (IS_END(p)) return PARSE_ERR;

    /* if IS_IMM, then parse the digit */
    if (IS_IMM(p)){
	p += 1;
	if(parse_digit(&p, value) == PARSE_ERR) return PARSE_ERR;
	else {
	    *ptr = p;
	    return PARSE_DIGIT;
        }    									//IN DOUBT!
    }
    /* if IS_LETTER, then parse the symbol */
    else if (IS_LETTER(p)) {
	if (parse_symbol(&p, name) == PARSE_ERR) return PARSE_ERR;
	else {
 	    *ptr = p;
	    return PARSE_SYMBOL;
	}
    }
    else return PARSE_ERR;
    /* set 'ptr' and 'name' or 'value' */

    //return PARSE_ERR;
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
parse_t parse_mem(char **ptr, long *value, regid_t *regid)
{
    long val = 0;
    char *p = *ptr;
    if (!ptr || !p) return PARSE_ERR;
    /* skip the blank and check */
    SKIP_BLANK(p);
    if (IS_END(p)) return PARSE_ERR;

    /* calculate the digit and register, (ex: (%rbp) or 8(%rbp)) */
    
    if (*p == '('){
	p += 1;
	if (parse_reg(&p, regid) == PARSE_ERR) return PARSE_ERR;
	
    } else if (IS_DIGIT(p)){
	if (parse_digit(&p, &val) == PARSE_ERR) return PARSE_ERR;
        if (*p == '('){
	    p += 1;
	    if (parse_reg(&p, regid) == PARSE_ERR) return PARSE_ERR;
	} else return PARSE_ERR;
    } else {
	 return PARSE_ERR;
    }    

    /* set 'ptr', 'value' and 'regid' */
    *value = val;
    if (*p != ')') return PARSE_ERR;
    *ptr = p + 1;
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
parse_t parse_data(char **ptr, char **name, long *value)
{
    char *p = *ptr;
    if (ptr == NULL || !p) return PARSE_ERR;
    /* skip the blank and check */
    SKIP_BLANK(p);
    if (IS_END(p)) return PARSE_ERR;

    /* if IS_DIGIT, then parse the digit */
    if (IS_DIGIT(p)){
	if (parse_digit(&p, value) == PARSE_ERR) return PARSE_ERR;   
	*ptr = p; 
	return PARSE_DIGIT;								
    }

    /* if IS_LETTER, then parse the symbol */
    else if (IS_LETTER(p)) {
	if (parse_symbol(&p, name) == PARSE_ERR) return PARSE_ERR;
	*ptr = p;
	return PARSE_SYMBOL;
    }
    else return PARSE_ERR;

    /* set 'ptr', 'name' and 'value' */
    
    //return ret;
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
parse_t parse_label(char **ptr, char **name)
{
    char *p = *ptr;
    if (ptr == NULL || p == NULL) return PARSE_ERR;
    /* skip the blank and check */
    SKIP_BLANK(p);
    if (IS_END(p)) return PARSE_ERR;

    /* allocate name and copy to it */
    int count = 0;
    while (IS_LETTER(&p[count]) || IS_DIGIT(&p[count])){
	count += 1;
    }
    if (p[count] != ':') return PARSE_ERR;
    char *lab_name = (char *)malloc(count+1);
    for (int i = 0; i < count; i++) {
	lab_name[i] = p[i];
    }
    lab_name[count] = '\0';
    p += count + 1;

    /* set 'ptr' and 'name' */
    *ptr = p;
    *name = lab_name;
    return PARSE_LABEL;
    //return PARSE_ERR;
}

/*
 * parse_line: parse a line of y64 code (e.g., 'Loop: mrmovq (%rcx), %rsi')
 * (you could combine above parse_xxx functions to do it)
 * args
 *     line: point to a line_t data with a line of y64 assembly code
 *
 * return
 *     PARSE_XXX: success, fill line_t with assembled y64 code
 *     PARSE_ERR: error, try to print err information (e.g., instr type and line number)
 */
type_t parse_line(line_t *line)
{
    //fprint("parse line\n");
    //bin_t *bin = line->y64bin;
    char *p;
    //char *yasm;

    //yasm = (char *)malloc(sizeof(char) * (strlen(line->y64asm) + 1));
    //strcpy(yasm, line->y64asm);
    p = line->y64asm;
    bool_t parse_flag = 0;
/* when finish parse an instruction or lable, we still need to continue check 
* e.g., 
*  Loop: mrmovl (%rbp), %rcx
*           call SUM  #invoke SUM function */
    while (1) {
	parse_flag ++;
	//err_print("We stuck here! num0");
    	/* skip blank and check IS_END */
    	SKIP_BLANK(p);
	//err_print("%c",*p);
    	if (IS_END(p)) {
	    //err_print("%x",(int)vmaddr);
	    //free(yasm);
	    return line->type;
        }
    	/* is a comment ? */
	//err_print("We stuck here! num21");
    	if (IS_COMMENT(p)) {
	    if (parse_flag > 1) return line->type;
	    //err_print("We stuck here! num1");
	    line->type = TYPE_COMM;
	    //free(yasm);
	    return TYPE_COMM;
    	}
    	/* is a label ? */
	//err_print("We stuck here! num22");
    	char *label = NULL;
	//err_print("we get here?");
    	if (parse_label(&p, &label) != PARSE_ERR){
		/* add labels to symbol tabel. */
	    if (add_symbol(label) == -1){
	    	line->type = TYPE_ERR;
		//free(yasm);
		err_print("Dup symbol:%s", label);
	    	return TYPE_ERR;
	    }
	    line->type =  TYPE_INS;
	    line->y64bin.addr = vmaddr;  
	    //parse_flag = 0;
	    continue;
        } 

        /* is an instruction ? */
	//err_print("We stuck here! num2");
	instr_t *ins = NULL;
	if (parse_instr(&p, &ins) == PARSE_ERR){
	    line->type = TYPE_ERR;
	    
	    //free(yasm);
	    return TYPE_ERR;
	    
	}
	

        /* set type and y64bin */
	line->type = TYPE_INS;
	line->y64bin.addr = vmaddr;
	line->y64bin.codes[0] = ins->code;
	line->y64bin.bytes = ins->bytes;
	

        /* update vmaddr */
	vmaddr += ins->bytes;    

        /* parse the rest of instruction according to the itype */
	switch (HIGH(ins->code)) {
	    case I_HALT:
	    case I_NOP:
	    case I_RET: {
		break;
	    }	
	    case I_PUSHQ:
	    case I_POPQ: {
		regid_t reg;
		if (parse_reg(&p, &reg) == PARSE_ERR) {
		    line->type = TYPE_ERR;
		    //free(yasm);
		    err_print("Invalid REG");
		    return TYPE_ERR;
		}
		line->y64bin.codes[1] = HPACK(reg, 0xF);
		break;
	    }
	    case I_RRMOVQ:
	    case I_ALU: {
		regid_t regA;
		regid_t regB;
		if (parse_reg(&p, &regA) == PARSE_ERR) {
		    line->type = TYPE_ERR;
		    //free(yasm);
		    err_print("Invalid REG");
		    return TYPE_ERR;
		}
		if (parse_delim(&p, ',') == PARSE_ERR) {
		    line->type = TYPE_ERR;
		    //free(yasm);
		    err_print("Invalid ','");
		    return TYPE_ERR;
		}
		if (parse_reg(&p, &regB) == PARSE_ERR) {
		    line->type = TYPE_ERR;
		    //free(yasm);
		    err_print("Invalid REG");
		    return TYPE_ERR;
		}
		line->y64bin.codes[1] = HPACK(regA, regB);
		break;
	    }
	    case I_IRMOVQ: {	
		char *name;
		long value;
		parse_t ret = parse_imm(&p, &name, &value);
		if (ret == PARSE_ERR) {
		    line->type = TYPE_ERR;
		    //free(yasm);
		    err_print("Invalid Immediate");
		    return TYPE_ERR;
		}
		if (ret == PARSE_SYMBOL) {
		    add_reloc(name, &(line->y64bin));                         // What is reloc for?	
		    //value = 0;		
		}
		  
	
		if (parse_delim(&p, ',') == PARSE_ERR) {
		    line->type = TYPE_ERR;
		    //free(yasm);
		    err_print("Invalid ','");
		    return TYPE_ERR;
		}
		regid_t regB;
		if (parse_reg(&p, &regB) == PARSE_ERR) {
		    line->type = TYPE_ERR;
		    //free(yasm);
		    err_print("Invalid REG");
		    return TYPE_ERR;
		}
		line->y64bin.codes[1] = HPACK(0xF, regB);
		for (int i = 0; i < 8; i++) {
		    line->y64bin.codes[2+i] = (value >> (i * 8)) & 0xFF;
	        } 
		break;
		
	    }
	    case I_RMMOVQ: {
		regid_t regA;
		regid_t regB;
		long value;
		if (parse_reg(&p, &regA) == PARSE_ERR) {
		    line->type = TYPE_ERR;
		    //free(yasm);
		    err_print("Invalid REG");
		    return TYPE_ERR;
		}
		if (parse_delim(&p, ',') == PARSE_ERR) {
		    line->type = TYPE_ERR;
		    //free(yasm);
		    err_print("Invalid ','");
		    return TYPE_ERR;
		}
		if (parse_mem(&p, &value, &regB) == PARSE_ERR) {
		    line->type = TYPE_ERR;
		    //free(yasm);
		    err_print("Invalid MEM");
		    return TYPE_ERR;
		}
		//err_print("back to rmmovq.");
		line->y64bin.codes[1] = HPACK(regA, regB);
		long *val = (long *)&(line->y64bin.codes[2]);
		*val = value;
		break;
	    }
	    case I_MRMOVQ: {
		regid_t regA;
		regid_t regB;
		long value;
		if (parse_mem(&p, &value, &regB) == PARSE_ERR) {
		    line->type = TYPE_ERR;
		    //free(yasm);
		    err_print("Invalid MEM");
		    return TYPE_ERR;
		}
		if (parse_delim(&p, ',') == PARSE_ERR) {
		    line->type = TYPE_ERR;
		    //free(yasm);
		    err_print("Invalid ','");
		    return TYPE_ERR;
		}
		if (parse_reg(&p, &regA) == PARSE_ERR) {
		    line->type = TYPE_ERR;
		    //free(yasm);
		    err_print("Invalid REG");
		    return TYPE_ERR;
		}
		
		line->y64bin.codes[1] = HPACK(regA, regB);
		long *val = (long *)&(line->y64bin.codes[2]);
		*val = value;
		break;
	    }
	    case I_JMP: 
	    case I_CALL: {
		char *name = NULL;
		//err_print("here");
		parse_t ret = parse_symbol(&p, &name);
		if (ret == PARSE_ERR) {
		    //err_print("get here?");
		    line->type = TYPE_ERR;
		    //free(yasm);
		    err_print("Invalid DEST");
		    return TYPE_ERR;
		}
		add_reloc(name, &(line->y64bin));                         // What is reloc for?	
		break;
	    }
	    case I_DIRECTIVE: {
		switch (LOW(ins->code)) {
		    case D_DATA: {
			long value = 0;
			char *name;
			parse_t ret = parse_data(&p, &name, &value);
			if (ret == PARSE_ERR) {
			    //err_print("parse err?");
			    line->type = TYPE_ERR;
			    return TYPE_ERR;
			}
			if (ret == PARSE_SYMBOL) {
			    add_reloc(name, &(line->y64bin));           // What is reloc for?
			}
			if (ret == PARSE_DIGIT) {
			    
			    for (int i = 0; i < ins->bytes; i++) {
				
				line->y64bin.codes[i] = ((unsigned long)value >> (i * 8)) & 0xFF;
			   }
			}
			//long *val = (long *)&(line->y64bin.codes);
			//*val = value;
			break;
		    }
		    case D_POS: {
			long value;
			if (parse_digit(&p, &value) == PARSE_ERR) {
			    line->type = TYPE_ERR;
			    //free(yasm);
			    return TYPE_ERR;
			}
			
			vmaddr = value;
			
			line->y64bin.addr = vmaddr;
			break;
		    }
		    case D_ALIGN: {
 			long value;
			if (parse_digit(&p, &value) == PARSE_ERR) {
			    line->type = TYPE_ERR;
			    //free(yasm);
			    return TYPE_ERR;
			}
			while (vmaddr % value) vmaddr++;
			line->y64bin.addr = vmaddr;
			break;
		    }
		    default: 
			line->type = TYPE_ERR;
			//free(yasm);
			return TYPE_ERR;
		}
		break;
	    }
	    default:
		line->type = TYPE_ERR;
		//free(yasm);
		return TYPE_ERR;
	
	}
    }
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
int assemble(FILE *in)
{
    static char asm_buf[MAX_INSLEN]; /* the current line of asm code */
    line_t *line;
    int slen;
    char *y64asm;

    /* read y64 code line-by-line, and parse them to generate raw y64 binary code list */
    while (fgets(asm_buf, MAX_INSLEN, in) != NULL) {
	//err_print("get a line.");
        slen  = strlen(asm_buf);
        while ((asm_buf[slen-1] == '\n') || (asm_buf[slen-1] == '\r')) { 
            asm_buf[--slen] = '\0'; /* replace terminator */
        }

        /* store y64 assembly code */
        y64asm = (char *)malloc(sizeof(char) * (slen + 1)); // free in finit
        strcpy(y64asm, asm_buf);

        line = (line_t *)malloc(sizeof(line_t)); // free in finit
        memset(line, '\0', sizeof(line_t));

        line->type = TYPE_COMM;
        line->y64asm = y64asm;
        line->next = NULL;

        line_tail->next = line;
        line_tail = line;
        lineno ++;

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
int relocate(void)
{
    reloc_t *rtmp = NULL;
    
    rtmp = reltab->next;
    while (rtmp) {
        /* find symbol */
	symbol_t *sym = find_symbol(rtmp->name);
	if (!sym) {
	    err_print("Unknown symbol:'%s'", rtmp->name);
	    return -1;
	}
	//err_print("reloc name:'%s'", rtmp->name);
        /* relocate y64bin according itype */
	byte_t itype = HIGH(rtmp->y64bin->codes[0]);
	//err_print("%d",itype);
	long value = sym->addr;
	//err_print("%x",(int)value);
	if (itype == I_IRMOVQ) {
	    //err_print("IRMOVQ");
	    for (int i = 0; i < 8; i++) {
		rtmp->y64bin->codes[2+i] = (value >> (i * 8)) & 0xFF;
		
	    }
	    //err_print("%d",rtmp->y64bin->codes);
	} else if (itype == I_CALL || itype == I_JMP){
	    //err_print("Icall,jmp");
	    for (int i = 0; i < 8; i++) {
		rtmp->y64bin->codes[1+i] = (value >> (i * 8)) & 0xFF;
	    }
	} else {
	    //err_print("direc");
	    for (int i = 0; i < 8; i++) {
		rtmp->y64bin->codes[i] = (value >> (i * 8)) & 0xFF;
	    }
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
int binfile(FILE *out)
{
    /* prepare image with y64 binary code */
    line_t *pline = line_head->next;

    /* binary write y64 code to output file (NOTE: see fwrite()) */

    while (pline){
	if (pline->type == TYPE_INS) {
	    //err_print("write halt.");
	    fseek(out, pline->y64bin.addr, SEEK_SET);
	    fwrite(pline->y64bin.codes, sizeof(byte_t), pline->y64bin.bytes, out);
	}
	pline = pline->next;
    }
    
    
    
    return 0;
}


/* whether print the readable output to screen or not ? */
bool_t screen = FALSE; 

static void hexstuff(char *dest, int value, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        char c;
        int h = (value >> 4*i) & 0xF;
        c = h < 10 ? h + '0' : h - 10 + 'a';
        dest[len-i-1] = c;
    }
}

void print_line(line_t *line)
{
    char buf[32];

    /* line format: 0xHHH: cccccccccccc | <line> */
    if (line->type == TYPE_INS) {
        bin_t *y64bin = &line->y64bin;
        int i;
        
        strcpy(buf, "  0x000:                      | ");
        
        hexstuff(buf+4, y64bin->addr, 3);
        if (y64bin->bytes > 0)
            for (i = 0; i < y64bin->bytes; i++)
                hexstuff(buf+9+2*i, y64bin->codes[i]&0xFF, 2);
    } else {
        strcpy(buf, "                              | ");
    }

    printf("%s%s\n", buf, line->y64asm);
}

/* 
 * print_screen: dump readable binary and assembly code to screen
 * (e.g., Figure 4.8 in ICS book)
 */
void print_screen(void)
{
    line_t *tmp = line_head->next;
    while (tmp != NULL) {
        print_line(tmp);
        tmp = tmp->next;
    }
}

/* init and finit */
void init(void)
{
    reltab = (reloc_t *)malloc(sizeof(reloc_t)); // free in finit
    memset(reltab, 0, sizeof(reloc_t));

    symtab = (symbol_t *)malloc(sizeof(symbol_t)); // free in finit
    memset(symtab, 0, sizeof(symbol_t));

    line_head = (line_t *)malloc(sizeof(line_t)); // free in finit
    memset(line_head, 0, sizeof(line_t));
    line_tail = line_head;
    lineno = 0;
}

void finit(void)
{
    //err_print("we get error here?");
    reloc_t *rtmp = NULL;
    while (reltab) {
        rtmp = reltab->next;
        if (reltab->name) 
            free(reltab->name);
        free(reltab);
        reltab = rtmp;
    }
    
    symbol_t *stmp = NULL;
     while (symtab){
        stmp = symtab->next;
        if (symtab->name) 
            free(symtab->name);
        free(symtab);
        symtab = stmp;
    }

    line_t *ltmp = NULL;
    while (line_head) {
        ltmp = line_head->next;
        if (line_head->y64asm) 
            free(line_head->y64asm);
        free(line_head);
        line_head = ltmp;
    } 
}

static void usage(char *pname)
{
    printf("Usage: %s [-v] file.ys\n", pname);
    printf("   -v print the readable output to screen\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    int rootlen;
    char infname[512];
    char outfname[512];
    int nextarg = 1;
    FILE *in = NULL, *out = NULL;
    
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

    /* parse input file name */
    rootlen = strlen(argv[nextarg])-3;
    /* only support the .ys file */
    if (strcmp(argv[nextarg]+rootlen, ".ys"))
        usage(argv[0]);
    
    if (rootlen > 500) {
        err_print("File name too long");
        exit(1);
    }
 

    /* init */
    init();

    
    /* assemble .ys file */
    strncpy(infname, argv[nextarg], rootlen);
    strcpy(infname+rootlen, ".ys");
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
    //err_print("get into relocate");
    if (relocate() < 0) {
        err_print("Relocate binary code error");
        exit(1);
    }


    /* generate .bin file */
    strncpy(outfname, argv[nextarg], rootlen);
    strcpy(outfname+rootlen, ".bin");
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


