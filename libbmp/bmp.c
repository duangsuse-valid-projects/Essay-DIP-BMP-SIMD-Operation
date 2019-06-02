#include <bmp.h>

#include <string.h>
#include <errno.h>
#include <malloc.h>

const char *BMP_Error_BadMagic = "Bad file magic";
const char *BMP_Error_BadSize = "Bad file size in header field";
const char *BMP_Error_BadFile = "Cannot read file";
const char *BMP_Error_CantAlloc = "Cannot make allocation";

static inline int flength(FILE *fp) {
  // not cancellable (EINTR)
  fpos_t oldpos;
  int length;

  fgetpos(fp, &oldpos);
  if (errno != 0) return -1;

  // non failable
  fseek(fp, 0, SEEK_END);
  length = ftell(fp);
  fsetpos(fp, &oldpos);

  return length;
}

Bmp readBmpFile(FILE *fp) {
  Bmp bmp = {};
  rewind(fp);

  // check header
  int8_t magic[2];
  fread(magic, sizeof(int8_t), 2, fp);
  if (errno != 0) {
    BMP_Errno = BadFile;
    BMP_Error = (char *)BMP_Error_BadFile;
    goto out;
  };

  int fsize = flength(fp);

  // not failable
  if (!(memcmp(&magic, &BM_MAGIC, 2) == 0)) {
    BMP_Errno = BadMagic;
    BMP_Error = (char *)BMP_Error_BadMagic;
    goto out;
  }

  // read header structure
  BmpHeader header;

  fread(&header, sizeof(BmpHeader), 1, fp);

  if (header.size != fsize) {
    BMP_Errno = BadSize;
    BMP_Error = (char *)BMP_Error_BadSize;
    goto out;
  };

  bmp.header = header;

  //printf("Pixel offset %i\nFtell %li\nFsize %i\n", header.offset_pixels, ftell(fp), fsize);
  // read inline bytes
  int32_t inline_size;

  fread(&inline_size, sizeof(int32_t), 1, fp);
  fseek(fp, -sizeof(int32_t), SEEK_CUR);

  bmp.inline_bytes = malloc(inline_size);
  bmp.inline_bytes_size = inline_size;
  if (errno == ENOMEM) {
    BMP_Errno = CantAlloc;
    BMP_Error = (char *)BMP_Error_CantAlloc;
    goto out;
  };
  fread(bmp.inline_bytes, sizeof(uint8_t), inline_size, fp);

  /* not volatile */ size_t ptr = 0;
  // read DIB information
  bmp.dib.size = bmp.inline_bytes[ptr];
  bmp.dib.width = bmp.inline_bytes[++ptr], bmp.dib.height = bmp.inline_bytes[++ptr];
  uint32_t resvdxcolors = bmp.inline_bytes[++ptr];

  bmp.dib.reservd = (0xFFFF & resvdxcolors);
  bmp.dib.colors = (0xFFFF & resvdxcolors << sizeof(uint16_t));

  // read pixel array into tail bytes
  int tail_size = fsize - header.offset_pixels; // sizeof(uint16_t) * 14 /* exclusive header, it's safe */;
  bmp.tail_bytes = malloc(tail_size);
  bmp.tail_bytes_size = tail_size;
  if (errno == ENOMEM) {
    BMP_Errno = CantAlloc;
    BMP_Error = (char *)BMP_Error_CantAlloc;
    goto out;
  };
  fseek(fp, header.offset_pixels, SEEK_SET); //fseek(fp, 1, SEEK_CUR);
  fread(bmp.tail_bytes, sizeof(uint8_t), tail_size, fp);

  bmp.pixels = (pixel_color *) bmp.tail_bytes;
out:
  return bmp;
}

void writeBmpFile(FILE *dst, const Bmp src) {
  rewind(dst);
  if (errno != 0) return; //...
  fwrite(BM_MAGIC, sizeof(char), 2, dst);
  fwrite(&src.header, sizeof(BmpHeader), 1, dst);
  //fwrite(&src.inline_bytes_size, sizeof(uint32_t), 1, dst);
  fwrite(src.inline_bytes, sizeof(uint8_t), src.inline_bytes_size, dst);
  fwrite(src.tail_bytes, sizeof(uint8_t), src.tail_bytes_size, dst);
}

void bmpFree(Bmp obj) {
  free(obj.inline_bytes);
  free(obj.tail_bytes);
}

