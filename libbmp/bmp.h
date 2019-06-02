#pragma once

#include <stdint.h>
#include <stdio.h>

// by duangsuse 2019 Jun
// BMP File struct reader / writer

// See https://en.wikipedia.org/wiki/BMP_file_format#File_structure

// ICC Color profile is NOT supported

// ... Unsupport all features except for things needed for implementing reverse color...

#ifndef __ORDER_LITTLE_ENDIAN__
#  error "Big endian machine is NOT supported"
#endif
// Big endian machine is NOT supprted

// 14 bytes

static const char BM_MAGIC[2] = { 'B', 'M' };

typedef enum BmpError {
  BadFile,
  BadMagic,
  BadSize,
  CantAlloc,
  None = 0
} BmpError;

static enum BmpError BMP_Errno = None;
static char *BMP_Error;

/** To store general information about the bitmap image file */
typedef struct BitMapHeader {
  int8_t dummy[2]; // BM

  int32_t size; // bytes 4

  int16_t reservd0, reservd1;

  int32_t offset_pixels;
} BitMapHeader;

_Static_assert(sizeof(BitMapHeader) - (_Alignof(BitMapHeader) - 1*sizeof(int8_t [2])) == 14, "BitMapHeader size must of 14 bytes");


/** Duplicate! */
typedef struct BmpHeader {
  int32_t size; // bytes 4

  int16_t reservd0, reservd1;

  int32_t offset_pixels;
} BmpHeader;

_Static_assert(sizeof(BmpHeader) == 14 - 2, "sizeof(BmpHeader) must be 14 - 2 in order to read directly into memory");

/** To store detailed information about the bitmap image and define the pixel format */
typedef struct DibHeader {
  uint32_t size;
  uint32_t width, height;

  uint16_t reservd;
  uint16_t colors;
} DibHeader;

typedef uint32_t pixel_color;

typedef struct Bmp {
  struct BmpHeader header;
  struct DibHeader dib;

  /** All bytes from header.offset_pixels to pixels */
  unsigned int *inline_bytes;
  size_t inline_bytes_size;

  /** Why pointer? Guess! */
  pixel_color *pixels;

  /** All bytes from pixels array to EOF */
  unsigned int *tail_bytes;
  size_t tail_bytes_size;
} Bmp;

Bmp readBmpFile(FILE *fp);
void writeBmpFile(FILE *dst, const Bmp src);
void bmpFree(Bmp obj);

/** To define colors used by the bitmap image data (Pixel array)  */
struct ColorTable {};

/** Structure alignment  */
struct Gap1 {};

typedef struct BitMap {
  struct BitMapHeader header;
  //struct DibHeader dib;

  /** To define the pixel format */
  int64_t ext_bit_masks[4];

  /** Dynamic size members throgh pointers */
  struct ColorTable *colors;
  struct Gap1 *gap1;

  /** To define the actual values of the pixels */
  int32_t pixels[]; // flexible
} BitMap;


