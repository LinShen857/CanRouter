# CanRouter

## 1. 模块作用

把多条 FDCAN 总线的接收统一成一张「总线 + 帧 ID → 设备」的分发表：`CanDevice` 在构造时挂到某条总线上，之后驱动中断里收帧、错误状态广播都由此模块完成，设备模块不需要各自注册回调。

Dispatches incoming frames from several FDCAN buses to attached `CanDevice` instances, and broadcasts bus error snapshots to them.

## 2. 构造接口

```cpp
CanRouter(LibXR::FDCAN& can1, LibXR::FDCAN& can2, LibXR::FDCAN& can3,
          BusConfig cfg1 = BusConfig(), BusConfig cfg2 = BusConfig(),
          BusConfig cfg3 = BusConfig());
```

依赖参数 / Dependencies:

- `LibXR::FDCAN& can1/can2/can3` — 三条总线的驱动实例

配置参数 / Configuration:

- `BusConfig extended_frames` / `fd_frames` — 该总线是否订阅扩展帧、是否订阅 CAN FD 帧

## 3. 公开接口

- `void Attach(Bus bus, CanDevice& device)` — 按 `device.motor_id` 分发
- `CanErrorInfo GetErrorInfo(Bus bus) const`
- `static CanRouter* Instance()`

## 4. 配置示例

```yaml
modules:
  - module: LinShen857/CanRouter
    id: can_router
    args:
      - can1: fdcan1
      - can2: fdcan2
      - can3: fdcan3
      - cfg1:
          extended_frames: false
          fd_frames: false
      - cfg2:
          extended_frames: false
          fd_frames: true
      - cfg3:
          extended_frames: false
          fd_frames: false
```

## 5. 依赖与硬件

模块依赖 / Module dependencies:

- `LinShen857/CanDevice`

硬件 / Hardware:

- `LibXR::FDCAN` × 3
