
#ifndef __UART_TRANSFER_
#define __UART_TRANSFER_

/* MAX Transfer lenth */
#define MAX_TRANSFER_DATA_LENTH_BYTES 1000


/* FRAME Macro */
typedef enum FRAME_STUCTURE
{
    FRAME_HEAD_1 = 0xC5,
    FRAME_HEAD_2 = 0x5C,
    FRAME_TAIL_1 = 0x5A,
    FRAME_TAIL_2 = 0xA5
}FRAME_STUCTURE;

typedef enum FRAME_COMMAND_TYPE
{
    CMD_DATA = 0x00,
    CMD_BEGIN = 0x01,
    CMD_END = 0x02,
    CMD_ACK = 0xFF
}FRAME_COMMAND_TYPE;

typedef enum RESULT_TYPE
{
    RES_OK = 0x00,
    RES_VERIFY_ERR = 0x01,
    RES_STORAGE_FULL = 0x02,
    RES_UNKONW_ERR = 0xFF
}RESULT_TYPE;

typedef struct result_struct{
    FRAME_COMMAND_TYPE cmd;
    RESULT_TYPE ret;
}result_struct;

typedef struct UT_buffer{
    uint8_t recvBuf[MAX_TRANSFER_DATA_LENTH_BYTES + 8];	//receive buf. A frame has 8 bytes at least.
    uint16_t recvBuf_tail;
}UT_buffer;

typedef struct UT_struct{
    /* func for transfer frame to server. eaxmple UART. !!!! Need to return dataLength !!!! */
    uint16_t (*transferCallback)(uint8_t *dataBuf, uint16_t dataLenth);
    /* func for wirte data to flash. !!!! Need to return dataLength !!!! */
    uint16_t (*dataCallback)(uint8_t *dataBuf, uint16_t dataLenth, uint32_t offset);
    uint32_t offset;            // offset of data will be stored in flash or other mem.
    uint32_t bytesStored;       // data already stored bytes
    uint32_t recv_finish : 1;   // receive finish flag
    uint32_t recving : 1;       // receving flag
    uint32_t recving_time1MsCnt : 8;    // the time for justice receive finish
    uint32_t reserved : 22;

}UT_struct;





/* func */

/** @brief Init UartTransfer Lib, mainly for register two func.
 * @param p_dataCallback func for wirte data to flash. !!!! Need to return dataLength !!!!
 * @param transferCallback func for transfer frame to server. eaxmple UART. !!!! Need to return dataLength !!!!
 * @author LXG.firestaradmin
*/
void UT_init(
    uint16_t (*p_dataCallback)(uint8_t *dataBuf, uint16_t dataLenth, uint32_t offset),
    uint16_t (*transferCallback)(uint8_t *dataBuf, uint16_t dataLenth));
void UT_pushData(uint8_t data);//串口处理函数，在串口中断中调用，将接收到的字节传入data
void UT_tim_process(void);  //1MS调用一次;
void UT_mainFun(void);      //在主函数中调用，需要一直循环调用
#endif


