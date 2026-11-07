// =====================================================================
//  LearnOpenGL - Shaders 장 / 덩어리 4: Shader 클래스 (shader.h)
//  (읽기용 사본. 실제로 쓸 때는 파일 이름을 shader.h 로 바꿔서 #include "shader.h")
//
//  ★ 표시는 "이 덩어리에서 새로 나온 것"에만 붙였다.
//   ★ 1) #ifndef / #define / #endif : 헤더 중복 포함 방지
//   ★ 2) Shader 클래스: 셰이더 관리 코드를 하나로 묶기 (ID, 생성자, use, setter)
//   ★ 3) 셰이더 소스를 "파일"에서 읽기 (ifstream, stringstream, rdbuf, exceptions, c_str)
//   ★ 4) use() : glUseProgram 감싸기
//   ★ 5) setBool / setInt / setFloat : uniform 설정을 이름 하나로 끝내기
//  (셰이더 컴파일·링크·오류 확인 코드는 덩어리 1~3의 main()에 있던 것을 그대로 옮긴 것이라 ★ 없음)
// =====================================================================

// ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
// ★ [새 내용 1] 헤더 가드 (#ifndef / #define / #endif)
// ★
// ★ 이 헤더를 여러 .cpp 파일이 #include 하면 같은 클래스 정의가 여러 번 들어가서 컴파일/링크 오류가 난다.
// ★ 그래서 "SHADER_H 라는 표시가 아직 없을 때만" 내용을 포함하고, 포함하는 순간 표시를 남긴다.
// ★ 두 번째부터는 #ifndef 가 거짓이라 통째로 건너뛴다. (#endif 까지)
// ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
#ifndef SHADER_H                       // ★ 아직 포함되지 않았을 때만
#define SHADER_H                       // ★ 포함 표시 (중복 포함 방지)

#include <glad/glad.h>                 // OpenGL 함수 선언

#include <string>                      // std::string
#include <fstream>                     // ★ 파일 읽기 (ifstream)
#include <sstream>                     // ★ 문자열 스트림 (stringstream)
#include <iostream>                    // 오류 출력

// ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
// ★ [새 내용 2] Shader 클래스: 셰이더 관련 코드를 하나로 묶기
// ★
// ★ 덩어리 1~3의 main()에는 셰이더 컴파일·링크 코드가 매번 수십 줄씩 반복됐다.
// ★ 그걸 클래스 하나로 옮겨서, main()에서는 이렇게 한 줄로 끝낸다:
// ★     Shader ourShader("shader.vs", "shader.fs");
// ★
// ★ 클래스가 가진 것:
// ★   ID            : 링크된 셰이더 프로그램의 번호 (예전의 shaderProgram 변수)
// ★   생성자        : 파일 두 개를 읽어서 컴파일 → 링크까지 전부 수행
// ★   use()         : glUseProgram(ID)
// ★   setBool/Int/Float : uniform 값 설정 (위치 조회 + glUniform 호출을 한 번에)
// ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
class Shader                                                            // ★ 셰이더 클래스
{
public:                                                                 // 바깥(main)에서 접근 가능
    unsigned int ID;                                                    // ★ 셰이더 프로그램 ID

    Shader(const char* vertexPath, const char* fragmentPath)            // ★ 생성자: 두 파일을 읽어 프로그램 생성
    {
        // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
        // ★ [새 내용 3] 셰이더 소스를 파일에서 읽기
        // ★
        // ★ 덩어리 1~3에서는 셰이더 코드가 C++ 안의 문자열("..."\n"...")이라 읽기 불편했다.
        // ★ 이제는 .vs/.fs 같은 일반 텍스트 파일에 셰이더를 쓰고, 실행 중에 읽어 온다.
        // ★   (확장자는 아무거나 상관없음. .vs/.fs 는 취향)
        // ★   => 셰이더를 고칠 때 C++ 를 다시 컴파일하지 않아도 된다 (프로그램만 다시 실행).
        // ★
        // ★ 읽는 흐름:
        // ★   ifstream.open(경로) → stringstream << rdbuf() (파일 내용 통째로 복사)
        // ★   → close() → str() 로 std::string 변환 → c_str() 로 C 문자열(const char*) 변환
        // ★   마지막 c_str() 이 필요한 이유: glShaderSource 가 const char* 를 받기 때문
        // ★
        // ★ exceptions(failbit | badbit): 파일을 못 열거나 읽기 실패하면 "예외"를 던지게 설정 → catch 에서 오류 출력
        // ★ 주의: 경로는 "프로그램을 실행한 위치" 기준이다. 못 찾으면 FILE_NOT_SUCCESFULLY_READ 가 뜬다.
        // ★ 읽기에 실패하면 소스가 빈 문자열이라, 이어서 컴파일 오류 로그도 같이 뜰 수 있다.
        // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
        // 1. 파일에서 소스 코드 읽기
        std::string vertexCode;                                         // ★ 정점 셰이더 소스 저장용
        std::string fragmentCode;                                       // ★ 프래그먼트 셰이더 소스 저장용
        std::ifstream vShaderFile;                                      // ★ 정점 셰이더 파일 스트림
        std::ifstream fShaderFile;                                      // ★ 프래그먼트 셰이더 파일 스트림
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit); // ★ 실패 시 예외 던지게 설정
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit); // ★ 실패 시 예외 던지게 설정
        try                                                             // ★ 예외가 날 수 있는 구간 시작
        {
            vShaderFile.open(vertexPath);                               // ★ 정점 셰이더 파일 열기
            fShaderFile.open(fragmentPath);                             // ★ 프래그먼트 셰이더 파일 열기
            std::stringstream vShaderStream, fShaderStream;             // ★ 내용을 담을 문자열 스트림
            vShaderStream << vShaderFile.rdbuf();                       // ★ 파일 버퍼 전체를 스트림으로 복사
            fShaderStream << fShaderFile.rdbuf();                       // ★ 파일 버퍼 전체를 스트림으로 복사
            vShaderFile.close();                                        // ★ 파일 닫기
            fShaderFile.close();                                        // ★ 파일 닫기
            vertexCode = vShaderStream.str();                           // ★ 스트림을 string으로 변환
            fragmentCode = fShaderStream.str();                         // ★ 스트림을 string으로 변환
        }
        catch (const std::ifstream::failure& e)                         // ★ 읽기 실패 시
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl; // ★ 오류 출력
        }
        const char* vShaderCode = vertexCode.c_str();                   // ★ OpenGL이 받는 C 문자열로 변환
        const char* fShaderCode = fragmentCode.c_str();                 // ★ OpenGL이 받는 C 문자열로 변환

        // 2. 셰이더 컴파일 (덩어리 1~3의 main()에 있던 코드를 그대로 옮김. 소스만 파일에서 읽은 문자열을 씀)
        unsigned int vertex, fragment;                                  // 셰이더 객체 ID
        int success;                                                    // 성공 여부
        char infoLog[512];                                              // 로그 버퍼

        vertex = glCreateShader(GL_VERTEX_SHADER);                      // 정점 셰이더 생성
        glShaderSource(vertex, 1, &vShaderCode, NULL);                  // 소스 연결 (파일에서 읽은 문자열)
        glCompileShader(vertex);                                        // 컴파일
        glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);             // 결과 조회
        if (!success)                                                   // 실패 시
        {
            glGetShaderInfoLog(vertex, 512, NULL, infoLog);             // 로그 가져오기
            std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl; // 출력
        }

        fragment = glCreateShader(GL_FRAGMENT_SHADER);                  // 프래그먼트 셰이더 생성
        glShaderSource(fragment, 1, &fShaderCode, NULL);                // 소스 연결 (파일에서 읽은 문자열)
        glCompileShader(fragment);                                      // 컴파일
        glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);           // 결과 조회
        if (!success)                                                   // 실패 시
        {
            glGetShaderInfoLog(fragment, 512, NULL, infoLog);           // 로그 가져오기
            std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl; // 출력
        }

        // 3. 프로그램 링크 (역시 기존 코드. 프로그램 번호를 지역 변수 대신 멤버 ID에 저장)
        ID = glCreateProgram();                                         // 프로그램 생성
        glAttachShader(ID, vertex);                                     // 정점 셰이더 부착
        glAttachShader(ID, fragment);                                   // 프래그먼트 셰이더 부착
        glLinkProgram(ID);                                              // 링크
        glGetProgramiv(ID, GL_LINK_STATUS, &success);                   // 결과 조회
        if (!success)                                                   // 실패 시
        {
            glGetProgramInfoLog(ID, 512, NULL, infoLog);                // 로그 가져오기
            std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl; // 출력
        }

        glDeleteShader(vertex);                                         // 프로그램에 링크됐으니 삭제
        glDeleteShader(fragment);                                       // 프로그램에 링크됐으니 삭제
    }

    // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
    // ★ [새 내용 4] use() : 이 셰이더 프로그램 활성화
    // ★
    // ★ main()의 렌더 루프에서 glUseProgram(shaderProgram) 대신 ourShader.use() 를 쓴다.
    // ★ 안에서 하는 일은 glUseProgram(ID) 하나뿐이다. (프로그램 번호를 클래스가 들고 있으니 인자가 필요 없음)
    // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
    void use()                                                          // ★ 이 셰이더 프로그램 활성화
    {
        glUseProgram(ID);                                               // 현재 프로그램으로 지정 (기존 함수 그대로)
    }

    // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
    // ★ [새 내용 5] uniform setter : 이름과 값만 주면 끝
    // ★
    // ★ 덩어리 2에서는 uniform 하나 넣으려고 이렇게 두 줄이 필요했다:
    // ★     int loc = glGetUniformLocation(shaderProgram, "ourColor");   // 이름 → 위치
    // ★     glUniform4f(loc, ...);                                       // 위치에 값 설정
    // ★ 그걸 함수 한 번으로 줄인다:   ourShader.setFloat("ourColor", 0.5f);
    // ★
    // ★ 타입별로 함수가 나뉜 이유: C 기반 OpenGL 에는 오버로딩이 없어서 glUniform1i / glUniform1f 처럼 따로 있다.
    // ★   (접미사 규칙은 덩어리 2에서 배움: i = int, f = float, 앞의 숫자 = 값 개수)
    // ★   bool 은 setter 가 따로 없어서 int(0/1)로 바꿔 glUniform1i 로 넘긴다.
    // ★
    // ★ 주의: 이 함수를 쓰려면 먼저 use() 로 프로그램을 활성화해야 한다. (glUniform 은 활성 프로그램에 값을 넣으므로)
    // ★ 참고: 이 장의 최종 main.cpp 에서는 uniform 을 안 써서 이 setter 들이 실제로 호출되지는 않는다.
    // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
    void setBool(const std::string& name, bool value) const             // ★ bool uniform 설정
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value); // ★ bool은 int로 바꿔 전달
    }
    void setInt(const std::string& name, int value) const               // ★ int uniform 설정
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);     // ★ 위치 조회 후 int 1개 설정
    }
    void setFloat(const std::string& name, float value) const           // ★ float uniform 설정
    {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);     // ★ 위치 조회 후 float 1개 설정
    }
};

#endif                                 // ★ 중복 포함 방지 끝