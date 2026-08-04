# Overview

包含基础 C++ 编程常识、数据结构和算法、多线程编程、网络编程、面向对象编程、POSIX 标准、系统编程、CMake 构建系统等内容。

## 目录说明

- algorithm: 用于存放基本算法库的使用，常用算法库的使用。
- bassis: 基本语法（尤其是类和模版相关的使用方法）
- multi-threading: 多线程相关的内容，包括线程的创建、管理和同步等。
- network: 网络编程相关的内容，包括套接字编程、HTTP 请求等。
- oop: 面向对象编程相关的内容，包括类、继承、多态等。
- posix: POSIX 标准相关的内容，包括文件操作、进程管理等。
- system: 系统编程相关的内容，包括系统调用、进程管理等。
- cmake: CMake 构建系统相关的内容，包括 CMakeLists.txt 的使用、常见命令等。
- 30day_make_cpp_server: 30 天手写 C++ 服务器（Reactor → 主从 Reactor → …）

---

## 推理专家学习路线

目标：从 C++ 网络基础出发，系统掌握 **LLM 推理引擎 + Online Serving**，最终达到能设计、优化、贡献推理系统的水平。

> 「专家」通常需要 **2~4 年**系统积累（业余则更长）。约 **9 个月** 可到「能投推理岗、面试能聊」；**2 年+** 接近「专家」。

### 总览

```text
阶段 0  基础巩固（进行中）     C++ 并发 + Reactor
阶段 1  Serving 与异步 I/O      asio + 带队列的 mock 推理服务
阶段 2  推理核心概念            prefill/decode、KV、量化、batch
阶段 3  llama.cpp 精读          C++ 推理引擎主战场
阶段 4  GPU 与算子              CUDA + PyTorch 推理路径
阶段 5  工业 serving            vLLM / Triton 调度与显存
阶段 6  专家纵深                性能、分布式推理、源码贡献
```

### 库 / 项目优先级（推理向）

```text
必学：  12day → asio → llama.cpp → CUDA → vLLM
选学：  brpc（RPC serving）、Triton（多框架 serving）
知道：  io_uring、Seastar、folly（按需查）
可略：  训练向 NCCL / Megatron（推理岗非主线）
```

---

### 阶段 0：底子打牢（2~3 周）

**目标**：能讲清「一个 TCP 请求在线程间怎么流转」。

| 内容 | 状态 |
|---|---|
| 单 Reactor → 主从 Reactor（12day） | ✅ |
| `QueueInLoop` + eventfd 跨线程唤醒 | ✅ |
| 补 muduo 几块：OutputBuffer、高水位、定时器 | 待做 |
| 读《Linux 多线程服务端编程》对应章节 | 可选 |

**产出**：12day 文档 + 能口述主/从 Reactor 分发 fd 的全流程。

---

### 阶段 1：Serving 层（3~4 周）

**目标**：从 echo server 到「像推理网关」的服务。

| 周 | 内容 |
|---|---|
| 1 | Asio tutorial：timer → async TCP echo |
| 2 | `strand`、多线程 `io_context`、连接生命周期 |
| 3 | **13day 项目**：主从 Reactor + 请求队列 + 固定 batch worker（sleep 模拟 GPU） |
| 4 | 压测：QPS、P99；理解排队延迟 vs 执行延迟 |

**必读概念**：背压、超时、慢连接、流式回包（chunked）。

**产出（简历项目 1）**：

> C++ 主从 Reactor 推理 mock 网关：N SubReactor 接连接，M worker batch 消费，profile 排队与 batch 延迟。

---

### 阶段 2：推理概念课（2~3 周，可与阶段 1 并行）

**目标**：不碰大源码，先建立 LLM 推理词汇表。

| 主题 | 要搞懂 |
|---|---|
| Prefill vs Decode | 为何 prefill 吃算力、decode 吃带宽/访存 |
| Autoregressive | 逐 token 生成，无法一次算完 |
| KV cache | 存什么、显存随序列长度怎么涨 |
| Batching | static vs continuous batching |
| 量化 | FP16 / INT8 / GGUF 区别与 trade-off |
| Sampling | temperature、top-p、greedy |

**实践**：

- 用 Ollama / llama.cpp CLI 跑一个小模型，观察 token 速度
- 画一张图：prompt → tokenize → prefill → decode loop → detokenize

**产出**：一篇笔记《LLM 推理数据流》（面试直接讲）。

---

### 阶段 3：llama.cpp 精读（6~8 周）⭐ C++ 主线

**目标**：能跟完「从 main 到生成一个 token」的调用链。

**阅读顺序**（由浅入深）：

```text
1. examples/main.cpp / simple.cpp     入口与参数
2. llama.cpp / llama.h                模型加载、context
3. decode 主循环                      llama_decode / sample / accept
4. ggml                                 张量、计算图、backend
5. 量化 gguf                            权重量化格式
6. server 模式（如有）                  HTTP + 并发（和 12day 对照）
```

**每周节奏**：2 天读代码 + 笔记；1 天改参数观察行为；周末总结一条调用链。

**产出（简历项目 2）**：

> 基于 llama.cpp 做最小改动：例如加日志统计 prefill/decode 耗时，或实现简单 batch queue。

---

### 阶段 4：GPU 与算子（8~12 周）

**目标**：从「只会 CPU 推理」到「能读 CUDA kernel、会用 PyTorch 推理栈」。

| 顺序 | 内容 |
|---|---|
| 1 | CUDA：stream、memory、简单 vector add kernel |
| 2 | PyTorch：`inference_mode`、export ONNX、TorchScript |
| 3 | TensorRT 或 ONNX Runtime：跑通一个模型 |
| 4 | 读 FlashAttention / PagedAttention 论文 + 博客 |
| 5 | 看 llama.cpp CUDA backend 或 vLLM 里一个 attention kernel |

**工具**：Nsight Systems、nvidia-smi、GPU util 监控。

**产出（简历项目 3）**：

> 同一小模型对比：llama.cpp CPU vs GPU vs TRT-LLM/ORT，写 latency/吞吐对比报告。

---

### 阶段 5：工业 Online Serving（6~8 周）

**目标**：补「云端高并发 LLM API」能力，面试常考。

| 顺序 | 内容 |
|---|---|
| 1 | 部署 vLLM 或 Triton + TensorRT-LLM，OpenAI 兼容 API |
| 2 | 读 vLLM：`LLMEngine` → `Scheduler` → `Worker`（Python 即可） |
| 3 | 理解 continuous batching、max_num_seqs、gpu_memory_utilization |
| 4 | 压测：并发 prompt、长上下文、流式输出下的 P99 |
| 5 | 可选：SGLang / TensorRT-LLM 扫一遍，知道差异 |

**与 12day/13day 的对照**：

- SubReactor ≈ 接连接
- vLLM Scheduler ≈ batch 合并 + KV 槽位分配
- GPU Worker ≈ batch worker（真算子）

**产出（简历项目 4）**：

> vLLM 部署 + 压测报告：并发 vs latency，分析瓶颈在 queue / prefill / decode。

---

### 阶段 6：专家纵深（持续 1 年+）

**目标**：能设计/优化推理系统，而不只是「会用框架」。

| 方向 | 内容 |
|---|---|
| 性能 | roofline、算子融合、kernel fusion、CUDA graph |
| 显存 | PagedAttention、prefix cache、KV offload |
| 多卡 | tensor parallel inference、pipeline parallel |
| 编译 | torch.compile、Triton DSL、XLA（了解即可） |
| 工程 | 模型热更新、A/B、灰度、SLO、限流 |
| 开源 | 给 llama.cpp / vLLM / ggml 提一个小 PR |

**推荐阅读**：PagedAttention 论文；vLLM / TensorRT-LLM 设计文档；FlashInfer、CUTLASS 示例。

**「专家」自检**：

- [ ] 能画完一条请求的端到端延迟分解
- [ ] 能解释为何 decode 是 memory-bound
- [ ] 能读 attention CUDA kernel 并说出优化点
- [ ] 能设计 batch 策略并估算吞吐上限
- [ ] 有 2+ 可展示的推理相关项目/贡献

---

### 时间线（业余每天 2h；全职可压缩 2~3 倍）

| 阶段 | 时长 | 累计 |
|---|---|---|
| 0 巩固 | 2~3 周 | ~1 月 |
| 1 Serving | 3~4 周 | ~2 月 |
| 2 概念 | 2~3 周 | ~2.5 月 |
| 3 llama.cpp | 6~8 周 | ~4.5 月 |
| 4 GPU | 8~12 周 | ~7 月 |
| 5 vLLM serving | 6~8 周 | ~9 月 |
| 6 专家纵深 | 12 月+ | ~2 年 |

---

### 接下来 4 周（可直接执行）

| 周 | 任务 |
|---|---|
| W1 | 补 12day OutputBuffer/高水位；开始 asio tutorial |
| W2 | asio async echo + strand；读 prefill/decode/KV 笔记 |
| W3 | **13day**：batch 队列 mock 推理 server |
| W4 | clone llama.cpp，跑通 CLI，跟 main → 第一次 decode |

---

### muduo / asio / vLLM / llama.cpp 对照

| muduo 概念 | asio 对应 | 推理框架 |
|---|---|---|
| `EventLoop` | `io_context` | vLLM Engine / llama context |
| `QueueInLoop` | `post` / `dispatch` | 跨线程调度任务 |
| `Channel` + epoll | `async_*` + handler | 请求就绪回调 |
| SubReactor 线程 | 多线程 `run()` + `strand` | 多 GPU / 多 worker |

**一句话路线**：

```text
C++ serving（12day/asio）→ LLM 概念 → llama.cpp 引擎 → GPU 算子 → vLLM 工业 serving → 长期性能与开源
```
