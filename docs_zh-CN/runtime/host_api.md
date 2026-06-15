> English: [docs/runtime/host_api.md](../../docs/runtime/host_api.md)

# B 线 · Runtime / AscendCL host API 文档

> **定位**：这些是 **host 侧** API（在 CPU 上执行），本身**不产生 AI Core 指令、不产 CAModel 仿真报告**；它们的角色是"**发射核函数的脚手架**"——分配内存、搬数据、起 stream、launch 核函数、同步、回收。A 线每个 example 的 `main.cpp` 都在用它们。本文档系统记录本项目实际用到的 host API（盘点自 172 个单元的 `main.cpp`）。

## 标准调用序列（每个 main.cpp 的骨架）

```c
aclInit(nullptr);                                   // 1. 初始化 ACL
aclrtSetDevice(0);                                  // 2. 绑定设备
aclrtStream stream = nullptr;
aclrtCreateStream(&stream);                         // 3. 创建 stream

// 4. host/device 内存分配
float *xH; aclrtMallocHost((void**)&xH, bytes);     //    host 侧
uint8_t *xD; aclrtMalloc((void**)&xD, bytes, ACL_MEM_MALLOC_HUGE_FIRST);  // device 侧
aclrtMemcpy(xD, bytes, xH, bytes, ACL_MEMCPY_HOST_TO_DEVICE);  // 5. H2D 搬入

ACLRT_LAUNCH_KERNEL(k_custom)(blockDim, stream, xD, zD);       // 6. 发射核函数
aclrtSynchronizeStream(stream);                     // 7. 等核函数完成

aclrtMemcpy(zH, bytes, zD, bytes, ACL_MEMCPY_DEVICE_TO_HOST);  // 8. D2H 取回

aclrtFree(xD); aclrtFreeHost(xH);                   // 9. 回收
aclrtDestroyStream(stream); aclrtResetDevice(0); aclFinalize();
```

所有调用都用 `CHECK_ACL(x)` 宏包裹（判 `aclError != ACL_SUCCESS` 即报错返回）。

## API 分组

### 初始化 / 去初始化
| API | 签名 | 用途 |
|-----|------|------|
| `aclInit` | `aclError aclInit(const char *configPath)` | 初始化 ACL，`nullptr`=默认配置。进程级，最先调。|
| `aclFinalize` | `aclError aclFinalize()` | 去初始化，最后调。|

### 设备
| API | 签名 | 用途 |
|-----|------|------|
| `aclrtSetDevice` | `aclError aclrtSetDevice(int32_t deviceId)` | 绑定当前线程到设备（仿真用 0）。|
| `aclrtResetDevice` | `aclError aclrtResetDevice(int32_t deviceId)` | 释放设备资源。|

### Stream（任务队列）
| API | 签名 | 用途 |
|-----|------|------|
| `aclrtCreateStream` | `aclError aclrtCreateStream(aclrtStream *stream)` | 创建 stream，核函数发射到它上面。|
| `aclrtSynchronizeStream` | `aclError aclrtSynchronizeStream(aclrtStream stream)` | 阻塞等待 stream 上所有任务（含核函数）完成。取回结果前必调。|
| `aclrtDestroyStream` | `aclError aclrtDestroyStream(aclrtStream stream)` | 销毁 stream。|

### 内存
| API | 签名 | 用途 |
|-----|------|------|
| `aclrtMallocHost` | `aclError aclrtMallocHost(void **ptr, size_t size)` | 分配 **host** 内存（页锁定，可高效 DMA）。|
| `aclrtFreeHost` | `aclError aclrtFreeHost(void *ptr)` | 释放 host 内存。|
| `aclrtMalloc` | `aclError aclrtMalloc(void **ptr, size_t size, aclrtMemMallocPolicy policy)` | 分配 **device(GM)** 内存。policy 常用 `ACL_MEM_MALLOC_HUGE_FIRST`。|
| `aclrtFree` | `aclError aclrtFree(void *ptr)` | 释放 device 内存。|
| `aclrtMemcpy` | `aclError aclrtMemcpy(void *dst, size_t destMax, const void *src, size_t count, aclrtMemcpyKind kind)` | host↔device 拷贝。kind：`ACL_MEMCPY_HOST_TO_DEVICE` / `ACL_MEMCPY_DEVICE_TO_HOST`。|

### 核函数发射
| API | 签名 | 用途 |
|-----|------|------|
| `ACLRT_LAUNCH_KERNEL` | `ACLRT_LAUNCH_KERNEL(kernelName)(blockDim, stream, args...)` | AscendC 提供的发射宏（由 `ascendc_library` 自动生成桩头 `aclrtlaunch_<kernelName>.h`）。`blockDim`=核数（多核同步样例用 8，其余 1），`args`=GM 指针。**这是 host 与 device kernel 的唯一桥梁**。|

### 平台信息（host tiling 用）
| API | 签名 | 用途 |
|-----|------|------|
| `platform_ascendc::PlatformAscendCManager::GetInstance()` | 返回 `PlatformAscendC*` | host 侧算 tiling 时取平台信息（核数/UB 大小等）。本项目 TopK（`TopKTilingFunc`）、Conv3D（`Conv3dTiling`）用到。include `tiling/platform/platform_ascendc.h`。|

## 覆盖边界（best-effort）
- 上述 13 个 API + 1 个 platform 接口，**已覆盖本项目 172 个单元 main.cpp 用到的全部 host API**。
- 未用到的 Runtime API（aclrtEvent 事件、aclrtMemset、多 stream/context、aclrtMemcpyAsync 等）属同族扩展，按需补；它们同样是 host 脚手架，不产 CAModel 报告。
- Runtime API 的"验证"= 调用返回 `ACL_SUCCESS` 且配合核函数跑通（已隐含在 172 个 PASSED 单元里）。
