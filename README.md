# Surface Pro 5 on Linux

Setup notes, reproducible fixes and local tooling for running Linux and Omarchy on a Microsoft Surface Pro 5 with the linux-surface kernel.

This repository is organized by hardware subsystem. Each area records the tested configuration, upstream provenance, installation and rollback procedures, and known limitations. Camera support and the on-screen keyboard are documented; additional areas can be added as they are investigated and verified.

| Area | Contents |
| --- | --- |
| [Hardware and environment](docs/hardware.md) | Device identity and baseline software |
| [On-screen keyboard](on-screen-keyboard/README.md) | Current Omarchy bar plugin and custom PC-layout wvkbd build |
| [Camera](camera/README.md) | Sensor/kernel patches, patched libcamera packaging, automatic virtual webcam and troubleshooting |

The current camera setup provides a working front webcam in Firefox through a fixed 720p capture pipeline that starts when an application requests video. See the [camera status](camera/docs/status.md) for which fixes are confirmed, experimental or optional.

Only source, configuration and documentation are tracked. Prebuilt modules, packages, firmware, core dumps and private machine logs are excluded. Components retain their own upstream licenses; see [licensing and attribution](LICENSES.md).
