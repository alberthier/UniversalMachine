#include "UniversalMachine.h"
#include <stdlib.h>
#include <stdio.h>
#include "ArrayList.h"

#define READ_BUFFER_SIZE 4096

#define OPCODE(a) ((a) & 0xF0000000)
#define OPCODE_INT(a) ((a) >> 28) & (0x0000000F))

#define CONDITIONAL_MOVE 0x00000000
#define ARRAY_INDEX      0x10000000
#define ARRAY_AMENDMENT  0x20000000
#define ADDITION         0x30000000
#define MULTIPLICATION   0x40000000
#define DIVISION         0x50000000
#define NOT_AND          0x60000000
#define HALT             0x70000000
#define ALLOCATION       0x80000000
#define ABANDONMENT      0x90000000
#define OUTPUT           0xA0000000
#define INPUT            0xB0000000
#define LOAD_PROGRAM     0xC0000000
#define SET_REGISTER     0xD0000000
#define NUM_OPCODES      14

#define REG_A(regs, opcode) ((regs)[0x00000007 & ((opcode) >> 6)])
#define REG_B(regs, opcode) ((regs)[0x00000007 & ((opcode) >> 3)])
#define REG_C(regs, opcode) ((regs)[0x00000007 & (opcode)])
#define REG_SET(regs, opcode) ((regs)[0x00000007 & ((opcode) >> 25)] = (0x01FFFFFF & (opcode)))

#define SWAP_ENDIAN(a) \
	(((a) >> 24) & 0x000000FF | \
	 ((a) >> 8 ) & 0x0000FF00 | \
	 ((a) << 8 ) & 0x00FF0000 | \
	 ((a) << 24) & 0xFF000000)

typedef void (*OpcodeHandler)(UniversalMachine* self, Platter p);

/* Private class methods */

void UniversalMachine_swapArrayEndian(Array* a)
{
	Platter* tmpIt = a->data;
	Platter *tmpItEnd = a->data + a->size;
	for (; tmpIt < tmpItEnd; ++tmpIt) {
		*tmpIt = SWAP_ENDIAN(*tmpIt);
	}
}

void UniversalMachine_loadFile(UniversalMachine* self, char* filename)
{
	printf("Loading %s ... ", filename);
	FILE* f = fopen(filename, "rb");

	if (!f) {
		printf("Unable to open file\n");
		exit(3);
	}

	int platterRead = 0;
	int platterCount = 0;
	int lastPlatterRead = 0;
	unsigned int chunks = 0;

	Array* a = NULL;
	ArrayList* aList = NULL;
	ArrayList* aList2 = NULL;
	do {
		a = Array_new(READ_BUFFER_SIZE);
		platterRead = fread(a->data, sizeof(Platter), a->size, f);
		if (platterRead == 0) {
			Array_delete(a);
			if (aList == NULL) {
				printf("File empty.\n");
				exit(5);
			}
			break;
		}
		aList2 = ArrayList_insert(aList2, a);
		if (aList == NULL) {
			aList = aList2;
		}
		platterCount += platterRead;

		// Conversion en little endian pour execution sur x86.
		UniversalMachine_swapArrayEndian(a);

		++chunks;
		lastPlatterRead = platterRead;
	} while (platterRead != 0);
	fclose(f);

	self->program = Array_new(platterCount);
	self->finger = self->program->data;

	Platter* tmpIt = self->program->data;
	unsigned int chunk = 0;

	for (aList2 = aList; aList2 != NULL; aList2 = aList2->next) {
		Platter* srcIt = aList2->array->data;

		Platter* srcItEnd = NULL;
		if (chunk == chunks - 1) {
			srcItEnd = aList2->array->data + lastPlatterRead;
		} else {
			srcItEnd = aList2->array->data + aList2->array->size;
		}
		for (; srcIt != srcItEnd; ++srcIt) {
			*tmpIt = *srcIt;
			++tmpIt;
		}
		++chunk;
	}
	ArrayList_delete(aList);
	printf("OK\n");
}

void UniversalMachine_conditionalMove(UniversalMachine* self, Platter p)
{
	if (REG_C(self->regs, p) != 0) {
		REG_A(self->regs, p) = REG_B(self->regs, p);
	}
}

void UniversalMachine_arrayIndex(UniversalMachine* self, Platter p)
{
	Platter offset = REG_C(self->regs, p);
	Array* array = (Array*) REG_B(self->regs, p);
	if (array != 0) {
		REG_A(self->regs, p) = array->data[offset];
	} else {
		REG_A(self->regs, p) = self->program->data[offset];
	}
}

void UniversalMachine_arrayAmendment(UniversalMachine* self, Platter p)
{
	Array* array = (Array*) REG_A(self->regs, p);
	if (array != 0) {
		array->data[REG_B(self->regs, p)] = REG_C(self->regs, p);
	} else {
		self->program->data[REG_B(self->regs, p)] = REG_C(self->regs, p);
	}
}

void UniversalMachine_addition(UniversalMachine* self, Platter p)
{
	REG_A(self->regs, p) = REG_B(self->regs, p) + REG_C(self->regs, p);
}

void UniversalMachine_multiplication(UniversalMachine* self, Platter p)
{
	REG_A(self->regs, p) = REG_B(self->regs, p) * REG_C(self->regs, p);
}

void UniversalMachine_division(UniversalMachine* self, Platter p)
{
	Platter c = REG_C(self->regs, p);
	if (c == 0) {
		printf("Division by 0\n");
		exit(1);
	}
	REG_A(self->regs, p) = REG_B(self->regs, p) / c;
}

void UniversalMachine_notAnd(UniversalMachine* self, Platter p)
{
	REG_A(self->regs, p) = ~REG_B(self->regs, p) | ~REG_C(self->regs, p);
}

void UniversalMachine_quit(UniversalMachine* self, Platter p)
{
    exit(0);
}

void UniversalMachine_allocation(UniversalMachine* self, Platter p)
{
	Array *a = Array_new(REG_C(self->regs, p));
	REG_B(self->regs, p) = (Platter) a;
}

void UniversalMachine_abandonment(UniversalMachine* self, Platter p)
{
	Array_delete((Array*) REG_C(self->regs, p));
}

void UniversalMachine_output(UniversalMachine* self, Platter p)
{
	Platter c = REG_C(self->regs, p);
	if (c > 255) {
		printf("Console can only print values in [0..255]\n");
		exit(2);
	}
	fputc((char) c, stdout);
}

void UniversalMachine_input(UniversalMachine* self, Platter p)
{
	char c;
	Platter n;
	c = fgetc(stdin);
	n = 0x000000FF & (Platter) c;
	if (c == EOF) {
		n = 0xFFFFFFFF;
	}
	REG_C(self->regs, p) = n;
}

void UniversalMachine_loadProgram(UniversalMachine* self, Platter p)
{
	Array* array = (Array*) REG_B(self->regs, p);
	if (array != 0) {
		Array_delete(self->program);
		self->program = Array_clone(array);
	}
	self->finger = self->program->data + REG_C(self->regs, p);
}

void UniversalMachine_setRegister(UniversalMachine* self, Platter p)
{
	REG_SET(self->regs, p);
}

void UniversalMachine_spin(UniversalMachine* self)
{
	Platter p;
    OpcodeHandler handlers[NUM_OPCODES] = {
            UniversalMachine_conditionalMove,
            UniversalMachine_arrayIndex,
            UniversalMachine_arrayAmendment,
            UniversalMachine_addition,
            UniversalMachine_multiplication,
            UniversalMachine_division,
            UniversalMachine_notAnd,
            UniversalMachine_quit,
            UniversalMachine_allocation,
            UniversalMachine_abandonment,
            UniversalMachine_output,
            UniversalMachine_input,
            UniversalMachine_loadProgram,
            UniversalMachine_setRegister
    };
    for (;;) {
        p = *(self->finger);
        ++(self->finger);
        handlers[OPCODE_INT(p)](self, p);
    }
    /*
	for(;;) {
		p = *(self->finger);
		++(self->finger);
		switch (OPCODE(p)) {
		case CONDITIONAL_MOVE:
			UniversalMachine_conditionalMove(self, p);
			break;
		case ARRAY_INDEX:
			UniversalMachine_arrayIndex(self, p);
			break;
		case ARRAY_AMENDMENT:
			UniversalMachine_arrayAmendment(self, p);
			break;
		case ADDITION:
			UniversalMachine_addition(self, p);
			break;
		case MULTIPLICATION:
			UniversalMachine_multiplication(self, p);
			break;
		case DIVISION:
			UniversalMachine_division(self, p);
			break;
		case NOT_AND:
			UniversalMachine_notAnd(self, p);
			break;
		case HALT:
			goto halt;
			break;
		case ALLOCATION:
			UniversalMachine_allocation(self, p);
			break;
		case ABANDONMENT:
			UniversalMachine_abandonment(self, p);
			break;
		case OUTPUT:
			UniversalMachine_output(self, p);
			break;
		case INPUT:
			UniversalMachine_input(self, p);
			break;
		case LOAD_PROGRAM:
			UniversalMachine_loadProgram(self, p);
			break;
		case SET_REGISTER:
			UniversalMachine_setRegister(self, p);
			break;
		default:
			goto unknownOpCode;
			break;
		}
	}
halt:
	return;
unknownOpCode:
	printf("Unknown platter opcode : 0x%x\n", p);
	return;*/
}

/* Public methods implementation */

UniversalMachine* UniversalMachine_new()
{
	UniversalMachine* self = malloc(sizeof(UniversalMachine));
	int i = 0;
	self->program = NULL;
	self->finger = NULL;
	for (i = 0; i < MAX_REGS; ++i) {
		self->regs[i] = 0;
	}
	return self;
}

void UniversalMachine_delete(UniversalMachine* self)
{
	free(self->program);
	free(self);
}

void UniversalMachine_run(UniversalMachine* self, char* filename)
{
	UniversalMachine_loadFile(self, filename);
	UniversalMachine_spin(self);
}
