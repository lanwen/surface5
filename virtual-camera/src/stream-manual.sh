#!/usr/bin/env bash
set -euo pipefail
exec gst-launch-1.0 -e \
  libcamerasrc camera-name='\\_SB_.PCI0.I2C2.CAMF' ! \
  'video/x-raw,format=NV12,width=1280,height=720,framerate=30/1' ! \
  videoconvert ! 'video/x-raw,format=YUY2,width=1280,height=720' ! \
  v4l2sink device=/dev/video42 sync=false
