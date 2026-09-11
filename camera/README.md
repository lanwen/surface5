# Surface Pro 5 camera fixes

Reproducible patches and an automatically activated virtual webcam for a Surface Pro 5 running Omarchy and the linux-surface kernel. This records a working local setup, including experiments that did not completely fix the underlying drivers.

**Working browser path:** front camera → fixed 1280×720 capture → V4L2 loopback → PipeWire/Firefox. Selecting **Surface Front 720p** starts capture automatically, including permission previews. The sensor stops three seconds after the last viewer disconnects.

| Directory | Contents |
| --- | --- |
| [virtual-camera/](virtual-camera/README.md) | Demand watcher, frame bridge, user services, reversible installer and configuration |
| [patches/linux/](patches/linux/README.md) | DW9719 binding fix, OV8865 mode/PM patches, optional orientation, experimental OV5693 patch |
| [packaging/libcamera/](packaging/libcamera/README.md) | Arch 0.7.2-4.1 package recipe and IPU3 crop-underflow workaround |
| [docs/status.md](docs/status.md) | Hardware, exact tested versions, verification and limitations |
| [docs/troubleshooting.md](docs/troubleshooting.md) | Diagnosis and recovery |

Start with the [status](docs/status.md), then the [virtual-camera instructions](virtual-camera/README.md) if native front capture at 1280×720 already works. Otherwise apply the relevant kernel/libcamera fixes first. Install only the changes your system needs; the experimental front patch is not a general fix for black frames.

No prebuilt modules, packages, firmware, core dumps or machine logs are distributed. Nothing in this repository uploads or records camera frames. Source provenance and licensing are recorded with each component in [LICENSES.md](../LICENSES.md).

[Automatic driver rebuilds with DKMS](packaging/surface5-camera-dkms/README.md) are available for the current patch set. Libcamera updates remain a separate reviewed rebuild.

## Check after updates

From the repository root, run `./camera/doctor/libcamera-doctor.py`. Add `--capture` for a brief virtual-camera test. The [doctor documentation](doctor/README.md) explains patch fingerprints, unknown-build results and exit codes.
