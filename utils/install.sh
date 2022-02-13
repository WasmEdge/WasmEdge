#!/bin/bash
set -e

RED=$'\e[0;31m'
GREEN=$'\e[0;32m'
YELLOW=$'\e[0;33m'
NC=$'\e[0m' # No Color
PERM_ROOT=1
TMP_DIR="/tmp/wasmedge.$$"
_LD_LIBRARY_PATH_="LD_LIBRARY_PATH"

if [[ $EUID -ne 0 ]]; then
    PERM_ROOT=0
fi

_ldconfig() {
    if [ $PERM_ROOT == 1 ]; then
        if command -v ldconfig &>/dev/null; then
            ldconfig "$IPATH/lib"
        elif command -v update_dyld_shared_cache &>/dev/null; then
            update_dyld_shared_cache
        fi
    fi
}

_downloader() {
    local url=$1
    if ! command -v curl &>/dev/null; then
        if ! command -v wget &>/dev/null; then
            echo "${RED}Please install wget or curl${NC}"
            exit 1
        else
            wget --help | grep -q '\--show-progress' &&
                _PROGRESS_OPT="--show-progress" || _PROGRESS_OPT=""

            wget q -c --directory-prefix="$TMP_DIR" "$_PROGRESS_OPT " "$url"
        fi
    else
        pushd "$TMP_DIR"
        curl -L -OC0 "$url" --progress-bar
        popd
    fi
}

_extractor() {
    local prefix="$IPKG"
    if ! command -v tar &>/dev/null; then
        echo "${RED}Please install tar${NC}"
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
            if [ ! -d "$IPATH/$filtered" ] && [[ ! "$filtered" =~ "download_dependencies" ]]; then
                if [[ "$2" =~ "lib" ]] && [[ ! "$IPATH/$filtered" =~ "/lib/" ]]; then
                    echo "#$IPATH/lib/$filtered" >>"$IPATH/env"
                    local _re_
                    _re_='.[0-9]{1,2}.[0-9]{1,2}.[0-9]{1,2}'
                    if [[ "$filtered" =~ $_re_$ ]]; then
                        local _f_ _f2_ _f3_ _f4_
                        _f_=${filtered//$_re_/}
                        _f2_=${filtered#$_f_}
                        _f2_=${BASH_REMATCH[*]}

                        IFS=. read -r var1 var2 <<<"$(if [[ "$filtered" =~ $_re_$ ]]; then
                            echo "${BASH_REMATCH[*]#.}"
                        fi)"

                        _f3_=${filtered//${_f2_}/} # libsome.so.xx.yy.zz --> libsome.so
                        _f4_="$_f3_.$var1"         # libsome.so.xx.yy.zz --> libsome.so.xx

                        ln -sf "$IPATH/lib/$filtered" "$IPATH/lib/$_f3_"
                        echo "#$IPATH/lib/$_f3_" >>"$IPATH/env"

                        ln -sf "$IPATH/lib/$filtered" "$IPATH/lib/$_f4_"
                        echo "#$IPATH/lib/$_f4_" >>"$IPATH/env"

                        # special case: libpng16.so.16.37.0 ---> libpng.so
                        if [[ "$filtered" =~ "libpng16.so.16.37.0" ]]; then
                            ln -sf "$IPATH/lib/$filtered" "$IPATH/lib/libpng.so"
                            echo "#$IPATH/lib/libpng.so" >>"$IPATH/env"
                        fi
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

if ! command -v git &>/dev/null; then
    echo "${RED}Please install git${NC}"
    exit 1
fi

if command -v sudo &>/dev/null; then
    if [ $PERM_ROOT == 1 ]; then
        if command -v getent &>/dev/null; then
            __HOME__=$(getent passwd "$SUDO_USER" | cut -d: -f6)
        fi
    fi
fi

if [ "$__HOME__" = "" ]; then
    __HOME__="$HOME"
fi

get_latest_release() {
    local res
    res=$(git ls-remote --refs --tags "https://github.com/$1.git" |
        cut -d '/' -f 3 |
        sort --version-sort |
        grep -e '^[0-9].[0-9].[0-9]$' |
        tail -1)
    echo "$res"
}

remote_version_availabilty() {
    # $1 repo , $2 version
    res=$(git ls-remote --refs --tags "https://github.com/$1.git" |
        cut -d '/' -f 3 |
        sort --version-sort)

    if [[ ! "$res" == *"$2"* ]]; then
        echo "${RED}$2 for $1 does not exist${NC}"
        exit 1
    fi

}

VERSION=$(get_latest_release WasmEdge/WasmEdge)
VERSION_IM=$(get_latest_release second-state/WasmEdge-image)
VERSION_IM_DEPS=$(get_latest_release second-state/WasmEdge-image)
VERSION_TF=$(get_latest_release second-state/WasmEdge-tensorflow)
VERSION_TF_DEPS=$(get_latest_release second-state/WasmEdge-tensorflow-deps)
VERSION_TF_TOOLS=$(get_latest_release second-state/WasmEdge-tensorflow-tools)

detect_os_arch() {
    RELEASE_PKG="manylinux2014_x86_64.tar.gz"
    IM_DEPS_RELEASE_PKG="manylinux1_x86_64.tar.gz"
    ARCH=$(uname -m)
    OS=$(uname)
    IM_EXT_COMPAT=1
    TF_EXT_COMPAT=1
    IPKG="WasmEdge-$VERSION-Linux"

    case $OS in
    'Linux')
        case $ARCH in
        'x86_64') ;;
        'arm64' | 'armv8*' | "aarch64") RELEASE_PKG="manylinux2014_aarch64.tar.gz" ;;
        "amd64") RELEASE_PKG="manylinux2014_amd64.tar.gz" ;;
        *)
            echo "${RED}Detected $OS-$ARCH${NC} - currently unsupported${NC}"
            exit 1
            ;;
        esac
        ;;
    'Darwin')
        case $ARCH in
        'x86_64') ;;
        'arm64') ;;
        'arm')
            ARCH="arm64"
            ;;
        *)
            echo "${RED}Detected $OS-$ARCH${NC} - currently unsupported${NC}"
            exit 1
            ;;
        esac
        _LD_LIBRARY_PATH_="DYLD_LIBRARY_PATH"
        IPKG="WasmEdge-$VERSION-Darwin"
        RELEASE_PKG="darwin_$ARCH.tar.gz"
        IM_EXT_COMPAT=0
        TF_EXT_COMPAT=0

        if ! command -v brew &>/dev/null; then
            echo "${RED}Brew is required${NC}"
            exit 1
        else
            if [ "$(brew list | grep llvm)" = "" ]; then
                echo "${YELLOW}Please run: brew install llvm${NC}"
                exit 1
            fi
        fi

        ;;
    *)
        echo "${RED}Detected $OS-$ARCH${NC} - currently unsupported${NC}"
        exit 1
        ;;
    esac

    echo "Detected $OS-$ARCH"
}

IPATH="$__HOME__/.wasmedge"
EXT="none"
VERBOSE=0

set_ENV() {
    ENV="#!/bin/sh
# wasmedge shell setup
# affix colons on either side of \$PATH to simplify matching
case ":\"\${PATH}\":" in
    *:\"$1/bin\":*)
        ;;
    *)
        # Prepending path in case a system-installed wasmedge needs to be overridden
        export PATH=\"$1/bin\":\$PATH
        ;;
esac
case ":\"\${"$_LD_LIBRARY_PATH_"}\":" in
    *:\"$1/lib\":*)
        ;;
    *)
        # Prepending path in case a system-installed wasmedge libs needs to be overridden
        export $_LD_LIBRARY_PATH_=\"$1/lib\":\$$_LD_LIBRARY_PATH_
        ;;
esac
case ":\"\${"LIBRARY_PATH"}\":" in
    *:\"$1/lib\":*)
        ;;
    *)
        export LIBRARY_PATH=\"$1/lib\":\$LIBRARY_PATH
        ;;
esac
case ":\"\${"C_INCLUDE_PATH"}\":" in
    *:\"$1/include\":*)
        ;;
    *)
        export C_INCLUDE_PATH=\"$1/include\":\$C_INCLUDE_PATH
        ;;
esac
case ":\"\${"CPLUS_INCLUDE_PATH"}\":" in
    *:\"$1/include\":*)
        ;;
    *)
        export CPLUS_INCLUDE_PATH=\"$1/include\":\$CPLUS_INCLUDE_PATH
        ;;
esac"
}

usage() {
    cat <<EOF
    Usage: $0 -p </path/to/install> [-V]
    WasmEdge installation, uninstallation and extensions install.
    Mandatory arguments to long options are mandatory for short options too.
    Long options should be assingned with '='

    -h,             --help                      Display help

    -p,             --path=[/usr/local]         Prefix / Path to install

    -v,             --version=VERSION           Set and Download specific 
                                                    version of WasmEdge
                        
                    --tf-version=VERSION_TF
                    --tf-deps-version=VERSION_TF_DEPS
                    --tf-tools-version=VERSION_TF_TOOLS
                    --image-version=VERSION_IM
                    --image-deps-version=VERSION_IM_DEPS

    -e,             --extension=[tf|image|all|none]  
                                                    Enable extension support 
                                                    i.e Tensorflow (tf) 
                                                        or Image (image)

    -V,             --verbose                   Run script in verbose mode.
                                                    Will print out each step 
                                                    of execution.
    
    -r              --remove-old=[yes|no]       Run Uninstallation script by 
                                                default. Specify \`no\` if you
                                                wish not to. 

    Example:
    ./$0 -p $IPATH -e all -v $VERSION --verbose
    
    Or
    ./$0 -p $IPATH --extension=all --path=/usr/local --verbose
    ./$0 -p $IPATH --extension=tf,image --path=/usr/local --verbose
    ./$0 -p $IPATH -e tensorflow,image --path=/usr/local --verbose
    ./$0 -p $IPATH -e tensorflow,image --path=/usr/local --verbose -r no
    
    About:

    - wasmedgec is the AOT compiler that compiles WebAssembly bytecode programs 
      (wasm programs) into native code (so program) on your deployment machine.
    
    - wasmedge is the runtime that executes the wasm program or the AOT compiled
      so program.

    - wasmedgec-tensorflow is the AOT compiler that compiles WebAssembly 
      bytecode programs (wasm programs) into native code (so program) on your 
      deployment machine. It is aware of WamsEdge's Tensorflow extension API.
    
    - wasmedge-tensorflow-lite is the runtime that executes the wasm program or 
      the AOT compiled so program with the Tensorflow Lite library.

EOF
}

on_exit() {
    cat <<EOF
${RED}
    Please see --help
    If issue perists make a trace using -V and submit it to
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
    rm -f "$TMP_DIR/WasmEdge-$VERSION-$RELEASE_PKG"
    rm -rf "$TMP_DIR/WasmEdge-$VERSION-Linux"
    rm -f "$TMP_DIR/WasmEdge-image-deps-$VERSION_IM-$IM_DEPS_RELEASE_PKG"
    rm -f "$TMP_DIR/WasmEdge-image-$VERSION_IM-$RELEASE_PKG"
    rm -f "$TMP_DIR/WasmEdge-tensorflow-deps-TF-$VERSION_TF_DEPS-$RELEASE_PKG"
    rm -f "$TMP_DIR/WasmEdge-tensorflow-deps-TFLite-$VERSION_TF_DEPS-$RELEASE_PKG"
    rm -f "$TMP_DIR/WasmEdge-tensorflow-$VERSION_TF-$RELEASE_PKG"
    rm -f "$TMP_DIR/WasmEdge-tensorflowlite-$VERSION_TF-$RELEASE_PKG"
    rm -f "$TMP_DIR/WasmEdge-tensorflow-tools-$VERSION_TF_TOOLS-$RELEASE_PKG"
}

install() {
    local dir=$1
    shift
    for var in "$@"; do
        echo "${GREEN}Installing $dir in $IPATH/$var ${NC}"
        if [ "$var" = "lib" ]; then
            if [ -d "$TMP_DIR/$dir"/lib64 ]; then
                cp -rf "$TMP_DIR/$dir"/lib64/* "$IPATH/$var"
            else
                cp -rf "$TMP_DIR/$dir"/lib/* "$IPATH/$var"
            fi
        else
            cp -rf "$TMP_DIR/$dir/$var"/* "$IPATH/$var"
        fi
    done
}

get_wasmedge_release() {
    echo "Fetching WasmEdge-$VERSION"
    _downloader "https://github.com/WasmEdge/WasmEdge/releases/download/$VERSION/WasmEdge-$VERSION-$RELEASE_PKG"
    _extractor -C "$TMP_DIR" -vxzf "$TMP_DIR/WasmEdge-$VERSION-$RELEASE_PKG"
}

wasmedge_post_install() {
    _ldconfig
}

wasmedge_checks() {
    # Check only MAJOR.MINOR.PATCH
    if [ $PERM_ROOT == 1 ]; then
        local version=$1
        shift
        for var in "$@"; do
            if [ "$var" == "" ]; then
                continue
            fi
            local V=$("$IPATH/bin/$var" --version | sed 's/^.*[^0-9]\([0-9]*\.[0-9]*\.[0-9]*\).*$/\1/')
            local V_=$(echo $version | sed 's/\([0-9]*\.[0-9]*\.[0-9]*\).*$/\1/')
            if [ "$V" = "$V_" ]; then
                echo "${GREEN}Installation of $var-$version successfull${NC}"
            else
                echo "${YELLOW}version $V_ does not match $V for $var-$version${NC}"
                exit 1
            fi
        done
    fi
}

get_wasmedge_image_deps() {
    echo "Fetching WasmEdge-image-deps-$VERSION_IM"
    _downloader "https://github.com/second-state/WasmEdge-image/releases/download/$VERSION_IM_DEPS/WasmEdge-image-deps-$VERSION_IM_DEPS-$IM_DEPS_RELEASE_PKG"

    _extractor -C "$IPATH/lib" -vxzf "$TMP_DIR/WasmEdge-image-deps-$VERSION_IM_DEPS-$IM_DEPS_RELEASE_PKG"

    _ldconfig
}

install_wasmedge_image() {
    echo "Fetching WasmEdge-image-$VERSION_IM"
    _downloader "https://github.com/second-state/WasmEdge-image/releases/download/$VERSION_IM/WasmEdge-image-$VERSION_IM-$RELEASE_PKG"
    _extractor -C "$IPATH" -vxzf "$TMP_DIR/WasmEdge-image-$VERSION_IM-$RELEASE_PKG"
    _ldconfig
}

get_wasmedge_tensorflow_deps() {
    [[ "$RELEASE_PKG" =~ "aarch64" ]] && echo "Tensorflow not supported" || echo "Fetching WasmEdge-tensorflow-deps-TF-$VERSION_TF_DEPS"
    _downloader "https://github.com/second-state/WasmEdge-tensorflow-deps/releases/download/$VERSION_TF_DEPS/WasmEdge-tensorflow-deps-TF-$VERSION_TF_DEPS-$RELEASE_PKG"

    echo "Fetching WasmEdge-tensorflow-deps-TFLite-$VERSION_TF_DEPS"
    _downloader "https://github.com/second-state/WasmEdge-tensorflow-deps/releases/download/$VERSION_TF_DEPS/WasmEdge-tensorflow-deps-TFLite-$VERSION_TF_DEPS-$RELEASE_PKG"

    [[ "$RELEASE_PKG" =~ "aarch64" ]] || _extractor -C "$IPATH/lib" -vxzf "$TMP_DIR/WasmEdge-tensorflow-deps-TF-$VERSION_TF_DEPS-$RELEASE_PKG"
    _extractor -C "$IPATH/lib" -vxzf "$TMP_DIR/WasmEdge-tensorflow-deps-TFLite-$VERSION_TF_DEPS-$RELEASE_PKG"

    _ldconfig
}

install_wasmedge_tensorflow() {
    [[ "$RELEASE_PKG" =~ "aarch64" ]] && echo "Tensorflow not supported" || echo "Fetching WasmEdge-tensorflow-$VERSION_TF" &&
        _downloader "https://github.com/second-state/WasmEdge-tensorflow/releases/download/$VERSION_TF/WasmEdge-tensorflow-$VERSION_TF-$RELEASE_PKG"

    echo "Fetching WasmEdge-tensorflowlite-$VERSION_TF"
    _downloader "https://github.com/second-state/WasmEdge-tensorflow/releases/download/$VERSION_TF/WasmEdge-tensorflowlite-$VERSION_TF-$RELEASE_PKG"

    echo "Fetching WasmEdge-tensorflow-tools-$VERSION_TF_TOOLS"
    _downloader "https://github.com/second-state/WasmEdge-tensorflow-tools/releases/download/$VERSION_TF_TOOLS/WasmEdge-tensorflow-tools-$VERSION_TF_TOOLS-$RELEASE_PKG"

    [[ "$RELEASE_PKG" =~ "aarch64" ]] || _extractor -C "$IPATH" -vxzf "$TMP_DIR/WasmEdge-tensorflow-$VERSION_TF-$RELEASE_PKG"
    _extractor -C "$IPATH" -vxzf "$TMP_DIR/WasmEdge-tensorflowlite-$VERSION_TF-$RELEASE_PKG"
    _extractor -C "$IPATH/bin" -vxzf "$TMP_DIR/WasmEdge-tensorflow-tools-$VERSION_TF_TOOLS-$RELEASE_PKG"

    rm -f "$IPATH/bin/download_dependencies_all.sh" \
        "$IPATH/bin/download_dependencies_tf.sh" \
        "$IPATH/bin/download_dependencies_tflite.sh"

    _ldconfig
}

install_image_extensions() {
    [[ "$RELEASE_PKG" =~ "aarch64" ]] &&
        [ "$(printf %s\\n%s\\n "0.9.1-beta.1" "$VERSION_IM_DEPS")" != "$(printf %s\\n%s "0.9.1-beta.1" "$VERSION_IM_DEPS" | sort --version-sort)" ] &&
        IM_EXT_COMPAT=0

    if [ $IM_EXT_COMPAT == 1 ]; then

        [ "$EXT_V_SET_WASMEDGE_IM" -eq 0 ] && VERSION_IM=$VERSION &&
            remote_version_availabilty second-state/WasmEdge-image "$VERSION_IM"

        [ "$EXT_V_SET_WASMEDGE_IM_DEPS" -eq 0 ] && VERSION_IM_DEPS=$VERSION

        [ "$(printf %s\\n%s\\n "$VERSION_IM_DEPS" "0.8.2")" == "$(printf %s\\n%s "$VERSION_IM_DEPS" "0.8.2" | sort --version-sort)" ] &&
            remote_version_availabilty second-state/WasmEdge-image "$VERSION_IM_DEPS" &&
            get_wasmedge_image_deps

        install_wasmedge_image
    else
        echo "${YELLOW}Image Extensions not supported${NC}"
    fi
}

install_tf_extensions() {
    [[ "$RELEASE_PKG" =~ "aarch64" ]] &&
        [ "$(printf %s\\n%s\\n "0.9.1-beta.1" "$VERSION_TF_DEPS")" != "$(printf %s\\n%s "0.9.1-beta.1" "$VERSION_TF_DEPS" | sort --version-sort)" ] &&
        TF_EXT_COMPAT=0

    if [ $TF_EXT_COMPAT == 1 ]; then

        [ "$EXT_V_SET_WASMEDGE_TF" -eq 0 ] && VERSION_TF=$VERSION &&
            remote_version_availabilty second-state/WasmEdge-tensorflow "$VERSION_TF"

        [ "$EXT_V_SET_WASMEDGE_TF_DEPS" -eq 0 ] && VERSION_TF_DEPS=$VERSION &&
            remote_version_availabilty second-state/WasmEdge-tensorflow-deps "$VERSION_TF_DEPS"

        [ "$EXT_V_SET_WASMEDGE_TF_TOOLS" -eq 0 ] && VERSION_TF_TOOLS=$VERSION &&
            remote_version_availabilty second-state/WasmEdge-tensorflow-tools "$VERSION_TF_TOOLS"

        get_wasmedge_tensorflow_deps
        install_wasmedge_tensorflow

        local _bin
        [[ "$RELEASE_PKG" =~ "aarch64" ]] && _bin="" || _bin="wasmedge-tensorflow"
        wasmedge_checks "$VERSION_TF_TOOLS" "$_bin" \
            wasmedge-tensorflow-lite
    else
        echo "${YELLOW}Tensorflow extensions not supported${NC}"
    fi
}

main() {

    trap on_exit EXIT

    # getopt is in the util-linux package,
    # it'll probably be fine, but it's of course a good thing to keep in mind.

    default=0
    EXT_V_SET_WASMEDGE=0
    EXT_V_SET_WASMEDGE_IM=0
    EXT_V_SET_WASMEDGE_IM_DEPS=0
    EXT_V_SET_WASMEDGE_TF=0
    EXT_V_SET_WASMEDGE_TF_DEPS=0
    EXT_V_SET_WASMEDGE_TF_TOOLS=0

    REMOVE_OLD=1

    local OPTIND
    while getopts "e:hp:v:r:V-:" OPT; do
        # support long options: https://stackoverflow.com/a/28466267/519360
        if [ "$OPT" = "-" ]; then   # long option: reformulate OPT and OPTARG
            OPT="${OPTARG%%=*}"     # extract long option name
            OPTARG="${OPTARG#$OPT}" # extract long option argument (may be empty)
            OPTARG="${OPTARG#=}"    # if long option argument, remove assigning `=`
        fi
        case "$OPT" in
        e | extension)
            EXT="${OPTARG}"
            ;;
        h | help)
            usage
            trap - EXIT
            exit 0
            ;;
        v | version)
            VERSION="${OPTARG}"
            EXT_V_SET_WASMEDGE=1
            ;;
        V | verbose)
            VERBOSE=1
            ;;
        p | path)
            IPATH="${OPTARG}"
            default=1
            ;;
        r | remove-old)
            REMOVE_OLD="${OPTARG}"
            ;;
        tf-version)
            VERSION_TF="${OPTARG}"
            EXT_V_SET_WASMEDGE_TF=1
            ;;
        tf-deps-version)
            VERSION_TF_DEPS="${OPTARG}"
            EXT_V_SET_WASMEDGE_TF_DEPS=1
            ;;
        tf-tools-version)
            VERSION_TF_TOOLS="${OPTARG}"
            EXT_V_SET_WASMEDGE_TF_TOOLS=1
            ;;
        image-version)
            VERSION_IM="${OPTARG}"
            EXT_V_SET_WASMEDGE_IM=1
            ;;
        image-deps-version)
            VERSION_IM_DEPS="$OPTARG"
            EXT_V_SET_WASMEDGE_IM_DEPS=1
            ;;
        ?)
            exit 2
            ;;
        ??*)
            echo "${RED}Illegal option${NC}"
            exit 1
            ;;
        *)
            echo "Internal error!"
            exit 1
            ;;
        esac
    done

    shift $((OPTIND - 1)) # remove parsed options and args from $@ list

    [ $EXT_V_SET_WASMEDGE -ne 1 ] && remote_version_availabilty WasmEdge/WasmEdge "$VERSION"

    detect_os_arch

    if [ "$REMOVE_OLD" == "1" ] || [[ "$REMOVE_OLD" =~ ^([yY][eE][sS]|[yY])$ ]]; then
        if [ -f "$IPATH/env" ]; then
            bash <(curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/uninstall.sh) -p "$IPATH" -q
        fi
    fi

    set_ENV "$IPATH"
    mkdir -p "$IPATH"
    mkdir -p "$TMP_DIR"

    echo "$ENV" >"$IPATH/env"
    echo "# Please do not edit comments below this for uninstallation purpose" >>"$IPATH/env"

    local _source=". \"$IPATH/env\""
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

    if [ ! $default == 1 ]; then
        echo "${YELLOW}No path provided"
        echo "Installing in $IPATH${NC}"
    fi

    if [ ! $VERBOSE == 0 ]; then
        echo "Verbose Mode"
        set -xv
    fi

    if [ -d "$IPATH" ]; then
        echo "WasmEdge Installation at $IPATH"
        make_dirs "include" "lib" "bin"

        get_wasmedge_release
        install "$IPKG" "include" "lib" "bin"
        wasmedge_post_install "$VERSION"
        wasmedge_checks "$VERSION" "wasmedge" "wasmedgec"
    else
        echo "Installation path invalid"
        exit 1
    fi

    if [ "$EXT" = "all" ]; then
        echo "All extensions"
        install_image_extensions
        install_tf_extensions
        end_message
        exit_clean 0
    fi

    if [[ "$EXT" =~ "image" ]]; then
        echo "Image Extensions"
        install_image_extensions
    fi
    if [[ "$EXT" =~ "tf" ]] || [[ "$EXT" =~ "tensorflow" ]]; then
        echo "Tensorflow Extensions"
        install_tf_extensions
    fi

    trap - EXIT
    cleanup
    end_message
}

end_message() {
    if [ ! $default == 1 ]; then
        echo ""
        echo "${GREEN}source $IPATH/env${NC} to use wasmedge binaries"
    else
        case ":${PATH}:" in
        *:"${IPATH%"/"}/bin":*)
            echo "${GREEN}WasmEdge binaries accessible${NC}"
            ;;
        *)
            echo "${GREEN}source $IPATH/env${NC} to use wasmedge binaries"
            ;;
        esac
    fi
}

main "$@"
