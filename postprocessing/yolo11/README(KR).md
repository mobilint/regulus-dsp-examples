# YOLO11 예제

[English](README.md)

이 예제는 640x640 이미지에서 객체를 검출합니다.
Regulus-RB의 NPU가 YOLO11 추론을 실행합니다.
SP500 DSP가 출력 텐서를 디코드하고 중복 검출을 제거합니다.
프로그램은 검출 박스를 PNG 이미지에 표시합니다.

입력 이미지의 크기는 모델 입력과 같은 640x640이어야 합니다.
다른 크기의 이미지는 [letterbox 예제](../../preprocessing/letterbox/README(KR).md)를 사용하여 변환할 수 있습니다.

| 파일 | 설명 |
|---|---|
| `yolo11_demo.cc` | 예제 소스 코드 |
| `Makefile` | 크로스 빌드 설정 |
| `yolo11s.regulus-rb.mxq` | Regulus-RB용 YOLO11s 모델 |
| `test-sample.png` | 640x640 입력 이미지 |

공통 툴체인 설정과 DSP 사용 순서는 [저장소 README](../../README(KR).md)를 참고하십시오.

## 빌드

```shell
make
```

다른 툴체인을 사용하려면 설치 경로를 지정하십시오.

```shell
make TOOLCHAIN_ROOT=/opt/crosstools/mobilint/2.0.0/<version>
```

빌드가 완료되면 AArch64 실행 파일 `yolo11_demo`가 생성됩니다.

## 보드에 복사

`<board>`는 보드 주소입니다.

```shell
ssh root@<board> 'mkdir -p ~/yolo11_demo'
scp yolo11_demo yolo11s.regulus-rb.mxq test-sample.png root@<board>:~/yolo11_demo/
```

## DSP 펌웨어 로드

보드를 부팅한 뒤 `dsp_firmware_load`를 한 번 실행하십시오.
현재 부팅에서 이미 실행했다면 다시 실행할 필요가 없습니다.

```shell
ssh root@<board>
dsp_firmware_load
```

## 실행

```shell
cd ~/yolo11_demo
./yolo11_demo yolo11s.regulus-rb.mxq test-sample.png
```

정상 출력의 예는 다음과 같습니다.

```text
detections 1, decode 2.060 ms
  person         0.83  [234, 23, 420, 634]
saved yolo11_out.png
```

예제 실행이 끝나면 보드 셸을 종료하십시오.

```shell
exit
```

검출 결과 한 줄에는 클래스 이름, 신뢰도 점수 및 박스 좌표가 있습니다.
박스 좌표 형식은 `[x1, y1, x2, y2]`입니다.

## 결과

기본 출력 파일은 `yolo11_out.png`입니다.
출력 이미지에는 검출 박스, 클래스 이름 및 신뢰도 점수가 있습니다.

개발 PC에서 다음 명령을 실행하여 결과를 복사하십시오.

```shell
scp root@<board>:~/yolo11_demo/yolo11_out.png .
```
