# SP500 API Reference

[한국어](sp500-api-reference(KR).md)

This document describes `libsp500.so` version 1.0.0.

Use these public headers:

```text
sp500/dsp.h
sp500/letterbox.h
sp500/log_mel.h
sp500/yolo.h
```

Each module header includes `sp500/dsp.h`.
All public declarations use `extern "C"` linkage.
The public headers use `<stdint.h>` and `<stddef.h>`.

## Common Rules

- Functions that return status use `sp500_status`.
- Initialize a parameter structure with `*_params_init()` before you use it.
- Set the necessary fields after initialization.
- Keep all `reserved` fields at 0.
- You can call the API from multiple threads and multiple processes.
- The system executes one DSP call at a time.

## `sp500/dsp.h`

### Status Codes

The type of `sp500_status` is `int32_t`.

| Code | Value | Meaning |
|---|---:|---|
| `SP500_OK` | 0 | The call was successful. The output buffer is valid. |
| `SP500_ERR_INVALID_ARG` | -1 | An argument is missing, out of range, or has an invalid combination. |
| `SP500_ERR_UNSUPPORTED` | -2 | The module does not support the requested combination. |
| `SP500_ERR_BUFFER_TOO_SMALL` | -3 | The buffer is too small. |
| `SP500_ERR_NOT_READY` | -4 | `sp500_dsp_open()` is not complete. |
| `SP500_ERR_NO_MEMORY` | -5 | The library cannot allocate host memory or shared memory. |
| `SP500_ERR_TIMEOUT` | -6 | The DSP did not respond before the timeout. |
| `SP500_ERR_DSP` | -7 | The DSP returned an error. |
| `SP500_ERR_INTERNAL` | -8 | An internal error occurred. |

```c
const char* sp500_status_string(sp500_status status);
```

This function returns the name of a status code.
It does not return `NULL`.

### Version

| Macro | Value |
|---|---:|
| `SP500_VERSION_MAJOR` | 1 |
| `SP500_VERSION_MINOR` | 0 |
| `SP500_VERSION_PATCH` | 0 |

`SP500_VERSION_INT` uses this expression:

```text
major * 10000 + minor * 100 + patch
```

```c
uint32_t sp500_version(void);
```

This function returns the version of the loaded library.
The return value uses the `SP500_VERSION_INT` format.

```c
const char* sp500_version_string(void);
```

This function returns the version of the loaded library as a string.
The string format is `major.minor.patch`.

### DSP Connection

| Constant | Value |
|---|---:|
| `SP500_DEFAULT_OPEN_TIMEOUT_MS` | 10000 |
| `SP500_DEFAULT_DSP_TIMEOUT_MS` | 3000 |

```c
sp500_status sp500_dsp_open(uint32_t timeout_ms);
```

This function connects to the DSP.
It maps the shared memory window.
The function waits if the DSP firmware is not ready.

`timeout_ms` is the connection timeout in milliseconds.
A value of 0 uses `SP500_DEFAULT_OPEN_TIMEOUT_MS`.

After the first successful call, the function does not make the connection again.

This function can return these status codes:

- `SP500_OK`
- `SP500_ERR_TIMEOUT`
- `SP500_ERR_NOT_READY`

```c
int sp500_dsp_is_open(void);
```

This function returns 1 if the DSP connection is ready.
It returns 0 if the connection is not ready.
The function does not wait.

```c
void sp500_set_dsp_timeout_ms(uint32_t timeout_ms);
```

This function sets the timeout for subsequent DSP calls.
The unit is milliseconds.

A value of 0 uses `SP500_DEFAULT_DSP_TIMEOUT_MS`.
A value less than the default also changes to the default.

This timeout does not cancel the DSP operation.
The DSP can continue the current operation after the timeout.
In this condition, a subsequent call can receive the result of the previous call.
If a timeout error occurs, reload the DSP firmware.
Do not retry the call repeatedly.

```c
uint32_t sp500_get_dsp_timeout_ms(void);
```

This function returns the current DSP call timeout in milliseconds.

### DSP Lock

```c
void sp500_dsp_lock(void);
```

This function acquires the recursive DSP lock.
The lock is valid between threads and processes.
Each DSP call also uses this lock internally.

```c
void sp500_dsp_unlock(void);
```

This function releases one level of the DSP lock.
Each `sp500_dsp_lock()` call requires one `sp500_dsp_unlock()` call.
If the number of calls does not match, DSP calls and shared memory allocation can stop in all processes.

### Shared Memory

| Constant | Value | Meaning |
|---|---:|---|
| `SP500_ALIGN_DSP` | 32 | Minimum alignment for a DSP buffer. |
| `SP500_ALIGN_NPU` | 4096 | Alignment for a buffer that the NPU accesses. |

```c
void* sp500_shmem_alloc(uint32_t size, uint32_t alignment);
```

This function allocates memory in the shared memory window.
`alignment` must be equal to or greater than `SP500_ALIGN_DSP`.
The function returns `NULL` if the allocation fails.
It also returns `NULL` if there is no DSP connection.

The allocator uses the first free space that is large enough.
The allocator does not compact free space.
Repeated allocation of buffers with different sizes and lifetimes can fragment memory.
Allocate the necessary buffers one time, and reuse them.

Do not store a shared memory buffer in a global object or a static object.
The object destructor can release the buffer after link termination.

```c
void sp500_shmem_free(void* ptr);
```

This function releases shared memory.
If `ptr` is `NULL`, the function does not do an operation.

```c
int sp500_shmem_contains(const void* ptr);
```

This function returns 1 if `ptr` is in the shared memory window.
An internal pointer in an allocated area is also valid.
The function returns 0 before a call to `sp500_dsp_open()`.

```c
uint64_t sp500_shmem_phys_addr(const void* ptr);
```

This function returns the physical address of a shared memory pointer.
It returns 0 if the pointer is outside the shared memory window.

### Buffers

```c
typedef struct sp500_buffer {
    void*    data;
    uint32_t size;
    uint32_t flags;
} sp500_buffer;
```

| Field | Meaning |
|---|---|
| `data` | Host virtual address. A value of `NULL` means that the buffer is not used. |
| `size` | Buffer size in bytes. |
| `flags` | Buffer properties. |

| Flag | Value | Meaning |
|---|---:|---|
| `SP500_BUFFER_SHARED` | `1 << 0` | The buffer is in the shared memory window. The DSP accesses the buffer without a copy. |

```c
sp500_buffer sp500_buffer_none(void);
```

This function returns an unused buffer value.

```c
sp500_buffer sp500_buffer_alloc(uint32_t size, uint32_t alignment);
```

This function allocates a buffer in shared memory.
The returned buffer has the `SP500_BUFFER_SHARED` flag.
If the allocation fails, `data` is `NULL`.

```c
sp500_buffer sp500_buffer_wrap(void* data, uint32_t size);
```

This function wraps application memory in an SP500 buffer.
For an input buffer, the DSP call copies the data to shared memory one time.
For an output buffer, the DSP call copies the data to application memory one time.

```c
void sp500_buffer_release(sp500_buffer* buffer);
```

If the buffer has the `SP500_BUFFER_SHARED` flag, this function releases the shared memory.
Then, it initializes the buffer to the unused state.

## `sp500/letterbox.h`

The Letterbox module processes 8-bit interleaved HWC images.
The module keeps the aspect ratio of the source image.
It fills the remaining area with `pad_value`.
There must be no additional padding at the end of a row.

The module uses this scale:

```text
scale = min(dst_w / src_w, dst_h / src_h)
```

Use these expressions to convert output coordinates to source coordinates:

```text
src_x = (dst_x - pad_x) / scale
src_y = (dst_y - pad_y) / scale
```

### Interpolation Methods

| Constant | Value | Meaning |
|---|---:|---|
| `SP500_RESIZE_NEAREST` | 0 | Nearest-neighbor interpolation. |
| `SP500_RESIZE_BILINEAR` | 1 | Bilinear interpolation. |
| `SP500_RESIZE_BICUBIC` | 2 | Bicubic interpolation. `a` is -0.75. |
| `SP500_RESIZE_LANCZOS` | 3 | Lanczos4 interpolation. It uses 8 taps. |

### `sp500_letterbox_params`

| Field | Type | Default | Condition and meaning |
|---|---|---|---|
| `src_w`, `src_h` | `int32_t` | None | Required. The range is 1 through 65535. |
| `dst_w`, `dst_h` | `int32_t` | 640 | Output size. The range is 1 through 65535. |
| `channels` | `int32_t` | 3 | Number of interleaved channels. |
| `interp` | `int32_t` | `SP500_RESIZE_BILINEAR` | Use one of the `SP500_RESIZE_*` values. |
| `pad_value` | `uint8_t` | 114 | Border value for all channels. |
| `center` | `uint8_t` | 1 | A value of 1 distributes the border on both sides. A value of 0 adds a border only on the right and bottom. |
| `scale_up` | `uint8_t` | 1 | A value of 1 permits upscaling. A value of 0 limits the scale to 1.0 or less. |
| `swap_rb` | `uint8_t` | 0 | A value of 1 swaps channel 0 and channel 2. This value requires three channels. |
| `reserved[4]` | `uint32_t` | 0 | Do not change the value. |

```c
void sp500_letterbox_params_init(sp500_letterbox_params* params);
```

Use this function to initialize the parameters to their default values.

### `sp500_letterbox_result`

| Field | Type | Meaning |
|---|---|---|
| `scale` | `float` | Applied scale. |
| `pad_x`, `pad_y` | `int32_t` | Left and top border sizes in pixels. |
| `host_seconds` | `double` | Round-trip time of the DSP call in seconds. |
| `reserved[4]` | `uint32_t` | Not used. |

### `sp500_letterbox()`

```c
sp500_status sp500_letterbox(const sp500_buffer* src,
                             const sp500_letterbox_params* params,
                             sp500_buffer* dst,
                             sp500_letterbox_result* result);
```

| Argument | Condition and meaning |
|---|---|
| `src` | Input buffer. Its size must be at least `src_w * src_h * channels` bytes. |
| `params` | Letterbox parameters. |
| `dst` | Output buffer. Its size must be at least `dst_w * dst_h * channels` bytes. |
| `result` | Receives the scale and border information. You can use `NULL`. |

This function can return these status codes:

- `SP500_OK`
- `SP500_ERR_INVALID_ARG`
- `SP500_ERR_BUFFER_TOO_SMALL`
- `SP500_ERR_UNSUPPORTED`
- `SP500_ERR_NO_MEMORY`
- `SP500_ERR_NOT_READY`
- `SP500_ERR_TIMEOUT`
- `SP500_ERR_DSP`

If `interp` is outside the supported range, the function returns `SP500_ERR_UNSUPPORTED`.
`result` is valid only when the return value is `SP500_OK`.

If the NPU will use the output directly, allocate `dst` with `sp500_buffer_alloc()`.
This method removes a copy between the DSP and NPU.

## `sp500/log_mel.h`

The Log-mel module processes float32 PCM audio.
The output format is the same as the Hugging Face `WhisperFeatureExtractor` format.

The module calculates the output in this sequence:

1. Apply reflect padding to the input.
2. Make frames with a length of `n_fft`.
3. Use `hop_length` for the interval between frames.
4. Apply a periodic Hann window.
5. Calculate the power spectrum.
6. Apply the Slaney mel filter bank.
7. Calculate `log10(max(1e-10, mel))`.
8. Apply normalization if `normalize` is 1.

Normalization limits the values to `max - 8` or greater.
Then, it calculates `(x + 4) / 4`.

The output uses frame-major format.

```text
features[t * n_mels + m]
n_frames = n_samples / hop_length
```

This format is the transpose of the Hugging Face `input_features` format.

| Constant | Value |
|---|---:|
| `SP500_LOG_MEL_MAX_MELS` | 256 |
| `SP500_LOG_MEL_MAX_FFT` | 2048 |

### `sp500_log_mel_params`

| Field | Type | Default | Condition and meaning |
|---|---|---|---|
| `n_samples` | `int32_t` | None | Required. Number of input samples. |
| `sample_rate` | `int32_t` | 16000 | Sample rate in hertz. |
| `n_fft` | `int32_t` | 400 | The range is 16 through 2048. The prime factors can only be 2, 3, and 5. |
| `hop_length` | `int32_t` | 160 | The range is 1 through 65535. |
| `n_mels` | `int32_t` | 80 | The range is 1 through 256. |
| `min_frequency` | `float` | 0 | Minimum frequency of the mel filter bank in hertz. |
| `max_frequency` | `float` | 8000 | Maximum frequency of the mel filter bank in hertz. |
| `normalize` | `uint8_t` | 1 | A value of 1 applies Whisper normalization. |
| `reserved0[3]` | `uint8_t` | 0 | Do not change the value. |
| `mel_filters` | `const float*` | `NULL` | User mel filter bank. The format is `[n_fft/2 + 1][n_mels]` row-major. |
| `mel_filters_count` | `uint32_t` | 0 | Number of elements in `mel_filters`. |
| `reserved[5]` | `uint32_t` | 0 | Do not change the value. |

If `mel_filters` is `NULL`, the module makes a Slaney filter bank.
The module reads `mel_filters` only while the function runs.
If `mel_filters` is `NULL`, `mel_filters_count` must be 0.

If you specify `mel_filters`, use this number of elements:

```text
sp500_log_mel_num_freq(n_fft) * n_mels
```

```c
void sp500_log_mel_params_init(sp500_log_mel_params* params);
```

Use this function to initialize the parameters to their default values.

```c
int32_t sp500_log_mel_num_frames(int32_t n_samples, int32_t hop_length);
```

This function returns `n_samples / hop_length`.
It returns 0 if an argument is not positive.

```c
int32_t sp500_log_mel_num_freq(int32_t n_fft);
```

This function returns `n_fft / 2 + 1`.
It returns 0 if `n_fft` is not supported.

### `sp500_log_mel_result`

| Field | Type | Meaning |
|---|---|---|
| `n_frames` | `int32_t` | Number of frames written to the output. |
| `max_log` | `float` | Maximum log10 mel value before normalization. |
| `host_seconds` | `double` | Execution time. The DSP function records round-trip time. The CPU function records CPU time. |
| `reserved[4]` | `uint32_t` | Not used. |

### `sp500_log_mel()`

```c
sp500_status sp500_log_mel(const sp500_buffer* audio,
                           const sp500_log_mel_params* params,
                           sp500_buffer* features,
                           sp500_log_mel_result* result);
```

| Argument | Condition and meaning |
|---|---|
| `audio` | Float32 input samples. The size must be at least `n_samples * 4` bytes. |
| `params` | Log-mel parameters. |
| `features` | Output buffer. The size must be at least `n_frames * n_mels * 4` bytes. |
| `result` | Receives the number of frames and the maximum value. You can use `NULL`. |

This function can return these status codes:

- `SP500_OK`
- `SP500_ERR_UNSUPPORTED`
- `SP500_ERR_INVALID_ARG`
- `SP500_ERR_BUFFER_TOO_SMALL`
- `SP500_ERR_NO_MEMORY`
- `SP500_ERR_NOT_READY`
- `SP500_ERR_TIMEOUT`
- `SP500_ERR_DSP`

The function returns `SP500_ERR_UNSUPPORTED` in these conditions:

- `n_fft` cannot be transformed.
- All weights in the mel filter bank are 0.
- The spacing in the mel filter bank is too small.

If `n_samples` is less than `hop_length` after parameter validation, `n_frames` is 0.
For a successful call, `n_samples` must be greater than `n_fft / 2`.
The DSP connection must also be ready.

### `sp500_log_mel_cpu()`

```c
sp500_status sp500_log_mel_cpu(const float* audio,
                               const sp500_log_mel_params* params,
                               float* features,
                               int32_t features_count,
                               sp500_log_mel_result* result);
```

This function calculates log-mel values on the host CPU.
The calculation is the same as `sp500_log_mel()`.
This function does not require `sp500_dsp_open()`.

`features_count` is the number of float elements in the `features` array.

This function can return these status codes:

- `SP500_OK`
- `SP500_ERR_INVALID_ARG`
- `SP500_ERR_UNSUPPORTED`
- `SP500_ERR_BUFFER_TOO_SMALL`
- `SP500_ERR_NO_MEMORY`

The DSP function and CPU function use the same parameter validation rules.

## `sp500/yolo.h`

The YOLO module processes output tensors from YOLO heads.
It decodes box coordinates.
It applies a confidence filter.
It removes duplicate detections.

Box coordinates are pixel coordinates in the network input image.

### Head Families

| Constant | Value | Box format | Duplicate removal | Supported tasks | Model families |
|---|---:|---|---|---|---|
| `SP500_YOLO_HEAD_ANCHORLESS` | 0 | DFL 4×16 logits | Per-class NMS | detect, segment, pose | YOLOv3u, YOLOv5u, YOLOv8, YOLOv9, GELAN, YOLO11, YOLO12 |
| `SP500_YOLO_HEAD_ANCHOR` | 1 | Anchor offset and objectness | Per-class NMS | detect, segment | YOLOv3, YOLOv5, YOLOv7 |
| `SP500_YOLO_HEAD_NMSFREE` | 2 | DFL 4×16 logits | Dual top-k | detect | YOLOv10 |
| `SP500_YOLO_HEAD_DFLFREE` | 3 | Four distance values | Dual top-k | detect, segment, pose | YOLO26 |

YOLOv3u and YOLOv5u use anchor-free DFL heads.
Use `SP500_YOLO_HEAD_ANCHORLESS` for these model families.

### Recommended Model Names

Use these primary names with `sp500_yolo_head_for_model()`:

| Model family | Primary name | Head family |
|---|---|---|
| YOLOv3 | `yolov3` | `SP500_YOLO_HEAD_ANCHOR` |
| YOLOv3u | `yolov3u` | `SP500_YOLO_HEAD_ANCHORLESS` |
| YOLOv5 | `yolov5` | `SP500_YOLO_HEAD_ANCHOR` |
| YOLOv5u | `yolov5u` | `SP500_YOLO_HEAD_ANCHORLESS` |
| YOLOv7 | `yolov7` | `SP500_YOLO_HEAD_ANCHOR` |
| YOLOv8 | `yolov8` | `SP500_YOLO_HEAD_ANCHORLESS` |
| YOLOv9 | `yolov9` | `SP500_YOLO_HEAD_ANCHORLESS` |
| GELAN | `gelan` | `SP500_YOLO_HEAD_ANCHORLESS` |
| YOLOv10 | `yolov10` | `SP500_YOLO_HEAD_NMSFREE` |
| YOLO11 | `yolo11` | `SP500_YOLO_HEAD_ANCHORLESS` |
| YOLO12 | `yolo12` | `SP500_YOLO_HEAD_ANCHORLESS` |
| YOLO26 | `yolo26` | `SP500_YOLO_HEAD_DFLFREE` |

This example identifies the head family of YOLOv8:

```c
int32_t head;
sp500_status status = sp500_yolo_head_for_model("yolov8", &head);
/* status == SP500_OK */
/* head == SP500_YOLO_HEAD_ANCHORLESS */
```

You can add a size suffix and a task suffix to the model name.

```c
int32_t head;
sp500_status status = sp500_yolo_head_for_model("yolo11s-seg", &head);
/* status == SP500_OK */
/* head == SP500_YOLO_HEAD_ANCHORLESS */
```

This function identifies the head family from the model name.
It does not examine the model file.
It does not examine the output tensors.
The output tensor format must match the selected head family and task.

The function uses these rules for name identification:

- It treats uppercase and lowercase letters as the same.
- It removes hyphens (`-`).
- It removes underscores (`_`).
- It removes periods (`.`).
- It removes spaces.
- It permits a size suffix.
- It permits a task suffix.

These names are valid examples:

```text
yolov8n
YOLOv8-seg
yolo11s
yolo26s-pose
```

For YOLOv3 and YOLOv5, `u` changes the head family.
The model uses the anchor-free family if `u` is directly after the generation number.
It also uses the anchor-free family if the last character of the normalized name is `u`.

These names use the anchor family:

```text
yolov5n
yolov5n6
```

These names use the anchor-free family:

```text
yolov5nu
yolov5n6u
yolov5u-seg
```

### Tasks

| Constant | Value | Output record |
|---|---:|---|
| `SP500_YOLO_TASK_DETECT` | 0 | `sp500_yolo_detection` |
| `SP500_YOLO_TASK_SEGMENT` | 1 | `sp500_yolo_segmentation` |
| `SP500_YOLO_TASK_POSE` | 2 | `sp500_yolo_pose` |

### Constants

| Constant | Value | Meaning |
|---|---:|---|
| `SP500_YOLO_LEVELS` | 3 | Number of feature levels. The strides are 8, 16, and 32. |
| `SP500_YOLO_POSE_KEYPOINTS` | 17 | Number of keypoints in COCO order. |
| `SP500_YOLO_DETECT_RECORD_FLOATS` | 6 | Number of float values in a detect record. |
| `SP500_YOLO_SEGMENT_RECORD_FLOATS` | 11 | Number of float values in a segment record. |
| `SP500_YOLO_POSE_RECORD_FLOATS` | 57 | Number of float values in a pose record. |
| `SP500_YOLO_MASK_NONE` | `0xFFFFFFFF` | Indicates that there is no mask. |
| `SP500_YOLO_MAX_CANDIDATES` | 1024 | Maximum number of candidates that can pass the confidence filter. |

If the number of candidates is more than `SP500_YOLO_MAX_CANDIDATES`, the module removes the excess candidates.
This operation occurs before duplicate removal.

### Output Records

#### `sp500_yolo_detection`

This structure has six float values.

| Field | Meaning |
|---|---|
| `x1`, `y1`, `x2`, `y2` | Box coordinates in pixels of the network input image. |
| `score` | Confidence score. The range is 0 through 1. |
| `class_id` | Class index stored as a float value. |

#### `sp500_yolo_segmentation`

This structure has 11 float values.

| Field | Meaning |
|---|---|
| `det` | A `sp500_yolo_detection` value. |
| `bx1`, `by1`, `bx2`, `by2` | Mask box. Coordinates align to 4-pixel units and do not exceed the input range. |
| `mask_offset` | Byte offset in `output.mask`. The uint32 bit pattern is stored in a float slot. |

The mask size is `(bx2 - bx1) * (by2 - by1)` bytes.
Each mask value is 0 or 1.
If `mask_offset` is `SP500_YOLO_MASK_NONE`, there is no mask.

```c
uint32_t sp500_yolo_mask_offset(const sp500_yolo_segmentation* record);
```

This function returns the mask offset in the record.
It returns `SP500_YOLO_MASK_NONE` if there is no mask.
It also returns `SP500_YOLO_MASK_NONE` if `record` is `NULL`.

#### `sp500_yolo_keypoint`

This structure has `x`, `y`, and `score` fields.
`x` and `y` are pixel coordinates in the network input image.
The range of `score` is 0 through 1.

#### `sp500_yolo_pose`

This structure has 57 float values.
It has `det` and `keypoints[17]`.

### `sp500_yolo_params`

| Field | Type | Default | Condition and meaning |
|---|---|---|---|
| `img_w`, `img_h` | `int32_t` | 640 | Network input size. The range is 1 through 65535. |
| `num_classes` | `int32_t` | 80 | Number of classes. The range is 1 through 65535. Use 1 for a pose head. |
| `max_detections` | `int32_t` | 100 | Maximum number of records to return. The range is 1 through 65535. |
| `conf_threshold` | `float` | 0.25 | Removes confidence scores less than this value. |
| `iou_threshold` | `float` | 0.7 | NMS threshold. NMSFREE and DFLFREE families do not use this value. |
| `anchors_per_level` | `int32_t` | 3 | Used only for ANCHOR decoding. The range is 1 through 3. |
| `anchors` | `const float*` | `NULL` | Used only for ANCHOR decoding. Specifies the `(w, h)` values of anchors. |
| `anchors_count` | `uint32_t` | 0 | Number of float elements in the `anchors` array. |
| `reserved0` | `uint32_t` | 0 | Do not change the value. |
| `proto` | `sp500_buffer` | Not used | Required for the SEGMENT task. The format is `[proto_h][proto_w][mask_coeffs]` float. |
| `mask_coeffs` | `int32_t` | 0 | Required for the SEGMENT task. Number of mask coefficients for one detection. |
| `proto_h`, `proto_w` | `int32_t` | 0 | Required for the SEGMENT task. Prototype dimensions. |
| `mask_threshold` | `float` | 0 | Used for the SEGMENT task. The mask value is 1 if the logit is more than this value. |
| `reserved[6]` | `uint32_t` | 0 | Do not change the value. |

Specify `anchors` in stride order 8, 16, and 32.
Each level requires `anchors_per_level` pairs of `(w, h)`.
Thus, the array requires `anchors_per_level * 6` float values.
The function reads `anchors` only while it runs.

If `anchors` is `NULL`, the module uses the default COCO YOLOv5 anchors.
This operation requires `anchors_per_level == 3`.

The SEGMENT task requires all five of these fields:

- `proto`
- `mask_coeffs`
- `proto_h`
- `proto_w`
- `mask_threshold`

```c
void sp500_yolo_params_init(sp500_yolo_params* params);
```

Use this function to initialize the parameters to their default values.

### `sp500_yolo_raw`

This structure specifies input for the ANCHORLESS, NMSFREE, and DFLFREE families.
Array indexes 0, 1, and 2 identify strides 8, 16, and 32.
Each plane uses HWC float format.

| Field | Condition and meaning |
|---|---|
| `box[3]` | Required. Use 4×16 DFL logits for ANCHORLESS and NMSFREE. Use four distance values for DFLFREE. |
| `cls[3]` | Required. Uses `num_classes` logits. |
| `extra[3]` | Uses `mask_coeffs` values for SEGMENT and 51 values for POSE. Not used for DETECT. |
| `reserved[4]` | Keep at 0. |

```c
void sp500_yolo_raw_init(sp500_yolo_raw* raw);
```

This function initializes all planes to the unused state.

### `sp500_yolo_raw_anchor`

This structure specifies input for the ANCHOR family.
Each plane uses this format:

```text
[ny][nx][anchors_per_level * no]
no = 5 + num_classes + mask_coeffs
```

Each item has this sequence:

1. `xy`
2. `wh`
3. `objectness`
4. Class scores
5. Mask coefficients

| Field | Condition and meaning |
|---|---|
| `plane[3]` | Required. Indexes 0, 1, and 2 identify strides 8, 16, and 32. |
| `reserved[4]` | Keep at 0. |

```c
void sp500_yolo_raw_anchor_init(sp500_yolo_raw_anchor* raw);
```

This function initializes all planes to the unused state.

### `sp500_yolo_output`

| Field | Condition and meaning |
|---|---|
| `records` | Required output buffer. Use `sp500_yolo_records_bytes()` to calculate the necessary size. |
| `mask` | Output buffer required for the SEGMENT task. The `mask_offset` of each record refers to this buffer. |
| `reserved[4]` | Keep at 0. |

`records` must have space for `params.max_detections` records.

```c
void sp500_yolo_output_init(sp500_yolo_output* output);
```

This function initializes all output buffers to the unused state.

### `sp500_yolo_result`

| Field | Type | Meaning |
|---|---|---|
| `count` | `uint32_t` | Number of records written to `output.records`. |
| `record_floats` | `int32_t` | Number of float values in one record. |
| `host_seconds` | `double` | Round-trip time of the DSP call in seconds. |
| `reserved[4]` | `uint32_t` | Not used. |

### Helper Functions

```c
int32_t sp500_yolo_record_floats(int32_t task);
```

This function returns the number of float values required for one task record.
It returns 0 if the task is not recognized.

```c
uint32_t sp500_yolo_records_bytes(int32_t task, int32_t max_detections);
```

This function returns the number of bytes required for `output.records`.
It returns 0 if an argument is invalid.

```c
int32_t sp500_yolo_grid_cells(int32_t img_w, int32_t img_h);
```

This function returns the sum of the grid cells for strides 8, 16, and 32.
For the ANCHOR family, multiply the return value by `anchors_per_level`.
It returns 0 if `img_w` or `img_h` is not positive.

```c
sp500_status sp500_yolo_head_for_model(const char* model_name,
                                        int32_t* head);
```

This function converts a model name to a head family.
It stores the converted value in `head`.

Refer to [Recommended Model Names](#recommended-model-names) for recommended names.

The function returns `SP500_OK` after a successful conversion.
It returns `SP500_ERR_INVALID_ARG` if an argument is `NULL`.
It also returns `SP500_ERR_INVALID_ARG` if it does not recognize the name.

### `sp500_yolo_decode()`

```c
sp500_status sp500_yolo_decode(int32_t head,
                               int32_t task,
                               const sp500_yolo_raw* raw,
                               const sp500_yolo_params* params,
                               sp500_yolo_output* output,
                               sp500_yolo_result* result);
```

| Argument | Condition and meaning |
|---|---|
| `head` | Use `SP500_YOLO_HEAD_ANCHORLESS`, `SP500_YOLO_HEAD_NMSFREE`, or `SP500_YOLO_HEAD_DFLFREE`. |
| `task` | Use a `SP500_YOLO_TASK_*` value. NMSFREE supports only DETECT. |
| `raw` | Input planes for three feature levels. |
| `params` | YOLO parameters. SEGMENT requires `proto`, `mask_coeffs`, `proto_h`, `proto_w`, and `mask_threshold`. |
| `output` | Requires `records`. SEGMENT also requires `mask`. |
| `result` | Receives the record count and execution time. You can use `NULL`. |

This function can return these status codes:

- `SP500_OK`
- `SP500_ERR_UNSUPPORTED`
- `SP500_ERR_INVALID_ARG`
- `SP500_ERR_BUFFER_TOO_SMALL`
- `SP500_ERR_NO_MEMORY`
- `SP500_ERR_NOT_READY`
- `SP500_ERR_TIMEOUT`
- `SP500_ERR_DSP`

The function returns `SP500_ERR_UNSUPPORTED` if the head family does not support the task.
It returns `SP500_ERR_BUFFER_TOO_SMALL` if `records` cannot store `max_detections` records.

The function returns `SP500_OK` if there are no detections.
In this condition, `result.count` is 0.

### `sp500_yolo_decode_anchor()`

```c
sp500_status sp500_yolo_decode_anchor(int32_t task,
                                      const sp500_yolo_raw_anchor* raw,
                                      const sp500_yolo_params* params,
                                      sp500_yolo_output* output,
                                      sp500_yolo_result* result);
```

This function is only for `SP500_YOLO_HEAD_ANCHOR`.
It supports the DETECT and SEGMENT tasks.
It uses `params.anchors_per_level` and `params.anchors`.

The return status codes are the same as for `sp500_yolo_decode()`.

### `sp500_yolo_postprocess()`

```c
sp500_status sp500_yolo_postprocess(int32_t head,
                                    int32_t task,
                                    const sp500_buffer* decoded,
                                    int32_t num_rows,
                                    const sp500_yolo_params* params,
                                    sp500_yolo_output* output,
                                    sp500_yolo_result* result);
```

This function selects results from a decoded candidate table.
It supports all head families.
The head family determines the duplicate removal method.
The head family also determines if an objectness column is used.

| Argument | Condition and meaning |
|---|---|
| `head` | YOLO head family. |
| `task` | YOLO task. |
| `decoded` | Decoded candidate table. |
| `num_rows` | Number of candidate rows. The value must be 1 or more. |
| `params` | YOLO parameters. |
| `output` | Output buffers. |
| `result` | Receives the record count and execution time. You can use `NULL`. |

One row of `decoded` contains this number of float values:

```text
4 + objectness + num_classes + extra
```

For the ANCHOR family, `objectness` is 1.
For the other families, `objectness` is 0.
For the SEGMENT task, `extra` is `mask_coeffs`.
For the POSE task, `extra` is 51.

For standard heads, use the return value of `sp500_yolo_grid_cells()` for `num_rows`.
For an ANCHOR head, multiply this value by `anchors_per_level`.

The return status codes are the same as for `sp500_yolo_decode()`.
