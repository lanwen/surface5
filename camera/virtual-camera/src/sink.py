#!/usr/bin/env python3
"""Keep the virtual producer attached while the physical sensor is idle."""
import os
import select
import socket
import subprocess
from pathlib import Path

from frames import FRAME_SIZE, frames


def main():
    path = Path(os.environ['XDG_RUNTIME_DIR']) / 'surface-camera-frames.sock'
    path.unlink(missing_ok=True)
    server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    video = None
    watch = None
    try:
        server.bind(str(path))
        os.chmod(path, 0o600)
        server.listen(1)
        video = os.open('/dev/video42', os.O_WRONLY)
        black = bytes([16, 128, 16, 128]) * (FRAME_SIZE // 4)
        if os.write(video, black) != FRAME_SIZE:
            raise RuntimeError('Incomplete initial frame write')
        watch_path = os.environ.get('SURFACE_CAMERA_WATCH', str(Path(__file__).with_name('watch')))
        watch = subprocess.Popen([watch_path])
        while watch.poll() is None:
            if not select.select([server], [], [], 1)[0]:
                continue
            connection, _ = server.accept()
            with connection:
                for frame in frames(connection):
                    if os.write(video, frame) != FRAME_SIZE:
                        raise RuntimeError('Incomplete frame write')
        raise RuntimeError('Camera demand watcher exited')
    finally:
        if watch is not None and watch.poll() is None:
            watch.terminate()
            watch.wait()
        if video is not None:
            os.close(video)
        server.close()
        path.unlink(missing_ok=True)


if __name__ == '__main__':
    main()
