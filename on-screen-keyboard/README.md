# On-screen keyboard through the Omarchy bar

This is the keyboard setup installed on the Surface Pro 5 on 2026-09-11: **omarchy-wvkbd 1.0.0**, launching **wvkbd-pcintl 0.20** with the custom full PC layout. Click/tap the keyboard icon in the bar to show or hide it. The widget highlights while the keyboard process is running.

## Current configuration

| Setting | Observed value |
| --- | --- |
| Plugin ID | `omarchy-wvkbd` |
| Layout placement | `bar.layout.right`, after `omarchy.monitor`, before `omarchy.power` |
| Layout entry | `{ "id": "omarchy-wvkbd" }` — no explicit setting overrides |
| Keyboard binary | `~/.local/bin/wvkbd-pcintl` (plugin default) |
| Keyboard layers | `full`, including landscape layers (plugin default) |
| Landscape height | 400 pixels (plugin default) |
| Activation | Bar button; optional existing IPC command `omarchy-shell wvkbd toggle` |
| Automatic activation / keybinding | No keyboard-specific autostart, user service, or Hyprland keybinding found in the current configuration |

The desktop bar itself is currently positioned on the right edge. Its broader layout is not required by this plugin and is not copied here. The minimal [bar-entry.json](bar-entry.json) captures only this keyboard's entry; do not replace your entire `shell.json` with it.

The plugin registers an IPC toggle but does not automatically show on text-field focus or keyboard detachment. It checks the process with `pgrep -x`, launches with an argument array, and hides it with `pkill -x`. No separate keyboard daemon starts at login; the plugin is loaded with the Omarchy shell.

## Source and provenance

- Plugin: [thesimonharms/omarchy-wvkbd-plugin](https://github.com/thesimonharms/omarchy-wvkbd-plugin), pinned [commit c5de0064](https://github.com/thesimonharms/omarchy-wvkbd-plugin/tree/c5de0064db4d448227dc3d98095b513e0dab0128), author Simon Harms, MIT license.
- Installed plugin checkout was clean at that commit. Its local remote pointed to a temporary review checkout; reproducibility here uses the public upstream commit, not that temporary path.
- `plugin/` is a byte-for-byte snapshot of the necessary QML, manifest, layout/keymap/config headers, build script, README and license. Unused upstream editor settings and bundled font assets are omitted; the widget uses Omarchy's existing icon font. Verify with `cd plugin && sha256sum -c ../SHA256SUMS.upstream`.
- The unmodified build script fetches [wvkbd](https://git.sr.ht/~proycon/wvkbd) at commit `6b41504a0cb58fd1163fa44692398fbd61f8905f` (v0.20), overlays the three PC-layout headers, and builds `LAYOUT=pcintl`. It installs only to `XDG_BIN_HOME` or `~/.local/bin`, without root.

## Reproduce

The inspected environment used Omarchy `4.0.3-1`, Quickshell `0.3.1-1`, Wayland `1.26.0-1`, libxkbcommon `1.13.2-1`, Pango `1:1.58.2-1`, Cairo `1.18.4-1`, and procps-ng `4.0.7-1`. Omarchy's shell/plugin API is required; a generic Quickshell installation alone does not provide its `qs.Ui` components.

Install build dependencies if missing:

```bash
sudo pacman -S --needed base-devel git wayland libxkbcommon pango cairo procps-ng scdoc
```

From the repository root, validate and build into a temporary destination without modifying your installed keyboard:

```bash
omarchy plugin validate ./on-screen-keyboard/plugin
keyboard_build=$(mktemp -d)
XDG_BIN_HOME="$keyboard_build" ./on-screen-keyboard/plugin/wvkbd-pcintl/build.sh
"$keyboard_build/wvkbd-pcintl" --version
```

Keep `keyboard_build` in the same terminal for installation below. The build fetches sources over the network; generated files are temporary. Ensure `~/.local/bin` is on the desktop session's `PATH`.

For a **new** installation, the following subshell refuses to overwrite an existing plugin or keyboard binary:

```bash
(
  set -eu
  plugin_dest="$HOME/.config/omarchy/plugins/omarchy-wvkbd"
  binary_dest="$HOME/.local/bin/wvkbd-pcintl"
  test ! -e "$plugin_dest"
  test ! -e "$binary_dest"
  mkdir -p "$HOME/.config/omarchy/plugins" "$HOME/.local/bin"
  cp -a ./on-screen-keyboard/plugin "$plugin_dest"
  install -m755 "$keyboard_build/wvkbd-pcintl" "$binary_dest"
  omarchy-shell shell rescanPlugins
  omarchy plugin enable omarchy-wvkbd --section right --before omarchy.power
)
```

If your bar has no `omarchy.power` widget, omit `--before omarchy.power`; the section remains right. The Omarchy command updates only plugin enablement/placement through its shell API. The existing working installation does not need reinstalling. For migration, first disable and back up the current plugin directory and keyboard binary outside `~/.config/omarchy/plugins`, and back up `~/.config/omarchy/shell.json` before replacing anything.

The vendored plugin is a pinned copy, not a git-managed plugin checkout, so `omarchy plugin update` is not its update mechanism. Review and refresh the snapshot deliberately to upgrade. Upstream's normal `omarchy plugin add https://github.com/thesimonharms/omarchy-wvkbd-plugin.git --enable --yes` follows the current remote and may differ from this recorded setup.

## Verify and troubleshoot

- `omarchy plugin list --json` should show `omarchy-wvkbd`.
- `command -v wvkbd-pcintl` should resolve to the intended binary; `wvkbd-pcintl --version` should report `wvkbd-0.20` for this build.
- Tap the bar icon: verify text entry, modifiers, arrow/function keys, and hide/show behavior. The IPC command `omarchy-shell wvkbd toggle` uses the same toggle logic.
- If the widget is missing, run `omarchy-shell shell rescanPlugins`, then retry enablement. `omarchy restart shell` is a fallback if reloading fails.
- If the widget appears but the keyboard does not, check the Omarchy shell's `PATH` and the custom binary; the stock `wvkbd-mobintl` binary is a different layout and is not this setup.
- Automatic text-field or tablet-mode activation is not configured by this snapshot.

Manifest validation, snapshot checksum verification and an isolated pinned build passed when preparing the repository; the resulting binary reported `wvkbd-0.20`. Publishing did not reload the shell, invoke the toggle, alter bindings, or replace the live binary.

## Rollback

For this newly installed snapshot:

```bash
omarchy plugin disable omarchy-wvkbd
pkill -x wvkbd-pcintl || true
omarchy plugin remove omarchy-wvkbd --yes
rm "$HOME/.local/bin/wvkbd-pcintl"
```

Only remove the binary if this setup installed it. For migration rollback, restore your saved plugin and binary, rescan, and re-enable at the previous placement. Restore only the relevant bar entry when possible; copying an old whole `shell.json` can discard later unrelated changes. System build dependencies can remain installed.
