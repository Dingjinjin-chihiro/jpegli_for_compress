# jpegli_for_compress

基于 Google [jpegli](https://github.com/google/jpegli) 的 JPEG 压缩工具链。

## 构建文档

详见 [docs/BUILD_GUIDE.md](docs/BUILD_GUIDE.md)

## 三种构建方案

| 方案 | 说明 | 命令 |
|------|------|------|
| **A** | 最小化 — 单文件 cjpegli，零依赖 | `ninja cjpegli` |
| **B** | 含 ssimulacra2 质量评估 | `ninja cjpegli djpegli ssimulacra2` |
| **C** | 全功能 — 含 C API 链接库 | `ninja` |

## 快速使用

```powershell
# 压缩
.\build\tools\cjpegli.exe input.ppm output.jpg -q 90

# 支持 quality 和 distance 两种控制方式
.\build\tools\cjpegli.exe input.ppm output.jpg -d 1.0
```
