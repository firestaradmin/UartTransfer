/************************************************************************************
*  Copyright (c), 2021, LXG.
*
* FileName		:UartTransfer.c
* Author		:firestaradmin
* Version		:1.1
* Date			:2021年6月16日
* Description	:串口接收BIN文件数据烧录至外部储存器
* History		:
    - Transfer_BIN_TO_ExFlash(UartTransferV10): old version, is deprecated. -2020
    - UartTransferV20: new version, updata its protocol frame format and logic mode. -2021/6/16
*
*
*************************************************************************************
@UartTransferV20:
Frame Format:
    {<Frame head: 2Byte> <Command: 1Byte> <Lenth: 2Byte> <Data: xByte> <BCC: 1Byte> <Frame tail: 2Byte>}
    | Byte Index | Content (HEX)    | Details                                                      |
    | :--------- | :--------------- | ------------------------------------------------------------ |
    | BYTE 1     | 0xC5             | Frame head                                                   |
    | BYTE 2     | 0x5C             | Frame head                                                   |
    | BYTE 3     | <COMMAND>        | Command byte                                                 |
    | BYTE 4     | <LENTH MSB>      | Data lenth MSB uint16_t : 8-15bit                            |
    | BYTE 5     | <LENTH LSB>      | Data lenth LSB uint16_t : 0-7bit                             |
    | ...        | <Data>           | Concrete data (if <LENTH> is zero, the frame can have no data.) |
    | ...        | <Data>           | Concrete data (if <LENTH> is zero, the frame can have no data.) |
    | BYTE N-2   | <BCC Check code> | The XOR of all bytes except the head and the tail of frame and the BCC code self. |
    | BYTE N-1   | 0x5A             | Frame tail                                                   |
    | BYTE N     | 0xA5             | Frame tail                                                   |

-  COMMAND

    Command byte decides the type and the content of the frame.

    | Command byte | Meaning        | note                                                         |
    | ------------ | -------------- | ------------------------------------------------------------ |
    | 0x00         | Data           | Represent the farme is the data of the BIN file which will be transfered. |
    | 0x01         | Transfer begin | Represent the BIN file is going to be transfered.            |
    | 0x02         | Transfer end   | Represent the BIN file is finished to be transfered.         |
    | 0x03         | reserved       |                                                              |
    | ...          | reserved       |                                                              |
    | 0xFF         | ACK of client  | Each time the client receives a frame of data, it needs to return an ACK. |

    ---

-  LENTH

    Lenth is a uint16_t variable, which is the lenth of the data in the frame. Unit: Byte, Big endian.

    If <LENTH> is equal zero, it indicates the frame has no data.

    ---

-  DATA

    Data is the data of the BIN file which will be transfered.

    ---

-  BCC Code

    The BCC code is used to verify the frame received is correct.

    BCC is the XOR of all bytes except the head and the tail of frame and the BCC code self.

    **sample**: a frame is <u>**C5 5C 00 00 04 AA BB CC DD XX 5A A5**</u> , the XX term is BCC Code.

    <u>XX = 00 ^ 00 ^ 04 ^ AA ^ BB ^ CC ^ DD = 0x04</u>

*************************************************************************************/


#include "stm32f1xx_hal.h"
#include "UartTransfer.h"

result_struct UT_framePrasing(void);
void UT_SendACK(FRAME_COMMAND_TYPE cmd, RESULT_TYPE res);
void UT_clearRecvBuf(void);

UT_struct recvStruct;
UT_buffer recvBuf;




void UT_init(
    uint16_t (*p_dataCallback)(uint8_t *dataBuf, uint16_t dataLenth, uint32_t offset),
    uint16_t (*transferCallback)(uint8_t *dataBuf, uint16_t dataLenth) )
{
    recvStruct.transferCallback = transferCallback;
    recvStruct.dataCallback = p_dataCallback;
    recvStruct.offset = 0;
    recvStruct.bytesStored = 0;
    recvStruct.recv_finish = 0;
    recvStruct.recving = 0;
    recvStruct.recving_time1MsCnt = 0;
    recvBuf.recvBuf_tail = 0;
}

//串口处理函数，在串口中断中调用，将接收到的字节传入data
void UT_pushData(uint8_t data)
{  	
	if(recvStruct.recv_finish == 0){
		recvBuf.recvBuf[recvBuf.recvBuf_tail++] = data;		// 存入缓存数组
		recvStruct.recving = 1;                     // 串口 接收标志
		recvStruct.recving_time1MsCnt = 0;	       // 串口接收定时器计数清零	
	}
	if(recvBuf.recvBuf_tail >= sizeof(recvBuf.recvBuf))
	{
		 recvBuf.recvBuf_tail = 0;                 // 防止数据量过大
	}		
	
}

//定时器处理函数，在定时器中断中调用，1Ms一次
void UT_tim_process(void)		//1MS调用一次
{
	/* 串口接收完成判断处理 */
	if(recvStruct.recving)                        	// 如果 usart接收数据标志为1
	{
		recvStruct.recving_time1MsCnt++;             // usart 接收计数	
		if(recvStruct.recving_time1MsCnt > 10)       // 当超过 3 ms 未接收到数据，则认为数据接收完成。
		{
			recvStruct.recv_finish = 1;
			recvStruct.recving = 0;
			recvStruct.recving_time1MsCnt = 0;
		}
	}
	
}

//在主函数中调用，需要一直循环调用，此函数为阻塞函数
void UT_mainFun(void)
{
    while(recvStruct.recv_finish != 1);	// wait until frame received finish
	result_struct ret = UT_framePrasing();
	recvBuf.recvBuf_tail = 0;
	recvStruct.recv_finish = 0;
	UT_SendACK(ret.cmd, ret.ret);
}

result_struct UT_framePrasing(void)
{
    result_struct ret;
    uint16_t length = 0;
	uint8_t cmd = 0;
	uint8_t bcc = 0x00;

	cmd = recvBuf.recvBuf[2];
    ret.cmd = cmd;
    ret.ret = RES_OK;

    if(recvBuf.recvBuf[0] != FRAME_HEAD_1){
        ret.ret = RES_VERIFY_ERR;
        return ret;
    }

	if(recvBuf.recvBuf[1] != FRAME_HEAD_2){
        ret.ret = RES_VERIFY_ERR;
        return ret;
    }
	
	length = recvBuf.recvBuf[3] * 0xFF + recvBuf.recvBuf[4];
	
    // Calc. BCC Code
	for(uint16_t i = 2; i < 5 + length; i ++){
		bcc ^= recvBuf.recvBuf[i];
	}
	
	if(bcc != recvBuf.recvBuf[5 + length]){
        ret.ret = RES_VERIFY_ERR;
        return ret;
    }
	
	if(recvBuf.recvBuf[6 + length] != FRAME_TAIL_1){
        ret.ret = RES_VERIFY_ERR;
        return ret;
    }
	if(recvBuf.recvBuf[7 + length] != FRAME_TAIL_2){
        ret.ret = RES_VERIFY_ERR;
        return ret;
    }
	
	
	if(cmd == CMD_DATA) // data frame 
	{
        uint16_t resLen = recvStruct.dataCallback(recvBuf.recvBuf + 5, length, recvStruct.offset + recvStruct.bytesStored);
        if(resLen != length)
            ret.ret = RES_STORAGE_FULL;
        else
            recvStruct.bytesStored += resLen;
    }
	else if(cmd == CMD_BEGIN) // begin frame
	{
        uint32_t *offset = recvBuf.recvBuf + 5;
        recvStruct.bytesStored = 0;
        recvStruct.offset = *offset;
    }
	else if(cmd == CMD_END) // end frame, u can do sth when transfer over.
	{
	
	}else {
		
	}
	
	
	return ret;
	
}

void UT_SendACK(FRAME_COMMAND_TYPE cmd, RESULT_TYPE res)
{
	uint8_t sendBuf[10] = {FRAME_HEAD_1, FRAME_HEAD_2, CMD_ACK, 0x00, 0x02, cmd, res, 0x00, FRAME_TAIL_1, FRAME_TAIL_2};
	sendBuf[7] = sendBuf[2] ^ sendBuf[3] ^ sendBuf[4] ^ sendBuf[5] ^ sendBuf[6] ; 
	
	recvStruct.transferCallback(sendBuf, sizeof(sendBuf));
}

void UT_clearRecvBuf(void)
{
	while(recvBuf.recvBuf_tail--)
	{
		recvBuf.recvBuf[recvBuf.recvBuf_tail] = 0;
	}
	
}











