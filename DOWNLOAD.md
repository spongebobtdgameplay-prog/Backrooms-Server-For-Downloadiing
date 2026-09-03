# Backrooms Offical — Windows

The public Windows download and the in-game updater use the **same file**:

https://github.com/spongebobtdgameplay-prog/Backrooms-Server-For-Downloadiing/releases/download/latest-windows/Backrooms-Offical-Windows-x64.exe

Filename:

\`Backrooms-Offical-Windows-x64.exe\`

It is a real Windows installer, not a ZIP. The installer uses the game's existing \`assets/icon/Backrooms.ico\`, so the download itself has the Backrooms Offical game icon.

The installer contains:

- \`Backrooms Offical.exe\`
- \`SDL3.dll\`
- the complete \`assets\` folder

The publish workflow replaces this stable release asset only after a successful Windows build and then updates \`update/latest.txt\` with the exact version and SHA-256 hash.
