#pragma once

inline void writeUInt32(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                 uint32_t Value, uint32_t &Ptr) {
  uint32_t *BufPtr = MemInst.getPointer<uint32_t *>(Ptr);
  *BufPtr = Value;
  Ptr += 4;
}

inline void writeIInt32(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                 int32_t Value, uint32_t &Ptr) {
  int32_t *BufPtr = MemInst.getPointer<int32_t *>(Ptr);
  *BufPtr = Value;
  Ptr += 4;
}

inline int32_t readIInt32(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                   uint32_t &Ptr) {
  int32_t *BufPtr = MemInst.getPointer<int32_t *>(Ptr);
  return *BufPtr;
}

inline uint32_t readUInt32(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                    uint32_t &Ptr) {
  uint32_t *BufPtr = MemInst.getPointer<uint32_t *>(Ptr);
  return *BufPtr;
}
