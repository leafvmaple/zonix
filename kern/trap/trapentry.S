.globl _trap_entry
_trap_entry:
    # push registers to build a trap frame
    # therefore make the stack look like a struct trapframe
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs
    pushal                                        # Push EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI

    call trap

    iret
