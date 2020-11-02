/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-08-09     imgcr       the first version
 */
#ifndef APPLICATIONS_RC522_H_
#define APPLICATIONS_RC522_H_

#include <stm32f1xx_hal.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <rthw.h>
#include <functional>

typedef rt_uint8_t u8;
typedef rt_uint32_t u32;

/////////////////////////////////////////////////////////////////////
//MF522命令字
/////////////////////////////////////////////////////////////////////
#define PCD_IDLE              0x00               //取消当前命令
#define PCD_AUTHENT           0x0E               //验证密钥
#define PCD_RECEIVE           0x08               //接收数据
#define PCD_TRANSMIT          0x04               //发送数据
#define PCD_TRANSCEIVE        0x0C               //发送并接收数据
#define PCD_RESETPHASE        0x0F               //复位
#define PCD_CALCCRC           0x03               //CRC计算

/////////////////////////////////////////////////////////////////////
//Mifare_One卡片命令字
/////////////////////////////////////////////////////////////////////
#define PICC_REQIDL           0x26               //寻天线区内未进入休眠状态
#define PICC_REQALL           0x52               //寻天线区内全部卡
#define PICC_ANTICOLL1        0x93               //防冲撞
#define PICC_ANTICOLL2        0x95               //防冲撞
#define PICC_AUTHENT1A        0x60               //验证A密钥
#define PICC_AUTHENT1B        0x61               //验证B密钥
#define PICC_READ             0x30               //读块
#define PICC_WRITE            0xA0               //写块
#define PICC_DECREMENT        0xC0               //扣款
#define PICC_INCREMENT        0xC1               //充值
#define PICC_RESTORE          0xC2               //调块数据到缓冲区
#define PICC_TRANSFER         0xB0               //保存缓冲区中数据
#define PICC_HALT             0x50               //休眠

/////////////////////////////////////////////////////////////////////
//MF522 FIFO长度定义
/////////////////////////////////////////////////////////////////////
#define DEF_FIFO_LENGTH       64                 //FIFO size=64byte
#define MAXRLEN  18

/////////////////////////////////////////////////////////////////////
//MF522寄存器定义
/////////////////////////////////////////////////////////////////////
// PAGE 0
#define     RFU00                 0x00
#define     CommandReg            0x01
#define     ComIEnReg             0x02
#define     DivlEnReg             0x03
#define     ComIrqReg             0x04
#define     DivIrqReg             0x05
#define     ErrorReg              0x06
#define     Status1Reg            0x07
#define     Status2Reg            0x08
#define     FIFODataReg           0x09
#define     FIFOLevelReg          0x0A
#define     WaterLevelReg         0x0B
#define     ControlReg            0x0C
#define     BitFramingReg         0x0D
#define     CollReg               0x0E
#define     RFU0F                 0x0F
// PAGE 1
#define     RFU10                 0x10
#define     ModeReg               0x11
#define     TxModeReg             0x12
#define     RxModeReg             0x13
#define     TxControlReg          0x14
#define     TxAutoReg             0x15
#define     TxSelReg              0x16
#define     RxSelReg              0x17
#define     RxThresholdReg        0x18
#define     DemodReg              0x19
#define     RFU1A                 0x1A
#define     RFU1B                 0x1B
#define     MifareReg             0x1C
#define     RFU1D                 0x1D
#define     RFU1E                 0x1E
#define     SerialSpeedReg        0x1F
// PAGE 2
#define     RFU20                 0x20
#define     CRCResultRegM         0x21
#define     CRCResultRegL         0x22
#define     RFU23                 0x23
#define     ModWidthReg           0x24
#define     RFU25                 0x25
#define     RFCfgReg              0x26
#define     GsNReg                0x27
#define     CWGsCfgReg            0x28
#define     ModGsCfgReg           0x29
#define     TModeReg              0x2A
#define     TPrescalerReg         0x2B
#define     TReloadRegH           0x2C
#define     TReloadRegL           0x2D
#define     TCounterValueRegH     0x2E
#define     TCounterValueRegL     0x2F
// PAGE 3
#define     RFU30                 0x30
#define     TestSel1Reg           0x31
#define     TestSel2Reg           0x32
#define     TestPinEnReg          0x33
#define     TestPinValueReg       0x34
#define     TestBusReg            0x35
#define     AutoTestReg           0x36
#define     VersionReg            0x37
#define     AnalogTestReg         0x38
#define     TestDAC1Reg           0x39
#define     TestDAC2Reg           0x3A
#define     TestADCReg            0x3B
#define     RFU3C                 0x3C
#define     RFU3D                 0x3D
#define     RFU3E                 0x3E
#define     RFU3F                 0x3F

/////////////////////////////////////////////////////////////////////
//和MF522通讯时返回的错误代码
/////////////////////////////////////////////////////////////////////
#define     MI_OK                 0
#define     MI_NOTAGERR           (1)
#define     MI_ERR                (2)

#define SHAQU1  0X01
#define KUAI4   0X04
#define KUAI7   0X07
#define REGCARD 0xa1
#define CONSUME 0xa2
#define READCARD    0xa3
#define ADDMONEY    0xa4

//
//#define  spi_cs 1;
//sbit  spi_ck=P0^6;
//sbit  spi_mosi=P0^7;
//sbit  spi_miso=P4^1;
//sbit  spi_rst=P2^7;
#define SPIReadByte()   SPIWriteByte(0)
u8 SPIWriteByte(u8 byte);
void SPI1_Init(void);
//void SPI2_Init(void);

#define SET_SPI_CS  (GPIOF->BSRR=0X01)
#define CLR_SPI_CS  (GPIOF->BRR=0X01)



#define SET_RC522RST  GPIOF->BSRR=0X02
#define CLR_RC522RST  GPIOF->BRR=0X02


/***********************RC522 函数宏定义**********************/
#define          RC522_CS_Enable()         rt_pin_write(4, PIN_LOW)
#define          RC522_CS_Disable()        rt_pin_write(4, PIN_HIGH)

#define          RC522_Reset_Enable()      rt_pin_write(8, PIN_LOW)
#define          RC522_Reset_Disable()     rt_pin_write(8, PIN_HIGH)

#define          RC522_SCK_0()             rt_pin_write(5, PIN_LOW)
#define          RC522_SCK_1()             rt_pin_write(5, PIN_HIGH)

#define          RC522_MOSI_0()            rt_pin_write(7, PIN_LOW)
#define          RC522_MOSI_1()            rt_pin_write(7, PIN_HIGH)

#define          RC522_MISO_GET()          rt_pin_read(6)

void             RC522_Handel               (void);
int             RC522_Init                 ( void );                       //初始化
void             PcdReset                   ( void );                       //复位
void             M500PcdConfigISOType       ( u8 type );                    //工作方式
char             PcdRequest                 ( u8 req_code, u8 * pTagType ); //寻卡
char             PcdAnticoll                ( u8 * pSnr);                   //读卡号

char             PcdSelect                  ( u8 * pSnr );
char             PcdAuthState               ( u8 ucAuth_mode, u8 ucAddr, u8 * pKey, u8 * pSnr );
char             PcdWrite                   ( u8 ucAddr, u8 * pData );
char             PcdRead                    ( u8 ucAddr, u8 * pData );
void ShowID(u8 *p);  //显示卡的卡号，以十六进制显示

extern char* POINT_LNG;
extern char* POINT_LAT;
extern char* POINT_LNG_ON;
extern char* POINT_LAT_ON;
extern char* POINT_LNG_OFF;
extern char* POINT_LAT_OFF;


/***********************RC522 参数配置**********************/
#define RC522_SPI_GPIO GPIOA


struct Rc522 {
    friend void RC522_Handel(void);
    void onCardInserted(std::function<void(rt_uint32_t)> cb) {
        onCardInsertedCb = cb;
    }

    std::function<void(rt_uint32_t)> onCardInsertedCb;
};

extern Rc522 rc522;

#endif /* APPLICATIONS_RC522_H_ */