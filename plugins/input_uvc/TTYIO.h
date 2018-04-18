#ifndef _TTY_IO_H_
#define _TTY_IO_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum TTY_DEV_ID
{
	TTY_DEV_0 = 0,
	TTY_DEV_1,
	TTY_DEV_2
}
TTY_DEV_ID;

typedef struct TTY_INFO TTY_INFO;

TTY_INFO *TTY_Ready(TTY_DEV_ID eID); 

int TTY_SetParity(TTY_INFO *ptty,int databits,int parity,int stopbits,int speed); 

int TTY_Clean(TTY_INFO *ptty); 

int TTY_Sendn(TTY_INFO *ptty,char *pbuf,int size); 

int TTY_SendnNB(TTY_INFO *ptty,char *pbuf,int size);

int TTY_Recvn(TTY_INFO *ptty,char *pbuf,int size); 

int TTY_RecvNB(TTY_INFO *pTTY, char *pBuf, int bufSize);

int TTY_Lock(TTY_INFO *ptty); 

int TTY_Unlock(TTY_INFO *ptty);

typedef enum
{
	TTY_SEND,
	TTY_RECV,
}
TTY_SRTYPE;

void TTY_SetSendRecvType(TTY_INFO *pTTY, TTY_SRTYPE eType);

#ifdef __cplusplus
}
#endif

#endif
