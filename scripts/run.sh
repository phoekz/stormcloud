set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"
BIN_DIR="$ROOT_DIR/build/Debug"
echo "Script location: $SCRIPT_DIR"
echo "Root location: $ROOT_DIR"
pushd $ROOT_DIR
cmake -S . -B build
cmake --build build
$BIN_DIR/stormcloud.exe temp/tokyo.oct
popd
