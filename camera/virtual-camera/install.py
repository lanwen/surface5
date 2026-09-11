#!/usr/bin/env python3
"""Install only project-owned files, backing up replacements for exact rollback."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

SOURCE = Path(__file__).resolve().parent


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=['install-user', 'uninstall-user', 'install-system', 'uninstall-system'])
    parser.add_argument('--replace', action='store_true', help='back up existing conflicting files before replacing')
    parser.add_argument('--activate', action='store_true', help='start/enable the user service and restart WirePlumber')
    parser.add_argument('--root', type=Path, help='stage in this directory without changing services (testing/packaging)')
    args = parser.parse_args()
    system = args.action.endswith('system')
    uninstall = args.action.startswith('uninstall')
    if args.activate and (system or args.root or uninstall):
        parser.error('--activate requires install-user without --root')
    if not args.root and system and os.geteuid() != 0:
        parser.error('system configuration requires root; run with sudo')
    if not args.root and not system and os.geteuid() == 0:
        parser.error('run the user installation as your desktop user, without sudo')
    root = args.root.resolve() if args.root else Path('/')
    home = Path.home()
    def target(path):
        return root / str(path).lstrip('/')
    state = target('/var/lib/surface5-camera' if system else home / '.local/state/surface5-camera')
    manifest = state / 'manifest.json'
    previous = json.loads(manifest.read_text()) if manifest.exists() else {}
    if system:
        files = {
            target('/etc/modprobe.d/surface-virtual-camera.conf'): SOURCE / 'config/v4l2loopback.conf',
            target('/etc/modules-load.d/surface-virtual-camera.conf'): SOURCE / 'config/modules-load.conf',
        }
    else:
        files = {target(home / '.local/lib/surface5-camera' / name): SOURCE / 'src' / name
                 for name in ['sink.py', 'stream-demand.py', 'frames.py']}
        files[target(home / '.local/lib/surface5-camera/watch')] = SOURCE / 'build/watch'
        for name in ['surface-camera-demand.service', 'surface-virtual-camera.service']:
            files[target(home / '.config/systemd/user' / name)] = SOURCE / 'systemd' / name
        files[target(home / '.config/wireplumber/wireplumber.conf.d/90-surface-virtual-camera.conf')] = SOURCE / 'config/90-surface-virtual-camera.conf'
    # Preflight every file before changing anything.
    check = {Path(p): None for p in previous} if uninstall else files
    for dest, src in check.items():
        if src is not None and not src.is_file():
            raise RuntimeError(f'Missing {src}; run make -C virtual-camera first')
        if dest.is_symlink() or (dest.exists() and not dest.is_file()):
            raise RuntimeError(f'Refusing non-regular destination: {dest}')
        if dest.exists():
            owned = str(dest) in previous and digest(dest) == previous[str(dest)]['installed']
            identical = src is not None and digest(dest) == digest(src)
            if not owned and not identical and (uninstall or not args.replace):
                raise RuntimeError(f'Conflicting file: {dest}; back it up or use --replace for installation')
    def ctl(*words):
        subprocess.run(['systemctl', '--user', *words], check=True)
    if uninstall and not system and not args.root and previous:
        ctl('disable', '--now', 'surface-camera-demand.service')
        ctl('stop', 'surface-virtual-camera.service')
    elif args.activate:
        # An older installation may own the virtual producer; stop it before replacing files.
        subprocess.run(['systemctl', '--user', 'stop', 'surface-camera-demand.service', 'surface-virtual-camera.service'], check=False)
    state.mkdir(parents=True, exist_ok=True)
    if uninstall:
        for name, entry in previous.items():
            dest = Path(name)
            if entry['backup']:
                shutil.copy2(state / entry['backup'], dest)
            else:
                dest.unlink(missing_ok=True)
        shutil.rmtree(state)
    else:
        for dest, src in files.items():
            name = str(dest)
            entry = previous.get(name)
            if entry is None:
                backup = None
                if dest.exists():
                    backup = hashlib.sha256(name.encode()).hexdigest() + '.backup'
                    shutil.copy2(dest, state / backup)
                entry = {'backup': backup}
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(src, dest)
            dest.chmod(0o755 if dest.name == 'watch' else 0o644)
            entry['installed'] = digest(dest)
            previous[name] = entry
            # Persist each backup before proceeding, making interrupted installs recoverable.
            manifest.write_text(json.dumps(previous, indent=2) + '\n')
    if not system and not args.root:
        ctl('daemon-reload')
        if args.activate:
            ctl('enable', '--now', 'surface-camera-demand.service')
            ctl('restart', 'wireplumber.service')
        elif uninstall and previous:
            ctl('restart', 'wireplumber.service')
    print(f'{args.action}: complete' + (' (staged; services untouched)' if args.root else ''))


if __name__ == '__main__':
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        sys.exit(str(error))
