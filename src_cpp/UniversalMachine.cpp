#include <algorithm>
#include <fstream>
#include <sstream>
#include <list>
#include <iostream>
#include "UniversalMachine.h"
#include "Exceptions.h"

#define READ_BUFFER_SIZE 4096

#define OPCODE(a) ((a) & 0xF0000000)

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

#define REG_A(regs, opcode) ((regs)[0x00000007 & ((opcode) >> 6)])
#define REG_B(regs, opcode) ((regs)[0x00000007 & ((opcode) >> 3)])
#define REG_C(regs, opcode) ((regs)[0x00000007 & (opcode)])
#define REG_SET(regs, opcode) ((regs)[0x00000007 & ((opcode) >> 25)] = (0x01FFFFFF & (opcode)))

#define SWAP_ENDIAN(a) \
	(((a) >> 24) & 0x000000FF | \
	 ((a) >> 8 ) & 0x0000FF00 | \
	 ((a) << 8 ) & 0x00FF0000 | \
	 ((a) << 24) & 0xFF000000)

using namespace std;

UniversalMachine::UniversalMachine()
{
	m_program = NULL;
	m_finger = NULL;
	fill(m_regs, m_regs + MAX_REGS, 0);
}

UniversalMachine::~UniversalMachine()
{
	delete m_program;
}
	
void UniversalMachine::run(std::string filename)
{
	loadFile(filename);
	spin();
}

void UniversalMachine::loadFile(std::string& filename)
{
	fstream src(filename.c_str(), ios::in | ios::binary);
	if (!src.is_open()) {
		throw BaseException(string("File ") + filename + " doesn't exist");
	}
	list<Platter*> content;
	Platter *tmp;
	int bytesRead = 0;
	int bytesCount = 0;
	while (!src.eof()) {
		Platter* tmp = new Platter[READ_BUFFER_SIZE];
		src.read((char*) tmp, READ_BUFFER_SIZE * sizeof(Platter));
		content.push_back(tmp);
		bytesRead = src.gcount();
		bytesCount += bytesRead;

		// Conversion en little endian pour execution sur x86.
		Platter* tmpIt = tmp;
		Platter *tmpItEnd = tmp + (bytesRead / sizeof(Platter));
		for (; tmpIt < tmpItEnd; ++tmpIt) {
			*tmpIt = SWAP_ENDIAN(*tmpIt);
		}
	}
	src.close();

	m_program = new Array(bytesCount / sizeof(Platter));
	m_finger = m_program->data;

	Platter* tmpIt = m_program->data;
	unsigned int bytesToCopy = READ_BUFFER_SIZE * sizeof(Platter);
	unsigned int chunkNb = 0;
	list<Platter*>::iterator it = content.begin();
	list<Platter*>::iterator itEnd = content.end();
	for (; it != itEnd; ++it) {
		if (chunkNb == content.size() - 1) {
			bytesToCopy = bytesRead;
		}
		copy(*it, *it + bytesToCopy / sizeof(Platter), tmpIt);
		tmpIt += bytesToCopy / sizeof(Platter);
		delete[] *it;
		++chunkNb;
	}

	fstream f2("out.dump", ios::out | ios::binary);
	cout << m_program->size << endl;
	f2.write((char*) m_program->data, READ_BUFFER_SIZE * sizeof(Platter));
	f2.close();
}

void UniversalMachine::spin()
{
	stringstream error;
	Platter p;
	bool halt = false;
	int cpt = 0;
	try {
		for(;;) {
			p = *m_finger;
			++m_finger;
			switch (OPCODE(p)) {
			case CONDITIONAL_MOVE:
				conditionalMove(p);
				break;
			case ARRAY_INDEX:
				arrayIndex(p);
				break;
			case ARRAY_AMENDMENT:
				arrayAmendment(p);
				break;
			case ADDITION:
				addition(p);
				break;
			case MULTIPLICATION:
				multiplication(p);
				break;
			case DIVISION:
				division(p);
				break;
			case NOT_AND:
				notAnd(p);
				break;
			case HALT:
				throw HaltUniversalMachine();
				break;
			case ALLOCATION:
				allocation(p);
				break;
			case ABANDONMENT:
				abandonment(p);
				break;
			case OUTPUT:
				output(p);
				break;
			case INPUT:
				input(p);
				break;
			case LOAD_PROGRAM:
				loadProgram(p);
				break;
			case SET_REGISTER:
				setRegister(p);
				break;
			default:
				error << "Unknown platter opcode : 0x" << hex << p;
				throw BaseException(error.str());
				break;
			}
			if (m_finger >= (m_program->data + m_program->size)) {
				throw BaseException("PC points outside of the program.");
			}
		}
	} catch (HaltUniversalMachine&) {
	} catch (exception &e) {
		stringstream trace;
		trace << e.what() << endl << endl;
		for (int i = 0; i < MAX_REGS / 2; ++i) {
			trace << "regs[" << i << "] = 0x" << hex << m_regs[i];
			trace << "\t\tregs[" << (MAX_REGS / 2 + i) << "] = 0x" << hex << m_regs[MAX_REGS / 2 + i];
			trace << endl;
		}
		if ((MAX_REGS % 2) == 1) {
			trace << "regs[" << MAX_REGS - 1 << "] = 0x" << hex << m_regs[MAX_REGS - 1];
			trace << endl;
		}
		trace << "Platter = 0x" << hex << p << "\t\t PC = 0x" << hex << m_finger - m_program->data << endl;
		throw BaseException(trace.str());
	}
}

void UniversalMachine::conditionalMove(Platter p)
{
	if (REG_C(m_regs, p) != 0) {
		REG_A(m_regs, p) = REG_B(m_regs, p);
	}
}

void UniversalMachine::arrayIndex(Platter p)
{
	Platter offset = REG_C(m_regs, p);
	Array* array = (Array*) REG_B(m_regs, p);
	if (array != 0) {
		REG_A(m_regs, p) = array->data[offset];
	} else {
		REG_A(m_regs, p) = m_program->data[offset];
	}
}

void UniversalMachine::arrayAmendment(Platter p)
{
	Array* array = (Array*) REG_A(m_regs, p);
	if (array != 0) {
		array->data[REG_B(m_regs, p)] = REG_C(m_regs, p);
	} else {
		m_program->data[REG_B(m_regs, p)] = REG_C(m_regs, p);
	}
}

void UniversalMachine::addition(Platter p)
{
	REG_A(m_regs, p) = REG_B(m_regs, p) + REG_C(m_regs, p);
}

void UniversalMachine::multiplication(Platter p)
{
	REG_A(m_regs, p) = REG_B(m_regs, p) * REG_C(m_regs, p);
}

void UniversalMachine::division(Platter p)
{
	Platter c = REG_C(m_regs, p);
	if (c == 0) {
		throw BaseException("Division by 0");
	}
	REG_A(m_regs, p) = REG_B(m_regs, p) / c;
}

void UniversalMachine::notAnd(Platter p)
{
	REG_A(m_regs, p) = ~REG_B(m_regs, p) | ~REG_C(m_regs, p);
}

void UniversalMachine::allocation(Platter p)
{
	Array *a = new Array(REG_C(m_regs, p));
	REG_B(m_regs, p) = (Platter) a;
}

void UniversalMachine::abandonment(Platter p)
{
	delete (Array*) REG_C(m_regs, p);
}

void UniversalMachine::output(Platter p)
{
	Platter c = REG_C(m_regs, p);
	if (c > 255) {
		throw BaseException("Console can only print values in [0..255]");
	}
	cout << (char) c;
}

void UniversalMachine::input(Platter p)
{
	char c;
	Platter n;
	c = cin.get();
	n = 0x000000FF & (Platter) c;
	if (cin.eof()) {
		n = 0xFFFFFFFF;
	}
	REG_C(m_regs, p) = n;
}

void UniversalMachine::loadProgram(Platter p)
{
	Array* array = (Array*) REG_B(m_regs, p);
	if (array != 0) {
		delete m_program;
		m_program = new Array(array->size);
		copy(array->data, array->data + array->size, m_program->data);
	}
	m_finger = m_program->data + REG_C(m_regs, p);
}

void UniversalMachine::setRegister(Platter p)
{
	REG_SET(m_regs, p);
}
