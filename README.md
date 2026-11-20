# VYRON-PSP

Experimental PSP emulator for the Nintendo 3DS family, targeting the New Nintendo 2DS XL.

> **Development status:** experimental and incomplete. A successful build does not mean commercial PSP games are playable. Real-game compatibility has not yet been demonstrated.

## Current build

The project builds a Nintendo 3DS `.3dsx` application and an installable homebrew `.cia` package with devkitPro/devkitARM. The current application provides a basic file browser for PSP ISO, PBP, PRX, and ELF files, plus an experimental MIPS execution core.

### Phase 8 — ARMv6K native-code generation foundation

**Implemented milestone: ARM instruction encoder plus a limited Allegrex-to-ARM translator.** The ARM-state emitter supports rotated-immediate data-processing instructions, register data-processing instructions, word LDR/STR with signed offsets, and relative B/BL encodings. The translator currently emits native ARM sequences for Allegrex ADDU, SUBU, AND, OR, XOR, NOR, SLT, SLTU, SLL, SRL, SRA, restricted SLTI/SLTIU immediates, full 16-bit LUI and ANDI/ORI/XORI immediate synthesis, and ADDIU with all signed 16-bit immediates (synthesizing constants when needed), plus SLLV/SRLV/SRAV with the MIPS five-bit shift mask and ANDI/ORI/XORI immediate synthesis. It accesses the guest GPR array through the supplied `PSP_CPU *` in ARM register R0 and preserves guest register zero by not writing it.

Unsupported instructions—including branches/delay slots, memory operations, exceptions, and coprocessor instructions—are explicitly rejected for interpreter fallback. Translation output is bounded by the caller-provided buffer, and regression tests check emitted instruction words, register offsets, signed ADDIU, unsupported-opcode fallback, and capacity failures.

**Not yet a functioning JIT:** generated words are not currently executed. The project still needs a verified executable-memory allocation/permission strategy for libctru/3DS, instruction-cache synchronization, a complete ABI-safe native block entry/exit wrapper, branch and delay-slot linking, precise exception/overflow behavior, memory/MMIO helpers, broad Allegrex coverage, and real New Nintendo 2DS XL testing. This milestone does not establish compatibility with commercial games; the project goal remains the entire PSP catalogue, with compatibility to be earned through implementation and testing rather than assumed.
### Phase 7 — Allegrex CPU optimization

**Translated-block milestone: implemented and under automated validation.** The core now caches sequential MIPS instruction words in bounded basic blocks, reuses cached blocks without repeating instruction-memory fetches, preserves the interpreter's delay-slot and exception behavior, and falls back to the interpreter when a block cannot be cached. A per-RAM-page write generation invalidates stale translated blocks when guest code modifies memory. The core run loop now uses this block translator; host tests compare it with the reference interpreter and cover branches, delay slots, execution budgets, and self-modifying code.

This is a portable cached-block interpreter/translator foundation, **not yet a native ARM machine-code JIT**. ARMv6K code emission, executable-memory permissions/cache synchronization, and performance benchmarking on the New Nintendo 2DS XL remain for the next CPU-optimization milestone. Unsupported instructions continue through the existing interpreter.

### Phase 6 — PSP audio output

**Implementation milestone: complete for the documented PCM output subset.** The emulator now has eight independently reserved PSP audio channels, mono/stereo PCM16 input, per-channel format, sample-length and volume controls, left/right panning, saturated stereo mixing, bounded sample queues, and ten audio HLE imports (channel reserve/release, blocking and nonblocking output, panned and standard output, rest-length query, data-length update, format change, and volume change), and a frontend path that submits mixed audio to Nintendo 3DS NDSP. Audio HLE and mixer behavior are covered by host regression tests.

This is not a complete PSP audio stack: compressed audio codecs, sceSas/ATRAC, output2/VAUDIO APIs, accurate blocking/wait timing, streaming callbacks, and per-game audio compatibility remain unsupported. Silence is submitted when no PSP samples are queued; real-hardware sound quality and timing still need testing on the New Nintendo 2DS XL.

### Phase 5 — PSP graphics renderer

**Implementation milestone: complete for the documented software-renderer subset.** The renderer decodes the implemented GE display-list commands, transforms and clips primitives, rasterizes supported primitive types, handles textures/CLUT/mipmaps, depth and alpha/color/stencil tests, blending, lighting/fog, patch tessellation, block transfers, and PSP EDRAM framebuffer presentation. GE address control (BASE/OFFSET/ORIGIN), bounded JUMP/BJUMP/CALL/RET flow, and separate draw/display framebuffers are covered by automated tests.

The full PSP GE is not claimed to be cycle-accurate or fully compatible. Unsupported GE commands, hardware timing, asynchronous list scheduling/callbacks, and broad real-game graphics compatibility remain outside this milestone and are tracked as later compatibility work. The latest GitHub Actions run builds the 3DSX and CIA and passes all configured unit/integration tests.

### Current capabilities

- Nintendo 3DS homebrew application build.
- PSP ISO9660 image inspection and extraction of `PSP_GAME/SYSDIR/EBOOT.BIN`.
- PBP executable-region extraction.
- ELF `ET_EXEC` loading and partial unencrypted PSP PRX loading, including PSPREL1 and compressed PSPREL2 relocation streams.
- A partial MIPS interpreter with delay-slot handling and exception basics, plus a cached MIPS basic-block translator with page-write invalidation and interpreter fallback.
- An ARMv6K instruction-encoding backend foundation for selected ARM-state instructions; generated words are not yet executed.
- Experimental PSP GE software-renderer subset: display-list decoding with bounded jump/call/return flow, 2D points/lines/line strips/triangles/triangle strips/triangle fans/sprites, 8-bit/16-bit/32-bit indexed vertices, interpolated vertex colors, world/view/projection transforms, viewport mapping, depth testing, configurable basic alpha blending and alpha testing, nearest-neighbor and bilinear 5650/5551/4444/8888 texture sampling with MODULATE/DECAL/BLEND/REPLACE/ADD texture functions, up to four directional diffuse lights, experimental point/spot lights with distance/spot attenuation, experimental Blinn-Phong specular materials, view-space fog, clear-mode color/depth writes, masked RGB color testing, face culling, perspective-correct texture interpolation, mipmap selection and filtering, Bezier and B-spline patch tessellation with configurable U/V subdivision and point/line/triangle output, and homogeneous frustum plus framebuffer/scissor clipping, plus experimental stencil tests/operations and 16-bit/32-bit GE block transfers between mapped PSP RAM buffers, rendered into the emulator framebuffer.
- Experimental framebuffer output with a separate PSP display framebuffer, `sceDisplaySetFrameBuf` HLE, mapped 2 MiB PSP EDRAM/VRAM, framebuffer pixel-format conversion (5650/5551/4444/8888), and Nintendo 3DS NDSP audio output.
- Synchronous PSP GE HLE for display-list enqueue/dequeue and list/draw synchronization queries; nonzero stall addresses, callbacks, and asynchronous scheduling are not implemented.
- PSP audio HLE for eight bounded PCM16 mono/stereo channels, standard and panned blocking/nonblocking output, channel reserve/release, rest-length queries, sample-count/format/volume controls, and host-side stereo mixing into Nintendo 3DS NDSP.
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
- CPU instruction coverage and timing are incomplete. Phase 7 uses a portable translated-block cache. Phase 8 adds an ARMv6K encoder and limited Allegrex-to-ARM translation for selected arithmetic, logical, immediate-shift, and variable-shift instructions. Generated words are still not executed: executable-memory management, instruction-cache synchronization, a native block entry/exit wrapper, broad Allegrex translation, and hardware benchmarking remain unimplemented.
- PSP GE support is an early software subset: point/spot lights and Blinn-Phong specular materials are experimental software-renderer implementations and are not yet validated against a broad PSP graphics test suite. The 2 MiB PSP EDRAM/VRAM region is mapped, GE framebuffer commands can target VRAM, and the display HLE can select a separate framebuffer for presentation; complete PSP GE register behavior and hardware timing are not implemented. Asynchronous GE scheduling, callback interrupts, resumable stall lists, and many display-list commands remain unsupported. Bezier and B-spline patch tessellation are newly implemented experimental paths; their floating-point evaluation and edge-case behavior still need comparison against PSP hardware/reference output. GE block transfers and fog/mipmap behavior are partial implementations. The PSP audio implementation is limited to PCM16 channel output and selected sceAudio HLE calls; ATRAC/SAS codecs, output2/VAUDIO, accurate blocking/wait timing, and full PSP audio compatibility remain unsupported.
- Commercial game compatibility is unknown and should not be assumed.

## Build

Build with a devkitPro environment configured for Nintendo 3DS:

```sh
make
```

The build produces `vyron_psp.3dsx` and its `.smdh` metadata. To also package an installable CIA, run:

```sh
make cia
```

This creates `vyron_psp.cia` using the project's ELF, SMDH icon, banner, and homebrew RSF metadata. The CIA is a homebrew build for installation with a compatible title installer; it is not a Nintendo-signed commercial title.

GitHub Actions builds both `.3dsx` and `.cia` outputs and runs the host-side unit/integration tests on pushes to `main`. The combined artifact is named `VYRON-PSP-3DS-BUILDS`.

## Install and use

1. For Homebrew Launcher, copy `vyron_psp.3dsx` to the SD card and launch it there. Alternatively, install `vyron_psp.cia` with FBI or another compatible homebrew title installer and launch VYRON PSP from the HOME Menu.
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
