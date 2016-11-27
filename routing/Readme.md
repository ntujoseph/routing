
main.c 主要調整參數
```c
#define ROLE 0    // this code is for  1: transmitter,  0: receiver    
#define MY_DEVICE_ADDR  0x0007   //mainly for transmitter device address (0~7)       
#define MY_DEVICE_ID  MY_DEVICE_ADDR   //should be (0~7)    


#define TARGET_ADDR 0x00F1   //receiver address
#define MAX_PACKET_COUNT 1000  //transmitter 會送1000個後就停止傳送
#define MAX_RECV_COUNT 400  //收到多少個封包後,就結算throughput    
``` 


### 1個收, 8個送

* Receiver address: 0x00F1
* ID 1:  Transmitter address: 0x0001  
* ID 2:  Transmitter address: 0x0002   
* ID 3:  Transmitter address: 0x0003   
* ID 4:  Transmitter address: 0x0004    
* ID 5:  Transmitter address: 0x0005   
* ID 6:  Transmitter address: 0x0006     
* ID 7:  Transmitter address: 0x0007     
* ID 8:  Transmitter address: 0x0008   

### 封包格式

dev_id | seq_no | data
-------|--------| -------------
2byte  | 2byte  | 60byte

```c
typedef struct _packet
{
  uint16_t dev_id __attribute__((packed)); //id=0,1,2,3,...,7, index to report table
  uint16_t seq_no __attribute__((packed));  //sequence number
   uint8_t data[60] __attribute__((packed));
	
}Packet;
```
**記錄每一個人的throughput統計表**

```c
typedef struct _record {
  
	uint16_t dev_id; 
	uint16_t last_seq_no; 
	uint16_t packet_count; 
	uint16_t t1;  //the time of the first packet
	uint16_t t2;  //the timestamp of being received packet
	uint8_t  finish;
} Record;
```
如:
## N=8, test=100 

ID COUNT SEQ   T1  T2   
 8  100  141   65  85   
ID COUNT SEQ   T1  T2  
 1  100  183   65  93  
ID COUNT SEQ   T1  T2  
 2  100  264   65  100  
ID COUNT SEQ   T1  T2   
 3  100  215   70  100  
ID COUNT SEQ   T1  T2  
 7  100  250   70  105   
ID COUNT SEQ   T1  T2  
 6  100  285   69  113  
ID COUNT SEQ   T1  T2   
 5  100  335   67  115  
ID COUNT SEQ   T1  T2   
 4  100  325   67  117   


# 計算每個Device的throughput

throughput=(packet_count\*64)/((T2-T1)*100ms)     
packet_count=MAX_RECV_COUNT(we set to 400)
