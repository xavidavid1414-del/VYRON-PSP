# VYRON-PSP

Experimental PSP emulator for the Nintendo 3DS family, targeting the New Nintendo 2DS XL.

> **Development status:** experimental and incomplete. A successful build does not mean commercial PSP games are playable. Real-game compatibility has not yet been demonstrated.

## Current build

The project builds a Nintendo 3DS `.3dsx` application with devkitPro/devkitARM. The current application provides a basic file browser for PSP ISO, PBP, PRX, and ELF files, plus an experimental MIPS execution core.

### Current capabilities

- Nintendo 3DS homebrew application build.
- PSP ISO9660 image inspection and extraction of `PSP_GAME/SYSDIR/EBOOT.BIN`.
- PBP executable-region extraction.
- ELF `ET_EXEC` loading and partial unencrypted PSP PRX loading, including PSPREL1 and compressed PSPREL2 relocation streams.
- A partial MIPS interpreter with delay-slot handling and exception basics.
- Experimental PSP GE software-renderer subset: display-list decoding with bounded jump/call/return flow, 2D points/lines/line strips/triangles/triangle strips/triangle fans/sprites, 8-bit/16-bit/32-bit indexed vertices, interpolated vertex colors, world/view/projection transforms, viewport mapping, depth testing, configurable basic alpha blending and alpha testing, nearest-neighbor and bilinear 5650/5551/4444/8888 texture sampling with MODULATE/DECAL/BLEND/REPLACE/ADD texture functions, basic directional diffuse lighting, face culling and scissor clipping, rendered into the emulator framebuffer.
- Experimental framebuffer output and Nintendo 3DS audio output plumbing.
- Synchronous PSP GE HLE for display-list enqueue/dequeue and list/draw synchronization queries; nonzero stall addresses, callbacks, and asynchronous scheduling are not implemented.
- HLE controller services for PSP controller read/peek calls, registered through a NID-to-syscall resolver for supported imports.
- Experimental PSP user-partition memory blocks with allocation, release, and block-head lookup HLE calls.
- Read-only ISO file services for `disc0:/` and `umd0:/`: open, read, seek, and close, including nested directory paths.
- Unit and integration tests for memory, interpreter, loader, PRX imports, controller and GE HLE dispatch, software graphics, and ELF loading.

### Known limitations

- Encrypted PSP `~PSP` executables are not decrypted.
- CSO-compressed images are not supported; use an uncompressed `.iso` image.
- PRX relocation support includes bounded `PT_PSPREL1` and compressed `PT_PSPREL2` handling for the implemented MIPS relocation types; uncommon relocation variants and complete kernel-module loading remain unsupported.
- HLE coverage is limited to the currently registered controller, user-memory block, read-only ISO file, and basic synchronous GE list NIDs; most PSP kernel/HLE modules and SDK services remain unimplemented.
- Memory block allocation currently supports user partition ID 2 only; kernel partitions and broader heap APIs are not implemented.
- ISO file writes and removable-memory-stick (`ms0:`) access are not implemented. File access is limited to the mounted ISO image.
- CPU instruction coverage and timing are incomplete.
- PSP GE support is an early software subset: mipmaps and indexed texture formats, multiple/point/spot lights, specular materials, fog, geometric clipping, perspective-correct interpolation, PSP EDRAM/framebuffer transfer commands, stencil/color tests, asynchronous GE scheduling, callback interrupts, resumable stall lists, and many display-list commands remain unsupported. Audio decoding/mixing is not implemented as a complete PSP audio pipeline.
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
3. Place an uncompressed PSP `.iso` image, or a supported `.pbp`, `.prx`, or `.elf` file, in `/PSP/GAMES`.
4. Select the file in VYRON-PSP and press **A** to inspect and attempt to load its executable. ISO files are read from `PSP_GAME/SYSDIR/EBOOT.BIN`; PBP, PRX, and ELF files are passed directly to the loader.

At this stage, loading an ISO or ELF does **not** guarantee that the game can boot or run.

### Lower-screen settings menu

- Press **Y** in the file browser to open the lower-screen settings page.
- Use **UP/DOWN** and **A**, or tap a setting with the stylus, to change an option.
- Press **B** to return to the game list.
- Settings are stored in `/PSP/CONFIG/settings.ini`.
- The menu stores preferences for the FPS counter, stylus menu, C-Stick, video fit mode, and individual A/B/X/Y-to-PSP face-button mappings.
- The FPS counter displays the frontend frame rate when enabled. FIT and STRETCH are connected to the top-screen framebuffer presentation.
- The 3DS frontend samples digital and analog input, supports C-Stick selection, and independently maps A/B/X/Y to PSP face buttons.
- `sceCtrlReadBufferPositive` and `sceCtrlPeekBufferPositive` are exposed through HLE/NID resolution. The stylus overlay is not implemented yet.

## Project goal

The long-term goal is to build a usable PSP emulator for the New Nintendo 2DS XL. Reaching that goal requires completing the PSP CPU and memory model, executable/decryption support where legally and technically appropriate, HLE/kernel services, graphics, audio, input, and extensive testing with homebrew software and real hardware.
