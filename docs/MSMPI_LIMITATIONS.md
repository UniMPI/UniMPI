# MS-MPI 功能限制调查报告

## 调查概述

本报告调查了 Microsoft MPI (MS-MPI) 相对于其他 MPI 实现（OpenMPI、MPICH、Intel MPI）的功能限制，特别是动态进程管理相关的功能。

## 受限制的功能

### 1. 动态进程管理 (`MPI_Comm_spawn`)

**限制状态**: ⚠️ 有限支持（测试中跳过）

**原因**:
- MS-MPI 基于 MPICH 的早期版本，动态进程实现不如 OpenMPI 完善
- Windows 进程模型与 Unix 不同，缺乏 `fork()` 系统调用
- Windows 进程创建开销更大，进程间通信需要更复杂的句柄继承机制
- 需要 `smpd` (Scalable MPI Daemon) 服务支持，在标准安装中不一定启用

**技术细节**:
- Windows 没有 Unix 的 `fork()`，`CreateProcess` 是更重量级的操作
- 动态进程需要：
  1. 与 mpiexec 通信请求新进程
  2. 在新节点上启动进程
  3. 建立进程间通信通道
  4. 处理 Windows 特定的句柄和安全性

### 2. 名称服务 (`MPI_Publish_name` / `MPI_Lookup_name`)

**限制状态**: ❌ 不支持（测试中跳过）

**原因**:
- MS-MPI 没有实现 MPI 名称服务守护进程（`mpirun --nameserv` 的等价物）
- Windows 缺乏内置的分布式命名服务
- 在 Linux/OpenMPI 中，这通常依赖于 `orted` 或独立的名字服务器

**技术细节**:
- OpenMPI 使用 `orte-server` 或 `mpirun --nameserv` 提供名称服务
- MS-MPI 没有等价的机制来维护全局服务名到端口名的映射
- 这些函数在头文件中有声明，但可能返回 `MPI_ERR_UNSUPPORTED_OPERATION`

### 3. 连接/接受 (`MPI_Comm_connect` / `MPI_Comm_accept`)

**限制状态**: ⚠️ 有限支持（测试中跳过）

**原因**:
- 需要工作端口和名称服务支持
- 在 Windows 域环境中，安全性和防火墙配置更复杂
- 与 `MPI_Open_port` 相关的网络端口分配可能受 Windows 防火墙限制

## 架构差异

### Windows vs Unix 进程模型

| 特性 | Unix/Linux | Windows |
|------|-----------|---------|
| 进程创建 | `fork()` + `exec()` | `CreateProcess()` |
| 进程开销 | 轻量级（copy-on-write） | 重量级（完整复制） |
| 句柄继承 | 文件描述符 | Windows 句柄（更复杂） |
| 进程间通信 | POSIX 共享内存/信号 | 命名管道/内存映射文件 |
| 守护进程 | Systemd/cron | Windows Service |

### MS-MPI vs 其他实现

| 功能 | OpenMPI | MPICH | Intel MPI | MS-MPI |
|------|---------|-------|-----------|--------|
| `MPI_Comm_spawn` | ✅ 完整支持 | ✅ 完整支持 | ✅ 完整支持 | ⚠️ 有限支持 |
| `MPI_Publish_name` | ✅ 完整支持 | ✅ 完整支持 | ✅ 完整支持 | ❌ 不支持 |
| `MPI_Lookup_name` | ✅ 完整支持 | ✅ 完整支持 | ✅ 完整支持 | ❌ 不支持 |
| `MPI_Comm_connect` | ✅ 完整支持 | ✅ 完整支持 | ✅ 完整支持 | ⚠️ 有限支持 |
| `MPI_Comm_accept` | ✅ 完整支持 | ✅ 完整支持 | ✅ 完整支持 | ⚠️ 有限支持 |
| `MPI_Open_port` | ✅ 完整支持 | ✅ 完整支持 | ✅ 完整支持 | ✅ 支持 |
| `MPI_Close_port` | ✅ 完整支持 | ✅ 完整支持 | ✅ 完整支持 | ✅ 支持 |

## 实现细节

### 测试代码中的处理

在 `tests/mpi/test_dynamic.c` 中，测试明确跳过了 MS-MPI 上的相关功能：

```c
/* Check if running on MS-MPI (limited spawn support) */
static int is_mswin(void) {
#ifdef _WIN32
    return 1;
#else
    return 0;
#endif
}

int test_spawn_single(void) {
    if (is_mswin()) {
        printf("  SKIP: spawn tests disabled on MS-MPI\n");
        return 0;
    }
    // ... 实际测试代码
}
```

### 函数声明 vs 实现

MS-MPI SDK 头文件 (`mpi.h`) 中**声明**了这些函数：
- `MPI_Comm_spawn`
- `MPI_Comm_spawn_multiple`
- `MPI_Publish_name`
- `MPI_Lookup_name`
- `MPI_Comm_connect`
- `MPI_Comm_accept`

但**运行时**可能返回错误码或无法正常工作，因为底层缺少完整的支持基础设施（如名称服务守护进程）。

## MPI-3.0 缺符号降级

除了动态进程等显式 stub(见上),MS-MPI 还有一类更普遍的降级问题:**部分 MPI-3.0 新增符号在 `msmpi.dll` 中不存在**,导致 uniMPI 跳转表对应字段为 `NULL`。

### uniMPI 的降级路径

1. **落地为 NULL**:`src/backends/msmpi.c` 对每个符号直接 `dlsym`;`msmpi.dll` 缺少时字段保持 `NULL`,无全局失败、无按符号 stub。
2. **宏层不拦截 NULL**:标准宏 `MPI_Mprobe` / `MPI_Iallreduce` / `MPI_Fetch_and_op` 等直接展开为 `unimpi.<field>`;字段为 NULL 时调用即空指针崩溃(零开销设计,宏层刻意不加运行时分支)。
3. **调用方/测试负责降级**:先检测 `unimpi.<fn> != NULL`(或测试的 `*_available()` 辅助),再调用;缺失则优雅跳过。

```c
if (unimpi.fetch_and_op != NULL) {
    MPI_Fetch_and_op(...);   /* 安全,符号存在 */
}
```

MPI-3.0 测试套件已统一采用该模式并降级为文档化跳过:

| 测试 | 辅助函数 | 检查的字段 |
|---|---|---|
| `test_mpi30_collective_corner.c` | `nbc_available` | `ibcast`/`igather`/`iscatter`/`iallreduce` |
| `test_mpi30_matched_probe.c` | `matched_probe_available` | `mprobe` |
| `test_mpi30_rma_atomic.c` | `atomics_available` / `request_rma_available` | `fetch_and_op`/`compare_and_swap`/`get_accumulate`/`rput`/`rget`/`raccumulate` |

### 审计状态

对照 `tools/mpi_version_gate.py` 的 `M30_CANONICAL`(MPI-3.0 新增符号全集)静态核查:每一簇在 `src/backends/msmpi.c` 均有 `dlsym` 落地,故 MS-MPI 缺失者一律落到 NULL 并被上述模式覆盖。**确切**的逐符号缺失清单无法在 Linux 主机上实测(无法加载 `msmpi.dll`),属静态核查 + 降级契约;需在 Windows 上运行 MS-MPI 测试套件(`test_msmpi_compatibility.bat`)做最终确认。已知的窄子集:非阻塞集合在 MS-MPI 上仅有文档记录的 9 个,故其 corner 测试必须以 `nbc_available()` 门控。

## 建议

### 对于 unimpi 用户

1. **避免使用动态进程**: 在 Windows/MS-MPI 上，避免使用 `MPI_Comm_spawn` 和相关的动态进程功能
2. **使用静态进程**: 使用 `mpiexec -np N` 启动所有需要的进程
3. **替代方案**: 如果需要动态工作负载分配，考虑使用 MPI 的 `MPI_Send`/`MPI_Recv` 在静态进程中实现任务分发

### 代码可移植性

```c
#ifdef _WIN32
    /* MS-MPI: 使用静态进程模式 */
    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    /* 所有进程在启动时已确定 */
#else
    /* OpenMPI/MPICH: 可以使用动态进程 */
    MPI_Comm_spawn(...);
#endif
```

## 结论

MS-MPI 的这些限制主要源于：
1. **Windows 架构差异** - 缺乏 Unix 的轻量级进程创建机制
2. **缺少基础设施** - 没有等价的名称服务守护进程
3. **安全模型** - Windows 的安全和防火墙配置更严格
4. **设计优先级** - MS-MPI 主要针对 HPC 工作负载的静态进程模型优化

对于大多数 HPC 应用，这些限制不影响使用，因为通常使用静态进程启动模式（`mpiexec -np N`）。动态进程主要用于更复杂的分布式计算场景（如网格计算、客户端-服务器模式）。
