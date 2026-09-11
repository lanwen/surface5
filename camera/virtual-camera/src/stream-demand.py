#!/usr/bin/env python3
"""Capture the known-working front mode and send complete YUYV frames."""
import os
import socket
import subprocess
import sys

from frames import FRAME_SIZE

command = [
    'gst-launch-1.0', '-q', '-e',
    'libcamerasrc', r'camera-name=\\_SB_.PCI0.I2C2.CAMF', '!',
    'video/x-raw,format=NV12,width=1280,height=720,framerate=30/1', '!',
    'videoconvert', '!', 'video/x-raw,format=YUY2,width=1280,height=720', '!',
    'fdsink', 'fd=1', 'sync=false',
]


def main():
    connection = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    connection.connect(os.path.join(os.environ['XDG_RUNTIME_DIR'], 'surface-camera-frames.sock'))
    process = subprocess.Popen(command, stdout=subprocess.PIPE)
    count = 0
    try:
        while True:
            frame = process.stdout.read(FRAME_SIZE)
            if len(frame) != FRAME_SIZE:
                raise RuntimeError('Camera stream ended before a complete frame')
            connection.sendall(frame)
            count += 1
            if count == 30:
                print('Delivered 30 complete frames to virtual camera', file=sys.stderr, flush=True)
    except KeyboardInterrupt:
        pass
    finally:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
        connection.close()


if __name__ == '__main__':
    main()
