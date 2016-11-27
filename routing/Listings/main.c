//-----------------------
// Wireless Communication Homework
// Author List: joseph(D05921016@ntu.edu.tw), ,  
// Date: Nov 27,2016
//---------------------
// Note: 
// LED Pin  on J3-03
//------------------------------

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "autonet.h"

//--------------------
#define MY_DEVICE_ADDR  0x0003   //!!!!! SHOULD set different mac addr  !!!!!!!!
#define KING_ID 'C'   //Target ID 'H' 
#define DEBUG 1
//------------------------------


#define PAN_ID  0x0007
#define RADIO_CHANNEL  25
#define TYPE 0x1

#define RCV_BUFSIZE 128
#define MAX 20  //routing table size
//define Packet type
#define WHOAMI_REQ 	0x8000
#define WHOAMI_REP 	0x8001
#define RREP 			  0x4000
#define FLAG   			0x2000
#define NORMAL   		0x1000


#define SLEEP_TIME 5000  //unit: ms
#define BLINK_PERIOD 300  //unit: ms


int debug=DEBUG;

typedef struct _host
{
  uint16_t my_ID;  
	uint16_t my_addr;
		
}Host;

typedef struct _packet
{
  uint16_t type;
  uint16_t dest_id;
  uint16_t dest_mac;
	uint16_t src_id;
  uint16_t src_mac;
	uint8_t length;
	uint8_t data[5];
}Packet;

	  

typedef struct _route {
  
  uint16_t dest_id;
  uint16_t dest_mac;
	uint16_t next_id;
  uint16_t next_mac;
	
}Route;

typedef struct _rtable {
        
  Route table[MAX];
  uint8_t index; 

}Route_Table;

Host host;
#define PRINT_BUFSIZE 128
char output_array[PRINT_BUFSIZE]={0}; 	

//Function list
//-------------------
void debug_print(char *s);
void show_myinfo(void);
void show_report(uint16_t id);
void dump_packet(Packet *p);
void req_whoami(void);
void reply_whoami(uint16_t dest_id,uint16_t macaddr);
void send_message( uint16_t type,uint16_t dest_id,uint16_t dest_mac, uint8_t *data, uint8_t size);
void send_RREP(uint16_t id, uint8_t *data, uint8_t size,Route_Table *tbl);
void send_FLAG(void);
void init(void);

void update_table(Packet *pkt,Route_Table *tbl);
//---Routing Table Processing-----------

void init_table(Route_Table *tbl);
uint8_t add_route(Route *route,Route_Table *tbl);
void dump_table(Route_Table *tbl);
uint16_t find_mac(uint16_t id, Route_Table *tbl);
//----Utility function---------------------
void pause(uint16_t period);
void blink_led(int count);

int main(void)
{
	
	uint8_t rcvd_msg[RCV_BUFSIZE];
	uint8_t rcvd_payload[RCV_BUFSIZE];
	uint8_t rcvd_length;
 

	uint8_t rcvd_payloadLength;
	uint8_t rcvd_rssi;
	Packet * pkt;
	Route r_entry;	
	uint16_t timer_count=0;
	Host host_info;
	Route_Table rtable;
	uint8_t blink_count=0;
	uint16_t prev_ID;
 
  
  //Packet packet={0x1,'A',0x10,'B',0x11,0,{0}};

	init();  //init all
  prev_ID=host.my_ID;
  init_table(&rtable);
	
  debug_print("Starging .....\r\n");
	 show_myinfo();


  	setTimer(1, 100, UNIT_MS);  //100ms
    //send WHOAMI packet in ordet to recognize the who i am (ID)
		debug_print("send WHOAMI packet .....\r\n");	 
		
		
		req_whoami();
		timer_count=0;
	
	while(1){
		
		 if(checkTimer(1)) { //timer
          timer_count++;
		  } 
		
	
		if(RF_Rx(rcvd_msg, &rcvd_length, &rcvd_rssi)){
			
			getPayloadLength(&rcvd_payloadLength, rcvd_msg);
			getPayload(rcvd_payload, rcvd_msg, rcvd_payloadLength);
					
		 pkt=(Packet *)rcvd_payload;
      dump_packet(pkt);
			
			//packet handleing
			switch (pkt->type) {
			
				case WHOAMI_REQ:
					reply_whoami(pkt->src_id,pkt->src_mac);
				
		
					break;
			 case WHOAMI_REP:
				 
			 		// ID sequence is determined by boot sequence 				    
					if (pkt->src_id >= host.my_ID) {
						  host.my_ID=pkt->src_id+1;
						  blink_count=(host.my_ID-prev_ID);
						  blink_led(blink_count); 
						  prev_ID=host.my_ID;
					}						
					 show_myinfo();
				   update_table(pkt,&rtable);
				 
					if (host.my_ID==KING_ID) {
					    //send RREP back
						uint16_t pre_id=host.my_ID-1;
			      uint8_t data[5];
					  memcpy(data,&host,sizeof(Host));
		        send_RREP(pre_id,data,sizeof(Host),&rtable);						
					
						//broadcast 'FLAG'  
					  debug_print("broadcast 'FLAG' ....\r\n");
						send_FLAG();
						
						 // sleep for 5 seconds
				
						pause(SLEEP_TIME);
						debug_print("wake up!!\r\n");
						
					}

					break;				

				
				case RREP:
				 //update routing table 	
          debug_print("Get RREP....\r\n");
						memcpy(&host_info,pkt->data,pkt->length);
						r_entry.dest_id=host_info.my_ID;
						r_entry.dest_mac=host_info.my_addr;
						r_entry.next_id=pkt->src_id;
						r_entry.next_mac=pkt->src_mac;
				  	add_route(&r_entry,&rtable);						
						dump_table(&rtable);
				    //send RREP back to hop by hop 
				    send_RREP(host.my_ID-1,pkt->data,pkt->length,&rtable);

					break;				
			  default: //normal pacekt
			     //do_packet
				    //......not yet ....
				  break;
						
			 } //end switch
	   }  //if rx available
		
		if (timer_count==10 && host.my_ID=='@') {  // wait for 1 second and nobody reply ,myID is 'A' (first one)
		    host.my_ID='A';			
		    timer_count=0;
		  	debug_print("I'm the first one , set myID to 'A'\r\n");	 
			  blink_led(1); 
			  show_myinfo();
		}
		
	}  //edn while(1)



		

} //end main

//---------------------------------------------
//Packet Handling
//---------------------------------------------
void init()
{
	Initial(MY_DEVICE_ADDR,TYPE, RADIO_CHANNEL, PAN_ID);
	
  host.my_addr=MY_DEVICE_ADDR;
	host.my_ID='@'; //init to 'A':0x41, @:0x40 
	
}



void debug_print(char *s)
{
	  int len=strlen(s);
	  COM2_Tx((uint8_t *)s,len);


}

//King ID will show report
void show_report(uint16_t id)
{
  uint8_t i;
	char output_array[PRINT_BUFSIZE]={0}; 	
		sprintf((char *)output_array,"ID COUNT SEQ T1	T2\r\n");
		debug_print(output_array);
		
	
	for(i=0;i<10;i++){
	
	//sprintf((char *)output_array,"%2d %4d %4d %4d %4d\r\n",
	//	 rtable[id].dev_id,rtable[id].packet_count,rtable[id].last_seq_no,rtable[id].t1,rtable[id].t2);
	//debug_print(output_array);
	}

}


void req_whoami()
{  	
	
	 Packet packet;
	 packet.type=WHOAMI_REQ; //whoami
	 packet.dest_id='0';
	 packet.dest_mac=0xFFFF;
	 packet.src_id=host.my_ID;
	 packet.src_mac=host.my_addr;	
	
	//broadcst
  RF_Tx(0xFFFF,(uint8_t *)&packet,sizeof(Packet));

}

void reply_whoami(uint16_t dest_id,uint16_t macaddr)
{
	  	
	 Packet packet;
	 packet.type=WHOAMI_REP; 
	 packet.dest_id=dest_id;
	 packet.dest_mac=macaddr;
	 packet.src_id=host.my_ID;
	 packet.src_mac=host.my_addr;	
	
	//unicast to sender
  RF_Tx(macaddr,(uint8_t *)&packet,sizeof(Packet));

}




void send_message( uint16_t type,uint16_t dest_id,uint16_t dest_mac, uint8_t *data, uint8_t size)
{
 
	 Packet packet;
	 packet.type=type; 
	 packet.dest_id=dest_id;
	 packet.dest_mac=dest_mac; 
	 packet.src_id=host.my_ID;
	 packet.src_mac=host.my_addr;	
	 memcpy(&packet.data,data,size);
	 packet.length=size;
	
  //unicast
  RF_Tx(packet.dest_mac,(uint8_t *)&packet,sizeof(Packet));
 
 

}


void send_FLAG()
{
  send_message(FLAG,'0',0xFFFF,NULL,0);

}


	
void send_RREP(uint16_t id, uint8_t *data, uint8_t size,Route_Table *tbl)
{
	//query a MAC addr with its ID  
	 uint16_t  q_addr;
  q_addr=find_mac(id,tbl);   
 	if (q_addr)
	   send_message(RREP,id,q_addr,data,size);

}

void dump_packet(Packet *p)
{
	
	 uint8_t i;
  
	if (debug) {
		sprintf((char *)output_array,"Receiving: %#x\t%c\t%#x\t%c\t%#x \t%d\r\n",p->type,p->dest_id,p->dest_mac,p->src_id,p->src_mac,p->length);
	   	debug_print(output_array);
	     for (i=0;i<p->length;i++) {
	       	sprintf((char *)output_array,"%#x ",p->data[i]);
				  debug_print(output_array);
	 
			 }
    debug_print("\r\n");
		 }
}



void show_myinfo()
{

  sprintf((char *)output_array,"My Info: ID=%c,MAC Addr=%#x\r\n",host.my_ID,host.my_addr);
	debug_print(output_array);
	
}

void update_table(Packet *pkt,Route_Table *tbl)
{
	 //update routing table 	

	  Route r_entry;	
		r_entry.dest_id=pkt->src_id;
		r_entry.dest_mac=pkt->src_mac;
		r_entry.next_id=pkt->src_id;
		r_entry.next_mac=pkt->src_mac;
		add_route(&r_entry,tbl);						
	  dump_table(tbl);

}
//------------------------------
//Utility function
//--------------------------------
void pause(uint16_t period)
{
		//sleep for SLEEP_TIME units
	  
		 setTimer(2,period, UNIT_MS);
     while (!checkTimer(2))
			        ;
			  
}

//blinking LED
void blink_led(int count)
{
	uint8_t i;
	for (i=0;i<count;i++) {		
	    setGPIO(2,1);
		  pause(BLINK_PERIOD);
	  	setGPIO(2,0);
		  pause(BLINK_PERIOD);
		
  }		
}




//---------------------------------------------
//Table Handling
//---------------------------------------------
void init_table(Route_Table *tbl)
{
 
   memset(tbl,0,sizeof(Route_Table));
   tbl->index=0;

}

uint8_t add_route(Route *route,Route_Table *tbl)
{
   
   //printf("%c %#x %c %#x\n",route->dest_id,route->dest_mac,route->next_id,route->next_mac);

   memcpy(&tbl->table[tbl->index],route,sizeof(Route));
   tbl->index++;
   return tbl->index;
  

}


uint16_t find_mac(uint16_t id, Route_Table *tbl)
{
   uint8_t i;

   for (i=0;i<tbl->index;i++)
   {
       
       if (id==tbl->table[i].dest_id) 
          return tbl->table[i].dest_mac;      
            
 
     
   }

   return 0;

}


void dump_table(Route_Table *tbl)
{
   uint8_t i;
   debug_print("dest_id\tdest_mac  next_id\tnext_mac\r\n");    
	
   for (i=0;i<tbl->index;i++)
   {
		 	sprintf((char *)output_array,"%c\t0x%x\t%c\t0x%x\r\n",tbl->table[i].dest_id,tbl->table[i].dest_mac,tbl->table[i].next_id,tbl->table[i].next_mac);
	   	debug_print(output_array);
     
  
   }

}



