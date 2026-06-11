# pyNLC Release Notes

Website: https://nolimitconnect.org

Source repository: https://gitlab.com/nolimitconnect/NoLimitConnect.git

## Version 1.1.3

Simplify Network settings
New version before fork to pyNLC a python based version of NoLimitConnect

## Version 1.1.2

Fixed audio streaming being truncated at end and session teardown issues
Fixed android audio record and push to talk

## Version 1.1.1

Version 1.1.1 is a major release focused on everyday reliability, smoother user experience, and stronger cross-platform delivery.

It brings meaningful improvements across networking, media, audio, startup performance, localization, and packaging.

### Highlights

- Expanded language support with bundled translations and in-app language selection.
- Faster startup path and improved startup sequencing.
- Better host/session reliability, including a fix for a join-order issue that could hide members.
- More consistent online and relay status behavior.
- New option to allow or disallow joining multiple hosts at once.
- Improved media playback and viewing across photos, webcam flows, and MJPEG paths.
- Audio pipeline stability improvements, including Android fixes, optional RNNoise, and AEC2 support.
- Better packaging and release flow for Windows, Linux, Flatpak, and signed Android builds.
- GitLab-backed download metadata with stable per-platform index files.

### Known Issues

- Very old GPUs may play audio while showing a solid blue video frame.
- In Visual Studio debug sessions on Windows, an exception may appear during shutdown in graphics-driver code (debug-only behavior).

### Notes

- This release contains many more internal fixes, cleanup changes, and UI improvements than are listed here.


## Previous Releases

### Version 1.0.12

- Many fixes and improvements.
- This version was not compatible with previous versions.

### Version 1.0.8

- First version published on FlatHub.