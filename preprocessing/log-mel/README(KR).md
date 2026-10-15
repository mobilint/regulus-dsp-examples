# SP500 log-mel 예제

[English](README.md)

이 예제는 16 kHz 모노 WAV 파일을 log-mel spectrogram으로 변환합니다.
SP500 DSP가 log-mel 값을 계산합니다.
프로그램은 결과를 PNG 파일로 저장합니다.

| 파일 | 설명 |
|---|---|
| `log_mel_demo.cc` | 예제 소스 코드 |
| `Makefile` | 크로스 빌드 설정 |
| `test-sample.wav` | 16 kHz 모노 16-bit PCM 입력 파일 |

`test-sample.wav`의 길이는 약 24초입니다.
파일은 NASA의 Apollo 11 교신 음성을 사용합니다.
원본은 [Wikimedia Commons](https://commons.wikimedia.org/wiki/File:Neil_Armstrong_small_step.wav)의 퍼블릭 도메인 파일입니다.
원본을 16 kHz 모노 16-bit PCM으로 변환했습니다.

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

빌드가 완료되면 AArch64 실행 파일 `log_mel_demo`가 생성됩니다.

## 보드에 복사

`<board>`는 보드 주소입니다.

```shell
ssh root@<board> 'mkdir -p ~/log_mel_demo'
scp log_mel_demo test-sample.wav root@<board>:~/log_mel_demo/
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
cd ~/log_mel_demo
./log_mel_demo test-sample.wav
```

정상 출력의 예는 다음과 같습니다.

```text
385803 samples, 24.11 s
2411 frames x 80 mels, dsp 42.971 ms
saved log_mel_out.png (2411x320)
```

예제 실행이 끝나면 보드 셸을 종료하십시오.

```shell
exit
```

## 결과

기본 출력 파일은 `log_mel_out.png`입니다.
각 프레임의 간격은 10 ms입니다.
프레임의 hop 길이는 160샘플입니다.
출력에는 80개의 mel 밴드가 있습니다.

이미지의 가로축은 시간입니다.
프레임 하나는 가로 1픽셀입니다.
세로축은 mel 밴드입니다.
밴드 하나는 세로 4픽셀입니다.
아래쪽이 낮은 주파수입니다.
밝은 픽셀은 큰 값을 나타냅니다.

개발 PC에서 다음 명령을 실행하여 결과를 복사하십시오.

```shell
scp root@<board>:~/log_mel_demo/log_mel_out.png .
```
