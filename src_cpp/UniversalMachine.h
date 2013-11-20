#ifndef __UNIVERSALMACHINE_H__
#define __UNIVERSALMACHINE_H__

#include <map>
#include <string>
#include "Defs.h"
#include "Array.h"

class UniversalMachine
{
private:
	Array *m_program;
	Platter *m_finger;
	Platter m_regs[MAX_REGS];

public:
	UniversalMachine();
	virtual ~UniversalMachine();
	
	virtual void run(std::string filename);

protected:
	virtual void loadFile(std::string& filename);
	virtual void spin();

private:
	void conditionalMove(Platter p);
	void arrayIndex(Platter p);
	void arrayAmendment(Platter p);
	void addition(Platter p);
	void multiplication(Platter p);
	void division(Platter p);
	void notAnd(Platter p);
	void allocation(Platter p);
	void abandonment(Platter p);
	void output(Platter p);
	void input(Platter p);
	void loadProgram(Platter p);
	void setRegister(Platter p);
};

#endif // __UNIVERSALMACHINE_H__
