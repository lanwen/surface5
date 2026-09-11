# Surface Pro 5 camera DKMS package

Version 6.19.8.1 contains the exact three patched driver sources used on this machine: DW9719 binding, OV8865 mode/PM and optional horizontal inversion, and the experimental OV5693 mode reapplication. The latter is included to preserve the running setup; it does not solve all low-resolution front-camera failures. `invert_hflip` still defaults off; the existing modprobe option enables it here.

The package downloads checksum-pinned upstream sources and applies the shared patches during the **unprivileged package build**. It installs the resulting source in `/usr/src/surface5-camera-6.19.8.1`. Kernel update hooks build this root-owned snapshot, never the changing Git checkout, and require no network access.

## Install

From this directory:

```bash
makepkg
sudo pacman -U surface5-camera-dkms-6.19.8.1-1-x86_64.pkg.tar.zst
```

Dependencies include `dkms` and `linux-surface-headers`. Arch's DKMS package hook builds the modules before this machine's Omarchy/Limine initramfs/UKI hook runs. The DKMS configuration targets `-surface` kernels on x86_64 only; the stock fallback kernel is not overridden. Check the entire update output for DKMS and boot-image failures.

**Migration from manual overrides:** before installing, preserve and remove only the manually installed `updates/dw9719.ko`, `updates/ov8865.ko`, and `updates/ov5693.ko` from the target kernel directory. Otherwise they can compete with DKMS's copies. On this machine the originals were backed up to `/var/lib/surface5-camera/manual-overrides-before-dkms/`; installation was checked before declaring migration complete. Do not remove the distribution's modules under `kernel/`.

Verify after installation and after each kernel update (replace the version with the target kernel, which can differ from `uname -r` before reboot):

```bash
dkms status
modinfo -k 6.19.8-arch1-3-surface -n dw9719
modinfo -k 6.19.8-arch1-3-surface -n ov8865
modinfo -k 6.19.8-arch1-3-surface -n ov5693
```

All three should resolve to `updates/dkms`. Do not hot-unload the interdependent camera graph. Newly installed modules take effect on reboot; the already loaded modules remain active until then.

## Scope and limitations

AUTOINSTALL requests a build for future Surface kernels using their target headers. Only **6.19.8-arch1-3-surface** has been tested. Successful compilation against a future kernel does not establish runtime compatibility; the 6.19.8 driver snapshot can also miss later upstream fixes. Review and update the pinned sources/patches as kernels advance, especially at major version changes. A compile failure is reported by the existing DKMS package hook; no successful rebuild or working camera is guaranteed in that case.

The driver source is compiled independently of the running kernel; the source version here is a package version, not an assertion that every future kernel uses that source. Automatic rebuilds do not update source pins, pull Git changes, pin system packages, or rebuild libcamera.

## Rollback

Removing this package invokes the standard DKMS removal hook:

```bash
sudo pacman -R surface5-camera-dkms
```

To restore the previous manual setup on the **same kernel version only**, copy the backed-up `.ko` files back to `/usr/lib/modules/6.19.8-arch1-3-surface/updates/`, run `sudo depmod -a 6.19.8-arch1-3-surface`, and regenerate that kernel's boot image using the installed Omarchy/Limine mechanism before reboot. For a different kernel, rebuild first. The orientation modprobe configuration is separately owned and is not removed by this package.

Without restoring overrides, removing the package returns to distribution modules; remove `invert_hflip=1` if the distribution OV8865 driver does not support it. Preserve a working boot/rollback option.

## Installation validation (2026-09-11)

- All pinned source/patch checksums passed; patched C sources compared byte-for-byte with the working manual build sources.
- All three modules compiled against 6.19.8-arch1-3-surface headers, then the package transaction independently built and installed them through the standard DKMS hook.
- `dkms status` reports `surface5-camera/6.19.8.1` installed; `pacman -Qkk` reports no altered package files.
- All three `modinfo` paths resolve to `updates/dkms`; installed and loaded source versions match, and the OV8865 `invert_hflip` parameter is present.
- Omarchy/Limine rebuilt and signed both installed UKIs successfully. The existing camera demand service remains active. No hot reload or reboot was performed; post-reboot operation remains to be checked.
- The installed toolchain emits a compiler-version difference warning (kernel GCC16.1.1, local GCC16.2.1); builds succeeded. DKMS3.4.3 also reports the nonfatal legacy CLEAN directive deprecation.
