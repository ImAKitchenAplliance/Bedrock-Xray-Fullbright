# Titanium Emporer — client binaries withdrawn

**Windows Defender blocked the 0.3 ZIP as Trojan:Script/Sabsik.FL.A!ml.**
Current binary downloads are withdrawn, including 0.2 as a precaution. Do not
restore blocked files or disable/exclude protection to run this project.
See [SECURITY-NOTICE.md](SECURITY-NOTICE.md) for evidence and limitations.
The implementation/build notes below are retained for source review, not as a
recommendation to run or rebuild the flagged client. No replacement is supplied.

## Withdrawn 0.3 implementation

Windows x64 client, formerly Project Middle. The injector now loads a DLL that
creates an in-process Win32 overlay over the Minecraft window. No resource pack or
behavior pack is used. **Windows/Minecraft execution is still unverified.**

## What this revision does

- Draws an orange-to-black header with **MINECRAFT** and **TITANIUM EDITION** beneath it.
- Draws a matching Titanium Emporer panel over the game window.
- Insert shows/hides the injected UI while Minecraft has focus.
- The overlay follows the game window, hides when the game loses focus, and passes
  mouse input through to Minecraft. It is intended for windowed/borderless mode;
  exclusive fullscreen may not display it.
- Detach asks the injected worker to stop and waits for termination before
  unloading. If shutdown fails, the DLL is left loaded and an error is shown.

This is a branding **overlay**, not a modification of Minecraft's native UI
resource tree or native buttons. Native menus/buttons are not recolored. Header
placement is approximate and may cover the vanilla logo at some UI scales.
The feature panel is informational; it has no enabled gameplay toggles.

## What remains unfinished

**X-ray, No Fall and Fly are not implemented.** No game-memory, rendering or packet
hooks are installed. The runtime activation contract remains disconnected from
Minecraft. Realm/local/server detection is also not implemented, so gameplay
features cannot activate. No Fall/Fly on Realms additionally depend on what the
Realm's authoritative simulation accepts.

The user's **Read build** output identified file version **1.26.63.1**. That verifies
identity inspection only, not attachment. The earlier title-screen version was
v26.60. No native game ABI is verified for either. See `RESEARCH.md` for the feature
investigation and the missing prerequisites; this UI revision is not a claim of a
working cheat client.

## Install and test

1. **Restart Minecraft first** to clear any earlier Titanium DLL. Do not mix the
   0.2 executable with the 0.3 DLL or extract over files still loaded by Minecraft.
2. Extract the whole new ZIP to a new folder. Keep `Titanium-Emporer.exe` and
   `titanium-client.dll` together.
3. Open Windows Bedrock and select windowed or borderless mode. Stay at its title
   screen for the first attachment test.
4. Open the launcher, choose **Inject UI**, then return focus to Minecraft. Success
   means the DLL's UI initialization answered; it does not enable gameplay features.
5. Check for the orange/black header and **TITANIUM EDITION**, press Insert twice,
   resize/move the game window, and Alt-Tab away/back. The overlay should track the
   window and be hidden over other apps. Mouse clicks should still reach Minecraft.
6. Return to the launcher and choose **Detach**. The overlay should disappear.

Send the **Inject UI report** and a game screenshot if it fails. A Read build report
alone cannot diagnose injection or UI creation. Startup codes: 40 = no suitable
same-process game window found; 41 = Win32 overlay setup failed; 42 = worker/event
creation failed; 43 = startup timeout; 44 = UI worker already running. Stop code
45 means shutdown did not finish. Restart Minecraft before retrying after a timeout.
An old DLL without the new exports is rejected and requires a game restart.

The injector only targets a single native x64 `Minecraft.Windows.exe`. It uses
ordinary Windows DLL loading with no driver, privilege escalation, loader-policy
bypass, persistence or automatic attachment. Windows/Minecraft may reject loading;
that is reported, not bypassed. Closing the launcher does **not** detach the DLL;
use Detach first. On timeout, outstanding remote work may still finish, so its path
buffer is retained rather than freed while in use.

## Build

Windows: Visual Studio 2022 Desktop development with C++, Windows SDK and CMake.
Run `scripts/build-windows.ps1` in developer PowerShell. Outputs are under
`build-windows/Release`. This route has not been run here.

Linux cross-build with Zig 0.15.2:

```sh
ZIG_GLOBAL_CACHE_DIR=/tmp/titanium-zig-cache ./scripts/build-cross.sh /path/to/zig
python3 tools/package.py out
```

Portable activation-contract tests:

```sh
g++ -std=c++17 -Wall -Wextra -Werror -pedantic -I include tests/runtime_test.cpp -o /tmp/titanium-tests
/tmp/titanium-tests
```

Validation here covers compilation, runtime contract tests, PE subsystem/exports,
and ZIP integrity. It does not validate Windows UI execution, injection into
Minecraft, native-menu theming, gameplay, or session classification.
