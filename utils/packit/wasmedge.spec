%global version 0.14.1
%global reponame WasmEdge
%global capi_soname 0
%global capi_version 0.1.0

Name:    wasmedge
Version: %{version}
Release: %autorelease
Summary: High performance WebAssembly Virtual Machine
# The entire source code is ASL 2.0 except LICENSE.spdx which is CC0
# Automatically converted from old format: ASL 2.0 and CC0 - review is highly recommended.
License: Apache-2.0 AND CC0-1.0
URL:     https://github.com/%{reponame}/%{reponame}
Source0: %{url}/releases/download/%{version}/%{reponame}-%{version}-src.tar.gz
BuildRequires: cmake
BuildRequires: gcc-c++
BuildRequires: git
BuildRequires: ninja-build
BuildRequires: spdlog-devel
Requires:      spdlog

# Specify LLVM version for Fedora >= 41
%if %{defined fedora} && 0%{?fedora} >= 41
BuildRequires: lld18-devel
BuildRequires: llvm18-devel
Requires:      lld18
Requires:      llvm18
%else
BuildRequires: lld-devel
BuildRequires: llvm-devel
Requires:      lld
Requires:      llvm
%endif

# Currently wasmedge could only be built on specific arches
ExclusiveArch: x86_64 aarch64
Provides: %{reponame} = %{version}-%{release}
Provides: bundled(blake3) = 1.2.0
Provides: bundled(wasi-cpp-header) = 0.0.1
Provides: wasm-library
Conflicts: %{name}-rt

%description
High performance WebAssembly Virtual Machine

%package rt
Summary: %{reponame} Runtime
Requires: spdlog
Provides: %{reponame}-rt = %{version}-%{release}
Provides: bundled(blake3) = 1.3.3
Provides: bundled(wasi-cpp-header) = 0.0.1
Provides: wasm-library
Conflicts: %{name}
RemovePathPostfixes: .rt

%description rt
This package contains only %{reponame} runtime without LLVM dependency.

%package devel
Summary: %{reponame} development files
Requires: %{name}%{?_isa} = %{version}-%{release}
Provides: %{reponame}-devel = %{version}-%{release}

%description devel
This package contains necessary header files for %{reponame} development.

%prep
%autosetup -n %{name}
[ -f VERSION ] || echo -n %{version} > VERSION

%build
%cmake -GNinja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF -DWASMEDGE_BUILD_TESTS=OFF -DLLVM_DIR=/usr/lib64/llvm18/lib/cmake/llvm
%cmake_build
mkdir rt
cd rt
%cmake -S .. -GNinja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF -DWASMEDGE_BUILD_TESTS=OFF -DWASMEDGE_BUILD_AOT_RUNTIME=OFF -DLLVM_DIR=/usr/lib64/llvm18/lib/cmake/llvm
%cmake_build

%install
cd rt
%cmake_install
mv %{buildroot}%{_bindir}/wasmedge{,.rt}
mv %{buildroot}%{_libdir}/lib%{name}.so.%{capi_version}{,.rt}
mv %{buildroot}%{_libdir}/lib%{name}.so.%{capi_soname}{,.rt}
mv %{buildroot}%{_libdir}/lib%{name}.so{,.rt}
rm -rf %{buildroot}%{_includedir}
cd ..
%cmake_install

%files
%license LICENSE LICENSE.spdx
%doc Changelog.md README.md SECURITY.md
%{_bindir}/wasmedge
%{_bindir}/wasmedgec
%{_libdir}/lib%{name}.so.%{capi_version}
%{_libdir}/lib%{name}.so.%{capi_soname}

%files rt
%license LICENSE LICENSE.spdx
%doc Changelog.md README.md SECURITY.md
%{_bindir}/wasmedge.rt
%{_libdir}/lib%{name}.so.%{capi_version}.rt
%{_libdir}/lib%{name}.so.%{capi_soname}.rt
%{_libdir}/lib%{name}.so.rt

%files devel
%dir %{_includedir}/%{name}
%{_includedir}/%{name}/enum.inc
%{_includedir}/%{name}/enum_configure.h
%{_includedir}/%{name}/enum_errcode.h
%{_includedir}/%{name}/enum_types.h
%{_includedir}/%{name}/int128.h
%{_includedir}/%{name}/version.h
%{_includedir}/%{name}/wasmedge.h
%{_libdir}/lib%{name}.so

%changelog
%autochangelog
