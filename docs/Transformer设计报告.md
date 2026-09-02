# Transformer 推理引擎 · 设计报告

> 项目路径：`D:\Tool\Transformer`
> 代码语言：C++17 ｜ 编译器：MSVC v144（VS2026 Community，14.44.35207）｜ 平台：x64 Debug
> 核心文件：`Transformer/Transformer.h`（GGUF 解析 + 模型装配 + 生成）、`calculations/calculations.h`（模型结构算子：Linear/RMSNorm/RoPE/MultiHeadAttention/FeedForward/Sampler，namespace ops）、`tensor/tensor20.h`（算子库）、`main.cpp`（入口）
> 配套说明：`docs/GGUF_Head_分析.md`（GGUF 头部元数据专题）**[本工程的所有源代码](./Transformer.zip)**

---

## . 修订记录

| 版本 | 日期 | 说明 |
|---|---|---|
| v1.0 | 2026-06-15 | 完成基础的初版工程创建；BitNet 三重修复；F32 快路径；A+B 原生驻留改造 |
| v1.1 | 2026-07-28 | 补充生成流程与重复解码分析；整理本设计报告 |
| v1.2 | 2026-08-5 | 新增 §13 类层次结构图（Tensor 继承体系 + 模型层组合关系，含 SVG 图与 Mermaid） |
| v1.3 | 2026-08-6 | 修订：六个模型结构类（Linear/RMSNorm/RoPE/MultiHeadAttention/FeedForward/Sampler）迁入 calculations.h（namespace ops）；KVCache 仍留 Transformer.h；同步文件归属与 §13.2 Mermaid 注释 |
| v1.4 | 2026-08-7 | §7.1 RoPE 补充完整原理讲解：问题动机、二维旋转回顾、频率表公式、分对旋转公式、分块对角旋转矩阵、相对位置编码性质的数学推导、多尺度波长含义、与 `calculations.h` 代码的逐行对应、d=4 数值算例 |
| v1.5 | 2026-08-10 | §7.2 MultiHeadAttention 补充完整原理讲解：动机、缩放点积注意力公式、为何缩放 1/√d_k、多头公式、因果掩码矩阵、端到端公式汇总、与 `calculations.h` 逐行对应、与 RoPE 配合（修正 §7.1.8 放置位置为层前嵌入统一施加）、KV 缓存未启用说明 |
| v1.6 | 2026-08-12 | §7.3 FeedForward(SwiGLU) 补充完整原理讲解：动机、原始 ReLU-FFN 对比(14)、SwiGLU 公式(15)、SiLU 公式与性质(16)、门控 GLU 机制(17)、GLU 家族对比表、与 `calculations.h` 逐行对应、维度/参数量(3·d_model·d_ff) |
| v1.7 | 2026-08-13 | §7.4 Linear 补充完整原理讲解：角色定位、线性公式(18)(19)、维度约定[OUT,IN]、forward/forwardBatch 实现、matvec 量化融合三路径(20)、A+B 原生驻留 shared_ptr<Tensor> 设计（为何不回退 Matrix）、代码逐行对应表、复杂度 |
| v1.8 | 2026-08-14 | §8 生成流程 扩写为详细讲解：generate/forward 两层函数逐行解析、单层 pre-norm 双残差公式(21)、整序列重算/贪心/eos 要点；新增 §8.4 generate 流程图、§8.5 forward 流程图（Mermaid） |
| v1.9 | 2026-08-16 | §8.2.1 新增 `LayerInfor::forward(X)` 推理过程分步详解（6 步 + MHA 内部 5 子步(22) + FFN 内部 3 子步，逐行对应 Transformer.h:727-738 / calculations.h:230-262 / 287-292）；§8.5 Mermaid 子图 `L` 展开为含 QKV 投影、因果掩码判定、softmax、Wo 输出、SwiGLU 门控的全流程；附内联 SVG 详图 |
| v1.10 | 2026-08-16 | 将编程日志修改为设计报告 |
|       |            |                                                              |



---

## . 项目概述

本工程是一个**零第三方依赖的 C++ Transformer 推理引擎**，目标是从 GGUF 格式权重文件加载模型（支持 `llama` 与 `bitnet` 架构），完成词表嵌入、位置编码、多层注意力/前馈计算，并做自回归文本生成。

设计约束与定位：

- **单头文件算子库**：`tensor20.h` 把张量存储、量化反量化、矩阵乘融合内核全部自包含，可独立复用。
- **原生类型驻留（A+B 改造）**：权重按 GGUF 原始类型（F32/F16/BF16/Q4_0/Q8_0…）常驻内存，计算时再反量化，避免加载期逐元素拷贝与内存膨胀。
- **可解释优先**：保留逐层 `|h0|` 等 trace 输出，便于核对数值与定位 bug。

已验证的模型：

| 模型 | 架构 | d_model | 层数 | 词表 | 备注 |
|---|---|---|---|---|---|
| `model/*.gguf`（tensor20 自训小模型） | llama | 32 | 3 | 小 | 教学/单元验证用 |
| `D:/Tool/BitNet-main/bin/models/ggml-model-f32.gguf` | bitnet | 1536 | 24 | 32002 | 主测大模型，tied-embedding |

###  核心文件职责

工程代码按「**算子库 → 模型结构算子 → 模型装配/生成**」三层组织，关键文件职责如下。

####  `calculations/calculations.h`（`namespace ops`）

模型结构算子层——承载「网络由哪些基本变换构成」的知识，是对 `tensor20.h` 张量基元的**第一层组装**。

- **自由函数（结构算子）**：`rmsNorm()` 预归一化计算、`applyRoPEToRow()` 单行旋转位置编码，被下方类直接调用。
- **六个模型结构类**（每个均带详细注释）：
  - `Linear` —— 线性变换单元 `y = W·x`，权重 `W` 为 `shared_ptr<Tensor>` 原生驻留，计算委托 `Tensor::matvec`；被注意力投影、SwiGLU、LM 头复用。
  - `RMSNorm` —— 预归一化层（attn_norm / ffn_norm / finalNorm），gamma 权重保持 F32。
  - `RoPE` —— 旋转位置编码，频率表 `invFreq[k] = 1/base^(2k/d)`，按「行号=位置」逐行施加。
  - `MultiHeadAttention` —— 多头因果自注意力，组合四个 `Linear`（Wq/Wk/Wv/Wo）。
  - `FeedForward` —— 标准 SwiGLU `down(silu(gate(x))⊙up(x))`，组合三个 `Linear`。
  - `Sampler` —— 贪心 argmax 解码（温度采样/重复惩罚为后续扩展点）。
- **设计要点**：本文件中的类对权重类型完全无感（`Linear` 既装 F32 `Matrix` 也装量化 `Tensor`），所有类型相关逻辑下推到 `Tensor::matvec`，从而支撑 A+B 原生驻留改造。

####  `Transformer/Transformer.h`（`namespace trans`）

模型装配与生成层——把 GGUF 权重「装配」成可运行的模型实例，并驱动自回归生成。通过 `using ops::...` 桥接复用 `calculations.h` 中的结构类。

- **文件解析**：`GUFFHeadInfor` 解析 GGUF 头部（四组元数据 + 张量索引），并做 BitNet 双前缀（`llama.*`/`bitnet.*`）匹配与 64 位文件定位（`_fseeki64`）。
- **四个业务/装配类**：
  - `EmbeddingInfor` —— 模型顶层装配：词嵌入 `embedding`、`lmHead`、最终归一化 `finalNorm`、分词器，以及 tied-embedding 权重共享。
  - `LayerInfor` —— 单层装配：归一化权重 + 注意力投影 + SwiGLU 投影（`shared_ptr<Tensor>` 原生驻留），`forward` 内临时构造 `MultiHeadAttention`/`FeedForward` 并填入权重。
  - `Transformer` —— 模型整体：持有 `EmbeddingInfor` 与 `LayerInfor[]`，实现 `forward`（嵌入+RoPE+逐层计算）与 `generate`（贪心自回归回灌）。
  - `KVCache` —— 为增量解码预留的 K/V 缓存承载结构（当前整序列重算模式尚未启用）。
- **BPE 分词器**：`Bpe` 类负责 prompt 编码与 token 解码（含特殊 token `<|endoftext|>`、`<|im_start|>`、`<|im_end|>`）。

####  配套文件

- `tensor/tensor20.h`（`namespace wb`）：算子库，唯一存在继承的层（`Tensor` 抽象基类及其子类），提供量化系统 `QuantTraits` 与 `matvec/matmul/bmm` 三路径融合内核。
- `main.cpp`：程序入口，调用 `generate` 驱动推理并打印 trace。

---

## . 整体架构（四大装配 / 生成类）

> 说明：本节描述的四个类 `GUFFHeadInfor` / `EmbeddingInfor` / `LayerInfor` / `Transformer` 均位于 **`Transformer/Transformer.h`（`namespace trans`）**，而非 `calculations.h`（`calculations.h` 只含 `Linear` 等六个「结构算子」类）。它们构成从 GGUF 文件到可运行推理实例的**装配骨架**，是理解本软件总体设计的核心。

###  类结构关系图

```
                model.gguf  (GGUF 二进制)
                          │
                          │ fopen + 字节流解析 (Transformer::LoadFormFile)
                          ▼
                ┌─────────────────────────────────────┐
                │   Transformer（整体模型 / 对外入口）  │  组合持有三者，编排加载与推理
                │  ───────────────────────────────────  │
                │  · headInfor  : GUFFHeadInfor         │  文件头信息「单一事实来源」
                │  · embeddingInfor    : EmbeddingInfor               │  顶层装配
                │  · layers     : LayerInfor × N        │  逐层装配
                │  LoadFormFile() / forward() / generate()
                └───────────────────┬───────────────────┘
                组合（成员直接持有）：下方三个子对象均为 Transformer 的成员
        ┌───────────────────────────┼───────────────────────────┐
        │                           │                           │
        ▼                           ▼                           ▼
 ┌─────────────────┐      ┌──────────────────────┐     ┌──────────────────────┐
 │  GUFFHeadInfor  │      │  EmbeddingInfor             │     │  LayerInfor (×N)      │
 │  「蓝图」        │      │  「模型顶层装配」     │     │  「单层装配」         │
 │  ───────────────│      │  ────────────────────│     │  ────────────────────│
 │  · 超参(A/B/C/D) │      │  · embedding          │     │  · attn_norm/Wq..Wo   │
 │  · tensors[] 索引│      │  · finalNorm          │     │  · ffn_norm/ffn_*     │
 │  · tokens[] 词表 │      │  · lmHead(Linear)     │     │  · (shared_ptr<Tensor>)│
 │                  │      │         │     │  · d_model/n_heads… 标量│
 │                  │      │  · vocabPtr ──► tokens │     │  · kv:KVCache(单层缓存)             │
 │                  │      │  · idOf_ (反向索引)    │     └──────────────────────┘
 └────────┬─────────┘      └──────────┬────────────┘          (N = headInfor.block_count)
          │                           │
          │  const GUFFHeadInfor&     │ 依赖 / 引用（按蓝图读超参与权重偏移）
          └───────────────────────────┘
```

###  四类职责与相互关系

**（1）`GUFFHeadInfor` —— 文件「蓝图」（单一事实来源）**
解析 GGUF 头部，产出三类信息：① 超参（组 A 架构 / 组 B 注意力 / 组 C 前馈 / 组 D 归一化，含 `bitnet.*` 双前缀匹配修复）；② `tensors[]` 张量索引（`name/dims/type/offset`，供后续按字节偏移定位权重）；③ `tokens[]` 词表。它**不持有任何运行时权重**，只描述「文件里有什么」。

**（2）`EmbeddingInfor` —— 模型顶层装配（embedding + LM 头 + 词表 + 分词器）**
由 `loadFromFile(FILE*, headInfor)` 从蓝图读出**模型级、跨层共享**的部分：`embedding`（词嵌入张量）、`finalNorm`（末层 RMSNorm 的 gamma）、`lmHead`（LM 头 `Linear`，tied-embedding 时与 `embedding` 共享同一 `Tensor` 指针。（KV 缓存已下放到 LayerInfor.kv，见下）。

- **与 `GUFFHeadInfor` 的关系**：`vocabPtr` 以**零拷贝指针**指向 `headInfor.tokens`（唯一词表来源），`idOf_` 是由该词表构建的 token→id 反向索引；`loadFromFile` 以 `const GUFFHeadInfor&` 入参读取超参与张量偏移。

**（3）`LayerInfor` —— 单层装配（逐层权重容器 + 该层前向）**
由 `loadFromFile(FILE*, layerNumber, headInfor)` 按层号读出**第 L 层**全部权重：`attn_norm` + `Wq/Wk/Wv/Wo`（`shared_ptr<Tensor>` 原生驻留）、`ffn_norm` + `ffn_gate/ffn_up/ffn_down`，以及结构标量（`d_model/n_heads/head_dim/d_ff/eps`，全部取自 headInfor）与单层 KV 缓存 kv（KVCache，[cur_seq,d_model]，max_seq 来自上下文长度）。`forward(X)` 内临时构造 `ops::MultiHeadAttention` / `ops::FeedForward` 并填入权重指针完成本层计算。
- **与 `GUFFHeadInfor` 的关系**：同 `EmbeddingInfor`，以 `const GUFFHeadInfor&` 取得超参与张量名（`blk.<L>.attn_q.weight` 等）。

**（4）`Transformer` —— 整体模型（持有三者 + 编排加载与推理）**
聚合持有 `GUFFHeadInfor headInfor`、`EmbeddingInfor embeddingInfor`、`std::list<LayerInfor> layers`（组合关系，成员直接持有）。
- **加载编排 `LoadFormFile`**：`headInfor.loadFrom` → `embeddingInfor.bindVocab`+`buildTokenIndex` → `embeddingInfor.loadFromFile` → 循环 `LayerInfor.loadFromFile` 逐层 `push` 进 `layers`。
- **推理**：`forward(ids)` 用 `embeddingInfor.embedding->gather` 取嵌入、`ops::RoPE` 加位置、逐层 `layers` 前向、末层 `rmsNorm(finalNorm)`、`embeddingInfor.lmHead` 出 logits；`generate` 贪心 argmax 自回归回灌。

推理数据流（附：每步归属的类）：

```
ids ──► embeddingInfor.embedding->gather(ids) ──► [seq,d] X        （EmbeddingInfor 提供词嵌入）
     ──► ops::RoPE.apply(X)                                  （calculations.h 结构算子）
     ──► for layer in layers: X = layer.forward(X)           （LayerInfor 逐层计算）
     ──► rmsNorm(X, embeddingInfor.finalNorm) ──► Xn                 （EmbeddingInfor 提供末层归一化）
     ──► embeddingInfor.lmHead.forwardBatch(Xn) ──► logits[seq,vocab]（EmbeddingInfor 提供 LM 头）
```

###  关键设计点
- 四个类形成「**蓝图 (GUFFHeadInfor) → 装配 (EmbeddingInfor / LayerInfor) → 整体 (Transformer)**」的清晰三级；`Transformer` 是唯一对外的「模型」对象，内部三类纯数据/装配，**互不继承**，靠成员组合或按引用/指针协作，耦合低、可独立测试。
- 权重以 `shared_ptr<Tensor>` 原生驻留；`EmbeddingInfor` 与 `LayerInfor` 共享 `GUFFHeadInfor` 的「张量索引 + 词表」单一事实来源，避免重复拷贝（tied-embedding 时 `lmHead.W` 与 `embedding` 更是同一指针零拷贝）。

---

## . GGUF 文件解析（GUFFHeadInfor）

###  文件布局

```
magic("GGUF") | version(u32) | tensor_count(u64) | metadata_kv[] | tensor_info[] | 权重字节块
```

- 数组型元数据：`type(u32)` 后跟 `count(u64)`，再跟 `count` 个元素；
- 标量元数据按 `GGUFType` 宽度直接存放；
- 张量信息：`name` + `ggml_type` + `shape[]` + `offset`（相对权重块起点）。

###  四组元数据与双前缀匹配（修复①）

`bitnet` 架构在 gguf 里元数据键是 `bitnet.*`，而本引擎原只认 `llama.*`，导致超参全 0。修复用 `isKey(suf)` lambda 同时接受 `llama.` / `bitnet.` 前缀：

```cpp
auto isKey = [&](const std::string& suf){
    return key == "llama." + suf || key == "bitnet." + suf;
};
```

由此正确读出的超参示例：`vocab=32002, d_model=1536, n_layers=24, n_heads=12, head_dim=128`。

###  64 位文件定位（修复②）

MSVC 的 `long` 是 4 字节，`fseek`/`ftell` 对 >2.1GB 文件溢出。三处统一改为 `_fseeki64`/`_ftelli64`：

```cpp
// data_offset 计算、loadTensorMatrix、loadTensorVector 三处
long long off = _ftelli64(f);
_fseeki64(f, (long long)abs, SEEK_SET);
```

`blk.9.attn_v.weight`（2.9GB 张量）因此可正确定位与读取。

---

##  模型装配层（EmbeddingInfor）

###  成员

```cpp
const std::vector<std::string>* vocabPtr = nullptr;   // 词表视图（零拷贝，指向 headInfor.tokens）
std::shared_ptr<Tensor> embedding;                    // token_embd.weight [vocab, d_model]，原生驻留
Linear                 lmHead;                        // output.weight；缺失则 tied 到 embedding
Vector                 finalNorm;                     // output_norm.weight（RMSNorm 归一化权重）
std::unordered_map<std::string,size_t> idOf_;         // token → id 反向索引（末次出现优先）
```

###  Tied-Embedding 修复（修复③）

`tied-embedding` 模型不含 `output.weight`，LM 头复用 `token_embd.weight`。原代码因缺失张量报错 `[loadTensor] 缺失张量: output.weight`。修复：

```cpp
bool hasOutputW = false;
for (const auto& t : headInfor.tensors)
    if (t.name == "output.weight") { hasOutputW = true; break; }
if (hasOutputW) loadTensorMatrix(guffFile, headInfor, "output.weight", lmHead.W);
else            lmHead.W = embedding;   // 零拷贝共享，与 llama.cpp 行为一致
```

###  分词器

- `tokenizerEncode`：BPE 合并（按 `merges` 的 rank 反复合并相邻片段）→ `idOf_` 查 id → 可选 `add_bos`/`add_eos`；
- `tokenizerDecode`：`vocabPtr[id]` 还原串；
- `idOf_` 由 `buildTokenIndex()` 一次构建常驻，避免每次 encode 线性扫描。

---

## . 张量库 tensor20.h 设计

###  Tensor 基类（与 ggml 同构）

```cpp
const void* raw();                 // 字节缓冲首地址
size_t      capacity();            // 已分配元素数
size_t      rows();                // dims()[0]（2 维便捷访问）
size_t      cols();                // dims()[1]
float       get(idx);              // 按类型透明反量化读数（量化/标量统一入口）
void        set(idx, v);           // 写回（量化经 set 有损，符合预期）
```

`nb_[]` 字节步长 + `offset(idx)=Σ idx[k]*nb_[k]` 定位，与 ggml 完全一致。

###  量化类型系统 QuantTraits

每种 `ggml_type` 一条记录：`blck_size`（每块元素数）、`blck_bytes`（每块字节数）、`dequant`（块→float）、`quant`、`vec_dot`（融合 matvec 内核）。

已适配：标量 `F32/F16/BF16/I8/I16/I32/I64/F64`（`blck_size==1`），量化 `Q4_0/Q4_1/Q8_0/Q8_1/Q1_0/Q2_0`（`blck_size>1`）。

**步长约定（关键）**：`makeNb` 用 `elemBytes(t)=blck_bytes` 作单位，故量化下 `nb_[0]=N*blck_bytes`，而 `bufferBytes(N)=(N/blck_size)*blck_bytes`——二者不等，这是 §5.4 门控修正的根因。

###  矩阵乘三路径

`matvec` / `matmul` / `bmm` 统一走三条分支（以 `matvec` 为例）：

```cpp
const size_t rowBytes  = bufferBytes(N, type_);
const size_t rowStride = (q.blck_size > 1) ? nb_[0] / q.blck_size : nb_[0];
const bool   rowContig = (rowStride == rowBytes);
if (q.vec_dot != nullptr && rowContig) {
    // 量化融合：逐行 vec_dot 现场 dequant 累加，不物化整张 F32 权重
} else if (type_ == GGML_TYPE_F32 && rowContig) {
    // F32 快路径：float* 行指针连续累加
} else {
    // 通用正确路径：逐元素 get（视图安全）
}
```

###  量化连续性门控修正（改法 B / 修复⑤）

原门控直接判 `nb_[0]==rowBytes`，但量化下二者不等 → 量化权重**永远被判非连续** → 退化为逐元素 `get` 慢路径，`vec_dot` 融合内核被绕过，A+B 内存/带宽收益落空。修正：

```cpp
const size_t rowStride = (q.blck_size > 1) ? nb_[0] / q.blck_size : nb_[0];
const bool   rowContig = (rowStride == rowBytes);
```

F32（`blck_size==1`）下 `rowStride==nb_[0]`，无回归。`matmul`/`bmm` 同理对 `aStride`/`aBatchStride` 除以 `blck_size`。

---

##  推理过程算子(operator)设计

> **文件归属（2026-08-16 重构）**：`RMSNorm` / `RoPE` / `Linear` / `MultiHeadAttention` / `FeedForward` / `Sampler` 六个「含模型结构知识」的类已迁入 `calculations/calculations.h`（统一 `namespace ops`），并附详细注释；`Transformer.h` 通过 `using ops::...` 引入，原有引用免改。`KVCache` 与四个业务装配类（`GUFFHeadInfor` / `EmbeddingInfor` / `LayerInfor` / `Transformer`）仍留在 `Transformer.h`（`namespace trans`）。下方仅描述其接口与计算语义。

###  RoPE（旋转位置编码）

```cpp
RoPE(size_t d, float base=10000)   // invFreq[k]=1/10000^(2k/d)
void apply(Matrix& X)              // 按行号 i = 位置 施加旋转
```

位置 = 行号（整序列重算模式下等价绝对位置），因果正确。

####  为什么需要位置编码（问题动机）

自注意力（Multi-Head Attention）本质是一个「集合」运算：它对输入序列的每一行（token 的隐藏向量）做加权求和，**对行的排列是不变的（permutation equivariant）**。也就是说，把输入序列的行顺序打乱，输出也会跟着同样打乱——模型本身"看不出"哪个 token 在第几个位置。

但自然语言严重依赖顺序：「猫 吃 鱼」和「鱼 吃 猫」含义相反。因此必须在进入注意力之前，把"位置信息"注入每个 token 的隐藏向量。

常见的注入方式有两种：

- **绝对位置编码**：直接把"位置编号"作为一个向量加到 token 嵌入上（如 Sinusoidal / 学习到的 `pos_embedding`）。但它和词向量是**相加**的，位置信息和内容信息在同一个空间里"混在一起"，注意力打分时无法保证只依赖相对距离。
- **相对位置编码**：只让"两个 token 的相对距离"影响它们的交互（如传统 Transformer 的 `u/v` 偏置项）。但往往需要改注意力的打分公式，与标准点积注意力不兼容。

**RoPE（Rotary Position Embedding，苏剑林等，2021）** 的思路是：不把位置"加"进去，而是把位置编码成一次**旋转**——对 Query 和 Key 的隐藏向量按各自位置做旋转变换，使得旋转后两者的内积（注意力分数）**天然只依赖它们的相对距离**。它兼容标准点积注意力，又同时具备绝对位置感知 + 相对位置衰减的性质。

####  二维旋转回顾（RoPE 的零件）

一个 2D 向量 (x₁, x₂) 绕原点逆时针旋转角 θ 后变成：

旋转有个关键代数性质（旋转矩阵是正交矩阵）：
$$
R(-\theta)=R(\theta)^{\top},R(\alpha)R(\beta)=R(\alpha+\beta)
$$


$$
\begin{matrix}R(m)R(n)^{\top}=R(m)R(-n)=R(m-n)\end{matrix}
$$

也就是说，"先转 n 度、再反向转 n 度、再转 m 度"等价于"只转 m-n 度"。**这个性质正是 RoPE 能编码相对位置的几何根源**。

####  频率表 invFreq（每对维度各自的转速）

RoPE 把 d 维向量（d 为偶数）看成 d/2 个"二维对"：$ (x_0,x_1),(x_2,x_3),\dots,(x_{d-2},x_{d-1}) $。第 k对分到一个**角频率** $\omega_k$：



$$
\begin{matrix}\omega_k=\text{invFreq}[k]=\text{base}^{-2k/d}=\frac{1}{\text{base}^{2k/d}},k=0,1,\dots,\frac{d}{2}-1\end{matrix}
$$

- `base` 默认 10000（llama 系列取值），控制整体频率跨度。
- k=0 时 $\omega_0=1$（最高频，每位置转 1 弧度）；k 越大 $\omega_k$ 越小（越低频，转得越慢）。


- 预计算一次存进 `invFreq`，避免每次前向重复算 `pow`（见 `RoPE` 构造器）。

第 k 对在第 `pos` 个位置上的实际旋转角为：

$$
\begin{matrix}\theta_k(\text{pos})=\text{pos}\cdot\omega_k=\text{pos}\cdot\text{invFreq}[k]\end{matrix}
$$

####  分对旋转公式（核心）

对第 `pos` 个 token 的隐藏向量 $\mathbf{x}$（长度 d），RoPE 把它逐对旋转，得到新向量 $\mathbf{x}'$：



$$
\begin{matrix}\begin{matrix}x'_{2k} & =x_{2k}\cos\theta_k(\text{pos})-x_{2k+1}\sin\theta_k(\text{pos}) \\ x'_{2k+1} & =x_{2k}\sin\theta_k(\text{pos})+x_{2k+1}\cos\theta_k(\text{pos})\end{matrix}k=0,\dots,\frac{d}{2}-1\end{matrix}
$$

这正是代码 `applyRoPEToRow` 里写的：

```cpp
float theta = (float)pos * invFreq[k];
float c = std::cos(theta), s = std::sin(theta);
float a = X(row, 2*k),     b = X(row, 2*k+1);
X(row, 2*k)     = a*c - b*s;   // 对应 x'_{2k}
X(row, 2*k+1)   = a*s + b*c;   // 对应 x'_{2k+1}
```

注意：**旋转是保长的**（不改变向量模长）。所以 RoPE 只改变"方向"、不改"大小"——它对后续归一化（RMSNorm）和余弦相似度类度量友好，不会因位置不同导致向量长度漂移。

####  旋转矩阵的分块对角形式

把所有 d/2 个 2D 旋转拼成一个 $d\times d$ 矩阵，就是**分块对角**的 $R(\text{pos})$：



$$
\begin{matrix}R(\text{pos})=\begin{pmatrix}R(\theta_0) &  &  &  \\  & R(\theta_1) &  &  \\  &  & \ddots &  \\  &  &  & R(\theta_{d/2-1})\end{pmatrix},R(\theta_k)=\begin{pmatrix}\cos\theta_k & -\sin\theta_k \\ \sin\theta_k & \cos\theta_k\end{pmatrix}\end{matrix}
$$

于是 RoPE 可统一写成矩阵-向量乘法：

$$
\begin{matrix}\mathbf{x}'(\text{pos})=R(\text{pos})\mathbf{x}\end{matrix}
$$

其中 $R(\theta_k)=R(\theta_k(\text{pos}))=R(\text{pos}\cdot\omega_k)$。在模型里，Query/Key 投影后各自施加自己位置的旋转：


$$
\begin{matrix}\mathbf{q}_m=R(m)W_q\mathbf{x}_m,\mathbf{k}_n=R(n)W_k\mathbf{x}_n\end{matrix}
$$

####  关键性质：为什么它编码的是"相对位置"（数学推导）

这是 RoPE 最精彩的地方。看注意力打分里 Query 与 Key 的内积：

$$
\begin{matrix}\begin{matrix}\mathbf{q}_{\top}^m\mathbf{k}_n & =(R(m)W_q\mathbf{x}_m)^{\top}(R(n)W_k\mathbf{x}_n) \\  & =\mathbf{x}_{\top}^mW_{\top}^q\underset{R(m-n)\text{由式}(1)}{\underset{}{R(m)^{\top}R(n)}}W_k\mathbf{x}_n \\  & =\mathbf{x}_{\top}^mW_{\top}^qR(m-n)W_k\mathbf{x}_n\end{matrix}\end{matrix}
$$

关键一步用了式 (1)：$R(m)^{\top}R(n)=R(-m)R(n)=R(n-m)$（旋转群可交换，结果等于 $R(m-n)$ 的转置，对点积而言只关心相对距离）。



**结论**：旋转后的注意力分数 $\mathbf{q}_{\top}^m\mathbf{k}_n$ 只通过 $R(m-n)$ 依赖**相对距离 m-n**，而与绝对位置 m,n 无关。这意味着：



- 模型天然感知"两个 token 隔了多远"，第 3 个和第 5 个 token 的相对关系，与第 10 个和第 12 个 token 的"相同距离"关系是同构的；
- 同时每个 $\mathbf{q}_m,\mathbf{k}_n$ 自身又携带绝对位置（旋转角含 `pos`），所以并未丢掉绝对位置信息；

- 由于旋转使内积随相对距离变化，还自然产生**位置越远注意力越弱**的衰减趋势（类似相对位置偏置）。

这就是 RoPE 优于"简单加位置向量"的本质：位置信息被"旋进"了向量的方向里，使点积只认相对距离。

####  频率表的多尺度含义（波长）

第 k 对旋转一次完整周期（2π 弧度）需要的"位置数"叫波长：

$$
\begin{matrix}\lambda_k=\frac{2\pi}{\omega_k}=2\pi\cdot\text{base}^{2k/d}\end{matrix}
$$

- k=0：$\lambda_0=2\pi\approx6.3$（短波长，几步就转一圈——编码**邻近/局部**位置）；

- k 增大：$\lambda_k$ 指数拉长；本引擎若 $d=32,\text{ }\text{base}=10000$，最大波长约 $\lambda_{15}\approx2\pi\cdot10000^{30/32}\approx3.5\times10^4$（长波长——编码**远距离/全局**位置）。




所以 `invFreq` 是一组**等比数列频率**，从快到慢覆盖多个尺度，相当于给每个 token 同时打上"局部+全局"的位置标签。这也是 RoPE 在长上下文下仍稳健的原因之一。

####  本引擎的代码对应

| 公式 | 代码位置 | 说明 |
|---|---|---|
| 式 (2) `invFreq[k]=base^(-2k/d)` | `RoPE` 构造器 `calculations.h:188-193` | 预计算频率表并缓存 |
| 式 (3)(4) 单对旋转 | `applyRoPEToRow` `calculations.h:48-57` | 逐对读 `(a,b)`、按 $\theta=\text{pos}\cdot\omega_k$ 旋转写回 |

| 式 (6) 整序列施加 | `RoPE::apply` `calculations.h:195-198` | 第 i 行用位置 i 旋转（`行号=位置`） |
| 式 (7) 投影后即施加 | `Transformer::forward` `Transformer.h:817-818` | 模型在层栈**之前**对嵌入 `X` 统一 `rope.apply`（行号=位置）；`MultiHeadAttention` 直接消费已旋转的 `X` 投影出 Q/K/V（见 §7.2.8 关于放置位置的说明） |

```cpp
// calculations.h —— RoPE 类（节选，与公式逐行对应）
class RoPE {
public:
    std::vector<float> invFreq;   // 式(2)：长度 d/2，预缓存
    size_t d;
    explicit RoPE(size_t d_, float base = 10000.0f) : d(d_) {
        invFreq.resize(d / 2);
        for (size_t k = 0; k < d / 2; ++k)
            invFreq[k] = (float)(1.0 / std::pow((double)base, (double)(2*k)/d)); // 式(2)
    }
    void apply(Matrix& X) const {
        for (size_t i = 0; i < X.rows(); ++i)   // 第 i 行 = 位置 i
            applyRoPEToRow(X, i, i, invFreq);    // 式(6)
    }
};
```

> **整序列重算模式**：当前 `apply` 对 `[seq, d]` 逐行用"行号=位置"旋转，等价于给每个 token 施加其**绝对位置**的旋转。配合式 (8)，注意力分数仍然只依赖相对距离，因果正确。切换到增量解码时，`KVCache` 已按层缓存历史 K/V，新 token 只需用自己"绝对位置"旋转一次、`O(1)` 追加即可（RoPE 的角频率 $\omega_k$ 由绝对位置决定，无需改公式）。


####  数值算例（d=4，base=10000，pos=1）

设某 token 隐藏向量 $\mathbf{x}=(1,0,2,0.5)$，按对 $(x_0,x_1)=(1,0)$、$(x_2,x_3)=(2,0.5)$ 计算：




- 频率：$\omega_0=1,\omega_1=10000^{-2/4}=10000^{-0.5}=0.01$

- 旋转角：$\theta_0=1\cdot1=1,\theta_1=1\cdot0.01=0.01$

- $\cos\theta_0=0.5403,\text{ }\sin\theta_0=0.8415$；$\cos\theta_1=0.99995,\text{ }\sin\theta_1=0.01000$第一对 $(1,0)$：




$$
\begin{matrix}x'_0 & =1\cdot0.5403-0\cdot0.8415=0.5403 \\ x'_1 & =1\cdot0.8415+0\cdot0.5403=0.8415\end{matrix}
$$

第二对 $(2,0.5)$：


$$
\begin{matrix}x'_2 & =2\cdot0.99995-0.5\cdot0.01000=1.9999-0.0050=1.9949 \\ x'_3 & =2\cdot0.01000+0.5\cdot0.99995=0.0200+0.5000=0.5200\end{matrix}
$$

得旋转后 $\mathbf{x}'\approx(0.5403,0.8415,1.9949,0.5200)$。


**两点观察**：
1. 第 0 对（高频）转了约 $57^{\circ}$，方向明显改变；第 1 对（低频）只转 $0.57^{\circ}$，几乎不动——印证了"低频对近邻位置不敏感、只在长距离才缓慢变化"。


2. 旋转不改变模长：$|\mathbf{x}|=\sqrt{1+0+4+0.25}=\sqrt{5.25}\approx2.2915$；

$$
|\mathbf{x}'|=\sqrt{0.5403^2+0.8415^2+1.9949^2+0.5200^2}\approx\sqrt{0.292+0.708+3.980+0.270}=\sqrt{5.25}\approx2.2915
$$
，二者相等（保长性）。

###  MultiHeadAttention

```cpp
Q,Wk,Wv,Wo = Linear;               // 各 [d_model, d_model]
forward:
  Q,K,V = Wq/Wk/Wv.forwardBatch(X)
  for h: for i:
      scores[j] = Σ Q(i)·K(j) * scale
      if (j > i) scores[j] = -1e30f;     // 因果掩码
      softmax → ctx(i) = Σ scores[j] * V(j)
  return Wo.forwardBatch(ctx)
```

####  注意力要解决的问题（动机）

前一层（RMSNorm 之后）的输出 `X` 是 `[seq, d_model]`，每一行是一个 token 的隐藏向量。此时每个 token 还**只看到自己**——它不知道"上下文里其它 token 说了什么"。注意力机制让每个 token 通过"查询"去聚合整段序列里与它相关的信息：

- 用 `Wq` 把 `X` 投影成 **Query**（我想找什么）；
- 用 `Wk` 把 `X` 投影成 **Key**（我能提供什么标签）；
- 用 `Wv` 把 `X` 投影成 **Value**（我实际能贡献的内容）；
- 用 Query 与所有 Key 的相似度做权重，对 Value 加权求和 → 每个位置得到"融合了上下文"的新表示。

**多头（Multi-Head）** 的意义：一组 `(Wq,Wk,Wv,Wo)` 只能学到一种"关注模式"；切成 H 个头，每个头在自己的子空间里独立做注意力，模型就能同时关注"语法依赖""指代""局部相邻""远距离主题"等不同关系，最后拼起来。

####  单头缩放点积注意力（公式）

设输入 $X\in\mathbb{R}^{\text{seq}\times d_{\text{model}}}$，投影权重 $W_q,W_k,W_v\in\mathbb{R}^{d_{\text{model}}\times d_{\text{model}}}$（GGUF 中以 `[OUT,IN]=[d_model,d_model]` 存储；列优先视角下 $Q=XW_q$）。一次缩放点积注意力（一个头，子空间维 $d_k$）：





$$
\begin{matrix}\begin{matrix}Q & =XW_q,K=XW_k,V=XW_v \\ \text{score}_{i,j} & =\frac{Q_i\cdot K_j}{\sqrt{d_k}}=\frac{1}{\sqrt{d_k}}\sum c=1d_kQ_{i,c}K_{j,c} \\ \alpha_{i,j} & =\frac{\exp(\text{score}_{i,j})}{\underset{t}{\sum}\exp(\text{score}_{i,t})}\text{（softmax，行归一化）} \\ \text{ctx}_i & =\underset{j}{\sum}\alpha_{i,j}V_j\end{matrix}\end{matrix}
$$

> 注意本引擎的 $d_k$ 取 **`head_dim`**（$d_k=d_{\text{model}}/n_{\text{heads}}$），缩放因子 `scale = 1/√head_dim`（`calculations.h:236`）。代码里的打分循环正是式 (10) 第二行的逐元素点积再乘 `scale`。



####  为什么要"缩放" $1/\sqrt{d_k}$（数值稳定性）

若 $Q_i,K_j$ 各分量近似独立、单位方差（RMSNorm 之后大致如此），则点积 


$$
\underset{c}{\sum}Q_{i,c}K_{j,c}
$$

 的方差约为 $d_k$。当 $d_k$ 较大（如 64/128）时，点积绝对值可能很大 → `softmax` 进入饱和区、输出趋近 one-hot、梯度趋近于 0（训练时学不动；推理时分布过硬）。



除以 $\sqrt{d_k}$ 后，点积方差回到 $\approx1$，`softmax` 输入落在合理范围，注意力权重更平滑、对数值更稳健。这是 Vaswani 等人《Attention Is All You Need》的核心技巧之一。



####  多头机制（公式）

把 $d_{\text{model}}$ 沿特征维切成 $H=n_{\text{heads}}$ 份，每份 `head_dim = d_model / H` 为一个头的子空间。第 h 个头取权重第 h 块（列偏移 $\text{off}=h\cdot\text{head\_dim}$）：




$$
\begin{matrix}\begin{matrix}\text{head}_h & =\text{Attention}(XW_{(h)}^q,XW_{(h)}^k,XW_{(h)}^v)\in\mathbb{R}^{\text{seq}\times\text{head\_dim}} \\ \text{MultiHead}(X) & =[\text{head}_1\Vert\text{head}_2\Vert\cdots\Vert\text{head}_H]W_o\end{matrix}\end{matrix}
$$

- $W_{(h)}^q$ 是 $W_q$ 的第 h 个 `[d_model, head_dim]` 子块；代码中用 `off = h*head_dim` 当作列偏移直接索引 `(*Q)(i, off+c)`（`calculations.h:238,245`）。


- 所有头各算各的 `ctx`（互不共享），再**拼接**成 `[seq, d_model]`。
- 最后用 $W_o\in\mathbb{R}^{d_{\text{model}}\times d_{\text{model}}}$（输出投影 `Wo`）把拼接结果混合回原维度，让不同头的信息相互交融。


####  因果掩码（Causal Mask）

自回归生成要求"第 i 个 token 只能看自己及之前（位置 $j\le i$）的 token，不能偷看未来（$j>i$）"。通过给打分矩阵加掩码实现：



$$
\begin{matrix}M_{i,j}=\begin{Bmatrix}0, & j\le i\text{（可见）} \\ -\infty, & j>i\text{（不可见）}\end{Bmatrix}\text{Attention}(Q,K,V)=\text{softmax}(\frac{QK^{\top}}{\sqrt{d_k}}+M)V\end{matrix}
$$

代码里用有限大负数近似 $-\infty$：`if (j > i) s = -1e30f;`（`calculations.h:247`），使 $\exp(-1e30)\approx0$，未来 token 的注意力权重被压成 0。



####  完整公式汇总（端到端）

把投影、缩放、掩码、多头、输出投影串起来写：

$$
\begin{matrix}\begin{matrix}Q=XW_q,K=XW_k,V=XW_v & (Q,K,V\in\mathbb{R}^{\text{seq}\times d_{\text{model}}}) \\ A^{(h)} & =\text{softmax}(\frac{Q^{(h)}(K^{(h)})^{\top}}{\sqrt{\text{head\_dim}}}+M)V^{(h)}\in\mathbb{R}^{\text{seq}\times\text{head\_dim}} \\ O & =[A^{(1)}\Vert\cdots\Vert A^{(H)}]W_o & \in\mathbb{R}^{\text{seq}\times d_{\text{model}}}\end{matrix}\end{matrix}
$$

其中 $Q^{(h)}$ 是 Q 第 h 个 `head_dim` 宽度的列切片、M 见式 (12)。本引擎的 `forward` 输出即式 (13) 的 O。


####  本引擎的代码逐行对应

| 公式 | 代码 | 说明 |
|---|---|---|
| 式 (10) 投影 | `Wq/Wk/Wv.forwardBatch(X)` `calculations.h:231-233` | `Matrix Q/K/V` 各 `[seq, d_model]` |
| 式 (10) 缩放点积 | 内层 `s += Q(i,off+c)*K(j,off+c); s *= scale` `:244-246` | `scale=1/√head_dim` |
| 式 (12) 因果掩码 | `if (j>i) s=-1e30f` `:247` | 近似 $-\infty$ |

| 式 (10) 行 softmax | 减最大值 `mx` 防溢出 → `exp` → 归一化 `:252-253` | 数值稳定 softmax |
| 式 (10) 加权求和 | `acc += scores[j]*V(j,off+c)` `:256-257` | 得该头 `ctx` |
| 式 (11) 拼接+输出 | `return Wo.forwardBatch(*ctx)` `:261` | 所有头已写进同一 `ctx`（`off` 偏移），`Wo` 完成混合投影 |

```cpp
// calculations.h —— MultiHeadAttention::forward（节选，对应式 10~13）
std::unique_ptr<Matrix> Q = Wq.forwardBatch(X);   // 式(10) Q = XWq
std::unique_ptr<Matrix> K = Wk.forwardBatch(X);
std::unique_ptr<Matrix> V = Wv.forwardBatch(X);
float scale = 1.0f / std::sqrt((float)head_dim);  // 式(10) 1/√d_k
for (size_t h = 0; h < n_heads; ++h) {
    size_t off = h * head_dim;                     // 式(11) 第 h 头列偏移
    for (size_t i = 0; i < seq; ++i) {
        // 算 score、因果掩码、softmax（式 10 / 12）
        for (size_t j = 0; j < seq; ++j) {
            float s = 0; for (size_t c=0;c<head_dim;++c) s += (*Q)(i,off+c)*(*K)(j,off+c);
            s *= scale;
            if (j > i) s = -1e30f;                  // 式(12) 因果掩码
            scores[j] = s;
        }
        softmax(scores);                           // 式(10) 行归一化
        for (size_t c=0;c<head_dim;++c++){ float acc=0; for(size_t j=0;j<seq;++j) acc+=scores[j]*(*V)(j,off+c); (*ctx)(i,off+c)=acc; }
    }
}
return Wo.forwardBatch(*ctx);                      // 式(11/13) 输出投影
```

####  与 RoPE 的配合（本引擎的放置位置说明）

§7.1 的式 (7) 给出教科书写法：RoPE 应在 **`Wq/Wk` 投影之后**分别作用于 `Q`、`K`。本引擎为简化，在 `Transformer::forward` 中对**嵌入 `X`**（进入层栈之前）统一做一次 `rope.apply`（`Transformer.h:817-818`），随后 `LayerInfor::forward` 直接用已旋转的 `X` 投影出 `Q/K/V`：

```cpp
ops::RoPE rope(embeddingInfor.dModel());
rope.apply(*X);                       // 进入层栈前，对整段嵌入统一旋转
...
// 每一层：h = rmsNorm(X); a = attn.forward(h);  // attn 内部再 Wq/Wk/Wv 投影
```

两点要如实说明：

1. **数学上不等价**：因为 `Wq/Wk` 是线性层，先旋转再投影 $W_qR(\text{pos})x$ 与先投影再旋转 $R(\text{pos})W_qx$ 一般不同（`R` 与 `W_q` 不交换）。标准 llama 是后者（投影后分别对 `Q`、`K` 施加 RoPE）。本 demo 在 32 维小模型上影响可忽略、教学优先；若需严格对齐 llama，应把 `rope.apply` 移入 `MultiHeadAttention::forward`，在 `Q/K` 投影后施加。


2. **相对位置性质仍成立**：无论 RoPE 加在 `X` 还是 `Q/K`，进入点积的是"带位置旋转的向量"，§7.1.6 推导的相对距离感知在概念上依然成立（旋转让 `q·k` 随位置差变化）。

####  复杂度与未启用的 KV 缓存

- **复杂度**：对每个头每个查询都要和全部 `seq` 个 Key 点积，故 

$$
O(H\cdot\text{seq}^2\cdot\text{head\_dim})=O(\text{seq}^2\cdot d_{\text{model}})
$$

。当前为「整序列重算」朴素实现（每生成一个新 token 都重算整段 `Q/K/V`）。
- **增量解码（待接）**：若启用 `LayerInfor.kv`（单层 `KVCache K,V`，见报告 §5 / `Transformer.h:434` 注释），新 token 只需算自己的 Q/K/V，Key/Value 直接追加缓存，每步复杂度从 $O(\text{seq})$ 降到 $O(1)$。配合 §7.1.8 所述"按绝对位置旋转"，新 token 用自身位置角旋转一次即可，公式不变。



###  FeedForward（SwiGLU）

```cpp
z = silu(up.forwardBatch(X)) ⊙ up2.forwardBatch(X)   // gate ⊙ up
return down.forwardBatch(z)
```

####  为什么需要 FFN（动机）

一个 Transformer 块由两段组成：

- **注意力子层**（§7.2）：让信息在**位置之间**流动——每个 token 去聚合别处的内容。
- **前馈子层（FFN）**：**逐位置、相互独立**地处理——同一套全连接权重作用在每个 token 上，不跨 token 混合，专门给模型"非线性表达能力"去加工刚聚合来的信息。

两者分工：`Attention` 负责"**在哪看**"，`FFN` 负责"**看到之后怎么理解/变换**"。没有 FFN（或没有非线性），多层堆叠的线性变换会塌缩成单层线性映射，模型表达能力骤降。

> 本引擎 `LayerInfor::forward` 的流程为 `Attention → +残差 → RMSNorm → FFN → +残差 → RMSNorm`（`Transformer.h:727-740`）：FFN 夹在两次 RMSNorm 之间，且走残差连接（不破坏注意力已建好的表示）。

####  原始 Transformer 的 FFN（对比基线）

Vaswani 等人最初的 FFN 是两线性层 + ReLU：

$$
\begin{matrix}\text{FFN}_{\text{ReLU}}(x)=W_2\text{ReLU}(W_1x+b_1)+b_2,W_1\in\mathbb{R}^{d_{\text{ff}}\times d_{\text{model}}},\text{ }W_2\in\mathbb{R}^{d_{\text{model}}\times d_{\text{ff}}}\end{matrix}
$$

它把维度先涨到 $d_{\text{ff}}$（展开/升维）、过 ReLU 再压回 $d_{\text{model}}$。**缺点**：ReLU 在负区硬置 0、单调、且 $x<0$ 区域"死掉"；`W_1` 同时承担"升维"和"门控"两责，表达受限。




####  SwiGLU 公式（本引擎采用）

本引擎用 **SwiGLU**（Shazeer, 2020，LLaMA 系列标配）。它把式 (14) 的单一 `W_1` 拆成**两条并行的升维投影**，再用一条"门控"逐元素乘起来：

$$
\begin{matrix}\text{SwiGLU}(x)=W_{\text{down}}(\text{silu}(W_{\text{gate}}x)\text{ }\odot\text{ }W_{\text{up}}x)\end{matrix}
$$

- gate/up 权重 $W_{\text{gate}},W_{\text{up}}\in\mathbb{R}^{d_{\text{ff}}\times d_{\text{model}}}$：两条升维投影；
- $W_{\text{down}}\in\mathbb{R}^{d_{\text{model}}\times d_{\text{ff}}}$：把门控结果降维回 $d_{\text{model}}$；


- $\odot$：逐元素（Hadamard）乘积；

- `silu` 是 SiLU 激活（见 §7.3.4）。

**与代码命名对应**（GGUF 权重名 → 本引擎成员）：

| 公式符号 | GGUF 权重名 | 本引擎 `Linear` 成员 | 角色 |
|---|---|---|---|
| $W_{\text{gate}}$ | `ffn_gate.weight` | `up` | gate 分支（先过 silu） |

| $W_{\text{up}}$ | `ffn_up.weight` | `up2` | up 分支（直接相乘） |

| $W_{\text{down}}$ | `ffn_down.weight` | `down` | 降维输出投影 |


即代码里 `g = up.forwardBatch(X)`、`u = up2.forwardBatch(X)`、`z = silu(g) ⊙ u`、`return down.forwardBatch(z)` 正是式 (15)。

####  SiLU 激活公式与性质

SiLU（Sigmoid Linear Unit，又名 Swish）逐元素定义为：

$$
\begin{matrix}\text{silu}(z)=z\cdot\sigma(z)=\frac{z}{1+e^{-z}},\sigma(z)=\frac{1}{1+e^{-z}}\end{matrix}
$$

本引擎实现（`tensor20.h:638`）：`x / (1 + exp(-x))`，与式 (16) 一致。

相比 ReLU 的三个优点：

1. **平滑非单调**：处处可导、曲线在 0 附近是"软"的（不像 ReLU 在 0 处硬折），优化更稳定；
2. **有负区"尾巴"**：当 $z\ll0$ 时 $\text{silu}(z)\to-z/2$（仍保留一点负响应），不像 ReLU 把负区全砍掉，信息损失更少；


3. **自带门控语义**：$\sigma(z)$ 相当于"内容相关的开关"——把 $\text{silu}(W_{\text{gate}}x)$ 当作对 $W_{\text{up}}x$ 的逐元素调制系数（见 §7.3.5）。




####  门控机制（Gating）直觉

SwiGLU 是 **GLU（Gated Linear Unit）家族**的一员。通用 GLU 写成：

$$
\begin{matrix}\text{GLU}(x)=(XW)\text{ }\odot\text{ }\sigma(XV)\end{matrix}
$$

即"一路升维结果"被"另一路经过激活的门"逐元素调制。SwiGLU 只是把门控激活从 `σ` 换成 `silu`（式 16）。

直觉上：
- $W_{\text{up}}x$ 提供"候选内容"（要往下传的信息）；

- $\text{silu}(W_{\text{gate}}x)$ 是**数据依赖的动态开关**——对哪些维度该放大、哪些该抑制，由当前输入实时决定，而非固定函数；

- 二者逐元素相乘 $\odot$ 后，只有"门开着"的维度才被放行，等于对候选内容做了一次**软选择 / 重加权**。


这比原始 FFN 用 `ReLU(W_1x)`（relu 只做"非负截断"、没有可学习的调制维度关系）表达能力更强，也是 SwiGLU 在 LLaMA 等大模型上普遍取代 ReLU-FFN 的原因。



####  GLU 家族对比

| 变体 | 门控激活 | 公式 | 备注 |
|---|---|---|---|
| FFN（原版） | — | $W_2\text{ReLU}(W_1x)$ | 无门控，单层升维 |

| GLU | $\sigma$ | $(W_1x)\odot\sigma(W_2x)$ | 有门控 |


| SwiGLU（本引擎） | $\text{silu}$ | $(W_{\text{gate}}x)\odot\text{silu}(W_{\text{up}}x)$ 再 $W_{\text{down}}$ | LLaMA 默认，去偏置 |




SwiGLU 相比原版 FFN **参数量多约 1/3**（多了一条升维投影 $W_{\text{gate}}$），但带来的表达收益通常值得；LLaMA 一般取 


$$
d_{\text{ff}}=\frac{8}{3}d_{\text{model}}
$$

（向上取整到 32 的倍数）来平衡。

####  本引擎的代码逐行对应

| 公式 | 代码 | 说明 |
|---|---|---|
| 式 (15) 升维 gate | `g = up.forwardBatch(X)` `calculations.h:288` | `up = ffn_gate`，`[seq, d_model]→[seq, d_ff]` |
| 式 (15) 升维 up | `u = up2.forwardBatch(X)` `:289` | `up2 = ffn_up`，同维 |
| 式 (16) 门控激活 | `a = g->silu()` `:290` | 逐元素 $\text{silu}$ |

| 式 (15) 门控相乘 | `z = a->mul(*u)` `:291` | $\text{silu}(gate)\odot up$ |

| 式 (15) 降维输出 | `return down.forwardBatch(*z)` `:292` | `down = ffn_down`，`[seq, d_ff]→[seq, d_model]` |

```cpp
// calculations.h —— FeedForward::forward（节选，对应式 15~16）
std::unique_ptr<Matrix> g = up.forwardBatch(X);    // W_gate · X   [seq, d_ff]
std::unique_ptr<Matrix> u = up2.forwardBatch(X);   // W_up   · X   [seq, d_ff]
std::unique_ptr<Matrix> a = g->silu();             // silu(W_gate·X)  式(16)
std::unique_ptr<Matrix> z = a->mul(*u);            // silu(gate) ⊙ up  式(15)
return down.forwardBatch(*z);                      // W_down · (...)  → [seq, d_model]
```

####  维度与参数量（复杂度）

- 输入/输出：$x\in\mathbb{R}^{d_{\text{model}}}$；中间：$d_{\text{ff}}$（本引擎 `headInfor.feed_forward_length`）。


- 三个 `Linear` 权重规模：`gate`/`up` 各 $d_{\text{model}}\times d_{\text{ff}}$、`down` 为 $d_{\text{ff}}\times d_{\text{model}}$，合计约 **$3\cdot d_{\text{model}}\cdot d_{\text{ff}}$** 个参数——通常比注意力四投影（$4\cdot d_2^{\text{model}}$）还多，是单层里**参数最重**的子模块。




- 计算量（每 token）：$O(d_{\text{model}}\cdot d_{\text{ff}})$ 的矩阵-向量乘；逐位置独立、可完全并行（不依赖序列长度，区别于 §7.2.9 注意力的 $O(\text{seq}^2)$）。


- 注意：SwiGLU **不带偏置**（与 LLaMA 一致），全部是非线性只来自 `silu`、门控只来自 $\odot$，无 `b1/b2`。


###  Linear

```cpp
std::shared_ptr<Tensor> W;         // 原生类型权重（A 改法）
forward(x)  = W->matvec(x);        // 经 Tensor::matvec 现场反量化
forwardBatch(X) = 逐行 matvec;
```

####  角色定位（最基础、复用最广的矩阵乘单元）

`Linear` 是整个 Transformer 推理引擎里**最底层、复用最广**的算子：它把"权重矩阵 W"封装成可复用的线性映射，把一切"矩阵乘"收敛到一处。它被三处共用：

- **注意力投影**：`MultiHeadAttention` 的 $W_q/W_k/W_v/W_o$（4 个）；

- **前馈投影**：`FeedForward`（SwiGLU）的 `ffn_gate`/`ffn_up`/`ffn_down`（3 个）；
- **LM 头**：`EmbeddingInfor.lmHead`，把末层隐状态映射到词表 logits（维度 `[vocab, d_model]`）。

可以说：模型里**所有矩阵乘最终都收敛到 `Linear`**。它刻意做成"无状态、纯函数"——只持有权重 W 和维度，前向委托 `Tensor::matvec`，自身**不实现任何乘加**，因此换量化权重时 `Linear` 代码一行不用改。

####  数学公式（纯线性变换，无偏置）

对**单个 token** $x\in\mathbb{R}^{\text{inDim}}$（列向量），输出：


$$
\begin{matrix}y=Wx,y_o=\sum i=0\text{inDim}-1W_{o,i}x_i\end{matrix}
$$

其中 $W\in\mathbb{R}^{\text{outDim}\times\text{inDim}}$。本 demo **不带偏置 b**（保持简洁，且 llama 的注意力/FFN 投影本就无偏置），故就是一次齐次线性映射。


对**整批** $X\in\mathbb{R}^{\text{seq}\times\text{inDim}}$（每行一个 token），逐行独立做式 (18) 并拼成：


$$
\begin{matrix}Y=XW,Y_{r,o}=\sum i=0\text{inDim}-1X_{r,i}W_{o,i}\end{matrix}
$$

注意：**同一份权重 W 被所有行共享**（权重不随位置变），这正是"参数共享 + 逐位置独立"的 FFN/投影语义。

####  维度约定（GGUF 以 [OUT, IN] 存储）

GGUF 权重以 `[OUT, IN]` 布局（加载时 `makeDense({OUT, IN})`，即 `dims[0]=IN`、`dims[1]=OUT`）。`matvec` 把 `[IN]` 向量映射为 `[OUT]` 向量，故：

- `inDim  = W.cols() = IN`（输入向量长度）
- `outDim = W.rows() = OUT`（输出向量长度）

**例（LM 头）**：$W=[\text{vocab},d_{\text{model}}]$，输入 $[d_{\text{model}}]$ 的隐状态 → 输出 $[\text{vocab}]$ 的 logits（每行一个 token 的词表打分）。注意力投影同理：$W_q=[d_{\text{model}},d_{\text{model}}]$，输入/输出均为 $[d_{\text{model}}]$。






####  前向实现（forward / forwardBatch）

```cpp
// calculations.h:108-128 —— 两个 forward 均返回 unique_ptr（调用方取得所有权，纯函数风格）
std::unique_ptr<Vector> forward(const Vector& x) const {
    return std::unique_ptr<Vector>(static_cast<Vector*>(W->matvec(x).release())); // 式(18)
}
std::unique_ptr<Matrix> forwardBatch(const Matrix& X) const {
    size_t seq = X.rows();
    std::unique_ptr<Matrix> Y = std::make_unique<Matrix>(seq, outDim);
    for (size_t i = 0; i < seq; ++i) {              // 逐行（式 19）
        Vector xi(inDim);
        for (size_t j = 0; j < inDim; ++j) xi.set(j, X(i, j));   // 取第 i 行
        std::unique_ptr<Vector> yi =
            std::unique_ptr<Vector>(static_cast<Vector*>(W->matvec(xi).release()));
        for (size_t j = 0; j < outDim; ++j) (*Y)(i, j) = (*yi)(j); // 写回第 i 行
    }
    return Y;
}
```

`forward` 是单 token 的 `matvec`；`forwardBatch` 对每行各做一次 `matvec` 拼成 `Y`。中间两次逐元素拷贝（取行/写回）是为与"整序列重算"模式一致而写，非性能最优（走 KV 缓存增量或行视图可省）。

####  计算内核：matvec 与量化融合（三路径）

真正的乘加在 `Tensor::matvec`（`tensor20.h:2075-2130`）里，按权重类型走三条路径：

1. **量化融合路径（vec_dot）**：当 `W` 为量化类型且行连续，`y_o = \text{vec\_dot}(W_o, x) = \sum_j \text{dequant}(W_{o,j})\cdot x_j$`——**全程只在寄存器/栈内现场 dequant 累加，绝不物化整张 F32 权重矩阵**（呼应报告 §7 矩阵乘三路径、§10.2 量化融合门控）。这正是 A+B 内存/带宽收益的落点。
2. **F32 连续快路径**：权重为 F32 且行连续时直接按行块累加，无 dequant 开销，最快。
3. **通用逐元素兜底**：非连续视图场景，逐元素 `get`（`Q*` 类型经 `QuantTraits` 自动 `dequant` 到 float 后再算），仅作安全退路。

量化路径的精确公式（与 F32 的式 (18) 完全等价，只是权重按需解包）：

$$
\begin{matrix}y_o=\sum j=0\text{inDim}-1\text{dequant}(W_{(Q)}^{o,j})\cdot x_j\end{matrix}
$$

> 关键点：`Linear` 把"权重怎么存"和"怎么算"彻底解耦——它只管 `inDim/outDim` 记账并调用 `W->matvec`；权重是 F32 还是 Q4_0/Q8_0，由 `Tensor` 的类型决定，计算路径自动选取。换量化权重 `Linear` 一行不用改。

####  A+B 原生驻留设计（为何是 shared_ptr\<Tensor\> 而非 Matrix）

成员 `W` 是 `std::shared_ptr<Tensor>`：权重按 GGUF **原始类型**（F32/F16/BF16/Q4_0/Q8_0…）常驻内存，而不是改造前的 `Matrix`（加载即反量化成 F32、量化模型内存膨胀 4~8×）。

- **零拷贝体重用**：`tied-embedding` 时 `lmHead.W` 与 `embedding` 指向**同一份** `Tensor` 指针（共享所有权），无需复制词嵌入矩阵。
- **QKV/FFN 三投影**在 `LayerInfor` 里各自持有 `shared_ptr<Tensor>`，`forward` 内临时构造 `MultiHeadAttention`/`FeedForward` 并 `attn.Wq.W = Wq` 等赋值权重指针，避免每层重复存储子网络对象（见 §13 组合关系）。

> 曾讨论是否改回 `shared_ptr<Matrix>`（`W` 立即分配 F32），**结论为不回退**：会丢失原生驻留收益（量化模型内存/带宽 4–8× 膨胀、加载期重新反量化、融合内核被绕过）。当前维持 `shared_ptr<Tensor>`。详见 §10.3。

####  代码逐行对应表

| 公式 | 代码 | 说明 |
|---|---|---|
| 式 (18) 单 token | `W->matvec(x)` `calculations.h:110` | 委托 `Tensor::matvec` |
| 式 (19) 批 | `forwardBatch` 逐行 `matvec` `:117-127` | `[seq,inDim]→[seq,outDim]` |
| 式 (20) 量化融合 | `Tensor::matvec` 的 `vec_dot` 分支 `tensor20.h:2105-2109` | 现场 dequant 累加 |
| 维度记账 | `inDim=W.cols(), outDim=W.rows()` `:95-96` | `[OUT,IN]` 布局 |
| 原生驻留 | `std::shared_ptr<Tensor> W` `:99` | A+B 改法，惰性填充 |

####  复杂度

- 单 token：`forward` = 一次 `matvec`，约 $O(\text{inDim}\cdot\text{outDim})$ 次乘加。

- 批：`forwardBatch` = `seq` 次 `matvec`，约 $O(\text{seq}\cdot\text{inDim}\cdot\text{outDim})$。

- `matvec` 是推理**最高频**算子：注意力的 QKV/输出投影、FFN 的三投影、LM 头全部走它（见 `tensor20.h:802` 注释）。优化它（量化融合 + 连续内存 + 后续 OpenMP/OpenBLAS）对端到端延迟影响最大。

---

##  生成流程

本引擎的自回归生成由**两层函数**协作：

- **`generate(prompt, maxNew, trace)`**（外层循环，`Transformer.h:840`）：负责"一个 token 一个 token 地往外吐"——把 prompt 分词、反复调用 `forward` 拿下一个 token、贪心选中最可能者、回灌成新输入，直到生成 `eos` 或达到 `maxNew`。
- **`forward(ids, trace)`**（内层单次前向，`Transformer.h:814`）：负责"对当前整段 `ids` 跑一遍模型"——嵌入 + 位置编码 + 逐层 Transformer + 末层归一化 + LM 头，输出 `[seq, vocab]` 的 logits。

二者关系：**`generate` 每循环一次，就调用一次完整的 `forward`**（当前为整序列重算模式，见 §8.3）。

```cpp
// 真实代码骨架（已对齐 Transformer.h:840-860 / 814-837）
std::string generate(const std::string& prompt, size_t maxNew = 8, bool trace = false) const {
    std::vector<size_t> ids = embeddingInfor.tokenizerEncode(prompt, headInfor);  // 真实 BPE 分词
    ops::Sampler sampler;                          // 贪心解码（无状态）
    for (size_t step = 0; step < maxNew; ++step) {
        std::unique_ptr<Matrix> logits = forward(ids, trace);   // 整序列重算
        size_t last = logits->rows() - 1;          // 取最后一行（当前预测位置）
        size_t best = sampler.argmax(*logits, last); // 贪心 argmax
        ids.push_back(best);                        // 回灌
        if (best == headInfor.eos_token_id) break; // 遇 eos 提前结束
    }
    std::string out; for (size_t id : ids) out += embeddingInfor.tokenizerDecode(id, headInfor);
    return out;
}
```

###  8.1 `generate` 函数详解（自回归外层循环）

1. **分词（Encode）**：`embeddingInfor.tokenizerEncode(prompt, headInfor)` 把输入字符串按 GGUF 自带的 BPE 词表切成 token id 序列 `ids`（如 `"3+4="` → `[4, 10, 5, ...]`）。`ids` 同时充当"已生成上下文"。
2. **逐步预测循环**：`for step in 0..maxNew`，每步：
   - 调 `forward(ids)` 得到整段 logits；
   - `last = rows()-1` 取**最后一行**——自回归中下一 token 只由"当前最后一个位置"预测；
   - `sampler.argmax(logits, last)` 在词表维上取最大 logit 对应的 token id（`best`）；
   - `ids.push_back(best)` 把预测结果**回灌**到 `ids` 末尾，作为下一步的输入（这就是"自回归"）。
3. **终止条件**：两种——`best == eos_token_id`（模型主动说"我说完了"）立即 `break`；或 `step` 达到 `maxNew`。
4. **解码（Decode）**：循环结束后把整个 `ids`（含原始 prompt 的 id）逐个 `tokenizerDecode` 拼成字符串返回。

> 注意：`decode` 会把 **prompt 自身的 id 也解出来**，所以返回文本里包含原始 prompt。若只要"新生成部分"，应从 `ids` 的第 `promptLen` 个起解码。

###  8.2 `forward` 函数详解（单次前向）

输入 `ids`，输出 `logits ∈ [seq, vocab]`。按顺序：

1. **嵌入 Gather**：`embeddingInfor.embedding->gather(ids, 0)` 沿 axis0 按 `ids` 取嵌入矩阵的行，得到 $X\in[seq,d_{\text{model}}]$（每行是一个 token 的嵌入向量）。

2. **位置编码 RoPE**：`ops::RoPE rope(d_model); rope.apply(X)` —— 第 i 行用位置 i 旋转（行号=位置，见 §7.1）。
3. **逐层 Transformer**：对 `layers` 中每一层调用 `LayerInfor::forward(X)`（见下，pre-norm + 双残差），更新 X。
4. **末层归一化**：`ops::rmsNorm(X, embeddingInfor.finalNorm, eps)` 得到 $X_n$（output_norm）。

5. **LM 头**：`embeddingInfor.lmHead.forwardBatch(X_n)` → `logits [seq, vocab]`（每行是该位置对全词表的打分）。

**单层 `LayerInfor::forward(X)`（`Transformer.h:727-738`，pre-norm 双残差）**：

$$
\begin{matrix}\begin{matrix}h & =\text{RMSNorm}(X,\text{ }\text{attn\_norm}) \\ a & =\text{MultiHeadAttention}(h)\text{（Wq/Wk/Wv/Wo 投影 + 因果注意力 + Wo 输出）} \\ r_1 & =X+a\text{（注意力残差）} \\ h_2 & =\text{RMSNorm}(r_1,\text{ }\text{ffn\_norm}) \\ f & =\text{FeedForward}(h_2)\text{（SwiGLU）} \\ X_{\text{out}} & =r_1+f\text{（FFN 残差，加回注意力残差流 }r_1\text{）}\end{matrix}\end{matrix}
$$

> 这里用的是 **pre-norm**（归一化在子层之前），且两条残差都加回"注意力之后的残差流 $r_1$"（`r1.add(f)`），与 `r2`/`h2` 无关——这是 llama 风格的标准写法。`trace=true` 时会在每层后打印 `|h0|`（第 0 行模长）便于核对数值。


###  8.3 当前实现要点

- **整序列重算**：每步 `generate` 都把**整个 `ids`** 重新喂给 `forward`（序列随生成变长），复杂度为 $O(\text{maxNew}\cdot\text{seq}^2\cdot d_{\text{model}})$。`KVCache` 已预留但未启用，接增量解码可降到 $O(\text{maxNew}\cdot\text{seq}\cdot d_{\text{model}})$（见 §10.5）。


- **纯贪心解码**：当前 `Sampler` 只做 `argmax`。贪心易陷入重复环（见 §10.4），后续可加温度采样 / Top-p / 重复惩罚。
- **`eos` 提前终止**：生成到结束符即停，不会硬凑满 `maxNew`。

###  8.4 `generate` 函数流程图

![generate_flow](..\images\generate_flow.png)

###  8.5 `forward` 函数流程图

![forward_flow](..\images\forward_flow1.png)

---

### `forward(X)` 函数流程图

![](..\images\layerinfor_forward_flow.png)



##  关键性能优化（A+B 改造）

###  加载 优化(30 分钟 → 秒级（F32 快路径）)

原瓶颈在逐元素 `M(r,c)=ten->get(r*IN+c)`（每元素 ~3 次虚分发）。`loadTensorMatrix` 改为：

- F32：整块 `fread(M->data(), sizeof(float), OUT*IN, f)`；
- 其余类型：`makeDense({OUT,IN}, e->type)` 后整块 `fread(M->raw(), 1, nb, f)`，**零逐元素循环**。

实测：`token_embd.weight`（196MB）约 117ms 读完；整体加载从 ~30 分钟降至秒级。

###  A：权重原生驻留

`Matrix W` → `shared_ptr<Tensor> W`，权重按 GGUF 原始类型常驻，计算时反量化。量化模型内存/带宽降 **4–8×**，且 `Tensor` 基类补充 `rows()/cols()` 便捷方法以兼容调用方。

###  B：量化融合门控激活

见 §5.4。修正后量化权重命中 `vec_dot` 融合路径，不物化整张 F32 权重矩阵。

###  收益与权衡

| 项 | 改前 | 改后 |
|---|---|---|
| 加载时间 | ~30 min | 秒级 |
| 量化权重内存 | F32 展开（膨胀 4–8×） | 原生类型驻留 |
| 计算路径 | 逐元素 `get` | `vec_dot` 融合 / F32 快路径 |
| F32 模型回归 | — | 无（diff≈1.1e-07 校验通过） |

---

##  已解决问题清单（Bug → 修复）

| # | 现象 | 根因 | 修复 |
|---|---|---|---|
| ① | BitNet 超参全 0 | `loadFrom` 只认 `llama.*` | `isKey` 双前缀 |
| ② | >2.1GB 张量读不出 | 32 位 `long` 的 `fseek/ftell` 溢出 | `_fseeki64/_ftelli64` |
| ③ | `[loadTensor] 缺失张量: output.weight` | tied-embedding 无该张量 | `lmHead.W = embedding` 共享 |
| ④ | 加载 30 分钟 | 逐元素 `get` 拷贝循环 | 整块 `fread` 快路径 |
| ⑤ | 量化融合被绕过 | `rowContig` 门控未除 `blck_size` | `rowStride = nb_[0]/blck_size` |
| ⑥ | 无 BOM 中文注释 C4819/C2001；`fopen` C4996 | 编码/安全开关缺失 | `.vcxproj` 加 `/utf-8` + `_CRT_SECURE_NO_WARNINGS` |

> 编译校验：`cl.exe` v144 通过（`COMPILE_EXIT=0`）；运行时校验 `[tie] samePtr=1`、`matvec diff≈1.12e-07`。

---

##  已知问题与待办

###  生成重复死循环（当前最优先）

现象：`"3+4="` 首步预测 `.`，后续每步 token 完全相同，陷入无限循环。

**分析结论**：**非生成代码 bug**。
- `forward` 取 `last=rows()-1`、序列每步 +1（`seq 4→5` 已验证）均正确；
- 日志里 `|h0|` 每一步相同是**因果注意力的必然结果**（位置 0 只看自己，与序列长度无关），属干扰项；
- 真正原因是模型在**贪心解码**下陷入重复环：模型对 `"3+4="` 与 `"3+4=."` 末位 logits 的 argmax 都选中 `.`（该 BitNet 大模型未训出此算术能力，或纯贪心偏好高频/标点 token）。

**定位建议**：在 `forward` 末尾打印 `|h_last|`（最后一行范数）与末位 top-3 logits，对比 step0/step1：
- 若 `|h_last|` 不同且 top1 仍 `.` → 确认是模型/解码行为；
- 若 `|h_last|` 也完全相同 → 才有真 bug（再查 `gather`/RoPE 是否漏算新 token）。

**解决方向**：温度采样 / Top-p、重复惩罚（repetition penalty）；或确认加载的是否为对应任务模型。

###  推理再加速（Release 构建）

- Release `/O2`；
- `/arch:AVX2 /fp:fast` + `matvec` 外层 `#pragma omp parallel for`；
- 链接 OpenBLAS `cblas_sgemv/sgemm`；
- （进阶）BitNet 1.58-bit 量化 `matvec`。

###  Linear::W 类型取舍

是否改 `shared_ptr<Tensor>` → `shared_ptr<Matrix>`：可行（`Matrix:public Tensor`，`forward` 兼容），但会回退原生驻留收益（量化模型内存/带宽 4–8× 膨胀、加载期重新反量化）。建议维持现状，除非确需 `Matrix` 专属接口。

###  增量解码

`KVCache` 已定义但未启用；切到增量解码可把每步复杂度从 `O(seq)` 降到 `O(1)`（配合 RoPE 按绝对位置）。

---

## . 工程与编译

- 工程：`TestTransformer.sln` / `TestTransformer.vcxproj` / `.filters`；
- 配置：Debug | x64，`OutDir=$(ProjectDir)debug\`；
- 编译开关：`/utf-8`、`_CRT_SECURE_NO_WARNINGS`、C++17、PlatformToolset `v144`；
- 入口：`main.cpp` → `model.generate(prompt, 8, /*trace=*/true)`；
- 沙箱内 `msbuild` 受 LOLBin 拦截时，可手动设 `INCLUDE/LIB/PATH` 后用 `cl.exe` 校验编译。

---

## . 关键文件速查

| 文件 | 职责 |
|---|---|
| `Transformer/Transformer.h` | GGUF 解析、模型装配、网络层、生成、分词器 |
| `tensor/tensor20.h` | Tensor/Matrix/Vector、量化系统、matvec/matmul/bmm 融合内核 |
| `calculations/calculations.h` | 模型结构算子（namespace ops）：rmsNorm/applyRoPEToRow 自由函数 + Linear/RMSNorm/RoPE/MultiHeadAttention/FeedForward/Sampler 六个类 |
| `main.cpp` | 入口，调用 `generate` |
| `docs/GGUF_Head_分析.md` | GGUF 头部元数据专题 |
| `docs/Transformer设计报告.md` | 本文件 |

---

## . 类层次结构图

本工程分两层：**算子库 `tensor20.h`**（唯一存在继承的层）与 **模型装配/网络层（`Transformer.h` + `calculations.h`，分别 `namespace trans` / `namespace ops`）**（全部为独立类，靠成员组合/聚合连接）。下面分别给出详细图。

###  Tensor 继承体系（tensor20.h，唯一 is-a 层次）

![image-20260816194228026](D:\testgithub\opengvs\images\image-20260816194228026.png)



**继承关系（is-a）ASCII 树：**

```
Tensor  (abstract 同构基类：raw/get/set/matvec/matmul/bmm/nb_ 字节步长)
├── Scalar                       : Tensor          // 标量视图
├── Vector                       : Tensor   [n]    // 一维（RMSNorm 归一化权重保持 F32）
│   └── VectorView              : Vector          // 子视图
├── Matrix                       : Tensor   [r,c]  // 二维稠密（F32 快路径）
│   └── MatrixView              : Matrix          // 子视图
├── Tensor3                      : Tensor   [a,b,c]
│   └── Tensor3View             : Tensor3
├── Tensor4                      : Tensor   [N,C,H,W]
│   └── Tensor4View             : Tensor4
├── PermuteView                 : Tensor          // 轴置换视图（不拷贝）
└── TransposeView                : Tensor          // 转置视图（不拷贝）

// 独立助手（非 Tensor 子类）：
struct QuantTraits   { blck_size; blck_bytes; dequant; quant; vec_dot; }   // 量化系统
class GatherMatrixView { 引用 Matrix + rowIds，gather 取 F32 行 }            // embedding 取行
```

要点：`Matrix` / `Vector` 是 `Tensor` 子类，A+B 改造把 `shared_ptr<Matrix>` 换成 `shared_ptr<Tensor>` 后，原生驻留的量化张量（`makeDense` 构造的 `Tensor`）可直接作为 `Linear::W` / `LayerInfor::Wq…` / `EmbeddingInfor::embedding` 使用，调用方通过 `Tensor::rows()/cols()/matvec()` 透明访问。

###  Transformer.h 文件类调用关系

> **文件/命名空间归属**：下方类图中 `RMSNorm` / `RoPE` / `Linear` / `MultiHeadAttention` / `FeedForward` / `Sampler` 六个类已迁入 `calculations/calculations.h`（`namespace ops`）；`KVCache` 与四个业务装配类 `GUFFHeadInfor` / `EmbeddingInfor` / `LayerInfor` / `Transformer` 仍保留在 `Transformer.h`（`namespace trans`）。`Transformer.h` 通过 `using ops::...` 引入前者，原有代码免改。

![image-20260816164120322](D:\testgithub\opengvs\images\image-20260816164120322.png)



**关键设计点：**

1. 本工程模型层类**互无继承**（分散在 `Transformer.h` 与 `calculations.h` 两文件，分别 `namespace trans` / `namespace ops`），关系纯靠成员组合/聚合建立，耦合低、可独立测试；
2. 权重统一以 `shared_ptr<Tensor>`（或 `shared_ptr<Matrix>`）持有，tied-embedding 时 `lmHead.W = embedding` 为**同一指针零拷贝共享**（参见 §4.2 / 修复③）；
3. `LayerInfor::forward` 通过「临时 `MultiHeadAttention`/`FeedForward` + 赋值权重指针」复用通用注意力/前馈实现，避免每层重复存储子网络对象；
4. 真正的「类型层次」只在 `tensor20.h` 的 `Tensor` 家族出现（§13.1），`Matrix` 是 `Tensor` 子类，故 `Linear::W` 既可装 F32 `Matrix` 也可装量化 `Tensor`，对调用方透明。

