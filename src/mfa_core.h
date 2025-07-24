#ifndef MFA_CORE_H
#define MFA_CORE_H

#include <string>
#include <vector>
#include <memory>

// 상수 정의
constexpr int SECRET_KEY_LENGTH = 20;
constexpr int BASE32_ENCODED_MAX_LENGTH = 64;
constexpr int OTP_DIGITS = 6;
constexpr int OTP_PERIOD = 30;
constexpr int ALLOWED_DRIFT_STEPS = 1;
constexpr int MAX_USER_ID_LENGTH = 50;
constexpr const char* ISSUER_NAME = "My_Awesome_Project";
constexpr const char* DEFAULT_USER_FILE = "data/users.dat";

/**
 * @brief 사용자 정보 구조체
 */
struct User {
    std::string user_id;
    std::string secret_base32;
    
    User() = default;
    User(const std::string& id, const std::string& secret) 
        : user_id(id), secret_base32(secret) {}
};

/**
 * @brief MFA 핵심 기능을 제공하는 클래스
 */
class MFACore {
private:
    std::string user_file_path;

    // Base32 인코딩/디코딩 헬퍼 함수들
    int base32_decode(const std::string& encoded, std::vector<unsigned char>& result);
    std::string base32_encode(const std::vector<unsigned char>& data);

    // 파일 I/O 헬퍼 함수들
    bool saveUserToFile(const User& user);
    std::vector<User> loadUsersFromFile();

public:
    /**
     * @brief 생성자
     * @param user_file 사용자 데이터 파일 경로
     */
    explicit MFACore(const std::string& user_file = DEFAULT_USER_FILE);

    /**
     * @brief 랜덤 시크릿 키 생성
     * @return Base32로 인코딩된 시크릿 키
     */
    std::string generateSecret();

    /**
     * @brief 새 사용자 등록
     * @param user_id 사용자 ID
     * @param user 등록된 사용자 정보를 받을 구조체
     * @return 성공 시 true, 실패 시 false
     */
    bool registerUser(const std::string& user_id, User& user);

    /**
     * @brief 사용자 찾기
     * @param user_id 찾을 사용자 ID
     * @param user 찾은 사용자 정보를 받을 구조체
     * @return 찾았으면 true, 못 찾았으면 false
     */
    bool findUser(const std::string& user_id, User& user);

    /**
     * @brief TOTP 코드 생성
     * @param secret_base32 Base32로 인코딩된 시크릿 키
     * @param time_value 시간 값 (기본값: 현재 시간)
     * @return 6자리 TOTP 코드, 실패 시 -1
     */
    int generateTOTPCode(const std::string& secret_base32, time_t time_value = 0);

    /**
     * @brief TOTP 검증
     * @param user_id 사용자 ID
     * @param otp_code 입력받은 OTP 코드
     * @param window 허용할 시간 윈도우 (기본값: ALLOWED_DRIFT_STEPS)
     * @return 성공 시 true, 실패 시 false
     */
    bool verifyTOTP(const std::string& user_id, const std::string& otp_code, int window = ALLOWED_DRIFT_STEPS);

    /**
     * @brief OTP URI 생성 (QR 코드용)
     * @param user 사용자 정보
     * @return OTP URI 문자열
     */
    std::string generateOTPURI(const User& user);

    /**
     * @brief QR 코드 URL 생성 (웹 서비스용)
     * @param user 사용자 정보
     * @return QR 코드 이미지 URL
     */
    std::string generateQRCodeURL(const User& user);

    /**
     * @brief 사용자 삭제
     * @param user_id 삭제할 사용자 ID
     * @return 성공 시 true, 실패 시 false
     */
    bool deleteUser(const std::string& user_id);

    /**
     * @brief 모든 사용자 목록 조회
     * @return 사용자 ID 목록
     */
    std::vector<std::string> listUsers();
};

#endif // MFA_CORE_H
