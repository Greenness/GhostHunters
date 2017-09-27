#include "myLib.h"


unsigned short * videoBuffer = (unsigned short *)0x6000000;

unsigned short *frontBuffer = (u16 *)0x6000000;
unsigned short *backBuffer =  (u16 *)0x600A000;

DMA *dma = (DMA *)0x40000B0;

void setPixel3(int row, int col, unsigned short color)
{
    videoBuffer[OFFSET(row, col, SCREENWIDTH)] = color;
}

void DMANow(int channel, volatile const void* source, volatile void* destination, unsigned int control) {
    dma[channel].src = source;
    dma[channel].dst = destination;
    dma[channel].cnt = DMA_ON | control;
}

void drawRect3(int row, int col, int height, int width, unsigned short color) {
    volatile unsigned short c = color;
    for (int r = 0; r < height; r++) {
        DMANow(3, &c, &videoBuffer[OFFSET(row + r, col, 240)], DMA_SOURCE_FIXED | width);
    }  
}

void fillScreen3(unsigned short color) {
    volatile unsigned short c = color;
    DMANow(3, &c, videoBuffer, DMA_SOURCE_FIXED | (240*160));
}

//basic collison detector for all types
//checks if Object1 is colliding with Object2
//ONLY if objects are rectangles
int collisionCheckRect(int row1, int col1, int height1, int width1,
                    int row2, int col2, int height2, int width2)
{
    if (row1 <= row2 + height2 && //bottom side of Object2
        row1 + height1 >= row2 && //top side
        col1 <= col2 + width2 && //left side
        col1 + width1 >= col2) { //right side
        return 1;
    } else {
        return 0;
    }
}

void drawBackgroundImage3(const unsigned short *image) {
    DMANow(3, (unsigned short *) image, videoBuffer, (240 * 160) | DMA_SOURCE_INCREMENT);
}

void drawImage3(const unsigned short *image, int row, int col, int height, int width) {
    for (int r = 0; r < height; r++) {
        DMANow(3, (unsigned short *) &image[OFFSET(r, 0, width)], &videoBuffer[OFFSET(row + r, col, 240)], width);
    }
}

void waitForVblank() {
    while(SCANLINECOUNTER > 160);
    while(SCANLINECOUNTER < 160);
}

//Mode 4 Functions
void setPixel4(int row, int col, unsigned char colorIndex) {
    int pixelNumber = OFFSET(row, col, 240);
    int shortNumber = pixelNumber/2;
    u16 theShort = videoBuffer[shortNumber];
    if(col & 1) {
        videoBuffer[shortNumber] = (theShort & 0x00FF) | (colorIndex << 8);

    } else {
        videoBuffer[shortNumber] = (theShort & 0xFF00) | (colorIndex);
    }
}

void drawRect4(int row, int col, int height, int width, unsigned char colorIndex)
{
    unsigned short pixels = colorIndex << 8 | colorIndex; // This combines the index into a short so we can copy it easily
    if (col & 1) {
        if (width & 1) {
            for (int r = 0; r < height; r++) {
                setPixel4(row + r, col, colorIndex);
                DMANow(3, &pixels, &videoBuffer[OFFSET(row + r, col + 1, 240)/2], DMA_SOURCE_FIXED | (width/2));
            }
        } else {
            for (int r = 0; r < height; r++) {
                setPixel4(row + r, col, colorIndex);
                DMANow(3, &pixels, &videoBuffer[OFFSET(row + r, col + 1, 240)/2], DMA_SOURCE_FIXED | (width/2 - 1));
                setPixel4(row + r, col + width - 1, colorIndex);
            }
        }
    } else {
        if (width & 1) {
            for (int r = 0; r < height; r++) {
                DMANow(3, &pixels, &videoBuffer[OFFSET(row + r, col, 240)/2], DMA_SOURCE_FIXED | (width/2));
                setPixel4(row + r, col + width - 1, colorIndex);
            }
        } else {
            for (int r = 0; r < height; r++) {
                 DMANow(3, &pixels, &videoBuffer[OFFSET(row + r, col, 240)/2], DMA_SOURCE_FIXED | (width/2));
            }
        }
    }
}

void drawBackgroundImage4(const unsigned short* image) {
    DMANow(3, (unsigned short*)image, videoBuffer, (240*160)/2);
}

void drawImage4(const unsigned short* image, int row, int col, int height, int width) {
    if (col & 1) {
        col += 1;
    }
    for(int i = 0; i < height; i++) {
        DMANow(3, (unsigned short*)&image[OFFSET(i, 0, width)/2], &videoBuffer[OFFSET(row+i, col, SCREENWIDTH)/2], (width/2));
    }
}

void loadPalette(const unsigned short* palette)
{
    DMANow(3, palette, PALETTE, 256);
}

void fillScreen4(unsigned char colorIndex) {
    volatile unsigned short pixels = (colorIndex << 8) | (colorIndex);
    DMANow(3, &pixels, videoBuffer, ((240*160) / 2) | DMA_SOURCE_FIXED);
}

void flipPage() {
    if(REG_DISPCTL & BACKBUFFER)
    {
        REG_DISPCTL &= ~BACKBUFFER;
        videoBuffer = backBuffer;
    }
    else
    {
        REG_DISPCTL |= BACKBUFFER;
        videoBuffer = frontBuffer;
    }
}

//Mode0 Functions
void loadBgTiles(volatile const unsigned short * tiles, unsigned int tilesLen, int cbb)
{
    DMANow(3, tiles, &CHARBLOCKBASE[cbb], tilesLen);
}

void loadBgMap(volatile const unsigned short * map, unsigned int mapLen, int sbb)
{
    DMANow(3, map, &SCREENBLOCKBASE[sbb], mapLen);
}

void loadSpritePalette(volatile const unsigned short * spritePal)
{
    DMANow(3, spritePal, SPRITE_PALETTE, 256);
}

void loadSpriteTiles(volatile const unsigned short * spriteTiles, unsigned int spriteTilesLen)
{
    DMANow(3, spriteTiles, &CHARBLOCKBASE[4], spriteTilesLen/2);
}

void loadMap(const unsigned short * map, unsigned short mapLen, unsigned short palIndex, unsigned short sbb)
{
    unsigned short newMap[mapLen/2];

    for(int i = 0; i < mapLen/2; i++)
    {
        unsigned short mask = map[i] & ~(PALBANK(0xF));
        newMap[i] = mask | PALBANK(palIndex);
    }
    
    DMANow(3, newMap, &SCREENBLOCKBASE[sbb], mapLen/2);
}

void paletteSwap(int src, int dst) {
    PALETTE[src] = PALETTE[dst];
}