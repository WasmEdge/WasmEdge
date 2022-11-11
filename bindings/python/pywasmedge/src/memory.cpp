#include "WasmEdge.hpp"

/* --------------- MemoryTypeCxt -------------------------------- */
pysdk::MemoryTypeCxt::MemoryTypeCxt(WasmEdge_Limit &Lim) {
  context = WasmEdge_MemoryTypeCreate(Lim);
}

pysdk::MemoryTypeCxt::MemoryTypeCxt(const WasmEdge_MemoryTypeContext *cxt)
    : base(cxt) {}

pysdk::MemoryTypeCxt::~MemoryTypeCxt() {
  if (_del)
    WasmEdge_MemoryTypeDelete(context);
}
/* --------------- MemoryTypeCxt End -------------------------------- */

/* --------------- Memory End -------------------------------- */
pysdk::Memory::Memory(pysdk::MemoryTypeCxt &mem_cxt) {
  context = WasmEdge_MemoryInstanceCreate(mem_cxt.get());
}

pysdk::Memory::Memory(const WasmEdge_MemoryInstanceContext *cxt) : base(cxt) {}

pysdk::Memory::Memory(WasmEdge_MemoryInstanceContext *cxt) : base(cxt) {}

pysdk::Memory::~Memory() {
  if (_del)
    WasmEdge_MemoryInstanceDelete(context);
}

pysdk::result pysdk::Memory::set_data(pybind11::tuple data_,
                                      const uint32_t &offset) {
  const uint32_t length = pybind11::len(data_);
  uint8_t Data[length];
  for (size_t i = 0; i < length; i++) {
    Data[i] = data_[i].cast<uint8_t>();
  }

  return pysdk::result(
      WasmEdge_MemoryInstanceSetData(context, Data, offset, length));
}

pybind11::tuple pysdk::Memory::get_data(const uint32_t &length,
                                        const uint32_t &offset) {

  uint8_t Data[length];

  pysdk::result res(
      WasmEdge_MemoryInstanceGetData(context, Data, offset, length));

  pybind11::list ret_list;
  for (size_t i = 0; i < length; i++) {
    ret_list.append(Data[i]);
  }
  return pybind11::make_tuple(res, ret_list);
}

uint32_t pysdk::Memory::get_page_size() {
  return WasmEdge_MemoryInstanceGetPageSize(context);
}

pysdk::result pysdk::Memory::grow_page(const uint32_t &size) {
  return pysdk::result(WasmEdge_MemoryInstanceGrowPage(context, size));
}

pysdk::MemoryTypeCxt pysdk::Memory::get_type() {
  return pysdk::MemoryTypeCxt(WasmEdge_MemoryInstanceGetMemoryType(context));
}
/* --------------- Memory End -------------------------------- */
