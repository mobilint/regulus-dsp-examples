# SP500 Letterbox Example

[한국어](README(KR).md)

This example converts an input image into a 640x640 letterbox image.
The SP500 DSP resizes the image and adds borders.
The program saves the result as a PNG file.

| File | Description |
|---|---|
| `letterbox_demo.cc` | Example source code. |
| `Makefile` | Cross-build settings. |
| `test-sample-1080p.png` | 1920x1080 input image. |

Refer to the [repository README](../../README.md) for the common toolchain settings and DSP procedure.

## Build

The Makefile selects the latest installed toolchain that has the environment setup file.
It then checks for the required SP500 files.
If a required file is missing, the build stops and tells you to set `TOOLCHAIN_ROOT`.

```shell
make
```

To use a different toolchain, specify its installation path.

```shell
make TOOLCHAIN_ROOT=/opt/crosstools/mobilint/2.0.0/<version>
```

The build creates the AArch64 executable `letterbox_demo`.

## Copy to the Board

Replace `<board>` with the board address.

```shell
ssh root@<board> 'mkdir -p ~/letterbox_demo'
scp letterbox_demo test-sample-1080p.png root@<board>:~/letterbox_demo/
```

## Load the DSP Firmware

Run `dsp_firmware_load` one time after the board starts.
If the firmware is already loaded, do not run the command again.

```shell
ssh root@<board>
dsp_firmware_load
```

## Run

```shell
cd ~/letterbox_demo
./letterbox_demo test-sample-1080p.png
```

The output is similar to this example:

```text
input 1920x1080 -> output 640x640
scale 0.3333 pad_x 0 pad_y 140 dsp 6.163 ms
saved letterbox_out.png
```

You can specify the output file name as the second argument.

```shell
./letterbox_demo test-sample-1080p.png out.png
```

Exit the board shell after the example is complete.

```shell
exit
```

## Result

The default output file is `letterbox_out.png`.
The program resizes the 1920x1080 input to 640x360.
It puts the image in the center of the 640x640 output.
The top and bottom borders are each 140 pixels high.
The pixel value of the borders is 114.

Run this command on the development PC to copy the result:

```shell
scp root@<board>:~/letterbox_demo/letterbox_out.png .
```
