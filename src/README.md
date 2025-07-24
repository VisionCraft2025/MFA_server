# MFA (Multi-Factor Authentication) Server

Google Authenticator와 호환되는 TOTP(Time-based One-Time Password) 기반의 다중 인증 서버입니다.

## 🚀 기능

- **사용자 등록**: 새로운 사용자 등록 및 시크릿 키 생성
- **TOTP 인증**: Google Authenticator 호환 6자리 OTP 코드 검증
- **사용자 관리**: 사용자 목록 조회 및 삭제
- **QR 코드 지원**: Google Authenticator 앱 연동을 위한 QR 코드 생성
- **RESTful API**: 표준 HTTP 메서드를 사용한 API 제공
- **실시간 디버깅**: 상세한 로그 출력으로 디버깅 지원

## 📋 요구사항

### 서버 빌드
- C++17 이상
- CMake 3.10 이상
- OpenSSL 라이브러리
- cpp-httplib 라이브러리

### 클라이언트 테스트
- Python 3.7 이상
- 필요한 Python 패키지:
  ```bash
  pip install requests pyotp qrcode[pil]
  ```

## 🛠️ 빌드 및 실행

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
./mfa-server
```

기본 설정:
- 포트: 8443
- 프로토콜: HTTP
- 데이터 파일: `data/users.dat`

## 📡 API 엔드포인트

### 1. 헬스 체크
**GET** `/health`

서버 상태를 확인합니다.

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

**요청 본문:**
```json
{
    "user_id": "사용자ID"
}
```

**응답 예시:**
```json
{
    "success": true,
    "user_id": "test_user",
    "secret": "NQDYP5LF4GHYTOLH4OQ5S4D53FBQNPBI",
    "qr_code_url": "https://api.qrserver.com/v1/create-qr-code/?size=200x200&data=otpauth%3A%2F%2Ftotp%2FMy_Awesome_Project%3Atest_user%3Fsecret%3DNQDYP5LF4GHYTOLH4OQ5S4D53FBQNPBI%26issuer%3DMy_Awesome_Project%26algorithm%3DSHA1%26digits%3D6%26period%3D30",
    "otp_uri": "otpauth://totp/My_Awesome_Project:test_user?secret=NQDYP5LF4GHYTOLH4OQ5S4D53FBQNPBI&issuer=My_Awesome_Project&algorithm=SHA1&digits=6&period=30"
}
```

### 3. TOTP 인증
**POST** `/api/authenticate`

Google Authenticator에서 생성된 6자리 OTP 코드로 사용자를 인증합니다.

**요청 본문:**
```json
{
    "user_id": "사용자ID",
    "otp_code": "123456"
}
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

**응답 예시:**
```json
{
    "success": true,
    "count": 2,
    "users": ["user1", "user2"]
}
```

### 5. 사용자 삭제
**DELETE** `/api/user/{user_id}`

지정된 사용자를 삭제합니다.

**성공 응답:**
```json
{
    "success": true,
    "message": "User 'test_user' deleted successfully"
}
```

**실패 응답:**
```json
{
    "success": false,
    "message": "User 'test_user' not found"
}
```

## 🖥️ 클라이언트 사용법

제공된 Python 클라이언트를 사용하여 API를 테스트할 수 있습니다.

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

### 옵션

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

### 보안 고려사항
- 시크릿 키는 `/dev/urandom`을 사용하여 생성
- TOTP 검증 시 시간 윈도우를 제한하여 재생 공격 방지
- 입력 검증을 통한 기본적인 보안 확보

## 🐛 디버깅

서버는 상세한 디버깅 로그를 출력합니다:
- 요청 수신 및 파싱 과정
- 사용자 등록/검색/삭제 과정
- TOTP 생성 및 검증 과정
- 파일 I/O 작업 결과

## 📄 예시 시나리오

### 완전한 워크플로우
```bash
# 1. 서버 상태 확인
python mfa_client_fixed.py health

# 2. 새 사용자 등록
python mfa_client_fixed.py register --user alice

# 3. Google Authenticator 앱으로 QR 코드 스캔

# 4. 등록된 사용자 확인
python mfa_client_fixed.py list

# 5. Google Authenticator에서 생성된 코드로 인증
python mfa_client_fixed.py auth --user alice --otp 789123

# 6. 사용자 삭제 (필요시)
python mfa_client_fixed.py delete --user alice
```

## 🚨 오류 처리

- **400 Bad Request**: 필수 파라미터 누락
- **401 Unauthorized**: OTP 인증 실패
- **404 Not Found**: 사용자를 찾을 수 없음
- **409 Conflict**: 이미 존재하는 사용자 등록 시도
- **500 Internal Server Error**: 서버 내부 오류

## 📚 참고사항

- TOTP는 RFC 6238 표준을 따릅니다
- Google Authenticator와 완전 호환됩니다
- 시간 동기화가 중요합니다 (서버와 클라이언트)
- 프로덕션 환경에서는 HTTPS 사용을 권장합니다

## 🔗 관련 링크

- [RFC 6238 - TOTP](https://tools.ietf.org/html/rfc6238)
- [Google Authenticator](https://play.google.com/store/apps/details?id=com.google.android.apps.authenticator2)
- [OpenSSL](https://www.openssl.org/)
- [cpp-httplib](https://github.com/yhirose/cpp-httplib)
