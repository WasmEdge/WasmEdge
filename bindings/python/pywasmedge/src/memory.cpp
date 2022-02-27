#include "WasmEdge.hpp"

/* --------------- MemoryTypeCxt -------------------------------- */
pysdk::MemoryTypeCxt::MemoryTypeCxt(WasmEdge_Limit &Lim) {
  MemTypeCxt = WasmEdge_MemoryTypeCreate(Lim);
}

pysdk::MemoryTypeCxt::MemoryTypeCxt(WasmEdge_MemoryTypeContext *cxt, bool del) {
  MemTypeCxt = cxt;
  delete_cxt = del;
}

pysdk::MemoryTypeCxt::~MemoryTypeCxt() {
  if (delete_cxt)
    WasmEdge_MemoryTypeDelete(MemTypeCxt);
}

WasmEdge_MemoryTypeContext *pysdk::MemoryTypeCxt::get() { return MemTypeCxt; }
/* --------------- MemoryTypeCxt End -------------------------------- */

/* --------------- Memory End -------------------------------- */
pysdk::Memory::Memory(pysdk::MemoryTypeCxt &mem_cxt) {
  HostMemory = WasmEdge_MemoryInstanceCreate(mem_cxt.get());
}

pysdk::Memory::Memory(WasmEdge_MemoryInstanceContext *cxt, bool del) {
  HostMemory = cxt;
  delete_mem = del;
}

pysdk::Memory::~Memory() {
  if (delete_mem)
    WasmEdge_MemoryInstanceDelete(HostMemory);
}

pysdk::result pysdk::Memory::set_data(pybind11::tuple data_,
                                      const uint32_t &offset) {
  const uint32_t length = pybind11::len(data_);
  uint8_t Data[length];
  for (size_t i = 0; i < length; i++) {
    Data[i] = data_[i].cast<uint8_t>();
  }

  return pysdk::result(
      WasmEdge_MemoryInstanceSetData(HostMemory, Data, offset, length));
}

pybind11::tuple pysdk::Memory::get_data(const uint32_t &length,
                                        const uint32_t &offset) {

  uint8_t Data[length];

  pysdk::result res(
      WasmEdge_MemoryInstanceGetData(HostMemory, Data, offset, length));

  pybind11::list ret_list;
  for (size_t i = 0; i < length; i++) {
    ret_list.append(Data[i]);
  }
  return pybind11::make_tuple(res, ret_list);
}

uint32_t pysdk::Memory::get_page_size() {
  return WasmEdge_MemoryInstanceGetPageSize(HostMemory);
}

pysdk::result pysdk::Memory::grow_page(const uint32_t &size) {
  return pysdk::result(WasmEdge_MemoryInstanceGrowPage(HostMemory, size));
}

pysdk::MemoryTypeCxt pysdk::Memory::get_type() {
  return pysdk::MemoryTypeCxt(
      const_cast<WasmEdge_MemoryTypeContext *>(
          WasmEdge_MemoryInstanceGetMemoryType(HostMemory)),
      false);
}
/* --------------- Memory End -------------------------------- */
