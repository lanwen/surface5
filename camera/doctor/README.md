# Libcamera doctor

From the repository root, run after an update:

```bash
./camera/doctor/libcamera-doctor.py
```

This checks:

- All four relevant Arch split packages are installed at the same version.
- Relevant installed libraries, IPU3 IPA/signature/proxy, cam executable and GStreamer plugin match the SHA256 fingerprints of the tested patched local build.
- `cam --help` and `gst-inspect-1.0 libcamerasrc` load successfully.
- The current command environment does not override libcamera/GStreamer/library loading.

It does not require root, change packages/configuration, access the network, start services, or request camera frames by default. GStreamer may update its normal per-user plugin cache. It inspects fresh processes; existing applications can retain old libraries after updates and may need restarting. Service-specific environment overrides and every possible dependency override are not audited.

## Optional capture check

```bash
./camera/doctor/libcamera-doctor.py --capture
```

This requests 60 1280×720 frames from `/dev/video42` through the existing demand service, which must already be active. It can turn the physical camera on. Frames go to a discard sink, never a recording or network destination. The ordinary demand watcher stops capture after viewers disconnect; other active viewers can keep it running. A busy device or another viewer can prevent this test from acquiring the camera. A timeout ends the test after 20 seconds.

Receiving frames is not an image-quality test: black/mirrored/stale frames are not detected. It does not certify Firefox preview or test the problematic 832×480 mode. The script deliberately does not provoke the known crash in a system process.

## Results

| Exit | Meaning |
| --- | --- |
| 0 | All requested checks passed; relevant binaries match the recorded patched build. Unrequested hardware tests remain unverified. |
| 1 | A package, runtime load, or requested capture check failed. |
| 2 | Review required: new/different build, changed fingerprints, or runtime overrides. The patch status is unknown. |

`--json` emits structured results and preserves these exit codes. Unknown builds are not silently treated as healthy, but a fingerprint mismatch does **not** establish that they are vulnerable. In particular, a rebuild of identical source with a different toolchain/signing key can have different fingerprints.

The baseline was checked against the original four local package archives, not generated solely from package version strings. It is not a security attestation and cannot prove arbitrary future builds contain an upstream fix. Do not replace fingerprints merely to make the doctor green. First follow the [upstream verification and regression procedure](../packaging/libcamera/README.md#tracking-when-the-local-patch-can-be-removed), validate the actual replacement build, then intentionally update the baseline.

Compiling the repository's extracted `validation/patched.cpp` proves only that that test source is patched; it would not verify the installed library. The doctor does not use that shortcut.
