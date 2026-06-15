# Ascend C · DropOut（高阶 adv_api）

- 类别：高阶 / filter
- 覆盖接口：`AscendC::DropOut<T>(dst, src, mask, sharedTmpBuffer, keepProb, DropOutShapeInfo)`
- 头文件：`lib/filter/dropout.h`（kernel）
- 状态：在本机 3510（Ascend950PR）无卡仿真上 **passed**。

## 为什么 DropOut 可确定性校验
Ascend C 的 DropOut **不是**在接口内部随机置零的。保留/丢弃由调用方传入的
`mask`（`uint8` tensor）决定，所以对固定 mask，输出完全可复现。产生随机 mask 的 RNG
是另一步骤，不在本接口内。

byte 掩码模式下，3510 实现逐元素计算：

```
dst[i] = (1 / keepProb) * (float)mask[i] * src[i]
```

`mask[i]` 由 `uint8` 转成计算 dtype 后参与乘法。`dropOutMode = 0`（默认）会自动选择布局：
当 `srcLastAxis == maskLastAxis` 时走 byte 对齐分支。

## 设计
- 形状 `[firstAxis = 1, lastAxis = 64]`，`dtype = float`。
- `src = 3.0`，`mask = 1`（全部保留），`keepProb = 0.5`。
- 期望 `dst = (1 / 0.5) * 1 * 3.0 = 6.0`，逐元素一致。已在 host 校验。

## 运行
```
bash harness/run_one.sh examples/ascendc/highlevel/dropout
```
