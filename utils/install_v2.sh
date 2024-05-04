#!/usr/bin/env bash

# This is the bootstrap Unix shell script for installing WasmEdge.
# It will detect the platform and architecture, download the corresponding
# WasmEdge release package, and install it to the specified path.

set -e

RED=$'\e[0;31m'
GREEN=$'\e[0;32m'
YELLOW=$'\e[0;33m'
NC=$'\e[0m' # No Color
TMP_DIR="/tmp/wasmedge.$$"

info() {
	command printf '\e[0;32mInfo\e[0m: %s\n\n' "$1"
}

warn() {
	command printf '\e[0;33mWarn\e[0m: %s\n\n' "$1"
}

error() {
	command printf '\e[0;31mError\e[0m: %s\n\n' "$1" 1>&2
}

eprintf() {
	command printf '%s\n' "$1" 1>&2
}

detect_cuda() {
	local cuda=""
	cuda=$(/usr/local/cuda/bin/nvcc --version 2>/dev/null | grep "Cuda compilation tools" | cut -f5 -d ' ' | cut -f1 -d ',')
	if [[ "${cuda}" =~ "12" ]]; then
		cuda="12"
	elif [[ "${cuda}" =~ "11" ]]; then
		cuda="11"
	else
		cuda=$(nvidia-smi -q 2>/dev/null | grep CUDA | cut -f2 -d ':' | cut -f2 -d ' ')
		if [[ "${cuda}" =~ "12" ]]; then
			cuda="12"
		elif [[ "${cuda}" =~ "11" ]]; then
			cuda="11"
		fi
	fi

	echo ${cuda}
}

_realpath() {
	[[ $1 = /* ]] && echo "$1" || echo "$PWD/${1#./}"
}

_downloader() {
	local url=$1
	if ! command -v curl &>/dev/null; then
		if ! command -v wget &>/dev/null; then
			error "Cannot find wget or curl"
			eprintf "Please install wget or curl"
			exit 1
		else
			wget -c --directory-prefix="$TMP_DIR" "$url"
		fi
	else
		pushd "$TMP_DIR"
		curl --progress-bar -L -OC0 "$url"
		popd
	fi
}

_extractor() {
	local prefix="$IPKG"
	if ! command -v tar &>/dev/null; then
		error "Cannot find tar"
		eprintf "Please install tar"
		exit_clean 1
	else
		local opt
		opt=$(tar "$@" 2>&1)
		for var in $opt; do
			local filtered=${var//$prefix/}
			filtered=${filtered//"lib64"/"lib"}
			if [[ "$filtered" =~ "x" ]]; then
				continue
			fi
			if [ ! -d "$IPATH/$filtered" ] ; then
				if [[ "$filtered" =~ "Plugin" ]] || [[ "$filtered" =~ "plugin" ]] || [[ "$filtered" =~ "ggml" ]]; then
					# Plugins installation is handled in install function
					continue
				fi
				if [[ "$2" =~ "lib" ]] && [[ ! "$IPATH/$filtered" =~ "/lib/" ]]; then
					echo "#$IPATH/lib/$filtered" >>"$IPATH/env"
					local _re_
					[[ "$OS" == "Linux" ]] && _re_='.[0-9]{1,2}.[0-9]{1,2}.[0-9]{1,2}$'
					[[ "$OS" == "Darwin" ]] && _re_='[0-9]{1,2}.[0-9]{1,2}.[0-9]{1,2}.'
					if [[ "$filtered" =~ $_re_ ]]; then
						local _f_ _f2_ _f3_ _f4_
						_f_=${filtered//$_re_/}
						_f2_=${filtered#$_f_}
						_f2_=${BASH_REMATCH[*]}

						IFS=. read -r var1 var2 <<<"$(if [[ "$filtered" =~ $_re_ ]]; then
						echo "${BASH_REMATCH[*]#.}"
						fi)"

						_f3_=${filtered//${_f2_}/}                                                  # libsome.so.xx.yy.zz --> libsome.so
						[[ "$OS" == "Linux" ]] && _f4_="$_f3_.$var1"                                # libsome.so.xx.yy.zz --> libsome.so.xx
						[[ "$OS" == "Darwin" ]] && _f4_="${filtered//.${_f2_}dylib/}"".$var1.dylib" # libsome.xx.yy.zz.dylib --> libsome.xx.dylib

						ln -sf "$IPATH/lib/$filtered" "$IPATH/lib/$_f3_"
						echo "#$IPATH/lib/$_f3_" >>"$IPATH/env"

						ln -sf "$IPATH/lib/$filtered" "$IPATH/lib/$_f4_"
						echo "#$IPATH/lib/$_f4_" >>"$IPATH/env"
					fi
				elif [[ "$2" =~ "bin" ]] && [[ ! "$IPATH/$filtered" =~ "/bin/" ]]; then
					echo "#$IPATH/bin/$filtered" >>"$IPATH/env"
				else
					echo "#$IPATH/$filtered" >>"$IPATH/env"
				fi
			fi
		done
	fi
}

if [ "$__HOME__" = "" ]; then
	__HOME__="$HOME"
fi

get_latest_release() {
	echo "0.13.5"
}

VERSION=$(get_latest_release)

check_os_arch() {
	[ -z "${ARCH}" ] && ARCH=$(uname -m)
	[ -z "${OS}" ] && OS=$(uname)
	RELEASE_PKG="ubuntu20.04_x86_64.tar.gz"
	IPKG="WasmEdge-${VERSION}-${OS}"
	_LD_LIBRARY_PATH_="LD_LIBRARY_PATH"

	case ${OS} in
		'Linux')
			case ${ARCH} in
				'x86_64') ARCH="x86_64";;
				'arm64' | 'armv8*' | 'aarch64') ARCH="aarch64" ;;
				'amd64') ARCH="x86_64" ;;
				*)
					error "Detected ${OS}-${ARCH} - currently unsupported"
					eprintf "Use --os and --arch to specify the OS and ARCH"
					exit 1
					;;
			esac
			if [ "${LEGACY}" == 1 ]; then
				RELEASE_PKG="manylinux2014_${ARCH}.tar.gz"
			else
				RELEASE_PKG="ubuntu20.04_${ARCH}.tar.gz"
			fi
			_LD_LIBRARY_PATH_="LD_LIBRARY_PATH"

			;;
		'Darwin')
			case ${ARCH} in
				'x86_64') ARCH="x86_64" ;;
				'arm64' | 'arm' | 'aarch64') ARCH="arm64" ;;
				*)
					error "Detected ${OS}-${ARCH} - currently unsupported"
					eprintf "Use --os and --arch to specify the OS and ARCH"
					exit 1
					;;
			esac
			RELEASE_PKG="darwin_${ARCH}.tar.gz"
			_LD_LIBRARY_PATH_="DYLD_LIBRARY_PATH"

			;;
		*)
			error "Detected ${OS}-${ARCH} - currently unsupported"
			eprintf "Use --os and --arch to specify the OS and ARCH"
			exit 1
			;;
	esac

	info "Detected ${OS}-${ARCH}"
}

IPATH="$__HOME__/.wasmedge"
VERBOSE=0
LEGACY=0
ENABLE_RUSTLS=0
ENABLE_NOAVX=0

set_ENV() {
	ENV="#!/bin/sh
	# wasmedge shell setup
	# affix colons on either side of \$PATH to simplify matching
	case ":\"\${PATH}\":" in
		*:\"$1/bin\":*)
			;;
		*)
			# Prepending path in case a system-installed wasmedge needs to be overridden
			if [ -n \"\${PATH}\" ]; then
				export PATH=\"$1/bin\":\$PATH
			else
				export PATH=\"$1/bin\"
		fi
		;;
esac
case ":\"\${"$_LD_LIBRARY_PATH_"}\":" in
	*:\"$1/lib\":*)
		;;
	*)
		# Prepending path in case a system-installed wasmedge libs needs to be overridden
		if [ -n \"\${"$_LD_LIBRARY_PATH_"}\" ]; then
			export $_LD_LIBRARY_PATH_=\"$1/lib\":\$$_LD_LIBRARY_PATH_
		else
			export $_LD_LIBRARY_PATH_=\"$1/lib\"
		fi
		;;
esac
case ":\"\${"LIBRARY_PATH"}\":" in
	*:\"$1/lib\":*)
		;;
	*)
		if [ -n \"\${LIBRARY_PATH}\" ]; then
			export LIBRARY_PATH=\"$1/lib\":\$LIBRARY_PATH
		else
			export LIBRARY_PATH=\"$1/lib\"
		fi
		;;
esac
case ":\"\${"C_INCLUDE_PATH"}\":" in
	*:\"$1/include\":*)
		;;
	*)
		if [ -n \"\${C_INCLUDE_PATH}\" ]; then
			export C_INCLUDE_PATH=\"$1/include\":\$C_INCLUDE_PATH
		else
			export C_INCLUDE_PATH=\"$1/include\"
		fi
		;;
esac
case ":\"\${"CPLUS_INCLUDE_PATH"}\":" in
	*:\"$1/include\":*)
		;;
	*)
		if [ -n \"\${CPLUS_INCLUDE_PATH}\" ]; then
			export CPLUS_INCLUDE_PATH=\"$1/include\":\$CPLUS_INCLUDE_PATH
		else
			export CPLUS_INCLUDE_PATH=\"$1/include\"
		fi
		;;
esac"
}

usage() {
	cat <<EOF
	Usage: $0 -p </path/to/install> [-V]
	WasmEdge installation.
	Mandatory arguments to long options are mandatory for short options too.
	Long options should be assigned with '='

	-h,             --help                      Display help

	--legacy                                    Enable legacy OS support.
													E.g., CentOS 7.

	-v,             --version                   Install the specific version.

	-V,             --verbose                   Run script in verbose mode.
													Will print out each step
													of execution.

	-p,             --path=[/usr/local]         Prefix / Path to install

	--noavx                                     Install the GGML noavx plugin.
													Default is disabled.

	--rustls                                    Install the Rustls plugin.
													Default is disabled.

	--os=[Linux/Darwin]                         Set the OS.
													Default is detected OS.

	--arch[x86_64/aarch64/arm64]                Set the ARCH.
													Default is detected ARCH.

	Example:
	./$0 -p $IPATH --verbose

	Or
	./$0 --path=/usr/local --verbose

	About:

	- wasmedge is the runtime that executes the wasm program or the AOT compiled
	  shared library format or universal wasm format programs.

EOF
}

on_exit() {
	cat <<EOF
${RED}
	Please see --help
	If issue persists make a trace using -V and submit it to
https://github.com/WasmEdge/WasmEdge/issues/new?assignees=&labels=&template=bug_report.md
${NC}
EOF
}

exit_clean() {
	trap - EXIT
	exit "$1"
}

make_dirs() {
	for var in "$@"; do
		if [ ! -d "$IPATH/$var" ]; then
			mkdir -p "$IPATH/$var"
		fi
	done
}

cleanup() {
	rm -f "${TMP_DIR}/WasmEdge-${VERSION}-${RELEASE_PKG}"
	rm -rf "${TMP_DIR}/WasmEdge-${VERSION}-${OS}"
}

install() {
	local dir=$1
	shift
	for var in "$@"; do
		if [ "$var" = "lib" ]; then
			if [ -d "$TMP_DIR/$dir"/lib64 ]; then
				cp -rf "$TMP_DIR/$dir"/lib64/* "$IPATH/$var"
			else
				cp -rf "$TMP_DIR/$dir"/lib/* "$IPATH/$var"
			fi
		elif [ "$var" = "plugin" ]; then
			if [ -d "$TMP_DIR/$dir"/plugin ]; then
				if [[ ! $IPATH =~ ^"/usr" ]]; then
					cp -rf "$TMP_DIR/$dir"/plugin/* "$IPATH/plugin"
				else
					cp -rf "$TMP_DIR/$dir"/plugin/* "$IPATH/lib"
				fi
				for _file_ in "$IPATH/$dir"/plugin/*; do
					if [[ "$_file_" =~ "Plugin" ]] || [[ "$_file_" =~ "plugin" ]] || [[ "$_file_" =~ "ggml" ]]; then
						local _plugin_name_=${_file_##*/}
						if [[ "$IPATH" =~ ^"/usr" ]]; then
							echo "#$_file_" >>"$IPATH/env"
						else
							echo "#$IPATH/plugin/$_plugin_name_" >>"$IPATH/env"
						fi
					fi
				done
			fi
		else
			cp -rf "$TMP_DIR/$dir/$var"/* "$IPATH/$var"
		fi
	done
}

get_wasmedge_release() {
	info "Fetching WasmEdge-$VERSION"
	_downloader "https://github.com/WasmEdge/WasmEdge/releases/download/$VERSION/WasmEdge-$VERSION-$RELEASE_PKG"
	_extractor -C "${TMP_DIR}" -vxzf "$TMP_DIR/WasmEdge-$VERSION-$RELEASE_PKG"
}

get_wasmedge_ggml_plugin() {
	info "Fetching WasmEdge-GGML-Plugin"
	local CUDA_EXT=""
	local NOAVX_EXT=""
	if [ "${ENABLE_NOAVX}" == "1" ]; then
		# If noavx is given, it will only use CPU with noavx instructions.
		info "NOAVX option is given: Use the noavx CPU version."
		NOAVX_EXT="-noavx"
	else
		cuda=$(detect_cuda)
		info "Detected CUDA version: ${cuda}"
		if [ "${cuda}" == "12" ]; then
			CUDA_EXT="-cuda"
		elif [ "${cuda}" == "11" ]; then
			if [ "${ARCH}" == "aarch64" ]; then
				CUDA_EXT="-cuda"
			else
				CUDA_EXT="-cuda-11"
			fi
		else
			CUDA_EXT=""
		fi
	fi
	_downloader "https://github.com/WasmEdge/WasmEdge/releases/download/$VERSION/WasmEdge-plugin-wasi_nn-ggml${CUDA_EXT}${NOAVX_EXT}-$VERSION-$RELEASE_PKG"
	local TMP_PLUGIN_DIR="${TMP_DIR}/${IPKG}/plugin"
	mkdir -p "${TMP_PLUGIN_DIR}"
	_extractor -C "${TMP_PLUGIN_DIR}" -vxzf "${TMP_DIR}/WasmEdge-plugin-wasi_nn-ggml${CUDA_EXT}${NOAVX_EXT}-${VERSION}-${RELEASE_PKG}"
}

get_wasmedge_rustls_plugin() {
	info "Fetching WasmEdge-Rustls-Plugin"
	_downloader "https://github.com/WasmEdge/WasmEdge/releases/download/$VERSION/WasmEdge-plugin-wasmedge_rustls-$VERSION-$RELEASE_PKG"
	local TMP_PLUGIN_DIR="${TMP_DIR}/${IPKG}/plugin"
	mkdir -p "${TMP_PLUGIN_DIR}"
	_extractor -C "${TMP_PLUGIN_DIR}" -vxzf "${TMP_DIR}/WasmEdge-plugin-wasmedge_rustls-${VERSION}-${RELEASE_PKG}"
}

wasmedge_checks() {
	if [ "${ARCH}" == $(uname -m) ] && [ "${OS}" == $(uname) ] ; then
		# Check only MAJOR.MINOR.PATCH
		local version=$1
		shift
		for var in "$@"; do
			if [ "$var" == "" ]; then
				continue
			fi
			local V=$("$IPATH/bin/$var" --version | sed 's/^.*[^0-9]\([0-9]*\.[0-9]*\.[0-9]*\).*$/\1/')
			local V_=$(echo $version | sed 's/\([0-9]*\.[0-9]*\.[0-9]*\).*$/\1/')
			if [ "$V" = "$V_" ]; then
				echo "${GREEN}Installation of $var-$version successful${NC}"
			else
				echo "${YELLOW}version $V_ does not match $V for $var-$version${NC}"
				exit 1
			fi
		done
	fi
	# Bypass if cross compile
}

main() {

	trap on_exit EXIT

	# getopt is in the util-linux package,
	# it'll probably be fine, but it's of course a good thing to keep in mind.

	local OPTIND
	while getopts "e:hp:v:r:u:V-:" OPT; do
		# support long options: https://stackoverflow.com/a/28466267/519360
		if [ "$OPT" = "-" ]; then   # long option: reformulate OPT and OPTARG
			OPT="${OPTARG%%=*}"     # extract long option name
			OPTARG="${OPTARG#$OPT}" # extract long option argument (may be empty)
			OPTARG="${OPTARG#=}"    # if long option argument, remove assigning `=`
		fi
		case "$OPT" in
			h | help)
				usage
				trap - EXIT
				exit 0
				;;
			l | legacy)
				LEGACY=1
				;;
			v | version)
				VERSION="${OPTARG}"
				;;
			V | verbose)
				VERBOSE=1
				;;
			p | path)
				IPATH="$(_realpath "${OPTARG}")"
				;;
			noavx)
				ENABLE_NOAVX=1
				;;
			rustls)
				ENABLE_RUSTLS=1
				;;
			os)
				OS="${OPTARG^}"
				;;
			arch)
				ARCH="${OPTARG}"
				;;
			?)
				exit 2
				;;
			??*)
				error "Illegal option -- ${OPTARG}"
				exit 1
				;;
			*)
				error "Unknown error"
				eprintf "please raise an issue on GitHub with the command you ran."
				exit 1
				;;
		esac
	done

	shift $((OPTIND - 1)) # remove parsed options and args from $@ list

	if [ ! $VERBOSE == 0 ]; then
		echo "Verbose Mode"
		set -xv
	fi

	check_os_arch

	# Run the uninstaller to remove any previous installations
	if [ -f "$IPATH/bin/wasmedge" ]; then
		bash <(curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/uninstall.sh) -p "$IPATH" -q
	fi

	set_ENV "$IPATH"
	mkdir -p "$IPATH"
	mkdir -p "$TMP_DIR"
	# Setup the plugin folder if the installation path is not in the system path
	[[ "$IPATH" =~ ^"/usr" ]] || mkdir -p "$IPATH/plugin"

	echo "$ENV" >"$IPATH/env"
	echo "# Please do not edit comments below this for uninstallation purpose" >> "$IPATH/env"

	local _source="source \"$IPATH/env\""
	local _grep=$(cat "$__HOME__/.profile" 2>/dev/null | grep "$IPATH/env")
	if [ "$_grep" = "" ]; then
		[ -f "$__HOME__/.profile" ] && echo "$_source" >>"$__HOME__/.profile"
	fi

	local _shell_ _shell_rc
	_shell_="${SHELL#${SHELL%/*}/}"
	_shell_rc=".""$_shell_""rc"

	if [[ "$_shell_" =~ "zsh" ]]; then
		local _grep=$(cat "$__HOME__/.zprofile" 2>/dev/null | grep "$IPATH/env")
		if [ "$_grep" = "" ]; then
			[ -f "$__HOME__/.zprofile" ] && echo "$_source" >>"$__HOME__/.zprofile"
		fi
	elif [[ "$_shell_" =~ "bash" ]]; then
		local _grep=$(cat "$__HOME__/.bash_profile" 2>/dev/null | grep "$IPATH/env")
		if [ "$_grep" = "" ]; then
			[ -f "$__HOME__/.bash_profile" ] && echo "$_source" >>"$__HOME__/.bash_profile"
		fi
	fi

	local _grep=$(cat "$__HOME__/$_shell_rc" | grep "$IPATH/env")
	if [ "$_grep" = "" ]; then
		[ -f "$__HOME__/$_shell_rc" ] && echo "$_source" >>"$__HOME__/$_shell_rc"
	fi

	if [ -d "$IPATH" ]; then
		info "WasmEdge Installation at $IPATH"
		make_dirs "include" "lib" "bin"

		get_wasmedge_release
		get_wasmedge_ggml_plugin

		if [ "${ENABLE_RUSTLS}" == 1 ]; then
			get_wasmedge_rustls_plugin
		fi

		install "$IPKG" "include" "lib" "bin" "plugin"
		wasmedge_checks "$VERSION" "wasmedge"
	else
		error "Installation path invalid"
		eprintf "Please provide a valid path"
		exit 1
	fi

	trap - EXIT
	cleanup
	end_message
}

end_message() {
	case ":${PATH}:" in
		*:"${IPATH%"/"}/bin":*)
			echo "${GREEN}WasmEdge binaries accessible${NC}"
			;;
		*)
			echo "${GREEN}source $IPATH/env${NC} to use wasmedge binaries"
			;;
	esac
}

main "$@"
