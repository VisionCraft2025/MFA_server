#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <memory>
#include "mfa_core.h"

// cpp-httplib 사용 여부 확인 및 조건부 포함
#if __has_include(<httplib.h>)
    #define HTTPLIB_AVAILABLE
    #include <httplib.h>
#elif __has_include("httplib.h")
    #define HTTPLIB_AVAILABLE
    #include "httplib.h"
#else
    // 더미 선언
    namespace httplib {
        struct Request { std::string body; };
        struct Response { int status = 200; };
        class Server {};
        class SSLServer {};
    }
#endif

/**
 * @brief MFA HTTPS 서버 클래스
 */
class MFAServer {
private:
#ifdef HTTPLIB_AVAILABLE
    std::unique_ptr<httplib::SSLServer> ssl_server;
    std::unique_ptr<httplib::Server> http_server;
#else
    void* ssl_server = nullptr;  // 더미 포인터
    void* http_server = nullptr; // 더미 포인터
#endif
    std::unique_ptr<MFACore> mfa_core;
    
    int port;
    bool use_ssl;
    std::string cert_path;
    std::string key_path;

    // 핸들러 메서드들
    void handleRegister(const httplib::Request& req, httplib::Response& res);
    void handleAuthenticate(const httplib::Request& req, httplib::Response& res);
    void handleDelete(const httplib::Request& req, httplib::Response& res);
    void handleList(const httplib::Request& req, httplib::Response& res);
    void handleHealth(const httplib::Request& req, httplib::Response& res);

    // 유틸리티 메서드들
    void setupRoutes();
    void setupCORS(httplib::Response& res);
    void setupErrorHandlers();
    bool validateJSONRequest(const std::string& body);
    void sendJSONResponse(httplib::Response& res, int status, const std::string& json);
    void sendErrorResponse(httplib::Response& res, int status, const std::string& message);

public:
    /**
     * @brief 생성자
     * @param port 서버 포트
     * @param cert_path SSL 인증서 파일 경로 (선택사항)
     * @param key_path SSL 키 파일 경로 (선택사항)
     * @param user_file 사용자 데이터 파일 경로
     */
    MFAServer(int port, 
              const std::string& cert_path = "", 
              const std::string& key_path = "",
              const std::string& user_file = DEFAULT_USER_FILE);

    /**
     * @brief 소멸자
     */
    ~MFAServer();

    /**
     * @brief 서버 시작
     * @return 성공 시 true, 실패 시 false
     */
    bool start();

    /**
     * @brief 서버 중지
     */
    void stop();

    /**
     * @brief SSL 사용 여부 확인
     * @return SSL 사용 시 true, HTTP 사용 시 false
     */
    bool isSSLEnabled() const { return use_ssl; }

    /**
     * @brief 서버 포트 반환
     * @return 포트 번호
     */
    int getPort() const { return port; }
};

#endif // SERVER_H
