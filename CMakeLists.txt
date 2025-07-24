cmake_minimum_required(VERSION 3.16)
project(mfa-server)

# C++17 표준 사용
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 컴파일 옵션
set(CMAKE_CXX_FLAGS "-Wall -Wextra -O2")
set(CMAKE_CXX_FLAGS_DEBUG "-g")

# 필요한 패키지 찾기
find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED)

# libqrencode 찾기
pkg_check_modules(QRENCODE REQUIRED libqrencode)

# 소스 파일들
set(SOURCES
    src/main.cpp
    src/mfa_core.cpp
    src/server.cpp
    src/handlers/register_handler.cpp
    src/handlers/auth_handler.cpp
)

# 실행 파일 생성
add_executable(mfa-server ${SOURCES})

# ==================== 주요 변경 사항 ====================

# 1. SSL 지원을 위한 컴파일 정의 추가 (가장 중요)
target_compile_definitions(mfa-server PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)

# 2. 헤더 파일 포함 디렉토리 설정 (Modern CMake 방식)
target_include_directories(mfa-server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${QRENCODE_INCLUDE_DIRS}
)
# OpenSSL의 include 경로는 아래 link_libraries에서 OpenSSL::SSL을 사용하면 자동으로 처리됩니다.

# 3. 라이브러리 링크 (Modern CMake 방식)
target_link_libraries(mfa-server PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ${QRENCODE_LIBRARIES}
    pthread
)
# =======================================================

# 설치 규칙
install(TARGETS mfa-server DESTINATION bin)

# 데이터 디렉토리 생성
install(DIRECTORY DESTINATION var/lib/mfa-server)

# 인증서 디렉토리 (옵션)
install(DIRECTORY certs/ DESTINATION etc/mfa-server/certs
        FILES_MATCHING PATTERN "*.crt" PATTERN "*.key")