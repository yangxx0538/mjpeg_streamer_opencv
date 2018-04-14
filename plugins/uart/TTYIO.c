#include "TTYIO.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/file.h>

#define TTYIO_DEBUG
#ifdef TTYIO_DEBUG
#define _DBG(fmt, args...)	fprintf(stdout, "TTYIO: "fmt, ##args)
#else
#define _DBG(fmt, args...)
#endif
#define _ERR(fmt, args...)	fprintf(stderr, "TTYIO: "fmt, ##args)


struct TTY_INFO
{ 
	int fd;
	pthread_mutex_t mt;
	char name[24];
	struct termios ntm;
	struct termios otm;
	int fdIOCtrl;
	int nGPIOCtrlNum;
	TTY_DEV_ID eID;
};


TTY_INFO *TTY_Ready(TTY_DEV_ID eID) 
{ 
	TTY_INFO *ptty; 

	ptty = (TTY_INFO *)malloc(sizeof(TTY_INFO)); 
	if(ptty == NULL)
	{
		printf("TTY_Ready malloc error!!\n");
		return NULL; 
	}
	memset(ptty, 0, sizeof(TTY_INFO)); 
	ptty->eID = eID;
	pthread_mutex_init(&ptty->mt,NULL); 

	sprintf(ptty->name, "/dev/ttyS1"); 
	_DBG("DEV=%s\n", ptty->name);
	ptty->fd = open(ptty->name, O_RDWR | O_NOCTTY |O_NDELAY);
	//ptty->fdIOCtrl = open("/dev/dm8127_gpio", O_RDWR);
	/*if(ptty->fdIOCtrl < 0)
	{
		//_ERR("Failed to open IOCtrl FD '%s'\n", DM_GPIO);
		free(ptty);
		return(NULL);
	}*/
	if (ptty->fd <0) 
	{
		_ERR("Failed to open TTY FD '%s'\n", ptty->name);
		free(ptty); 
		return NULL; 
	} 
	
/*
#ifdef DM3730
	if(eID == TTY_DEV_0)
		ptty->nGPIOCtrlNum = DM3730_GPIO_RS485_1_EN;
	else if(eID == TTY_DEV_1)
		ptty->nGPIOCtrlNum = DM3730_GPIO_RS485_2_EN;
#else	//DM6446
	if(eID == TTY_DEV_1)
		ptty->nGPIOCtrlNum = DM644X_GPIO_RS485TR;
	else if(eID == TTY_DEV_2)
		ptty->nGPIOCtrlNum = DM644X_GPIO_ALM_OUT0;	//��Ӳ���ϱ�����Ϊ�ڶ�485�ڵĿ���

#endif*/
	/* ȡ�ò��ұ���ԭ�������� */
	tcgetattr(ptty->fd,&ptty->otm); 
	return ptty; 
} 

/* �������豸��Դ */

int TTY_Clean(TTY_INFO *ptty) 
{ 
	/* �رմ򿪵Ĵ����豸 */

	if(ptty->fd>0) 
	{
		tcsetattr(ptty->fd,TCSANOW,&ptty->otm); 
		close(ptty->fd); 
		ptty->fd = -1; 
	}
	/*if(ptty->fdIOCtrl > 0)
	{
		close(ptty->fdIOCtrl);
		ptty->fdIOCtrl = -1;
	}*/
	free(ptty); 
	ptty = NULL; 

	return 0; 
} 

/* ���ô�������λ��ֹͣλ��У��λ �Ͳ�����*/
/* ptty ��������(TTY_INFO *),�Ѿ���ʼ���Ĵ����豸��Ϣ�ṹָ�� */
/* databits ��������(int), ����λ,ȡֵΪ7����8 */
/* parity ��������(int),Ч������ ȡֵΪN,E,O,,S */
/* stopbits ��������(int),ֹͣλ,ȡֵΪ1����2 */
/* return ����ֵ����(int),����ִ�гɹ�������ֵ�����򷵻ش������ֵ */

int TTY_SetParity(TTY_INFO *ptty,int databits,int parity,int stopbits,int speed) 
{ 
	/* ȡ�ô������� */
	if( tcgetattr(ptty->fd,&ptty->ntm) != 0) 
	{ 
		_DBG("SetupSerial [%s]\n",ptty->name); 
		return 1; 
	} 

	bzero(&ptty->ntm, sizeof(ptty->ntm)); 
	ptty->ntm.c_cflag = CS8 | CLOCAL | CREAD; 
	ptty->ntm.c_iflag = IGNPAR; 
	ptty->ntm.c_oflag = 0; 

	/* ���ô��ڵĸ��ֲ��� */

	ptty->ntm.c_cflag &= ~CSIZE; 
	switch (databits)  /*��������λ��*/
	{ 
	case 7: 
		ptty->ntm.c_cflag |= CS7; 
		break; 
	case 8: 
		ptty->ntm.c_cflag |= CS8; 
		//printf("n=8\n");
		break; 
	default: 
		_ERR("Unsupported data size\n"); 
		return 5; 
	} 

	switch (parity) 
	{ 
		/* ������żУ��λ��  */ 

	case 'n': 
	case 'N': 
		ptty->ntm.c_cflag &= ~PARENB; /* Clear parity enable */ 
		ptty->ntm.c_iflag &= ~INPCK; /* Enable parity checking */ 
		//printf("no odd\n");
		break; 
	case 'o': 
	case 'O': 
		ptty->ntm.c_cflag |= (PARODD|PARENB); /* ����Ϊ��Ч��*/ 
		ptty->ntm.c_iflag |= INPCK; /* Disnable parity checking */ 
		break; 
	case 'e': 
	case 'E': 
		ptty->ntm.c_cflag |= PARENB; /* Enable parity */ 
		ptty->ntm.c_cflag &= ~PARODD; /* ת��ΪżЧ��*/ 
		ptty->ntm.c_iflag |= INPCK; /* Disnable parity checking */ 
		break; 
	case 'S': 
	case 's': /*as no parity*/ 
		ptty->ntm.c_cflag &= ~PARENB; 
		ptty->ntm.c_cflag &= ~CSTOPB; 
		break; 
	default: 
		_ERR("Unsupported parity\n"); 
		return 2; 
	} 

	/* ����ֹͣλ */

	switch (stopbits) 
	{ 
	case 1: 
		ptty->ntm.c_cflag &= ~CSTOPB; 
		//printf("stop = 1\n");
		break; 
	case 2: 
		ptty->ntm.c_cflag |= CSTOPB; 
		break; 
	default: 
		_ERR("Unsupported stop bits\n"); 
		return 3; 
	} 

	ptty->ntm.c_lflag = 0; 
	ptty->ntm.c_cc[VTIME] = 0; // inter-character timer unused 
	ptty->ntm.c_cc[VMIN] = 1; // blocking read until 1 chars received 


	//ptty->ntm.c_cflag = /*CS8 |*/ CLOCAL | CREAD; 
	switch(speed) 
	{ 
	case 300: 
		ptty->ntm.c_cflag |= B300; 
		break; 
	case 1200: 
		ptty->ntm.c_cflag |= B1200; 
		break; 
	case 2400: 
		ptty->ntm.c_cflag |= B2400; 
		break; 
	case 4800: 
		ptty->ntm.c_cflag |= B4800; 
		break; 
	case 9600: 
		ptty->ntm.c_cflag |= B9600; 
		break; 
	case 19200: 
		ptty->ntm.c_cflag |= B19200; 
		break; 
	case 38400: 
		ptty->ntm.c_cflag |= B38400; 
		break; 
	case 115200: 
		ptty->ntm.c_cflag |= B115200; 
		break; 
	} 
	ptty->ntm.c_iflag = IGNPAR; 
	ptty->ntm.c_oflag = 0; 


	tcflush(ptty->fd, TCIFLUSH); 
	if (tcsetattr(ptty->fd,TCSANOW,&ptty->ntm) != 0) 
	{ 
		_DBG("SetupSerial \n"); 
		return 4; 
	} 

	return 0; 
} 

int TTY_Recvn(TTY_INFO *ptty,char *pbuf,int size) 
{ 
	int ret,left,bytes; 

	left = size; 

//	ioctl(ptty->fdIOCtrl, _IO('G', 0), (2*32 + 8));

	while(left>0) 
	{ 
		ret = 0; 
		bytes = 0; 

		pthread_mutex_lock(&ptty->mt); 
		ioctl(ptty->fd, FIONREAD, &bytes); 
		if(bytes>0) 
		{ 
			ret = read(ptty->fd,pbuf,left); 
		} 
		pthread_mutex_unlock(&ptty->mt); 
		if(ret >0) 
		{ 
			left -= ret; 
			pbuf += ret; 
		} 
		usleep(100); 
	} 

	return size - left; 
} 

int TTY_RecvNB(TTY_INFO *pTTY, char *pBuf, int bufSize)
{
	int bytesToRead = 0;
	int bytesRead = 0;

	//ioctl(pTTY->fdIOCtrl, _IO('G', 1), (2*32 + 8));


	ioctl(pTTY->fd, FIONREAD, &bytesToRead);
	if(bytesToRead > 0)
	{
		int bytesToFlush;
		//����й�������ݣ�������һ����
		_DBG("BytesToRead = %d / BufSize = %d\n", bytesToRead, bufSize);
		while(bytesToRead > bufSize)
		{
			bytesToFlush = bytesToRead - bufSize;
			bytesToFlush = bytesToFlush < bufSize ? bytesToFlush : bufSize;
			bytesRead = read(pTTY->fd, pBuf, bytesToFlush);
			_DBG("Flushed %d bytes\n", bytesRead);
			bytesToRead -= bytesRead;
		}
		bytesRead = read(pTTY->fd, pBuf, bufSize);
		_DBG("Read %d bytes\n", bytesRead);
	}

	return(bytesRead);
}

int TTY_Sendn(TTY_INFO *ptty,char *pbuf,int size) 
{
	//���ԣ�3730��6446����Ҫ�ȴ�������ȫ���ͳ�ȥ����ܸ����շ�״̬����Ȼ���·������ݴ���
	tcflush(ptty->fd, TCIFLUSH); 

	int nRT = TTY_SendnNB(ptty, pbuf, size);
	
	tcdrain(ptty->fd);
	return(nRT); 
}

int TTY_SendnNB(TTY_INFO *ptty,char *pbuf,int size)
{
	int ret,nleft; 
	char *ptmp; 

	ret = 0; 
	nleft = size; 
	ptmp = pbuf; 

	//ioctl(ptty->fdIOCtrl, _IO('G', 0), (2*32 + 8));


	while(nleft>0) 
	{ 
		pthread_mutex_lock(&ptty->mt); 
		ret = write(ptty->fd,ptmp,nleft); 
		pthread_mutex_unlock(&ptty->mt); 

		if(ret >0) 
		{ 
			nleft -= ret; 
			ptmp += ret; 
		} 
	}
	_DBG("SEND %d bytes\n", size - nleft);
	return size - nleft; 
}

int TTY_Lock(TTY_INFO *ptty) 
{ 
	if(ptty->fd < 0) 
	{ 
		return 1; 
	} 

	return flock(ptty->fd,LOCK_EX); 
}

int TTY_Unlock(TTY_INFO *ptty) 
{ 
	if(ptty->fd < 0) 
	{ 
		return 1; 
	} 

	return flock(ptty->fd,LOCK_UN); 
}

void TTY_SetSendRecvType(TTY_INFO *pTTY, TTY_SRTYPE eType)
{
#ifdef DM6446
	int nValue = -1;
	if(pTTY->eID == TTY_DEV_1)
	{
		nValue = eType == TTY_SEND ? 1 : 0;
	}
	else if(pTTY->eID == TTY_DEV_2)
	{
		nValue = eType == TTY_RECV ? 1 : 0;
	}

	/*if(nValue >= 0)
	{
		ioctl(pTTY->fdIOCtrl, nValue, pTTY->nGPIOCtrlNum);
	}*/
#endif
}
