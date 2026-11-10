# Client binaries withdrawn following Defender detection

A user reported Windows Security blocking and removing
`Titanium-Emporer-0.3-Windows.zip` as **Trojan:Script/Sabsik.FL.A!ml**, severity Severe.
The screenshot identifies the archive/download, not an individual archive member.

Leave the removed file removed. Do not restore it, disable protection, or add an
antivirus exclusion to run it. The detection has not been confirmed as a false
positive. No replacement binary is supplied.

The 0.3 archive and the earlier 0.2 client archive have been removed from the
current repository tree and download links. The 0.2 removal is precautionary; no
0.2 detection was reported. Historical Git commits can still contain the old files;
this withdrawal does not erase Git history. Resource packs are separate artifacts
and were not identified in the reported alert.

## Bounded local audit

- ZIP integrity checks pass on the local 0.3 archive.
- Every packaged source file matches the corresponding checkout file.
- Rebuilding the EXE and DLL with the recorded Zig 0.15.2 compiler gives the same
  SHA-256 hashes as the packaged binaries.
- The reviewed injector source performs `VirtualAllocEx`, `WriteProcessMemory`,
  `CreateRemoteThread` and DLL loading in `Minecraft.Windows.exe`. These injection
  operations are security-sensitive and can trigger detection.
- No downloader, persistence mechanism, protection exclusion, or arbitrary
  attacker-command facility was found in the reviewed project source/scripts.
  The included PowerShell script runs local CMake build/test commands.
- Imported DLLs inspected in these local binaries are Windows API and CRT DLLs.
- No Horion source, SDK, binary, or other third-party cheat client code is packaged.

These observations are **not a malware-clearance result**. No Windows/Defender
scan or dynamic Windows execution was performed here. Matching a local rebuild
does not establish safety, the reason for detection, or that the user's downloaded
bytes exactly matched the local archive. Do not recover a removed file merely to
compare its hash. No sample has been submitted to an external scanning service.

## Local artifact identifiers for investigation

- ZIP: `2e8b10d05827bb729794bff74610a94a3b90d9fd70c24602c28907d1134b5f07`
- EXE: `a507fe3576d1f78d7a0f1e0c0d155517b6b185ac6b18eb232aef706adaabeae5`
- DLL: `22dbd6c21c8a20cb56ba38f1a42536cbc0f2c514ee3274ed5b4a534a55e2c497`

If the user already ran an extracted executable, use Windows Security to run a
full scan and review Protection History for additional detections. A removed
archive alone does not prove that malware executed or that the computer is infected.
