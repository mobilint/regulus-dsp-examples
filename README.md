# SP500 Vision DSP Examples

[한국어](README(KR).md)

This repository provides examples that use the SP500 Vision DSP in Regulus-RB.
SP500 runs image and signal processing operations.
The host application controls SP500 through the C API in `libsp500.so`.

## Examples

| Category | Example | Function |
|---|---|---|
| Preprocessing | [letterbox](preprocessing/letterbox) | Resizes an image without changing its aspect ratio and adds borders. |
| Preprocessing | [log-mel](preprocessing/log-mel) | Converts PCM audio into a log-mel spectrogram. |
| Postprocessing | [YOLO11](postprocessing/yolo11) | Converts YOLO11 output tensors into detection results. |
| Postprocessing | [YOLO26](postprocessing/yolo26) | Converts YOLO26 output tensors into detection results. |
| Postprocessing | [YOLOv8](postprocessing/yolov8) | Converts YOLOv8 output tensors into detection results. |

Refer to the [SP500 API Reference](docs/sp500-api-reference.md) for function parameters, return values, and limitations.

## Toolchain

If the toolchain is not installed, download the Regulus-RB toolchain from the [Mobilint Image Archive](https://dl.mobilint.com).
Extract the archive and run the installation script.

```shell
unset LD_LIBRARY_PATH
sh install-regulus-toolchain.sh -y
```

The Makefile in each example selects the latest installed toolchain that has the environment setup file in `/opt/crosstools/mobilint/2.0.0`.
It then checks for the development files that the example requires.
The preprocessing examples check for the SP500 header and library.
The YOLO examples check for the SP500 and qbruntime headers and libraries.
If a required file is missing, the build stops and tells you to set `TOOLCHAIN_ROOT`.
Go to the example directory and run `make`.

```shell
cd preprocessing/letterbox
make
```

To use a different toolchain, specify its installation path.

```shell
make TOOLCHAIN_ROOT=/opt/crosstools/mobilint/2.0.0/<version>
```

`TOOLCHAIN_ROOT` must contain the `environment-setup-cortexa53-mobilint-linux` file and all development files that the example requires.

## Load the DSP Firmware

Run this command one time after the board starts.

```shell
dsp_firmware_load
```

This command loads the SP500 firmware and starts the DSP application.
The firmware remains loaded until you turn off or restart the board.

The command uses these files:

```text
/usr/lib/firmware/mobilint/sp500/ptcm.bin
/usr/lib/firmware/mobilint/sp500/dtcm.bin
/usr/lib/firmware/mobilint/sp500/external.bin
```

After the firmware starts, the host application can call the SP500 API.

## SP500 API

The library and header files are at these paths on Regulus-RB:

```text
/usr/lib/libsp500.so
/usr/include/sp500/dsp.h
/usr/include/sp500/letterbox.h
/usr/include/sp500/log_mel.h
/usr/include/sp500/yolo.h
```

Each module header includes `dsp.h`.
An application only needs the header for the module that it uses.
The public functions use C linkage and can be called from C and C++.

### Common API

| Function | API | Description |
|---|---|---|
| Status string | `sp500_status_string()` | Converts an `sp500_status` value into a string. |
| DSP connection | `sp500_dsp_open()` | Prepares the DSP connection and shared memory mapping. |
| Call timeout | `sp500_set_dsp_timeout_ms()` | Sets the timeout for one DSP call. |
| Call lock | `sp500_dsp_lock()`, `sp500_dsp_unlock()` | Keeps one lock active for multiple DSP calls. |
| General buffer | `sp500_buffer_wrap()` | Wraps application memory in an SP500 buffer. |
| Shared buffer | `sp500_buffer_alloc()`, `sp500_buffer_release()` | Allocates and releases shared memory that the DSP and NPU can access. |

A function that returns `sp500_status` is successful when it returns `SP500_OK`.
A negative value indicates an error.

### Module API

| Module | API | Input | Output |
|---|---|---|---|
| Letterbox | `sp500_letterbox()` | 8-bit HWC image | A resized image with borders and coordinate restoration values. |
| Log-mel | `sp500_log_mel()` | float32 PCM audio | A log-mel spectrogram in the Whisper format. |
| YOLO | `sp500_yolo_decode()` | YOLO head output tensors | Results after decoding, confidence filtering, and duplicate removal. |

The YOLO API supports the `detect`, `segment`, and `pose` tasks.
Support depends on the head family.
`sp500_yolo_head_for_model()` converts a model name into a head family.

## API Call Sequence

1. Call `sp500_dsp_open(0)`.
2. Initialize the parameter structure with `*_params_init()`.
3. Set the necessary parameters.
4. Prepare the input and output `sp500_buffer` objects.
5. Call the module function.
6. Make sure that the return value is `SP500_OK`.
7. Release each buffer from `sp500_buffer_alloc()` with `sp500_buffer_release()`.

## Build a User Application

Apply the toolchain environment and link `libsp500.so`.

```shell
source <TOOLCHAIN_ROOT>/environment-setup-cortexa53-mobilint-linux
$CXX main.cc -lsp500 -o main
```

Load the firmware on the board and run the application.

```shell
dsp_firmware_load
./main
```
