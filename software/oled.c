/*
 * oled.c - SSD1306 SPI OLED driver for STM32F373 Seq2
 * 11-18-2018 E. Brombaugh
 */

#include <stdlib.h>
#include <string.h>
#include "oled.h"
#include "font_8x8.h"
#include "arial_24_bold_32_numpunct.h"
#include "printf.h"

#define SSD1306_SETCONTRAST 0x81
#define SSD1306_SEGREMAP 0xA0
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON 0xA5
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_INVERTDISPLAY 0xA7
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF
#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS 0xDA
#define SSD1306_SETVCOMDETECT 0xDB
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE 0xD9
#define SSD1306_SETMULTIPLEX 0xA8
#define SSD1306_SETLOWCOLUMN 0x00
#define SSD1306_SETHIGHCOLUMN 0x10
#define SSD1306_SETSTARTLINE 0x40
#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR   0x22
#define SSD1306_COMSCANINC 0xC0
#define SSD1306_COMSCANDEC 0xC8
#define SSD1306_CHARGEPUMP 0x8D
#define SSD1306_EXTERNALVCC 0x1
#define SSD1306_SWITCHCAPVCC 0x2

/* choose VCC mode */
#define SSD1306_EXTERNALVCC 0x1
#define SSD1306_SWITCHCAPVCC 0x2
//#define vccstate SSD1306_EXTERNALVCC
#define vccstate SSD1306_SWITCHCAPVCC

/* LCD frame buffer */
uint8_t oled_buffer[OLED_MAXBUFS][OLED_BUFSZ];

/* macros to define pins */
#define OLED_CS_PIN                        GPIO_PIN_2
#define OLED_CS_GPIO_PORT                  GPIOB
#define OLED_DC_PIN                        GPIO_PIN_9
#define OLED_DC_GPIO_PORT                  GPIOE
#define OLED_RST_PIN                       GPIO_PIN_8
#define OLED_RST_GPIO_PORT                 GPIOE

/* macros for GPIO setting */
#define OLED_CS_LOW()     HAL_GPIO_WritePin(OLED_CS_GPIO_PORT,OLED_CS_PIN,GPIO_PIN_RESET)
#define OLED_CS_HIGH()    HAL_GPIO_WritePin(OLED_CS_GPIO_PORT,OLED_CS_PIN,GPIO_PIN_SET)
#define OLED_DC_LOW()     HAL_GPIO_WritePin(OLED_DC_GPIO_PORT,OLED_DC_PIN,GPIO_PIN_RESET)
#define OLED_DC_HIGH()    HAL_GPIO_WritePin(OLED_DC_GPIO_PORT,OLED_DC_PIN,GPIO_PIN_SET)
#define OLED_RST_LOW()    HAL_GPIO_WritePin(OLED_RST_GPIO_PORT,OLED_RST_PIN,GPIO_PIN_RESET)
#define OLED_RST_HIGH()   HAL_GPIO_WritePin(OLED_RST_GPIO_PORT,OLED_RST_PIN,GPIO_PIN_SET)

/* SPI port handle */
static SPI_HandleTypeDef SpiHandle;

/*
 * initialize the SPI comm port
 */
void oled_comm_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	/* Enable SCK, MOSI  GPIO clocks */
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();

	/* Configure SPI1 CLK as alternate function push-pull on PA12 */
	GPIO_InitStructure.Pin = GPIO_PIN_12;
	GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Alternate = GPIO_AF6_SPI1;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Configure SPI1 MOSI as alternate function push-pull on PF6 */
	GPIO_InitStructure.Pin = GPIO_PIN_6;
	GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Alternate = GPIO_AF5_SPI1;
	HAL_GPIO_Init(GPIOF, &GPIO_InitStructure);

	/* Enable NRST, CS, DC GPIO clocks */
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();

	/* Setup GPIO control bits */
	OLED_DC_LOW();
	OLED_RST_HIGH();
	OLED_CS_HIGH();
	
	/* CS pin configuration */
	GPIO_InitStructure.Pin =  GPIO_PIN_2;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStructure.Pull = GPIO_NOPULL ;
	HAL_GPIO_Init(OLED_CS_GPIO_PORT, &GPIO_InitStructure);
  
	/* DC, RST pin configuration */
	GPIO_InitStructure.Pin =  GPIO_PIN_8|GPIO_PIN_9;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStructure.Pull = GPIO_NOPULL ;
	HAL_GPIO_Init(OLED_DC_GPIO_PORT, &GPIO_InitStructure);
  
	/* Enable the SPI peripheral */
    __HAL_RCC_SPI1_CLK_ENABLE();
    
	SpiHandle.Instance               = SPI1;
	SpiHandle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
	SpiHandle.Init.Direction         = SPI_DIRECTION_2LINES;
	SpiHandle.Init.CLKPhase          = SPI_PHASE_1EDGE;
	SpiHandle.Init.CLKPolarity       = SPI_POLARITY_LOW;
	SpiHandle.Init.DataSize          = SPI_DATASIZE_8BIT;
	SpiHandle.Init.FirstBit          = SPI_FIRSTBIT_MSB;
	SpiHandle.Init.TIMode            = SPI_TIMODE_DISABLE;
	SpiHandle.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
	SpiHandle.Init.CRCPolynomial     = 7;
	SpiHandle.Init.NSS               = SPI_NSS_SOFT;
	SpiHandle.Init.Mode              = SPI_MODE_MASTER;
	HAL_SPI_Init(&SpiHandle);

	/* Enable the SPI peripheral */
    __HAL_SPI_ENABLE(&SpiHandle);
}

/*
 * send a byte via spi
 */
void spi_tx(uint8_t data)
{
    uint8_t dummy __attribute__ ((unused));
    
	/* Wait until the transmit buffer is empty */
	while(__HAL_SPI_GET_FLAG(&SpiHandle, SPI_FLAG_TXE) == RESET)
	{
	}

	/* Send the byte */
	*(__IO uint8_t *) ((uint32_t)SPI1+0x0C) = data;
	
	/* Wait to receive a byte*/
	while(__HAL_SPI_GET_FLAG(&SpiHandle, SPI_FLAG_RXNE) == RESET)
	{
	}

	/* get the byte read from the SPI bus */ 
	dummy = *(__IO uint8_t *) ((uint32_t)SPI1+0x0C);
}

/*
 * Send a command byte to the OLED via SPI
 */
uint32_t oled_command(uint8_t cmd)
{
	/* set DC low for command */
	OLED_DC_LOW();
	
	/* set CS low */
	OLED_CS_LOW();
	
	/* send a byte */
	spi_tx(cmd);
	
	/* set CS high */
	OLED_CS_HIGH();
    
    return 0;
}

/*
 * Send a block of data bytes to the OLED via SPI
 */
uint32_t oled_data(uint8_t *data, uint8_t sz)
{
	/* set DC high for data */
	OLED_DC_HIGH();
	
	/* set CS low */
	OLED_CS_LOW();
	
    /* send all bytes */
	while(sz--)
    {
        spi_tx(*data++);
	}
    
	/* set CS high */
	OLED_CS_HIGH();
    
    return 0;
}

/*
 * Initialize the SSD1306
 */
void oled_init(void)
{
    /* initialize the comm port */
    oled_comm_init();
    
    /* Toggle OLED Reset */
    OLED_RST_LOW();
    HAL_Delay(10);
    OLED_RST_HIGH();
	
	/* clear the frame buffer */
	oled_clear(0,0);
		
	/* Init the OLED controller for 128x64 */
    oled_command(SSD1306_DISPLAYOFF);                    // 0xAE
    oled_command(SSD1306_SETDISPLAYCLOCKDIV);            // 0xD5
    oled_command(0x80);                                  // the suggested ratio 0x80
    oled_command(SSD1306_SETMULTIPLEX);                  // 0xA8
    oled_command(0x3F);
    oled_command(SSD1306_SETDISPLAYOFFSET);              // 0xD3
    oled_command(0x0);                                   // no offset
    oled_command(SSD1306_SETSTARTLINE | 0x0);            // line #0
    oled_command(SSD1306_CHARGEPUMP);                    // 0x8D
    if (vccstate == SSD1306_EXTERNALVCC) 
		oled_command(0x10);
    else 
		oled_command(0x14);
    oled_command(SSD1306_MEMORYMODE);                    // 0x20
    oled_command(0x00);                                  // 0x0 act like ks0108
    oled_command(SSD1306_SEGREMAP | 0x1);
    oled_command(SSD1306_COMSCANDEC);
    oled_command(SSD1306_SETCOMPINS);                    // 0xDA
    oled_command(0x12);
    oled_command(SSD1306_SETCONTRAST);                   // 0x81
    if (vccstate == SSD1306_EXTERNALVCC) 
		oled_command(0x9F);
    else 
		oled_command(0x8F);
    oled_command(SSD1306_SETPRECHARGE);                  // 0xd9
    if (vccstate == SSD1306_EXTERNALVCC) 
		oled_command(0x22);
    else 
		oled_command(0xF1);
    oled_command(SSD1306_SETVCOMDETECT);                 // 0xDB
    oled_command(0x40);
    oled_command(SSD1306_DISPLAYALLON_RESUME);           // 0xA4
    oled_command(SSD1306_NORMALDISPLAY);                 // 0xA6
	
	oled_command(SSD1306_DISPLAYON);	                 // 0xAF --turn on oled panel
	
	/* update the display */
	oled_refresh(0);
}

/*
 * Get address of the frame buffer
 */
uint8_t *oled_get_fb(uint8_t buf_num)
{
	return oled_buffer[buf_num];
}

/*
 * Copy buffer
 */
void oled_cpy_buf(uint8_t dst_num, uint8_t src_num)
{
	memcpy(oled_buffer[dst_num], oled_buffer[src_num], OLED_BUFSZ);
}

/*
 * Send the frame buffer
 */
void oled_refresh(uint8_t buf_num)
{
	uint16_t i;
	
	oled_command(SSD1306_COLUMNADDR);
	oled_command(0);   // Column start address (0 = reset)
	oled_command(OLED_W-1); // Column end address (127 = reset)
	
	oled_command(SSD1306_PAGEADDR);
	oled_command(0); // Page start address (0 = reset)
	oled_command(7); // Page end address

    for (i=0; i<OLED_BUFSZ; i++)
	{
		/* send a block of data */
		oled_data(&oled_buffer[buf_num][i], 16);
		
		/* Adafruit only increments by 15 - why? */
		i+=15;
    }
}

/*
 * clear the display buffer
 */
void oled_clear(uint8_t buf_num, uint8_t color)
{
	uint16_t i;
	uint8_t byte = (color == 1) ? 0xFF : 0x00;
	
	for(i=0;i<OLED_BUFSZ;i++)
	{
		oled_buffer[buf_num][i] = byte;
	}
}

/*
 * draw a single pixel
 */
void oled_drawPixel(uint8_t buf_num, uint8_t x, uint8_t y, uint8_t color)
{
	/* clip to display dimensions */
	if ((x >= OLED_W) || (y >= OLED_H))
	return;

	/* plot */
	if (color == 1) 
		oled_buffer[buf_num][x+ (y/8)*OLED_W] |= (1<<(y%8));  
	else
		oled_buffer[buf_num][x+ (y/8)*OLED_W] &= ~(1<<(y%8)); 
}

/*
 * invert a single pixel
 */
void oled_xorPixel(uint8_t buf_num, uint8_t x, uint8_t y)
{
	/* clip to display dimensions */
	if ((x >= OLED_W) || (y >= OLED_H))
	return;

	/* xor */
	oled_buffer[buf_num][x+ (y/8)*OLED_W] ^= (1<<(y%8));  
}

/*
 * get a single pixel
 */
uint8_t oled_getPixel(uint8_t buf_num, uint8_t x, uint8_t y)
{
	uint8_t result;
	
	/* clip to display dimensions */
	if ((x >= OLED_W) || (y >= OLED_H))
		return 0;

	/* get byte @ coords */
	result = oled_buffer[buf_num][x+ (y/8)*OLED_W];
	
	/* get desired bit */
	return (result >> (y%8)) & 1;
}

/*
 * Blit
 */
void oled_blit(uint8_t src_num, uint8_t src_x, uint8_t src_y, uint8_t w, uint8_t h,
			   uint8_t dst_num, uint8_t dst_x, uint8_t dst_y)
{
	uint8_t dx, dy;
	
	/* clip to display dimensions */
	if((src_x >= OLED_W) || (src_y >= OLED_H))
		return;
	if((dst_x >= OLED_W) || (dst_y >= OLED_H))
		return;
	if((w==0) || (h==0))
		return;
	if((src_y+h-1) >= OLED_H)
		h = OLED_H-src_y;
	if((src_x+w-1) >= OLED_W)
		w = OLED_W-src_x;
	if((dst_y+h-1) >= OLED_H)
		h = OLED_H-dst_y;
	if((dst_x+w-1) >= OLED_W)
		w = OLED_W-dst_x;
	
	/* make it sensitive to src/dst overlap by changing start/end directions */

	/* copy */
	for(dx=0;dx<w;dx++)
	{
		for(dy=0;dy<h;dy++)
		{
			oled_drawPixel(dst_num, dst_x+dx, dst_y+dy, 
				oled_getPixel(src_num, src_x+dx, src_y+dy));
		}
	}
}

/*
 * sliding transition
 */
void oled_slide(uint8_t src0_num, uint8_t src1_num, uint8_t dst_num, uint8_t dir)
{
	uint8_t i;
	
	switch(dir)
	{
		case OLED_LEFT:
			for(i=0;i<=OLED_W;i+=4)
			{
				oled_blit(src0_num, i, 0, OLED_W-i, 64, dst_num, 0, 0);
				oled_blit(src1_num, 0, 0, i, 64, dst_num, OLED_W-i, 0);
				oled_refresh(dst_num);
				HAL_Delay(2);
			}
			break;
		
		case OLED_RIGHT:
			for(i=0;i<=OLED_W;i+=4)
			{
				oled_blit(src0_num, 0, 0, OLED_W-i, 64, dst_num, i, 0);
				oled_blit(src1_num, OLED_W-i, 0, i, 64, dst_num, 0, 0);
				oled_refresh(dst_num);
				HAL_Delay(2);
			}
			break;
		
		case OLED_UP:
			for(i=0;i<=OLED_H;i+=4)
			{
				oled_blit(src0_num, 0, i, 128, OLED_H-i, dst_num, 0, 0);
				oled_blit(src1_num, 0, 0, 128, i, dst_num, 0, OLED_H-i);
				oled_refresh(dst_num);
				HAL_Delay(2);
			}
			break;
		
		case OLED_DOWN:
			for(i=0;i<=OLED_H;i+=4)
			{
				oled_blit(src0_num, 0, 0, 128, OLED_H-i, dst_num, 0, i);
				oled_blit(src1_num, 0, OLED_H-i, 128, i, dst_num, 0, 0);
				oled_refresh(dst_num);
				HAL_Delay(2);
			}
			break;
		
		default:
			break;
	}
}

/*
 *  fast vert line
 */
void oled_drawFastVLine(uint8_t buf_num, uint8_t x, uint8_t y, uint8_t h, uint8_t color)
{
	// clipping
	if((x >= OLED_W) || (y >= OLED_H)) return;
	if((y+h-1) >= OLED_H) h = OLED_H-y;
	while(h--)
	{
        oled_drawPixel(buf_num, x, y++, color);
	}
}

/*
 *  fast horiz line
 */
void oled_drawFastHLine(uint8_t buf_num, uint8_t x, uint8_t y, uint8_t w, uint8_t color)
{
	// clipping
	if((x >= OLED_W) || (y >= OLED_H)) return;
	if((x+w-1) >= OLED_W)  w = OLED_W-x;

	while (w--)
	{
        oled_drawPixel(buf_num, x++, y, color);
	}
}

/*
 * abs() helper function for line drawing
 */
int16_t gfx_abs(int16_t x)
{
	return (x<0) ? -x : x;
}

/*
 * swap() helper function for line drawing
 */
void gfx_swap(uint16_t *z0, uint16_t *z1)
{
	uint16_t temp = *z0;
	*z0 = *z1;
	*z1 = temp;
}

/*
 * Bresenham line draw routine swiped from Wikipedia
 */
void oled_line(uint8_t buf_num, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t color)
{
	int16_t steep;
	int16_t deltax, deltay, error, ystep, x, y;

	/* flip sense 45deg to keep error calc in range */
	steep = (gfx_abs(y1 - y0) > gfx_abs(x1 - x0));

	if(steep)
	{
		gfx_swap(&x0, &y0);
		gfx_swap(&x1, &y1);
	}

	/* run low->high */
	if(x0 > x1)
	{
		gfx_swap(&x0, &x1);
		gfx_swap(&y0, &y1);
	}

	/* set up loop initial conditions */
	deltax = x1 - x0;
	deltay = gfx_abs(y1 - y0);
	error = deltax/2;
	y = y0;
	if(y0 < y1)
		ystep = 1;
	else
		ystep = -1;

	/* loop x */
	for(x=x0;x<=x1;x++)
	{
		/* plot point */
		if(steep)
			/* flip point & plot */
			oled_drawPixel(buf_num, y, x, color);
		else
			/* just plot */
			oled_drawPixel(buf_num, x, y, color);

		/* update error */
		error = error - deltay;

		/* update y */
		if(error < 0)
		{
			y = y + ystep;
			error = error + deltax;
		}
	}
}

/*
 *  draws a circle
 */
void oled_Circle(int8_t buf_num, int16_t x, int16_t y, int16_t radius, int8_t color)
{
    /* Bresenham algorithm */
    int16_t x_pos = -radius;
    int16_t y_pos = 0;
    int16_t err = 2 - 2 * radius;
    int16_t e2;

    do {
        oled_drawPixel(buf_num, x - x_pos, y + y_pos, color);
        oled_drawPixel(buf_num, x + x_pos, y + y_pos, color);
        oled_drawPixel(buf_num, x + x_pos, y - y_pos, color);
        oled_drawPixel(buf_num, x - x_pos, y - y_pos, color);
        e2 = err;
        if (e2 <= y_pos) {
            err += ++y_pos * 2 + 1;
            if(-x_pos == y_pos && e2 <= x_pos) {
              e2 = 0;
            }
        }
        if (e2 > x_pos) {
            err += ++x_pos * 2 + 1;
        }
    } while (x_pos <= 0);
}

/*
 *  draws a filled circle
 */
void oled_FilledCircle(int8_t buf_num, int16_t x, int16_t y, int16_t radius, int8_t color)
{
    /* Bresenham algorithm */
    int16_t x_pos = -radius;
    int16_t y_pos = 0;
    int16_t err = 2 - 2 * radius;
    int16_t e2;

    do {
        oled_drawPixel(buf_num, x - x_pos, y + y_pos, color);
        oled_drawPixel(buf_num, x + x_pos, y + y_pos, color);
        oled_drawPixel(buf_num, x + x_pos, y - y_pos, color);
        oled_drawPixel(buf_num, x - x_pos, y - y_pos, color);
        oled_drawFastHLine(buf_num, x + x_pos, y + y_pos, 2 * (-x_pos) + 1, color);
        oled_drawFastHLine(buf_num, x + x_pos, y - y_pos, 2 * (-x_pos) + 1, color);
        e2 = err;
        if (e2 <= y_pos) {
            err += ++y_pos * 2 + 1;
            if(-x_pos == y_pos && e2 <= x_pos) {
                e2 = 0;
            }
        }
        if(e2 > x_pos) {
            err += ++x_pos * 2 + 1;
        }
    } while(x_pos <= 0);
}

/*
 *  draw a box
 */
void oled_Box(uint8_t buf_num, uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color)
{
	oled_drawFastVLine(buf_num, x, y, h, color);
	oled_drawFastVLine(buf_num, x+w-1, y, h, color);
	oled_drawFastHLine(buf_num, x, y, w, color);
	oled_drawFastHLine(buf_num, x, y+h-1, w, color);
}

/*
 * draw a rectangle in the buffer
 */
void oled_drawrect(uint8_t buf_num, uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color)
{
	uint8_t m, n=y, iw = w;
	
	/* scan vertical */
	while(h--)
	{
		m=x;
		w=iw;
		/* scan horizontal */
		while(w--)
		{
			/* invert pixels */
			oled_drawPixel(buf_num, m++, n, color);
		}
		n++;
	}
}

/*
 * invert a rectangle in the buffer
 */
void oled_xorrect(uint8_t buf_num, uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
	uint8_t m, n=y, iw = w;
	
	/* scan vertical */
	while(h--)
	{
		m=x;
		w=iw;
		/* scan horizontal */
		while(w--)
		{
			/* invert pixels */
			oled_xorPixel(buf_num, m++, n);
		}
		n++;
	}
}

/*
 * Draw character to the display buffer
 */
void oled_drawchar(uint8_t buf_num, uint8_t x, uint8_t y, uint8_t chr, uint8_t color)
{
	uint16_t i, j, col;
	uint8_t d;
	
	for(i=0;i<8;i++)
	{
		d = fontdata[(chr<<3)+i];
		for(j=0;j<8;j++)
		{
			if(d&0x80)
				col = color;
			else
				col = (~color)&1;
			
			oled_drawPixel(buf_num, x+j, y+i, col);
			
			// next bit
			d <<= 1;
		}
	}
}

/*
 * draw a string to the display
 */
void oled_drawstr(uint8_t buf_num, uint8_t x, uint8_t y, char *str, uint8_t color)
{
	uint8_t c;
	
	while((c=*str++))
	{
		oled_drawchar(buf_num, x, y, c, color);
		x += 8;
		if(x>120)
			break;
	}
}

#if 1
/*
 * draw a string using the 32-bit high font
 * horiz posn given by x,vert row on given by y
 */
void oled_drawbitfont(uint8_t buf_num, uint8_t x, uint8_t y, char *str, uint8_t color)
{
	uint8_t char_width, gx, gy, d, j, col;
	char* cptr = str;
	TCDATA* char_data;
	
	/* scan through string */
	while(*cptr)
	{
		/* get pointer to start of font char data */
		char_data = arial_24_bold_32_numpunct[(uint8_t)*cptr];
		
		/* get width of char */
		char_width = *char_data++;
		
		/* loop over char data */
		for(gx=0;gx<char_width;gx++)
		{
			if((x+gx) < OLED_W)
			{
				for(gy=0;gy<4;gy++)
				{
					d = *char_data++;
					
					/* loop over bits & plot pixels */
					for(j=0;j<8;j++)
					{
						if(d&0x1)
							col = color;
						else
							col = (~color)&1;
						
						oled_drawPixel(buf_num, x+gx, y+gy*8+j, col);
						
						// next bit
						d >>= 1;
					}
				}
			}
		}
		
		/* next char */
		x += char_width;
		cptr++;
	}
}
#endif

/*
 * render grayscale map to oled with simple threshold
 */
void oled_gray_slice(uint8_t buf_num, uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t *bmp, uint8_t t)
{
	uint8_t dx, dy;
	
	for(dy=0;dy<h;dy++)
	{
		for(dx=0;dx<w;dx++)
		{
			oled_drawPixel(buf_num, x+dx, y+dy, (*bmp++>t));
		}
	}
}

/* line buffer for error diffusion */
int16_t lines[2][128];

/*
 * render grayscale map to oled with Floyd-Steinberg error diffusion
 */
void oled_gray_fs(uint8_t buf_num, uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t *bmp)
{
	uint8_t dx, dy;
	uint8_t *b = bmp;
	uint8_t oldpix, newpix;
	int16_t err;
	int16_t thr;
    
	/* copy first two lines */
	for(dy=0;dy<2;dy++)
	{
		for(dx=0;dx<w;dx++)
		{
			lines[dy][dx] = *b++;
		}
	}
    
    /* dither the threshold a bit */
    thr = 124 + (rand()&0x7);
	
	/* scan thru & do diffusion */
	for(dy=0;dy<h;dy++)
	{
		for(dx=0;dx<w;dx++)
		{
			/* quantize */
			oldpix = lines[0][dx];
			newpix = (oldpix>thr) ? 255 : 0;
			
			/* plot point */
			oled_drawPixel(buf_num, x+dx, y+dy, (newpix>0) ? 1 : 0);
			err = oldpix-newpix;
			
			/* update diffusion */
			if(dx<(w-1)) lines[0][dx+1] += ((7*err)>>4);
			if(dx>0) lines[1][dx-1] += ((3*err)>>4);
			lines[1][dx] += ((5*err)>>4);
			if(dx<(w-1)) lines[1][dx+1] += (err>>4);
		}
		
		/* advance diffusion */
		for(dx=0;dx<w;dx++)
		{
			lines[0][dx] = lines[1][dx];
			if(dy<(h-1)) lines[1][dx] = *b++;
		}
	}
}
