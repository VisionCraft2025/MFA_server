# MFA (Multi-Factor Authentication) Server

C++로 구현된 TOTP(Time-based One-Time Password) 인증 서버입니다. Google Authenticator와 완전 호환되며, RESTful API를 통해 다중 인증 기능을 제공합니다.

## 🚀 기능

- **사용자 등록**: 새로운 사용자 등록 및 시크릿 키 생성
- **TOTP 인증**: Google Authenticator 호환 6자리 OTP 코드 검증
- **사용자 관리**: 사용자 목록 조회 및 삭제
- **QR 코드 지원**: Google Authenticator 앱 연동을 위한 QR 코드 생성
- **RESTful API**: 표준 HTTP 메서드를 사용한 API 제공
- **실시간 디버깅**: 상세한 로그 출력으로 디버깅 지원
- **HTTPS 지원**: SSL/TLS 암호화 (선택사항)
- **CORS 지원**: 웹 클라이언트 호환

## 📋 요구사항

### 서버 빌드
- C++17 이상
- CMake 3.10 이상
- OpenSSL 라이브러리
- cpp-httplib 라이브러리
- libqrencode (선택사항)

### 클라이언트 테스트
- Python 3.7 이상
- 필요한 Python 패키지:
  ```bash
  pip install requests pyotp qrcode[pil]
  ```

## �️ 빌드 및 실행

### 의존성 설치

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential cmake pkg-config
sudo apt-get install libssl-dev libqrencode-dev

# cpp-httplib 설치 (헤더 온리 라이브러리)
# 방법 1: 패키지 매니저 (Ubuntu 20.04 이상)
sudo apt-get install libhttplib-dev

# 방법 2: 직접 설치
git clone https://github.com/yhirose/cpp-httplib.git
sudo cp cpp-httplib/httplib.h /usr/local/include/

# 방법 3: 직접 다운로드
wget https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h
sudo cp httplib.h /usr/local/include/
```

### 서버 빌드
```bash
cd MFA
mkdir -p build
cd build
cmake ..
make
```

### 서버 실행
```bash
# HTTP 모드 (개발용)
./mfa-server --port 8080

# 기본 설정으로 실행 (포트 8443)
./mfa-server

# HTTPS 모드 (운영용) - 인증서 필요
./mfa-server --port 8443 --cert ../certs/server.crt --key ../certs/server.key
```

기본 설정:
- 포트: 8443
- 프로토콜: HTTP (SSL 인증서 없이 실행시)
- 데이터 파일: `data/users.dat`

## 🔧 설정 옵션

```bash
./mfa-server [옵션]

옵션:
  --port <포트>        서버 포트 (기본값: 8443)
  --cert <파일>        SSL 인증서 파일 경로
  --key <파일>         SSL 키 파일 경로
  --data <파일>        사용자 데이터 파일 경로 (기본값: data/users.dat)
  --help              이 도움말 출력
```
```

## 📡 API 엔드포인트

### 1. 헬스 체크
**GET** `/health`

서버 상태를 확인합니다.

```bash
curl http://localhost:8080/health
```

**응답 예시:**
```json
{
    "status": "healthy",
    "service": "mfa-server"
}
```

### 2. 사용자 등록
**POST** `/api/register`

새로운 사용자를 등록하고 TOTP 시크릿을 생성합니다.

```bash
curl -X POST http://localhost:8080/api/register \
  -H "Content-Type: application/json" \
  -d '{"user_id": "john_doe"}'
```

**응답 예시:**
```json
{
    "success": true,
    "user_id": "john_doe",
    "secret": "NQDYP5LF4GHYTOLH4OQ5S4D53FBQNPBI",
    "qr_code_url": "https://api.qrserver.com/v1/create-qr-code/?size=200x200&data=otpauth%3A%2F%2Ftotp%2FMy_Awesome_Project%3Ajohn_doe%3Fsecret%3DNQDYP5LF4GHYTOLH4OQ5S4D53FBQNPBI%26issuer%3DMy_Awesome_Project%26algorithm%3DSHA1%26digits%3D6%26period%3D30",
    "otp_uri": "otpauth://totp/My_Awesome_Project:john_doe?secret=NQDYP5LF4GHYTOLH4OQ5S4D53FBQNPBI&issuer=My_Awesome_Project&algorithm=SHA1&digits=6&period=30"
}
```

### 3. TOTP 인증
**POST** `/api/authenticate`

Google Authenticator에서 생성된 6자리 OTP 코드로 사용자를 인증합니다.

```bash
curl -X POST http://localhost:8080/api/authenticate \
  -H "Content-Type: application/json" \
  -d '{"user_id": "john_doe", "otp_code": "123456"}'
```

**성공 응답:**
```json
{
    "success": true,
    "message": "Authentication successful"
}
```

**실패 응답:**
```json
{
    "success": false,
    "message": "Authentication failed"
}
```

### 4. 사용자 목록 조회
**GET** `/api/users`

등록된 모든 사용자 목록을 조회합니다.

```bash
curl http://localhost:8080/api/users
```

**응답 예시:**
```json
{
    "success": true,
    "count": 2,
    "users": ["john_doe", "alice"]
}
```

### 5. 사용자 삭제
**DELETE** `/api/user/{user_id}`

지정된 사용자를 삭제합니다.

```bash
curl -X DELETE http://localhost:8080/api/user/john_doe
```

**성공 응답:**
```json
{
    "success": true,
    "message": "User 'john_doe' deleted successfully"
}
```

**실패 응답:**
```json
{
    "success": false,
    "message": "User 'john_doe' not found"
}
```

## �️ 클라이언트 사용법

제공된 Python 클라이언트를 사용하여 API를 쉽게 테스트할 수 있습니다.

### 기본 사용법
```bash
python mfa_client_fixed.py <command> [options]
```

### 명령어

#### 1. 서버 상태 확인
```bash
python mfa_client_fixed.py health
```

#### 2. 사용자 등록
```bash
python mfa_client_fixed.py register --user my_user
```

등록 후 QR 코드가 터미널에 출력되므로 Google Authenticator 앱으로 스캔하세요.

#### 3. 사용자 목록 조회
```bash
python mfa_client_fixed.py list
```

#### 4. OTP 인증 (Google Authenticator 앱 사용)
```bash
python mfa_client_fixed.py auth --user my_user --otp 123456
```
여기서 `123456`은 Google Authenticator 앱에서 표시되는 실제 6자리 코드입니다.

#### 5. OTP 인증 (자동 생성)
```bash
python mfa_client_fixed.py auth --user my_user --secret NQDYP5LF4GHYTOLH4OQ5S4D53FBQNPBI
```
등록 시 받은 Base32 시크릿을 사용하여 자동으로 OTP를 생성하고 인증합니다.

#### 6. 사용자 삭제
```bash
python mfa_client_fixed.py delete --user my_user
```

#### 7. 전체 시나리오 테스트
```bash
python mfa_client_fixed.py full_test
```
등록 → 목록 조회 → 인증 → 삭제 → 목록 재조회 순서로 전체 기능을 테스트합니다.

### 클라이언트 옵션

- `--server URL`: 서버 주소 지정 (기본값: http://auth.kwon.pics:8443)
- `--ssl`: HTTPS 사용
- `--user USER_ID`: 사용자 ID
- `--otp OTP_CODE`: 6자리 OTP 코드 (Google Authenticator에서 생성)
- `--secret SECRET_KEY`: Base32 시크릿 키 (자동 OTP 생성용)

## 📱 Google Authenticator 설정

1. **사용자 등록** 후 출력되는 QR 코드를 Google Authenticator 앱으로 스캔
2. 또는 수동으로 다음 정보 입력:
   - **계정명**: 등록한 사용자 ID
   - **키**: 서버에서 제공한 Base32 시크릿
   - **발급자**: My_Awesome_Project

## �🛡️ 보안 기능

- **HTTPS 지원**: SSL/TLS 암호화 (선택사항)
- **TOTP 알고리즘**: RFC 6238 표준 준수
- **시간 윈도우**: ±30초 허용으로 시계 편차 대응
- **입력 검증**: JSON 요청 및 파라미터 검증
- **CORS 지원**: 웹 클라이언트 호환
- **시크릿 키 보안**: `/dev/urandom`을 사용한 안전한 키 생성
- **재생 공격 방지**: 제한된 시간 윈도우로 보안 강화

## 🔧 구현 세부사항

### TOTP 알고리즘
- **알고리즘**: HMAC-SHA1
- **자릿수**: 6자리
- **시간 간격**: 30초
- **허용 시간 편차**: ±30초 (총 3개 시간 윈도우)

### 데이터 저장
- 사용자 데이터는 바이너리 파일(`data/users.dat`)에 저장
- 각 사용자 레코드는 고정 크기 구조체로 저장
- 사용자 ID: 최대 50바이트
- 시크릿 키: 최대 64바이트 (Base32 인코딩)

## 📂 프로젝트 구조

```
MFA/
├── src/
│   ├── main.cpp              # 메인 엔트리포인트
│   ├── mfa_core.cpp         # MFA 핵심 로직
│   ├── mfa_core.h           # MFA 헤더
│   ├── server.cpp           # HTTP 서버
│   ├── server.h             # 서버 헤더
│   └── handlers/            # API 핸들러
│       ├── register_handler.cpp
│       └── auth_handler.cpp
├── certs/                   # SSL 인증서
├── data/                    # 사용자 데이터
├── CMakeLists.txt          # 빌드 설정
└── README.md
```

## 🧪 테스트

### 기본 테스트

```bash
# 서버 시작
./mfa-server --port 8080 &

# 사용자 등록 테스트
curl -X POST http://localhost:8080/api/register \
  -H "Content-Type: application/json" \
  -d '{"user_id": "test_user"}'

# 헬스 체크
curl http://localhost:8080/health
```

### Google Authenticator 연동

1. 사용자 등록 API 호출
2. 응답에서 `qr_code_url` 확인
3. Google Authenticator 앱에서 QR 코드 스캔
4. 앱에서 생성된 6자리 코드로 인증 테스트

## 🔄 현재 개발 상태

### ✅ 완료된 기능

- [x] 프로젝트 구조 설정
- [x] MFA 핵심 로직 (C → C++ 포팅)
- [x] 기본 HTTP 서버 구조
- [x] CMake 빌드 시스템
- [x] 명령행 인터페이스

### 🚧 진행 중

- [ ] API 핸들러 구현
- [ ] JSON 파싱 (nlohmann/json 통합)
- [ ] SSL/TLS 설정
- [ ] 에러 처리 강화

### 📋 다음 단계

- [ ] Rate limiting
- [ ] 로깅 시스템
- [ ] 단위 테스트
- [ ] Docker 컨테이너화
- [ ] API 문서화 (Swagger)

## �️ 클라이언트 사용법

제공된 Python 클라이언트를 사용하여 API를 쉽게 테스트할 수 있습니다.

### 기본 사용법
```bash
python mfa_client_fixed.py <command> [options]
```

### 명령어

#### 1. 서버 상태 확인
```bash
python mfa_client_fixed.py health
```

#### 2. 사용자 등록
```bash
python mfa_client_fixed.py register --user my_user
```

등록 후 QR 코드가 터미널에 출력되므로 Google Authenticator 앱으로 스캔하세요.

#### 3. 사용자 목록 조회
```bash
python mfa_client_fixed.py list
```

#### 4. OTP 인증 (Google Authenticator 앱 사용)
```bash
python mfa_client_fixed.py auth --user my_user --otp 123456
```
여기서 `123456`은 Google Authenticator 앱에서 표시되는 실제 6자리 코드입니다.

#### 5. OTP 인증 (자동 생성)
```bash
python mfa_client_fixed.py auth --user my_user --secret NQDYP5LF4GHYTOLH4OQ5S4D53FBQNPBI
```
등록 시 받은 Base32 시크릿을 사용하여 자동으로 OTP를 생성하고 인증합니다.

#### 6. 사용자 삭제
```bash
python mfa_client_fixed.py delete --user my_user
```

#### 7. 전체 시나리오 테스트
```bash
python mfa_client_fixed.py full_test
```
등록 → 목록 조회 → 인증 → 삭제 → 목록 재조회 순서로 전체 기능을 테스트합니다.

### 클라이언트 옵션

- `--server URL`: 서버 주소 지정 (기본값: http://auth.kwon.pics:8443)
- `--ssl`: HTTPS 사용
- `--user USER_ID`: 사용자 ID
- `--otp OTP_CODE`: 6자리 OTP 코드 (Google Authenticator에서 생성)
- `--secret SECRET_KEY`: Base32 시크릿 키 (자동 OTP 생성용)

## 📱 Google Authenticator 설정

1. **사용자 등록** 후 출력되는 QR 코드를 Google Authenticator 앱으로 스캔
2. 또는 수동으로 다음 정보 입력:
   - **계정명**: 등록한 사용자 ID
   - **키**: 서버에서 제공한 Base32 시크릿
   - **발급자**: My_Awesome_Project

## 🛡️ 보안 기능

- **HTTPS 지원**: SSL/TLS 암호화 (선택사항)
- **TOTP 알고리즘**: RFC 6238 표준 준수
- **시간 윈도우**: ±30초 허용으로 시계 편차 대응
- **입력 검증**: JSON 요청 및 파라미터 검증
- **CORS 지원**: 웹 클라이언트 호환
- **시크릿 키 보안**: `/dev/urandom`을 사용한 안전한 키 생성
- **재생 공격 방지**: 제한된 시간 윈도우로 보안 강화

## 🔧 구현 세부사항

### TOTP 알고리즘
- **알고리즘**: HMAC-SHA1
- **자릿수**: 6자리
- **시간 간격**: 30초
- **허용 시간 편차**: ±30초 (총 3개 시간 윈도우)

### 데이터 저장
- 사용자 데이터는 바이너리 파일(`data/users.dat`)에 저장
- 각 사용자 레코드는 고정 크기 구조체로 저장
- 사용자 ID: 최대 50바이트
- 시크릿 키: 최대 64바이트 (Base32 인코딩)

## ❗ 문제 해결

### 일반적인 문제들

#### 1. 컴파일 오류
**문제**: OpenSSL 라이브러리를 찾을 수 없음
```
fatal error: openssl/hmac.h: No such file or directory
```

**해결책**:
```bash
# Ubuntu/Debian
sudo apt-get install libssl-dev

# CentOS/RHEL
sudo yum install openssl-devel
```

#### 2. 빌드 디렉토리 문제
**문제**: CMake 구성 오류
```
CMake Error: The source directory does not appear to contain CMakeLists.txt
```

**해결책**:
```bash
cd MFA
rm -rf build
mkdir build
cd build
cmake ..
make
```

#### 3. 서버 실행 오류
**문제**: 포트가 이미 사용 중
```
bind: Address already in use
```

**해결책**:
```bash
# 8443 포트를 사용하는 프로세스 확인
sudo lsof -i :8443

# 프로세스 종료 후 서버 재시작
sudo kill -9 <PID>
./mfa-server
```

#### 4. OTP 인증 실패
**문제**: Google Authenticator의 OTP가 항상 실패
- **시간 동기화 확인**: 서버와 클라이언트의 시간이 동기화되어 있는지 확인
- **시크릿 키 확인**: 등록 시 받은 Base32 시크릿이 정확한지 확인
- **QR 코드 재생성**: 사용자를 삭제하고 다시 등록

#### 5. Python 클라이언트 오류
**문제**: 모듈을 찾을 수 없음
```
ModuleNotFoundError: No module named 'pyotp'
```

**해결책**:
```bash
pip install pyotp qrcode requests
```

### 디버깅 팁

1. **서버 로그 확인**: 서버는 자세한 디버그 정보를 콘솔에 출력합니다.
2. **curl 테스트**: 기본적인 API 테스트를 위해 curl 사용:
   ```bash
   curl -X GET http://localhost:8443/api/health
   ```
3. **데이터 파일 확인**: `data/users.dat` 파일이 올바르게 생성되었는지 확인
4. **네트워크 연결**: 방화벽 설정이 8443 포트를 차단하지 않는지 확인

### cpp-httplib 설치 문제

```bash
# 직접 다운로드
wget https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h
sudo cp httplib.h /usr/local/include/
```

### SSL 인증서 생성 (개발용)

```bash
mkdir -p certs
openssl req -x509 -newkey rsa:4096 -keyout certs/server.key -out certs/server.crt -days 365 -nodes -subj "/CN=localhost"
```

## 🚀 향후 개발 계획

- [ ] 웹 기반 관리 인터페이스
- [ ] 사용자 그룹 관리
- [ ] 백업 키 생성
- [ ] 로깅 시스템
- [ ] 단위 테스트
- [ ] Docker 컨테이너화
- [ ] API 문서화 (Swagger)

## 📚 추가 자료

- [RFC 6238 - TOTP 표준](https://tools.ietf.org/html/rfc6238)
- [Google Authenticator 공식 문서](https://github.com/google/google-authenticator)
- [OpenSSL 문서](https://www.openssl.org/docs/)
- [cpp-httplib GitHub](https://github.com/yhirose/cpp-httplib)

## 👥 기여하기

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📄 라이선스

이 프로젝트는 MIT 라이선스 하에 배포됩니다. 자세한 내용은 `LICENSE` 파일을 참조하세요.

---

**🔐 MFA 시스템 - 보안을 위한 첫 걸음**

문제가 있거나 개선사항이 있다면 이슈를 생성해 주세요!

## 📄 라이선스

MIT License
