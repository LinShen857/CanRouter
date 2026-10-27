#include "CanRouter.hpp"

namespace
{
CanRouter* g_router = nullptr;

void ClassicCallback(bool in_isr, CanRouter::Bus bus, const LibXR::CAN::ClassicPack& frame)
{
  (void)in_isr;
  if (g_router != nullptr)
  {
    g_router->DispatchClassic(bus, frame);
  }
}

void FdCallback(bool in_isr, CanRouter::Bus bus, const LibXR::FDCAN::FDPack& frame)
{
  (void)in_isr;
  if (g_router != nullptr)
  {
    g_router->DispatchFd(bus, frame);
  }
}

template <typename CallbackT>
void RegisterFrames(LibXR::FDCAN& can, const CallbackT& cb, bool extended_frames)
{
  can.Register(cb, LibXR::CAN::Type::STANDARD, LibXR::CAN::FilterMode::ID_RANGE, 0, 0x7FF);
  if (extended_frames)
  {
    can.Register(cb, LibXR::CAN::Type::EXTENDED, LibXR::CAN::FilterMode::ID_RANGE, 0,
                 0x1FFFFFFF);
  }
}
}  // namespace

CanRouter::CanRouter(LibXR::FDCAN& can1, LibXR::FDCAN& can2, LibXR::FDCAN& can3,
                     const BusConfig& cfg1, const BusConfig& cfg2, const BusConfig& cfg3)
{
  RegisterBus(can1, Bus::CAN1, cfg1);
  RegisterBus(can2, Bus::CAN2, cfg2);
  RegisterBus(can3, Bus::CAN3, cfg3);
  g_router = this;
}

CanRouter* CanRouter::Instance()
{
  return g_router;
}

void CanRouter::RegisterBus(LibXR::FDCAN& can, Bus bus, const BusConfig& cfg)
{
  bus_driver_[static_cast<uint8_t>(bus)] = &can;

  const auto classic = LibXR::CAN::Callback::Create(ClassicCallback, bus);
  RegisterFrames(can, classic, cfg.extended_frames);
  can.Register(classic, LibXR::CAN::Type::ERROR, LibXR::CAN::FilterMode::ID_RANGE, 0,
               UINT32_MAX);

  if (cfg.fd_frames)
  {
    const auto fd = LibXR::FDCAN::CallbackFD::Create(FdCallback, bus);
    RegisterFrames(can, fd, cfg.extended_frames);
  }
}

void CanRouter::Attach(Bus bus, CanDevice& device)
{
  for (auto& entry : device_table_[static_cast<uint8_t>(bus)])
  {
    if (entry.device == nullptr)
    {
      entry.device = &device;
      entry.id = device.motor_id;
      return;
    }
  }
}

CanErrorInfo CanRouter::GetErrorInfo(Bus bus) const
{
  return bus_error_[static_cast<uint8_t>(bus)];
}

CanDevice* CanRouter::FindDevice(Bus bus, uint32_t id) const
{
  for (const auto& entry : device_table_[static_cast<uint8_t>(bus)])
  {
    if (entry.device != nullptr && entry.id == id)
    {
      return entry.device;
    }
  }
  return nullptr;
}

void CanRouter::HandleError(Bus bus, const LibXR::CAN::ClassicPack& frame)
{
  const auto index = static_cast<uint8_t>(bus);
  ++bus_error_[index].count;
  bus_error_[index].last = LibXR::CAN::ToErrorID(frame.id);
  if (bus_driver_[index] != nullptr)
  {
    bus_driver_[index]->GetErrorState(bus_error_[index].state);
  }

  for (auto& entry : device_table_[index])
  {
    if (entry.device != nullptr)
    {
      entry.device->SetBusError(bus_error_[index]);
    }
  }
}

void CanRouter::DispatchClassic(Bus bus, const LibXR::CAN::ClassicPack& frame)
{
  if (frame.type == LibXR::CAN::Type::ERROR)
  {
    HandleError(bus, frame);
    return;
  }
  if (CanDevice* device = FindDevice(bus, frame.id); device != nullptr)
  {
    device->OnClassicFrame(frame);
  }
}

void CanRouter::DispatchFd(Bus bus, const LibXR::FDCAN::FDPack& frame)
{
  if (CanDevice* device = FindDevice(bus, frame.id); device != nullptr)
  {
    device->OnFdFrame(frame);
  }
}
