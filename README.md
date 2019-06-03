# BitMapRevcolorSIMD 矩阵 BMP 位图处理算法随想

__！！！__ 此项目包括以下学习：

+ CMake + C11 程序开发
+ 某种 BitMap 格式读写
+ SIMD 单指令多数据计算，这里使用 x86 架构的 MMX

推荐使用 KDevelop 进行开发，话说 KDEvelop 对开发工具的集成好哇，代码高亮很用心、Go to definition、Quick fix 很方便

<sup>﹗</sup> 项目 __仅为随想__，可能有 __严重__ 的安全和健壮性（比如未处理的系统 API `errno` exception safety）、兼容性、性能问题，请 __不要__ 在生产环境中使用 __任何代码__（包括位图操作和 BMP 文件序列化基础）！

>往下面翻，到最底下应该可以看到输出实例图片，若是流量党误入十分危险，要知道 BMP 可是相当古老的图像格式了，PNG 能用 100K 的它硬是要免压缩 1.5M 才可能存下来

## 命令行参数 Usage

+ `bitmaprevcolorsimd` 你好世界！
+ `bitmaprevcolorsimd <filename>` 测试对文件的解析
+ `bitmaprevcolorsimd <source> <dest>` 写到文件，将会根据环境变量判断采取一串行动

### 环境变量 Environ Variables

以下操作以 source 为输入 dest 为输出叠加执行（实际上所有操作叠加式在 source 的 buffer 里进行）

- `INV=` 只要不为空，进行串行（scalar）反色
- `FASTINV=` 只要不为空，进行 MMX SIMD 指令集加速反色（不一定是反色，实际上是以 8 像素，四个 `long long int` 为单位对 `unsigned int` 位图进行矩阵处理）
- `DENSE=` 只要不为空，进行 Dense 操作，如果格式 `DENSE=xskip,yskip` 正确且 `xskip`, `yskip` 皆大于零，以他们为循环步长，否则以默认值 1,1 为 dense 像素点的间隔（直接由循环步长决定）

- `MASK=` 只要不为空，设置 FASTINV AND 和 XOR 操作的 bitmask，预期 16 进制值
- `ALPHA=` 尝试支持 Alpha 色值
- `THR=`只要不为空，尝试并发多线程执行 SIMD 操作，我不知道这（设置正确的 `thread_local` 后）有没有可能或者会不会更慢！注意，__目前的所有测试都失败了__
- `XOR=` 只要不为空，FASTINVCOLOR 将对每个像素（准确地说是对每两个像素）执行 XOR 运算而非 AND 运算，这有可能实现反色

- `DBG=` 只要不为空，打印调试信息

调试信息包括

+ INV 的 `(x,y)=#AARRGGBB`
+ THR 的 `Starting new thread (%u) at %li` 和 `Finished thread (%u) at %li`
+ 矩阵处理的 `Row %i = 0x%zx;  XMM 0x%zx: %lli %lli %lli %lli;   = %lli %lli %lli %lli`

## 编译 Compilation

编译可能需要你的操作系统支持线程（比如 `pthread`，`CMAKE_THREAD_LIBS_INIT`），虽然不是所有操作都需要多线程 ~~而且，需要多线程的操作现在没法使用，永远段错误~~

你需要 GCC, Clang 等支持 `-std=c11` 标准的编译器、以一个小端字序的机器为目标（这样许多 ARM 机器上可能就不能编译了）

此外，为了支持 SIMD 计算，还要支持 `-march=core2 -mmmx -msse` 选项，并且有如下断言

```c11
_Static_assert(sizeof(unsigned int) == 4, "MMXr size");
_Static_assert(sizeof(long long) == 8, "MMXr size");
```

如果你的机器有所不同，可以以 2 为参数取商或者积

```bash
[DuangSUSE@duangsuse]~/projects/Essay-DIP-BMP-SIMD-Operation% cmake -S . -B build
-- The C compiler identification is GNU 8.3.1
-- Check for working C compiler: /usr/bin/cc
-- Check for working C compiler: /usr/bin/cc -- works
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Detecting C compile features
-- Detecting C compile features - done
-- Looking for pthread.h
-- Looking for pthread.h - found
-- Looking for pthread_create
-- Looking for pthread_create - not found
-- Looking for pthread_create in pthreads
-- Looking for pthread_create in pthreads - not found
-- Looking for pthread_create in pthread
-- Looking for pthread_create in pthread - found
-- Found Threads: TRUE  
-- Configuring done
-- Generating done
-- Build files have been written to: /home/DuangSUSE/projects/Essay-DIP-BMP-SIMD-Operation/build

[DuangSUSE@duangsuse]~/projects/Essay-DIP-BMP-SIMD-Operation% pushd build 

[DuangSUSE@duangsuse]~/projects/Essay-DIP-BMP-SIMD-Operation/build% make -j4
Scanning dependencies of target libbmp
[ 25%] Building C object libbmp/CMakeFiles/libbmp.dir/bmp.c.o
[ 50%] Linking C static library liblibbmp.a
[ 50%] Built target libbmp
Scanning dependencies of target bitmaprevcolorsimd
[ 75%] Building C object CMakeFiles/bitmaprevcolorsimd.dir/main.c.o
[100%] Linking C executable bitmaprevcolorsimd
[100%] Built target bitmaprevcolorsimd
```

## 快速过一遍！Skim through

### CMake 项目管理

> libbmp

```cmake
cmake_minimum_required(VERSION 3.0)

project (libbmp LANGUAGES C)

set (CMAKE_INCLUDE_CURRENT_DIR ON)
set (CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -std=c11)

add_library (libbmp STATIC bmp.c)
```

> bitmaprevcolorsimd

```cmake
cmake_minimum_required(VERSION 3.10)

project(bitmaprevcolorsimd LANGUAGES C)

add_subdirectory(libbmp)

set (CMAKE_INCLUDE_CURRENT_DIR ON)

set (CMAKE_C_FLAGS "-march=core2 -mmmx -msse")

set (CMAKE_THREAD_LIBS_INIT ON)
find_package(Threads REQUIRED)

add_executable(bitmaprevcolorsimd main.c)

target_link_libraries(bitmaprevcolorsimd libbmp Threads::Threads)

install(TARGETS bitmaprevcolorsimd RUNTIME DESTINATION bin)
```

### C11 源代码 :: libbmp

> 时间仓促，完成度低！ __只断言支持原生小端字序（Little Endian）目标机器__ （没有写字节序翻转的算法）

```c
typedef struct Bmp {
  struct BmpHeader header;
  struct DibHeader dib;

  unsigned int *inline_bytes;
  size_t inline_bytes_size;

  pixel_color *pixels;

  unsigned int *tail_bytes;
  size_t tail_bytes_size;
} Bmp;

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
```

```c
void writeBmpFile(FILE *dst, const Bmp src) {
  rewind(dst);
  if (errno != 0) return; //...
  fwrite(BM_MAGIC, sizeof(char), 2, dst);
  fwrite(&src.header, sizeof(BmpHeader), 1, dst);
  //fwrite(&src.inline_bytes_size, sizeof(uint32_t), 1, dst);
  fwrite(src.inline_bytes, sizeof(uint8_t), src.inline_bytes_size, dst);
  fwrite(src.tail_bytes, sizeof(uint8_t), src.tail_bytes_size, dst);
}
```

### C11 源代码 :: main.c

#### 简单处理部分

```c
if (getenv("INV"))
for (unsigned int row = 0; row <bmp.dib.height; row +=ystep) {
  for (unsigned int col = 0; col <bmp.dib.width; col +=xstep) {
  uint32_t *pos = &bmp.pixels[row*bmp.dib.width+col];
  uint32_t pix = *pos;
  cp a = pix<<24, r = pix<<16, g = pix<<8, b = pix;

  if (debug) printf("(%u,%u)=#%02i%02i%02i%02i ", row,col, a, r, g, b);

  *pos = alpha ? ~(*pos ^ 0xFF000000) : ~*pos;
  }
  if (debug)printf("\n");
}
```

#### Dense 处理部分

这个部分稍微改造一点，可以分析并作为 [ASCII 字符画生成器](https://gist.github.com/duangsuse/8261454d4c4157f2bb8ccf233a00d48a)使用

```c
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
```

#### SIMD 处理部分

> 时间仓促，完成度低！ __只支持所有位图宽度为 8 整数倍的图像！不然处理程序将直接返回__

```c
static void handleEightPixel(ullptr scan1, ullptr scan0, unsigned row) {
  ullptr
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
  _y0 = _mm_cvtm64_si64(x0), _y1 = _mm_cvtm64_si64(x1),
  y2 = _mm_cvtm64_si64(x2), y3 = _mm_cvtm64_si64(x3);

  if (debug) {
    printf("Row %i = 0x%zx\n", row, (size_t)(void *) scan0);
    printf(" XMM 0x%zx: %lli %lli %lli %lli", (size_t)(void *) scan1, *p0, *p1, *p2, *p3);
    printf("  = %lli %lli %lli %lli\n", *p0, *p1, *p2, *p3);
  }

  *p0=_y0,*p1=_y1,*p2=y2,*p3=y3;
}

static void handleRow(unsigned int *matrix, unsigned int row) {
  long long *scan0 = (long long *) &matrix[row*row];

  void (*proced)(ullptr, ullptr, unsigned) = (usexor ? handleEightPixelJustInv : handleEightPixel);
  for (long long *scan1 = scan0 +stride, *end = scan0-4; scan1 !=end; scan1 -=4) {
    proced((ullptr) scan1, (ullptr) scan0, row);
  }
}

void fastinv(const Bmp img) {
  //...
  /*...*/ else for (int row = h; row !=-1; --row)handleRow(pix, row);
}
```

## License

> 时间仓促没有去弄 Licence，代码基本不可复用，随便弄算了

## 滑稽 🌝 Funny

来自 [Telegram 滑稽表情包]，滑稽生活，健康你我！（滑稽）

[Telegram 滑稽表情包]: https://t.me/addstickers/TiebaStickers

## 相关链接 See Also

+ [C# GTK# 多线程反色](https://github.com/duangsuse-valid-projects/Essay-CSharp-DIP.Performance.Parallel)
+ [AsciiKt](https://github.com/duangsuse-valid-projects/Essay-Kotlin-AsciiArt/)
+ [Img2Ascii](https://gist.github.com/duangsuse/8261454d4c4157f2bb8ccf233a00d48a)

## 算法输出？比如说？ Images （多图慎滑，当然主动加载的可别怪我）

这些是<ruby>我不记得参数<rt>蠢人啊 <img alt="(滑稽)" src="huaji.bmp" width="12" /></rt></ruby>了的图片 ![bits](exp.bmp)

<p align="center">
 <img alt="avatar" src="avatar.bmp" width="30%" />
</p>

就可以变成

类型 | 结果
:-- | :-:
FastInv(<abbr title="位逻辑与运算 &">AND</abbr>) | ![finvand](out.bmp)
[FastInv(XOR)](#note1) | ![finvand](out4.bmp)
FastInv(AND) blue | ![finvand1](out.png)
Inv | ![inverse](out0.bmp)
Dense | ![dense](out1.bmp)
Dense&FastInv(AND) | ![densefinv](out3.bmp)

<p id="my-machine">测试都在 <b>Intel(R) Core(TM) i5-7500 CPU @ 3.40GHz</b> 微处理器上进行</p>

<p id="note1"><code>DBG= FASTINV= XOR= MASK=00ff00aa ./build/bitmaprevcolorsimd out1.bmp out4.bmp</code> <a href="#my-machine">我这里</a> 1858 cpu time (user+system) 即可完成</p>

