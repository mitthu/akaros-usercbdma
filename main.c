/*
UserSpace CBDMA Driver
======================

For kernel space
----------------
uintptr_t uva2kva(struct proc *p, void *uva, size_t len, int prot)
prot is e.g. PROT_WRITE (writable by userspace).
returns a KVA, which you can convert to a phys addr with PADDR().
*/

#include <stdio.h>

int main() {
	printf("bare-bones\n");
	return 0;
}
