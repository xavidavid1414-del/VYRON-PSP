# VYRON-PSP

Experimental PSP emulator for the Nintendo 3DS family, targeting the New Nintendo 2DS XL.

> **Development status:** experimental and incomplete. A successful build does not mean commercial PSP games are playable. Real-game compatibility has not yet been demonstrated.

## Current build

The project builds a Nintendo 3DS `.3dsx` application with devkitPro/devkitARM. The current application provides a basic file browser for PSP ISO images and an experimental MIPS execution core.

### Current capabilities

- Nintendo 3DS homebrew application build.
- PSP ISO9660 image inspection and extraction of `PSP_GAME/SYSDIR/EBOOT.BIN`.
- PBP executable-region extraction.
- ELF `ET_EXEC` loading and partial unencrypted PSP PRX loading.
- A partial MIPS interpreter with delay-slot handling and exception basics.
- Experimental framebuffer output and Nintendo 3DS audio output plumbing.
- HLE controller services for PSP controller read/peek calls, registered through a NID-to-syscall resolver for supported imports.
- Experimental PSP user-partition memory blocks with allocation, release, and block-head lookup HLE calls.
- Read-only ISO file services for `disc0:/` and `umd0:/`: open, read, seek, and close, including nested directory paths.\n- Unit and integration tests for memory, interpreter, loader, PRX imports, controller HLE dispatch, and ELF loading.

### Known limitations

- Encrypted PSP `~PSP` executables are not decrypted.
- CSO-compressed images are not supported; use an uncompressed `.iso` image.
- PRX relocation support is partial; `PT_PSPREL2` is unsupported.
- HLE coverage is limited to the currently registered controller, user-memory block, and read-only ISO file NIDs; most PSP kernel/HLE modules and SDK services remain unimplemented.
- Memory block allocation currently supports user partition ID 2 only; kernel partitions and broader heap APIs are not implemented.
- ISO file writes and removable-memory-stick (`ms0:`) access are not implemented. File access is limited to the mounted ISO image.
- CPU instruction coverage and timing are incomplete.
- PSP GPU command processing, display lists, textures, frame synchronization, and audio decoding/mixing are not implemented as a complete PSP graphics/audio pipeline.
- Commercial game compatibility is unknown and should not be assumed.

## Build

Build with a devkitPro environment configured for Nintendo 3DS:

```sh
make
```

The build produces `vyron_psp.3dsx` and its `.smdh` metadata.

GitHub Actions also builds the application and runs the host-side unit/integration tests on pushes to `main`.

## Install and use

1. Copy `vyron_psp.3dsx` to the 3DS SD card and launch it with Homebrew Launcher or an equivalent homebrew method.
2. Launch VYRON-PSP once; it creates the storage folders automatically:
   - `/PSP/GAMES` for uncompressed PSP `.iso` images.
   - `/PSP/SAVES` for future save data.
   - `/PSP/CONFIG` for future settings files.
   - `/PSP/SYSTEM` for internal emulator data.
3. Place an uncompressed PSP `.iso` image in `/PSP/GAMES`.
4. Select the image in VYRON-PSP and press **A** to inspect and attempt to load its executable.

At this stage, loading an ISO or ELF does **not** guarantee that the game can boot or run.

### Lower-screen settings menu

- Press **Y** in the file browser to open the lower-screen settings page.
- Use **UP/DOWN** and **A**, or tap a setting with the stylus, to change an option.
- Press **B** to return to the game list.
- Settings are stored in `/PSP/CONFIG/settings.ini`.
- The menu stores preferences for the FPS counter, stylus menu, C-Stick, video fit mode, and individual A/B/X/Y-to-PSP face-button mappings.
- The FPS counter displays the frontend frame rate when enabled. FIT and STRETCH are connected to the top-screen framebuffer presentation.
- The 3DS frontend now samples held buttons and maps them to PSP controller bit values. A/B and X/Y swaps are applied, and the controller module tracks pressed/released transitions.
- **Input limitation:** this controller state is not yet exposed to PSP software. PSP imports such as `sceCtrlReadBufferPositive` and `sceCtrlPeekBufferPositive` plus their HLE/NID resolution are still missing. C-Stick analog input and the stylus overlay are also not implemented.

## Project goal

The long-term goal is to build a usable PSP emulator for the New Nintendo 2DS XL. Reaching that goal requires completing the PSP CPU and memory model, executable/decryption support where legally and technically appropriate, HLE/kernel services, graphics, audio, input, and extensive testing with homebrew software and real hardware.
