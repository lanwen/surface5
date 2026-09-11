#!/usr/bin/env bash
# Build override modules for the tested kernel; never installs or reloads them.
set -euo pipefail
here=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
kernel=6.19.8-arch1-3-surface
out=${1:?Usage: build.sh EMPTY_OUTPUT_DIRECTORY [--orientation] [--experimental-front]}
shift
orientation=false
front=false
for arg in "$@"; do
  case $arg in
    --orientation) orientation=true ;;
    --experimental-front) front=true ;;
    *) echo "Unknown argument: $arg" >&2; exit 2 ;;
  esac
done
if [[ -e $out ]]; then echo 'Output directory must not already exist' >&2; exit 1; fi
[[ -d /usr/lib/modules/$kernel/build ]] || { echo "Install matching linux-surface headers for $kernel" >&2; exit 1; }
mkdir -p "$out/drivers/media/i2c"
out=$(cd -- "$out" && pwd)
base=https://raw.githubusercontent.com/archlinux/linux/v6.19.8-arch1/drivers/media/i2c
curl -fLsS https://raw.githubusercontent.com/linux-surface/kernel/0c2fbead4937c3f06bef64bd123998f72f57f370/drivers/media/i2c/dw9719.c -o "$out/drivers/media/i2c/dw9719.c"
for sensor in ov8865 ov5693; do
  curl -fLsS "$base/$sensor.c" -o "$out/drivers/media/i2c/$sensor.c"
done
(cd "$out/drivers/media/i2c" && sha256sum -c "$here/SHA256SUMS.sources")
patch --batch --fuzz=0 -d "$out" -p1 -i "$here/0001-dw9719-i2c-id.patch"
patch --batch --fuzz=0 -d "$out" -p1 -i "$here/0002-ov8865-program-mode-and-balance-pm.patch"
if "$orientation"; then patch --batch --fuzz=0 -d "$out" -p1 -i "$here/0003-ov8865-invert-hflip-optional.patch"; fi
modules='dw9719.o ov8865.o'
if "$front"; then
  patch --batch --fuzz=0 -d "$out" -p1 -i "$here/0004-ov5693-reapply-mode-experimental.patch"
  modules+=' ov5693.o'
fi
printf 'obj-m += %s\n' "$modules" > "$out/drivers/media/i2c/Makefile"
make -C "/usr/lib/modules/$kernel/build" M="$out/drivers/media/i2c" modules
printf 'Built modules in %s/drivers/media/i2c; see README.md for installation.\n' "$out"
