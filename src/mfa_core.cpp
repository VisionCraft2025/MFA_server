#include "mfa_core.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <ctime>
#include <random>
#include <unistd.h>
#include <algorithm>
#include <openssl/hmac.h>
#include <openssl/evp.h>

MFACore::MFACore(const std::string& user_file) : user_file_path(user_file) {
    // 데이터 디렉토리가 없으면 생성
    if (user_file_path.find('/') != std::string::npos) {
        std::string dir = user_file_path.substr(0, user_file_path.find_last_of('/'));
        // 간단한 디렉토리 생성 (실제로는 더 견고한 구현 필요)
        int result = system(("mkdir -p " + dir).c_str());
        (void)result; // unused variable warning 방지
    }
}

int MFACore::base32_decode(const std::string& encoded, std::vector<unsigned char>& result) {
    const std::string base32_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    int buffer = 0;
    int bits_left = 0;
    int count = 0;
    
    result.clear();
    result.reserve(encoded.length() * 5 / 8);
    
    for (size_t i = 0; i < encoded.length() && encoded[i] != '='; i++) {
        size_t pos = base32_chars.find(encoded[i]);
        if (pos == std::string::npos) {
            std::cerr << "Base32 디코딩 오류: 유효하지 않은 문자 '" << encoded[i] << "'" << std::endl;
            return -1;
        }
        
        buffer <<= 5;
        buffer |= pos;
        bits_left += 5;
        
        if (bits_left >= 8) {
            result.push_back(static_cast<unsigned char>(buffer >> (bits_left - 8)));
            count++;
            bits_left -= 8;
        }
    }
    return count;
}

std::string MFACore::base32_encode(const std::vector<unsigned char>& data) {
    const std::string base32_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    std::string result;
    
    int buffer = 0;
    int bits_left = 0;
    
    for (unsigned char byte : data) {
        buffer = (buffer << 8) | byte;
        bits_left += 8;
        
        while (bits_left >= 5) {
            result += base32_chars[(buffer >> (bits_left - 5)) & 0x1F];
            bits_left -= 5;
        }
    }
    
    if (bits_left > 0) {
        buffer <<= (5 - bits_left);
        result += base32_chars[buffer & 0x1F];
    }
    
    // 패딩 추가
    while (result.length() % 8 != 0) {
        result += '=';
    }
    
    return result;
}

std::string MFACore::generateSecret() {
    std::vector<unsigned char> key(SECRET_KEY_LENGTH);
    
    // /dev/urandom에서 랜덤 바이트 생성
    std::ifstream urandom("/dev/urandom", std::ios::binary);
    if (urandom.is_open()) {
        urandom.read(reinterpret_cast<char*>(key.data()), SECRET_KEY_LENGTH);
        urandom.close();
    } else {
        // fallback: C++ random number generator
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        
        for (int i = 0; i < SECRET_KEY_LENGTH; i++) {
            key[i] = static_cast<unsigned char>(dis(gen));
        }
    }
    
    return base32_encode(key);
}

bool MFACore::registerUser(const std::string& user_id, User& user) {
    std::cout << "[MFA_CORE] registerUser called with user_id: " << user_id << std::endl;
    
    // 중복 확인
    User existing_user;
    if (findUser(user_id, existing_user)) {
        std::cout << "[MFA_CORE] User already exists: " << user_id << std::endl;
        return false; // 이미 존재하는 사용자
    }
    
    std::cout << "[MFA_CORE] User not found, creating new user" << std::endl;
    
    // 새 사용자 생성
    user.user_id = user_id;
    user.secret_base32 = generateSecret();
    
    std::cout << "[MFA_CORE] Generated secret: " << user.secret_base32 << std::endl;
    
    // 파일에 저장
    bool result = saveUserToFile(user);
    std::cout << "[MFA_CORE] saveUserToFile result: " << (result ? "SUCCESS" : "FAILED") << std::endl;
    
    return result;
}

bool MFACore::findUser(const std::string& user_id, User& user) {
    std::vector<User> users = loadUsersFromFile();
    
    for (const auto& u : users) {
        if (u.user_id == user_id) {
            user = u;
            return true;
        }
    }
    
    return false;
}

int MFACore::generateTOTPCode(const std::string& secret_base32, time_t time_value) {
    if (time_value == 0) {
        time_value = time(nullptr);
    }
    
    std::vector<unsigned char> secret;
    int secret_len = base32_decode(secret_base32, secret);
    if (secret_len <= 0) {
        std::cerr << "TOTP 생성 실패: Base32 디코딩 오류" << std::endl;
        return -1;
    }
    
    // 시간 카운터 계산
    unsigned long long counter = static_cast<unsigned long long>(time_value) / OTP_PERIOD;
    unsigned char counter_bytes[8];
    for (int i = 7; i >= 0; i--) {
        counter_bytes[i] = static_cast<unsigned char>(counter & 0xff);
        counter >>= 8;
    }
    
    // HMAC-SHA1 계산
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;
    
    HMAC(EVP_sha1(), secret.data(), secret.size(), counter_bytes, 8, hash, &hash_len);
    
    if (hash_len == 0) {
        std::cerr << "HMAC 계산 실패" << std::endl;
        return -1;
    }
    
    // Dynamic truncation
    int offset = hash[hash_len - 1] & 0xf;
    int code = ((hash[offset] & 0x7f) << 24) |
               ((hash[offset + 1] & 0xff) << 16) |
               ((hash[offset + 2] & 0xff) << 8) |
               (hash[offset + 3] & 0xff);
    
    return code % 1000000;
}

bool MFACore::verifyTOTP(const std::string& user_id, const std::string& otp_code, int window) {
    std::cout << "[MFA_CORE] verifyTOTP called for user: " << user_id << ", OTP: " << otp_code << std::endl;
    
    User user;
    if (!findUser(user_id, user)) {
        std::cout << "[MFA_CORE] User not found: " << user_id << std::endl;
        return false;
    }
    
    std::cout << "[MFA_CORE] User found, secret: " << user.secret_base32 << std::endl;
    
    int input_code;
    try {
        input_code = std::stoi(otp_code);
    } catch (const std::exception&) {
        std::cout << "[MFA_CORE] Invalid OTP format: " << otp_code << std::endl;
        return false;
    }
    
    if (input_code < 0 || input_code > 999999) {
        std::cout << "[MFA_CORE] OTP out of range: " << input_code << std::endl;
        return false;
    }
    
    time_t current_time = time(nullptr);
    std::cout << "[MFA_CORE] Current time: " << current_time << std::endl;
    
    // 윈도우 범위 내에서 검증
    for (int i = -window; i <= window; i++) {
        time_t check_time = current_time + (i * OTP_PERIOD);
        int generated = generateTOTPCode(user.secret_base32, check_time);
        
        std::cout << "[MFA_CORE] Window " << i << ": generated=" << generated << ", input=" << input_code << std::endl;
        
        if (generated == input_code) {
            std::cout << "[MFA_CORE] OTP match found at window " << i << std::endl;
            return true;
        }
    }
    
    std::cout << "[MFA_CORE] No OTP match found" << std::endl;
    return false;
}

std::string MFACore::generateOTPURI(const User& user) {
    std::ostringstream uri;
    uri << "otpauth://totp/" << ISSUER_NAME << ":" << user.user_id
        << "?secret=" << user.secret_base32
        << "&issuer=" << ISSUER_NAME
        << "&algorithm=SHA1"
        << "&digits=" << OTP_DIGITS
        << "&period=" << OTP_PERIOD;
    
    return uri.str();
}

std::string MFACore::generateQRCodeURL(const User& user) {
    std::string otp_uri = generateOTPURI(user);
    std::ostringstream url;
    
    url << "https://api.qrserver.com/v1/create-qr-code/?size=200x200&data=";
    
    // URL 인코딩
    for (char c : otp_uri) {
        if (c == ':') url << "%3A";
        else if (c == '/') url << "%2F";
        else if (c == '?') url << "%3F";
        else if (c == '&') url << "%26";
        else if (c == '=') url << "%3D";
        else url << c;
    }
    
    return url.str();
}

bool MFACore::saveUserToFile(const User& user) {
    std::cout << "[MFA_CORE] saveUserToFile called for: " << user.user_id << std::endl;
    std::cout << "[MFA_CORE] File path: " << user_file_path << std::endl;
    
    std::ofstream file(user_file_path, std::ios::binary | std::ios::app);
    if (!file.is_open()) {
        std::cerr << "[MFA_CORE] 사용자 파일 열기 실패: " << user_file_path << std::endl;
        return false;
    }
    
    // 간단한 바이너리 형식으로 저장 (기존 C 구조체와 호환)
    char user_id_buf[MAX_USER_ID_LENGTH] = {0};
    char secret_buf[BASE32_ENCODED_MAX_LENGTH] = {0};
    
    strncpy(user_id_buf, user.user_id.c_str(), MAX_USER_ID_LENGTH - 1);
    strncpy(secret_buf, user.secret_base32.c_str(), BASE32_ENCODED_MAX_LENGTH - 1);
    
    std::cout << "[MFA_CORE] Writing user data to file..." << std::endl;
    file.write(user_id_buf, MAX_USER_ID_LENGTH);
    file.write(secret_buf, BASE32_ENCODED_MAX_LENGTH);
    
    bool result = file.good();
    std::cout << "[MFA_CORE] File write result: " << (result ? "SUCCESS" : "FAILED") << std::endl;
    
    return result;
}

std::vector<User> MFACore::loadUsersFromFile() {
    std::cout << "[MFA_CORE] loadUsersFromFile called" << std::endl;
    std::cout << "[MFA_CORE] File path: " << user_file_path << std::endl;
    
    std::vector<User> users;
    std::ifstream file(user_file_path, std::ios::binary);
    
    if (!file.is_open()) {
        std::cout << "[MFA_CORE] File does not exist or cannot be opened" << std::endl;
        return users; // 파일이 없으면 빈 벡터 반환
    }
    
    char user_id_buf[MAX_USER_ID_LENGTH];
    char secret_buf[BASE32_ENCODED_MAX_LENGTH];
    
    int user_count = 0;
    while (file.read(user_id_buf, MAX_USER_ID_LENGTH) && 
           file.read(secret_buf, BASE32_ENCODED_MAX_LENGTH)) {
        
        User user;
        user.user_id = std::string(user_id_buf);
        user.secret_base32 = std::string(secret_buf);
        
        // null 종료 문자 처리
        if (auto pos = user.user_id.find('\0'); pos != std::string::npos) {
            user.user_id = user.user_id.substr(0, pos);
        }
        if (auto pos = user.secret_base32.find('\0'); pos != std::string::npos) {
            user.secret_base32 = user.secret_base32.substr(0, pos);
        }
        
        std::cout << "[MFA_CORE] Loaded user " << user_count << ": " << user.user_id << std::endl;
        users.push_back(user);
        user_count++;
    }
    
    std::cout << "[MFA_CORE] Total users loaded: " << user_count << std::endl;
    
    return users;
}

bool MFACore::deleteUser(const std::string& user_id) {
    std::vector<User> users = loadUsersFromFile();
    
    auto it = std::remove_if(users.begin(), users.end(),
        [&user_id](const User& user) {
            return user.user_id == user_id;
        });
    
    if (it == users.end()) {
        return false; // 사용자를 찾지 못함
    }
    
    users.erase(it, users.end());
    
    // 파일 다시 쓰기
    std::ofstream file(user_file_path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }
    
    for (const auto& user : users) {
        char user_id_buf[MAX_USER_ID_LENGTH] = {0};
        char secret_buf[BASE32_ENCODED_MAX_LENGTH] = {0};
        
        strncpy(user_id_buf, user.user_id.c_str(), MAX_USER_ID_LENGTH - 1);
        strncpy(secret_buf, user.secret_base32.c_str(), BASE32_ENCODED_MAX_LENGTH - 1);
        
        file.write(user_id_buf, MAX_USER_ID_LENGTH);
        file.write(secret_buf, BASE32_ENCODED_MAX_LENGTH);
    }
    
    return file.good();
}

std::vector<std::string> MFACore::listUsers() {
    std::vector<std::string> user_ids;
    std::vector<User> users = loadUsersFromFile();
    
    for (const auto& user : users) {
        user_ids.push_back(user.user_id);
    }
    
    return user_ids;
}
