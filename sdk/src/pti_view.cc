#include "pti/pti_view.h"

#include "internal_helper.h"
#include "view_handler.h"

//
// TODO: parse different exception types, analyse caught exception and return
// different error code.
//
pti_result ptiViewEnable(pti_view_kind view_kind) {
  try {
    if (!(IsPtiViewKindEnum(view_kind))) {
      return pti_result::PTI_ERROR_BAD_ARGUMENT;
    }
    return Instance().Enable(view_kind);
  } catch (const std::overflow_error& e) {
    return pti_result::PTI_ERROR_INTERNAL;
  } catch (const std::runtime_error& e) {
    return pti_result::PTI_ERROR_INTERNAL;
  } catch (const std::exception& e) {
    return pti_result::PTI_ERROR_INTERNAL;
  } catch (...) {
    return pti_result::PTI_ERROR_INTERNAL;
  }
}

//
// TODO: parse different exception types, analyse caught exception and return
// different error code.
//
pti_result ptiViewDisable(pti_view_kind view_kind) {
  try {
    if (!(IsPtiViewKindEnum(view_kind))) {
      return pti_result::PTI_ERROR_BAD_ARGUMENT;
    }
    return Instance().Disable(view_kind);
  } catch (const std::overflow_error& e) {
    return pti_result::PTI_ERROR_INTERNAL;
  } catch (const std::runtime_error& e) {
    return pti_result::PTI_ERROR_INTERNAL;
  } catch (const std::exception& e) {
    return pti_result::PTI_ERROR_INTERNAL;
  } catch (...) {
    return pti_result::PTI_ERROR_INTERNAL;
  }
}

//
// TODO: parse different exception types, analyse caught exception and return
// different error code.
//
pti_result ptiViewSetCallbacks(pti_fptr_buffer_requested fptr_bufferRequested,
                               pti_fptr_buffer_completed fptr_bufferCompleted) {
  try {
    return Instance().RegisterBufferCallbacks(fptr_bufferRequested, fptr_bufferCompleted);
  } catch (const std::overflow_error& e) {
    return pti_result::PTI_ERROR_INTERNAL;
  } catch (const std::runtime_error& e) {
    return pti_result::PTI_ERROR_INTERNAL;
  } catch (const std::exception& e) {
    return pti_result::PTI_ERROR_INTERNAL;
  } catch (...) {
    return pti_result::PTI_ERROR_INTERNAL;
  }
}

//
// TODO: parse different exception types, analyse caught exception and return
// different error code.
//
pti_result ptiViewGetNextRecord(uint8_t* buffer, size_t valid_bytes,
                                pti_view_record_base** record) {
  try {
    return GetNextRecord(buffer, valid_bytes, record);
  } catch (const std::overflow_error& e) {
    return pti_result::PTI_ERROR_INTERNAL;
  } catch (const std::runtime_error& e) {
    return pti_result::PTI_ERROR_INTERNAL;
  } catch (const std::exception& e) {
    return pti_result::PTI_ERROR_INTERNAL;
  } catch (...) {
    return pti_result::PTI_ERROR_INTERNAL;
  }
}

//
// TODO: parse different exception types, analyse caught exception and return
// different error code.
//
pti_result ptiFlushAllViews() {
  try {
    return Instance().FlushBuffers();
  } catch (const std::overflow_error& e) {
    return pti_result::PTI_ERROR_INTERNAL;
  } catch (const std::runtime_error& e) {
    return pti_result::PTI_ERROR_INTERNAL;
  } catch (const std::exception& e) {
    return pti_result::PTI_ERROR_INTERNAL;
  } catch (...) {
    return pti_result::PTI_ERROR_INTERNAL;
  }
}

//
// TODO: parse different exception types, analyse caught exception and return
// different error code.
//
pti_result ptiViewPushExternalCorrelationId(pti_view_external_kind external_kind,
                                            uint64_t external_id) {
  try {
    return Instance().PushExternalKindId(external_kind, external_id);
  } catch (const std::overflow_error& e) {
    return pti_result::PTI_ERROR_INTERNAL;
  } catch (const std::runtime_error& e) {
    return pti_result::PTI_ERROR_INTERNAL;
  } catch (const std::exception& e) {
    return pti_result::PTI_ERROR_INTERNAL;
  } catch (...) {
    return pti_result::PTI_ERROR_INTERNAL;
  }
}

//
// TODO: parse different exception types, analyse caught exception and return
// different error code.
//
pti_result ptiViewPopExternalCorrelationId(pti_view_external_kind external_kind,
                                           uint64_t* p_external_id) {
  try {
    return Instance().PopExternalKindId(external_kind, p_external_id);
  } catch (const std::overflow_error& e) {
    return pti_result::PTI_ERROR_INTERNAL;
  } catch (const std::runtime_error& e) {
    return pti_result::PTI_ERROR_INTERNAL;
  } catch (const std::exception& e) {
    return pti_result::PTI_ERROR_INTERNAL;
  } catch (...) {
    return pti_result::PTI_ERROR_INTERNAL;
  }
}

// Capture all overhead_kind types and associate strings to static storage
//
inline constexpr static std::array<const char* const, 6> kOverheadKindType = {
    "INVALID", "UNKNOWN", "RESOURCE", "BUFFER_FLUSH", "BUFFER_DRIVER", "BUFFER_TIME"};

// Returns the stringified version of overhead kind type back.
const char* ptiViewOverheadKindToString(pti_view_overhead_kind type) {
  switch (type) {
    case PTI_VIEW_OVERHEAD_KIND_INVALID:
      return kOverheadKindType[0];
    case PTI_VIEW_OVERHEAD_KIND_UNKNOWN:
      return kOverheadKindType[1];
    case PTI_VIEW_OVERHEAD_KIND_RESOURCE:
      return kOverheadKindType[2];
    case PTI_VIEW_OVERHEAD_KIND_BUFFER_FLUSH:
      return kOverheadKindType[3];
    case PTI_VIEW_OVERHEAD_KIND_DRIVER:
      return kOverheadKindType[4];
    case PTI_VIEW_OVERHEAD_KIND_TIME:
      return kOverheadKindType[5];
  }
  return kOverheadKindType[0];
}

// Capture all memory types and associate strings to static storage
//
inline constexpr static std::array<const char* const, 5> kMemoryType = {
    "INVALID", "MEMORY(Unknown)", "HOST", "DEVICE", "SHARED"};

// Returns the stringified version of memory type back.
const char* ptiViewMemoryTypeToString(pti_view_memory_type type) {
  switch (type) {
    case PTI_VIEW_MEMORY_TYPE_MEMORY:
      return kMemoryType[1];
    case PTI_VIEW_MEMORY_TYPE_HOST:
      return kMemoryType[2];
    case PTI_VIEW_MEMORY_TYPE_DEVICE:
      return kMemoryType[3];
    case PTI_VIEW_MEMORY_TYPE_SHARED:
      return kMemoryType[4];
  }
  return kMemoryType[0];
}

// Capture all memcpy types and associate strings to static storage
//
inline constexpr static std::array<const char* const, 17> kMemcpyType = {
    "INVALID", "M2M", "M2H", "M2D", "M2S", "H2M", "H2H", "H2D", "H2S",
    "D2M",     "D2H", "D2D", "D2S", "S2M", "S2H", "S2D", "S2S"};

// Returns the stringified version of memcpy type back.
const char* ptiViewMemcpyTypeToString(pti_view_memcpy_type type) {
  switch (type) {
    case PTI_VIEW_MEMCPY_TYPE_M2M:
      return kMemcpyType[1];
    case PTI_VIEW_MEMCPY_TYPE_M2H:
      return kMemcpyType[2];
    case PTI_VIEW_MEMCPY_TYPE_M2D:
      return kMemcpyType[3];
    case PTI_VIEW_MEMCPY_TYPE_M2S:
      return kMemcpyType[4];
    case PTI_VIEW_MEMCPY_TYPE_H2M:
      return kMemcpyType[5];
    case PTI_VIEW_MEMCPY_TYPE_H2H:
      return kMemcpyType[6];
    case PTI_VIEW_MEMCPY_TYPE_H2D:
      return kMemcpyType[7];
    case PTI_VIEW_MEMCPY_TYPE_H2S:
      return kMemcpyType[8];
    case PTI_VIEW_MEMCPY_TYPE_D2M:
      return kMemcpyType[9];
    case PTI_VIEW_MEMCPY_TYPE_D2H:
      return kMemcpyType[10];
    case PTI_VIEW_MEMCPY_TYPE_D2D:
      return kMemcpyType[11];
    case PTI_VIEW_MEMCPY_TYPE_D2S:
      return kMemcpyType[12];
    case PTI_VIEW_MEMCPY_TYPE_S2M:
      return kMemcpyType[13];
    case PTI_VIEW_MEMCPY_TYPE_S2H:
      return kMemcpyType[14];
    case PTI_VIEW_MEMCPY_TYPE_S2D:
      return kMemcpyType[15];
    case PTI_VIEW_MEMCPY_TYPE_S2S:
      return kMemcpyType[16];
  }
  return kMemcpyType[0];
}
