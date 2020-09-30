//==============================================================
// Copyright © 2020 Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================

#include <iostream>

#include "ze_intercept.h"
#include "ze_utils.h"

static ZeIntercept* intercept = nullptr;

extern "C"
#if defined(_WIN32)
__declspec(dllexport)
#endif
void Usage() {
  std::cout <<
    "Usage: ./ze_intercept[.exe] [options] <application> <args>" <<
    std::endl;
  std::cout << "Options:" << std::endl;
  std::cout <<
    "--call-logging [-c]             Trace host API calls" <<
    std::endl;
  std::cout <<
    "--host-timing  [-h]             Report host API execution time" <<
    std::endl;
  std::cout <<
    "--device-timing [-d]            Report kernels execution time" <<
    std::endl;
}

extern "C"
#if defined(_WIN32)
__declspec(dllexport)
#endif
int ParseArgs(int argc, char* argv[]) {
  int app_index = 1;
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--call-logging") == 0 ||
        strcmp(argv[i], "-c") == 0) {
      utils::SetEnv("ZEI_CallLogging=1");
      ++app_index;
    } else if (strcmp(argv[i], "--host-timing") == 0 ||
               strcmp(argv[i], "-h") == 0) {
      utils::SetEnv("ZEI_HostTiming=1");
      ++app_index;
    } else if (strcmp(argv[i], "--device-timing") == 0 ||
               strcmp(argv[i], "-d") == 0) {
      utils::SetEnv("ZEI_DeviceTiming=1");
      ++app_index;
    } else {
      break;
    }
  }
  return app_index;
}

extern "C"
#if defined(_WIN32)
__declspec(dllexport)
#endif
void SetToolEnv() {
  utils::SetEnv("ZET_ENABLE_API_TRACING_EXP=1");
}

static unsigned ReadArgs() {
  std::string value;
  unsigned options = 0;

  value = utils::GetEnv("ZEI_CallLogging");
  if (!value.empty() && value == "1") {
    options |= (1 << ZEI_CALL_LOGGING);
  }

  value = utils::GetEnv("ZEI_HostTiming");
  if (!value.empty() && value == "1") {
    options |= (1 << ZEI_HOST_TIMING);
  }

  value = utils::GetEnv("ZEI_DeviceTiming");
  if (!value.empty() && value == "1") {
    options |= (1 << ZEI_DEVICE_TIMING);
  }

  return options;
}

void EnableProfiling() {
  ze_result_t status = ZE_RESULT_SUCCESS;
  status = zeInit(ZE_INIT_FLAG_GPU_ONLY);
  PTI_ASSERT(status == ZE_RESULT_SUCCESS);

  ze_device_handle_t device = nullptr;
  ze_driver_handle_t driver = nullptr;

  utils::ze::GetIntelDeviceAndDriver(ZE_DEVICE_TYPE_GPU, device, driver);
  if (device == nullptr || driver == nullptr) {
    std::cout << "[WARNING] Unable to find target" <<
      " device for tracing" << std::endl;
    return;
  }

  ze_context_handle_t context = utils::ze::GetContext(driver);
  PTI_ASSERT(context != nullptr);

  intercept = ZeIntercept::Create(context, device, ReadArgs());
}

void DisableProfiling() {
  if (intercept != nullptr) {
    delete intercept;
    std::cerr << std::endl;
  }
}