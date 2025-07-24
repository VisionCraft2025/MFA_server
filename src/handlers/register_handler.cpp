#include "../server.h"
#include "../mfa_core.h"
#include <iostream>
#include <sstream>

// 이 파일은 나중에 server.cpp에서 분리된 핸들러 로직을 담을 예정입니다.
// 현재는 기본 구조만 제공합니다.

namespace RegisterHandler {
    
    // JSON 파싱 헬퍼 함수 (간단한 구현)
    std::string extractUserID(const std::string& json_body) {
        // 간단한 JSON 파싱 (나중에 nlohmann/json으로 교체 예정)
        size_t start = json_body.find("\"user_id\"");
        if (start == std::string::npos) return "";
        
        start = json_body.find(":", start);
        if (start == std::string::npos) return "";
        
        start = json_body.find("\"", start);
        if (start == std::string::npos) return "";
        start++;
        
        size_t end = json_body.find("\"", start);
        if (end == std::string::npos) return "";
        
        return json_body.substr(start, end - start);
    }
    
    // 응답 JSON 생성 함수
    std::string createSuccessResponse(const User& user, const std::string& qr_url) {
        std::ostringstream json;
        json << "{"
             << "\"success\": true,"
             << "\"user_id\": \"" << user.user_id << "\","
             << "\"secret\": \"" << user.secret_base32 << "\","
             << "\"qr_code_url\": \"" << qr_url << "\""
             << "}";
        return json.str();
    }
    
    std::string createErrorResponse(const std::string& error) {
        std::ostringstream json;
        json << "{"
             << "\"success\": false,"
             << "\"error\": \"" << error << "\""
             << "}";
        return json.str();
    }
}

// 향후 구현될 함수들:
// void handleRegisterRequest(MFACore* mfa_core, const std::string& request_body, std::string& response);
// bool validateRegisterInput(const std::string& user_id);
