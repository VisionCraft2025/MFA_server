#include "../server.h"
#include "../mfa_core.h"
#include <iostream>
#include <sstream>

// 이 파일은 나중에 server.cpp에서 분리된 인증 핸들러 로직을 담을 예정입니다.
// 현재는 기본 구조만 제공합니다.

namespace AuthHandler {
    
    // 인증 요청에서 사용자 ID와 OTP 추출
    struct AuthRequest {
        std::string user_id;
        std::string otp_code;
        bool valid = false;
    };
    
    AuthRequest parseAuthRequest(const std::string& json_body) {
        AuthRequest req;
        
        // 간단한 JSON 파싱 (나중에 nlohmann/json으로 교체 예정)
        size_t user_start = json_body.find("\"user_id\"");
        size_t otp_start = json_body.find("\"otp_code\"");
        
        if (user_start == std::string::npos || otp_start == std::string::npos) {
            return req; // invalid
        }
        
        // user_id 추출
        user_start = json_body.find(":", user_start);
        user_start = json_body.find("\"", user_start) + 1;
        size_t user_end = json_body.find("\"", user_start);
        req.user_id = json_body.substr(user_start, user_end - user_start);
        
        // otp_code 추출
        otp_start = json_body.find(":", otp_start);
        otp_start = json_body.find("\"", otp_start) + 1;
        size_t otp_end = json_body.find("\"", otp_start);
        req.otp_code = json_body.substr(otp_start, otp_end - otp_start);
        
        req.valid = !req.user_id.empty() && !req.otp_code.empty();
        return req;
    }
    
    // 응답 JSON 생성
    std::string createAuthResponse(bool success, const std::string& message = "") {
        std::ostringstream json;
        json << "{"
             << "\"success\": " << (success ? "true" : "false");
        
        if (!message.empty()) {
            json << ",\"message\": \"" << message << "\"";
        }
        
        json << "}";
        return json.str();
    }
    
    // OTP 코드 검증
    bool validateOTPFormat(const std::string& otp) {
        if (otp.length() != 6) return false;
        
        for (char c : otp) {
            if (c < '0' || c > '9') return false;
        }
        
        return true;
    }
}

// 향후 구현될 함수들:
// void handleAuthRequest(MFACore* mfa_core, const std::string& request_body, std::string& response);
// bool rateLimitCheck(const std::string& user_id);
