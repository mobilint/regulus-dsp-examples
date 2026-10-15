# SP500 API 참조

[English](sp500-api-reference.md)

이 문서는 `libsp500.so` 버전 1.0.0을 설명합니다.

다음 공개 헤더를 사용하십시오.

```text
sp500/dsp.h
sp500/letterbox.h
sp500/log_mel.h
sp500/yolo.h
```

각 모듈 헤더는 `sp500/dsp.h`를 포함합니다.
모든 공개 선언은 `extern "C"` 연결 규약을 사용합니다.
공개 헤더는 `<stdint.h>`와 `<stddef.h>`를 사용합니다.

## 공통 규칙

- 상태를 반환하는 함수는 `sp500_status`를 반환합니다.
- 파라미터 구조체는 `*_params_init()`으로 먼저 초기화하십시오.
- 초기화한 뒤에 필요한 필드를 설정하십시오.
- 모든 `reserved` 필드는 0으로 유지하십시오.
- API는 여러 스레드와 여러 프로세스에서 호출할 수 있습니다.
- DSP 호출은 시스템 전체에서 한 번에 하나씩 실행됩니다.

## `sp500/dsp.h`

### 상태 코드

`sp500_status`의 형식은 `int32_t`입니다.

| 코드 | 값 | 의미 |
|---|---:|---|
| `SP500_OK` | 0 | 호출이 성공했습니다. 출력 버퍼가 유효합니다 |
| `SP500_ERR_INVALID_ARG` | -1 | 인자가 없거나 범위를 벗어났습니다. 인자 조합이 잘못된 경우도 포함합니다 |
| `SP500_ERR_UNSUPPORTED` | -2 | 모듈이 요청한 조합을 지원하지 않습니다 |
| `SP500_ERR_BUFFER_TOO_SMALL` | -3 | 버퍼 크기가 부족합니다 |
| `SP500_ERR_NOT_READY` | -4 | `sp500_dsp_open()`이 완료되지 않았습니다 |
| `SP500_ERR_NO_MEMORY` | -5 | 호스트 메모리 또는 공유 메모리를 할당할 수 없습니다 |
| `SP500_ERR_TIMEOUT` | -6 | 제한 시간 안에 DSP가 응답하지 않았습니다 |
| `SP500_ERR_DSP` | -7 | DSP가 오류를 반환했습니다 |
| `SP500_ERR_INTERNAL` | -8 | 내부 오류가 발생했습니다 |

```c
const char* sp500_status_string(sp500_status status);
```

이 함수는 상태 코드의 이름을 반환합니다.
이 함수는 `NULL`을 반환하지 않습니다.

### 버전

| 매크로 | 값 |
|---|---:|
| `SP500_VERSION_MAJOR` | 1 |
| `SP500_VERSION_MINOR` | 0 |
| `SP500_VERSION_PATCH` | 0 |

`SP500_VERSION_INT`는 다음 식을 사용합니다.

```text
major * 10000 + minor * 100 + patch
```

```c
uint32_t sp500_version(void);
```

이 함수는 로드된 라이브러리의 버전을 반환합니다.
반환값은 `SP500_VERSION_INT` 형식입니다.

```c
const char* sp500_version_string(void);
```

이 함수는 로드된 라이브러리의 버전을 문자열로 반환합니다.
문자열 형식은 `major.minor.patch`입니다.

### DSP 연결

| 상수 | 값 |
|---|---:|
| `SP500_DEFAULT_OPEN_TIMEOUT_MS` | 10000 |
| `SP500_DEFAULT_DSP_TIMEOUT_MS` | 3000 |

```c
sp500_status sp500_dsp_open(uint32_t timeout_ms);
```

이 함수는 DSP에 연결합니다.
이 함수는 공유 메모리 창을 매핑합니다.
DSP 펌웨어가 준비되지 않았으면 함수가 대기합니다.

`timeout_ms`는 연결 제한 시간입니다.
단위는 ms입니다.
값이 0이면 `SP500_DEFAULT_OPEN_TIMEOUT_MS`를 사용합니다.

첫 번째 호출이 성공한 뒤에는 연결을 다시 만들지 않습니다.

이 함수는 다음 상태를 반환할 수 있습니다.

- `SP500_OK`
- `SP500_ERR_TIMEOUT`
- `SP500_ERR_NOT_READY`

```c
int sp500_dsp_is_open(void);
```

DSP 연결이 준비되었으면 1을 반환합니다.
그렇지 않으면 0을 반환합니다.
이 함수는 대기하지 않습니다.

```c
void sp500_set_dsp_timeout_ms(uint32_t timeout_ms);
```

이 함수는 이후 DSP 호출의 제한 시간을 설정합니다.
단위는 ms입니다.

값이 0이면 `SP500_DEFAULT_DSP_TIMEOUT_MS`를 사용합니다.
기본값보다 작은 값도 기본값으로 변경됩니다.

이 제한 시간은 DSP 작업을 취소하지 않습니다.
제한 시간이 지나도 DSP는 기존 작업을 계속할 수 있습니다.
이 상태에서 다음 호출이 이전 호출의 결과를 받을 수 있습니다.
제한 시간 오류가 발생하면 DSP 펌웨어를 다시 로드하십시오.
반복 재시도를 하지 마십시오.

```c
uint32_t sp500_get_dsp_timeout_ms(void);
```

이 함수는 현재 DSP 호출 제한 시간을 반환합니다.
단위는 ms입니다.

### DSP 잠금

```c
void sp500_dsp_lock(void);
```

이 함수는 재진입 가능한 DSP 잠금을 획득합니다.
잠금은 스레드와 프로세스 사이에서 유효합니다.
각 DSP 호출도 이 잠금을 내부에서 사용합니다.

```c
void sp500_dsp_unlock(void);
```

이 함수는 DSP 잠금을 한 단계 해제합니다.
각 `sp500_dsp_lock()` 호출에는 한 번의 `sp500_dsp_unlock()` 호출이 필요합니다.
잠금 호출의 수가 맞지 않으면 모든 프로세스의 DSP 호출과 공유 메모리 할당이 멈출 수 있습니다.

### 공유 메모리

| 상수 | 값 | 의미 |
|---|---:|---|
| `SP500_ALIGN_DSP` | 32 | DSP 버퍼의 최소 정렬 값입니다 |
| `SP500_ALIGN_NPU` | 4096 | NPU가 접근하는 버퍼의 정렬 값입니다 |

```c
void* sp500_shmem_alloc(uint32_t size, uint32_t alignment);
```

이 함수는 공유 메모리 창에서 메모리를 할당합니다.
`alignment`는 `SP500_ALIGN_DSP` 이상이어야 합니다.
할당에 실패하면 `NULL`을 반환합니다.
DSP 연결이 없어도 `NULL`을 반환합니다.

할당기는 첫 번째로 맞는 빈 공간을 사용합니다.
할당기는 빈 공간을 압축하지 않습니다.
크기와 사용 기간이 다른 버퍼를 반복해서 할당하면 메모리가 단편화될 수 있습니다.
작업에 필요한 버퍼를 한 번 할당하고 반복해서 사용하십시오.

공유 메모리 버퍼를 전역 객체 또는 정적 객체에 저장하지 마십시오.
링크 종료 후에 객체의 소멸자가 버퍼를 해제할 수 있습니다.

```c
void sp500_shmem_free(void* ptr);
```

이 함수는 공유 메모리를 해제합니다.
`ptr`이 `NULL`이면 작업을 하지 않습니다.

```c
int sp500_shmem_contains(const void* ptr);
```

`ptr`이 공유 메모리 창 안에 있으면 1을 반환합니다.
할당 영역 안의 내부 포인터도 유효합니다.
`sp500_dsp_open()`을 호출하기 전에는 0을 반환합니다.

```c
uint64_t sp500_shmem_phys_addr(const void* ptr);
```

이 함수는 공유 메모리 포인터의 물리 주소를 반환합니다.
포인터가 공유 메모리 창 밖에 있으면 0을 반환합니다.

### 버퍼

```c
typedef struct sp500_buffer {
    void*    data;
    uint32_t size;
    uint32_t flags;
} sp500_buffer;
```

| 필드 | 의미 |
|---|---|
| `data` | 호스트 가상 주소입니다. `NULL`이면 버퍼를 사용하지 않습니다 |
| `size` | 버퍼의 바이트 수입니다 |
| `flags` | 버퍼 속성입니다 |

| 플래그 | 값 | 의미 |
|---|---:|---|
| `SP500_BUFFER_SHARED` | `1 << 0` | 버퍼가 공유 메모리 창에 있습니다. DSP는 복사 없이 버퍼에 접근합니다 |

```c
sp500_buffer sp500_buffer_none(void);
```

이 함수는 사용하지 않는 버퍼 값을 반환합니다.

```c
sp500_buffer sp500_buffer_alloc(uint32_t size, uint32_t alignment);
```

이 함수는 공유 메모리에 버퍼를 할당합니다.
반환된 버퍼에는 `SP500_BUFFER_SHARED` 플래그가 있습니다.
할당에 실패하면 `data`가 `NULL`입니다.

```c
sp500_buffer sp500_buffer_wrap(void* data, uint32_t size);
```

이 함수는 응용 프로그램의 메모리를 SP500 버퍼로 지정합니다.
입력 버퍼로 사용하면 DSP 호출이 데이터를 공유 메모리로 한 번 복사합니다.
출력 버퍼로 사용하면 DSP 호출이 데이터를 응용 프로그램 메모리로 한 번 복사합니다.

```c
void sp500_buffer_release(sp500_buffer* buffer);
```

버퍼에 `SP500_BUFFER_SHARED` 플래그가 있으면 공유 메모리를 해제합니다.
그 뒤에 버퍼를 사용하지 않는 상태로 초기화합니다.

## `sp500/letterbox.h`

Letterbox 모듈은 8-bit interleaved HWC 이미지를 처리합니다.
모듈은 원본의 가로세로 비율을 유지합니다.
모듈은 남은 영역을 `pad_value`로 채웁니다.
행 끝에는 추가 패딩이 없어야 합니다.

모듈은 다음 배율을 사용합니다.

```text
scale = min(dst_w / src_w, dst_h / src_h)
```

다음 식으로 출력 좌표를 원본 좌표로 변환하십시오.

```text
src_x = (dst_x - pad_x) / scale
src_y = (dst_y - pad_y) / scale
```

### 보간 방식

| 상수 | 값 | 의미 |
|---|---:|---|
| `SP500_RESIZE_NEAREST` | 0 | nearest-neighbor 보간입니다 |
| `SP500_RESIZE_BILINEAR` | 1 | bilinear 보간입니다 |
| `SP500_RESIZE_BICUBIC` | 2 | bicubic 보간입니다. `a`는 -0.75입니다 |
| `SP500_RESIZE_LANCZOS` | 3 | Lanczos4 보간입니다. 8 tap을 사용합니다 |

### `sp500_letterbox_params`

| 필드 | 형식 | 기본값 | 조건 및 의미 |
|---|---|---|---|
| `src_w`, `src_h` | `int32_t` | 없음 | 필수 값입니다. 범위는 1부터 65535까지입니다 |
| `dst_w`, `dst_h` | `int32_t` | 640 | 출력 크기입니다. 범위는 1부터 65535까지입니다 |
| `channels` | `int32_t` | 3 | interleaved 채널 수입니다 |
| `interp` | `int32_t` | `SP500_RESIZE_BILINEAR` | `SP500_RESIZE_*` 값 중 하나를 사용하십시오 |
| `pad_value` | `uint8_t` | 114 | 모든 채널에 사용하는 테두리 값입니다 |
| `center` | `uint8_t` | 1 | 1이면 테두리를 양쪽에 분배합니다. 0이면 오른쪽과 아래쪽에만 추가합니다 |
| `scale_up` | `uint8_t` | 1 | 1이면 확대할 수 있습니다. 0이면 배율을 1.0 이하로 제한합니다 |
| `swap_rb` | `uint8_t` | 0 | 1이면 채널 0과 채널 2를 교환합니다. 이 값에는 채널 3개가 필요합니다 |
| `reserved[4]` | `uint32_t` | 0 | 값을 변경하지 마십시오 |

```c
void sp500_letterbox_params_init(sp500_letterbox_params* params);
```

이 함수를 사용하여 파라미터를 기본값으로 초기화하십시오.

### `sp500_letterbox_result`

| 필드 | 형식 | 의미 |
|---|---|---|
| `scale` | `float` | 적용한 배율입니다 |
| `pad_x`, `pad_y` | `int32_t` | 왼쪽과 위쪽의 테두리 크기입니다. 단위는 픽셀입니다 |
| `host_seconds` | `double` | DSP 호출의 왕복 시간입니다. 단위는 초입니다 |
| `reserved[4]` | `uint32_t` | 사용하지 않습니다 |

### `sp500_letterbox()`

```c
sp500_status sp500_letterbox(const sp500_buffer* src,
                             const sp500_letterbox_params* params,
                             sp500_buffer* dst,
                             sp500_letterbox_result* result);
```

| 인자 | 조건 및 의미 |
|---|---|
| `src` | 입력 버퍼입니다. 크기는 `src_w * src_h * channels` 바이트 이상이어야 합니다 |
| `params` | Letterbox 파라미터입니다 |
| `dst` | 출력 버퍼입니다. 크기는 `dst_w * dst_h * channels` 바이트 이상이어야 합니다 |
| `result` | 배율과 테두리 정보를 받습니다. `NULL`을 사용할 수 있습니다 |

이 함수는 다음 상태를 반환할 수 있습니다.

- `SP500_OK`
- `SP500_ERR_INVALID_ARG`
- `SP500_ERR_BUFFER_TOO_SMALL`
- `SP500_ERR_UNSUPPORTED`
- `SP500_ERR_NO_MEMORY`
- `SP500_ERR_NOT_READY`
- `SP500_ERR_TIMEOUT`
- `SP500_ERR_DSP`

`interp` 값이 지원 범위 밖이면 `SP500_ERR_UNSUPPORTED`를 반환합니다.
`result`는 반환값이 `SP500_OK`일 때만 유효합니다.

NPU가 출력을 바로 사용하려면 `dst`를 `sp500_buffer_alloc()`로 할당하십시오.
이 방법은 DSP와 NPU 사이의 복사를 없앩니다.

## `sp500/log_mel.h`

Log-mel 모듈은 float32 PCM 오디오를 처리합니다.
출력 형식은 Hugging Face `WhisperFeatureExtractor`와 같습니다.

모듈은 다음 순서로 계산합니다.

1. 입력에 reflect 패딩을 적용합니다.
2. `n_fft` 길이의 프레임을 만듭니다.
3. 프레임 간격에 `hop_length`를 사용합니다.
4. periodic Hann window를 적용합니다.
5. power spectrum을 계산합니다.
6. Slaney mel filter bank를 적용합니다.
7. `log10(max(1e-10, mel))`을 계산합니다.
8. `normalize`가 1이면 정규화를 적용합니다.

정규화는 값을 `max - 8` 이상으로 제한합니다.
그 뒤에 `(x + 4) / 4`를 계산합니다.

출력은 frame-major 형식입니다.

```text
features[t * n_mels + m]
n_frames = n_samples / hop_length
```

이 형식은 Hugging Face `input_features`의 전치 형식입니다.

| 상수 | 값 |
|---|---:|
| `SP500_LOG_MEL_MAX_MELS` | 256 |
| `SP500_LOG_MEL_MAX_FFT` | 2048 |

### `sp500_log_mel_params`

| 필드 | 형식 | 기본값 | 조건 및 의미 |
|---|---|---|---|
| `n_samples` | `int32_t` | 없음 | 필수 값입니다. 입력 샘플 수입니다 |
| `sample_rate` | `int32_t` | 16000 | 샘플 속도입니다. 단위는 Hz입니다 |
| `n_fft` | `int32_t` | 400 | 범위는 16부터 2048까지입니다. 소인수는 2, 3, 5만 사용할 수 있습니다 |
| `hop_length` | `int32_t` | 160 | 범위는 1부터 65535까지입니다 |
| `n_mels` | `int32_t` | 80 | 범위는 1부터 256까지입니다 |
| `min_frequency` | `float` | 0 | mel filter bank의 최소 주파수입니다. 단위는 Hz입니다 |
| `max_frequency` | `float` | 8000 | mel filter bank의 최대 주파수입니다. 단위는 Hz입니다 |
| `normalize` | `uint8_t` | 1 | 1이면 Whisper 정규화를 적용합니다 |
| `reserved0[3]` | `uint8_t` | 0 | 값을 변경하지 마십시오 |
| `mel_filters` | `const float*` | `NULL` | 사용자 mel filter bank입니다. 형식은 `[n_fft/2 + 1][n_mels]` row-major입니다 |
| `mel_filters_count` | `uint32_t` | 0 | `mel_filters`의 원소 수입니다 |
| `reserved[5]` | `uint32_t` | 0 | 값을 변경하지 마십시오 |

`mel_filters`가 `NULL`이면 모듈이 Slaney filter bank를 만듭니다.
모듈은 함수가 실행되는 동안에만 `mel_filters`를 읽습니다.
`mel_filters`가 `NULL`이면 `mel_filters_count`는 0이어야 합니다.

`mel_filters`를 지정하면 다음 원소 수를 사용하십시오.

```text
sp500_log_mel_num_freq(n_fft) * n_mels
```

```c
void sp500_log_mel_params_init(sp500_log_mel_params* params);
```

이 함수를 사용하여 파라미터를 기본값으로 초기화하십시오.

```c
int32_t sp500_log_mel_num_frames(int32_t n_samples, int32_t hop_length);
```

이 함수는 `n_samples / hop_length`를 반환합니다.
인자가 양수가 아니면 0을 반환합니다.

```c
int32_t sp500_log_mel_num_freq(int32_t n_fft);
```

이 함수는 `n_fft / 2 + 1`을 반환합니다.
`n_fft`가 지원되지 않으면 0을 반환합니다.

### `sp500_log_mel_result`

| 필드 | 형식 | 의미 |
|---|---|---|
| `n_frames` | `int32_t` | 출력에 기록한 프레임 수입니다 |
| `max_log` | `float` | 정규화 전의 최대 log10 mel 값입니다 |
| `host_seconds` | `double` | 실행 시간입니다. DSP 함수에는 왕복 시간이 들어갑니다. CPU 함수에는 CPU 시간이 들어갑니다 |
| `reserved[4]` | `uint32_t` | 사용하지 않습니다 |

### `sp500_log_mel()`

```c
sp500_status sp500_log_mel(const sp500_buffer* audio,
                           const sp500_log_mel_params* params,
                           sp500_buffer* features,
                           sp500_log_mel_result* result);
```

| 인자 | 조건 및 의미 |
|---|---|
| `audio` | float32 입력 샘플입니다. 크기는 `n_samples * 4` 바이트 이상이어야 합니다 |
| `params` | Log-mel 파라미터입니다 |
| `features` | 출력 버퍼입니다. 크기는 `n_frames * n_mels * 4` 바이트 이상이어야 합니다 |
| `result` | 프레임 수와 최대값을 받습니다. `NULL`을 사용할 수 있습니다 |

이 함수는 다음 상태를 반환할 수 있습니다.

- `SP500_OK`
- `SP500_ERR_UNSUPPORTED`
- `SP500_ERR_INVALID_ARG`
- `SP500_ERR_BUFFER_TOO_SMALL`
- `SP500_ERR_NO_MEMORY`
- `SP500_ERR_NOT_READY`
- `SP500_ERR_TIMEOUT`
- `SP500_ERR_DSP`

다음 조건에서는 `SP500_ERR_UNSUPPORTED`를 반환합니다.

- `n_fft`를 변환할 수 없습니다.
- mel filter bank의 모든 가중치가 0입니다.
- mel filter bank의 간격이 너무 작습니다.

파라미터 검증을 통과한 뒤 `n_samples`가 `hop_length`보다 작으면 `n_frames`는 0입니다.
이 호출이 성공하려면 `n_samples`가 `n_fft / 2`보다 커야 합니다.
DSP 연결도 준비되어야 합니다.

### `sp500_log_mel_cpu()`

```c
sp500_status sp500_log_mel_cpu(const float* audio,
                               const sp500_log_mel_params* params,
                               float* features,
                               int32_t features_count,
                               sp500_log_mel_result* result);
```

이 함수는 호스트 CPU에서 log-mel 값을 계산합니다.
계산 방법은 `sp500_log_mel()`과 같습니다.
이 함수에는 `sp500_dsp_open()`이 필요하지 않습니다.

`features_count`는 `features` 배열의 float 원소 수입니다.

이 함수는 다음 상태를 반환할 수 있습니다.

- `SP500_OK`
- `SP500_ERR_INVALID_ARG`
- `SP500_ERR_UNSUPPORTED`
- `SP500_ERR_BUFFER_TOO_SMALL`
- `SP500_ERR_NO_MEMORY`

DSP 함수와 CPU 함수는 같은 파라미터 검증 규칙을 사용합니다.

## `sp500/yolo.h`

YOLO 모듈은 YOLO 헤드의 출력 텐서를 처리합니다.
모듈은 박스 좌표를 디코드합니다.
모듈은 신뢰도 필터를 적용합니다.
모듈은 중복 검출을 제거합니다.

박스 좌표는 네트워크 입력 이미지의 픽셀 좌표입니다.

### 헤드 계열

| 상수 | 값 | 박스 형식 | 중복 제거 | 지원 태스크 | 모델 계열 |
|---|---:|---|---|---|---|
| `SP500_YOLO_HEAD_ANCHORLESS` | 0 | DFL 4×16 logits | 클래스별 NMS | detect, segment, pose | YOLOv3u, YOLOv5u, YOLOv8, YOLOv9, GELAN, YOLO11, YOLO12 |
| `SP500_YOLO_HEAD_ANCHOR` | 1 | anchor offset와 objectness | 클래스별 NMS | detect, segment | YOLOv3, YOLOv5, YOLOv7 |
| `SP500_YOLO_HEAD_NMSFREE` | 2 | DFL 4×16 logits | dual top-k | detect | YOLOv10 |
| `SP500_YOLO_HEAD_DFLFREE` | 3 | 거리 값 4개 | dual top-k | detect, segment, pose | YOLO26 |

YOLOv3u와 YOLOv5u는 anchor-free DFL 헤드를 사용합니다.
두 모델 계열에는 `SP500_YOLO_HEAD_ANCHORLESS`를 사용하십시오.

### 권장 모델 이름

`sp500_yolo_head_for_model()`에는 다음 기본 이름을 사용하십시오.

| 모델 계열 | 기본 이름 | 헤드 계열 |
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

다음 예시는 YOLOv8의 헤드 계열을 확인합니다.

```c
int32_t head;
sp500_status status = sp500_yolo_head_for_model("yolov8", &head);
/* status == SP500_OK */
/* head == SP500_YOLO_HEAD_ANCHORLESS */
```

모델 이름에 크기 접미사와 태스크 접미사를 추가할 수도 있습니다.

```c
int32_t head;
sp500_status status = sp500_yolo_head_for_model("yolo11s-seg", &head);
/* status == SP500_OK */
/* head == SP500_YOLO_HEAD_ANCHORLESS */
```

이 함수는 모델 이름에서 헤드 계열을 판별합니다.
이 함수는 모델 파일을 검사하지 않습니다.
이 함수는 출력 텐서를 검사하지 않습니다.
출력 텐서 형식은 선택한 헤드 계열과 태스크에 맞아야 합니다.

이름 판별에는 다음 규칙을 사용합니다.

- 대문자와 소문자를 같은 문자로 처리합니다.
- 연결 문자(`-`)를 제거합니다.
- 밑줄(`_`)을 제거합니다.
- 마침표(`.`)를 제거합니다.
- 공백을 제거합니다.
- 크기 접미사를 사용할 수 있습니다.
- 태스크 접미사를 사용할 수 있습니다.

다음 이름은 유효한 예입니다.

```text
yolov8n
YOLOv8-seg
yolo11s
yolo26s-pose
```

YOLOv3과 YOLOv5에서 `u`는 헤드 계열을 변경합니다.
`u`가 세대 번호 바로 뒤에 있으면 anchor-free 계열입니다.
정규화한 이름의 마지막 문자가 `u`인 경우에도 anchor-free 계열입니다.

다음 이름은 anchor 계열입니다.

```text
yolov5n
yolov5n6
```

다음 이름은 anchor-free 계열입니다.

```text
yolov5nu
yolov5n6u
yolov5u-seg
```

### 태스크

| 상수 | 값 | 출력 레코드 |
|---|---:|---|
| `SP500_YOLO_TASK_DETECT` | 0 | `sp500_yolo_detection` |
| `SP500_YOLO_TASK_SEGMENT` | 1 | `sp500_yolo_segmentation` |
| `SP500_YOLO_TASK_POSE` | 2 | `sp500_yolo_pose` |

### 상수

| 상수 | 값 | 의미 |
|---|---:|---|
| `SP500_YOLO_LEVELS` | 3 | feature level 수입니다. stride는 8, 16, 32입니다 |
| `SP500_YOLO_POSE_KEYPOINTS` | 17 | COCO 순서의 keypoint 수입니다 |
| `SP500_YOLO_DETECT_RECORD_FLOATS` | 6 | detect 레코드의 float 수입니다 |
| `SP500_YOLO_SEGMENT_RECORD_FLOATS` | 11 | segment 레코드의 float 수입니다 |
| `SP500_YOLO_POSE_RECORD_FLOATS` | 57 | pose 레코드의 float 수입니다 |
| `SP500_YOLO_MASK_NONE` | `0xFFFFFFFF` | 마스크가 없음을 나타냅니다 |
| `SP500_YOLO_MAX_CANDIDATES` | 1024 | 신뢰도 필터를 통과할 수 있는 최대 후보 수입니다 |

후보 수가 `SP500_YOLO_MAX_CANDIDATES`보다 크면 초과 후보를 제거합니다.
이 작업은 중복 제거 전에 실행됩니다.

### 출력 레코드

#### `sp500_yolo_detection`

이 구조체는 6개의 float 값으로 구성됩니다.

| 필드 | 의미 |
|---|---|
| `x1`, `y1`, `x2`, `y2` | 박스 좌표입니다. 단위는 네트워크 입력 이미지의 픽셀입니다 |
| `score` | 신뢰도 점수입니다. 범위는 0부터 1까지입니다 |
| `class_id` | 클래스 인덱스입니다. 정수를 float 형식으로 저장합니다 |

#### `sp500_yolo_segmentation`

이 구조체는 11개의 float 값으로 구성됩니다.

| 필드 | 의미 |
|---|---|
| `det` | `sp500_yolo_detection` 값입니다 |
| `bx1`, `by1`, `bx2`, `by2` | 마스크 박스입니다. 좌표는 4픽셀 단위로 맞춥니다. 좌표는 입력 범위를 벗어나지 않습니다 |
| `mask_offset` | `output.mask` 안의 바이트 오프셋입니다. uint32 비트 패턴을 float 슬롯에 저장합니다 |

마스크 크기는 `(bx2 - bx1) * (by2 - by1)` 바이트입니다.
각 마스크 값은 0 또는 1입니다.
`mask_offset`이 `SP500_YOLO_MASK_NONE`이면 마스크가 없습니다.

```c
uint32_t sp500_yolo_mask_offset(const sp500_yolo_segmentation* record);
```

이 함수는 레코드의 마스크 오프셋을 반환합니다.
마스크가 없으면 `SP500_YOLO_MASK_NONE`을 반환합니다.
`record`가 `NULL`이어도 `SP500_YOLO_MASK_NONE`을 반환합니다.

#### `sp500_yolo_keypoint`

이 구조체에는 `x`, `y`, `score` 필드가 있습니다.
`x`와 `y`는 네트워크 입력 이미지의 픽셀 좌표입니다.
`score`의 범위는 0부터 1까지입니다.

#### `sp500_yolo_pose`

이 구조체는 57개의 float 값으로 구성됩니다.
구조체에는 `det`와 `keypoints[17]`이 있습니다.

### `sp500_yolo_params`

| 필드 | 형식 | 기본값 | 조건 및 의미 |
|---|---|---|---|
| `img_w`, `img_h` | `int32_t` | 640 | 네트워크 입력 크기입니다. 범위는 1부터 65535까지입니다 |
| `num_classes` | `int32_t` | 80 | 클래스 수입니다. 범위는 1부터 65535까지입니다. pose 헤드에는 1을 사용합니다 |
| `max_detections` | `int32_t` | 100 | 반환할 레코드의 최대 수입니다. 범위는 1부터 65535까지입니다 |
| `conf_threshold` | `float` | 0.25 | 이 값보다 작은 신뢰도 점수를 제거합니다 |
| `iou_threshold` | `float` | 0.7 | NMS 임계값입니다. NMSFREE와 DFLFREE 계열은 이 값을 사용하지 않습니다 |
| `anchors_per_level` | `int32_t` | 3 | ANCHOR 디코드에만 사용합니다. 범위는 1부터 3까지입니다 |
| `anchors` | `const float*` | `NULL` | ANCHOR 디코드에만 사용합니다. anchor의 `(w, h)` 값을 지정합니다 |
| `anchors_count` | `uint32_t` | 0 | `anchors` 배열의 float 원소 수입니다 |
| `reserved0` | `uint32_t` | 0 | 값을 변경하지 마십시오 |
| `proto` | `sp500_buffer` | 사용하지 않음 | SEGMENT 태스크에는 필수입니다. 형식은 `[proto_h][proto_w][mask_coeffs]` float입니다 |
| `mask_coeffs` | `int32_t` | 0 | SEGMENT 태스크에는 필수입니다. 검출 한 개의 마스크 계수 수입니다 |
| `proto_h`, `proto_w` | `int32_t` | 0 | SEGMENT 태스크에는 필수입니다. prototype의 크기입니다 |
| `mask_threshold` | `float` | 0 | SEGMENT 태스크에 사용합니다. logit이 이 값보다 크면 마스크 값은 1입니다 |
| `reserved[6]` | `uint32_t` | 0 | 값을 변경하지 마십시오 |

`anchors`는 stride 8, 16, 32 순서로 지정하십시오.
각 level에는 `anchors_per_level`개의 `(w, h)` 쌍이 필요합니다.
따라서 배열에는 `anchors_per_level * 6`개의 float 값이 필요합니다.
함수는 실행되는 동안에만 `anchors`를 읽습니다.

`anchors`가 `NULL`이면 COCO YOLOv5 기본 anchor를 사용합니다.
이 동작에는 `anchors_per_level == 3`이 필요합니다.

SEGMENT 태스크에는 다음 다섯 필드가 모두 필요합니다.

- `proto`
- `mask_coeffs`
- `proto_h`
- `proto_w`
- `mask_threshold`

```c
void sp500_yolo_params_init(sp500_yolo_params* params);
```

이 함수를 사용하여 파라미터를 기본값으로 초기화하십시오.

### `sp500_yolo_raw`

이 구조체는 ANCHORLESS, NMSFREE 및 DFLFREE 계열의 입력을 지정합니다.
배열 인덱스 0, 1, 2는 stride 8, 16, 32를 나타냅니다.
각 plane은 HWC float 형식입니다.

| 필드 | 조건 및 의미 |
|---|---|
| `box[3]` | 필수입니다. ANCHORLESS와 NMSFREE에는 4×16 DFL logits를 사용합니다. DFLFREE에는 거리 값 4개를 사용합니다 |
| `cls[3]` | 필수입니다. `num_classes`개의 logits를 사용합니다 |
| `extra[3]` | SEGMENT에는 `mask_coeffs`개의 값을 사용합니다. POSE에는 51개의 값을 사용합니다. DETECT에는 사용하지 않습니다 |
| `reserved[4]` | 0으로 유지하십시오 |

```c
void sp500_yolo_raw_init(sp500_yolo_raw* raw);
```

이 함수는 모든 plane을 사용하지 않는 상태로 초기화합니다.

### `sp500_yolo_raw_anchor`

이 구조체는 ANCHOR 계열의 입력을 지정합니다.
각 plane은 다음 형식을 사용합니다.

```text
[ny][nx][anchors_per_level * no]
no = 5 + num_classes + mask_coeffs
```

각 항목의 순서는 다음과 같습니다.

1. `xy`
2. `wh`
3. `objectness`
4. 클래스 점수
5. 마스크 계수

| 필드 | 조건 및 의미 |
|---|---|
| `plane[3]` | 필수입니다. 인덱스 0, 1, 2는 stride 8, 16, 32를 나타냅니다 |
| `reserved[4]` | 0으로 유지하십시오 |

```c
void sp500_yolo_raw_anchor_init(sp500_yolo_raw_anchor* raw);
```

이 함수는 모든 plane을 사용하지 않는 상태로 초기화합니다.

### `sp500_yolo_output`

| 필드 | 조건 및 의미 |
|---|---|
| `records` | 필수 출력 버퍼입니다. `sp500_yolo_records_bytes()`로 필요한 크기를 계산하십시오 |
| `mask` | SEGMENT 태스크에 필요한 출력 버퍼입니다. 각 레코드의 `mask_offset`이 이 버퍼를 가리킵니다 |
| `reserved[4]` | 0으로 유지하십시오 |

`records`에는 `params.max_detections`개의 레코드를 저장할 공간이 있어야 합니다.

```c
void sp500_yolo_output_init(sp500_yolo_output* output);
```

이 함수는 모든 출력 버퍼를 사용하지 않는 상태로 초기화합니다.

### `sp500_yolo_result`

| 필드 | 형식 | 의미 |
|---|---|---|
| `count` | `uint32_t` | `output.records`에 기록한 레코드 수입니다 |
| `record_floats` | `int32_t` | 레코드 한 개의 float 수입니다 |
| `host_seconds` | `double` | DSP 호출의 왕복 시간입니다. 단위는 초입니다 |
| `reserved[4]` | `uint32_t` | 사용하지 않습니다 |

### 도우미 함수

```c
int32_t sp500_yolo_record_floats(int32_t task);
```

이 함수는 태스크의 레코드 한 개에 필요한 float 수를 반환합니다.
태스크를 인식할 수 없으면 0을 반환합니다.

```c
uint32_t sp500_yolo_records_bytes(int32_t task, int32_t max_detections);
```

이 함수는 `output.records`에 필요한 바이트 수를 반환합니다.
인자가 잘못되면 0을 반환합니다.

```c
int32_t sp500_yolo_grid_cells(int32_t img_w, int32_t img_h);
```

이 함수는 stride 8, 16, 32의 grid cell 수를 합산합니다.
ANCHOR 계열에는 반환값에 `anchors_per_level`을 곱하십시오.
`img_w`나 `img_h`가 양수가 아니면 0을 반환합니다.

```c
sp500_status sp500_yolo_head_for_model(const char* model_name,
                                        int32_t* head);
```

이 함수는 모델 이름을 헤드 계열로 변환합니다.
이 함수는 변환한 값을 `head`에 저장합니다.

권장 이름은 [권장 모델 이름](#권장-모델-이름)을 참고하십시오.

변환에 성공하면 `SP500_OK`를 반환합니다.
인자가 `NULL`이면 `SP500_ERR_INVALID_ARG`를 반환합니다.
이름을 인식할 수 없어도 `SP500_ERR_INVALID_ARG`를 반환합니다.

### `sp500_yolo_decode()`

```c
sp500_status sp500_yolo_decode(int32_t head,
                               int32_t task,
                               const sp500_yolo_raw* raw,
                               const sp500_yolo_params* params,
                               sp500_yolo_output* output,
                               sp500_yolo_result* result);
```

| 인자 | 조건 및 의미 |
|---|---|
| `head` | `SP500_YOLO_HEAD_ANCHORLESS`, `SP500_YOLO_HEAD_NMSFREE` 또는 `SP500_YOLO_HEAD_DFLFREE`를 사용하십시오 |
| `task` | `SP500_YOLO_TASK_*` 값을 사용하십시오. NMSFREE는 DETECT만 지원합니다 |
| `raw` | 세 feature level의 입력 plane입니다 |
| `params` | YOLO 파라미터입니다. SEGMENT에는 `proto`, `mask_coeffs`, `proto_h`, `proto_w`, `mask_threshold`가 필요합니다 |
| `output` | `records`가 필요합니다. SEGMENT에는 `mask`도 필요합니다 |
| `result` | 레코드 수와 실행 시간을 받습니다. `NULL`을 사용할 수 있습니다 |

이 함수는 다음 상태를 반환할 수 있습니다.

- `SP500_OK`
- `SP500_ERR_UNSUPPORTED`
- `SP500_ERR_INVALID_ARG`
- `SP500_ERR_BUFFER_TOO_SMALL`
- `SP500_ERR_NO_MEMORY`
- `SP500_ERR_NOT_READY`
- `SP500_ERR_TIMEOUT`
- `SP500_ERR_DSP`

헤드 계열이 태스크를 지원하지 않으면 `SP500_ERR_UNSUPPORTED`를 반환합니다.
`records`가 `max_detections`개의 레코드를 저장할 수 없으면 `SP500_ERR_BUFFER_TOO_SMALL`을 반환합니다.

검출 결과가 없어도 함수는 `SP500_OK`를 반환합니다.
이 조건에서 `result.count`는 0입니다.

### `sp500_yolo_decode_anchor()`

```c
sp500_status sp500_yolo_decode_anchor(int32_t task,
                                      const sp500_yolo_raw_anchor* raw,
                                      const sp500_yolo_params* params,
                                      sp500_yolo_output* output,
                                      sp500_yolo_result* result);
```

이 함수는 `SP500_YOLO_HEAD_ANCHOR` 전용입니다.
이 함수는 DETECT와 SEGMENT 태스크를 지원합니다.
이 함수는 `params.anchors_per_level`과 `params.anchors`를 사용합니다.

반환 상태는 `sp500_yolo_decode()`와 같습니다.

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

이 함수는 디코드된 후보 테이블에서 결과를 선택합니다.
이 함수는 모든 헤드 계열을 지원합니다.
헤드 계열은 중복 제거 방식을 결정합니다.
헤드 계열은 objectness 열의 사용 여부도 결정합니다.

| 인자 | 조건 및 의미 |
|---|---|
| `head` | YOLO 헤드 계열입니다 |
| `task` | YOLO 태스크입니다 |
| `decoded` | 디코드된 후보 테이블입니다 |
| `num_rows` | 후보 행의 수입니다. 값은 1 이상이어야 합니다 |
| `params` | YOLO 파라미터입니다 |
| `output` | 출력 버퍼입니다 |
| `result` | 레코드 수와 실행 시간을 받습니다. `NULL`을 사용할 수 있습니다 |

`decoded`의 한 행은 다음 float 값을 포함합니다.

```text
4 + objectness + num_classes + extra
```

ANCHOR 계열에서는 `objectness`가 1입니다.
다른 계열에서는 `objectness`가 0입니다.
SEGMENT 태스크에서는 `extra`가 `mask_coeffs`입니다.
POSE 태스크에서는 `extra`가 51입니다.

표준 헤드에는 `sp500_yolo_grid_cells()`의 반환값을 `num_rows`로 사용하십시오.
ANCHOR 헤드에는 이 값에 `anchors_per_level`을 곱하십시오.

반환 상태는 `sp500_yolo_decode()`와 같습니다.
