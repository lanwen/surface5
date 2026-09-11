# Kernel camera patches

These are source patches for **6.19.8-arch1-3-surface**. Reassess against newer kernels: do not install an old `.ko` into a new kernel. Matching `linux-surface-headers`, `base-devel`, `curl` and `patch` are required. Secure Boot/module-signature enforcement may require signing local modules with your own trusted key.

| Order | Patch | Status |
| --- | --- | --- |
| 1 | `0001-dw9719-i2c-id.patch` | Local fix: bind the VCM using the I2C ID `dw9719` and correct `DW9719` model value; both cameras enumerate. |
| 2 | `0002-ov8865-program-mode-and-balance-pm.patch` | Upstream-proposed two-patch series: track programmed mode/Bayer code, program at stream start, reapply controls, balance failed stream-start PM reference. Rear picture worked on this machine. |
| 3 | `0003-ov8865-invert-hflip-optional.patch` | Optional local correction, applied after 2; parameter defaults off. Corrected rear orientation on this machine. |
| 4 | `0004-ov5693-reapply-mode-experimental.patch` | Experimental front mode reapplication. **Did not fix 832×480 black frames.** Not required by the default build. |

Patches 1, 2 and 4 touch different drivers; 3 depends on 2. Kernel changes retain the GPL licensing of their target files. Patch 2 retains the author's commit headers and sign-offs; local patches do not claim upstream submission or review.

## Reproduce

From the camera directory (`cd camera`), choose an empty output path outside the checkout:

```bash
./patches/linux/build.sh /tmp/surface5-kernel-build --orientation
```

Add `--experimental-front` only to reproduce the front experiment. The script verifies pinned source checksums and applies patches without fuzz before compiling. It never installs or loads a module.

Sources:

- DW9719: [linux-surface kernel commit 0c2fbead](https://github.com/linux-surface/kernel/blob/0c2fbead4937c3f06bef64bd123998f72f57f370/drivers/media/i2c/dw9719.c), including its existing 10ms wake delay.
- OV8865/OV5693: [Arch v6.19.8-arch1](https://github.com/archlinux/linux/tree/v6.19.8-arch1/drivers/media/i2c).
- Tested kernel recipe: [linux-surface arch-6.19.8-3](https://github.com/linux-surface/linux-surface/blob/arch-6.19.8-3/pkg/arch/kernel/PKGBUILD), SHA256 `a47b521418257274e0371d27530a922c06943bfe3129bcb9f16bc19753f9281e` matched package `.BUILDINFO`.
- Rear series: [linux-surface PR #2169](https://github.com/linux-surface/linux-surface/pull/2169), saved PR commit `9c03c5f5f13e0f83724562496f2b68325e826717`, embedded commits `da86337de2afb132ecbdf3365c1bd6787cee602e` and `c6432f54839f6704e7e13447cc40926c5b9cbef6`. Author: Jurison Murati. [Mailing-list submission](https://lore.kernel.org/linux-media/20260609232255.13559-1-eng.juri@gmail.com/). Inclusion in later releases is not established here.

## Install and rollback

Close camera applications. Before replacing any existing override, copy it to your own backup directory. These commands preserve packaged modules but overwrite an existing override of the same name:

```bash
test "$(uname -r)" = 6.19.8-arch1-3-surface
sudo install -Dm644 /tmp/surface5-kernel-build/drivers/media/i2c/dw9719.ko /usr/lib/modules/6.19.8-arch1-3-surface/updates/dw9719.ko
sudo install -Dm644 /tmp/surface5-kernel-build/drivers/media/i2c/ov8865.ko /usr/lib/modules/6.19.8-arch1-3-surface/updates/ov8865.ko
sudo depmod -a 6.19.8-arch1-3-surface
systemctl reboot
```

Only if using patch 3, create `/etc/modprobe.d/ov8865-orientation.conf` with `options ov8865 invert_hflip=1` before reboot. Preserve an existing file first. Removing this option and rebooting disables inversion while retaining the rear mode fix. Browser self-preview mirroring can be independent of sensor orientation.

For the optional front experiment, install `ov5693.ko` to the same `updates/` directory before `depmod`. Prefer a clean reboot over hot-unloading this interdependent media graph.

To roll back, restore your previous overrides, or remove only the overrides you installed, remove the optional orientation configuration, run `sudo depmod -a 6.19.8-arch1-3-surface`, then reboot. Never delete the packaged modules. Repeat validation after kernel updates. These manual overrides do not automatically rebuild; use the [DKMS package](../../packaging/surface5-camera-dkms/README.md) for automatic rebuilding.
