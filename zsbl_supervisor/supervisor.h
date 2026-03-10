#ifndef SUPERVISOR_H
#define SUPERVISOR_H

void supervisor_main(unsigned long hartid, unsigned long dtb);
void supervisor_trap_handler(void);

#endif
