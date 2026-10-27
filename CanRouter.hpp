#pragma once

// clang-format off
/* === MODULE MANIFEST V2 ===
module_description: 多条 FDCAN 总线的统一接收分发与总线错误广播
depends:
  - id: LinShen857/CanDevice
    ref: same-or-dev
=== END MANIFEST === */
// clang-format on

#include <cstdint>

#include "CanDevice.hpp"
#include "can.hpp"

class CanRouter
{
 public:
  enum class Bus : uint8_t
  {
    CAN1,
    CAN2,
    CAN3
  };

  struct BusConfig
  {
    bool extended_frames = true;
    bool fd_frames = true;
  };

  CanRouter(LibXR::FDCAN& can1, LibXR::FDCAN& can2, LibXR::FDCAN& can3,
            BusConfig cfg1 = BusConfig(), BusConfig cfg2 = BusConfig(),
            BusConfig cfg3 = BusConfig());

  CanRouter(const CanRouter&) = delete;
  CanRouter& operator=(const CanRouter&) = delete;

  /// 把设备挂到总线上，按 motor_id 精确匹配接收帧 / Attach a device, keyed by motor_id.
  void Attach(Bus bus, CanDevice& device);

  CanErrorInfo GetErrorInfo(Bus bus) const;

  /// 该实例在构造时登记为当前路由 / The instance registered by its constructor.
  static CanRouter* Instance();

  /// 由驱动的接收回调调用，运行在中断上下文 / Called from the driver callback, in ISR context.
  void DispatchClassic(Bus bus, const LibXR::CAN::ClassicPack& frame);
  void DispatchFd(Bus bus, const LibXR::FDCAN::FDPack& frame);

 private:
  static constexpr uint8_t bus_num_ = 3;
  static constexpr uint8_t max_devices_ = 16;

  struct DeviceEntry
  {
    CanDevice* device = nullptr;
    uint32_t id = 0;
  };

  void RegisterBus(LibXR::FDCAN& can, Bus bus, const BusConfig& cfg);
  void HandleError(Bus bus, const LibXR::CAN::ClassicPack& frame);
  CanDevice* FindDevice(Bus bus, uint32_t id) const;

  DeviceEntry device_table_[bus_num_][max_devices_] {};
  LibXR::FDCAN* bus_driver_[bus_num_] {};
  mutable CanErrorInfo bus_error_[bus_num_] {};
};
