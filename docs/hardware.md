# Hardware and environment

This repository documents a Microsoft **Surface Pro 5**, SKU **Surface_Pro_1796**, running Linux with Omarchy. Results describe this machine unless a component's documentation states otherwise; they are not a blanket compatibility claim for other Surface generations.

The initial desktop setup used **Omarchy 4.0.2**. The kernel checked on 2026-09-11 was **6.19.8-arch1-3-surface**, from package `linux-surface 6.19.8.arch1-3`. Component-specific versions and observations belong in their subsystem documentation so later updates can be tracked independently.

Currently documented:

- [Camera hardware, software versions and validation](../camera/docs/status.md)
- [Camera installation and fixes](../camera/README.md)
- [On-screen keyboard plugin and current configuration](../on-screen-keyboard/README.md)

During keyboard setup documentation, the installed desktop package was confirmed as **Omarchy 4.0.3-1** (2026-09-11).

Kernel override modules must be rebuilt or reassessed when the kernel changes. Always retain component-specific rollback instructions and previous working packages before applying local fixes.
