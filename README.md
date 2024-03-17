# WindowsScreenCapture
Capture window or monitor using 'Windows Graphics Capture' from the command line.

![Sample](https://github.com/EX-EXE/WindowsScreenCapture/assets/114784289/f87d27c3-f03c-4546-ac8e-c281d209a349)

## Arguments

|Argument|Description|Example|
|-|-|-|
| /Output: | Specify the destination for saving captured data. | /Output: "C://OutputDir/Output.png" |
| /Format: | Specifies the image format.(Default: png)[png/bmp/dds] | /Format: png |
| /ProcessId: | This is the Process ID (PID) of the target for capture. | /ProcessId: 10000 |
| /ProcessName: | This is the Process Name of the target for capture. | /ProcessName: Taskmgr.exe |
| /Monitor: | This is the Monitor Index of the target for capture. | /Monitor: 0 |
| /CaptureCursor: | This specifies whether to include the mouse cursor in the capture. (Default: Disable) | /CaptureCursor: Enable |
| /TimeoutSec: | This specifies the timeout duration for capturing (in seconds) | /TimeoutSec: 10.0 |
| /Licence: | Displays the license of the library being used. | /Licence: |

## Packages
- [DirectXTK](https://github.com/microsoft/DirectXTK)
