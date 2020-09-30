//==============================================================
// Copyright © 2020 Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================

#ifndef PTI_SAMPLES_ZE_HOT_KERNELS_ZE_KERNEL_COLLECTOR_H_
#define PTI_SAMPLES_ZE_HOT_KERNELS_ZE_KERNEL_COLLECTOR_H_

#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "utils.h"
#include "ze_utils.h"

namespace ze_kernel_collector {
  const uint32_t kKernelLength = 10;
  const uint32_t kCallsLength = 12;
  const uint32_t kSimdLength = 5;
  const uint32_t kTransferredLength = 20;
  const uint32_t kTimeLength = 20;
  const uint32_t kPercentLength = 10;
} // namespace ze_kernel_collector

struct KernelInstance {
  std::string name;
  size_t simd_width;
  size_t bytes_transferred;
  ze_event_pool_handle_t event_pool;
  ze_event_handle_t event;
};

struct Kernel {
  uint64_t total_time;
  uint64_t min_time;
  uint64_t max_time;
  uint64_t call_count;
  size_t simd_width;
  size_t bytes_transferred;

  bool operator>(const Kernel& r) const {
    if (total_time != r.total_time) {
      return total_time > r.total_time;
    }
    return call_count > r.call_count;
  }

  bool operator!=(const Kernel& r) const {
    if (total_time == r.total_time) {
      return call_count != r.call_count;
    }
    return true;
  }
};

using KernelInfoMap = std::map<std::string, Kernel>;
using KernelNameMap = std::map<ze_kernel_handle_t, std::string>;

class ZeKernelCollector {
 public: // Interface

  static ZeKernelCollector* Create(ze_context_handle_t context,
                                   ze_device_handle_t device) {
    PTI_ASSERT(context != nullptr);
    PTI_ASSERT(device != nullptr);

    ZeKernelCollector* collector = new ZeKernelCollector(context, device);
    PTI_ASSERT(collector != nullptr);

    ze_result_t status = ZE_RESULT_SUCCESS;
    zet_tracer_exp_desc_t tracer_desc = {
        ZET_STRUCTURE_TYPE_TRACER_EXP_DESC, nullptr, collector};
    zet_tracer_exp_handle_t tracer = nullptr;
    status = zetTracerExpCreate(context, &tracer_desc, &tracer);
    if (status != ZE_RESULT_SUCCESS) {
      std::cerr <<
        "[WARNING] Unable to create Level Zero tracer for target context" <<
        std::endl;
      delete collector;
      return nullptr;
    }

    collector->EnableTracing(tracer);
    return collector;
  }

  static void PrintKernelsTable(const KernelInfoMap& kernel_info_map) {
    std::set< std::pair<std::string, Kernel>,
              utils::Comparator > sorted_list(
        kernel_info_map.begin(), kernel_info_map.end());

    uint64_t total_duration = 0;
    size_t max_name_length = ze_kernel_collector::kKernelLength;
    for (auto& value : sorted_list) {
      total_duration += value.second.total_time;
      if (value.first.size() > max_name_length) {
        max_name_length = value.first.size();
      }
    }

    if (total_duration == 0) {
      return;
    }

    std::cerr << std::setw(max_name_length) << "Kernel" << "," <<
      std::setw(ze_kernel_collector::kCallsLength) << "Calls" << "," <<
      std::setw(ze_kernel_collector::kSimdLength) << "SIMD" << "," <<
      std::setw(ze_kernel_collector::kTransferredLength) <<
        "Transferred (bytes)" << "," <<
      std::setw(ze_kernel_collector::kTimeLength) << "Time (ns)" << "," <<
      std::setw(ze_kernel_collector::kPercentLength) << "Time (%)" << "," <<
      std::setw(ze_kernel_collector::kTimeLength) << "Average (ns)" << "," <<
      std::setw(ze_kernel_collector::kTimeLength) << "Min (ns)" << "," <<
      std::setw(ze_kernel_collector::kTimeLength) << "Max (ns)" << std::endl;

    for (auto& value : sorted_list) {
      const std::string& function = value.first;
      uint64_t call_count = value.second.call_count;
      size_t simd_width = value.second.simd_width;
      size_t bytes_transferred = value.second.bytes_transferred;
      uint64_t duration = value.second.total_time;
      uint64_t avg_duration = duration / call_count;
      uint64_t min_duration = value.second.min_time;
      uint64_t max_duration = value.second.max_time;
      float percent_duration = 100.0f * duration / total_duration;
      std::cerr << std::setw(max_name_length) << function << "," <<
        std::setw(ze_kernel_collector::kCallsLength) << call_count << "," <<
        std::setw(ze_kernel_collector::kSimdLength) << simd_width << "," <<
        std::setw(ze_kernel_collector::kTransferredLength) <<
          bytes_transferred << "," <<
        std::setw(ze_kernel_collector::kTimeLength) << duration << "," <<
        std::setw(ze_kernel_collector::kPercentLength) << std::setprecision(2) <<
          std::fixed << percent_duration << "," <<
        std::setw(ze_kernel_collector::kTimeLength) << avg_duration << "," <<
        std::setw(ze_kernel_collector::kTimeLength) << min_duration << "," <<
        std::setw(ze_kernel_collector::kTimeLength) << max_duration << std::endl;
    }
  }

  ~ZeKernelCollector() {
    ze_result_t status = ZE_RESULT_SUCCESS;

    if (tracer_ != nullptr) {
      status = zetTracerExpDestroy(tracer_);
      PTI_ASSERT(status == ZE_RESULT_SUCCESS);
    }

    PTI_ASSERT(context_ != nullptr);
    status = zeContextDestroy(context_);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);
  }

  void DisableTracing() {
    PTI_ASSERT(tracer_ != nullptr);
    ze_result_t status = ZE_RESULT_SUCCESS;
    status = zetTracerExpSetEnabled(tracer_, false);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);
  }

  const KernelInfoMap& GetKernelInfoMap() const {
    return kernel_info_map_;
  }

 private: // Implementation

  ZeKernelCollector(ze_context_handle_t context, ze_device_handle_t device) :
      context_(context) {
    PTI_ASSERT(context_ != nullptr);
    PTI_ASSERT(device != nullptr);

    ze_device_properties_t props{};
    props.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES;
    ze_result_t status = zeDeviceGetProperties(device, &props);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);

    timer_resolution_ = props.timerResolution;
  }

  void EnableTracing(zet_tracer_exp_handle_t tracer) {
    PTI_ASSERT(tracer != nullptr);
    tracer_ = tracer;

    zet_core_callbacks_t prologue_callbacks{};
    zet_core_callbacks_t epilogue_callbacks{};

    prologue_callbacks.Event.pfnDestroyCb = OnEnterEventDestroy;

    prologue_callbacks.Event.pfnHostResetCb = OnEnterEventHostReset;

    prologue_callbacks.EventPool.pfnCreateCb = OnEnterEventPoolCreate;
    epilogue_callbacks.EventPool.pfnCreateCb = OnExitEventPoolCreate;

    prologue_callbacks.CommandList.pfnAppendLaunchKernelCb =
      OnEnterCommandListAppendLaunchKernel;
    epilogue_callbacks.CommandList.pfnAppendLaunchKernelCb =
      OnExitCommandListAppendLaunchKernel;

    prologue_callbacks.CommandList.pfnAppendMemoryCopyCb =
      OnEnterCommandListAppendMemoryCopy;
    epilogue_callbacks.CommandList.pfnAppendMemoryCopyCb =
      OnExitCommandListAppendMemoryCopy;

    epilogue_callbacks.Kernel.pfnCreateCb = OnExitKernelCreate;

    epilogue_callbacks.Kernel.pfnDestroyCb = OnExitKernelDestroy;

    epilogue_callbacks.CommandQueue.pfnSynchronizeCb = OnExitCommandQueueSynchronize;
    epilogue_callbacks.CommandQueue.pfnDestroyCb = OnExitCommandQueueDestroy;

    ze_result_t status = ZE_RESULT_SUCCESS;
    status = zetTracerExpSetPrologues(tracer_, &prologue_callbacks);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);
    status = zetTracerExpSetEpilogues(tracer_, &epilogue_callbacks);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);
    status = zetTracerExpSetEnabled(tracer_, true);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);
  }

  void AddKernelName(ze_kernel_handle_t kernel, std::string name) {
    PTI_ASSERT(kernel != nullptr);
    PTI_ASSERT(!name.empty());

    const std::lock_guard<std::mutex> lock(lock_);
    PTI_ASSERT(kernel_name_map_.count(kernel) == 0);
    kernel_name_map_[kernel] = name;
  }

  void RemoveKernelName(ze_kernel_handle_t kernel) {
    PTI_ASSERT(kernel != nullptr);

    const std::lock_guard<std::mutex> lock(lock_);
    PTI_ASSERT(kernel_name_map_.count(kernel) == 1);
    kernel_name_map_.erase(kernel);
  }

  std::string GetKernelName(ze_kernel_handle_t kernel) {
    PTI_ASSERT(kernel != nullptr);

    const std::lock_guard<std::mutex> lock(lock_);
    if (kernel_name_map_.count(kernel) == 1) {
      return kernel_name_map_[kernel];
    }

    return std::string();
  }

  void AddKernelInstance(const KernelInstance& instance) {
    const std::lock_guard<std::mutex> lock(lock_);
    kernel_instance_list_.push_back(instance);
  }

  void ProcessInstance(ze_event_handle_t event) {
    PTI_ASSERT(event != nullptr);
    const std::lock_guard<std::mutex> lock(lock_);

    size_t i = 0;
    while (i < kernel_instance_list_.size()) {
      if (kernel_instance_list_[i].event == event) {
        break;
      }
      ++i;
    }

    if (i == kernel_instance_list_.size()) {
      return;
    }

    ProcessInstance(kernel_instance_list_[i]);
    kernel_instance_list_[i] =
      kernel_instance_list_[kernel_instance_list_.size() - 1];
    kernel_instance_list_.pop_back();
  }

  void ProcessInstance(const KernelInstance& instance) {
    ze_result_t status = ZE_RESULT_SUCCESS;
    status = zeEventQueryStatus(instance.event);
    if (status == ZE_RESULT_SUCCESS) {
      ze_kernel_timestamp_result_t timestamp{};
      status = zeEventQueryKernelTimestamp(instance.event, &timestamp);
      PTI_ASSERT(status == ZE_RESULT_SUCCESS);

      uint64_t start = timestamp.global.kernelStart;
      uint64_t end = timestamp.global.kernelEnd;
      uint64_t time = 0;
      if (start < end) {
        time = (end - start) * timer_resolution_;
      } else { // 32-bit timer overflow
        PTI_ASSERT(start < (1ULL << 32));
        time = ((1ULL << 32) - start + end) * timer_resolution_;
      }
      AddKernelInfo(instance.name, time,
                    instance.simd_width,
                    instance.bytes_transferred);
    }

    if (instance.event_pool != nullptr) {
      ze_result_t status = ZE_RESULT_SUCCESS;
      status = zeEventDestroy(instance.event);
      PTI_ASSERT(status == ZE_RESULT_SUCCESS);
      status = zeEventPoolDestroy(instance.event_pool);
      PTI_ASSERT(status == ZE_RESULT_SUCCESS);
    }
  }

  void ProcessInstances() {
    const std::lock_guard<std::mutex> lock(lock_);
    for (const auto& instance : kernel_instance_list_) {
      ProcessInstance(instance);
    }
    kernel_instance_list_.clear();
  }

  void AddKernelInfo(std::string name, uint64_t time,
                     size_t simd_width, size_t bytes_transferred) {
    PTI_ASSERT(!name.empty());
    if (kernel_info_map_.count(name) == 0) {
      kernel_info_map_[name] = {
        time, time, time, 1, simd_width, bytes_transferred};
    } else {
      Kernel& kernel = kernel_info_map_[name];
      kernel.total_time += time;
      if (time > kernel.max_time) {
        kernel.max_time = time;
      }
      if (time < kernel.min_time) {
        kernel.min_time = time;
      }
      kernel.call_count += 1;
      kernel.bytes_transferred += bytes_transferred;
      PTI_ASSERT(kernel.simd_width == simd_width);
    }
  }

 private: // Callbacks

  static void OnExitKernelCreate(
      ze_kernel_create_params_t *params, ze_result_t result,
      void *global_data, void **instance_data) {
    if (result == ZE_RESULT_SUCCESS) {
      ZeKernelCollector* collector =
        reinterpret_cast<ZeKernelCollector*>(global_data);
      PTI_ASSERT(collector != nullptr);
      collector->AddKernelName(
          **(params->pphKernel), (*(params->pdesc))->pKernelName);
    }
  }

  static void OnExitKernelDestroy(ze_kernel_destroy_params_t *params,
                                  ze_result_t result,
                                  void *global_data,
                                  void **instance_data) {
    if (result == ZE_RESULT_SUCCESS) {
      ZeKernelCollector* collector =
        reinterpret_cast<ZeKernelCollector*>(global_data);
      PTI_ASSERT(collector != nullptr);
      collector->RemoveKernelName(*(params->phKernel));
    }
  }

  static void OnEnterEventPoolCreate(ze_event_pool_create_params_t *params,
                                     ze_result_t result,
                                     void *global_data,
                                     void **instance_data) {
    const ze_event_pool_desc_t* desc = *(params->pdesc);
    if (desc == nullptr) {
      return;
    }
    if (desc->flags & ZE_EVENT_POOL_FLAG_IPC) {
      return;
    }

    ze_event_pool_desc_t* profiling_desc = new ze_event_pool_desc_t;
    PTI_ASSERT(profiling_desc != nullptr);
    profiling_desc->stype = desc->stype;
    // PTI_ASSERT(profiling_desc->stype == ZE_STRUCTURE_TYPE_EVENT_POOL_DESC);
    profiling_desc->pNext = desc->pNext;
    profiling_desc->flags = (desc->flags | ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP);
    profiling_desc->count = desc->count;

    *(params->pdesc) = profiling_desc;
    *instance_data = profiling_desc;
  }

  static void OnExitEventPoolCreate(ze_event_pool_create_params_t *params,
                                    ze_result_t result,
                                    void *global_data,
                                    void **instance_data) {
    ze_event_pool_desc_t* desc =
      static_cast<ze_event_pool_desc_t*>(*instance_data);
    if (desc != nullptr) {
      delete desc;
    }
  }

  static void OnEnterEventDestroy(ze_event_destroy_params_t *params,
                                  ze_result_t result,
                                  void *global_data,
                                  void **instance_data) {
    if (*(params->phEvent) != nullptr) {
      ZeKernelCollector* collector =
        reinterpret_cast<ZeKernelCollector*>(global_data);
      PTI_ASSERT(collector != nullptr);
      collector->ProcessInstance(*(params->phEvent));
    }
  }

  static void OnEnterEventHostReset(ze_event_host_reset_params_t *params,
                                    ze_result_t result,
                                    void *global_data,
                                    void **instance_data) {
    if (*(params->phEvent) != nullptr) {
      ZeKernelCollector* collector =
        reinterpret_cast<ZeKernelCollector*>(global_data);
      PTI_ASSERT(collector != nullptr);
      collector->ProcessInstance(*(params->phEvent));
    }
  }

  static void CreateEvent(ze_context_handle_t context,
                          ze_event_pool_handle_t& event_pool,
                          ze_event_handle_t& event) {
    PTI_ASSERT(context != nullptr);
    ze_result_t status = ZE_RESULT_SUCCESS;

    ze_event_pool_desc_t event_pool_desc = {
        ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr,
        ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP, 1};
    status = zeEventPoolCreate(
        context, &event_pool_desc, 0, nullptr, &event_pool);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);

    ze_event_desc_t event_desc = {
        ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0,
        ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_HOST};
    zeEventCreate(event_pool, &event_desc, &event);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);
  }

  static void OnEnterCommandListAppendLaunchKernel(
      ze_command_list_append_launch_kernel_params_t* params,
      ze_result_t result, void* global_data, void** instance_data) {
    ZeKernelCollector* collector =
        reinterpret_cast<ZeKernelCollector*>(global_data);
    PTI_ASSERT(collector != nullptr);

    if (*(params->phKernel) ==  nullptr) {
      return;
    }

    KernelInstance* instance = new KernelInstance;
    PTI_ASSERT(instance != nullptr);
    instance->name = collector->GetKernelName(*(params->phKernel));
    PTI_ASSERT(!instance->name.empty());

    ze_kernel_properties_t props{};
    ze_result_t status = zeKernelGetProperties(*(params->phKernel), &props);
    PTI_ASSERT(status == ZE_RESULT_SUCCESS);
    instance->simd_width = props.maxSubgroupSize;
    instance->bytes_transferred = 0;

    if (*(params->phSignalEvent) == nullptr) {
      CreateEvent(collector->context_, instance->event_pool, instance->event);
      *(params->phSignalEvent) = instance->event;
    } else {
      instance->event_pool = nullptr;
      instance->event = *(params->phSignalEvent);
    }

    *instance_data = static_cast<void*>(instance);
  }

  static void OnEnterCommandListAppendMemoryCopy(
      ze_command_list_append_memory_copy_params_t* params,
      ze_result_t result, void* global_data, void** instance_data) {
    ZeKernelCollector* collector =
        reinterpret_cast<ZeKernelCollector*>(global_data);
    PTI_ASSERT(collector != nullptr);

    KernelInstance* instance = new KernelInstance;
    instance->name = "zeCommandListAppendMemoryCopy";
    instance->bytes_transferred = *(params->psize);
    instance->simd_width = 0;

    if (*(params->phSignalEvent) == nullptr) {
      CreateEvent(collector->context_, instance->event_pool, instance->event);
      *(params->phSignalEvent) = instance->event;
    } else {
      instance->event_pool = nullptr;
      instance->event = *(params->phSignalEvent);
    }

    *instance_data = static_cast<void*>(instance);
  }

  static void OnExitKernelAppend(void* global_data, void** instance_data,
                                 ze_result_t result) {
    KernelInstance* instance = static_cast<KernelInstance*>(*instance_data);
    if (instance == nullptr) {
      return;
    }

    if (result != ZE_RESULT_SUCCESS) {
      if (instance->event_pool != nullptr) {
        ze_result_t status = ZE_RESULT_SUCCESS;
        status = zeEventDestroy(instance->event);
        PTI_ASSERT(status == ZE_RESULT_SUCCESS);
        status = zeEventPoolDestroy(instance->event_pool);
        PTI_ASSERT(status == ZE_RESULT_SUCCESS);
      }
    } else {
      ZeKernelCollector* collector =
        reinterpret_cast<ZeKernelCollector*>(global_data);
      PTI_ASSERT(collector != nullptr);
      collector->AddKernelInstance(*instance);
    }

    delete instance;
  }

  static void OnExitCommandListAppendLaunchKernel(
      ze_command_list_append_launch_kernel_params_t* params,
      ze_result_t result, void* global_data, void** instance_data) {
    PTI_ASSERT(*(params->phSignalEvent) != nullptr);
    OnExitKernelAppend(global_data, instance_data, result);
  }

  static void OnExitCommandListAppendMemoryCopy(
      ze_command_list_append_memory_copy_params_t* params,
      ze_result_t result, void* global_data, void** instance_data) {
    PTI_ASSERT(*(params->phSignalEvent) != nullptr);
    OnExitKernelAppend(global_data, instance_data, result);
  }

  static void OnExitCommandQueueSynchronize(
      ze_command_queue_synchronize_params_t* params,
      ze_result_t result, void* global_data, void** instance_data) {
    if (result == ZE_RESULT_SUCCESS) {
      ZeKernelCollector* collector =
        reinterpret_cast<ZeKernelCollector*>(global_data);
      PTI_ASSERT(collector != nullptr);
      collector->ProcessInstances();
    }
  }

  static void OnExitCommandQueueDestroy(
      ze_command_queue_destroy_params_t* params,
      ze_result_t result, void* global_data, void** instance_data) {
    if (result == ZE_RESULT_SUCCESS) {
      ZeKernelCollector* collector =
        reinterpret_cast<ZeKernelCollector*>(global_data);
      PTI_ASSERT(collector != nullptr);
      collector->ProcessInstances();
    }
  }

 private: // Data

  ze_context_handle_t context_ = nullptr;
  zet_tracer_exp_handle_t tracer_ = nullptr;

  uint64_t timer_resolution_ = 0;

  std::mutex lock_;
  KernelInfoMap kernel_info_map_;
  KernelNameMap kernel_name_map_;
  std::vector<KernelInstance> kernel_instance_list_;
};

#endif // PTI_SAMPLES_ZE_HOT_KERNELS_ZE_KERNEL_COLLECTOR_H_