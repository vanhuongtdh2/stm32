#include <stm32l1xx_gpio.h>
#include <stm32l1xx_rcc.h>
#include <string.h> // For memcpy

#include "delay.h"
#include "spi.h"

#include "ST7541.h"


GrayScale_TypeDef         lcd_color       = gs_black; // Foreground color
uint16_t                  scr_width       = SCR_W; // Screen width
uint16_t                  scr_height      = SCR_H; // Screen height
ScrOrientation_TypeDef    scr_orientation = scr_normal; // Screen orientation

// Display buffer
uint8_t vRAM[(SCR_W * SCR_H) >> 2] __attribute__((aligned(4)));

// Grayscale palette (PWM values for white, light gray, dark gray, black)
uint8_t const GrayPalette[] = {0x00,0x00,0xaa,0xaa,0xdd,0xdd,0xff,0xff}; // 15PWM
//uint8_t const GrayPalette[] = {0x00,0x00,0xdd,0xdd,0xee,0xee,0xff,0xff}; // 15PWM
//uint8_t const GrayPalette[] = {0x00,0x00,0x55,0x55,0xaa,0xaa,0xcc,0xcc}; // 12PWM
//uint8_t const GrayPalette[] = {0x00,0x00,0x77,0x77,0x88,0x88,0x99,0x99}; // 9PWM


// Send single byte command to display
// input:
//   cmd - display command
void ST7541_cmd(uint8_t cmd) {
	ST7541_RS_L();
	SPIx_Send(ST7541_SPI_PORT,cmd);
}

// Send double byte command to display
// input:
//   cmd1 - first byte of display command
//   cmd2 - first byte of display command
void ST7541_cmd_double(uint8_t cmd1, uint8_t cmd2) {
	ST7541_RS_L();
	SPIx_Send(ST7541_SPI_PORT,cmd1);
	SPIx_Send(ST7541_SPI_PORT,cmd2);
}

// Send data byte to display
// input:
//   data - data byte
void ST7541_data(uint8_t data) {
	ST7541_RS_H();
	SPIx_Send(ST7541_SPI_PORT,data);
}

// Initialize the display control GPIO pins
void ST7541_InitGPIO(void) {
	GPIO_InitTypeDef PORT;

	// Enable the GPIO peripheral(s) clock
	RCC->AHBENR |= ST7541_GPIO_PERIPH;

	// Configure the GPIO pins
	PORT.GPIO_Mode = GPIO_Mode_OUT;
	PORT.GPIO_OType = GPIO_OType_PP;
	PORT.GPIO_PuPd = GPIO_PuPd_UP;
	PORT.GPIO_Speed = GPIO_Speed_40MHz;

	ST7541_CS_H();
	PORT.GPIO_Pin = ST7541_CS_PIN;
	GPIO_Init(ST7541_CS_PORT,&PORT);

	ST7541_RS_L();
	PORT.GPIO_Pin = ST7541_RS_PIN;
	GPIO_Init(ST7541_RS_PORT,&PORT);

	ST7541_RST_H();
	PORT.GPIO_Pin = ST7541_RST_PIN;
	GPIO_Init(ST7541_RST_PORT,&PORT);
}

// Initialize SPI peripheral and ST7541 display
// note: SPI peripheral must be initialized before
//       Delay must be initialized before
void ST7541_Init(void) {
	uint8_t i;

	// Reset display
	ST7541_CS_H();
	ST7541_RST_L();
	Delay_ms(10);
	ST7541_RST_H();
	Delay_ms(10);
	ST7541_CS_L();

	ST7541_cmd(0xc8); // COM reverse direction
	ST7541_cmd(0xa1); // ADC reverse direction
	ST7541_cmd_double(0x40,0); // Initial display line 0
	ST7541_cmd_double(0x44,0); // Initial COM0 0
	ST7541_cmd_double(0x48,128); // Duty cycle 128

	ST7541_cmd(0xab); // Enable built-in oscillator circuit

//	ST7541_cmd(0x20); // Regulator resistor: 2.3
//	ST7541_cmd(0x21); // Regulator resistor: 3.0
//	ST7541_cmd(0x22); // Regulator resistor: 3.7
//	ST7541_cmd(0x23); // Regulator resistor: 4.4
//	ST7541_cmd(0x24); // Regulator resistor: 5.1
//	ST7541_cmd(0x25); // Regulator resistor: 5.8
//	ST7541_cmd(0x26); // Regulator resistor: 6.5
	ST7541_cmd(0x27); // Regulator resistor: 7.2

//	ST7541_cmd_double(0x81,26); // Contrast
	ST7541_cmd_double(0x81,32); // Contrast
//	ST7541_cmd_double(0x81,41); // Contrast

//	ST7541_cmd(0x50); // LCD bias: 1/5
//	ST7541_cmd(0x51); // LCD bias: 1/6
//	ST7541_cmd(0x52); // LCD bias: 1/7
//	ST7541_cmd(0x53); // LCD bias: 1/8
//	ST7541_cmd(0x54); // LCD bias: 1/9
//	ST7541_cmd(0x55); // LCD bias: 1/10
//	ST7541_cmd(0x56); // LCD bias: 1/11
	ST7541_cmd(0x57); // LCD bias: 1/12

//	ST7541_cmd_double(0x38,0xf4); // Frame rate 123Hz, Booster efficiency level 2
//	ST7541_cmd_double(0x38,0xf0); // Frame rate 123Hz, Booster efficiency level 0
//	ST7541_cmd_double(0x38,0x54); // Frame rate 67Hz, Booster efficiency level 2
	ST7541_cmd_double(0x38,0x04); // Frame rate 77Hz, Booster efficiency level 2
//	ST7541_cmd_double(0x38,0x00); // Frame rate 77Hz, Booster efficiency level 0
//	ST7541_cmd_double(0x38,0x14); // Frame rate 51Hz, Booster efficiency level 2
//	ST7541_cmd_double(0x38,0x74); // Frame rate 70Hz, Booster efficiency level 2

	ST7541_cmd(0x2e); // Power control: VC,VR,VF = 1,1,0 (internal voltage booster)
//	ST7541_cmd(0x2a); // Power control: VC,VR,VF = 0,1,0 (external LCD bias supply)
	Delay_ms(10);
	ST7541_cmd(0x2f); // Power control: VC,VR,VF = 1,1,1 (internal voltage booster)
//	ST7541_cmd(0x2b); // Power control: VC,VR,VF = 0,1,1 (external LCD bias supply)

//	ST7541_cmd(0x64); // DC-DC converter: 3 times boosting circuit
//	ST7541_cmd(0x65); // DC-DC converter: 4 times boosting circuit
//	ST7541_cmd(0x66); // DC-DC converter: 5 times boosting circuit
	ST7541_cmd(0x67); // DC-DC converter: 6 times boosting circuit

//	ST7541_cmd(0x90); // FRC/PWM mode: 4FRC, 9PWM
//	ST7541_cmd(0x92); // FRC/PWM mode: 4FRC, 12PWM
//	ST7541_cmd(0x93); // FRC/PWM mode: 4FRC, 15PWM
//	ST7541_cmd(0x94); // FRC/PWM mode: 3FRC, 9PWM
//	ST7541_cmd(0x96); // FRC/PWM mode: 3FRC, 12PWM
	ST7541_cmd(0x97); // FRC/PWM mode: 3FRC, 15PWM

	// Enable high power mode
//	ST7541_cmd_double(0xf7,0x1a); // High power mode enable
//	ST7541_cmd_double(0xf3,0x0d); // High power mode control

	// Load grayscale palette
	for (i = 0; i < 8; i++)	ST7541_cmd_double(0x88 + i,GrayPalette[i]);

	ST7541_cmd(0xaf); // Display ON

	ST7541_Orientation(scr_normal);

	ST7541_CS_H();
}

// Reset display registers
// Doesn't affect the display memory and some registers
void ST7541_Reset(void) {
	ST7541_CS_L();
	ST7541_cmd(0xe2);
	ST7541_CS_H();
}

// Set LCD contrast
// input:
//   res_ratio: internal regulator resistor ratio [0..7], power on value is 0 => 2.3
//   lcd_bias: LCD bias [0..7], power on value is 7 => 1/12
//   el_vol: electronic volume [0..63], power on value is 32
void ST7541_Contrast(uint8_t res_ratio, uint8_t lcd_bias, uint8_t el_vol) {
	ST7541_CS_L();
	ST7541_cmd(0x20 | (res_ratio & 0x07)); // Regulator resistor ratio
	ST7541_cmd(0x50 | (lcd_bias & 0x07)); // LCD bias
	ST7541_cmd_double(0x81,el_vol & 0x3f); // Electronic volume
	ST7541_CS_H();
}

// Set all LCD pixels on or off
// Input:
//   state: ON or OFF
// Doesn't affect the display memory
void ST7541_SetAllPixelOn(OnOffStatus state) {
	ST7541_CS_L();
	ST7541_cmd(state == ON ? 0xa5 : 0xa4);
	ST7541_CS_H();
}

// Set inverse display pixels
// Input:
//   state - NORMAL or INVERT
void ST7541_SetInvert(InvertStatus state) {
	ST7541_CS_L();
	ST7541_cmd(state == NORMAL ? 0xa6 : 0xa7);
	ST7541_CS_H();
}

// Toggle display on/off
// Input:
//   state - ENABLED or DISABLED
// Doesn't affect the display memory
void ST7541_SetDisplayState(DisplayState state) {
	ST7541_CS_L();
	ST7541_cmd(state == ENABLED ? 0xaf : 0xae);
	ST7541_CS_H();
}

// Configure partial display on LCD
// input:
//   COM - initial display COM
//   Line - initial display line
//   Duty - partial display duty
void ST7541_SetDisplayPartial(uint8_t COM, uint8_t Line, uint8_t Duty) {
	ST7541_CS_L();
	ST7541_cmd_double(0x40,Line & 0x7f); // Line
	ST7541_cmd_double(0x44,COM & 0x7f);  // COM
	ST7541_cmd_double(0x48,Duty);        // Duty
	ST7541_CS_H();
}

// Enter display power save mode
// input:
//   state - ON puts display to sleep mode, OFF - returns to normal mode
void ST7541_PowerSave(OnOffStatus state) {
	ST7541_CS_L();
	ST7541_cmd(state == ON ? 0xa9 : 0xa8);
	ST7541_CS_H();
}

// Set X coordinate mapping (normal or mirrored)
// Input:
//   state: NORMAL or INVERT
// Doesn't affect the display memory
void ST7541_SetXDir(InvertStatus MX) {
	ST7541_CS_L();
	ST7541_cmd(MX == NORMAL ? 0xa0 : 0xa1); // SEG direction select
	ST7541_CS_H();
}

// Set Y coordinate mapping (normal or mirrored)
// Input:
//   state: NORMAL or INVERT
// Doesn't affect the display memory
void ST7541_SetYDir(InvertStatus MY) {
	ST7541_CS_L();
	ST7541_cmd(MY == NORMAL ? 0xc8 : 0xc0); // COM direction select
	ST7541_CS_H();
}

// Set display column:page according to pixel coordinates
// Input:
//   X, Y - pixel coordinates
void ST7541_SetAddr(uint8_t X, uint8_t Y) {
	ST7541_CS_L();
	ST7541_cmd(X & 0x0f); // Column address LSB
	ST7541_cmd(0x10 | ((X > 4) & 0x07)); // Column address MSB
	ST7541_cmd(0xb0 | ((Y / 8) & 0x0f)); // Page address
	ST7541_CS_H();
}

// Set scroll line number
// Input:
//   line - start line number (0..127)
void ST7541_SetScrollLine(uint8_t line) {
	ST7541_CS_L();
	ST7541_cmd_double(0x40,line & 0x7f); // Set initial display line
	ST7541_CS_H();
}

// Set display orientation
void ST7541_Orientation(uint8_t orientation) {
	ST7541_CS_L();
	switch(orientation) {
	case scr_CW:
		scr_width  = SCR_H;
		scr_height = SCR_W;
		ST7541_SetXDir(NORMAL);
		ST7541_SetYDir(NORMAL);
		break;
	case scr_CCW:
		scr_width  = SCR_W;
		scr_height = SCR_H;
		ST7541_SetXDir(INVERT);
		ST7541_SetYDir(INVERT);
		break;
	case scr_180:
		scr_width  = SCR_W;
		scr_height = SCR_H;
		ST7541_SetXDir(NORMAL);
		ST7541_SetYDir(INVERT);
		break;
	default:
		scr_width  = SCR_H;
		scr_height = SCR_W;
		ST7541_SetXDir(INVERT);
		ST7541_SetYDir(NORMAL);
		break;
	}
	scr_orientation = orientation;
	ST7541_CS_H();
}

// Send vRAM buffer into display
void ST7541_Flush(void) {
	// Send video buffer with SPI 16-bit frame
	ST7541_SetAddr(0,0);
	ST7541_CS_L();
	ST7541_RS_H();
	// Disable the SPI peripheral, set 16-bit data frame format and then enable the SPI back
	ST7541_SPI_PORT->CR1 &= ~SPI_CR1_SPE;
	ST7541_SPI_PORT->CR1 |= SPI_CR1_DFF | SPI_CR1_SPE;
	// Send buffer
	SPIx_SendBuf16(ST7541_SPI_PORT,(uint16_t *)&vRAM[0],(SCR_W * SCR_H) >> 3);
	ST7541_CS_H();
	// Disable the SPI peripheral, set 8-bit data frame format and then enable the SPI back
	ST7541_SPI_PORT->CR1 &= ~(SPI_CR1_DFF | SPI_CR1_SPE);
	ST7541_SPI_PORT->CR1 |= SPI_CR1_SPE;

/*
	// Send video buffer with SPI 8-bit frame
	ST7541_SetAddr(0,0);
	ST7541_CS_L();
	ST7541_RS_H();
	SPIx_SendBuf(ST7541_SPI_PORT,vRAM,(SCR_W * SCR_H) >> 2);
	ST7541_CS_H();
*/
}

// Send vRAM buffer into display with DMA
void ST7541_Flush_DMA(void) {
	ST7541_SetAddr(0,0);
	ST7541_CS_L();
	ST7541_RS_H();
	// Configure the DMA transfer
	SPIx_Configure_DMA_TX(ST7541_SPI_PORT,vRAM,(SCR_W * SCR_H) >> 2);
	// Enable the DMA channel
	SPIx_SetDMA(ST7541_SPI_PORT,ENABLE);
	// Wait until DMA transfer is over
	while (!(SPI1_DMA_PERIPH->ISR & SPI1_DMA_TX_TCIF));
	// Disable the DMA channel
	SPIx_SetDMA(ST7541_SPI_PORT,DISABLE);
	// Ensure that the SPI communication is complete
	while (!(ST7541_SPI_PORT->SR & SPI_SR_TXE));
	while (ST7541_SPI_PORT->SR & SPI_SR_BSY);
	ST7541_CS_H();
}

// Fill vRAM memory with specified pattern
// input:
//   pattern - byte to fill vRAM buffer
void ST7541_Fill(uint16_t pattern) {
	uint16_t i;
	union {
		uint16_t w;
		struct {
			uint8_t b1;
			uint8_t b0;
		};
	} p;

	p.w = pattern;
	for (i = 0; i < (SCR_W * SCR_H) >> 2; ) {
		vRAM[i++] = p.b1;
		vRAM[i++] = p.b0;
	}
}

// Set pixel in vRAM buffer
// input:
//   X, Y - pixel coordinates
//   GS - grayscale pixel color (gs_[white,ltgray,dkgray,black])
void Pixel(uint8_t X, uint8_t Y, GrayScale_TypeDef GS) {
	uint8_t XX = X;
	uint8_t YY = Y;
	uint8_t bit;
	uint16_t offset;

	if (scr_orientation == scr_CW || scr_orientation == scr_CCW) {
		XX = Y;
		YY = X;
	}

	bit = 1 << (YY % 8);
//	offset = ((YY >> 3) * (SCR_W << 1)) + (XX << 1);
	offset = ((YY >> 3) << 8) + (XX << 1); // screen width 128, therefore this a bit faster

	if (GS == gs_white) {
		vRAM[offset]     &= ~bit;
		vRAM[offset + 1] &= ~bit;
	} else if (GS == gs_black) {
		vRAM[offset]     |=  bit;
		vRAM[offset + 1] |=  bit;
	} else if (GS == gs_dkgray) {
		vRAM[offset]     |=  bit;
		vRAM[offset + 1] &= ~bit;
	} else {
		// gs_ltgray
		vRAM[offset]     &= ~bit;
		vRAM[offset + 1] |=  bit;
	}
}

// Draw horizontal line
// input:
//   X1, X2 - left and right horizontal coordinates (X1 must be less than X2)
//   Y - vertical coordinate
//   GS - grayscale pixel color
void HLine(uint8_t X1, uint8_t X2, uint8_t Y, GrayScale_TypeDef GS) {
	uint8_t X,eX;

	if (X1 > X2) {
		X = X1; eX = X2;
	} else {
		X = X2; eX = X1;
	}
	do {
		Pixel(X,Y,GS);
	} while (X-- > eX);

/*
	uint8_t X;

	for (X = X1; X <= X2; X++) Pixel(X,Y,GS);
*/
}

// Draw vertical line
// input:
//   X - horizontal coordinate
//   Y1,Y2 - top and bottom vertical coordinates (Y1 must be less than Y2)
//   GS - grayscale pixel color
void VLine(uint8_t X, uint8_t Y1, uint8_t Y2, GrayScale_TypeDef GS) {
	uint8_t Y,eY;

	if (Y1 > Y2) {
		Y = Y1; eY = Y2;
	} else {
		Y = Y2; eY = Y1;
	}
	do {
		Pixel(X,Y,GS);
	} while (Y-- > eY);

/*
	uint8_t Y;

	for (Y = Y1; Y <= Y2; Y++) Pixel(X,Y,GS);
*/
}

// Draw rectangle
// input:
//   X1,Y1 - top left coordinates
//   X2,Y2 - bottom right coordinates
//   GS - grayscale pixel color
// note: X1 must be less than X2 and Y1 must be less than Y2
void Rect(uint8_t X1, uint8_t Y1, uint8_t X2, uint8_t Y2, GrayScale_TypeDef GS) {
	HLine(X1,X2,Y1,GS);
	HLine(X1,X2,Y2,GS);
	VLine(X1,Y1 + 1,Y2 - 1,GS);
	VLine(X2,Y1 + 1,Y2 - 1,GS);
}

// Draw filled rectangle
// input:
//   X1,Y1 - top left coordinates
//   X2,Y2 - bottom right coordinates
//   GS - grayscale pixel color
// note: X1 must be less than X2 and Y1 must be less than Y2
void FillRect(uint8_t X1, uint8_t Y1, uint8_t X2, uint8_t Y2, GrayScale_TypeDef GS) {
	uint8_t Y;

	for (Y = Y1; Y <= Y2; Y++) HLine(X1,X2,Y,GS);
}

// Draw line
// input:
//    X1,Y1 - top left coordinates
//    X2,Y2 - bottom right coordinates
//    GS - grayscale pixel color
void Line(int16_t X1, int16_t Y1, int16_t X2, int16_t Y2, GrayScale_TypeDef GS) {
	int16_t dX = X2-X1;
	int16_t dY = Y2-Y1;
	int16_t dXsym = (dX > 0) ? 1 : -1;
	int16_t dYsym = (dY > 0) ? 1 : -1;

	if (dX == 0) {
		if (Y2 > Y1) VLine(X1,Y1,Y2,GS); else VLine(X1,Y2,Y1,GS);
		return;
	}
	if (dY == 0) {
		if (X2 > X1) HLine(X1,X2,Y1,GS); else HLine(X2,X1,Y1,GS);
		return;
	}

	dX *= dXsym;
	dY *= dYsym;
	int16_t dX2 = dX << 1;
	int16_t dY2 = dY << 1;
	int16_t di;

	if (dX >= dY) {
		di = dY2 - dX;
		while (X1 != X2) {
			Pixel(X1,Y1,GS);
			X1 += dXsym;
			if (di < 0) {
				di += dY2;
			} else {
				di += dY2 - dX2;
				Y1 += dYsym;
			}
		}
	} else {
		di = dX2 - dY;
		while (Y1 != Y2) {
			Pixel(X1,Y1,GS);
			Y1 += dYsym;
			if (di < 0) {
				di += dX2;
			} else {
				di += dX2 - dY2;
				X1 += dXsym;
			}
		}
	}
	Pixel(X1,Y1,GS);
}

// Draw ellipse
// input:
//   X,Y - coordinates of center of the ellipse
//   A,B - horizontal and vertical radiuses
//    GS - grayscale pixel color
void Ellipse(uint16_t X, uint16_t Y, uint16_t A, uint16_t B, GrayScale_TypeDef GS) {
	int16_t Xc = 0, Yc = B;

	long A2 = (long)A*A, B2 = (long)B*B;
	long C1 = -((A2 >> 2) + A % 2 + B2);
	long C2 = -((B2 >> 2) + B % 2 + A2);
	long C3 = -((B2 >> 2) + B % 2);
	long t = -A2 * Yc;
	long dXt = B2*Xc*2, dYt = -A2*Yc*2;
	long dXt2 = B2*2, dYt2 = A2*2;
	while (Yc >= 0 && Xc <= A) {
		Pixel(X + Xc,Y + Yc,GS);
		if (Xc != 0 || Yc != 0) Pixel(X - Xc,Y - Yc,GS);
		if (Xc != 0 && Yc != 0) {
			Pixel(X + Xc,Y - Yc,GS);
			Pixel(X - Xc,Y + Yc,GS);
		}
		if (t + Xc*B2 <= C1 || t + Yc*A2 <= C3) {
			Xc++;
			dXt += dXt2;
			t   += dXt;
		} else if (t - Yc*A2 > C2) {
			Yc--;
			dYt += dYt2;
			t   += dYt;
		} else {
			Xc++;
			Yc--;
			dXt += dXt2;
			dYt += dYt2;
			t   += dXt;
			t   += dYt;
		}
	}
}

// Draw single character
// input:
//   X,Y - character top left corner coordinates
//   Char - character to be drawn
//   Font - pointer to font
// return: character width in pixels
uint8_t DrawChar(uint8_t X, uint8_t Y, uint8_t Char, const Font_TypeDef *Font) {
	uint8_t chW,chH;
	uint8_t pX = X;
	uint8_t pY = Y;
	uint16_t i,j,k;
	uint8_t buffer[32];
	uint8_t tmpCh;

	chW = Font->font_Width; // Character width
	chH = Font->font_Height; // Character height
	if (Char < 32 || Char > 0x7e) Char = 0x7e;
	memcpy(buffer,&Font->font_Data[(Char - 32) * Font->font_BPC],Font->font_BPC);

	if (Font->font_Scan == font_V) {
		if (chH < 8) {
			// Small font, one byte height
			for (i = 0; i < chW; i++) {
				tmpCh = buffer[i];
				for (j = 0; j < chH; j++) {
					if ((tmpCh >> j) & 0x01) Pixel(pX,pY,lcd_color);
					pY++;
				}
				pY = Y;
				pX++;
			}
		} else {
			// Big font, more than one byte height
			for (k = 0; k <= (chH / 8); k++) {
				for (i = 0; i < chW; i++) {
					tmpCh = buffer[(i << 1) + k];
					for (j = 0; j < 8; j++) {
						if ((tmpCh >> (7 - j)) & 0x01) Pixel(pX,pY,lcd_color);
						pY++;
						if (pY > Y + chH) break;
					}
					pX++;
				}
				pY = Y + (k << 3);
				pX = X;
			}
		}
	} else {
		for (j = 0; j < chH; j++) {
			tmpCh = buffer[j];
			for (i = 0; i < chW; i++) {
				if ((tmpCh >> i) & 0x01) Pixel(pX,pY,lcd_color);
				pX++;
			}
			pX = X;
			pY++;
		}
	}

	return Font->font_Width + 1;
}

// Draw string
// input:
//   X,Y - top left coordinates of first character
//   str - pointer to zero-terminated string
//   Font - pointer to font
// return: string width in pixels
uint16_t PutStr(uint8_t X, uint8_t Y, char *str, const Font_TypeDef *Font) {
	uint8_t strLen = 0;

    while (*str) {
        X += DrawChar(X,Y,*str++,Font);
        if (X > scr_width - Font->font_Width - 1) break;
        strLen++;
    };

    return strLen * (Font->font_Width + 1);
}

// Draw string with line feed (by screen edge)
// input:
//   X,Y - top left coordinates of first character
//   str - pointer to zero-terminated string
//   Font - pointer to font
// return: string width in pixels
uint16_t PutStrLF(uint8_t X, uint8_t Y, char *str, const Font_TypeDef *Font) {
	uint8_t strLen = 0;

    while (*str) {
        DrawChar(X,Y,*str++,Font);
        if (X < scr_width - Font->font_Width - 1) {
        	X += Font->font_Width + 1;
        } else if (Y < scr_height - Font->font_Height - 1) {
        	X = 0; Y += Font->font_Height;
        } else {
        	X = 0; Y = 0;
        }
        strLen++;
    };

    return strLen * (Font->font_Width + 1);
}

// Draw signed integer value
// input:
//   X,Y - top left coordinates of first symbol
//   num - signed integer value
//   Font - pointer to font
// return: number width in pixels
uint8_t PutInt(uint8_t X, uint8_t Y, int32_t num, const Font_TypeDef *Font) {
	uint8_t str[11]; // 10 chars max for UINT32_MAX
	int8_t i = 0;
	uint8_t neg = 0;
	int8_t intLen;
	uint8_t pX;

	if (num < 0) {
		neg = 1;
		num *= -1;
	}
	do { str[i++] = num % 10 + '0'; } while ((num /= 10) > 0);
	if (neg) str[i++] = '-';
	intLen = i;
	pX = X + (intLen - 1) * (Font->font_Width + 1);
	for (i = 0; i < intLen; i++) {
		pX -= DrawChar(pX,Y,str[i],Font);
	}

    return intLen * (Font->font_Width + 1);
}

// Draw unsigned integer value
// input:
//   X,Y - top left coordinates of first symbol
//   num - unsigned integer value
//   Font - pointer to font
// return: number width in pixels
uint8_t PutIntU(uint8_t X, uint8_t Y, uint32_t num, const Font_TypeDef *Font) {
	uint8_t str[11]; // 10 chars max for UINT32_MAX
	int8_t i = 0;
	int8_t intLen;
	uint8_t pX;

	do { str[i++] = num % 10 + '0'; } while ((num /= 10) > 0);
	intLen = i;
	pX = X + (intLen - 1) * (Font->font_Width + 1);
	for (i = 0; i < intLen; i++) {
		pX -= DrawChar(pX,Y,str[i],Font);
	}

    return intLen * (Font->font_Width + 1);
}

// Draw signed integer value with decimal point
// input:
//   X,Y - top left coordinates of first symbol
//   num - unsigned integer value
//   decimals - number of digits after decimal point
//   Font - pointer to font
// return: number width in pixels
uint8_t PutIntF(uint8_t X, uint8_t Y, int32_t num, uint8_t decimals, const Font_TypeDef *Font) {
	uint8_t str[12];
	int8_t i;
	uint8_t neg;
	int8_t strLen;

	if (num < 0) {
		neg  = 1;
		num *= -1;
	} else neg = 0;

	i = 0;
	do { str[i++] = num % 10 + '0'; } while ((num /= 10) > 0);
	strLen = i;
	if (strLen <= decimals) {
		for (i = strLen; i <= decimals; i++) str[i] = '0';
		strLen = decimals + 1;
	}
	if (neg) str[strLen++] = '-';

	neg = X;
	for (i = 0; i < strLen; i++) {
		DrawChar(neg,Y,str[strLen - i - 1],Font);
		neg += Font->font_Width + 1;
		if (strLen - i - 1 == decimals && decimals != 0) {
			Rect(neg,Y + Font->font_Height - 2,neg + 1,Y + Font->font_Height - 1,lcd_color);
			neg += 3;
		}
	}

	return (neg - X);
}

// Draw signed integer value with leading zeros
// input:
//   X,Y - top left coordinates of first symbol
//   num - unsigned integer value
//   digits - minimal number of length (e.g. num=35, digits=5 --> 00035)
//   Font - pointer to font
// return: number width in pixels
uint8_t PutIntLZ(uint8_t X, uint8_t Y, int32_t num, uint8_t digits, const Font_TypeDef *Font) {
	uint8_t i;
	uint8_t neg;
	int32_t tmp = num;
	uint8_t len = 0;
	uint8_t pX = X;

	if (tmp < 0) {
		neg  = 1;
		tmp *= -1;
	} else neg = 0;

	do { len++; } while ((tmp /= 10) > 0); // Calculate number length in symbols

	if (len > digits) {
		X += PutInt(X,Y,num,Font);
		return X;
	}

	if (neg) {
		DrawChar(X,Y,'-',Font);
		X += Font->font_Width + 1;
		num *= -1;
	}
	for (i = 0; i < digits - len; i++) {
		DrawChar(X,Y,'0',Font);
		X += Font->font_Width + 1;
	}
	X += PutInt(X,Y,num,Font);

	return X - pX;
}

// Draw integer as hexadecimal
// input:
//   X,Y - top left coordinates of first symbol
//   num - unsigned integer value
//   Font - pointer to font
// return: number width in pixels
uint8_t PutHex(uint8_t X, uint8_t Y, uint32_t num, const Font_TypeDef *Font) {
	uint8_t str[11]; // 10 chars max for UINT32_MAX
	int8_t i = 0;
	uint32_t onum = num;
	int8_t intLen;
	uint8_t pX;

	do { str[i++] = "0123456789ABCDEF"[num % 0x10]; } while ((num /= 0x10) > 0);
	if (onum < 0x10) str[i++] = '0';

	intLen = i;

	pX = X + (intLen - 1) * (Font->font_Width + 1);
	for (i = 0; i < intLen; i++) {
		pX -= DrawChar(pX,Y,str[i],Font);
	}

    return intLen * (Font->font_Width + 1);
}
