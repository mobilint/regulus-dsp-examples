# YOLOv8 Example

[한국어](README(KR).md)

This example detects objects in a 640x640 image.
The Regulus-RB NPU runs YOLOv8 inference.
The SP500 DSP decodes the output tensors and removes duplicate detections.
The program draws the detection boxes on a PNG image.

The input image size must be the same as the 640x640 model input.
Use the [letterbox example](../../preprocessing/letterbox) to convert an image that has a different size.

| File | Description |
|---|---|
| `yolov8_demo.cc` | Example source code. |
| `Makefile` | Cross-build settings. |
| `yolov8n.regulus-rb.mxq` | YOLOv8n model for Regulus-RB. |
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

The build creates the AArch64 executable `yolov8_demo`.

## Copy to the Board

Replace `<board>` with the board address.

```shell
ssh root@<board> 'mkdir -p ~/yolov8_demo'
scp yolov8_demo yolov8n.regulus-rb.mxq test-sample.png root@<board>:~/yolov8_demo/
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
cd ~/yolov8_demo
./yolov8_demo yolov8n.regulus-rb.mxq test-sample.png
```

The output is similar to this example:

```text
detections 1, decode 1.853 ms
  person         0.73  [236, 34, 420, 617]
saved yolov8_out.png
```

Exit the board shell after the example is complete.

```shell
exit
```

Each detection line contains the class name, confidence score, and box coordinates.
The box coordinate format is `[x1, y1, x2, y2]`.

## Result

The default output file is `yolov8_out.png`.
The output image contains the detection boxes, class names, and confidence scores.

Run this command on the development PC to copy the result:

```shell
scp root@<board>:~/yolov8_demo/yolov8_out.png .
```
