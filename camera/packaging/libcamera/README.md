# libcamera 0.7.2-4.1 local Arch package

The IPU3 `calculateBDSHeight()` calculation subtracted a maximum crop height (540) from a smaller unsigned intermediate height (536). The wraparound reversed clamp bounds and caused a SIGABRT in WirePlumber while Firefox requested 832×480. `ipu3-crop-underflow.patch` uses a lower bound of 1 when that subtraction would reach zero or underflow, preserving assertions.

This is a targeted local workaround, not a complete audit of all IPU3 sizing math and not a claim of upstream inclusion. It stopped the observed crash but did **not** make all front sensor modes work. The virtual camera avoids those modes.

## Provenance

Based on [Arch packaging tag 0.7.2-4](https://gitlab.archlinux.org/archlinux/packaging/packages/libcamera/-/tree/0.7.2-4), commit `ae31dca61cfa72046048bd85923d78c231d3ac11`. `PKGBUILD.stock` is retained for comparison; SHA256 `811cf722c31ef3e0276378ee743f039c445d907a1423a0b4409ea30de07dc891` matched the original installed package `.BUILDINFO`.

Changes: package release 4.1, crop guard, test discovery uses Meson's last field, and documentation/Python bindings disabled. The original Python 3.14 source compatibility patch is retained. Source tag is libcamera v0.7.2. Both patches have SHA512/BLAKE2 checksums in the recipe. Arch recipe licensing is in `LICENSE`; libcamera changes and extracted calculation tests retain upstream LGPL-2.1-or-later licensing.

## Build and install

Use a matching Arch environment or clean chroot, with `base-devel` installed. These are historical version-pinned packages; review dependencies before installing on a newer system.

```bash
cd camera/packaging/libcamera
makepkg --verifysource
makepkg -s
sudo pacman -U ./libcamera-0.7.2-4.1-x86_64.pkg.tar.zst ./libcamera-ipa-0.7.2-4.1-x86_64.pkg.tar.zst ./libcamera-tools-0.7.2-4.1-x86_64.pkg.tar.zst ./gst-plugin-libcamera-0.7.2-4.1-x86_64.pkg.tar.zst
```

Keep all four runtime packages from the same build together. IPA libraries are signed during the build; mixed library/IPA builds may fail verification. The optional debug package is not needed to run the camera. Restart camera applications and WirePlumber afterward. Do not split a filename across shell lines.

For rollback, retain the four previous matching packages from `/var/cache/pacman/pkg`, install them together with `pacman -U`, then restart consumers. Ordinary package upgrades can supersede this local version; review whether the workaround is still necessary before carrying it forward.

## Validation

The original package build on 2026-09-11 passed **48 tests, 1 expected failure, 30 skips, 0 unexpected failures**. Three existing namespace-requiring GStreamer tests remain excluded by the recipe. Hardware skips are not evidence that hardware works.

`validation/original.cpp` and `patched.cpp` extract the sizing algorithm with minimal scaffolding; `results.json` records the historical comparison. With installed libcamera development headers/libs, reproduce in an output directory:

```bash
c++ -std=c++17 -O2 -D_GLIBCXX_ASSERTIONS validation/original.cpp $(pkg-config --cflags --libs libcamera) -o /tmp/surface5-sizing-original
c++ -std=c++17 -O2 -D_GLIBCXX_ASSERTIONS validation/patched.cpp $(pkg-config --cflags --libs libcamera) -o /tmp/surface5-sizing-patched
(ulimit -c 0; /tmp/surface5-sizing-original 832 480)  # expected assertion failure
/tmp/surface5-sizing-patched 832 480
/tmp/surface5-sizing-patched 832 480 boundaries
```

For input1296×972/output832×480, patched output was IF1296×756 / BDS864×504 / GDC832×480, scale1.5. Original and patched agreed for outputs640×480 and1280×720. Boundary tests exercise heights536,540,544,972 and BDS factors. These are algorithm checks, not end-to-end camera validation.

## Tracking when the local patch can be removed

Checked on **2026-09-11**: official libcamera HEAD was [`87c7285663aaad7608fdc18d5216ec6811c685c7`](https://gitlab.freedesktop.org/camera/libcamera/-/commit/87c7285663aaad7608fdc18d5216ec6811c685c7). Its [`imgu.cpp`](https://gitlab.freedesktop.org/camera/libcamera/-/blob/87c7285663aaad7608fdc18d5216ec6811c685c7/src/libcamera/pipeline/ipu3/imgu.cpp) still contains `unsigned int minIFHeight = iif.height - ImgUDevice::kIFMaxCropHeight;` without the local guard. This was verified from the official Git repository, not inferred from a release number.

No matching upstream submission was identified in the searches performed. The official [Patchwork underflow search](https://patchwork.libcamera.org/project/libcamera/list/?q=underflow&archive=both&state=*) returned no patches; the [ImgU patch history](https://patchwork.libcamera.org/project/libcamera/list/?q=imgu&archive=both&state=*) did not identify an equivalent new fix. These searches do not prove no differently named submission exists. There is currently **no verified PR/MR/patch ID to watch**, and this repository has not submitted one on the user's behalf. The OV8865 linux-surface PR documents a separate kernel-driver fix.

To return to an unpatched distribution package:

1. Identify the upstream commit that fixes this underflow or replaces the affected calculation safely.
2. Confirm the exact Arch package's source includes that commit (or an equivalent backport); an open or merged submission alone is insufficient.
3. Run the regression calculation for the original 832×480 failure and the documented boundary cases against that source, then test real 1280×720 capture and Firefox through the virtual camera.
4. Install matching libcamera/IPA/tools/GStreamer packages together and remove the local package patch only after those checks pass.

Fixing this crash alone does not establish that native low-resolution front capture works. The virtual camera may still be necessary.

## Installed build check

Run `./camera/doctor/libcamera-doctor.py` from the repository root after updates. It checks package consistency, relevant binary fingerprints and runtime loading; new builds require review even when 720p capture works. See the [doctor documentation](../../doctor/README.md).
