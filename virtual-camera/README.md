# Automatic front virtual camera

The device is **Surface Front 720p**, `/dev/video42`. A lightweight user service holds the virtual output open; v4l2loopback client-usage events start real capture only when a viewer begins streaming, and stop it after three idle seconds. Firefox permission previews count as viewers. No physical sensor capture/conversion runs while idle.

## Prerequisites

Native front capture at **1280×720** must work. This bridge does not fix a broken sensor or missing firmware. Tested with v4l2loopback **0.15.4**; it requires that version's private client-usage event API. The event constant is taken from [the driver source](https://github.com/v4l2loopback/v4l2loopback/blob/v0.15.4/v4l2loopback.c). It is not a stable public V4L2 ABI.

On Arch/Omarchy, install dependencies with matching headers for your running kernel:

```bash
sudo pacman -S --needed base-devel python v4l-utils v4l2loopback-dkms linux-surface-headers gstreamer gst-plugins-base gst-plugins-good gst-plugin-libcamera
```

If using this repository's libcamera package, install its matching four split packages together; do not replace just the GStreamer plugin with an incompatible repository version. PipeWire and WirePlumber must already be set up for the desktop. Normal active-session access to `/dev/video42` is required; do not run the user services as root.

## Install

Run from the repository root. Build and test first:

```bash
make -C virtual-camera check
```

Install the two root-owned boot configuration files, then load the virtual module:

```bash
sudo python3 virtual-camera/install.py install-system
sudo modprobe v4l2loopback
```

The installer refuses conflicting files. `--replace` explicitly backs them up for rollback. Existing configuration files for other v4l2loopback devices elsewhere in `/etc/modprobe.d` must be reconciled manually. If the module is already loaded with different options, reboot after configuration instead of assuming `modprobe` changes the live device. Confirm that `/dev/video42` has label **Surface Front 720p** before continuing:

```bash
v4l2-ctl -d /dev/video42 --info
python3 virtual-camera/install.py install-user --activate
```

For migration from the original local prototype, use `install-user --replace --activate`; it backs up the previous units/configuration and stops their processes before replacement. The original files under `surface-camera-fix` are untouched.

The user installer copies the watcher and Python files to `~/.local/lib/surface5-camera`, services to `~/.config/systemd/user`, and a WirePlumber override to its user configuration directory. Service paths use `%h`, so moving/deleting this checkout after installation is fine. It enables automatic login activation and restarts WirePlumber (which may briefly affect audio).

Restart Firefox and select **Surface Front 720p**. The WirePlumber configuration disables physical libcamera devices in that user session, keeping browsers from negotiating the broken sensor resolutions. Other V4L2 cameras remain available.

For packaging/review without changing the machine, stage files in a temporary directory:

```bash
python3 virtual-camera/install.py install-system --root /tmp/surface5-stage
python3 virtual-camera/install.py install-user --root /tmp/surface5-stage
```

Staging never invokes systemctl or loads modules. `--activate` is unavailable with staging. The installer's fixed home-relative layout intentionally follows systemd's normal user paths rather than custom XDG_CONFIG_HOME overrides.

## Operation

```bash
systemctl --user disable --now surface-camera-demand  # disable automatic capture
systemctl --user enable --now surface-camera-demand   # enable again
journalctl --user -u surface-camera-demand -u surface-virtual-camera -f
```

`surface-camera-demand` remains active while idle; `surface-virtual-camera` only runs during capture. To force a manual capture for diagnosis, start `surface-virtual-camera` while the demand service is running; it requires the sink. Close camera viewers before using native qcam/cam, then stop the demand service.

`src/stream-manual.sh` preserves the original direct GStreamer reference pipeline. It needs the automatic demand service stopped first because both write the same device.

## Architecture

- `watch.c` subscribes to `V4L2_EVENT_PRIVATE_START + 0x08E00000 + 1`, reads the driver's streaming-client count and controls the capture unit. Empty dequeue handles both EAGAIN and ENOENT.
- `sink.py` holds the virtual producer descriptor and writes an initial black frame, allowing idle format enumeration with `exclusive_caps=1` and `keep_format=1`. It does not open a physical camera.
- `stream-demand.py` runs GStreamer with CAMF → NV12 1280×720/30 → YUY2 and sends frames through a mode-0600 Unix socket in `XDG_RUNTIME_DIR`.
- `frames.py` reconstructs full frames and discards a partial last frame when a writer disconnects. A new activation begins at a fresh frame boundary.

The format, sensor ID, loopback device and three-second idle delay are intentionally fixed to the tested setup. Change the C/Python sources and service/config files together if adapting them. See [validation limits](../docs/status.md#repository-changes-versus-the-running-prototype).

## Uninstall / restore

Close camera viewers, then run:

```bash
python3 virtual-camera/install.py uninstall-user
sudo python3 virtual-camera/install.py uninstall-system
```

This stops/disables the project services, removes installed files or restores originals backed up during installation, reloads user units and restarts WirePlumber. Files edited after installation are protected: resolve the conflict before retrying. Backup manifests live in `~/.local/state/surface5-camera` and `/var/lib/surface5-camera`. Restoring an older unit does not automatically re-enable it; review it and enable deliberately if wanted.

Packages and kernel modules remain installed. Reboot to apply the restored/removed boot configuration, or unload v4l2loopback only after all consumers have closed it. Kernel/libcamera patches have separate rollback procedures.
