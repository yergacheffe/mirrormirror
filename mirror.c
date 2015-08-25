// first OpenVG program
// Anthony Starks (ajstarks@gmail.com)
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "VG/openvg.h"
#include "VG/vgu.h"
#include "EGL/egl.h"
#include "GLES/gl.h"
#include "bcm_host.h"
//#include "fontinfo.h"
//#include "shapes.h"
#include <jpeglib.h>
#include <gd.h>

#define null ((void *)0L)

#define RGB(r, g, b) ((r << 16) + (g << 8) + b)

#define R(c) ((c & 0xFF0000) >> 16)
#define G(c) ((c & 0xFF00) >> 8)
#define B(c) (c & 0xFF)

#define GREY(c) ((R(c) + G(c) + B(c)) / 3)
#define MIX(a, b, c) (a + (((b - a) * c) / 0xFF))

#define RGBMIX(c1, c2, d) \
  (RGB(MIX(R(c1), R(c2), d), MIX(G(c1), G(c2), d), MIX(B(c1), G(c2), d)))

#define xxxFAKE_INPUT 1

uint32_t hsv2rgb(float hue, float sat, float val)
{
    double      hh, p, q, t, ff;
    long        i;
    int         red, green, blue;
    
    hh = hue;

    if(sat <= 0.0) 
    {       
        red   = val;
        green = val;
        blue  = val;
    }
    else
    {
      if(hh >= 360.0) hh = hh-360.0;
      hh /= 60.0;
      i = (long)hh;
      ff = hh - i;
      p = val * (1.0 - sat);
      q = val * (1.0 - (sat * ff));
      t = val * (1.0 - (sat * (1.0 - ff)));
  
      switch(i) {
      case 0:
          red = val * 255.0;
          green = t * 255.0;
          blue = p * 255.0;
          break;
      case 1:
          red = q * 255.0;
          green = val * 255.0;
          blue = p * 255.0;
          break;
      case 2:
          red = p * 255.0;
          green = val * 255.0;
          blue = t * 255.0;
          break;
  
      case 3:
          red = p * 255.0;
          green = q * 255.0;
          blue = val * 255.0;
          break;
      case 4:
          red = t * 255.0;
          green = p * 255.0;
          blue = val * 255.0;
          break;
      case 5:
      default:
          red = val  * 255.0;
          green = p * 255.0;
          blue = q * 255.0;
          break;
      }
    }
   uint32_t result = (((uint32_t)red) << 16) | 
                     (((uint32_t)green) << 8) |
                     (((uint32_t)blue));
                     
   return result;
}
          
          

VGImage loadJpeg(const char *filename, VGubyte **outData, unsigned int *outWidth, unsigned int *outHeight ) {
	FILE *infile;
	struct jpeg_decompress_struct jdc;
	struct jpeg_error_mgr jerr;
	JSAMPARRAY buffer;
	unsigned int bstride;
	unsigned int bbpp;

	VGImage img;
	VGubyte *data;
	unsigned int width;
	unsigned int height;
	unsigned int dstride;
	unsigned int dbpp;

	VGubyte *brow;
	VGubyte *drow;
	unsigned int x;
	unsigned int lilEndianTest = 1;
	VGImageFormat rgbaFormat;

	// Check for endianness
	if (((unsigned char *)&lilEndianTest)[0] == 1)
        {
fprintf(stdout, "VG_sABGR_8888\n");
		rgbaFormat = VG_sABGR_8888;
}
	else
{
fprintf(stdout, "VG_sRGBA_8888\n");
		rgbaFormat = VG_sRGBA_8888;
}

	// Try to open image file
	infile = fopen(filename, "rb");
	if (infile == NULL) {
		printf("Failed opening '%s' for reading!\n", filename);
		return VG_INVALID_HANDLE;
	}
	// Setup default error handling
	jdc.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&jdc);

	// Set input file
	jpeg_stdio_src(&jdc, infile);

	// Read header and start
	jpeg_read_header(&jdc, TRUE);
	jpeg_start_decompress(&jdc);
	width = jdc.output_width;
	height = jdc.output_height;
	if (outWidth)
          *outWidth = width;
        if (outHeight)
          *outHeight = height;

	// Allocate buffer using jpeg allocator
	bbpp = jdc.output_components;
	bstride = width * bbpp;
	buffer = (*jdc.mem->alloc_sarray)
	    ((j_common_ptr) & jdc, JPOOL_IMAGE, bstride, 1);

	// Allocate image data buffer
	dbpp = 4;
	dstride = width * dbpp;
	data = (VGubyte *) malloc(dstride * height);

	// Iterate until all scanlines processed
	while (jdc.output_scanline < height) {

		// Read scanline into buffer
		jpeg_read_scanlines(&jdc, buffer, 1);
		drow = data + (height - jdc.output_scanline) * dstride;
		brow = buffer[0];
		// Expand to RGBA
		for (x = 0; x < width; ++x, drow += dbpp, brow += bbpp) {
			switch (bbpp) {
			case 4:
				drow[0] = brow[0];
				drow[1] = brow[1];
				drow[2] = brow[2];
				drow[3] = brow[3];
				break;
			case 3:
				drow[0] = brow[0];
				drow[1] = brow[1];
				drow[2] = brow[2];
				drow[3] = 255;
				break;
			}
		}
	}

	// Create VG image
	img = vgCreateImage(rgbaFormat, width, height, VG_IMAGE_QUALITY_BETTER);
	vgImageSubData(img, data, dstride, rgbaFormat, 0, 0, width, height);

	// Cleanup
	jpeg_destroy_decompress(&jdc);
	fclose(infile);
        if (outData)
        {
          *outData = data;
        }
        else
          free(data);

	return img;
}

int getLuma(x, y, w, h)
{


}

int initCalled = 0;
int windowWidth, windowHeight;


void myrect(unsigned char* pBuffer, int x, int y, int w, int h, int luma)
{
	uint32_t color = hsv2rgb(1.0f * (rand()%360), 1.0, (float)(luma / 255.0));

	unsigned char *pScanline = &pBuffer[y*windowWidth*4 + x*4];
	while (h-- > 0)
	{
		int xwalk;
		for (xwalk=0; xwalk<w;++xwalk)
		{
			pScanline[xwalk*4+3]   = 255;
			pScanline[xwalk*4+0] = color >> 16;
			pScanline[xwalk*4+1] = (color >> 8) & 255;
			pScanline[xwalk*4+2] = color & 255;
		}

		pScanline += windowWidth*4;
	}
}

unsigned char *buf;

void processMirrorFrame(uint8_t *pFrame, int width, int height)
{
  if (!initCalled)
  {
    init(&windowWidth, &windowHeight);
    initCalled = 1;
    buf = malloc(windowWidth * windowHeight * 4);
  }
  // fprintf(stdout, "Calling start x=%d, y=%d\n", w, h);
  // fprintf(stdout, "VGErr=%d\n", vgGetError());
 
#ifdef FAKE_INPUT
  // Load jpeg as source data
  unsigned char *pRawFrame;
  (void) loadJpeg("ill.jpg", &pRawFrame, &width, &height);

  // Now iterate the frame and convert to YaYb
  unsigned char *pSrc = pFrame;
  unsigned char *pDst = pFrame;
  int totalPixels = width * height;

  while (totalPixels--)
  {
    unsigned int luma = (pSrc[0] + pSrc[1] + pSrc[2]) / 3;
    *pDst = luma;

    pSrc +- 4;
    pDst += 2;
  }
 
  pFrame = pRawFrame; 
#endif 
  // Always seed with the same value so we get the same color sequence
  srand(822);

  Start(windowWidth, windowHeight);
  Background(60,0,20);
	Fill(222,222,0,1);
  int walk;

memset(buf, 0, windowWidth*windowHeight*4);

  int x,y; 
  int yCells = 36;
  int xCells = 64;
  int cellWidth = width/xCells;
  int cellHeight = height/yCells;
  int windowCellWidth = windowWidth / xCells;
  int windowCellHeight = windowHeight / yCells;

  for(y = 0; y < yCells; y++)
    for(x = 0; x < xCells; x++)
    {
        uint8_t *pWalker = &pFrame[y*cellHeight*width*2 + x*cellWidth*2];
        uint32_t accumulator = 0;

        int xincell, yincell;
        for (yincell=0; yincell<cellHeight;++yincell)
        {
          for (xincell=0; xincell<cellWidth;++xincell)
          {
		accumulator += pWalker[xincell*2];
          }
          pWalker += width*2;
        }

	accumulator = accumulator / (cellWidth*cellHeight);
        int centerX = x*windowCellWidth + windowCellWidth/2;
        int centerY =  y*windowCellHeight + windowCellHeight/2;
centerY = windowHeight-centerY;
        int radius =  ((VGfloat)accumulator) / 256.0 * windowCellHeight;
radius = radius / 2;
radius = windowCellHeight / 2;
myrect(buf,
        centerX-radius, centerY-radius, radius*2, radius*2, accumulator & 255);
//	ircle(x*windowCellWidth - windowCellWidth/2, y*windowCellHeight - windowCellHeight/2, ((VGfloat)accumulator) / 256.0 * windowCellWidth);
//      int c = gdImageGetPixel(pFrame, x, y);
//      pFrame->tpixels[y][x] = 0xff000000 | R(c) | (G(c)<<8) | (B(c)<<16) ;
//      gdImageSetPixel(pFrame, x, y, 0xff0000ff );
    }

makeimage(0.0,0.0,windowWidth, windowHeight, buf);
 // makeimage(100.0,100.0, gdImageSX(pFrame)+2, gdImageSY(pFrame), (char*)pFrame->tpixels[0]);
  End();

}

int notmain() {
    int width, height;
    char s[3];

    init(&width, &height);                  // Graphics initialization

    VGubyte *pPixels = null;
    VGImage vgImage;
    unsigned int imgWidth, imgHeight;
    vgImage = loadJpeg("poop.jpg", &pPixels, &imgWidth, &imgHeight);
    fprintf(stdout, "%xhh %xhh %xhh %xhh\n", pPixels[0], pPixels[1], pPixels[2], pPixels[3]);
    
    long int pixelCount = imgWidth * imgHeight;
    VGubyte *pFiddle = pPixels;
    while (pixelCount--)
    {
      pFiddle[3] = 0;
      pFiddle += 4;
    }
    vgImageSubData(vgImage, pPixels, imgWidth*4, VG_sRGBA_8888, 0, 0, imgWidth, imgHeight);

    vgSetPixels(100, 100, vgImage, 0, 0, imgWidth, imgHeight);
	End();						   // End the picture



    fgets(s, 2, stdin);                     // look at the pic, end with [RETURN]
    finish();                               // Graphics cleanup
    exit(0);
}

