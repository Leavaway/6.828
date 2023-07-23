#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/string.h>
#include <kern/pci.h>
#include <kern/pcireg.h>

// LAB 6: Your driver code here
volatile uint32_t* e1000;

// Initialize transmit module based on precedure in 14.5
void e1000_transmit_init(){
    // Allocate a memory to transmit descriptor arryas.
    memset(trans_desc_list, 0 , sizeof(struct e1000_rx_desc) * TRANS_DESC_SIZE);
    memset(trans_desc_buffer, 0 , sizeof(struct tBuffer) * TRANS_DESC_SIZE);

    for(int i=0;i<TRANS_DESC_SIZE;i++) {
		// Combine tran buffer zone to descriptor
		trans_desc_list[i].buffer_addr = PADDR(trans_desc_buffer[i].buffer);
		// Set RS to let e1000 report status information
		trans_desc_list[i].lower.flags.cmd = (E1000_TXD_CMD_RS>>24)|(E1000_TXD_CMD_EOP>>24);
		trans_desc_list[i].upper.fields.status = E1000_TXD_STAT_DD;
	}

	// Set TDBAL to the address of first element in the trans_desc_list
	e1000[E1000_TDBAL >> 2] = PADDR(&trans_desc_list[0]); 
	// Consider 32-bit address space, set TDBAH to 0
	e1000[E1000_TDBAH >> 2] = 0; 
	// Set TDLEN to byte size of descriptor array.
	e1000[E1000_TDLEN >> 2] = TRANS_DESC_SIZE*sizeof(struct e1000_rx_desc); 
	// Set the head offset of queue to 0 
	e1000[E1000_TDH >> 2] = 0;
	// Set the tail offset of queue to 0
	// The queue is empty with TDH and TDT are set to 0
	e1000[E1000_TDT >> 2] = 0;
	// Set TCTL
	e1000[E1000_TCTL >> 2] |= (E1000_TCTL_EN|E1000_TCTL_PSP); 
	e1000[E1000_TCTL >> 2] &= ~E1000_TCTL_CT; 
	e1000[E1000_TCTL >> 2] |= 0x10<<4;
	e1000[E1000_TCTL >> 2] &= ~E1000_TCTL_COLD; 
	e1000[E1000_TCTL >> 2] |= 0x40<<12;
	e1000[E1000_TIPG >> 2] |= (10)|(4<<10)|(6<<20);


};

// Transmit data with address of data and its length.
int e1000_transmit_data(void* addr, int length){
	// Get the tail of the queue
	int tail = e1000[E1000_TDT >> 2];
	struct e1000_tx_desc *next_desc = &trans_desc_list[tail];
	if(!(next_desc->upper.fields.status&E1000_TXD_STAT_DD)){
		return -1;
	}
	memmove(KADDR(next_desc->buffer_addr),addr,length);
	next_desc->upper.fields.status &= ~E1000_TXD_STAT_DD;
	next_desc->lower.flags.length = (uint16_t)length;
	e1000[E1000_TDT >> 2] = (tail+1)%TRANS_DESC_SIZE;
	return length;
}

// Initialize the e1000 receice module based on procedure in 14.4
void e1000_receive_init(){
	// Set the RAL/RAH
	e1000[E1000_RA >> 2] = 0x52 | (0x54<<8) | (0x00<<16) | (0x12<<24);
	e1000[(E1000_RA >> 2) + 1] = 0x34 | (0x56<<8) | E1000_RAH_AV;
	// Set the MTA to 0
	e1000[E1000_MTA >> 2] = 0;
	// Allocate the memory and initialize
	memset(rec_desc_list,0,sizeof(struct e1000_rx_desc)*REC_DESC_SIZE);
	memset(rec_desc_buffer,0,sizeof(struct tBuffer)*REC_DESC_SIZE);
	int i;
	for(i=0;i<REC_DESC_SIZE;i++){
		rec_desc_list[i].buffer_addr = PADDR(rec_desc_buffer[i].buffer);
	}
	// Set RDBAL/RDBAH with the address of the region
	e1000[E1000_RDBAL >> 2] = PADDR(&rec_desc_list[0]); 
	e1000[E1000_RDBAH >> 2] = 0; 
	// Set the RDLEN to the size of descriptor ring
	e1000[E1000_RDLEN >> 2] = REC_DESC_SIZE*sizeof(struct e1000_rx_desc);
	// Set RDH to o the first valid receive descriptor in the descriptor ring 
	e1000[E1000_RDH >> 2] = 0;
	// Set RDT to one descriptor beyond the last valid descriptor in the descriptor ring.
	e1000[E1000_RDT >> 2] = REC_DESC_SIZE-1;
	// Set RCTL
	e1000[E1000_RCTL >> 2] = (E1000_RCTL_EN | E1000_RCTL_BAM |
                                     E1000_RCTL_SZ_2048 |
                                     E1000_RCTL_SECRC);
}

// Receive data and copy it to addr
int e1000_receive_data(void* addr){
	int tail = e1000[E1000_RDT>>2];
	tail = (tail + 1) % REC_DESC_SIZE;
	struct e1000_rx_desc *rx_hold = &rec_desc_list[tail];
	if((rx_hold->status & E1000_TXD_STAT_DD) == E1000_TXD_STAT_DD){
		int len = rx_hold->length;
		memcpy(addr, rec_desc_buffer[tail].buffer, len);
		e1000[E1000_RDT>>2] = tail;
		return len;
	}
	return -1;
}

int
pci_e1000_attach(struct pci_func * pcif) 
{
    pci_func_enable(pcif);
    e1000 = mmio_map_region(pcif->reg_base[0],pcif->reg_size[0]); 
	e1000_transmit_init();
	e1000_receive_init();
    cprintf("E1000 STATUS: %x\n", e1000[E1000_STATUS/4]);
    return 0;
}