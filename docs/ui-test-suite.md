# UI Test Suite

The project now includes a generated macOS UI test bundle at `Tests/MusicToDriveByUITests` with 512 scenario tests.

## Coverage Shape

Each generated test combines five dimensions:

- initial controls window state: auto-visible or hidden at launch
- controls-panel route: toolbar, keyboard shortcut, panel hide cycle, or mixed toolbar plus keyboard flow
- fullscreen route: none, toolbar button, controls-panel button, or keyboard shortcut
- gameplay input probe: none, `C`, `Tab`, `T`, `1`, `2`, `Y`, or `F`
- post-route assertion mode: HUD only or controls-panel inspection

## Current Failure Notes

The suite intentionally marks a few unstable paths with `XCTExpectFailure` so they remain visible without pretending they are production-ready:

- keyboard-driven controls panel toggles
- fullscreen transitions under UI automation
- keyboard delivery into the Metal gameplay surface

## Latest Validation

These checkpoints were completed while landing the suite:

- `swift build`
- `xcodebuild ... build-for-testing`

An actual UI test execution was also attempted with two focused scenarios. The runner failed before the test cases could drive the app:

`MusicToDriveByUITests-Runner encountered an error: Timed out while enabling automation mode.`

Result bundle:

- `/tmp/musictodriveby-uitests-build/Logs/Test/Test-MusicToDriveBy-2026.05.01_12-05-45--0700.xcresult`

That means the suite is compiled and wired, but the local macOS automation environment still needs follow-up before the scenarios can execute end-to-end.
