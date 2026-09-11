#!/usr/bin/env python3
"""Read-only libcamera checks; --capture explicitly requests a brief camera stream."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

PACKAGES = ('libcamera', 'libcamera-ipa', 'libcamera-tools', 'gst-plugin-libcamera')
BASELINE = Path(__file__).with_name('known-good.json')


def run(argv, timeout=15):
    try:
        proc = subprocess.run(argv, capture_output=True, text=True, timeout=timeout)
        return proc.returncode, (proc.stdout + proc.stderr).strip()
    except (OSError, subprocess.TimeoutExpired) as error:
        return 1, str(error)


def package_result(versions):
    missing = [name for name in PACKAGES if name not in versions]
    if missing:
        return 'FAIL', 'Missing packages: ' + ', '.join(missing)
    if len(set(versions.values())) != 1:
        return 'FAIL', 'Split package versions differ: ' + ', '.join(f'{k}={v}' for k, v in versions.items())
    return 'PASS', 'Matching split package versions: ' + versions['libcamera']


def fingerprint_result(versions, baseline, root=Path('/')):
    if versions != baseline['packages']:
        return 'REVIEW', 'Installed build is not the recorded patched build; its underflow-fix status is unknown.'
    changed = []
    for name, expected in baseline['sha256'].items():
        path = root / name.lstrip('/')
        try:
            actual = hashlib.sha256(path.read_bytes()).hexdigest()
        except OSError:
            actual = None
        if actual != expected:
            changed.append(name)
    if changed:
        return 'REVIEW', 'Binary fingerprint differs or is missing: ' + ', '.join(changed)
    return 'PASS', 'Relevant binaries match the tested patched build (including IPU3 IPA and signature).'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture', action='store_true', help='receive 60 frames from /dev/video42; may activate the physical camera')
    parser.add_argument('--json', action='store_true', help='machine-readable output; exit 0=checked baseline healthy, 1=failure, 2=review needed')
    args = parser.parse_args()
    results = []

    def report(name, status, detail):
        results.append(dict(check=name, status=status, detail=detail))

    try:
        baseline = json.loads(BASELINE.read_text())
    except (OSError, ValueError) as error:
        report('baseline', 'FAIL', str(error))
        baseline = None
    versions = {}
    for name in PACKAGES:
        rc, output = run(['pacman', '-Q', name])
        if not rc:
            fields = output.split()
            if len(fields) == 2 and fields[0] == name:
                versions[name] = fields[1]
    report('packages', *package_result(versions))
    if baseline:
        report('patch-fingerprint', *fingerprint_result(versions, baseline))
    overrides = [name for name in os.environ if name.startswith(('LIBCAMERA_', 'GST_PLUGIN_', 'GST_REGISTRY', 'LD_LIBRARY_PATH', 'LD_PRELOAD')) and os.environ[name]]
    if overrides:
        report('environment', 'REVIEW', 'Runtime overrides may bypass the checked files: ' + ', '.join(sorted(overrides)))
    else:
        report('environment', 'PASS', 'No libcamera/GStreamer/loader overrides in this command environment.')
    # Check actual runtime loading, not the extracted historical regression source.
    rc, output = run(['cam', '--help'])
    report('cam-loader', 'FAIL' if rc else 'PASS', output[-1500:] if rc else 'cam starts and resolves its runtime libraries.')
    rc, output = run(['gst-inspect-1.0', 'libcamerasrc'])
    report('gstreamer-plugin', 'FAIL' if rc else 'PASS', output[-1500:] if rc else 'GStreamer loads libcamerasrc.')
    if args.capture:
        rc, output = run(['systemctl', '--user', 'is-active', 'surface-camera-demand.service'])
        if rc:
            report('capture', 'FAIL', 'Demand service is not active. No service was started or reconfigured.')
        else:
            rc, output = run([
                'gst-launch-1.0', '-q', 'v4l2src', 'device=/dev/video42', 'num-buffers=60', '!',
                'video/x-raw,format=YUY2,width=1280,height=720', '!', 'fakesink',
            ], timeout=20)
            report('capture', 'FAIL' if rc else 'PASS', output[-1500:] if rc else
                   'Received 60 virtual-camera frames at 1280x720. Does not certify image quality, Firefox, or the 832x480 crash fix.')
    else:
        report('capture', 'SKIP', 'Not requested. Use --capture to activate/test the virtual camera; no frames are recorded.')
    statuses = {item['status'] for item in results}
    code = 1 if 'FAIL' in statuses else 2 if 'REVIEW' in statuses else 0
    summary = ('Failures detected.' if code == 1 else 'Review required; patch protection is not verified.' if code == 2
               else 'Checked runtime and patched-build fingerprints pass; untested camera behavior is not certified.')
    if args.json:
        print(json.dumps(dict(exit_code=code, summary=summary, results=results), indent=2))
    else:
        for item in results:
            print(f"[{item['status']}] {item['check']}: {item['detail']}")
        print(summary)
        if code:
            print('See camera/packaging/libcamera/README.md for upstream checks and matched package rebuilds. No packages were changed.')
    return code


if __name__ == '__main__':
    sys.exit(main())
