
#include "mem.h"
#include "stdlib.h"
#include "string.h"
#include <pthread.h>
#include <stdio.h>

static BYTE _ram[RAM_SIZE];

static struct {
	uint32_t proc;	// ID of process currently uses this page
	int index;	// Index of the page in the list of pages allocated
			// to the process.
	int next;	// The next page in the list. -1 if it is the last
			// page.
} _mem_stat [NUM_PAGES];

static pthread_mutex_t mem_lock;

void init_mem(void) {
	memset(_mem_stat, 0, sizeof(*_mem_stat) * NUM_PAGES);
	memset(_ram, 0, sizeof(BYTE) * RAM_SIZE);
	pthread_mutex_init(&mem_lock, NULL);
}

/* get offset of the virtual address */
static addr_t get_offset(addr_t addr) {
	return addr & ~((~0U) << OFFSET_LEN);
}

/* get the first layer index */
static addr_t get_first_lv(addr_t addr) {
	return addr >> (OFFSET_LEN + PAGE_LEN);
}

/* get the second layer index */
static addr_t get_second_lv(addr_t addr) {
	return (addr >> OFFSET_LEN) - (get_first_lv(addr) << PAGE_LEN);
}

/* Search for page table table from the a segment table */
static struct page_table_t * get_page_table(
		addr_t index, 	// Segment level index
		struct seg_table_t * seg_table) { // first level table
	
	/*
	 * TODO: Given the Segment index [index], you must go through each
	 * row of the segment table [seg_table] and check if the v_index
	 * field of the row is equal to the index
	 *
	 * */

	int i;
	for (i = 0; i < seg_table->size; i++) {
		// Enter your code here
		if(index == seg_table->table[i].v_index){
			return seg_table->table[i].pages;
		}
	}
	return NULL;

}

/* Translate virtual address to physical address. If [virtual_addr] is valid,
 * return 1 and write its physical counterpart to [physical_addr].
 * Otherwise, return 0 */
static int translate(
		addr_t virtual_addr, 	// Given virtual address
		addr_t * physical_addr, // Physical address to be returned
		struct pcb_t * proc) {  // Process uses given virtual address

	/* Offset of the virtual address */
	addr_t offset = get_offset(virtual_addr);
	/* The first layer index */
	addr_t first_lv = get_first_lv(virtual_addr);
	/* The second layer index */
	addr_t second_lv = get_second_lv(virtual_addr);
	
	/* Search in the first level */
	struct page_table_t * page_table = NULL;
	page_table = get_page_table(first_lv, proc->seg_table);
	if (page_table == NULL) {
		return 0;
	}

	int i;
	for (i = 0; i < page_table->size; i++) {
		if (page_table->table[i].v_index == second_lv) {
			/* TODO: Concatenate the offset of the virtual addess
			 * to [p_index] field of page_table->table[i] to 
			 * produce the correct physical address and save it to
			 * [*physical_addr]  */
			//concatenate_virtual_addr(virtual_addr, physical_addr, proc);
			uint32_t p_part = page_table->table[i].p_index << OFFSET_LEN;
			*physical_addr = p_part + offset;
			return 1;
		}
	}
	return 0;	
}

addr_t alloc_mem(uint32_t size, struct pcb_t * proc) {
	pthread_mutex_lock(&mem_lock);
	addr_t ret_mem = 0;
	/* TODO: Allocate [size] byte in the memory for the
	 * process [proc] and save the address of the first
	 * byte in the allocated memory region to [ret_mem].
	 * */

	uint32_t num_pages = ((size % PAGE_SIZE) == 0) ? size / PAGE_SIZE : size / PAGE_SIZE + 1; // Number of pages we will use
	int mem_avail = 0; // We could allocate new memory region or not?

	/* First we must check if the amount of free memory in
	 * virtual address space and physical address space is
	 * large enough to represent the amount of required 
	 * memory. If so, set 1 to [mem_avail].
	 * Hint: check [proc] bit in each page of _mem_stat
	 * to know whether this page has been used by a process.
	 * For virtual memory space, check bp (break pointer).
	 * */
////////////////////////////////////
//printf("alloc %4d, id: %d\n", num_pages, proc->pid);


	int i, spc_avail = 0;
	for (i = 0; i < NUM_PAGES; i++)
	{
		if (_mem_stat[i].proc == 0)
		{
			spc_avail++;
		}
		if (spc_avail == num_pages)
		{
			mem_avail = 1;
			break;
		}
	}

	if(proc->bp + num_pages * PAGE_SIZE > RAM_SIZE){
		mem_avail = 0;
		//printf("2\n");
	}
	
	if (mem_avail) {
		/* We could allocate new memory region to the process */
		ret_mem = proc->bp;
		proc->bp += num_pages * PAGE_SIZE;
		/* Update status of physical pages which will be allocated
		 * to [proc] in _mem_stat. Tasks to do:
		 * 	- Update [proc], [index], and [next] field
		 * 	- Add entries to segment table page tables of [proc]
		 * 	  to ensure accesses to allocated memory slot is
		 * 	  valid. */
		addr_t vmem_addr = ret_mem;
		//spc_avail = 0;
		int spc_need = 0;
		int seg_idx = -1; 
		int p_last = -1;
		int frame_prev = -1;
		for (i = 0; i < NUM_PAGES; i++){
			if(_mem_stat[i].proc == 0){
				_mem_stat[i].proc = proc->pid;
				_mem_stat[i].index = spc_need;
				if(frame_prev != -1){
					_mem_stat[frame_prev].next = i;
					//printf("3\n");
				}
				frame_prev = i;
				seg_idx = get_first_lv(vmem_addr);
				//printf("4\n");
				if(proc->seg_table->table[seg_idx].pages == NULL){
					//proc is not in seg table
					proc->seg_table->table[seg_idx].pages = malloc(sizeof(struct page_table_t));
					proc->seg_table->table[seg_idx].pages->size = 0;
					//printf("not here\n");
				}
				proc->seg_table->table[seg_idx].pages->size++;
				p_last = proc->seg_table->table[seg_idx].pages->size - 1;

				proc->seg_table->table[seg_idx].v_index = seg_idx;
				proc->seg_table->table[seg_idx].pages->table[p_last].v_index = get_second_lv(vmem_addr);
				proc->seg_table->table[seg_idx].pages->table[p_last].p_index = i;
//printf("5\n");

				vmem_addr += PAGE_SIZE;
				spc_need++;
				proc->seg_table->size++;
				if(spc_need == num_pages){
					_mem_stat[i].next = -1;
					//printf("%d\n", spc_need);
					break;
				}
				//printf("check %d\n", spc_need);
			}
			else{
				//printf("ocp slot: %d\n", _mem_stat[i].proc);
				continue;
			}
			
		}
	}
	//printf("7\n");
	pthread_mutex_unlock(&mem_lock);
	return ret_mem;
}

int free_mem(addr_t address, struct pcb_t * proc) {
	/*TODO: Release memory region allocated by [proc]. The first byte of
	 * this region is indicated by [address]. Task to do:
	 * 	- Set flag [proc] of physical page use by the memory block
	 * 	  back to zero to indicate that it is free.
	 * 	- Remove unused entries in segment table and page tables of
	 * 	  the process [proc].
	 * 	- Remember to use lock to protect the memory from other
	 * 	  processes.  */
////printf("free\n");

	pthread_mutex_lock(&mem_lock);	
	//int num_pages = 0;
	addr_t paddr;
	addr_t vaddr = address;
	int i;
	if(translate(address,&paddr,proc)){	// check address is valid and get physical_addr
		addr_t ppage=paddr>>OFFSET_LEN;

		while(ppage!=-1){
			_mem_stat[ppage].proc=0;
			addr_t seg_idx = get_first_lv(vaddr);
			for (i = 0; i < proc->seg_table->table[seg_idx].pages->size; i++) 
				if (proc->seg_table->table[seg_idx].pages->table[i].p_index == ppage) {
					proc->seg_table->table[seg_idx].pages->table[i].v_index = 0;
					proc->seg_table->table[seg_idx].pages->table[i].p_index = 0;
					//printf("not 1\n");
			}
			ppage=_mem_stat[ppage].next;
			vaddr+=PAGE_SIZE;
			//printf("not 2\n");
		}
	}
	//printf("not 3\n");
	pthread_mutex_unlock(&mem_lock);
	return 0;
}
/////////////////////////
int read_mem(addr_t address, struct pcb_t * proc, BYTE * data) {
	addr_t physical_addr;
	if (translate(address, &physical_addr, proc)) {
		*data = _ram[physical_addr];
		return 0;
	}else{
		return 1;
	}
}

int write_mem(addr_t address, struct pcb_t * proc, BYTE data) {
	addr_t physical_addr;
	if (translate(address, &physical_addr, proc)) {
		_ram[physical_addr] = data;
		return 0;
	}else{
		return 1;
	}
}

void dump(void) {
	int i;
	for (i = 0; i < NUM_PAGES; i++) {
		if (_mem_stat[i].proc != 0) {
			printf("%03d: ", i);
			printf("%05x-%05x - PID: %02d (idx %03d, nxt: %03d)\n",
				i << OFFSET_LEN,
				((i + 1) << OFFSET_LEN) - 1,
				_mem_stat[i].proc,
				_mem_stat[i].index,
				_mem_stat[i].next
			);
			int j;
			for (	j = i << OFFSET_LEN;
				j < ((i+1) << OFFSET_LEN) - 1;
				j++) {
				
				if (_ram[j] != 0) {
					printf("\t%05x: %02x\n", j, _ram[j]);
				}
					
			}
		}
	}
}


