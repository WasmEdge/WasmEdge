FROM ubuntu:22.04 AS base

RUN apt update && \
	apt upgrade -y && \
	apt install -y git wget

RUN mkdir -p /src

RUN git clone https://github.com/WasmEdge/WasmEdge.git /src/WasmEdge -b dm4/cann
RUN git clone https://github.com/ggerganov/llama.cpp.git /src/llama.cpp -b b3651
RUN git clone https://github.com/simdjson/simdjson.git /src/simdjson -b v3.10.0
RUN git clone https://github.com/fmtlib/fmt.git /src/fmt -b 11.0.2
RUN git clone https://github.com/gabime/spdlog.git /src/spdlog -b v1.13.0

FROM ubuntu:22.04 AS src
COPY --from=base /src /
