#include "ns.h"

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
	while(1)
	{
		// Read the packet
		if(NSREQ_OUTPUT == ipc_recv(NULL,&nsipcbuf,NULL)) 
		{	
			// Send the packet and check the result. Yield if failed.
			while(-1 == sys_e1000_transmit(nsipcbuf.pkt.jp_data,nsipcbuf.pkt.jp_len)){
				sys_yield();
			}

		}
	}
}
