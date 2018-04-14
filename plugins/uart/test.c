#include "TTYIO.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main()
{
	
	TTY_INFO *info = TTY_Ready(TTY_DEV_0); 
	TTY_SetParity(info, 8, 'n', 1, 115200);
	printf("init\n");
	unsigned char buf[1024];
	char receiveBuf[1024];
	int receivenum = 0;
	int i = 0;
	int ret;
	unsigned char num = 0;
	printf("init1\n");
	for (i = 0;i< 1024;i++)
	{
		
		buf[i]= num +0x31;
		num++;
		if(num==0xff) num = 0;
	}
	memset(receiveBuf,0,1024);
	ret = 16;
	printf("你好\n");
	
	buf[10] = '\n';
	TTY_Sendn(info, (char *)buf, 10);
	
	while(1)
	{
		
		
		// // receivenum = TTY_RecvNB(info, receiveBuf, 1024);
		// // for (i = 0;i<receivenum;i++)
		// // {
			// // fprintf(stdout,"%c",receiveBuf[i]);
		// // }
		
		// // if (receivenum > 0)
		// // {
			// // TTY_Sendn(info, (char *)receiveBuf, receivenum);
			// // /*ret = ret << 1;	
			// // if (ret > 1024) ret = 16;*/
		// // }
		
		buf[10] = '\n';
		TTY_Sendn(info, (char *)buf, 10);
		
		// //receivenum = TTY_RecvNB(info, receiveBuf, 1024);
		// // if(receivenum > 0)
		// // {
			// // printf("recv\n");
			// // TTY_Sendn(info, (char *)buf, receivenum);
			// // receivenum = 0;
		// // };
		
		
	}
	
	return 0;
}
