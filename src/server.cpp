#include "server.h"
#include <iostream>
#include <fstream>
#include <memory>
#include <map>
#include <sstream>

// cpp-httplib 사용 여부 확인
#if __has_include(<httplib.h>)
    #define HTTPLIB_AVAILABLE
    #include <httplib.h>
#elif __has_include("httplib.h")
    #define HTTPLIB_AVAILABLE
    #include "httplib.h"
#else
    // cpp-httplib이 없는 경우를 위한 더미 구현
    namespace httplib {
        struct Request { 
            std::string body; 
            std::string method;
            std::string path;
        };
        struct Response { 
            int status = 200;
            std::map<std::string, std::string> headers;
            
            void set_content(const std::string& content, const std::string& type) {
                (void)content; (void)type; // unused warning 방지
            }
            void set_header(const std::string& key, const std::string& value) {
                headers[key] = value;
            }
        };
        class Server {
        public:
            bool listen(const std::string& host, int port) { 
                (void)host; (void)port; // unused warning 방지
                return false; 
            }
            void stop() {}
            template<typename Func>
            void Post(const std::string& pattern, Func func) { (void)pattern; (void)func; }
            template<typename Func>
            void Get(const std::string& pattern, Func func) { (void)pattern; (void)func; }
            template<typename Func>
            void Delete(const std::string& pattern, Func func) { (void)pattern; (void)func; }
            template<typename Func>
            void Options(const std::string& pattern, Func func) { (void)pattern; (void)func; }
            template<typename Func>
            void set_error_handler(Func func) { (void)func; }
        };
        class SSLServer : public Server {};
    }
#endif

MFAServer::MFAServer(int port, const std::string& cert_path, const std::string& key_path, const std::string& user_file) 
    : port(port), cert_path(cert_path), key_path(key_path) {
    
    // MFA 코어 초기화
    mfa_core = std::make_unique<MFACore>(user_file);
    
    // SSL 사용 여부 결정
    use_ssl = !cert_path.empty() && !key_path.empty();

#ifdef HTTPLIB_AVAILABLE
    if (use_ssl) {
        // SSL 인증서 파일 존재 확인
        std::ifstream cert_file(cert_path);
        std::ifstream key_file(key_path);
        
        if (!cert_file.good()) {
            throw std::runtime_error("SSL 인증서 파일을 열 수 없습니다: " + cert_path);
        }
        if (!key_file.good()) {
            throw std::runtime_error("SSL 키 파일을 열 수 없습니다: " + key_path);
        }
        
        ssl_server = std::make_unique<httplib::SSLServer>(cert_path.c_str(), key_path.c_str());
    } else {
        http_server = std::make_unique<httplib::Server>();
    }
#else
    std::cout << "경고: cpp-httplib이 설치되지 않았습니다. 더미 모드로 실행됩니다." << std::endl;
#endif
    
    setupRoutes();
    setupErrorHandlers();
}

MFAServer::~MFAServer() {
    stop();
}

void MFAServer::setupRoutes() {
#ifdef HTTPLIB_AVAILABLE
    httplib::Server* server = nullptr;
    
    if (use_ssl && ssl_server) {
        server = ssl_server.get();
    } else if (!use_ssl && http_server) {
        server = http_server.get();
    }
    
    if (!server) return;
    
    // API 라우트 설정
    server->Post("/api/register", [this](const httplib::Request& req, httplib::Response& res) {
        handleRegister(req, res);
    });
    
    server->Post("/api/authenticate", [this](const httplib::Request& req, httplib::Response& res) {
        handleAuthenticate(req, res);
    });
    
    server->Delete("/api/user/(.+)", [this](const httplib::Request& req, httplib::Response& res) {
        handleDelete(req, res);
    });
    
    server->Get("/api/users", [this](const httplib::Request& req, httplib::Response& res) {
        handleList(req, res);
    });
    
    server->Get("/health", [this](const httplib::Request& req, httplib::Response& res) {
        handleHealth(req, res);
    });
    
    // CORS 프리플라이트 요청 처리
    server->Options(".*", [this](const httplib::Request& req, httplib::Response& res) {
        (void)req; // unused parameter warning 방지
        setupCORS(res);
    });
#endif
}

void MFAServer::setupErrorHandlers() {
#ifdef HTTPLIB_AVAILABLE
    httplib::Server* server = nullptr;
    
    if (use_ssl && ssl_server) {
        server = ssl_server.get();
    } else if (!use_ssl && http_server) {
        server = http_server.get();
    }
    
    if (!server) return;
    
    server->set_error_handler([](const httplib::Request& req, httplib::Response& res) {
        (void)req; // unused parameter warning 방지
        res.set_content("{\"success\": false, \"error\": \"Internal server error\"}", "application/json");
    });
#endif
}

bool MFAServer::start() {
#ifdef HTTPLIB_AVAILABLE
    httplib::Server* server = nullptr;
    
    if (use_ssl && ssl_server) {
        server = ssl_server.get();
    } else if (!use_ssl && http_server) {
        server = http_server.get();
    }
    
    if (!server) {
        std::cerr << "서버 인스턴스가 초기화되지 않았습니다." << std::endl;
        return false;
    }
    
    std::cout << (use_ssl ? "HTTPS" : "HTTP") << " 서버가 포트 " << port << "에서 시작됩니다..." << std::endl;
    
    // 서버 시작 (블로킹)
    return server->listen("0.0.0.0", port);
#else
    std::cerr << "오류: cpp-httplib 라이브러리가 설치되어 있지 않습니다." << std::endl;
    std::cerr << "설치 방법:" << std::endl;
    std::cerr << "  wget https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h" << std::endl;
    std::cerr << "  sudo cp httplib.h /usr/local/include/" << std::endl;
    return false;
#endif
}

void MFAServer::stop() {
#ifdef HTTPLIB_AVAILABLE
    if (use_ssl && ssl_server) {
        ssl_server->stop();
    } else if (!use_ssl && http_server) {
        http_server->stop();
    }
#endif
}

void MFAServer::setupCORS(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    res.set_header("Access-Control-Max-Age", "3600");
}

void MFAServer::sendJSONResponse(httplib::Response& res, int status, const std::string& json) {
    setupCORS(res);
    res.status = status;
    res.set_content(json, "application/json");
}

void MFAServer::sendErrorResponse(httplib::Response& res, int status, const std::string& message) {
    std::string json = "{\"success\": false, \"error\": \"" + message + "\"}";
    sendJSONResponse(res, status, json);
}

void MFAServer::handleRegister(const httplib::Request& req, httplib::Response& res) {
    std::cout << "\n=== [DEBUG] Register Request Received ===" << std::endl;
    std::cout << "Request body: " << req.body << std::endl;
    
    try {
        // JSON 파싱 - user_id 추출
        std::string user_id;
        size_t start = req.body.find("\"user_id\"");
        if (start != std::string::npos) {
            start = req.body.find(":", start);
            if (start != std::string::npos) {
                start = req.body.find("\"", start) + 1;
                size_t end = req.body.find("\"", start);
                if (end != std::string::npos) {
                    user_id = req.body.substr(start, end - start);
                }
            }
        }
        
        std::cout << "[DEBUG] Parsed user_id: '" << user_id << "'" << std::endl;
        
        if (user_id.empty()) {
            std::cout << "[DEBUG] Error: user_id is empty" << std::endl;
            sendErrorResponse(res, 400, "Invalid request: user_id is required");
            return;
        }
        
        std::cout << "[DEBUG] Attempting to register user: " << user_id << std::endl;
        
        // 사용자 등록 시도
        User new_user;
        if (!mfa_core->registerUser(user_id, new_user)) {
            std::cout << "[DEBUG] Registration failed for user: " << user_id << std::endl;
            sendErrorResponse(res, 409, "User already exists or registration failed");
            return;
        }
        
        std::cout << "[DEBUG] User registered successfully!" << std::endl;
        std::cout << "[DEBUG] User ID: " << new_user.user_id << std::endl;
        std::cout << "[DEBUG] Secret: " << new_user.secret_base32 << std::endl;
        
        // QR 코드 URL 생성
        std::string qr_url = mfa_core->generateQRCodeURL(new_user);
        std::cout << "[DEBUG] QR URL: " << qr_url << std::endl;
        
        // 성공 응답 생성
        std::ostringstream json;
        json << "{"
             << "\"success\": true,"
             << "\"user_id\": \"" << new_user.user_id << "\","
             << "\"secret\": \"" << new_user.secret_base32 << "\","
             << "\"qr_code_url\": \"" << qr_url << "\","
             << "\"otp_uri\": \"" << mfa_core->generateOTPURI(new_user) << "\""
             << "}";
        
        std::cout << "[DEBUG] Sending response: " << json.str() << std::endl;
        sendJSONResponse(res, 200, json.str());
        std::cout << "=== [DEBUG] Register Request Completed ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "[DEBUG] Exception in handleRegister: " << e.what() << std::endl;
        sendErrorResponse(res, 500, "Internal server error: " + std::string(e.what()));
    }
}

void MFAServer::handleAuthenticate(const httplib::Request& req, httplib::Response& res) {
    std::cout << "\n=== [DEBUG] Authenticate Request Received ===" << std::endl;
    std::cout << "Request body: " << req.body << std::endl;
    
    try {
        // JSON 파싱 - user_id와 otp_code 추출
        std::string user_id, otp_code;
        
        // user_id 추출
        size_t start = req.body.find("\"user_id\"");
        if (start != std::string::npos) {
            start = req.body.find(":", start);
            if (start != std::string::npos) {
                start = req.body.find("\"", start) + 1;
                size_t end = req.body.find("\"", start);
                if (end != std::string::npos) {
                    user_id = req.body.substr(start, end - start);
                }
            }
        }
        
        // otp_code 추출
        start = req.body.find("\"otp_code\"");
        if (start != std::string::npos) {
            start = req.body.find(":", start);
            if (start != std::string::npos) {
                start = req.body.find("\"", start) + 1;
                size_t end = req.body.find("\"", start);
                if (end != std::string::npos) {
                    otp_code = req.body.substr(start, end - start);
                }
            }
        }
        
        std::cout << "[DEBUG] Parsed user_id: '" << user_id << "'" << std::endl;
        std::cout << "[DEBUG] Parsed otp_code: '" << otp_code << "'" << std::endl;
        
        if (user_id.empty() || otp_code.empty()) {
            std::cout << "[DEBUG] Error: user_id or otp_code is empty" << std::endl;
            sendErrorResponse(res, 400, "Invalid request: user_id and otp_code are required");
            return;
        }
        
        std::cout << "[DEBUG] Attempting to verify TOTP for user: " << user_id << std::endl;
        
        // TOTP 검증
        bool is_valid = mfa_core->verifyTOTP(user_id, otp_code);
        
        std::cout << "[DEBUG] TOTP verification result: " << (is_valid ? "SUCCESS" : "FAILED") << std::endl;
        
        if (is_valid) {
            std::cout << "[DEBUG] Sending success response" << std::endl;
            sendJSONResponse(res, 200, "{\"success\": true, \"message\": \"Authentication successful\"}");
        } else {
            std::cout << "[DEBUG] Sending failure response" << std::endl;
            sendJSONResponse(res, 401, "{\"success\": false, \"message\": \"Authentication failed\"}");
        }
        
        std::cout << "=== [DEBUG] Authenticate Request Completed ===" << std::endl;
        
    } catch (const std::exception& e) {
        sendErrorResponse(res, 500, "Internal server error: " + std::string(e.what()));
    }
}

void MFAServer::handleDelete(const httplib::Request& req, httplib::Response& res) {
    try {
        // URL에서 사용자 ID 추출 (/api/user/{user_id})
        std::string path = req.path;
        size_t last_slash = path.find_last_of('/');
        
        if (last_slash == std::string::npos || last_slash == path.length() - 1) {
            sendErrorResponse(res, 400, "Invalid request: user_id is required in URL");
            return;
        }
        
        std::string user_id = path.substr(last_slash + 1);
        
        if (user_id.empty()) {
            sendErrorResponse(res, 400, "Invalid request: user_id cannot be empty");
            return;
        }
        
        // 사용자 삭제 시도
        bool deleted = mfa_core->deleteUser(user_id);
        
        if (deleted) {
            std::ostringstream json;
            json << "{\"success\": true, \"message\": \"User '" << user_id << "' deleted successfully\"}";
            sendJSONResponse(res, 200, json.str());
        } else {
            std::ostringstream json;
            json << "{\"success\": false, \"message\": \"User '" << user_id << "' not found\"}";
            sendJSONResponse(res, 404, json.str());
        }
        
    } catch (const std::exception& e) {
        sendErrorResponse(res, 500, "Internal server error: " + std::string(e.what()));
    }
}

void MFAServer::handleList(const httplib::Request& req, httplib::Response& res) {
    std::cout << "\n=== [DEBUG] List Request Received ===" << std::endl;
    
    try {
        (void)req; // unused parameter warning 방지
        
        std::cout << "[DEBUG] Calling mfa_core->listUsers()" << std::endl;
        
        // 모든 사용자 목록 조회
        std::vector<std::string> users = mfa_core->listUsers();
        
        std::cout << "[DEBUG] Retrieved " << users.size() << " users" << std::endl;
        for (size_t i = 0; i < users.size(); i++) {
            std::cout << "[DEBUG] User " << i << ": " << users[i] << std::endl;
        }
        
        // JSON 응답 생성
        std::ostringstream json;
        json << "{"
             << "\"success\": true,"
             << "\"count\": " << users.size() << ","
             << "\"users\": [";
        
        for (size_t i = 0; i < users.size(); i++) {
            if (i > 0) json << ",";
            json << "\"" << users[i] << "\"";
        }
        
        json << "]}";
        
        std::cout << "[DEBUG] Sending response: " << json.str() << std::endl;
        sendJSONResponse(res, 200, json.str());
        std::cout << "=== [DEBUG] List Request Completed ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "[DEBUG] Exception in handleList: " << e.what() << std::endl;
        sendErrorResponse(res, 500, "Internal server error: " + std::string(e.what()));
    }
}

void MFAServer::handleHealth(const httplib::Request& req, httplib::Response& res) {
    (void)req; // unused parameter warning 방지
    sendJSONResponse(res, 200, "{\"status\": \"healthy\", \"service\": \"mfa-server\"}");
}
