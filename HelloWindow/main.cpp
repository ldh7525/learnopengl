// ============================================================================
// [덩어리 2] 텍스처 입히기 : 텍스처 좌표 → 셰이더 → 사각형에 나무 상자 그림
// ----------------------------------------------------------------------------
// 다 치고 나면 : 사각형 위에 container.jpg(나무 상자)가 보인다.
//                (프래그먼트 셰이더의 "디스코" 줄을 켜면 정점 색과 섞인 상자가 보인다)
// ----------------------------------------------------------------------------
// 이 덩어리의 새 내용 (덩어리 1에 없던 것만, ★ 표시 줄)
//   1. 정점 데이터에 텍스처 좌표(s, t) 2개 추가 → 꼭짓점 하나가 float 8개
//   2. glVertexAttribPointer(2, ...) / glEnableVertexAttribArray(2) : 텍스처 좌표용 2번 입력 칸
//   3. 정점 셰이더 : layout(location = 2) in vec2 aTexCoord, out vec2 TexCoord 로 좌표 전달
//   4. 프래그먼트 셰이더 : in vec2 TexCoord, uniform sampler2D, texture(샘플러, 좌표) 로 색 읽기
//   5. (꺼둠) texture(...) * vec4(ourColor, 1.0) : 텍스처 색 × 정점 색 = "디스코" 상자
//   6. 렌더 루프 안 glBindTexture : 그리기 직전에 텍스처를 꽂으면 셰이더 샘플러가 그걸 읽는다
// ----------------------------------------------------------------------------
// 필요한 것 : glad, GLFW, stb_image.h(같은 폴더), container.jpg(실행 폴더)
// ============================================================================

#include <glad/glad.h>   // OpenGL 함수 주소를 불러오는 GLAD (GLFW보다 먼저 포함해야 함)
#include <GLFW/glfw3.h>  // 창 만들기와 키 입력 처리를 맡는 GLFW

#define STB_IMAGE_IMPLEMENTATION  // 이 파일에서 stb_image 함수들의 실제 몸통까지 만들라는 표시
#include "stb_image.h"            // 이미지 로딩 라이브러리 헤더 (위 정의 덕분에 구현도 포함됨)

#include <iostream>      // 콘솔에 글자를 찍는 std::cout 사용

void framebuffer_size_callback(GLFWwindow* window, int width, int height);  // 창 크기가 바뀔 때 불릴 함수 선언
void processInput(GLFWwindow* window);                                       // 매 프레임 키 입력을 확인할 함수 선언
unsigned int createShaderProgram(const char* vsSource, const char* fsSource); // 셰이더 프로그램을 만드는 도우미 함수 선언

const unsigned int SCR_WIDTH = 800;   // 창 가로 크기 (픽셀)
const unsigned int SCR_HEIGHT = 600;  // 창 세로 크기 (픽셀)

// 정점 셰이더 소스
const char* vertexShaderSource =
"#version 330 core\n"                        // GLSL 3.3 코어 버전으로 작성한다
"layout (location = 0) in vec3 aPos;\n"      // 0번 입력 칸: 정점 위치 (float 3개짜리 vec3)
"layout (location = 1) in vec3 aColor;\n"    // 1번 입력 칸: 정점 색 (float 3개짜리 vec3)
// ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
// ★ [새 내용] 텍스처 좌표를 받아서 프래그먼트 셰이더로 넘기기
// ★ - 텍스처 좌표: 이 꼭짓점이 텍스처 그림의 "어느 지점"에 해당하는지. (0,0)=왼쪽 아래, (1,1)=오른쪽 위.
// ★ - 정점 셰이더는 받아서 그대로 넘기기만 한다.
// ★   꼭짓점 사이 픽셀들의 좌표는 OpenGL이 자동으로 보간(중간값 계산)해 준다.
// ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
"layout (location = 2) in vec2 aTexCoord;\n" // ★ 2번 입력 칸: 텍스처 좌표 (float 2개짜리 vec2)
"out vec3 ourColor;\n"                       // 프래그먼트 셰이더로 넘겨줄 색
"out vec2 TexCoord;\n"                       // ★ 프래그먼트 셰이더로 넘겨줄 텍스처 좌표
"void main()\n"                              // 정점 셰이더의 시작 함수
"{\n"                                        // main 몸통 시작
"    gl_Position = vec4(aPos, 1.0);\n"       // 위치에 w=1을 붙여 그대로 화면 좌표로 내보낸다
"    ourColor = aColor;\n"                   // 입력받은 색을 그대로 다음 단계로 넘긴다
"    TexCoord = aTexCoord;\n"                // ★ 입력받은 텍스처 좌표를 그대로 다음 단계로 넘긴다
"}\0";                                       // main 몸통 끝 + 문자열 끝 표시

// 프래그먼트 셰이더 소스
const char* fragmentShaderSource =
"#version 330 core\n"                        // GLSL 3.3 코어 버전으로 작성한다
"out vec4 FragColor;\n"                      // 이 픽셀의 최종 색을 내보낼 출력 변수
"in vec3 ourColor;\n"                        // 정점 셰이더에서 보간되어 들어온 색 (디스코 줄을 켤 때 쓰임)
"in vec2 TexCoord;\n"                        // ★ 정점 셰이더에서 보간되어 들어온 텍스처 좌표
// ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
// ★ [새 내용] 샘플러(sampler2D)와 texture() 함수
// ★ - sampler2D : 셰이더 안에서 "2D 텍스처"를 가리키는 자료형. (1D면 sampler1D, 3D면 sampler3D)
// ★   uniform으로 선언해 두면, 프로그램 쪽에서 꽂아 둔 텍스처를 읽을 수 있다.
// ★ - texture(샘플러, 좌표) : 그 좌표의 색을 꺼내 온다(=샘플링).
// ★   덩어리 1에서 정한 감싸기·필터링 설정이 여기서 적용된다.
// ★ - 이 샘플러에 값을 따로 넣지 않으면 0번 텍스처 유닛을 가리킨다.
// ★   (일부 드라이버는 기본값을 안 줘서 검은 화면이 될 수 있음 → 덩어리 3에서 해결)
// ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
"uniform sampler2D texture1;\n"              // ★ 2D 텍스처를 읽어 올 샘플러 (이름: texture1)
"void main()\n"                              // 프래그먼트 셰이더의 시작 함수
"{\n"                                        // main 몸통 시작
"    FragColor = texture(texture1, TexCoord);\n" // ★ 텍스처에서 이 픽셀 좌표의 색을 읽어 그대로 출력
// ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
// ★ [새 내용 · 꺼둠] "디스코" 상자
// ★ - 텍스처 색에 정점 색을 곱하면 두 색이 섞인다. 위 줄 대신 아래 줄을 켜 보면 된다.
// ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
// "    FragColor = texture(texture1, TexCoord) * vec4(ourColor, 1.0);\n" // ★ (꺼둠) 텍스처 색 × 정점 색 → 알록달록 섞인 상자
"}\0";                                       // main 몸통 끝 + 문자열 끝 표시

int main()                                                                   // 프로그램 시작점
{                                                                            // main 몸통 시작
    glfwInit();                                                              // GLFW 라이브러리 초기화
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);                           // OpenGL 주 버전을 3으로 요청
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);                           // OpenGL 부 버전을 3으로 요청 (합쳐서 3.3)
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);           // 옛날 기능을 뺀 코어 프로파일 사용
#ifdef __APPLE__                                                             // macOS에서 컴파일할 때만
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);                     // 맥에서 필요한 호환 설정 켜기
#endif                                                                       // macOS 전용 구간 끝

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL); // 800x600 창 만들기
    if (window == NULL)                                                      // 창 만들기에 실패했다면
    {                                                                        // 실패 처리 시작
        std::cout << "Failed to create GLFW window" << std::endl;            // 실패 메시지 출력
        glfwTerminate();                                                     // GLFW 정리
        return -1;                                                           // 오류로 프로그램 종료
    }                                                                        // 실패 처리 끝
    glfwMakeContextCurrent(window);                                          // 이 창의 OpenGL 컨텍스트를 지금부터 사용
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);       // 창 크기가 바뀌면 위 콜백이 불리도록 등록

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))                 // GLAD로 OpenGL 함수 주소들을 불러오고, 실패했다면
    {                                                                        // 실패 처리 시작
        std::cout << "Failed to initialize GLAD" << std::endl;               // 실패 메시지 출력
        return -1;                                                           // 오류로 프로그램 종료
    }                                                                        // 실패 처리 끝

    unsigned int shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource); // 두 셰이더를 컴파일·링크해 프로그램 하나로 만든다 (메모: 이 링크 때 sampler2D 유니폼과 2번 입력 칸이 프로그램에 들어간다)

    // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
    // ★ [새 내용] 꼭짓점마다 텍스처 좌표 붙이기
    // ★ - 꼭짓점 하나 = 위치 3 + 색 3 + 텍스처 좌표 2 = float 8개.
    // ★ - 사각형의 네 모서리를 텍스처 그림의 네 모서리에 맞춘다.
    // ★   오른쪽 위 ↔ (1,1), 오른쪽 아래 ↔ (1,0), 왼쪽 아래 ↔ (0,0), 왼쪽 위 ↔ (0,1)
    // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
    float vertices[] = {                                                     // 사각형 꼭짓점 4개의 데이터 (한 꼭짓점 = 위치 3 + 색 3 + 텍스처 좌표 2)
        // 위치(x, y, z)       // 색(r, g, b)        // 텍스처 좌표(s, t)     // 열 제목
         0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,                // ★ 오른쪽 위 꼭짓점, 빨강, 텍스처의 오른쪽 위(1,1)
         0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,                // ★ 오른쪽 아래 꼭짓점, 초록, 텍스처의 오른쪽 아래(1,0)
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,                // ★ 왼쪽 아래 꼭짓점, 파랑, 텍스처의 왼쪽 아래(0,0)
        -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f                 // ★ 왼쪽 위 꼭짓점, 노랑, 텍스처의 왼쪽 위(0,1)
    };                                                                       // 정점 배열 끝
    unsigned int indices[] = {                                               // 사각형을 삼각형 2개로 나누는 꼭짓점 번호
        0, 1, 3,                                                             // 첫 번째 삼각형: 오른쪽 위, 오른쪽 아래, 왼쪽 위
        1, 2, 3                                                              // 두 번째 삼각형: 오른쪽 아래, 왼쪽 아래, 왼쪽 위
    };                                                                       // 인덱스 배열 끝

    unsigned int VBO, VAO, EBO;                                              // 정점 버퍼, 정점 배열 객체, 인덱스 버퍼의 ID를 담을 변수
    glGenVertexArrays(1, &VAO);                                              // VAO 하나 만들기
    glGenBuffers(1, &VBO);                                                   // VBO 하나 만들기
    glGenBuffers(1, &EBO);                                                   // EBO 하나 만들기

    glBindVertexArray(VAO);                                                  // 지금부터의 정점 설정을 이 VAO에 기록

    glBindBuffer(GL_ARRAY_BUFFER, VBO);                                      // VBO를 정점 버퍼 자리에 바인딩
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // 정점 데이터를 GPU 메모리로 복사

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);                              // EBO를 인덱스 버퍼 자리에 바인딩
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); // 인덱스 데이터를 GPU 메모리로 복사

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); // 0번 칸(위치): float 3개, 꼭짓점 하나가 float 8개 간격, 맨 앞에서 시작 (메모: 텍스처 좌표가 붙어 간격 6→8)
    glEnableVertexAttribArray(0);                                            // 0번 칸 사용 켜기
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); // 1번 칸(색): float 3개, 같은 간격, float 3개 뒤에서 시작 (메모: 간격 6→8)
    glEnableVertexAttribArray(1);                                            // 1번 칸 사용 켜기
    // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
    // ★ [새 내용] 텍스처 좌표용 2번 입력 칸 연결
    // ★ - 셰이더의 layout(location = 2)과 번호를 맞춘다.
    // ★ - float 2개, 꼭짓점 간격 8개, 위치 3 + 색 3 = float 6개 뒤에서 시작.
    // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); // ★ 2번 칸(텍스처 좌표): float 2개, 간격 8개, float 6개 뒤에서 시작
    glEnableVertexAttribArray(2);                                            // ★ 2번 칸 사용 켜기

    unsigned int texture;                                                    // 텍스처 객체의 ID를 담을 변수
    glGenTextures(1, &texture);                                              // 텍스처 객체 1개를 만들고 그 ID를 texture에 넣는다
    glBindTexture(GL_TEXTURE_2D, texture);                                   // 이 텍스처를 2D 텍스처 자리에 꽂는다 (이후 설정 대상)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);            // 가로(S) 방향: 범위 밖이면 반복
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);            // 세로(T) 방향: 범위 밖이면 반복
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // 축소될 때: 밉맵 두 단계를 섞고, 각 단계 안에서도 부드럽게 섞기
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);        // 확대될 때: 주변 텍셀을 부드럽게 섞기
    int width, height, nrChannels;                                           // 이미지의 가로, 세로, 색 채널 수를 받을 변수
    unsigned char* data = stbi_load("container.jpg", &width, &height, &nrChannels, 0); // container.jpg를 읽어 픽셀 바이트 배열로 받는다
    if (data)                                                                // 이미지를 제대로 읽었다면
    {                                                                        // 성공 처리 시작
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data); // 픽셀 데이터를 현재 텍스처의 0단계로 GPU에 올린다
        glGenerateMipmap(GL_TEXTURE_2D);                                     // 현재 텍스처의 밉맵 단계들을 자동 생성
    }                                                                        // 성공 처리 끝
    else                                                                     // 이미지를 못 읽었다면 (경로 오류 등)
    {                                                                        // 실패 처리 시작
        std::cout << "Failed to load texture" << std::endl;                  // 실패 메시지 출력
    }                                                                        // 실패 처리 끝
    stbi_image_free(data);                                                   // stbi_load가 만든 픽셀 배열 메모리를 돌려준다

    while (!glfwWindowShouldClose(window))                                   // 창을 닫으라는 신호가 올 때까지 반복 (렌더 루프)
    {                                                                        // 렌더 루프 몸통 시작
        processInput(window);                                                // 키 입력 확인 (ESC면 닫기)

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);                                // 화면을 지울 색: 청록빛 회색
        glClear(GL_COLOR_BUFFER_BIT);                                        // 그 색으로 화면 색 버퍼를 지운다

        // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
        // ★ [새 내용] 그리기 직전에 텍스처 꽂기
        // ★ - glBindTexture 함수 자체는 덩어리 1에서 나왔지만, 여기서는 의미가 다르다.
        // ★   "설정하려고" 꽂는 게 아니라 "셰이더의 sampler2D가 읽게 하려고" 꽂는다.
        // ★ - 그리기 전에 꽂아 두면 셰이더 샘플러에 자동으로 연결된다(기본 0번 유닛).
        // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
        glBindTexture(GL_TEXTURE_2D, texture);                               // ★ 이번 그리기에서 셰이더가 읽을 텍스처를 꽂는다

        glUseProgram(shaderProgram);                                         // 이 셰이더 프로그램으로 그리겠다고 선택
        glBindVertexArray(VAO);                                              // 사각형 설정이 담긴 VAO 바인딩
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);                 // 인덱스 6개(삼각형 2개)로 사각형 그리기

        glfwSwapBuffers(window);                                             // 다 그린 뒷 버퍼를 화면에 보여준다
        glfwPollEvents();                                                    // 키보드·마우스·창 이벤트 처리
    }                                                                        // 렌더 루프 몸통 끝

    glDeleteVertexArrays(1, &VAO);                                           // VAO 삭제
    glDeleteBuffers(1, &VBO);                                                // VBO 삭제
    glDeleteBuffers(1, &EBO);                                                // EBO 삭제
    glDeleteProgram(shaderProgram);                                          // 셰이더 프로그램 삭제

    glfwTerminate();                                                         // GLFW가 쓰던 자원 모두 정리
    return 0;                                                                // 정상 종료
}                                                                            // main 몸통 끝

unsigned int createShaderProgram(const char* vsSource, const char* fsSource) // 정점·프래그먼트 소스로 셰이더 프로그램을 만드는 함수
{                                                                            // 함수 몸통 시작
    int success;                                                             // 컴파일·링크 성공 여부를 받을 변수
    char infoLog[512];                                                       // 오류 메시지를 받을 글자 버퍼

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);            // 정점 셰이더 객체 만들기
    glShaderSource(vertexShader, 1, &vsSource, NULL);                        // 정점 셰이더에 소스 문자열 연결
    glCompileShader(vertexShader);                                           // 정점 셰이더 컴파일
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);                // 컴파일이 성공했는지 물어본다
    if (!success)                                                            // 실패했다면
    {                                                                        // 실패 처리 시작
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);                // 오류 메시지 받아오기
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl; // 오류 메시지 출력
    }                                                                        // 실패 처리 끝

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);        // 프래그먼트 셰이더 객체 만들기
    glShaderSource(fragmentShader, 1, &fsSource, NULL);                      // 프래그먼트 셰이더에 소스 문자열 연결
    glCompileShader(fragmentShader);                                         // 프래그먼트 셰이더 컴파일
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);              // 컴파일이 성공했는지 물어본다
    if (!success)                                                            // 실패했다면
    {                                                                        // 실패 처리 시작
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);              // 오류 메시지 받아오기
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl; // 오류 메시지 출력
    }                                                                        // 실패 처리 끝

    unsigned int program = glCreateProgram();                                // 셰이더 프로그램 객체 만들기
    glAttachShader(program, vertexShader);                                   // 프로그램에 정점 셰이더 붙이기
    glAttachShader(program, fragmentShader);                                 // 프로그램에 프래그먼트 셰이더 붙이기
    glLinkProgram(program);                                                  // 붙인 셰이더들을 하나의 프로그램으로 링크 (메모: 새 sampler2D 유니폼이 프로그램에 자리 잡는 시점)
    glGetProgramiv(program, GL_LINK_STATUS, &success);                       // 링크가 성공했는지 물어본다
    if (!success)                                                            // 실패했다면
    {                                                                        // 실패 처리 시작
        glGetProgramInfoLog(program, 512, NULL, infoLog);                    // 오류 메시지 받아오기
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl; // 오류 메시지 출력
    }                                                                        // 실패 처리 끝

    glDeleteShader(vertexShader);                                            // 링크가 끝났으니 정점 셰이더 객체는 삭제
    glDeleteShader(fragmentShader);                                          // 링크가 끝났으니 프래그먼트 셰이더 객체도 삭제
    return program;                                                          // 완성된 프로그램 ID를 돌려준다
}                                                                            // 함수 몸통 끝

void processInput(GLFWwindow* window)                                        // 키 입력을 확인하는 함수
{                                                                            // 함수 몸통 시작
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)                   // ESC 키가 눌려 있다면
        glfwSetWindowShouldClose(window, true);                              // 창을 닫으라는 신호를 켠다
}                                                                            // 함수 몸통 끝

void framebuffer_size_callback(GLFWwindow* window, int width, int height)    // 창 크기가 바뀌면 GLFW가 부르는 함수
{                                                                            // 함수 몸통 시작
    glViewport(0, 0, width, height);                                         // 그리는 영역을 새 창 크기에 맞춘다
}                                                                            // 함수 몸통 끝

// ============================================================================
// 확인 필요 목록
//   - 0번·1번 glVertexAttribPointer의 간격(stride) 6→8 변경 : 줄 자체는 앞 장 코드라 ★ 없이 메모만 달았다.
//   - vec4(ourColor, 1.0) 같은 벡터 생성 문법 : Shaders 장에서 나온 것으로 보고 디스코 줄의 ★는
//     "텍스처 색 × 정점 색"이라는 이 장의 새 의미 때문에만 달았다.
//   - 이 파일은 샘플러에 유닛 번호를 직접 넣지 않는다(본문 그대로).
//     화면이 검게 나오는 드라이버라면 덩어리 3의 glUniform1i 방식을 쓰면 된다.
// ============================================================================