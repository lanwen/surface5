# Requirements for automatic rebuilds

The current kernel build helper targets only `6.19.8-arch1-3-surface`. It is not a DKMS package and must not be installed as an unrestricted update hook.

A maintainable automatic setup requires:

1. A versioned Arch DKMS source package for the patched `dw9719`, `ov8865` and (if retained) experimental `ov5693` modules. Install audited sources and `dkms.conf` into `/usr/src/<package>-<version>`; do not have privileged hooks build arbitrary changing checkout files or download source during an upgrade.
2. A build that accepts DKMS's target kernel version and uses that kernel's matching headers, rather than `uname -r`. During an update the running kernel can still be the old one.
3. Explicit supported kernel versions and reviewed source/patch compatibility. The pinned 6.19.8 drivers cannot be assumed compatible with every future kernel. Preserve linux-surface-specific changes and remove patches when upstream incorporates them.
4. DKMS installation under the target kernel's updates directory, depmod, required module signing, and appropriate initramfs/UKI regeneration ordering. Verify the new boot artifacts load the overrides. Existing v4l2loopback-dkms already has its own automatic rebuild integration.
5. Visible build failure reporting and a tested bootable rollback. Automatic rebuilding does not guarantee compatibility with future kernel APIs.

Libcamera is separate: DKMS cannot rebuild it. Its patch must be checked against each newer package version, reapplied only if still needed, then the matching split packages rebuilt/tested together. A maintained custom package repository or a review-and-build workflow can distribute those packages. No such automatic workflow is currently installed. Permanently freezing a library while the rest of Arch updates can cause dependency/ABI inconsistencies.

The virtual-camera watcher itself is a userspace program. Rebuild it with `make -C camera/virtual-camera`; kernel updates normally do not require recompilation, but its private v4l2loopback event interface must remain compatible.
