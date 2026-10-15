# SP500 Log-mel Example

[한국어](README(KR).md)

This example converts a 16 kHz mono WAV file into a log-mel spectrogram.
The SP500 DSP calculates the log-mel values.
The program saves the result as a PNG file.

| File | Description |
|---|---|
| `log_mel_demo.cc` | Example source code. |
| `Makefile` | Cross-build settings. |
| `test-sample.wav` | 16 kHz mono 16-bit PCM input file. |

The duration of `test-sample.wav` is approximately 24 seconds.
The file contains NASA Apollo 11 communication audio.
The source is the public-domain file from [Wikimedia Commons](https://commons.wikimedia.org/wiki/File:Neil_Armstrong_small_step.wav).
The source file was converted to 16 kHz mono 16-bit PCM.

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

The build creates the AArch64 executable `log_mel_demo`.

## Copy to the Board

Replace `<board>` with the board address.

```shell
ssh root@<board> 'mkdir -p ~/log_mel_demo'
scp log_mel_demo test-sample.wav root@<board>:~/log_mel_demo/
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
cd ~/log_mel_demo
./log_mel_demo test-sample.wav
```

The output is similar to this example:

```text
385803 samples, 24.11 s
2411 frames x 80 mels, dsp 42.971 ms
saved log_mel_out.png (2411x320)
```

Exit the board shell after the example is complete.

```shell
exit
```

## Result

The default output file is `log_mel_out.png`.
The interval between frames is 10 ms.
The frame hop length is 160 samples.
The output has 80 mel bands.

The horizontal axis of the image is time.
One frame is one pixel wide.
The vertical axis is the mel band.
One band is four pixels high.
The lower frequencies are at the bottom.
Bright pixels indicate large values.

Run this command on the development PC to copy the result:

```shell
scp root@<board>:~/log_mel_demo/log_mel_out.png .
```
