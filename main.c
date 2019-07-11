/*
UserSpace CBDMA Driver
======================

TODO
----
- port struct desc + status combo from kernel
- malloc src and dst regions for performing memcopy
- construct the struct on the page
- open #cbdma/ucopy and write the addr of desc
- print out the buffers & status field after return
- add struct ucdma contains: struct desc, status, dmacount

TODO: in kernel/../cbdm.c
-------------------------
- create #cbdma/ucopy file
- write function: issue_dma()
    * takes a struct ucdma
    * issue the relevant writes and update struct ucdma
- on open: construct help buffer
- on read: print help
- on write: (variant #1)
    * convert addr to kaddr
    * read the struct and convert all user-space addrs to paddr variants
    * call issue_dma() and return (blocking call)
- on write: (variant #2)
    * this will be the IOMMU enabled version

For kernel space
----------------
uintptr_t uva2kva(struct proc *p, void *uva, size_t len, int prot)
prot is e.g. PROT_WRITE (writable by userspace).
returns a KVA, which you can convert to a phys addr with PADDR().
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>

/* Get the 2^20th page number. 2^20 * 4k = 4GB */
#define PAGENUM (1 << 20)

void panic(char *str) {
	perror(str);
	exit(2);
}

void *map_page()
{
	void* region;
	size_t pagesize = getpagesize();

	printf("System page size: %zu bytes\n", pagesize);
	
	region = mmap((void*) (pagesize * PAGENUM), pagesize,
		PROT_READ|PROT_WRITE|PROT_EXEC,MAP_ANON|MAP_PRIVATE, 0, 0);

	if (region == MAP_FAILED)
		panic("cannot mmap");

	return region;
}

void unmap_page(void *region)
{
	int err;
	size_t pagesize = getpagesize();

	err = munmap(region, pagesize);
	if (err)
		panic("cannot munmap");
}

int main()
{
	char *region;
	
	region = map_page();
	
	strcpy(region, "Hello, poftut.com");
	printf("Contents of region: %s\n", region);

	unmap_page(region);

	return 0;
}
