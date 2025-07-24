#include <iostream>
#include <string>
#include <csignal>
#include <memory>
#include "server.h"
#include "mfa_core.h"

// 전역 서버 인스턴스 (시그널 핸들링용)
std::unique_ptr<MFAServer> g_server;

// 시그널 핸들러
void signalHandler(int signal) {
    std::cout << "\n신호 " << signal << " 수신. 서버를 종료합니다..." << std::endl;
    if (g_server) {
        g_server->stop();
    }
    exit(0);
}

void printUsage(const char* program_name) {
    std::cout << "MFA HTTPS Server" << std::endl;
    std::cout << "사용법: " << program_name << " [옵션]" << std::endl;
    std::cout << std::endl;
    std::cout << "옵션:" << std::endl;
    std::cout << "  --port <포트>        서버 포트 (기본값: 8443)" << std::endl;
    std::cout << "  --cert <파일>        SSL 인증서 파일 경로" << std::endl;
    std::cout << "  --key <파일>         SSL 키 파일 경로" << std::endl;
    std::cout << "  --data <파일>        사용자 데이터 파일 경로 (기본값: data/users.dat)" << std::endl;
    std::cout << "  --help              이 도움말 출력" << std::endl;
    std::cout << std::endl;
    std::cout << "예시:" << std::endl;
    std::cout << "  HTTP 모드:  " << program_name << " --port 8080" << std::endl;
    std::cout << "  HTTPS 모드: " << program_name << " --port 8443 --cert server.crt --key server.key" << std::endl;
}

int main(int argc, char* argv[]) {
    // 기본 설정
    int port = 8443;
    std::string cert_path;
    std::string key_path;
    std::string data_file = DEFAULT_USER_FILE;

    // 명령행 인자 파싱
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
        else if (arg == "--port" && i + 1 < argc) {
            try {
                port = std::stoi(argv[++i]);
                if (port <= 0 || port > 65535) {
                    std::cerr << "오류: 유효하지 않은 포트 번호: " << port << std::endl;
                    return 1;
                }
            } catch (const std::exception&) {
                std::cerr << "오류: 유효하지 않은 포트 번호: " << argv[i] << std::endl;
                return 1;
            }
        }
        else if (arg == "--cert" && i + 1 < argc) {
            cert_path = argv[++i];
        }
        else if (arg == "--key" && i + 1 < argc) {
            key_path = argv[++i];
        }
        else if (arg == "--data" && i + 1 < argc) {
            data_file = argv[++i];
        }
        else {
            std::cerr << "오류: 알 수 없는 옵션: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }

    // SSL 설정 검증
    if ((!cert_path.empty() && key_path.empty()) || 
        (cert_path.empty() && !key_path.empty())) {
        std::cerr << "오류: SSL을 사용하려면 --cert와 --key를 모두 지정해야 합니다." << std::endl;
        return 1;
    }

    try {
        // 서버 생성
        g_server = std::make_unique<MFAServer>(port, cert_path, key_path, data_file);

        // 시그널 핸들러 등록
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);

        // 서버 정보 출력
        std::cout << "=== MFA Server ===" << std::endl;
        std::cout << "포트: " << port << std::endl;
        std::cout << "프로토콜: " << (g_server->isSSLEnabled() ? "HTTPS" : "HTTP") << std::endl;
        std::cout << "데이터 파일: " << data_file << std::endl;
        
        if (g_server->isSSLEnabled()) {
            std::cout << "SSL 인증서: " << cert_path << std::endl;
            std::cout << "SSL 키: " << key_path << std::endl;
        }
        
        std::cout << std::endl;
        std::cout << "API 엔드포인트:" << std::endl;
        std::cout << "  POST /api/register      - 사용자 등록" << std::endl;
        std::cout << "  POST /api/authenticate  - OTP 인증" << std::endl;
        std::cout << "  DELETE /api/user/<id>   - 사용자 삭제" << std::endl;
        std::cout << "  GET /api/users          - 사용자 목록" << std::endl;
        std::cout << "  GET /health             - 헬스 체크" << std::endl;
        std::cout << std::endl;

        // 서버 시작
        std::cout << "서버를 시작합니다..." << std::endl;
        if (!g_server->start()) {
            std::cerr << "오류: 서버 시작 실패" << std::endl;
            return 1;
        }

    } catch (const std::exception& e) {
        std::cerr << "오류: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
