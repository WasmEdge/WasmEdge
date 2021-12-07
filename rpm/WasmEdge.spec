%global gittag 0.9.0-rc.5
Name:    wasmedge
Version: 0.9.0-rc.5
Release: 1%{?dist}
Summary: High performance WebAssembly Virtual Machine
License: Apache 2.0
URL:     https://github.com/WasmEdge/WasmEdge
Source0: https://github.com/WasmEdge/WasmEdge/archive/%{gittag}/%{name}-%{version}.tar.gz
BuildRequires: gcc-c++,cmake,ninja-build,boost-devel,spdlog-devel,llvm12-devel,lld-devel
Requires:      llvm12
ExclusiveArch: x86_64 aarch64

%description
High performance WebAssembly Virtual Machine

%prep
%autosetup -n WasmEdge-%{gittag}
sed -i -e 's@0.0.0-unreleased@%{gittag}@' CMakeLists.txt include/CMakeLists.txt

%build
cmake -Bbuild -GNinja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DWASMEDGE_BUILD_TESTS=OFF -DLLVM_DIR=%{_libdir}/llvm12/lib/cmake/llvm -DCMAKE_INSTALL_PREFIX=%{_prefix} .
cmake --build build

%install
DESTDIR=%{buildroot} cmake --build build --target install

%files
%{_bindir}/*
%{_libdir}/*
%{_includedir}/*

%changelog
