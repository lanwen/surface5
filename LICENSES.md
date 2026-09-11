# Licensing and attribution

This repository contains multiple independently licensed components; no single license replaces their upstream licenses.

- Kernel patches in `camera/patches/linux`: GPL-2.0-only for DW9719/OV5693 (their source SPDX identifier is GPL-2.0), and GPL-2.0-or-later for OV8865. Upstream rear patch author/sign-offs are preserved verbatim. Local amendments are provided under the same terms.
- `camera/packaging/libcamera/PKGBUILD`, `PKGBUILD.stock` and the existing Arch Python compatibility patch: retain Arch packaging provenance and the included `camera/packaging/libcamera/LICENSE` permission notice. The Python patch modifies upstream LGPL code and retains its own headers.
- `camera/packaging/libcamera/ipu3-crop-underflow.patch` and `validation/*.cpp`: LGPL-2.1-or-later, as adaptations/extractions of libcamera's `src/libcamera/pipeline/ipu3/imgu.cpp`. Upstream authors retain their copyrights; see the v0.7.2 source. Validation scaffolding and local guard are distributed on those same terms.
- `on-screen-keyboard/plugin/`: MIT, copyright (c) 2026 thesimonharms; original notice is retained in `on-screen-keyboard/plugin/LICENSE`. The upstream wvkbd source is fetched separately for builds and retains its own licensing.
- Newly written virtual-camera code, installer, tests, build helper, DKMS packaging recipe/configuration and documentation: MIT, below.

## MIT license for original repository material

Copyright (c) 2026 surface5 contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
