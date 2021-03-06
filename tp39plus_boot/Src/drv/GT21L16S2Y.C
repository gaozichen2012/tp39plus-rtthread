#include "GT21L16S2Y.H"
#include "stm32f0xx_gpio.h"
#include "stm32f0xx_rcc.h"
#include "string.h"
#include "stdio.h"


//CS, MI, SCL
void spi_gpio_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHBPeriphClockCmd( RCC_AHBPeriph_GPIOB, ENABLE); 	
	GPIO_InitStructure.GPIO_Pin = SPI_SCLK_PIN ;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;   
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_Init(SPI_SCLK_PORT, &GPIO_InitStructure);		  
	
	RCC_AHBPeriphClockCmd( RCC_AHBPeriph_GPIOB, ENABLE); 	
	GPIO_InitStructure.GPIO_Pin = SPI_MOSI_PIN ;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;   
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_Init(SPI_MOSI_PORT, &GPIO_InitStructure);		
	
	RCC_AHBPeriphClockCmd( RCC_AHBPeriph_GPIOA, ENABLE); 	
	GPIO_InitStructure.GPIO_Pin = SPI_CS_PIN ;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;   
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_Init(SPI_CS_PORT, &GPIO_InitStructure);		 

	RCC_AHBPeriphClockCmd( RCC_AHBPeriph_GPIOB, ENABLE); 	
	GPIO_InitStructure.GPIO_Pin = SPI_MISO_PIN ;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;   
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_Init(SPI_MISO_PORT, &GPIO_InitStructure);		 

}  

//读取GT21L16S数据
//addr& 00ffffff  3个byte  
//datlen 读取长度
//读取时序为 首字节 03普通读取，后三个字节为地址
static void gt21l16s2_read_addr(uint32_t addr, uint8_t * dat, uint8_t datlen)
{
	uint8_t i,j;
	addr &= 0x00ffffff;
	addr |= 0x03000000;
	memset(dat, 0, datlen);
	SPI_CS_LOW();
	
	for(i=0; i<32; i++)//发送bit数据
	{
		if(addr & (0x80000000 >> i))
			SPI_MOSI_HIGHT();
		else
			SPI_MOSI_LOW();
		SPI_CLK_LOW(); 
		SPI_CLK_HIGHT(); //scl上升沿数据被GT21L锁定		
	}
	//开始读数据， scl下降沿被移出
	for(i=0; i<datlen; i++)
	{
		for(j=0;j<8;j++)
		{
			SPI_CLK_HIGHT();
			SPI_CLK_LOW(); 
			if(SPI_MISO_READ())
				*(dat+i) = *(dat+i) | (0x01<<(7-j));
			else
				*(dat+i) =  *(dat+i) & ~(0x01<<(7-j));		
		}			
	}	
	
	SPI_CS_HIGHT();
}




void gt21l16s2_init(void)
{
	uint16_t tmp;
	spi_gpio_init();
	tmp = gb2312_to_unicode(0xe2b2);
	tmp = tmp;
}


uint8_t gt21l16s2_get_gb2312_15x16(uint8_t *out_dat, uint8_t* dat_gb2312)
{
	uint32_t addr;
	uint8_t MSB, LSB;
	MSB = *(uint8_t*)dat_gb2312;
	LSB = *(uint8_t*)(dat_gb2312+1);
	if(MSB>=0XA4 && MSB<=0XA8 && LSB>=0XA1)
		addr = ADDR_BASE_15X16;
	else if(MSB>=0XA1 && MSB<=0XA9 && LSB>=0XA1)
		addr = ((MSB - 0xa1)*94 + (LSB-0XA1))*32 + ADDR_BASE_15X16;
	else if(MSB >= 0XB0 && MSB<=0XF7 && LSB>=0XA1)
		addr = ((MSB-0XB0)*94 + (LSB - 0XA1)+846)*32 + ADDR_BASE_15X16;
	else
	{
		if(gt21l16s2_get_ascii_8x16(out_dat, * dat_gb2312))
		{
			if(gt21l16s2_get_ascii_8x16(out_dat+16, * (dat_gb2312+1)))
				return 1;
		}
		else
			return 0;
	}
	gt21l16s2_read_addr(addr, out_dat, 32);
	return 1;
}

//输入ascii码，返回8*16点阵信息
uint8_t gt21l16s2_get_ascii_8x16(uint8_t *out_dat, uint8_t ascii)
{
	uint32_t addr;

	if(ascii >= 0x20 && ascii <= 0x7E)
		addr = (ascii - 0X20)*16 +ADDR_ASCII_8X16;
	else
		return 0;
	gt21l16s2_read_addr(addr, out_dat, 16);
	return 1;
}


uint8_t gt21l16s2_get_gb2312_11x12(uint8_t *out_dat, uint8_t* dat_gb2312)
{
	uint32_t addr;
	uint8_t MSB, LSB;
	MSB = *(uint8_t*)dat_gb2312;
	LSB = *(uint8_t*)(dat_gb2312+1);
	if(MSB>=0XA1 && MSB<=0XA3 && LSB>=0XA1)
		addr = ((MSB - 0xa1)*94 + (LSB-0XA1))*24 + ADDR_BASE_11X16;
	else if(MSB==0XA9 && LSB>=0XA1)
		addr = (282+(LSB - 0XA1))*24 + ADDR_BASE_11X16;
	else if(MSB >= 0XB0 && MSB<=0XF7 && LSB>=0XA1)
		addr = ((MSB-0XB0)*94 + (LSB - 0XA1)+376)*24 + ADDR_BASE_11X16;
	else
	{
		if(gt21l16s2_get_ascii_8x16(out_dat, * dat_gb2312))
		{
			if(gt21l16s2_get_ascii_8x16(out_dat+16, * (dat_gb2312+1)))
				return 1;
		}
		else
			return 0;
	}
	gt21l16s2_read_addr(addr, out_dat, 24);
	return 1;
}

//输入ascii码，返回8*16点阵信息
uint8_t gt21l16s2_get_ascii_6x12(uint8_t *out_dat, uint8_t ascii)
{
	uint32_t addr;

	if(ascii >= 0x20 && ascii <= 0x7E)
		addr = (ascii - 0X20)*12+ADDR_ASCII_8X12;
	else
		return 0;
	gt21l16s2_read_addr(addr, out_dat, 12);
	return 1;
}




uint16_t unicode_to_gb2312(uint16_t UCSData)
{

	uint8_t result = 0;
	uint16_t gbcode;
	int h;
	if(UCSData<0xa0) result=1;
	else if(UCSData<=0xf7) h=UCSData-160;	// 160
	else if(UCSData<0x2c7) result=1;
	else if(UCSData<=0x2c9) h=UCSData-160-463;	//623
	else if(UCSData<0x2010) result=1;
	else if(UCSData<=0x2312) h=UCSData-160-463-7494;	//8117
	else if(UCSData<0x2460) result=1;	
	else if(UCSData<=0x2642) h=UCSData-160-463-7494-333;	//8450
	else if(UCSData<0x3000) result=1;
	else if(UCSData<=0x3017) h=UCSData-160-463-7494-333-2493;	//10943
	else if(UCSData<0x3220) result=1;
	else if(UCSData<=0x3229) h=UCSData-160-463-7494-333-2493-520;	//11463
	else if(UCSData<0x4e00) result=1;
	else if(UCSData<=0x9b54) h=UCSData-160-463-7494-333-2493-520-7126;	//18589
	else if(UCSData<0x9c7c) result=1;
	else if(UCSData<=0x9ce2) h=UCSData-160-463-7494-333-2493-520-7126-295;	//18884
	else if(UCSData<0x9e1f) result=1;
	else if(UCSData<=0x9fa0) h=UCSData-160-463-7494-333-2493-520-7126-295-316;	//19200
	else if(UCSData<0xe76c) result=1;
	else if(UCSData<=0xe774) h=UCSData-160-463-7494-333-2493-520-7126-295-316-18379;	//37579
	else if(UCSData<0xff00) result=1;
	else if(UCSData<=0xff5f) h=UCSData-160-463-7494-333-2493-520-7126-295-316-18379-6027;	//43606
	else if(UCSData<0xffe0) result=1;
	else if(UCSData<=0xffe5) h=UCSData-160-463-7494-333-2493-520-7126-295-316-18379-6027-128;	//43734
	else result=1;

	if(result==0)
	{
		gt21l16s2_read_addr((uint32_t)ADDR_UNICODE_TO_GB2312 + (h<<1),(uint8_t*)&gbcode,2);
		gbcode = (gbcode>>8)+(gbcode<<8);
	}
	else
	{
		gbcode = 0x0000;
	}
	//转换成小端模式
	result = (gbcode>>8)&0xff;
	gbcode = ((gbcode<<8)&0xff00) | result;
	return gbcode;

}


//GB2UCSAddr_Start //输入小端模式
uint16_t gb2312_to_unicode(uint16_t GBData)
{
	uint8_t codeH= 0;
	uint8_t codeL= 0;
	uint32_t index;
	uint16_t ucode;
	codeH = GBData >> 8;  //输入小端模式
	codeL = GBData & 0x00ff;
	if((codeH>=0xA1) && (codeH<=0xa3) && (codeL >=0xA1))
	{
		index=(codeH - 0xA1) * 94 + (codeL - 0xA1);
	}
	else if((codeH==0xA9) && (codeL >=0xA1) && (codeL<=0xf6))
	{
		index = 282 + (codeL - 0xA1);
	}
	else if((codeH>=0xb0) && (codeH<=0xf7) && (codeL >=0xA1))
	{
		index = (codeH - 0xB0) * 94 + (codeL - 0xA1)+ 368;
	}
	else
	{
		index=0;
	}
	gt21l16s2_read_addr((uint32_t)ADDR_GB2312_TO_UNICODE + (index<<1),(uint8_t*)&ucode,2);
	ucode = (ucode>>8)+(ucode<<8);
	return ucode;
}





