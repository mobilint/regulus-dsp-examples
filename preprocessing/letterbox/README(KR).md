# SP500 letterbox 예제

[English](README.md)

이 예제는 입력 이미지를 640x640 letterbox 이미지로 변환합니다.
SP500 DSP가 이미지 크기를 변경하고 테두리를 추가합니다.
프로그램은 결과를 PNG 파일로 저장합니다.

| 파일 | 설명 |
|---|---|
| `letterbox_demo.cc` | 예제 소스 코드 |
| `Makefile` | 크로스 빌드 설정 |
| `test-sample-1080p.png` | 1920x1080 입력 이미지 |

공통 툴체인 설정과 DSP 사용 순서는 [저장소 README](../../README(KR).md)를 참고하십시오.

## 빌드

Makefile은 환경 설정 파일이 있는 최신 툴체인을 선택합니다.
그다음 필요한 SP500 파일을 검사합니다.
필수 파일이 없으면 빌드를 중단하고 `TOOLCHAIN_ROOT` 지정 방법을 안내합니다.

```shell
make
```

다른 툴체인을 사용하려면 설치 경로를 지정하십시오.

```shell
make TOOLCHAIN_ROOT=/opt/crosstools/mobilint/2.0.0/<version>
```

빌드가 완료되면 AArch64 실행 파일 `letterbox_demo`가 생성됩니다.

## 보드에 복사

`<board>`는 보드 주소입니다.

```shell
ssh root@<board> 'mkdir -p ~/letterbox_demo'
scp letterbox_demo test-sample-1080p.png root@<board>:~/letterbox_demo/
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
cd ~/letterbox_demo
./letterbox_demo test-sample-1080p.png
```

정상 출력의 예는 다음과 같습니다.

```text
input 1920x1080 -> output 640x640
scale 0.3333 pad_x 0 pad_y 140 dsp 6.163 ms
saved letterbox_out.png
```

출력 파일 이름을 두 번째 인자로 지정할 수 있습니다.

```shell
./letterbox_demo test-sample-1080p.png out.png
```

예제 실행이 끝나면 보드 셸을 종료하십시오.

```shell
exit
```

## 결과

기본 출력 파일은 `letterbox_out.png`입니다.
1920x1080 입력은 640x360으로 축소됩니다.
이미지는 640x640 출력의 중앙에 배치됩니다.
위쪽과 아래쪽에는 각각 140픽셀의 테두리가 생깁니다.
테두리의 픽셀 값은 114입니다.

개발 PC에서 다음 명령을 실행하여 결과를 복사하십시오.

```shell
scp root@<board>:~/letterbox_demo/letterbox_out.png .
```
