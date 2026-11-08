/* ============================================================================
 * 덩어리 2 : 텍스처 두 장을 섞기 + 위아래 뒤집기   (본문: Texture Units ~ 장 끝)
 * ----------------------------------------------------------------------------
 * 이 덩어리의 새 내용 (★ 표시된 줄만, 덩어리 1에 이미 있던 것은 제외)
 *   1. 이미지 위아래 뒤집기    : stbi_set_flip_vertically_on_load(true)
 *   2. 두 번째 텍스처          : awesomeface.png 를 GL_RGBA 형식으로 올리기 (투명도 채널 포함)
 *   3. 프래그먼트 셰이더       : sampler2D 두 개 (texture1, texture2) + mix() 함수
 *   4. 샘플러 ↔ 텍스처 유닛 연결 : glUniform1i 직접 호출 / setInt (렌더 루프 전에 한 번)
 *   5. 텍스처 유닛 켜고 바인딩 : glActiveTexture(GL_TEXTURE0 / GL_TEXTURE1) + glBindTexture
 *   ※ 두 번째 텍스처를 만드는 절차(glGenTextures, glTexParameteri, stbi_load ...)는
 *     덩어리 1과 같은 반복이라 ★를 달지 않았다.
 *
 * 실행 전 준비물 : stb_image.h, container.jpg, awesomeface.png (작업 디렉터리에)
 * 파일 분리 안내 : 셰이더(.vs/.fs)는 문자열로 이 파일 안에 넣었다. stb_image.h 는
 *                  외부 라이브러리 헤더라 합칠 수 없어 따로 받아 둬야 한다.
 * ============================================================================ */

#include <glad/glad.h>                 // OpenGL 함수 주소를 불러오는 GLAD (GLFW보다 먼저 include)
#include <GLFW/glfw3.h>                // 창 만들기와 키 입력을 맡는 GLFW

#define STB_IMAGE_IMPLEMENTATION       // 이 파일에서 stb_image 함수의 몸체까지 만들라고 지정 (프로젝트 전체에서 한 번만)
#include "stb_image.h"                 // 이미지 로딩 라이브러리 헤더

#include <iostream>                    // 콘솔에 글자를 찍기 위한 표준 입출력

 // ---------------------------------------------------------------------------
 // 셰이더 소스 (앞 장에서는 .vs / .fs 파일이었지만, 파일 하나로 묶기 위해 문자열로 넣음)
 // ---------------------------------------------------------------------------
const char* vertexShaderSource = R"(#version 330 core   // GLSL 3.30 코어 버전을 쓴다
layout (location = 0) in vec3 aPos;      // 0번 속성: 정점 위치 (숫자 3개)
layout (location = 1) in vec3 aColor;    // 1번 속성: 정점 색 (숫자 3개)
layout (location = 2) in vec2 aTexCoord; // 2번 속성: 텍스처 좌표 (숫자 2개)
out vec3 ourColor;                       // 프래그먼트 셰이더로 넘길 색
out vec2 TexCoord;                       // 프래그먼트 셰이더로 넘길 텍스처 좌표
void main()                              // 정점 하나마다 실행되는 시작 함수
{                                        // main 시작
    gl_Position = vec4(aPos, 1.0);       // 위치를 그대로 화면 좌표로 내보냄 (w = 1.0)
    ourColor = aColor;                   // 받은 색을 그대로 다음 단계로 넘김
    TexCoord = aTexCoord;                // 받은 텍스처 좌표를 그대로 다음 단계로 넘김
}                                        // main 끝
)";                                      // 정점 셰이더 문자열 끝

const char* fragmentShaderSource = R"(#version 330 core // GLSL 3.30 코어 버전을 쓴다
out vec4 FragColor;                      // 이 픽셀의 최종 색 (출력)
in vec3 ourColor;                        // 정점 셰이더에서 보간되어 들어온 색 (여기선 안 씀)
in vec2 TexCoord;                        // 정점 셰이더에서 보간되어 들어온 텍스처 좌표
/* ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
 * ★ 샘플러 두 개 + mix()
 * ★ - 샘플러 하나 = 텍스처 유닛 하나. texture1 은 0번 유닛, texture2 는 1번 유닛을 읽도록 C++ 쪽에서 정해 준다.
 * ★ - mix(a, b, t): a 와 b 를 t 비율로 섞는다. t=0 이면 a 그대로, t=1 이면 b 그대로.
 * ★   0.2 → 상자(a) 80% + 웃는 얼굴(b) 20%.
 * ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★ */
uniform sampler2D texture1;              // ★ 첫 번째 텍스처(상자)를 가리키는 샘플러
uniform sampler2D texture2;              // ★ 두 번째 텍스처(웃는 얼굴)를 가리키는 샘플러
uniform float adjuster;
void main()                              // 픽셀 하나마다 실행되는 시작 함수
{                                        // main 시작
    FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2+adjuster); // ★ 두 텍스처 색을 8:2 로 섞어서 출력
}                                        // main 끝
)";                                      // 프래그먼트 셰이더 문자열 끝

// ---------------------------------------------------------------------------
// 셰이더 클래스 (앞 장 Shader 클래스를, 파일 경로 대신 소스 문자열을 받도록 줄인 것)
// ---------------------------------------------------------------------------
class Shader                                                          // 셰이더 프로그램 하나를 감싸는 클래스
{                                                                     // 클래스 시작
public:                                                               // 밖에서 쓸 수 있는 부분
    unsigned int ID;                                                  // 링크된 셰이더 프로그램의 번호
    Shader(const char* vSrc, const char* fSrc)                        // 정점·프래그먼트 소스 문자열을 받아 만드는 생성자
    {                                                                 // 생성자 시작
        unsigned int v = compile(GL_VERTEX_SHADER, vSrc, "VERTEX");    // 정점 셰이더 컴파일
        unsigned int f = compile(GL_FRAGMENT_SHADER, fSrc, "FRAGMENT");// 프래그먼트 셰이더 컴파일
        ID = glCreateProgram();                                       // 빈 셰이더 프로그램 만들기
        glAttachShader(ID, v);                                        // 정점 셰이더 붙이기
        glAttachShader(ID, f);                                        // 프래그먼트 셰이더 붙이기
        glLinkProgram(ID);                                            // 두 셰이더를 하나로 링크 (메모: 링크 후에야 texture1/texture2 위치를 찾을 수 있음)
        checkErrors(ID, "PROGRAM");                                   // 링크 에러가 있으면 출력
        glDeleteShader(v);                                            // 링크가 끝난 정점 셰이더 조각은 삭제
        glDeleteShader(f);                                            // 링크가 끝난 프래그먼트 셰이더 조각은 삭제
    }                                                                 // 생성자 끝
    void use() const { glUseProgram(ID); }                            // 이 프로그램을 "지금 쓰는 셰이더"로 설정
    void setInt(const char* name, int value) const                    // 정수 유니폼에 값 넣기
    {                                                                 // setInt 시작
        glUniform1i(glGetUniformLocation(ID, name), value);           // 이름으로 위치를 찾아 정수 하나 전달
    }       
    
    void setFloat(const char* name, float value) const
    {
        glUniform1f(glGetUniformLocation(ID, name), value);
    }
    // setInt 끝
private:                                                              // 클래스 안에서만 쓰는 부분
    static unsigned int compile(GLenum type, const char* src, const char* tag) // 셰이더 하나를 컴파일하는 함수
    {                                                                 // compile 시작
        unsigned int s = glCreateShader(type);                        // 빈 셰이더 객체 만들기
        glShaderSource(s, 1, &src, NULL);                             // 소스 문자열 연결
        glCompileShader(s);                                           // 컴파일
        checkErrors(s, tag);                                          // 컴파일 에러가 있으면 출력
        return s;                                                     // 셰이더 번호 돌려줌
    }                                                                 // compile 끝
    static void checkErrors(unsigned int obj, const char* tag)        // 컴파일/링크 에러 검사 함수
    {                                                                 // checkErrors 시작
        int ok;                                                       // 성공 여부를 받을 변수
        char log[1024];                                               // 에러 메시지를 받을 공간
        bool isProgram = (tag[0] == 'P');                             // "PROGRAM" 이면 링크 검사, 아니면 컴파일 검사
        if (isProgram) glGetProgramiv(obj, GL_LINK_STATUS, &ok);      // 링크 성공 여부 확인
        else           glGetShaderiv(obj, GL_COMPILE_STATUS, &ok);    // 컴파일 성공 여부 확인
        if (ok) return;                                               // 성공이면 그냥 끝
        if (isProgram) glGetProgramInfoLog(obj, 1024, NULL, log);     // 링크 에러 메시지 가져오기
        else           glGetShaderInfoLog(obj, 1024, NULL, log);      // 컴파일 에러 메시지 가져오기
        std::cout << "ERROR::" << tag << "\n" << log << std::endl;    // 에러 메시지 출력
    }                                                                 // checkErrors 끝
};                                                                    // 클래스 끝

void framebuffer_size_callback(GLFWwindow* window, int width, int height); // 창 크기 변경 콜백 선언
void processInput(GLFWwindow* window, Shader & ourshader);                                // 키 입력 처리 함수 선언

const unsigned int SCR_WIDTH = 800;                                   // 창 가로 크기
const unsigned int SCR_HEIGHT = 600;                                  // 창 세로 크기

int main()                                                            // 프로그램 시작점
{                                                                     // main 시작
    glfwInit();                                                       // GLFW 초기화
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);                    // OpenGL 주 버전 3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);                    // OpenGL 부 버전 3 (→ 3.3)
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);    // 코어 프로파일 사용
#ifdef __APPLE__                                                      // macOS 에서만
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);              // 전방 호환 모드 켜기 (macOS 필수)
#endif                                                                // macOS 조건 끝

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL); // 창 만들기
    if (window == NULL)                                               // 창 만들기에 실패하면
    {                                                                 // if 시작
        std::cout << "Failed to create GLFW window" << std::endl;     // 실패 메시지 출력
        glfwTerminate();                                              // GLFW 정리
        return -1;                                                    // 비정상 종료
    }                                                                 // if 끝
    glfwMakeContextCurrent(window);                                   // 이 창의 OpenGL 컨텍스트를 현재로 설정
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);// 창 크기 바뀌면 콜백 호출되게 등록

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))          // GLAD로 OpenGL 함수 주소 불러오기
    {                                                                 // if 시작
        std::cout << "Failed to initialize GLAD" << std::endl;        // 실패 메시지 출력
        return -1;                                                    // 비정상 종료
    }                                                                 // if 끝

    Shader ourShader(vertexShaderSource, fragmentShaderSource);       // 셰이더 컴파일 + 링크

    float vertices[] = {                                              // 정점 데이터 배열 시작
        // 위치               // 색                // 텍스처 좌표
         0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   2.0f, 2.0f,         // 오른쪽 위 정점 → 그림 오른쪽 위 (1,1)
         0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   2.0f, 0.0f,         // 오른쪽 아래 정점 → 그림 오른쪽 아래 (1,0)
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,         // 왼쪽 아래 정점 → 그림 왼쪽 아래 (0,0)
        -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 2.0f          // 왼쪽 위 정점 → 그림 왼쪽 위 (0,1)
    };                                                                // 정점 데이터 배열 끝
    unsigned int indices[] = {                                        // 인덱스(정점 번호) 배열 시작
        0, 1, 3,                                                      // 첫 번째 삼각형: 오른쪽 위·오른쪽 아래·왼쪽 위
        1, 2, 3                                                       // 두 번째 삼각형: 오른쪽 아래·왼쪽 아래·왼쪽 위
    };                                                                // 인덱스 배열 끝

    unsigned int VBO, VAO, EBO;                                       // 버퍼·배열 객체 번호를 담을 변수
    glGenVertexArrays(1, &VAO);                                       // VAO 하나 만들기
    glGenBuffers(1, &VBO);                                            // VBO 하나 만들기
    glGenBuffers(1, &EBO);                                            // EBO 하나 만들기

    glBindVertexArray(VAO);                                           // VAO 연결 (이후 설정이 여기에 기록됨)
    glBindBuffer(GL_ARRAY_BUFFER, VBO);                               // VBO 연결
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);       // 정점 데이터를 GPU로 복사
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);                       // EBO 연결
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); // 인덱스 데이터를 GPU로 복사

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);                   // 위치 속성: 숫자 3개, 정점 간격 8칸
    glEnableVertexAttribArray(0);                                     // 0번 속성 켜기
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); // 색 속성: 숫자 3개, 3칸 뒤에서 시작
    glEnableVertexAttribArray(1);                                     // 1번 속성 켜기
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); // 텍스처 좌표 속성: 숫자 2개, 6칸 뒤에서 시작
    glEnableVertexAttribArray(2);                                     // 2번 속성 켜기

    /* ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
     * ★ 이미지 위아래 뒤집기
     * ★ - OpenGL 은 텍스처 좌표 y=0 을 그림 "아래"로 보지만, 이미지 파일은 첫 줄이 "위"다 → 그대로면 거꾸로 보임.
     * ★ - 이 한 줄을 이미지를 읽기 "전에" 부르면, 이후 stbi_load 가 위아래를 뒤집어서 읽어 준다.
     * ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★ */
    stbi_set_flip_vertically_on_load(true);                           // ★ 앞으로 읽는 모든 이미지를 위아래 뒤집어서 읽기

    // ---- 첫 번째 텍스처 (상자) : 덩어리 1과 같은 절차 ----
    unsigned int texture1, texture2;                                  // 텍스처 두 개의 번호를 담을 변수
    glGenTextures(1, &texture1);                                      // 첫 번째 텍스처 객체 만들기
    glBindTexture(GL_TEXTURE_2D, texture1);                           // 첫 번째 텍스처 연결 (이후 설정은 여기에 적용)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);     // 가로축: 범위 밖이면 반복
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);     // 세로축: 범위 밖이면 반복
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // 축소될 때: 밉맵 두 장 + 픽셀 섞기
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // 확대될 때: 주변 픽셀 섞기
    int width, height, nrChannels;                                    // 이미지 가로·세로·채널 수를 받을 변수
    unsigned char* data = stbi_load("container.jpg", &width, &height, &nrChannels, 0); // 상자 이미지 읽기
    if (data)                                                         // 읽기에 성공했으면
    {                                                                 // if 시작
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data); // jpg 는 RGB 3채널로 올리기
        glGenerateMipmap(GL_TEXTURE_2D);                              // 밉맵 자동 생성
    }                                                                 // if 끝
    else                                                              // 읽기에 실패했으면
    {                                                                 // else 시작
        std::cout << "Failed to load texture" << std::endl;           // 실패 메시지 출력
    }                                                                 // else 끝
    stbi_image_free(data);                                            // CPU 쪽 이미지 메모리 해제

    // ---- 두 번째 텍스처 (웃는 얼굴) : 절차는 같고, 형식만 RGBA ----
    glGenTextures(1, &texture2);                                      // 두 번째 텍스처 객체 만들기
    glBindTexture(GL_TEXTURE_2D, texture2);                           // 두 번째 텍스처 연결 (이후 설정은 여기에 적용)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);     // 가로축: 범위 밖이면 반복
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);     // 세로축: 범위 밖이면 반복
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // 축소될 때: 밉맵 두 장 + 픽셀 섞기
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // 확대될 때: 주변 픽셀 섞기
    data = stbi_load("awesomeface.png", &width, &height, &nrChannels, 0); // 웃는 얼굴 이미지 읽기 (변수는 재사용)
    if (data)                                                         // 읽기에 성공했으면
    {                                                                 // if 시작
        /* ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
         * ★ png 는 투명도(alpha) 채널까지 4채널 → 원본 형식을 GL_RGBA 로 알려 줘야 한다.
         * ★ - 7번째 인자(원본 형식)를 RGB 로 두면 한 픽셀을 3바이트씩 잘못 읽어 그림이 깨진다.
         * ★ - 3번째 인자(GPU 쪽 저장 형식)는 본문대로 GL_RGB. (공식 소스는 GL_RGBA. 이 장 결과는 같다)
         * ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★ */
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data); // ★ 원본이 RGBA 4채널이라고 알려 주고 올리기
        glGenerateMipmap(GL_TEXTURE_2D);                              // 밉맵 자동 생성
    }                                                                 // if 끝
    else                                                              // 읽기에 실패했으면
    {                                                                 // else 시작
        std::cout << "Failed to load texture" << std::endl;           // 실패 메시지 출력
    }                                                                 // else 끝
    stbi_image_free(data);                                            // CPU 쪽 이미지 메모리 해제

    /* ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
     * ★ 샘플러 ↔ 텍스처 유닛 번호 연결 (렌더 루프 전에 한 번만)
     * ★ - 샘플러 유니폼에 넣는 정수 = "몇 번 텍스처 유닛을 읽을지". texture1 → 0번, texture2 → 1번.
     * ★ - 유니폼 값은 "지금 쓰는 셰이더"에 들어가므로 반드시 use() 를 먼저.
     * ★ - 아래 두 줄은 같은 일을 하는 두 가지 방법(직접 호출 / Shader 클래스).
     * ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★ */
    ourShader.use();                                                  // 유니폼을 넣기 전에 셰이더부터 사용 상태로
    glUniform1i(glGetUniformLocation(ourShader.ID, "texture1"), 0);   // ★ texture1 샘플러는 0번 유닛을 읽어라 (직접 호출)
    ourShader.setInt("texture2", 1);                                  // ★ texture2 샘플러는 1번 유닛을 읽어라 (클래스 함수로)

	ourShader.setFloat("adjuster", 0);                                    // ★ adjuster 유니폼은 0으로 초기화

    while (!glfwWindowShouldClose(window))                            // 창이 닫힐 때까지 반복 (렌더 루프)
    {                                                                 // 렌더 루프 시작
        processInput(window, ourShader);                                         // 키 입력 처리 (ESC면 종료 표시)

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);                         // 화면 지울 색 지정 (청록 회색)
        glClear(GL_COLOR_BUFFER_BIT);                                 // 그 색으로 화면 지우기

        /* ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
         * ★ 텍스처 유닛 켜고 바인딩
         * ★ - glActiveTexture(GL_TEXTUREn): n번 유닛(텍스처를 꽂는 슬롯)을 "현재 슬롯"으로 고른다.
         * ★ - 그다음 glBindTexture 는 그 슬롯에 텍스처를 꽂는다. 최소 16개(0~15), GL_TEXTURE0 + n 으로도 쓸 수 있음.
         * ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★ */
        glActiveTexture(GL_TEXTURE0);                                 // ★ 0번 슬롯을 현재 슬롯으로
        glBindTexture(GL_TEXTURE_2D, texture1);                       // ★ 0번 슬롯에 상자 텍스처 꽂기
        glActiveTexture(GL_TEXTURE1);                                 // ★ 1번 슬롯을 현재 슬롯으로
        glBindTexture(GL_TEXTURE_2D, texture2);                       // ★ 1번 슬롯에 웃는 얼굴 텍스처 꽂기

        ourShader.use();                                              // 우리 셰이더 사용
        glBindVertexArray(VAO);                                       // 사각형 VAO 연결
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);          // 인덱스 6개로 삼각형 2개 그리기

        glfwSwapBuffers(window);                                      // 그린 화면을 앞으로 내보내기
        glfwPollEvents();                                             // 키보드·마우스 등 이벤트 처리
    }                                                                 // 렌더 루프 끝

    glDeleteVertexArrays(1, &VAO);                                    // VAO 삭제
    glDeleteBuffers(1, &VBO);                                         // VBO 삭제
    glDeleteBuffers(1, &EBO);                                         // EBO 삭제

    glfwTerminate();                                                  // GLFW 정리
    return 0;                                                         // 정상 종료
}                                                                     // main 끝

float adjusterValue = 0.0f; 

void processInput(GLFWwindow* window, Shader& ourShader)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    bool changed = false;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        adjusterValue += 0.01f;
        changed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        adjusterValue -= 0.01f;
        changed = true;
    }

    // 0.2 + adjuster 가 0~1 을 벗어나지 않게
    if (adjusterValue > 0.8f)  adjusterValue = 0.8f;
    if (adjusterValue < -0.2f) adjusterValue = -0.2f;

    if (changed) {
        float t = 0.2f + adjusterValue;   // 셰이더의 mix 비율과 같은 값
        std::cout << "adjuster = " << adjusterValue
            << "  | 상자 " << (1.0f - t) * 100 << "% : 얼굴 " << t * 100 << "%"
            << std::endl;
    }

    ourShader.setFloat("adjuster", adjusterValue);
}                                                           // 함수 끝

void framebuffer_size_callback(GLFWwindow* window, int width, int height) // 창 크기가 바뀔 때 불리는 함수
{                                                                     // 함수 시작
    glViewport(0, 0, width, height);                                  // 그리는 영역을 새 창 크기에 맞춤
}                                                                     // 함수 끝