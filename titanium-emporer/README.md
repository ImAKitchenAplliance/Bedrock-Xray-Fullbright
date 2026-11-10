# Titanium Emporer

Windows-only native launcher and diagnostic injection prototype, formerly Project
Middle. **Game features are not implemented.** X-ray, No Fall and Fly are listed
as unavailable; there is no game adapter, render hook, in-game menu, or verified
Realm/local/server classifier. The new interface is the standalone launcher.

## Interface

- Orange-to-black gradient header and orange gradient primary action.
- Black panels, warm white type, orange accents and a dark Windows title bar where
  supported by the OS.
- Exactly three feature cards: X-ray, No Fall and Fly, all marked unavailable.
- Read build, Attach diagnostic, Detach and Copy report controls.
- Background diagnostic operations keep the UI responsive. Closing waits for an
  outstanding operation to finish, and keyboard tab navigation is supported.
- System DPI scaling and a resizable window with minimum layout dimensions.

## Run on Windows x64

Extract the entire ZIP, keep `Titanium-Emporer.exe` and `titanium-client.dll`
together, and open `Titanium-Emporer.exe`. It requires native Windows x64.
No iOS or behavior-pack component is included.

Start Bedrock and stay at its title screen. **Read build** is read-only and reports
the Minecraft process ID and executable file version. It does not attach the DLL
or transmit information. Copy report copies only the displayed report locally.

**Attach diagnostic** asks Windows to load this project's DLL into the single
`Minecraft.Windows.exe` process and calls its diagnostic export. It does not
install game hooks or enable gameplay features. The app explains this before
attachment. **Detach** unloads this hook-free diagnostic DLL. Repeated attachment
is rejected; mismatched DLL paths are rejected. No other target process, arbitrary
DLL picker, automatic injection, privilege escalation, policy bypass or persistence
is included. Windows/Minecraft policy may reject DLL loading; errors are reported.

Injection and the graphical interface have not been runtime-tested in Windows.
Start with Read build. If a remote operation times out, restart Minecraft before
retrying: a remote thread may still complete, and its argument cannot be freed
safely while it is in use. Future hooked versions will require game-thread cleanup
before unloading; the current Detach implementation is only for this inert DLL.

## Intended gameplay scope

Local worlds and Realms only; dedicated servers are excluded. A portable activation
contract defaults to denying unknown builds, unknown sessions and unknown world
identities. It supports only X-ray, No Fall and Fly. World changes or loss of
verified evidence remove active changes. This contract is unit-tested with a fake
backend, but it is **not yet connected to the real game**. No game build is supported
at present. The previously reported v26.60 title-screen version is a starting point,
not a verified native ABI. Realms can reject client movement changes and compute
fall damage themselves; injection alone does not establish working Fly or No Fall.

## Build

Windows: Visual Studio 2022 Desktop development with C++, Windows SDK and CMake.
Run `scripts/build-windows.ps1` in a developer PowerShell. Outputs are in
`build-windows/Release`. The MSVC/CMake route is provided but has not been run here.

Cross-build on Linux with Zig 0.15.2:

```sh
ZIG_GLOBAL_CACHE_DIR=/tmp/titanium-zig-cache ./scripts/build-cross.sh /path/to/zig
python3 tools/package.py out
```

Native runtime contract tests:

```sh
g++ -std=c++17 -Wall -Wextra -Werror -pedantic -I include tests/runtime_test.cpp -o /tmp/titanium-tests
/tmp/titanium-tests
```

Validation completed here: runtime contract tests, x64 Windows cross-compilation,
PE GUI-subsystem verification, DLL export/import inspection, and archive integrity.
These checks do not establish that the interface or injection works in Minecraft.
