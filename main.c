#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <libbmp/bmp.h>
#include <errno.h>
#include <stdbool.h>

#include <threads.h>

/* ----

  Essay - SIMD BMP inv color & dense

   ---- */

const static char *Env_DBG = "DBG";

static void fastinv(const Bmp img);

static void cflush() {
  fflush(stdout);
}

static void iflush_exit() {
  cflush();
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  puts("Hello, World!");

  if (argc <2) return -1;

  FILE *fbmp = fopen(argv[1], "rb");

  Bmp bmp = readBmpFile(fbmp);

  printf("Bmp file (%s):\nFile size: %i\nImagable starts: %i\nAllocated pixel starts: 0x%zx\n", argv[1], bmp.header.size, bmp.header.offset_pixels, (size_t) bmp.pixels);

  printf("Bmp DIB Header\n size: %i\n (w, h): (%i, %i)\n resrvd, colors: %hu,%hu\n", bmp.dib.size, bmp.dib.width, bmp.dib.height, bmp.dib.reservd, bmp.dib.colors);

  /**
  unsigned pointbs = bmp.dib.colors;
  unsigned stride = bmp.dib.width *1;
  typedef unsigned int cp;
  unsigned xstep = 2, ystep = 2;
  for (unsigned int row = 0; row !=bmp.dib.height; row +=ystep)
  for (unsigned int col = 0; col !=bmp.dib.width; col +=xstep) {
    cp
     c0 = bmp.pixels[row*stride + col],
     c1 = bmp.pixels[row*stride + (col-1)],
     c2 = bmp.pixels[(row-1)*stride + col],
     c3 = bmp.pixels[(row-1)*stride + (col-1)];

    cp sum = c0 + c1 + c2 + c3;
    cp average = sum / 4;

    bmp.pixels[row*stride + col] = average;
  }
  */

  if (bmp.dib.colors != 4) exit(EXIT_FAILURE);

  signal(SIGSEGV, iflush_exit);

  // x->+Infinity; y->+Infinity

  // ...
  // (1, 0) (1, 1) (1, 2) ...
  // (0, 0) (0, 1) (0, 2) ...

  unsigned xstep = 1, ystep = 1;
  typedef uint8_t cp;

  clock_t started = clock();

  bool debug = getenv(Env_DBG) !=NULL;

  if (getenv("INV"))
    for (unsigned int row = 0; row <bmp.dib.height; row +=ystep) {
      for (unsigned int col = 0; col <bmp.dib.width; col +=xstep) {
        uint32_t *pos = &bmp.pixels[row*bmp.dib.width+col];
        uint32_t pix = *pos;
        cp a = pix<<24, r = pix<<16, g = pix<<8, b = pix;

        if (debug) printf("(%u,%u)=#%02i%02i%02i%02i ", row,col, a, r, g, b);

        *pos = ~*pos;
      }
      if (debug)printf("\n");
    }
  printf("Finished in %li\n", clock() -started);

  if (getenv("FASTINV")) fastinv(bmp);

  unsigned w = bmp.dib.width;
  char *denseenv = getenv("DENSE");
  if (denseenv ==NULL) goto skipDense;

  char *dense = malloc(strlen(denseenv));
  strcpy(dense, denseenv);

  char *saved_tokstate;
  char *tok;

  unsigned rn, cn;

  printf("DENSE: %s ", dense);
  tok = strtok_r(dense, ",", &saved_tokstate);
  rn = atoi(tok !=NULL?tok: "1");
  tok = strtok_r(NULL, ",", &saved_tokstate);
  cn = atoi(tok !=NULL?tok: "1");

  //if (rn ==0 || cn ==0) { rn=1, cn=1; }

  printf("skip w=%i, h=%i\n", rn, cn);

  // just use 4 pixels to figure out what color to be replaced with
  for (unsigned int row = 0; row <bmp.dib.height; row +=rn) {
    for (unsigned int col = 0; col <bmp.dib.width; col +=cn) {

      cp
      c00 = bmp.pixels[row*w + col] &0xFF000000,
      c01 = bmp.pixels[row*w + (col+1)],
      c10 = bmp.pixels[(row+1)*w + col] &0x0a1000bb,
      c11 = bmp.pixels[(row+1)*w + (col+1)] &0x00aa1100;

      cp sum
        = c10 + c11;
      cp sum2
        = c00 + c01;

      cp average = (sum / 2) + sum2 / 2; // >>2

      bmp.pixels[row*w + col] = average;
    }
    //printf("\n");
  }

skipDense:
  if (isatty(fileno(stdout))) signal(SIGINT, cflush);

  if (argc == 3) {
    FILE *fout = fopen(argv[2], "wb+");
    fprintf(stderr, "Writing to %s\n", argv[2]);
    writeBmpFile(fout, bmp);
    fflush(fout);
    fclose(fout);
  }

  bmpFree(bmp);

  printf("ok\n");

  exit(EXIT_SUCCESS);
}

#include <xmmintrin.h>

// MMX: process 8 pixels in a roll
thread_local __m64 arg0;
thread_local __m64 x0, x1, x2, x3;
thread_local long long y0_, y1_, y2, y3;

static size_t w, h;
static size_t stride;

bool debug = false;

typedef long long *llptr;

static void handleEightPixel(long long *scan1, long long *scan0, unsigned row) {
  llptr
  p0=scan1 +0,
  p1=scan1 +1,
  p2=scan1 +2,
  p3=scan1 +3;

  // 4 * m128 -> 8 * u64
  x0 = _mm_cvtsi64_m64(*p0);
  x1 = _mm_cvtsi64_m64(*p1);
  x2 = _mm_cvtsi64_m64(*p2);
  x3 = _mm_cvtsi64_m64(*p3);

  // inverse color??? May not. just for fun.
  x0 = _mm_and_si64(x0, arg0);
  x1 = _mm_and_si64(x1, arg0);
  x2 = _mm_and_si64(x2, arg0);
  x3 = _mm_and_si64(x3, arg0);

  // do output
  y0_ = _mm_cvtm64_si64(x0), y1_ = _mm_cvtm64_si64(x1),
  y2 = _mm_cvtm64_si64(x2), y3 = _mm_cvtm64_si64(x3);

  if (debug) {
    printf("Row %i = 0x%zx\n", row, (size_t)(void *) scan0);
    printf(" XMM 0x%zx: %lli %lli %lli %lli", (size_t)(void *) scan1, *p0, *p1, *p2, *p3);
    printf("  = %lli %lli %lli %lli\n", *p0, *p1, *p2, *p3);
  }

  *p0=y0_,*p1=y1_,*p2=y2,*p3=y3;
}

static void handleEightPixelJustInv(long long *scan1, long long *scan0, unsigned row) {
  llptr
  p0=scan1,
  p1=scan1 +1,
  p2=scan1 +2,
  p3=scan1 +3;

  // 4 * m128 -> 8 * u64
  x0 = _mm_cvtsi64_m64(*p0);
  x1 = _mm_cvtsi64_m64(*p1);
  x2 = _mm_cvtsi64_m64(*p2);
  x3 = _mm_cvtsi64_m64(*p3);

  // inverse color??? May not. just for fun.
  x0 = _mm_xor_si64(x0, arg0);
  x1 = _mm_xor_si64(x1, arg0);
  x2 = _mm_xor_si64(x2, arg0);
  x3 = _mm_xor_si64(x3, arg0);

  // do output
  y0_ = _mm_cvtm64_si64(x0), y1_ = _mm_cvtm64_si64(x1),
  y2 = _mm_cvtm64_si64(x2), y3 = _mm_cvtm64_si64(x3);

  if (debug) {
    printf("Row %i = 0x%zx\n", row, (size_t)(void *) scan0);
    printf(" XMM 0x%zx: %lli %lli %lli %lli", (size_t)(void *) scan1, *p0, *p1, *p2, *p3);
    printf("  = %lli %lli %lli %lli\n", *p0, *p1, *p2, *p3);
  }

  *p0=y0_,*p1=y1_,*p2=y2,*p3=y3;
}

bool usexor;

static void handleRow(unsigned int *matrix, unsigned int row) {
  long long *scan0 = (long long *) &matrix[row*row];

  void (*proced)(long long *, long long *, unsigned) = (usexor ? handleEightPixelJustInv : handleEightPixel);
  for (long long *scan1 = scan0 +stride, *end = scan0-4; scan1 !=end; scan1 -=4) {
    proced(scan1, scan0, row);
  }
}

typedef struct RowRtnT {
  unsigned int *matr;
  unsigned int n;
} RowRtnT;

int handleRowThreadRtn(void *args) {
  struct RowRtnT as = *(struct RowRtnT *) args;
  if (debug) printf("Starting new thread (%u) at %li\n", as.n, clock());
  handleRow(as.matr, as.n);
  if (debug) printf("Finished thread (%u) at %li\n", as.n, clock());
  return 0;
}

void fastinv(const Bmp img) {
  clock_t begtime = clock();

  _Static_assert(sizeof(unsigned int) == 4, "MMXr size");
  _Static_assert(sizeof(long long) == 8, "MMXr size");

  unsigned int *pix = img.pixels;
  w = img.dib.width, h = img.dib.height;
  stride = w/2;

  int mask = 0x0000EC00;
  char *maskenv;
  if ((maskenv = getenv("MASK")) !=NULL) sscanf(maskenv, "%x", &mask);

  debug = getenv(Env_DBG) !=NULL;

  arg0 = _mm_cvtsi64_m64(mask);

  bool parallel = getenv("THR") !=NULL;
  usexor = getenv("XOR") !=NULL;

  if (w % sizeof(long long) != 0) return; // just a test!
  // process in a row
  thrd_t threads[h];
  unsigned int cthr = 0;
  if (parallel) for (int row = h; row !=-1; --row) {
      RowRtnT args = { .matr=pix, .n=row };
      thrd_create(&threads[cthr++], handleRowThreadRtn, (void *) &args);
    } else for (int row = h; row !=-1; --row)handleRow(pix, row);

  for (; cthr >0; cthr--) thrd_join(threads[cthr], NULL);
  _mm_empty();

  printf("SIMD invcolor finished in %li\n", clock() - begtime);
}

