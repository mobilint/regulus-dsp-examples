# SP500 Vision DSP 예제

[English](README.md)

이 저장소는 Regulus-RB의 SP500 Vision DSP를 사용하는 예제를 제공합니다.
SP500은 영상 및 신호 처리 연산을 실행합니다.
호스트 응용 프로그램은 `libsp500.so`의 C API로 SP500을 제어합니다.

## 예제

| 구분 | 예제 | 기능 |
|---|---|---|
| 전처리 | [letterbox](preprocessing/letterbox/README(KR).md) | 이미지 비율을 유지하면서 크기를 변경하고 테두리를 추가합니다 |
| 전처리 | [log-mel](preprocessing/log-mel/README(KR).md) | PCM 오디오를 log-mel spectrogram으로 변환합니다 |
| 후처리 | [YOLO11](postprocessing/yolo11/README(KR).md) | YOLO11 출력 텐서를 검출 결과로 변환합니다 |
| 후처리 | [YOLO26](postprocessing/yolo26/README(KR).md) | YOLO26 출력 텐서를 검출 결과로 변환합니다 |
| 후처리 | [YOLOv8](postprocessing/yolov8/README(KR).md) | YOLOv8 출력 텐서를 검출 결과로 변환합니다 |

함수의 인자, 반환값, 제한 조건은 [SP500 API 참조](docs/sp500-api-reference(KR).md)를 참고하십시오.

## 툴체인

툴체인이 없으면 [Mobilint Image Archive](https://dl.mobilint.com)에서 Regulus-RB 툴체인을 다운로드하십시오.
압축을 풀고 설치 스크립트를 실행하십시오.

```shell
unset LD_LIBRARY_PATH
sh install-regulus-toolchain.sh -y
```

각 예제의 Makefile은 `/opt/crosstools/mobilint/2.0.0` 아래에서 환경 설정 파일이 있는 최신 툴체인을 선택합니다.
그다음 예제에 필요한 개발 파일을 검사합니다.
전처리 예제는 SP500 헤더와 라이브러리를 검사합니다.
YOLO 예제는 SP500과 qbruntime의 헤더와 라이브러리를 검사합니다.
필수 파일이 없으면 빌드를 중단하고 `TOOLCHAIN_ROOT` 지정 방법을 안내합니다.
빌드할 예제 디렉터리로 이동한 뒤 `make`를 실행하십시오.

```shell
cd preprocessing/letterbox
make
```

다른 툴체인을 사용하려면 설치 경로를 지정하십시오.

```shell
make TOOLCHAIN_ROOT=/opt/crosstools/mobilint/2.0.0/<version>
```

`TOOLCHAIN_ROOT`에는 `environment-setup-cortexa53-mobilint-linux` 파일과 예제에 필요한 모든 개발 파일이 있어야 합니다.

## DSP 펌웨어 로드

보드를 부팅한 뒤 다음 명령을 한 번 실행하십시오.

```shell
dsp_firmware_load
```

이 명령은 SP500 펌웨어를 로드하고 DSP 응용 프로그램을 시작합니다.
펌웨어는 보드가 꺼지거나 재부팅될 때까지 유지됩니다.

명령은 다음 파일을 사용합니다.

```text
/usr/lib/firmware/mobilint/sp500/ptcm.bin
/usr/lib/firmware/mobilint/sp500/dtcm.bin
/usr/lib/firmware/mobilint/sp500/external.bin
```

펌웨어가 시작되면 호스트 응용 프로그램이 SP500 API를 호출할 수 있습니다.

## SP500 API

라이브러리와 헤더는 Regulus-RB의 다음 경로에 있습니다.

```text
/usr/lib/libsp500.so
/usr/include/sp500/dsp.h
/usr/include/sp500/letterbox.h
/usr/include/sp500/log_mel.h
/usr/include/sp500/yolo.h
```

모듈 헤더는 `dsp.h`를 포함합니다.
응용 프로그램은 사용할 모듈의 헤더만 포함하면 됩니다.
공개 함수는 C 연결 규약을 사용하므로 C와 C++에서 호출할 수 있습니다.

### 공통 API

| 기능 | 함수 | 설명 |
|---|---|---|
| 상태 문자열 | `sp500_status_string()` | `sp500_status` 값을 문자열로 변환합니다 |
| DSP 연결 | `sp500_dsp_open()` | DSP 연결과 공유 메모리 매핑을 준비합니다 |
| 호출 제한 시간 | `sp500_set_dsp_timeout_ms()` | DSP 호출 한 번의 제한 시간을 설정합니다 |
| 호출 잠금 | `sp500_dsp_lock()`, `sp500_dsp_unlock()` | 여러 DSP 호출을 하나의 잠금 구간에서 실행합니다 |
| 일반 버퍼 | `sp500_buffer_wrap()` | 응용 프로그램 메모리를 SP500 버퍼로 지정합니다 |
| 공유 버퍼 | `sp500_buffer_alloc()`, `sp500_buffer_release()` | DSP와 NPU가 접근할 공유 메모리를 할당하고 해제합니다 |

`sp500_status`를 반환하는 함수는 `SP500_OK`이면 성공입니다.
음수 값은 오류입니다.

### 모듈 API

| 모듈 | 함수 | 입력 | 출력 |
|---|---|---|---|
| Letterbox | `sp500_letterbox()` | 8-bit HWC 이미지 | 크기를 변경하고 테두리를 추가한 이미지와 좌표 복원 값 |
| Log-mel | `sp500_log_mel()` | float32 PCM 오디오 | Whisper 형식의 log-mel spectrogram |
| YOLO | `sp500_yolo_decode()` | YOLO 헤드 출력 텐서 | 디코드, 신뢰도 필터 및 중복 제거를 적용한 결과 |

YOLO API는 `detect`, `segment`, `pose` 태스크를 지원합니다.
지원 범위는 헤드 계열에 따라 다릅니다.
`sp500_yolo_head_for_model()`은 모델 이름을 헤드 계열로 변환합니다.

## API 호출 순서

1. `sp500_dsp_open(0)`을 호출하십시오.
2. `*_params_init()`으로 파라미터 구조체를 초기화하십시오.
3. 필요한 파라미터를 설정하십시오.
4. 입력 및 출력용 `sp500_buffer`를 준비하십시오.
5. 모듈 함수를 호출하십시오.
6. 반환값이 `SP500_OK`인지 확인하십시오.
7. `sp500_buffer_alloc()`으로 만든 버퍼를 `sp500_buffer_release()`로 해제하십시오.

## 사용자 응용 프로그램 빌드

툴체인 환경을 적용하고 `libsp500.so`를 링크하십시오.

```shell
source <TOOLCHAIN_ROOT>/environment-setup-cortexa53-mobilint-linux
$CXX main.cc -lsp500 -o main
```

보드에서 펌웨어를 로드한 뒤 응용 프로그램을 실행하십시오.

```shell
dsp_firmware_load
./main
```
