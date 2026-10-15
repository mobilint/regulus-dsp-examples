# YOLO26 Example

[한국어](README(KR).md)

This example detects objects in a 640x640 image.
The Regulus-RB NPU runs YOLO26 inference.
The SP500 DSP decodes the output tensors and selects the detections with the dual top-k method.
The program draws the detection boxes on a PNG image.

The input image size must be the same as the 640x640 model input.
Use the [letterbox example](../../preprocessing/letterbox) to convert an image that has a different size.

| File | Description |
|---|---|
| `yolo26_demo.cc` | Example source code. |
| `Makefile` | Cross-build settings. |
| `yolo26s.regulus-rb.mxq` | YOLO26s model for Regulus-RB. |
| `test-sample.png` | 640x640 input image. |

Refer to the [repository README](../../README.md) for the common toolchain settings and DSP procedure.

## Build

```shell
make
```

To use a different toolchain, specify its installation path.

```shell
make TOOLCHAIN_ROOT=/opt/crosstools/mobilint/2.0.0/<version>
```

The build creates the AArch64 executable `yolo26_demo`.

## Copy to the Board

Replace `<board>` with the board address.

```shell
ssh root@<board> 'mkdir -p ~/yolo26_demo'
scp yolo26_demo yolo26s.regulus-rb.mxq test-sample.png root@<board>:~/yolo26_demo/
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
cd ~/yolo26_demo
./yolo26_demo yolo26s.regulus-rb.mxq test-sample.png
```

The output is similar to this example:

```text
detections 1, decode 1.325 ms
  person         0.96  [224, 27, 416, 624]
saved yolo26_out.png
```

Exit the board shell after the example is complete.

```shell
exit
```

Each detection line contains the class name, confidence score, and box coordinates.
The box coordinate format is `[x1, y1, x2, y2]`.

## Result

The default output file is `yolo26_out.png`.
The output image contains the detection boxes, class names, and confidence scores.

Run this command on the development PC to copy the result:

```shell
scp root@<board>:~/yolo26_demo/yolo26_out.png .
```
