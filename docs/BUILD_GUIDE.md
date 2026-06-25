# JPEGLI (libjpegli) 构建指南

> 更新时间: 2026-06-25
> 环境: Windows 11 + Visual Studio 2022 + CMake + Ninja

---

## 目录

1. [项目简介](#1-项目简介)
2. [环境要求](#2-环境要求)
3. [获取源码](#3-获取源码)
4. [CMake 配置选项详解](#4-cmake-配置选项详解)
5. [构建步骤](#5-构建步骤)
6. [构建产物](#6-构建产物)
7. [使用示例](#7-使用示例)
8. [构建脚本](#8-构建脚本)
9. [常见问题](#9-常见问题)

---

## 1. 项目简介

**JPEGLI** 是 Google 的下一代 JPEG 编码库（libjxl 子项目），使用 C++17 开发。

核心特性：
- 自适应量化技术，同等文件大小下提供更高视觉质量
- XYB 色彩空间处理
- Highway SIMD 指令集加速（AVX2/SSE4/NEON 等）
- 兼容 libjpeg API

### 命令行工具

| 工具 | 说明 |
|------|------|
| `cjpegli` | JPEG 编码器 — 将 PPM/PNM/PFM/PAM/PGX 压缩为 JPEG |
| `djpegli` | JPEG 解码器 — 将 JPEG 解压为 PPM/PNM/PFM/PAM/PGX |

---

## 2. 环境要求

### 必需工具

| 工具 | 版本要求 | 用途 |
|------|---------|------|
| CMake | >= 3.16 | 构建系统生成器 |
| Ninja | 任意版本 | 构建工具（推荐，比 Makefile 快 2-3 倍） |
| C++17 编译器 | MSVC 2019+ / Clang 7+ / GCC 9+ | 编译 C++ 代码 |
| Git | 任意版本 | 获取源码和子模块 |

### 可选工具

| 工具 | 用途 |
|------|------|
| NASM | x86/x64 SIMD 汇编优化（仅 Linux/macOS 需要） |

### 本项目使用的环境

- **OS**: Windows 11 64-bit
- **编译器**: Microsoft Visual Studio 2022 Community (MSVC 19.44)
- **CMake**: 4.3.3
- **Ninja**: 1.13.0
- **Git**: 2.54.0

---

## 3. 获取源码

### 3.1 克隆仓库（含子模块）

```bash
git clone https://github.com/google/jpegli.git --recursive --shallow-submodules
cd jpegli
```

> **重要**：子模块是必需的，它们包含 Highway SIMD 库、色彩管理库（skcms/lcms2）、zlib 等核心依赖。缺少子模块会导致编译失败。

### 3.2 如果已克隆但缺少子模块

```bash
git submodule update --init --recursive --depth 1 --recommend-shallow
```

### 3.3 子模块清单

| 子模块 | 用途 | 必要性 |
|--------|------|--------|
| `third_party/highway` | Google Highway SIMD 抽象层 | **必需** |
| `third_party/skcms` | Google 色彩管理库 | **必需**（默认） |
| `third_party/zlib` | 压缩库 | 可选（libpng 需要） |
| `third_party/googletest` | 单元测试框架 | 可选（测试用） |
| `third_party/libjpeg-turbo` | libjpeg API 兼容层 | 可选 |
| `third_party/libpng` | PNG 读写库 | 可选 |
| `third_party/lcms` | 备选色彩管理引擎 | 可选（skcms OFF 时） |
| `third_party/sjpeg` | 简易 JPEG 编码器 | 可选 |
| `third_party/apngdis` | APNG 解析 | 可选 |

---

## 4. CMake 配置选项详解

### 4.1 核心选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `CMAKE_BUILD_TYPE` | (空) | `Release` / `Debug` / `RelWithDebInfo` |
| `BUILD_TESTING` | ON | 编译测试（建议 OFF 以加快编译） |
| `JPEGLI_STATIC` | OFF | **静态链接**，生成单文件 exe，无需 DLL |
| `JPEGLI_ENABLE_LTO` | OFF | 链接时优化（仅 Linux GCC/Clang 有效） |

### 4.2 功能开关（按需关闭以加速编译）

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `JPEGLI_ENABLE_TOOLS` | ON | 编译 cjpegli/djpegli 工具 |
| `JPEGLI_ENABLE_DEVTOOLS` | OFF | 开发者工具（ssimulacra2 等） |
| `JPEGLI_ENABLE_BENCHMARK` | ON | 基准测试工具 |
| `JPEGLI_ENABLE_FUZZERS` | 见备注 | 模糊测试（x86_64 Linux 默认 ON） |
| `JPEGLI_ENABLE_JNI` | ON | Java JNI 封装 |
| `JPEGLI_ENABLE_MANPAGES` | ON | man 手册页 |
| `JPEGLI_ENABLE_DOXYGEN` | ON | API 文档 |
| `JPEGLI_ENABLE_OPENEXR` | ON | OpenEXR 格式支持 |
| `JPEGLI_ENABLE_SJPEG` | ON | sjpeg 编码器支持 |
| `JPEGLI_ENABLE_JPEGLI_LIBJPEG` | ON | 编译 libjpeg.so 兼容库 |
| `JPEGLI_BUNDLE_LIBPNG` | OFF | 内置 libpng（Windows 默认 OFF） |
| `JPEGLI_ENABLE_SKCMS` | ON | 使用 skcms（OFF 则使用 lcms2） |
| `JPEGLI_ENABLE_TCMALLOC` | OFF | tcmalloc 内存分配器（Windows 不支持） |

### 4.3 SIMD 指令集选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `JPEGLI_ENABLE_HWY_AVX2` | ON | AVX2 指令集（Haswell 及以上 CPU） |
| `JPEGLI_ENABLE_HWY_AVX3` | OFF | AVX-512（需 CPU 支持） |
| `JPEGLI_ENABLE_HWY_SSE4` | ON | SSE4 指令集 |
| `JPEGLI_ENABLE_HWY_SSE2` | ON | SSE2 指令集（x64 默认支持） |

完整的 HWY 目标列表：`AVX2, AVX3, AVX3_DL, AVX3_SPR, AVX3_ZEN4, EMU128, NEON, SCALAR, SSE2, SSE4, SSSE3, WASM` 等。

> 启用更多指令集意味着二进制文件更大，但能在支持的 CPU 上获得更好性能。关闭不需要的指令集可以减小体积。

### 4.4 三种构建配置详解

| 对比维度 | 配置A：最小化 | 配置B：含质量评估 | 配置C：全功能 |
|---------|-------------|-----------------|-------------|
| **适用场景** | 生产环境批量压缩 | 开发调参、质量对比 | 全面测试与评估 |
| **产物大小** | ~1 MB (单文件) | ~3.5 MB (含 lib) | ~10 MB+ (含完整依赖) |
| **运行依赖** | 零依赖 | `jpegli_cms.dll` + `jpegli_threads.dll` | 同配置B |
| **编译目标数** | ~111 | ~150 | ~200+ |
| **典型耗时** | ~3 min | ~5 min | ~8 min |

---

#### 配置A：最小化构建 — 仅 JPEG 压缩（生产推荐）

适用场景：
- 只需要 `cjpegli` 命令行工具做批量压缩
- 需要单文件部署，零运行时依赖
- CI/CD 流水线集成

```powershell
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release `
    -DBUILD_TESTING=OFF `
    -DJPEGLI_ENABLE_TOOLS=ON `
    -DJPEGLI_ENABLE_DEVTOOLS=OFF `
    -DJPEGLI_ENABLE_BENCHMARK=OFF `
    -DJPEGLI_ENABLE_FUZZERS=OFF `
    -DJPEGLI_ENABLE_JNI=OFF `
    -DJPEGLI_ENABLE_MANPAGES=OFF `
    -DJPEGLI_ENABLE_DOXYGEN=OFF `
    -DJPEGLI_ENABLE_OPENEXR=OFF `
    -DJPEGLI_ENABLE_SJPEG=OFF `
    -DJPEGLI_ENABLE_JPEGLI_LIBJPEG=OFF `
    -DJPEGLI_BUNDLE_LIBPNG=OFF `
    -DJPEGLI_STATIC=ON
```

**产物**：单文件 `cjpegli.exe`（~1 MB），零依赖，即拷即用。
**使用**：`cjpegli input.ppm output.jpg -q 90`

---

#### 配置B：标准构建 — 含质量评估工具

适用场景：
- 需要对比不同质量参数下的输出效果
- 编码器调参、质量验证
- 对比不同编码器的 SSIMULACRA2 评分

```powershell
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release `
    -DBUILD_TESTING=OFF `
    -DJPEGLI_ENABLE_TOOLS=ON `
    -DJPEGLI_ENABLE_DEVTOOLS=ON `
    -DJPEGLI_ENABLE_BENCHMARK=OFF `
    -DJPEGLI_ENABLE_FUZZERS=OFF `
    -DJPEGLI_ENABLE_JNI=OFF `
    -DJPEGLI_ENABLE_OPENEXR=OFF `
    -DJPEGLI_ENABLE_SJPEG=OFF `
    -DJPEGLI_ENABLE_JPEGLI_LIBJPEG=OFF `
    -DJPEGLI_STATIC=ON
```

**产物**：
| 工具 | 大小 | 说明 |
|------|------|------|
| `tools/cjpegli.exe` | ~1 MB | JPEG 编码器 |
| `tools/djpegli.exe` | ~600 KB | JPEG 解码器 |
| `tools/ssimulacra2.exe` | ~500 KB | SSIMULACRA2 质量评分工具 |

**质量评估示例**：
```powershell
# 用不同质量参数压缩同一张图
cjpegli input.ppm q90.jpg -q 90
cjpegli input.ppm q70.jpg -q 70
cjpegli input.ppm q50.jpg -q 50

# 用 SSIMULACRA2 评分对比
ssimulacra2 input.ppm q90.jpg   # 输出: 85.xx
ssimulacra2 input.ppm q70.jpg   # 输出: 72.xx
ssimulacra2 input.ppm q50.jpg   # 输出: 55.xx
```

> SSIMULACRA2 评分范围 0-100，分数越高表示压缩损失越小。详见 [SSIMULACRA2 说明](#ssimulacra2-说明)。

---

#### 配置C：全功能构建 — 含 C API 链接库

适用场景：
- 需要在 C/C++ 项目中直接调用 jpegli API 压缩内存数据
- 需要使用 jpegli 原生 API（`jpegli_*` 函数族）
- 需要使用 benchmark_xl 做性能基准测试
- 需要使用 libjpeg API 兼容库（仅 Linux）

```powershell
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release `
    -DBUILD_TESTING=OFF `
    -DJPEGLI_ENABLE_TOOLS=ON `
    -DJPEGLI_ENABLE_DEVTOOLS=ON `
    -DJPEGLI_ENABLE_BENCHMARK=ON `
    -DJPEGLI_ENABLE_FUZZERS=OFF `
    -DJPEGLI_ENABLE_JNI=OFF `
    -DJPEGLI_ENABLE_OPENEXR=OFF `
    -DJPEGLI_ENABLE_SJPEG=OFF `
    -DJPEGLI_ENABLE_JPEGLI_LIBJPEG=ON `
    -DJPEGLI_BUNDLE_LIBPNG=OFF `
    -DJPEGLI_STATIC=ON
```

**产物**（除配置B的全部工具外，新增以下链接库）：
| 库文件 | 大小 | 说明 |
|-------|------|------|
| `lib/jpegli-static.lib` | ~3.5 MB | **静态链接库**（你的项目要链接的目标） |
| `lib/jpegli_cms.lib` | ~766 KB | 色彩管理导入库 |
| `lib/jpegli_threads.lib` | ~166 KB | 线程支持导入库 |
| `tools/benchmark_xl.exe` | ~1.5 MB | 综合性能基准测试工具 |

**生成的头文件**（位于 `build/lib/include/jpegli/`）：
| 头文件 | 来源 | 用途 |
|-------|------|------|
| `jpeglib.h` | CMake 生成 | 标准 libjpeg 数据结构定义 |
| `jconfig.h` | CMake 生成 | 平台配置宏 |
| `jmorecfg.h` | CMake 生成 | 额外类型定义 |
| `encode.h` | `lib/jpegli/encode.h` | jpegli 编码 API |
| `decode.h` | `lib/jpegli/decode.h` | jpegli 解码 API |
| `common.h` | `lib/jpegli/common.h` | jpegli 公共 API |

---

### 4.5 三种配置的包含关系

```
配置A（最小化）
└── cjpegli.exe + djpegli.exe
    └── 静态链接 jpegli-static + highway + skcms

配置B（含质量评估）
├── 配置A的所有内容
└── 额外：ssimulacra2.exe
    └── 额外依赖：lib/extras（图像编解码，已静态包含）

配置C（全功能）
├── 配置B的所有内容
├── 额外：benchmark_xl.exe
├── 额外：jpegli-static.lib（供外部项目链接）
└── 额外：导出的头文件（供外部项目 include）
```

---

## 5. 构建步骤

### 5.1 加载 Visual Studio 开发环境

CMake 需要 VS 环境变量来找到 C++ 编译器和标准库头文件。

**PowerShell 方式（推荐）：**

```powershell
$vsPath = "C:\Program Files\Microsoft Visual Studio\2022\Community"
$devShellModule = "$vsPath\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
Import-Module $devShellModule
Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -DevCmdArguments "-arch=x64"
```

**CMD 方式：**

```cmd
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
```

### 5.2 配置（Configure）

```powershell
cd jpegli
mkdir build
cd build

# 选择你需要的配置（以下是配置A：最小化构建）
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release `
    -DBUILD_TESTING=OFF `
    -DJPEGLI_ENABLE_TOOLS=ON `
    -DJPEGLI_ENABLE_DEVTOOLS=OFF `
    -DJPEGLI_ENABLE_BENCHMARK=OFF `
    -DJPEGLI_ENABLE_FUZZERS=OFF `
    -DJPEGLI_ENABLE_JNI=OFF `
    -DJPEGLI_ENABLE_MANPAGES=OFF `
    -DJPEGLI_ENABLE_DOXYGEN=OFF `
    -DJPEGLI_ENABLE_OPENEXR=OFF `
    -DJPEGLI_ENABLE_SJPEG=OFF `
    -DJPEGLI_ENABLE_JPEGLI_LIBJPEG=OFF `
    -DJPEGLI_BUNDLE_LIBPNG=OFF `
    -DJPEGLI_STATIC=ON
```

成功输出示例：
```
-- Building tools: cjpegli;djpegli
-- Configuring done
-- Generating done
-- Build files have been written to: C:/Code/jpegli/build
```

### 5.3 编译（Build）

```powershell
# 编译全部工具（根据配置，约 111-200 个目标）
ninja

# 或只编译特定工具
ninja cjpegli           # 仅编码器
ninja cjpegli djpegli   # 编码器 + 解码器
```

编译完成后，产物位于 `build/tools/` 目录。

### 5.4 验证

```powershell
# 查看帮助
.\tools\cjpegli.exe -h

# 压缩测试
.\tools\cjpegli.exe input.ppm output.jpg -q 90

# 解压测试
.\tools\djpegli.exe output.jpg decoded.ppm
```

---

## 6. 构建产物

### 6.1 配置A：最小化构建（静态链接）

| 文件 | 大小 | 说明 |
|------|------|------|
| `tools/cjpegli.exe` | ~1 MB | JPEG 编码器，**零依赖单文件** |
| `tools/djpegli.exe` | ~600 KB | JPEG 解码器，**零依赖单文件** |

无需任何 DLL，直接运行。**适合生产环境部署。**

### 6.2 配置B：标准构建（含质量评估）

| 文件 | 大小 | 说明 |
|------|------|------|
| `tools/cjpegli.exe` | ~1 MB | JPEG 编码器 |
| `tools/djpegli.exe` | ~600 KB | JPEG 解码器 |
| `tools/ssimulacra2.exe` | ~500 KB | SSIMULACRA2 质量评分工具 |

> 如果未启用 `JPEGLI_STATIC=ON`，则运行时需将 `build/lib` 目录加入 PATH（依赖 `jpegli_cms.dll` + `jpegli_threads.dll`）。

### 6.3 配置C：全功能构建（含链接库）

| 命令行工具 | 大小 | 说明 |
|-----------|------|------|
| `tools/cjpegli.exe` | ~1 MB | JPEG 编码器 |
| `tools/djpegli.exe` | ~600 KB | JPEG 解码器 |
| `tools/ssimulacra2.exe` | ~500 KB | 质量评分 |
| `tools/benchmark_xl.exe` | ~1.5 MB | 综合性能基准测试 |

| 链接库 | 大小 | 说明 |
|-------|------|------|
| `lib/jpegli-static.lib` | ~3.5 MB | **主链接库**，供外部项目使用 |
| `lib/jpegli_cms.lib` | ~766 KB | 色彩管理导入库 |
| `lib/jpegli_threads.lib` | ~166 KB | 线程支持导入库 |

| 头文件 | 路径 | 说明 |
|-------|------|------|
| `build/lib/include/jpegli/jpeglib.h` | 生成文件 | libjpeg 兼容数据结构 |
| `lib/jpegli/encode.h` | 源码文件 | jpegli 编码 API 声明 |
| `lib/jpegli/decode.h` | 源码文件 | jpegli 解码 API 声明 |
| `lib/jpegli/common.h` | 源码文件 | jpegli 公共 API 声明 |

---

## 7. 使用示例

### 7.1 基本压缩

```powershell
# 质量 90（默认）
cjpegli input.ppm output.jpg -q 90

# 视觉无损模式（butteraugli distance = 1.0）
cjpegli input.ppm output.jpg -d 1.0

# 指定色度采样
cjpegli input.ppm output.jpg -q 90 --chroma_subsampling=420
```

### 7.2 质量参数说明

| 参数 | 范围 | 说明 |
|------|------|------|
| `-q` / `--quality` | 1-100 | 质量值（推荐 68-96），类似 libjpeg 语义 |
| `-d` / `--distance` | 0.0-25.0 | Butteraugli 距离，越小质量越高（1.0 = 视觉无损） |
| `-p` / `--progressive_level` | 0-2 | 渐进式级别（2 = 最高） |
| `--chroma_subsampling` | 444/440/422/420 | 色度采样 |

### 7.3 配置C：C 语言 API 集成（压缩内存数据）

如果你需要在 C/C++ 项目中直接调用 jpegli 压缩内存中的像素数据（无需写文件），使用配置C编译出的静态库。

#### 头文件包含

```cpp
// 需要添加的 include 路径：
//   -Ibuild/lib/include/jpegli       → jpeglib.h, jconfig.h
//   -Ilib/jpegli                     → encode.h, decode.h, common.h
//   -Ilib/base                       → include_jpeglib.h

// 链接库：
//   build/lib/jpegli-static.lib
//   build/lib/jpegli_cms.lib
//   build/lib/jpegli_threads.lib
```

#### 完整示例：内存 RGB → JPEG 内存缓冲

```cpp
#include "lib/jpegli/encode.h"   // jpegli_* 编码 API
#include "lib/jpegli/common.h"   // jpegli_std_error 等

#include <setjmp.h>
#include <cstdio>

struct my_error_mgr {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

void my_error_exit(j_common_ptr cinfo) {
    my_error_mgr* myerr = (my_error_mgr*)cinfo->err;
    (*cinfo->err->output_message)(cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}

// 压缩内存 RGB 数据到 JPEG 内存缓冲
// 返回 true 表示成功，outbuf/outsize 由 jpegli 分配，调用者负责释放
bool compress_rgb_to_jpeg(
    unsigned char* rgb,       // 输入 RGB 像素数据
    int width, int height,    // 图像宽度和高度
    int quality,              // JPEG 质量 1-100
    unsigned char** outbuf,   // 输出 JPEG 数据指针
    unsigned long* outsize    // 输出 JPEG 数据大小
) {
    struct jpeg_compress_struct cinfo;
    struct my_error_mgr jerr;
    
    cinfo.err = jpegli_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    
    if (setjmp(jerr.setjmp_buffer)) {
        jpegli_destroy_compress(&cinfo);
        return false;
    }
    
    jpegli_create_compress(&cinfo);
    jpegli_mem_dest(&cinfo, outbuf, outsize);  // 输出到内存缓冲

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpegli_set_defaults(&cinfo);
    jpegli_set_quality(&cinfo, quality, TRUE);
    jpegli_start_compress(&cinfo, TRUE);

    JSAMPROW row_pointer[1];
    while (cinfo.next_scanline < height) {
        row_pointer[0] = &rgb[cinfo.next_scanline * width * 3];
        jpegli_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpegli_finish_compress(&cinfo);
    jpegli_destroy_compress(&cinfo);
    return true;
}

// 使用示例
void example() {
    unsigned char* rgb_data = /* 你的 RGB 像素数据 */;
    unsigned char* jpeg_buf = nullptr;
    unsigned long jpeg_size = 0;
    
    if (compress_rgb_to_jpeg(rgb_data, 1920, 1080, 90, &jpeg_buf, &jpeg_size)) {
        printf("JPEG 压缩成功: %lu bytes\n", jpeg_size);
        // 使用 jpeg_buf ...
        free(jpeg_buf);  // jpegli_mem_dest 分配的内存由调用者释放
    }
}
```

> **API 命名差异**：jpegli 原生 API 使用 `jpegli_*` 前缀而非 `jpeg_*`，但参数和用法完全一致。详细对照见下方表格。

#### `jpeg_*` vs `jpegli_*` API 对照表

| 标准 libjpeg 函数 | jpegli 等效函数 | 说明 |
|-------------------|----------------|------|
| `jpeg_create_compress` | `jpegli_create_compress` | 初始化压缩对象 |
| `jpeg_set_defaults` | `jpegli_set_defaults` | 设置默认参数 |
| `jpeg_set_quality` | `jpegli_set_quality` | 设置质量 |
| `jpeg_start_compress` | `jpegli_start_compress` | 开始压缩 |
| `jpeg_write_scanlines` | `jpegli_write_scanlines` | 写入扫描行 |
| `jpeg_finish_compress` | `jpegli_finish_compress` | 完成压缩 |
| `jpeg_mem_dest` | `jpegli_mem_dest` | 输出到内存 |
| `jpeg_std_error` | `jpegli_std_error` | 错误处理 |
| `jpeg_destroy_compress` | `jpegli_destroy_compress` | 销毁压缩对象 |

> **数据结构无需改动**：`jpeg_compress_struct`、`jpeg_error_mgr`、`JSAMPROW` 等类型定义与 libjpeg 完全一致，直接使用。

#### jpegli 独有的增强 API

```cpp
// 使用 butteraugli distance 替代 quality（更精确的视觉质量控制）
void jpegli_set_distance(j_compress_ptr cinfo, float distance, boolean force_baseline);

// 设置 PSNR 目标值，自动搜索最优 distance
void jpegli_set_psnr(j_compress_ptr cinfo, float psnr, float tolerance,
                     float min_distance, float max_distance);

// 启用/禁用自适应量化（默认启用）
void jpegli_enable_adaptive_quantization(j_compress_ptr cinfo, boolean value);

// 设置渐进式级别：0=顺序编码，1-2=渐进编码
void jpegli_set_progressive_level(j_compress_ptr cinfo, int level);
```

### 7.4 配置B/C：SSIMULACRA2 图像质量评估

SSIMULACRA2 是一个基于人眼视觉感知的图像质量评估指标，分数范围 **0~100**（越高越好）。

#### 评分含义

| 分数 | 主观感受 | 等效压缩参数 |
|------|---------|-------------|
| 100 | 数学无损 | - |
| 90+ | 视觉无损（翻页对比看不出差异） | `-q 95` 或 `-d 0.5` |
| 85+ | 极高质量（并排对比看不出差异） | `-q 90` 或 `-d 1.0` |
| 80+ | 高质量（难以察觉瑕疵） | `-q 85` 或 `-d 1.5` |
| 70+ | 高质量（需仔细对比） | `-q 73` 或 `-d 2.5` |
| 50 | 中等质量 | `-q 45` 或 `-d 5` |
| 30 | 低质量 | `-q 20` 或 `-d 9` |
| 10 | 极低质量 | `-q 12` 或 `-d 14` |

#### 使用方式

```powershell
# 对比原图和压缩后的质量损失
ssimulacra2.exe original.png compressed.jpg
# 输出: 85.4321

# 批量评估
for ($q = 70; $q -le 95; $q += 5) {
    cjpegli input.ppm "q$q.jpg" -q $q
    $score = ssimulacra2 input.ppm "q$q.jpg"
    Write-Output "Quality $q => Score $score"
}
```

#### 工作原理简述

- 在 **6 个尺度**（1:1 到 1:32）上分析图像
- 比较 **3 个色彩分量**（XYB 色彩空间）
- 计算 **3 种误差图**：SSIM'（结构相似度）、ringing（振铃伪影）、blurring（模糊）
- 通过 **108 个加权子分数** 综合得出最终评分
- 权重基于多个主观数据集（CID22、TID2013、Kadid10k）训练得出

### 7.5 Docker 中使用

```dockerfile
FROM mcr.microsoft.com/windows/servercore:ltsc2022
COPY cjpegli.exe /tools/
ENTRYPOINT ["/tools/cjpegli.exe"]
```

---

## 8. 构建脚本

项目提供了 PowerShell 构建脚本，封装了完整的 CMake + Ninja 流程。

### 8.1 快速开始

```powershell
# 配置A：最小化构建 — 单文件 cjpegli（生产环境推荐）
.\tools\scripts\build_jpegli.ps1 -Target Minimal -Static

# 配置B：标准构建 — 含 ssimulacra2 质量评估
.\tools\scripts\build_jpegli.ps1 -Target Standard -Static

# 配置C：全功能构建 — 含链接库 + benchmark
.\tools\scripts\build_jpegli.ps1 -Target Full -Static
```

### 8.2 脚本参数

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `-Target` | Minimal/Standard/Full/Debug | Standard | 构建档次（见下方对照表） |
| `-BuildType` | Release/Debug/RelWithDebInfo | Release | 构建类型 |
| `-Static` | switch | 否 | **静态链接**，推荐启用 |
| `-Clean` | switch | 否 | 清理 CMake 缓存后重建 |
| `-NinjaJobs` | int | CPU 核心数 | 并行编译任务数 |
| `-NoVsEnv` | switch | 否 | 跳过 VS 环境检测（已在 VS 终端中时使用） |

### 8.3 三档 Target 对比（A/B/C）

| 映射 | Target | 对应配置 | DEVTOOLS | BENCHMARK | 产物 | 编译目标数 | 典型耗时 |
|-----|--------|---------|----------|-----------|------|-----------|---------|
| **A** | Minimal | 最小化 | OFF | OFF | cjpegli + djpegli | ~111 | ~3 min |
| **B** | Standard | 含质量评估 | ON | OFF | + ssimulacra2 | ~150 | ~5 min |
| **C** | Full | 全功能 | ON | ON | + benchmark_xl + 链接库 | ~200+ | ~8 min |

---

## 9. 常见问题

### 9.1 fatal error C1083: 无法打开包括文件: "cstdint"

**原因**: 未在 VS 开发者环境中运行 CMake，Ninja 找不到标准库头文件。

**解决**: 在运行 CMake 前加载 VS 环境变量：

```powershell
Import-Module "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
Enter-VsDevShell -VsInstallPath "C:\Program Files\Microsoft Visual Studio\2022\Community" -DevCmdArguments "-arch=x64"
```

### 9.2 运行时 STATUS_DLL_NOT_FOUND (0xC0000135)

**原因**: 动态链接时，`jpegli_cms.dll` 和 `jpegli_threads.dll` 不在 PATH 中。

**解决一（推荐）**：使用 `-DJPEGLI_STATIC=ON` 静态链接，生成单文件 exe。

**解决二**：设置 PATH 后运行：

```powershell
$env:Path = "build\lib;$env:Path"
.\build\tools\cjpegli.exe input.ppm output.jpg
```

### 9.3 编译速度慢

- 使用 Ninja 而非 Makefile（2-3x 更快）
- 关闭 `BUILD_TESTING`、`JPEGLI_ENABLE_FUZZERS` 等不需要的选项
- 只编译需要的目标：`ninja cjpegli` 而非 `ninja`
- 使用 `-Clean` 参数仅清除 CMake 缓存，保留已编译的对象文件

### 9.4 如何获得最佳编码性能

推荐的最小化构建配置已经优化了性能：

- **Release 模式**：O2 优化已启用
- **Highway SIMD**：AVX2/SSE4 等指令集自动启用
- **静态链接**：消除了 DLL 加载和重定位开销
- **skcms 色彩管理**：比 lcms2 更轻量

如果需要进一步提升性能，可以考虑：
- 在 Linux 上使用 Clang 编译器（通常比 GCC 生成更快的代码）
- 启用 `JPEGLI_ENABLE_LTO`（仅 Linux GCC/Clang）
- 在支持 AVX-512 的 CPU 上启用 `JPEGLI_ENABLE_HWY_AVX3`

---

## 附录：完整 CMake 选项参考

```powershell
# 查看所有 JPEGLI 相关选项
cd build
cmake .. -LA 2>&1 | findstr JPEGLI
```

### 第三方依赖

```
third_party/
├── highway/        # Google Highway SIMD（必需）
├── skcms/          # Google 色彩管理（必需）
├── zlib/           # 压缩库
├── googletest/     # 测试框架
├── libjpeg-turbo/  # libjpeg 兼容层
├── libpng/         # PNG 支持
├── lcms/           # 备选色彩管理
├── sjpeg/          # 简易 JPEG 编码
└── apngdis/        # APNG 解析
```
