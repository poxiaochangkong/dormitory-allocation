# 宿舍匹配算法设计文档

> **版本**: v2.0  
> **状态**: 设计阶段  
> **最后更新**: 2026-06-03  
> **文件位置**: `docs/algorithm.md`

---

## 目录

1. [问题定义](#1-问题定义)
2. [算法架构总览](#2-算法架构总览)
3. [评分体系](#3-评分体系)
4. [第一阶段：配对形成](#4-第一阶段配对形成pair-formation)
5. [第二阶段：配对合并成房间](#5-第二阶段配对合并成房间pair-merging)
6. [第三阶段：模拟退火精炼](#6-第三阶段模拟退火精炼sa-refinement)
7. [边界情况处理](#7-边界情况处理)
8. [复杂度分析](#8-复杂度分析)
9. [参数配置](#9-参数配置)
10. [与现有代码的集成](#10-与现有代码的集成)

---

## 1. 问题定义

### 1.1 形式化建模

给定：
- 学生集合 `S = {s₁, s₂, ..., sₙ}`，每个学生有一个性别 `g(sᵢ)`
- 宿舍集合 `D = {d₁, d₂, ..., dₘ}`，每个宿舍容量为 `cap(dⱼ)`（通常为 4）
- 兼容性评分函数 `score(sᵢ, sⱼ) ∈ [-1, 1]`
- 否决冲突检测函数 `veto(sᵢ, sⱼ) ∈ {true, false}`

目标：将学生分配到宿舍中，最大化全局兼容性分数：

$$
\max \sum_{d \in D} \sum_{\substack{s_i, s_j \in d \\ i < j}} \text{score}(s_i, s_j)
$$

约束条件：
1. **容量约束**：每个宿舍 `d` 中学生数 ≤ `cap(d)`
2. **性别约束**：同一宿舍内所有学生性别相同
3. **否决约束**：同一宿舍内无否决冲突（`veto(sᵢ, sⱼ) = false` 对所有 `i ≠ j`）
4. **完整性约束**：每个学生恰好分配到一个宿舍

### 1.2 问题复杂度

这是一个 **带约束的最大权重 k-聚类问题**（k = 宿舍容量），属于 **NP-hard**。因此采用启发式算法求近似最优解。

---

## 2. 算法架构总览

```
┌─────────────────────────────────────────────┐
│              输入：学生数据 + 宿舍数据          │
└──────────────────┬──────────────────────────┘
                   │
                   ▼
┌─────────────────────────────────────────────┐
│  预计算：O(n²) 计算所有学生对的 pair_score      │
│  （含相似度、互补度、否决检测）                   │
└──────────────────┬──────────────────────────┘
                   │
                   ▼
┌─────────────────────────────────────────────┐
│  第一阶段：配对形成（Pair Formation）           │
│  贪心近似最大权重匹配 → ⌊n/2⌋ 个配对            │
└──────────────────┬──────────────────────────┘
                   │
                   ▼
┌─────────────────────────────────────────────┐
│  第二阶段：配对合并（Pair Merging）             │
│  两个配对合并为一个 4 人房间                     │
└──────────────────┬──────────────────────────┘
                   │
                   ▼
┌─────────────────────────────────────────────┐
│  第三阶段：模拟退火精炼（SA Refinement）        │
│  随机交换 + 概率性接受劣解 → 跳出局部最优        │
└──────────────────┬──────────────────────────┘
                   │
                   ▼
┌─────────────────────────────────────────────┐
│              输出：最优宿舍分配方案              │
└─────────────────────────────────────────────┘
```

**设计理由**：宿舍匹配具有天然的两层结构 —— **个人 → 室友对 → 房间**。分阶段求解可以降低每步的搜索空间，同时保证解的质量。

---

## 3. 评分体系

### 3.1 总分公式

对于学生 `a` 和 `b`，兼容性分数定义如下：

```
if HasVetoConflict(a, b):
    pair_score(a, b) = -1.0    // 一票否决
else:
    pair_score(a, b) = w_sim × similarity(a, b)
                      + w_comp × complementarity(a, b)
                      + w_veto × veto_safety(a, b)
```

其中权重使用**双方权重的平均值**（对称化）：

```
w_sim  = (a.similarity_weight      + b.similarity_weight)      / 2.0
w_comp = (a.complementarity_weight + b.complementarity_weight) / 2.0
w_veto = (a.veto_safety_weight     + b.veto_safety_weight)     / 2.0
```

### 3.2 相似度计算（Similarity）—— 高斯核欧氏距离

使用 **6 个维度**的归一化欧氏距离，经高斯核变换：

| 维度 | 数据类型 | 范围 | 说明 |
|------|----------|------|------|
| `sleep_schedule` | 字符串→数值 | 1-4 | early=1, normal=2, late=3, very_late=4 |
| `hygiene_level` | 整数 | 1-5 | 卫生习惯等级 |
| `noise_tolerance` | 整数 | 1-5 | 噪音容忍度 |
| `temperature_preference` | 整数 | 18-28 | 偏好温度（°C） |
| `social_preference` | 字符串→数值 | 1-3 | introvert=1, ambivert=2, extrovert=3 |
| `gaming_behavior` | 字符串→数值 | 1-4 | never=1, sometimes=2, often=3, always=4 |

**归一化**：每个维度除以其最大范围，映射到 `[0, 1]`。

**高斯核变换**：

$$
\text{similarity}(a, b) = \exp\left(-\frac{\|\mathbf{a} - \mathbf{b}\|^2}{2\sigma^2}\right)
$$

其中：
- `‖a - b‖²` 是归一化后的欧氏距离平方
- `σ = 0.3`（控制容忍度参数）

**高斯核 vs 线性衰减**的对比：

| 距离 | 线性 `1 - d` | 高斯核 `exp(-d²/0.18)` |
|------|-------------|----------------------|
| 0.0  | 1.000       | 1.000                |
| 0.1  | 0.900       | 0.961                |
| 0.3  | 0.700       | 0.741                |
| 0.5  | 0.500       | 0.439                |
| 1.0  | 0.000       | 0.061                |

高斯核更符合人际兼容性感知：小差异几乎不扣分，大差异严重扣分。

### 3.3 互补度计算（Complementarity）—— MBTI 认知功能模型

不使用简单的字母匹配，而是基于 **MBTI 认知功能栈** 计算互补性。

**8 种认知功能**：

| 功能 | 代号 | 类型 |
|------|------|------|
| 内倾直觉 | Ni | 感知 |
| 外倾直觉 | Ne | 感知 |
| 内倾思考 | Ti | 判断 |
| 外倾思考 | Te | 判断 |
| 内倾感觉 | Si | 感知 |
| 外倾感觉 | Se | 感知 |
| 内倾情感 | Fi | 判断 |
| 外倾情感 | Fe | 判断 |

**每种 MBTI 类型对应的功能栈**（前 4 位）：

```
ISTJ: Si Te Fi Ne    ISFJ: Si Fe Ti Ne
INFJ: Ni Fe Ti Se    INTJ: Ni Te Fi Se
ISTP: Ti Se Ni Fe    ISFP: Fi Se Ni Te
INFP: Fi Ne Si Te    INTP: Ti Ne Si Fe
ESTP: Se Ti Fe Ni    ESFP: Se Fi Te Ni
ENFP: Ne Fi Te Si    ENTP: Ne Ti Fe Si
ESTJ: Te Si Ne Fi    ESFJ: Fe Si Ne Ti
ENFJ: Fe Ni Se Ti    ENTJ: Te Ni Se Fi
```

**互补规则**：

```
1. 感知功能互补：Ni↔Ne, Si↔Se（维度内互补，得分 0.5）
2. 判断功能互补：Ti↔Te, Fi↔Fe（维度内互补，得分 0.5）
3. 同功能冲突：完全相同功能（如 Ni-Ni），得分 0.0
4. 部分互补：不同维度（如 Ni-Ti），得分 0.2
```

**计算方式**：

```
comp_score = 0
for func_a in top_4_functions(a):
    for func_b in top_4_functions(b):
        comp_score += complementarity(func_a, func_b)
comp_score = normalize(comp_score)  // 归一化到 [0, 1]
```

### 3.4 否决安全度（Veto Safety）

```
if HasVetoConflict(a, b):  return -1.0  // 一票否决
else:                       return  1.0
```

**否决冲突检测**（`HasVetoConflict`）：

从 `veto_items` 和 `raw_answers` 推导的行为标签中检测：

| 标签 | 推导来源 | 冲突规则 |
|------|----------|----------|
| `smoke_alcohol` | `raw_answers` 中的吸烟/饮酒题 | 吸烟者 vs 不吸烟者 |
| `pets` | `raw_answers` 中的宠物题 | 想养宠物 vs 过敏/不想 |
| `opposite_sex` | `raw_answers` 中的异性接触题 | 有异性来访 vs 反对 |
| `midnight_gaming` | `gaming_behavior == "always"` + `sleep_schedule == "very_late"` | 深夜打游戏 vs 早睡 |
| `loud_speaker` | `noise_tolerance ≤ 2` | 外放 vs 安静需求 |
| `bad_hygiene` | `hygiene_level ≤ 2` | 卫生差 vs 卫生要求高 |
| `over_demand` | 社交偏好极端 | 强势 vs 需要空间 |
| `boundary_violation` | 综合判断 | 边界感差 vs 需要隐私 |

### 3.5 房间级评分

对于已形成的房间 `r = {s₁, s₂, s₃, s₄}`，房间级评分为：

$$
\text{room\_score}(r) = 0.7 \times \text{avg\_pair} + 0.3 \times \text{min\_pair}
$$

其中：
- `avg_pair` = 房间内所有 C(4,2) = 6 对学生 `pair_score` 的**平均值**
- `min_pair` = 房间内 6 对学生 `pair_score` 的**最小值**

`min_pair` 权重确保不会出现"一对差配拖垮全房间"的情况。

**全局目标函数**：

$$
\text{GlobalScore} = \sum_{r \in D} \text{room\_score}(r)
$$

---

## 4. 第一阶段：配对形成（Pair Formation）

### 4.1 算法：贪心近似最大权重匹配

**输入**：
- 学生列表 `students`（已按性别分组）
- 所有学生对的 `pair_scores` map

**输出**：
- 配对列表 `pairs`：每组 `[(i, j), ...]`
- 未配对学生列表 `unpaired`

### 4.2 伪代码

```
function FormPairs(students, pair_scores):
    // 按性别分组
    male_students = [s for s in students if s.gender == "male"]
    female_students = [s for s in students if s.gender == "female"]
    
    pairs = []
    unpaired = []
    
    // 对每个性别组独立配对
    for group in [male_students, female_students]:
        // 收集组内所有有效候选对
        candidates = []
        for i, j in combinations(group, 2):
            if pair_scores[(i,j)] > 0:  // 排除否决冲突（-1）
                candidates.append((pair_scores[(i,j)], i, j))
        
        // 按 pair_score 降序排列
        sort(candidates, descending by score)
        
        // 贪心配对
        assigned = set()
        for score, i, j in candidates:
            if i not in assigned and j not in assigned:
                pairs.append((i, j))
                assigned.add(i)
                assigned.add(j)
        
        // 收集未配对学生
        for s in group:
            if s not in assigned:
                unpaired.append(s)
    
    return pairs, unpaired
```

### 4.3 为什么用贪心匹配？

- **真正的最大权重完美匹配**（Blossom 算法）时间复杂度 O(n³)，实现复杂
- 贪心近似匹配复杂度 O(n² log n)，且在实践中与最优解差距很小（通常 < 5%）
- 宿舍匹配不需要理论最优，因为后续还有 SA 精炼阶段

---

## 5. 第二阶段：配对合并成房间（Pair Merging）

### 5.1 算法：配对间兼容性匹配

**核心思想**：将两个配对（2人 + 2人）合并为一个 4 人房间。

### 5.2 伪代码

```
function MergePairsIntoRooms(pairs, unpaired, students, pair_scores):
    rooms = []
    
    // 计算两个配对之间的兼容性分数
    // pair_group_A = (a1, a2), pair_group_B = (b1, b2)
    // 兼容性 = 4 对交叉 pair_score 的平均值
    function cross_compatibility(pairA, pairB):
        a1, a2 = pairA
        b1, b2 = pairB
        scores = [
            pair_scores[(a1, b1)],
            pair_scores[(a1, b2)],
            pair_scores[(a2, b1)],
            pair_scores[(a2, b2)]
        ]
        if any(s < 0 for s in scores):  // 有否决冲突
            return -1.0
        return mean(scores)
    
    // 收集所有候选合并
    merge_candidates = []
    for i, j in combinations(range(len(pairs)), 2):
        compat = cross_compatibility(pairs[i], pairs[j])
        if compat > 0:
            merge_candidates.append((compat, i, j))
    
    // 按兼容性降序排列
    sort(merge_candidates, descending by compat)
    
    // 贪心合并
    merged = set()
    for compat, i, j in merge_candidates:
        if i not in merged and j not in merged:
            room = [pairs[i][0], pairs[i][1], pairs[j][0], pairs[j][1]]
            rooms.append(room)
            merged.add(i)
            merged.add(j)
    
    // 未合并的配对拆开，学生加入 unpaired
    remaining_unpaired = list(unpaired)
    for i in range(len(pairs)):
        if i not in merged:
            remaining_unpaired.extend(pairs[i])
    
    // 处理剩余学生：贪心填入已有房间或组建新房间
    for student in remaining_unpaired:
        placed = false
        // 优先填入有空位的已有房间
        for room in rooms:
            if len(room) < 4 and can_add(room, student):
                room.append(student)
                placed = true
                break
        // 无法填入则新建房间
        if not placed:
            rooms.append([student])
    
    return rooms

function can_add(room, student):
    // 性别一致
    if student.gender != room[0].gender:
        return false
    // 无否决冲突
    for member in room:
        if pair_scores[(member, student)] < 0:
            return false
    return true
```

---

## 6. 第三阶段：模拟退火精炼（SA Refinement）

### 6.1 算法：模拟退火全局优化

在第二阶段的解基础上，通过随机交换和概率性接受劣解来寻找全局更优的分配方案。

### 6.2 伪代码

```
function RefineBySA(rooms, students, pair_scores):
    // 参数配置
    T_init = 1.0           // 初始温度
    T_min = 0.001          // 终止温度
    alpha = 0.997          // 降温系数
    iterations_per_T = len(students)  // 每个温度的迭代次数
    
    // 计算初始全局分数
    current_score = ComputeGlobalScore(rooms, pair_scores)
    best_rooms = deep_copy(rooms)
    best_score = current_score
    
    T = T_init
    
    while T > T_min:
        for iter in range(iterations_per_T):
            // 随机选择交换操作
            op = random_choice(["swap_students", "swap_rooms"])
            
            if op == "swap_students":
                // 随机选两个房间，各选一个学生交换
                r1, r2 = random_choice(rooms, 2)
                s1 = random_choice(r1)
                s2 = random_choice(r2)
                
                // 约束检查
                if not valid_swap(r1, r2, s1, s2):
                    continue
                
                // 执行交换
                r1.remove(s1); r1.append(s2)
                r2.remove(s2); r2.append(s1)
                
                // 计算新分数
                new_score = ComputeGlobalScore(rooms, pair_scores)
                delta = new_score - current_score
                
                // Metropolis 准则
                if delta > 0 or random() < exp(delta / T):
                    current_score = new_score
                    if current_score > best_score:
                        best_score = current_score
                        best_rooms = deep_copy(rooms)
                else:
                    // 回滚交换
                    r1.remove(s2); r1.append(s1)
                    r2.remove(s1); r2.append(s2)
            
            elif op == "swap_rooms":
                // 随机选两个房间中的部分成员交换（3人↔1人, 2人↔2人等）
                // 实现 similar to swap_students
        
        // 降温
        T = T * alpha
    
    return best_rooms

function valid_swap(room1, room2, student1, student2):
    // 性别约束：交换后两个房间的性别一致性
    // 即 student1.gender == room2 中其他人的 gender
    // 且 student2.gender == room1 中其他人的 gender
    for member in room1:
        if member != student1 and member.gender != student2.gender:
            return false
    for member in room2:
        if member != student2 and member.gender != student1.gender:
            return false
    
    // 否决约束：新成员不与房间内其他人冲突
    for member in room1:
        if member != student1 and pair_scores[(member, student2)] < 0:
            return false
    for member in room2:
        if member != student2 and pair_scores[(member, student1)] < 0:
            return false
    
    return true

function ComputeGlobalScore(rooms, pair_scores):
    total = 0
    for room in rooms:
        if len(room) < 2:
            continue
        pair_scores_in_room = []
        for i, j in combinations(room, 2):
            pair_scores_in_room.append(pair_scores[(i, j)])
        avg_score = mean(pair_scores_in_room)
        min_score = min(pair_scores_in_room)
        room_score = 0.7 * avg_score + 0.3 * min_score
        total += room_score
    return total
```

### 6.3 退火策略说明

| 参数 | 值 | 说明 |
|------|-----|------|
| `T_init` | 1.0 | 初始温度，使初始接受概率约 60-70% |
| `T_min` | 0.001 | 终止温度，此时几乎只接受更优解 |
| `α` | 0.997 | 降温系数（越接近 1 越慢，质量越高） |
| `iterations_per_T` | n | 每个温度的迭代次数 = 学生总数 |

**总迭代次数估算**：
```
降温次数 = log(T_min / T_init) / log(α) = log(0.001) / log(0.997) ≈ 2300
总迭代 ≈ 2300 × n
```

- n = 100 → 约 23 万次迭代，运行 < 1 秒
- n = 500 → 约 115 万次迭代，运行 5-15 秒
- n = 2000 → 约 460 万次迭代，运行 1-3 分钟

---

## 7. 边界情况处理

### 7.1 学生数量边界

| 情况 | 处理方式 |
|------|----------|
| n = 1 | 单人房间，跳过匹配 |
| n = 2-3 | 组成一个小房间，无需第二阶段 |
| n 为奇数 | 最后一个房间为 3 人 |
| n 不能被 4 整除 | 允许 2-3 人的小房间，优先填满 4 人房 |

### 7.2 性别不均衡

| 情况 | 处理方式 |
|------|----------|
| 某性别只有 1 人 | 单人房间 |
| 某性别 2-3 人 | 一个小房间 |
| 某性别不能被 4 整除 | 允许 1 个 2 人或 3 人房间 |

### 7.3 否决冲突严重

| 情况 | 处理方式 |
|------|----------|
| 某学生与所有同性学生都有否决冲突 | 放入"待人工分配"队列，记录原因 |
| 某房间无法找到兼容的第 4 人 | 允许 3 人房间 |
| 配对阶段大量否决冲突 | 降低配对阈值，允许"勉强兼容"的配对 |

### 7.4 数据缺失

| 情况 | 处理方式 |
|------|----------|
| 学生未提交问卷 | 使用默认值（问卷所有维度取中间值） |
| `raw_answers` 为空 | `DeriveBehaviorTags` 不推导额外标签 |
| 权重未设置 | 使用默认权重（0.5 / 0.2 / 0.3） |

---

## 8. 复杂度分析

### 8.1 时间复杂度

| 阶段 | 复杂度 | 说明 |
|------|--------|------|
| 预计算 pair_scores | O(n²) | 遍历所有学生对 |
| 第一阶段：配对形成 | O(n² log n) | 排序候选对 |
| 第二阶段：配对合并 | O((n/2)² log(n/2)) | 排序候选合并 |
| 第三阶段：模拟退火 | O(n × iter_total) | iter_total ≈ 2300n |
| **总计** | **O(n² + n × iter_total)** | |

对于 n = 500：
- 预计算：250,000 次评分计算
- SA：约 5.75 亿次迭代（含分数计算），但每次只涉及局部重算
- 优化后实际运行时间约 5-30 秒

### 8.2 空间复杂度

| 数据结构 | 大小 |
|----------|------|
| `pair_scores` map | O(n²) |
| 学生配置文件 | O(n) |
| 房间分配结果 | O(n) |
| **总计** | **O(n²)** |

对于 n = 500：约 250,000 个 pair_score，每个 double 占 8 字节 → 约 2 MB

### 8.3 局部分数优化

SA 阶段不需要每次重算全局分数。交换两个学生只影响涉及的两个房间：

```
delta = (new_room1_score + new_room2_score) - (old_room1_score + old_room2_score)
```

将每次迭代的复杂度从 O(n²) 降到 O(1)（只重算两个房间内的 6+6=12 对 pair_score）。

---

## 9. 参数配置

### 9.1 权重参数（可通过 preference 表或 rule_config 覆盖）

| 参数 | 默认值 | 范围 | 说明 |
|------|--------|------|------|
| `similarity_weight` | 0.5 | [0, 1] | 相似度权重 |
| `complementarity_weight` | 0.2 | [0, 1] | 互补度权重 |
| `veto_safety_weight` | 0.3 | [0, 1] | 否决安全度权重 |

**约束**：`w_sim + w_comp + w_veto = 1.0`

### 9.2 高斯核参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `sigma` | 0.3 | 控制容忍度，越小越严格 |

### 9.3 模拟退火参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `T_init` | 1.0 | 初始温度 |
| `T_min` | 0.001 | 终止温度 |
| `alpha` | 0.997 | 降温系数 |
| `iterations_per_T` | n | 每温度迭代次数 |

### 9.4 房间评分参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `avg_weight` | 0.7 | 房间内平均分权重 |
| `min_weight` | 0.3 | 房间内最低分权重 |

---

## 10. 与现有代码的集成

### 10.1 修改范围

| 文件 | 修改内容 |
|------|----------|
| `backend/include/algorithm/MatchEngine.h` | 新增方法声明、新增辅助结构体 |
| `backend/src/algorithm/MatchEngine.cpp` | **重写核心分配逻辑** |
| 其他后端文件 | **不改** |

### 10.2 新增方法声明（MatchEngine 类）

```cpp
// Phase 1: Pair Formation
static std::vector<std::pair<int,int>> FormPairs(
    const std::vector<int>& indices,
    const std::map<std::pair<int,int>, double>& pair_scores);

// Phase 2: Pair Merging
static std::vector<std::vector<int>> MergePairsIntoRooms(
    const std::vector<std::pair<int,int>>& pairs,
    const std::vector<int>& unpaired,
    const std::vector<StudentProfile>& students,
    const std::map<std::pair<int,int>, double>& pair_scores,
    int room_capacity);

// Phase 3: SA Refinement
static std::vector<std::vector<int>> RefineBySA(
    std::vector<std::vector<int>> rooms,
    const std::vector<StudentProfile>& students,
    const std::map<std::pair<int,int>, double>& pair_scores,
    int room_capacity);

// Utility: Compute global score
static double ComputeGlobalScore(
    const std::vector<std::vector<int>>& rooms,
    const std::map<std::pair<int,int>, double>& pair_scores);

// Utility: Compute single room score
static double ComputeRoomScore(
    const std::vector<int>& room,
    const std::map<std::pair<int,int>, double>& pair_scores);
```

### 10.3 评分函数修改

- `CalculateSimilarity`：改为高斯核，**签名不变**
- `CalculatePairScore`：对称化权重，**签名不变**
- `CalculateComplementarity`：升级为认知功能模型，**签名不变**

### 10.4 ExecuteAllocation 调用链

```cpp
// 旧版
auto rooms = GreedyAssign(profiles, dorms, pair_scores);

// 新版
auto pairs = FormPairs(profiles, pair_scores);
auto rooms = MergePairsIntoRooms(pairs, unpaired, profiles, pair_scores, 4);
auto final_rooms = RefineBySA(rooms, profiles, pair_scores, 4);
```

**对外接口 `ExecuteAllocation` 的签名和返回类型完全不变**，Service 层无需任何修改。

### 10.5 不需要修改的文件

- `StudentService.h / .cpp` — 不改
- `AdminService.h / .cpp` — 不改
- `CryptoUtil` / `Logger` — 不改
- `integration_test.cpp` — 不改（测试的是 API 行为，不是算法内部）
- `CMakeLists.txt` — 不改（无新外部依赖）

---

## 附录 A：算法选择对比

| 对比项 | 贪心（旧） | 两阶段+SA（新） | 图分割（KL） | 整数规划（ILP） | 遗传算法 |
|--------|-----------|----------------|-------------|----------------|---------|
| 全局最优性 | ❌ 局部 | ✅ 近似最优 | ✅ 近似 | ✅✅ 理论最优 | ✅ 近似 |
| 实现难度 | 低 | **中** | 中 | 高 | 高 |
| 外部依赖 | 无 | **无** | METIS | OR-Tools | 无 |
| 运行时间(500人) | <1s | **5-30s** | 1-5s | 10s-10min | 30s-5min |
| 可控性 | 低 | **高** | 中 | 低 | 中 |
| 改动量 | - | **200-300行** | 300+行 | 500+行 | 400+行 |

## 附录 B：参考文献

1. Kirkpatrick, S., et al. (1983). "Optimization by Simulated Annealing." *Science*, 220(4598), 671-680.
2. Kernighan, B.W., Lin, S. (1970). "An efficient heuristic procedure for partitioning graphs." *Bell System Technical Journal*, 49(2), 291-307.
3. Myers, I.B. (1962). *Introduction to Type: A Description of the Theory and Applications of the Myers-Briggs Type Indicator.* Consulting Psychologists Press.
4. CPAI (College Personality Inventory for Adjustment) — 宿舍匹配中的心理学维度参考。