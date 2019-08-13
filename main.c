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
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <inttypes.h>
#include <fcntl.h>

#define CBDMA_DESC_CTRL_INTR_ON_COMPLETION           	0x00000001
#define CBDMA_DESC_CTRL_WRITE_CHANCMP_ON_COMPLETION  	0x00000008

/* Get the 2^20th page number. 2^20 * 4k = 4GB */
#define PAGENUM (1 << 20)
#define BUFFERSIZE 20
#define PCI_DEV "00:03.0"

/* Descriptor structue as defined in the programmer's guide.
 * It describes a single DMA transfer
 */
struct desc {
        uint32_t  xfer_size;
        uint32_t  descriptor_control;
        uint64_t  src_addr;
        uint64_t  dest_addr;
        uint64_t  next_desc_addr;
        uint64_t  next_source_address;
        uint64_t  next_destination_address;
        uint64_t  reserved0;
        uint64_t  reserved1;
} __attribute__((packed));

/* describe a DMA */
struct ucbdma {
        struct desc             desc;
        volatile uint64_t       status;
        uint16_t                ndesc;
};

void panic(char *str) {
	perror(str);
	exit(2);
}

void *map_page()
{
	void* region;
	size_t pagesize = getpagesize();

	printf("[user] page size: %zu bytes\n", pagesize);
	
	region = mmap((void*) (pagesize * PAGENUM), pagesize,
		PROT_READ|PROT_WRITE|PROT_EXEC,MAP_ANON|MAP_PRIVATE, 0, 0);

	if (region == MAP_FAILED)
		panic("cannot mmap");

	return region;
}

void test_mmap(char *region)
{
	strcpy(region, "Hello, poftut.com");
	printf("Contents of region: %s\n", region);
}

void unmap_page(void *region)
{
	int err;
	size_t pagesize = getpagesize();

	err = munmap(region, pagesize);
	if (err)
		panic("cannot munmap");
}

void issue_dma(struct ucbdma *ptr)
{
	int fd = open("/cbdma/ucopy", O_RDWR);

	printf("[user] ucbdma ptr: %p\n", ptr);
	write(fd, ptr, sizeof(struct ucbdma *));

	close(fd);
}

void fill_buffer(char *buffer, char c, int size)
{
	memset(buffer, c, size);
	buffer[size-1] = '\0';
}

void dump_ucbdma(struct ucbdma *ucbdma)
{
	printf("[user] ucbdma: %p\n", ucbdma);
	printf("[user] \tdesc->xref_size: %d\n", ucbdma->desc.xfer_size);
	printf("[user] \tdesc->src_addr: %p\n", ucbdma->desc.src_addr);
	printf("[user] \tdesc->dest_addr: %p\n", ucbdma->desc.dest_addr);
	printf("[user] \tndesc: %d\n", ucbdma->ndesc);
	printf("[user] \tstatus: 0x%llx\n", ucbdma->status);
}

void attach_device(void)
{
	int fd = open("/sys/iommu/attach", O_RDWR);
	char buf[1024];

	sprintf(buf, "%s %d\n", PCI_DEV, getpid());
	write(fd, buf, strlen(buf));

	close(fd);

	system("cat /sys/iommu/mappings");
}

void detach_device(void)
{
	int fd = open("/sys/iommu/detach", O_RDWR);

	write(fd, PCI_DEV, strlen(PCI_DEV));

	close(fd);
}

int main()
{
	char *region;
	struct ucbdma *ucbdma;
	char src[BUFFERSIZE], dst[BUFFERSIZE];

	attach_device();
	
	/* setup src and dst buffers */
	fill_buffer(src, '1', BUFFERSIZE);
	fill_buffer(dst, '0', BUFFERSIZE);
	printf("[user] src: %s\n", src);
	printf("[user] dst: %s\n", dst);

	/* map page for placing ucbdma */
	region = map_page();
	// test_mmap(region);

	/* setup ucbdma*/
	ucbdma = (struct ucbdma *) region;
	ucbdma->status = 0;
	ucbdma->desc.descriptor_control
		= CBDMA_DESC_CTRL_INTR_ON_COMPLETION
		| CBDMA_DESC_CTRL_WRITE_CHANCMP_ON_COMPLETION;
	ucbdma->desc.xfer_size = BUFFERSIZE;
	ucbdma->desc.src_addr  = (uint64_t) src;
	ucbdma->desc.dest_addr = (uint64_t) dst;
	ucbdma->ndesc = 1;

	dump_ucbdma(ucbdma);
	issue_dma(ucbdma);
	dump_ucbdma(ucbdma);

	printf("[user] channel_status: %llx\n", ucbdma->status);
	printf("[user] src: %s\n", src);
	printf("[user] dst: %s\n", dst);

	/* cleanup */
	unmap_page(region);

	detach_device();

	return 0;
}
