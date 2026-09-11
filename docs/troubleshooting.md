# Troubleshooting

```bash
systemctl --user status surface-camera-demand surface-virtual-camera
journalctl --user -u surface-camera-demand -u surface-virtual-camera -n 80
v4l2-ctl -d /dev/video42 --all
wpctl status
```

- **Camera missing:** verify v4l2loopback loaded and `/dev/video42` is the virtual camera. With `exclusive_caps=1`, an attached producer is required for reliable Firefox format enumeration. `surface-camera-demand` must be running even while the sensor is idle. `keep_format=1` alone was insufficient.
- **Firefox spins or says blocked:** close and reopen its preview, select **Surface Front 720p**, then reload the tab. Restart Firefox if it cached an earlier device failure. `media.webrtc.camera.allow-pipewire=true` was used on the tested installation. Check camera permissions and service logs before changing drivers.
- **Front selected but rear appears:** verify the virtual source is selected and WirePlumber's physical libcamera monitor is disabled. The bridge's sensor ID is CAMF.
- **Black native front camera:** 832×480 is a known failing mode here. Validate native 1280×720 first; the experimental mode patch did not fix every mode.
- **Camera stays on:** a permission preview is a real viewer. Close all previews/tabs/apps using video and wait three seconds. The demand service staying active is normal; the physical `surface-virtual-camera` service should stop.
- **Need native cam/qcam:** close browser viewers and stop `surface-camera-demand`; that also stops its capture service. Start it again afterward. The manual reference pipeline in `virtual-camera/src/stream-manual.sh` cannot run concurrently with the automatic producer.
- **After a kernel upgrade:** check matching v4l2loopback DKMS build and kernel headers. Sensor override modules are tied to the old kernel and need reassessment/rebuild if still necessary.
- **WirePlumber crashes:** inspect `coredumpctl info wireplumber` locally. The known crop assertion fix belongs in libcamera, not WirePlumber. Do not publish core dumps; they may contain private process memory.

A frame-consumer smoke test (starts the real camera) is:

```bash
timeout 15 gst-launch-1.0 -q v4l2src device=/dev/video42 num-buffers=60 ! fakesink
```

After it exits, check the physical service stops within three seconds. Visual confirmation is still required: receiving buffers does not prove the picture is correct.
