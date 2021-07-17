

# UartTransferV20 Design Sheet

> STM32串口烧录BIN/HEX文件 设计书

> @author firestaradmin 2021年6月16日

[TOC]

## 0 Brief

<u>**UartTransfer Lib**</u> is for transfering data by UART, and it is all programmed in C.

Can use in embeded-device or other C application.

## 1 Protocol Frame Format

### 1.1 Frame Format

{<Frame head: 2Byte> <Command: 1Byte> <Lenth: 2Byte> <Data: xByte> <BCC: 1Byte> <Frame tail: 2Byte>}

| Byte Index | Content (HEX)    | Details                                                      |
| :--------- | :--------------- | ------------------------------------------------------------ |
| BYTE 1     | 0xC5             | Frame head                                                   |
| BYTE 2     | 0x5C             | Frame head                                                   |
| BYTE 3     | <COMMAND>        | Command byte                                                 |
| BYTE 4     | <LENTH MSB>      | Data lenth MSB [uint16_t : 8-15bit]                          |
| BYTE 5     | <LENTH LSB>      | Data lenth LSB [uint16_t : 0-7bit]                           |
| ...        | <Data>           | Concrete data (if <LENTH> is zero, the frame can have no data.) |
| ...        | <Data>           | Concrete data (if <LENTH> is zero, the frame can have no data.) |
| BYTE N-2   | <BCC Check code> | The XOR of all bytes except the head and the tail of frame and the BCC code self. |
| BYTE N-1   | 0x5A             | Frame tail                                                   |
| BYTE N     | 0xA5             | Frame tail                                                   |

- #### COMMAND

  Command byte decides the type and the content of the frame.

  | Command byte | Meaning        | note                                                         |
  | ------------ | -------------- | ------------------------------------------------------------ |
  | 0x00         | Data           | Represent the <Data> contnet of the farme is the data of the BIN file which will be transfered. |
  | 0x01         | Transfer begin | Represent the BIN file is going to be transfered.            |
  | 0x02         | Transfer end   | Represent the BIN file is finished to be transfered.         |
  | 0x03         | reserved       |                                                              |
  | ...          | reserved       |                                                              |
  | 0xFF         | ACK of client  | Each time the client receives a frame of data, it needs to return an ACK. |

  ---

- #### LENTH

    Lenth is a uint16_t variable, which is the lenth of the data in the frame. Unit: Byte, Big endian.

    If <LENTH> is equal zero, it indicates the frame has no data.

    ---

- #### DATA

    Data is the data of the BIN file which will be transfered.

    ---

- #### BCC Code

    The BCC code is used to verify the frame received is correct.

    BCC is the XOR of all bytes except the head and the tail of frame and the BCC code self.

    **sample**: a frame is <u>**C5 5C 00 00 04 AA BB CC DD XX 5A A5**</u> , the XX term is BCC Code.

    <u>XX = 00 ^ 00 ^ 04 ^ AA ^ BB ^ CC ^ DD = 0x04</u>

---

### 1.2 Begin Frame Sample

Begin Frame has a 4 bytes data which indicate that the offset position of the BIN file will be stored in. Unit: Byte, Big endian.

**sample**: The file is to be stored at 0xF00 offset position of SPI-Flash.

The Frame: <u>**C5 5C 01 00 04 00 00 0F 00 0A 5A A5**</u>

---

### 1.3 End Frame Sample

End Frame has no data, so the <LENTH> of the frame is 0.

**sample**: Indicate the file has been transfered.

The Frame: <u>**C5 5C 02 00 00 02 5A A5**</u>

---

### 1.4 Transfer data Frame Sample

Data Frame has uncertain lenth data. 

**smaple**:  Transfer 10 Byte data: [ <u>01 02 03 04 05 06 07 08 09 0A</u> ]

The Frame: <u>**C5 5C 00 00 0A 01 02 03 04 05 06 07 08 09 0A 0B  5A A5**</u>

---

### 1.5 ACK Frame Sample

When the client has received a frame, it need to return an ACK Frame.

The <COMMAND> of ACK Frame is 0xFF, and the <LENTH> is fixed at 0x0002 that indicates data just two byte. 

The first byte of data is COMMAND which be responded, and the second byte of data is processing result (<PROCESS RESULT>).

- ##### \<PROCESS RESULT\> Table

    | \<PROCESS RESULT\> | describe                    |
    | ------------------ | --------------------------- |
    | 0x00               | OK                          |
    | 0x01               | Frame verify error          |
    | 0x02               | Internal storage full error |
    | ...                | Reserved                    |
    | 0xFF               | Unkown error                |

**sample**:  The ACK to Transfer begin command, and the client processs it OK.

The Frame: <u>**C5 5C FF 00 02 01 00 FC 5A A5**</u>


