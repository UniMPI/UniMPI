# TFTK-MPI Wrapper 设计文档

**日期**: 2025-04-03  
**主题**: MPI 运行时包装层  
**目标**: 零开销、完整 MPI-3 API、多后端运行时加载

---

## 1. 架构概述

```
┌─────────────────────────────────────────────────────────────┐
│                    用户代码层                                │
│  ┌─────────────┐    ┌─────────────────────────────────────┐ │
│  │ 函数指针风格 │    │ 标准 MPI 风格（可选宏封装）          │ │
│  │ tftk_mpi.xxx│    │ MPI_Send, MPI_Init...               │ │
│  └──────┬──────┘    └──────────────────┬──────────────────┘ │
└─────────┼──────────────────────────────┼────────────────────┘
          │                              │
          └──────────────┬───────────────┘
                         │
┌────────────────────────▼────────────────────────────────────┐
│              TFTK-MPI 运行时核心层                          │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │ 后端检测    │  │ 动态加载器  │  │ 函数指针表 (vtable)  │  │
│  │  auto-detect│  │  dlopen/dlsym│  │  400+ MPI-3 函数    │  │
│  └─────────────┘  └─────────────┘  └─────────────────────┘  │
└────────────────────────┬────────────────────────────────────┘
                         │
         ┌───────────────┼───────────────┐
         │               │               │
    ┌────▼────┐    ┌────▼────┐    ┌────▼────┐
    │OpenMPI  │    │Intel-MPI│    │  MPICH  │
    │libmpi.so│    │libmpi.so│    │libmpi.so│
    └─────────┘    └─────────┘    └─────────┘
```

---

## 2. 核心组件

### 2.1 后端检测与加载

**支持的 MPI 实现：**

| 优先级 | 后端 | 库名 (Linux) | 库名 (Windows) |
|--------|------|--------------|----------------|
| 1 | 用户指定 | `TFTK_MPI_BACKEND` 环境变量 | 同上 |
| 2 | OpenMPI | `libmpi.so.40`, `libmpi.so.20` | 不支持 |
| 3 | Intel-MPI | `libmpi.so` | 不支持 |
| 4 | MPICH | `libmpich.so` | `msmpi.dll` |
| 5 | MS-MPI | N/A | `msmpi.dll` |

**加载流程：**

1. 读取 `TFTK_MPI_BACKEND` 环境变量
2. 尝试加载指定后端
3. 失败后进入自动检测（按优先级）
4. 解析核心符号（MPI_Init, MPI_Finalize）
5. 验证 ABI 兼容性
6. 填充完整 vtable

### 2.2 函数指针表 (Vtable)

包含 400+ MPI-3 标准函数，按功能分组：

- **环境管理**: Init, Finalize, Abort, Initialized...
- **点对点通信**: Send, Recv, Isend, Irecv, Sendrecv...
- **集合通信**: Bcast, Reduce, Scatter, Gather, Allreduce...
- **非阻塞集合**: Ibarrier, Ireduce, Ibcast...
- **数据类型**: Type_create, Type_commit, Type_free...
- **通信域**: Comm_create, Comm_split, Comm_dup...
- **进程组**: Group_incl, Group_range_incl...
- **RMA/One-Sided**: Put, Get, Accumulate, Win_create...
- **动态进程**: Comm_spawn, Open_port, Publish_name...
- **并行 I/O**: File_open, File_read, File_write...

---

## 3. API 设计

### 3.1 公共 API

```c
// 初始化与关闭
int tftk_mpi_init(int *argc, char ***argv);        // 自动检测后端
int tftk_mpi_init_with(const char *backend_name);  // 指定后端
int tftk_mpi_finalize(void);

// 查询
const char *tftk_mpi_get_backend_name(void);
int tftk_mpi_is_initialized(void);

// 错误处理
const char *tftk_mpi_error_string(int error_code);
```

### 3.2 函数指针表访问

```c
// 直接访问 vtable（零开销）
extern tftk_mpi_vtable_t tftk_mpi;

// 使用示例
tftk_mpi.init(&argc, &argv);
tftk_mpi.send(buf, count, MPI_INT, dest, tag, MPI_COMM_WORLD);
```

### 3.3 可选宏封装

定义 `TFTK_MPI_USE_STD_NAMES` 后，标准 MPI 命名可用：

```c
#define MPI_Init tftk_mpi.init
#define MPI_Send tftk_mpi.send
// ... 所有 MPI 函数
```

---

## 4. 错误处理

| 错误码 | 含义 | 处理方式 |
|--------|------|----------|
| `TFTK_MPI_OK` | 成功 | 正常继续 |
| `TFTK_MPI_ERR_NO_BACKEND` | 未找到 MPI 后端 | 返回错误，提示安装 MPI |
| `TFTK_MPI_ERR_BACKEND_LOAD` | 加载库失败 | 返回错误，检查库路径 |
| `TFTK_MPI_ERR_ABI_MISMATCH` | ABI 不兼容 | 返回错误，提示版本不匹配 |

运行时错误直接透传底层 MPI 错误码。

---

## 5. 后端特性

### 5.1 OpenMPI
- Linux 库名: `libmpi.so.40` (v4.x), `libmpi.so.20` (v3.x)
- Windows: 不支持
- ABI: 版本间不兼容，需运行时检测
- 特殊: 需处理 `ompi_mpi_comm_world` 等全局符号

### 5.2 Intel-MPI
- Linux 库名: `libmpi.so`
- 兼容 OpenMPI ABI
- 需处理 I_MPI 环境变量

### 5.3 MPICH
- Linux 库名: `libmpich.so`
- Windows: `msmpi.dll` (MPICH 派生)
- ABI: 使用全局变量 `MPI_COMM_WORLD`

### 5.4 MS-MPI
- Windows 专用: `msmpi.dll`

---

## 6. 构建系统

CMake 配置选项：

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `TFTK_MPI_BUILD_EXAMPLES` | ON | 构建示例程序 |
| `TFTK_MPI_BUILD_TESTS` | ON | 构建测试 |
| `TFTK_MPI_ENABLE_STD_MACROS` | OFF | 默认启用标准 MPI 宏 |

---

## 7. 测试策略

```
tests/
├── unit/
│   ├── test_loader.c       # 测试后端加载逻辑
│   └── test_vtable.c       # 测试 vtable 填充完整性
├── integration/
│   ├── test_init.c         # 测试初始化/关闭
│   ├── test_p2p.c          # 测试点对点通信
│   └── test_collective.c   # 测试集合通信
└── backend/
    ├── test_openmpi.sh     # OpenMPI 后端验证
    ├── test_mpich.sh       # MPICH 后端验证
    └── test_intelmpi.sh    # Intel-MPI 后端验证
```

---

## 8. 设计决策总结

| 决策 | 选择 | 理由 |
|------|------|------|
| 调用机制 | 函数指针表 | 单次间接跳转，接近零开销 |
| 初始化 | 单线程，显式 | 简单可靠，无锁开销 |
| API 风格 | 双模式 | 函数指针 + 可选宏 |
| 后端检测 | 自动 + 手动 | 灵活性优先 |
| 错误处理 | 透传 | 保持 MPI 语义透明 |
| MPI 标准 | MPI-3.x | 现代标准，完整功能 |

---

## 9. 项目结构

```
tftk-mpi-wrapper/
├── include/
│   └── tftk_mpi.h
├── src/
│   ├── core.c          # 初始化和核心逻辑
│   ├── loader.c        # 动态库加载
│   ├── vtable.c        # vtable 定义和填充
│   └── backends/
│       ├── openmpi.c
│       ├── mpich.c
│       └── msmpi.c
├── tests/
├── examples/
│   ├── minimal.c       # 函数指针风格示例
│   └── std_style.c     # 宏封装风格示例
└── CMakeLists.txt
```

---

## 10. 使用示例

### 函数指针风格

```c
#include "tftk_mpi.h"

int main(int argc, char **argv) {
    // 初始化（自动检测后端）
    tftk_mpi_init(&argc, &argv);
    
    int rank, size;
    tftk_mpi.comm_rank(MPI_COMM_WORLD, &rank);
    tftk_mpi.comm_size(MPI_COMM_WORLD, &size);
    
    printf("Hello from rank %d of %d\n", rank, size);
    
    tftk_mpi_finalize();
    return 0;
}
```

### 标准 MPI 风格

```c
#define TFTK_MPI_USE_STD_NAMES
#include "tftk_mpi.h"

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    printf("Hello from rank %d of %d\n", rank, size);
    
    MPI_Finalize();
    return 0;
}
```
