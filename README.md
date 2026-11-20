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
- Unit and integration tests for memory, interpreter, loader, and ELF loading.

### Known limitations

- Encrypted PSP `~PSP` executables are not decrypted.
- CSO-compressed images are not supported; use an uncompressed `.iso` image.
- PRX relocation support is partial; `PT_PSPREL2` is unsupported.
- PSP kernel/HLE modules, system services, imports, and much of the PSP SDK environment are not implemented.
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

## Project goal

The long-term goal is to build a usable PSP emulator for the New Nintendo 2DS XL. Reaching that goal requires completing the PSP CPU and memory model, executable/decryption support where legally and technically appropriate, HLE/kernel services, graphics, audio, input, and extensive testing with homebrew software and real hardware.
