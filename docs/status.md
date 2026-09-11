# Tested hardware and status

Recorded 2026-09-11 on Surface Pro 5, SKU `Surface_Pro_1796`, originally configured with Omarchy 4.0.2. Current package versions were checked while preparing this repository.

| Component | Tested version |
| --- | --- |
| Kernel | `6.19.8-arch1-3-surface` (`linux-surface 6.19.8.arch1-3`) |
| libcamera | local `0.7.2-4.1` |
| PipeWire | `1:1.6.8-1` |
| WirePlumber | `0.5.17-1` |
| Firefox | `155.0.1-1` |
| v4l2loopback-dkms | `0.15.4-2` |

| Camera | ACPI ID | Sensor | CSI port |
| --- | --- | --- | --- |
| Front | `\_SB_.PCI0.I2C2.CAMF` | OV5693 | 1 |
| Rear | `\_SB_.PCI0.I2C3.CAMR` | OV8865, DW9719 autofocus | 0 |

Intel IPU3 firmware was present at `/usr/lib/firmware/intel/ipu3-fw.bin.zst`. The virtual-camera bridge fixes the front stream to NV12 1280×720 at 30fps, converting to packed YUYV/YUY2 for `/dev/video42`.

## Evidence

- DW9719 binding fix enabled camera enumeration.
- Rear mode patches eliminated the reported green picture; optional driver inversion corrected horizontal orientation in native tools.
- Direct front capture at 1280×720 worked; 832×480 remained black, even with the experimental OV5693 stream-start patch.
- A WirePlumber SIGABRT was traced to libcamera IPU3 unsigned crop-height underflow and a `std::clamp` assertion. The package workaround prevented that assertion; it did not solve all physical camera format negotiation.
- Original demand service completed two start/60-frame/stop cycles. Firefox/Meet operation was subsequently confirmed by the user.
- Original service idle sample: roughly 9MiB service memory and 0.006% of one core across 10 seconds. This is a short observation, not a general benchmark; active conversion costs more.

## Repository changes versus the running prototype

The running installation was left untouched while publishing. Repository paths are relocatable; the installer uses `%h/.local/lib/surface5-camera` instead of a personal checkout path. Frame transport uses a private Unix stream socket instead of the prototype's FIFO: incomplete frames are discarded at connection boundaries so interrupted capture cannot misalign a later activation. Unit tests cover fragmented and interrupted frames plus staged install/rollback and conflict protection. The relocated/socket version has not yet undergone a fresh Firefox hardware test; the original FIFO version is the user-confirmed installation.

The physical libcamera monitor is disabled in WirePlumber to prevent browsers negotiating broken sensor modes. This affects all physical libcamera cameras in that user session. The automatic virtual camera currently exposes **front only**, and only one capture pipeline can own it. This is a local workaround, not a claim that upstream drivers are fully fixed.
