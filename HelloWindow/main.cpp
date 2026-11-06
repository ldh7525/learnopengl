// =====================================================================
// 덩어리 1 : 텍스처 준비하기 (본문: Texture Wrapping ~ Generating a texture)
// 완성되는 것: 이미지 파일 container.jpg가 GPU 안의 "텍스처 객체"로 올라간다.
//   화면에는 아직 앞 장의 색깔 사각형만 보인다. (텍스처를 셰이더에 연결하는 건 덩어리 2)
//   콘솔에 "Failed to load texture"가 안 뜨면 성공.
//
// [이 덩어리의 새 내용]  (★ 표시된 줄)
//   1. #include "stb_image.h"            : 이미지 로딩 라이브러리 사용 (구현은 stb_image.cpp)
//   2. glGenTextures / glBindTexture      : 텍스처 객체 만들기·묶기
//   3. glTexParameteri (WRAP_S, WRAP_T)   : 텍스처 감싸기(wrapping) 방식 설정
//   4. glTexParameteri (MIN/MAG_FILTER)   : 텍스처 필터링 + 밉맵 필터링 방식 설정
//   5. stbi_load                          : 이미지 파일 → 픽셀 바이트 배열 + 가로/세로/채널 수
//   6. glTexImage2D                       : 픽셀 배열을 텍스처 객체에 올리기
//   7. glGenerateMipmap                   : 밉맵 자동 생성
//   8. stbi_image_free                    : 다 쓴 이미지 메모리 해제
//
// 빌드 예: g++ main.cpp stb_image.cpp glad.c -lglfw -ldl -o chunk1   (환경에 따라 다름)
// 실행 위치(작업 폴더)에 shader.vs, shader.fs, container.jpg 가 있어야 함
// =====================================================================
#include <glad/glad.h>                     // OpenGL 함수들을 쓸 수 있게 해 주는 glad 헤더 (GLFW보다 먼저 포함)
#include <GLFW/glfw3.h>                    // 창 만들기·입력 처리를 해 주는 GLFW 헤더
#include "stb_image.h"                     // ★ 이미지 로딩 함수(stbi_load 등)의 이름을 가져옴 (몸통은 stb_image.cpp에 있음)

#include "shader_s.h"                      // 앞 장에서 만든 셰이더 클래스

#include <iostream>                        // 콘솔 출력(std::cout)을 위한 헤더

void framebuffer_size_callback(GLFWwindow* window, int width, int height);  // 창 크기가 바뀔 때 부를 함수를 미리 알려 둠
void processInput(GLFWwindow* window);     // 키 입력을 처리할 함수를 미리 알려 둠

const unsigned int SCR_WIDTH = 800;        // 창 가로 크기 (픽셀)
const unsigned int SCR_HEIGHT = 600;       // 창 세로 크기 (픽셀)

int main()                                 // 프로그램 시작점
{                                          // main 몸통 시작
    glfwInit();                            // GLFW 라이브러리 초기화
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);   // OpenGL 주 버전 3을 쓰겠다고 알림
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);   // OpenGL 부 버전 3을 쓰겠다고 알림 (합쳐서 3.3)
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 옛날 기능을 뺀 코어 프로필 사용

#ifdef __APPLE__                           // 맥(macOS)에서 빌드할 때만
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);  // 맥에서 필요한 호환 설정 켜기
#endif                                     // 맥 전용 구역 끝

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);  // 창 만들기
    if (window == NULL)                    // 창 만들기에 실패했다면
    {                                      // 실패 처리 시작
        std::cout << "Failed to create GLFW window" << std::endl;  // 실패했다고 출력
        glfwTerminate();                   // GLFW 정리
        return -1;                         // 프로그램을 오류로 끝냄
    }                                      // 실패 처리 끝
    glfwMakeContextCurrent(window);        // 이 창의 OpenGL 컨텍스트를 지금 쓸 컨텍스트로 정함
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);  // 창 크기가 바뀌면 이 함수를 부르도록 등록

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))  // glad로 OpenGL 함수 주소를 불러옴, 실패하면
    {                                      // 실패 처리 시작
        std::cout << "Failed to initialize GLAD" << std::endl;  // 실패했다고 출력
        return -1;                         // 프로그램을 오류로 끝냄
    }                                      // 실패 처리 끝

    Shader ourShader("shader.vs", "shader.fs");  // 셰이더 파일 두 개를 읽어 컴파일·링크 (앞 장 셰이더 그대로)

    float vertices[] = {                   // 사각형 꼭짓점 4개의 데이터 (한 줄 = 꼭짓점 하나)
        // 위치(x, y, z)        // 색(r, g, b)
         0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,  // 오른쪽 위: 빨강
         0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,  // 오른쪽 아래: 초록
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,  // 왼쪽 아래: 파랑
        -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f   // 왼쪽 위: 노랑
    };                                     // 꼭짓점 데이터 끝
    unsigned int indices[] = {             // 꼭짓점 4개를 어떤 순서로 이어 삼각형 2개를 만들지 (EBO용)
        0, 1, 3,                           // 첫 번째 삼각형: 오른쪽 위, 오른쪽 아래, 왼쪽 위
        1, 2, 3                            // 두 번째 삼각형: 오른쪽 아래, 왼쪽 아래, 왼쪽 위
    };                                     // 인덱스 데이터 끝
    unsigned int VBO, VAO, EBO;            // 정점 버퍼, 정점 배열 객체, 인덱스 버퍼의 번호를 담을 변수
    glGenVertexArrays(1, &VAO);            // VAO 하나 만들기
    glGenBuffers(1, &VBO);                 // VBO 하나 만들기
    glGenBuffers(1, &EBO);                 // EBO 하나 만들기

    glBindVertexArray(VAO);                // 지금부터의 정점 설정을 이 VAO에 기록

    glBindBuffer(GL_ARRAY_BUFFER, VBO);    // VBO를 정점 데이터용 버퍼로 묶기
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);  // 꼭짓점 데이터를 GPU 버퍼로 복사

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);  // EBO를 인덱스 데이터용 버퍼로 묶기
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);  // 인덱스 데이터를 GPU 버퍼로 복사

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);  // 0번 속성(위치): float 3개, 꼭짓점 하나가 float 6개 크기, 맨 앞부터 시작
    glEnableVertexAttribArray(0);          // 0번 속성 켜기
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));  // 1번 속성(색): float 3개, 꼭짓점 하나가 float 6개 크기, float 3개 뒤부터 시작
    glEnableVertexAttribArray(1);          // 1번 속성 켜기

    // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
    // ★ [무엇이 새로운가] 텍스처 객체 만들기: glGenTextures, glBindTexture
    // ★ [왜 필요한가] VBO·VAO처럼 텍스처도 GPU 안의 "객체"이고 번호(ID)로 가리킨다.
    // ★   이미지를 올리거나 옵션을 바꾸려면 먼저 객체를 만들고 "지금 작업할 텍스처"로 묶어야 한다.
    // ★ [흐름] glGenTextures(만들 개수, 번호 받을 변수 주소) → texture에 번호가 들어옴
    // ★   → glBindTexture(GL_TEXTURE_2D, texture) → 이후 GL_TEXTURE_2D 대상으로 부르는
    // ★   glTexParameteri / glTexImage2D / glGenerateMipmap 은 전부 이 텍스처에 적용된다.
    // ★ [주의] 함수들의 첫 인자 GL_TEXTURE_2D는 "어떤 종류 자리에 묶인 텍스처냐"(대상, target).
    // ★   2D 이미지를 쓰니까 GL_TEXTURE_2D. (1D, 3D 텍스처도 존재함)
    // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
    unsigned int texture;                  // ★ 텍스처 객체의 번호(ID)를 담을 변수
    glGenTextures(1, &texture);            // ★ 텍스처 객체 1개를 만들고 그 번호를 texture에 받음
    glBindTexture(GL_TEXTURE_2D, texture); // ★ 이 텍스처를 2D 텍스처 자리에 묶음 → 이후 설정이 전부 이 텍스처에 적용됨

    // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
    // ★ [무엇이 새로운가] 텍스처 감싸기(Texture Wrapping) 설정: glTexParameteri(..., GL_TEXTURE_WRAP_S/T, ...)
    // ★ [왜 필요한가] 텍스처 좌표는 보통 0~1 범위다. 이 범위를 벗어난 좌표(예: 1.5)가 오면
    // ★   어떤 색을 줄지 정해야 한다. 옵션 4가지:
    // ★     GL_REPEAT          : 기본값. 이미지를 반복 (좌표의 정수 부분은 버림, 1.5 → 0.5)
    // ★     GL_MIRRORED_REPEAT : 반복하되 한 번 반복할 때마다 거울처럼 뒤집음
    // ★     GL_CLAMP_TO_EDGE   : 좌표를 0~1로 잘라 붙임 → 바깥은 가장자리 픽셀이 쭉 늘어난 무늬
    // ★     GL_CLAMP_TO_BORDER : 범위 밖은 사용자가 정한 테두리 색으로 칠함
    // ★ [흐름] 축마다 따로 설정한다. S축 = 가로(x), T축 = 세로(y). (3D 텍스처면 R축 = z도 있음)
    // ★   glTexParameteri(대상, 어떤 옵션을 어느 축에, 값) → 지금 묶인 텍스처에 저장됨.
    // ★ [직접 해볼 실험] (덩어리 2에서 텍스처 좌표를 0~2로 바꾼 뒤 해야 눈에 보임)
    // ★   GL_REPEAT 자리에 GL_MIRRORED_REPEAT 또는 GL_CLAMP_TO_EDGE를 넣어 보기.
    // ★   GL_CLAMP_TO_BORDER를 쓴다면 테두리 색도 따로 정해야 한다. 이때는 float 배열을 넘기는
    // ★   fv 버전 함수를 쓴다:
    // ★     float borderColor[] = { 1.0f, 1.0f, 0.0f, 1.0f };                  // 테두리 색 (노랑, 불투명)
    // ★     glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor); // 테두리 색 지정
    // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);  // ★ 가로(S축) 방향으로 범위를 벗어나면 이미지를 반복
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);  // ★ 세로(T축) 방향으로 범위를 벗어나면 이미지를 반복

    // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
    // ★ [무엇이 새로운가] 텍스처 필터링 + 밉맵 필터링: GL_TEXTURE_MIN_FILTER, GL_TEXTURE_MAG_FILTER
    // ★ [왜 필요한가] 텍스처 좌표는 해상도와 상관없는 실수라서, 그 좌표가 텍셀(텍스처의 픽셀)
    // ★   정확히 한가운데 떨어지는 일이 거의 없다. 어느 텍셀 색을 줄지 정해야 한다.
    // ★     GL_NEAREST : 중심이 가장 가까운 텍셀 하나의 색을 그대로 씀 → 크게 늘리면 각진 도트 느낌
    // ★     GL_LINEAR  : 주변 텍셀 색을 거리에 따라 섞음 → 크게 늘려도 부드러움
    // ★   MAG_FILTER = 텍스처가 확대될 때(화면에 크게), MIN_FILTER = 축소될 때(화면에 작게) 쓸 방식.
    // ★ [밉맵] 멀리 있어 작게 보이는 물체에 큰 텍스처를 쓰면 픽셀 하나가 텍스처의 넓은 영역을
    // ★   대표해야 해서 지글거리는 잡티가 생기고 메모리 대역폭도 낭비된다. 그래서 원본을 가로·세로
    // ★   절반씩 줄인 이미지들을 미리 만들어 두고(밉맵) 거리에 맞는 크기를 골라 쓴다.
    // ★   축소 필터에는 "밉맵 단계 고르기 방식 + 단계 안 샘플링 방식"을 합친 4가지를 쓸 수 있다:
    // ★     GL_NEAREST_MIPMAP_NEAREST : 가장 맞는 밉맵 하나 고르고, 그 안에서 가장 가까운 텍셀
    // ★     GL_LINEAR_MIPMAP_NEAREST  : 가장 맞는 밉맵 하나 고르고, 그 안에서 선형 보간
    // ★     GL_NEAREST_MIPMAP_LINEAR  : 가장 맞는 밉맵 둘을 섞고, 각 단계는 가장 가까운 텍셀
    // ★     GL_LINEAR_MIPMAP_LINEAR   : 가장 맞는 밉맵 둘을 섞고, 각 단계도 선형 보간 (가장 부드러움)
    // ★ [주의] 확대 필터(MAG)에 밉맵 옵션을 넣는 것은 흔한 실수. 확대할 때는 밉맵을 안 쓰므로
    // ★   효과가 없고 GL_INVALID_ENUM 오류가 난다. 확대 필터는 GL_NEAREST 또는 GL_LINEAR만.
    // ★ [직접 해볼 실험] MAG_FILTER를 GL_NEAREST로 바꾸고 텍스처를 크게 확대해 보면 도트가 보인다.
    // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);  // ★ 축소될 때: 가까운 밉맵 두 단계를 섞고 각 단계 안에서도 선형 보간
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);                // ★ 확대될 때: 주변 텍셀 색을 섞어 부드럽게 (밉맵 옵션 금지)

    // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
    // ★ [무엇이 새로운가] stbi_load로 이미지 파일 읽기
    // ★ [왜 필요한가] glTexImage2D에 넘길 "픽셀 색 바이트 배열"과 이미지 가로·세로 크기가 필요하다.
    // ★ [흐름] stbi_load(파일 경로, &가로, &세로, &채널 수, 0)
    // ★   → 성공하면 픽셀 바이트 배열의 주소를 돌려주고, 세 int 변수에 가로·세로·채널 수를 채워 줌.
    // ★     (container.jpg는 R,G,B 세 채널이라 nrChannels = 3)
    // ★   → 실패하면(파일이 없거나 경로가 틀림) NULL을 돌려줌 → if (data)로 확인.
    // ★   마지막 인자 0 = "채널 수를 바꾸지 말고 파일에 있는 그대로 줘".
    // ★ [주의] 경로는 "실행할 때의 작업 폴더" 기준이다. 실패 메시지가 뜨면 container.jpg 위치부터 확인.
    // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
    int width, height, nrChannels;         // ★ stbi_load가 채워 줄 이미지 가로, 세로, 색 채널 수
    unsigned char* data = stbi_load("container.jpg", &width, &height, &nrChannels, 0);  // ★ 이미지 파일을 읽어 픽셀 바이트 배열로 받음 (실패 시 NULL)
    if (data)                              // ★ 이미지를 제대로 읽었다면
    {                                      // ★ 성공 처리 시작
        // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
        // ★ [무엇이 새로운가] glTexImage2D(픽셀을 텍스처에 올리기) + glGenerateMipmap(밉맵 자동 생성)
        // ★ [glTexImage2D 인자 9개]
        // ★   1) GL_TEXTURE_2D    : 지금 2D 자리에 묶인 텍스처에 올림 (1D·3D 자리의 텍스처는 영향 없음)
        // ★   2) 0                : 밉맵 단계. 0 = 원본(기본) 단계. 단계별로 직접 올릴 때만 바꿈
        // ★   3) GL_RGB           : GPU 안에 "어떤 형식으로 저장할지". 이미지가 RGB라 RGB로 저장
        // ★   4) width, 5) height : 만들 텍스처의 가로·세로 (stbi_load가 알려 준 값)
        // ★   6) 0                : 항상 0 (옛날 기능의 흔적)
        // ★   7) GL_RGB           : "넘겨주는 원본 데이터"의 색 형식
        // ★   8) GL_UNSIGNED_BYTE : 원본 데이터 한 칸의 자료형 (stbi_load는 바이트 = unsigned char로 줌)
        // ★   9) data             : 실제 픽셀 바이트 배열
        // ★ [흐름] 이 호출 뒤 텍스처에는 0단계(원본)만 들어 있다. 위에서 MIN_FILTER를 밉맵 방식으로
        // ★   정했으니 나머지 단계가 필요하다 → glGenerateMipmap이 절반, 또 절반... 을 자동으로 만든다.
        // ★ [주의] 밉맵 필터를 정해 놓고 glGenerateMipmap을 빼먹으면 텍스처가 "미완성"으로 취급되어
        // ★   검게 나올 수 있다.
        // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);  // ★ 픽셀 배열을 지금 묶인 텍스처의 0단계(원본)로 올림
        glGenerateMipmap(GL_TEXTURE_2D);   // ★ 지금 묶인 텍스처의 밉맵(점점 절반 크기 이미지들)을 자동으로 만듦
    }                                      // ★ 성공 처리 끝
    else                                   // ★ 이미지를 못 읽었다면
    {                                      // ★ 실패 처리 시작
        std::cout << "Failed to load texture" << std::endl;  // ★ 텍스처 이미지 읽기 실패를 콘솔에 알림
    }                                      // ★ 실패 처리 끝
    // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
    // ★ [무엇이 새로운가] stbi_image_free : stbi_load가 잡은 메모리 돌려주기
    // ★ [왜 필요한가] glTexImage2D가 픽셀을 GPU 쪽으로 이미 복사했으므로, CPU 쪽 원본 배열은
    // ★   더 이상 필요 없다. 안 지우면 메모리가 새어 나간다.
    // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
    stbi_image_free(data);                 // ★ CPU 메모리에 있던 이미지 바이트 배열 해제

    while (!glfwWindowShouldClose(window)) // 창을 닫으라는 요청이 올 때까지 반복 (렌더 루프)
    {                                      // 렌더 루프 몸통 시작
        processInput(window);              // 키 입력 처리 (ESC면 창 닫기)

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);  // 화면을 지울 때 쓸 배경색 (짙은 청록)
        glClear(GL_COLOR_BUFFER_BIT);      // 화면(색 버퍼)을 배경색으로 지움

        ourShader.use();                   // 셰이더 프로그램 사용 시작
        glBindVertexArray(VAO);            // 사각형 정점 설정이 기록된 VAO 묶기
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);  // 인덱스 6개로 삼각형 2개(사각형) 그리기 — 아직 텍스처는 안 쓰임

        glfwSwapBuffers(window);           // 다 그린 화면을 앞으로 꺼내 보여 줌 (더블 버퍼 교체)
        glfwPollEvents();                  // 키보드·마우스·창 이벤트 확인
    }                                      // 렌더 루프 몸통 끝

    glDeleteVertexArrays(1, &VAO);         // VAO 삭제
    glDeleteBuffers(1, &VBO);              // VBO 삭제
    glDeleteBuffers(1, &EBO);              // EBO 삭제

    glfwTerminate();                       // GLFW가 쓰던 자원 모두 정리
    return 0;                              // 프로그램 정상 종료
}                                          // main 몸통 끝

void processInput(GLFWwindow* window)      // 키 입력 처리 함수
{                                          // 함수 몸통 시작
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)  // ESC 키가 눌려 있다면
        glfwSetWindowShouldClose(window, true);  // 창을 닫으라고 표시 → 렌더 루프가 끝남
}                                          // 함수 몸통 끝

void framebuffer_size_callback(GLFWwindow* window, int width, int height)  // 창 크기가 바뀔 때 GLFW가 불러 주는 함수
{                                          // 함수 몸통 시작
    glViewport(0, 0, width, height);       // 그림 그릴 영역을 새 창 크기에 맞춤
}                                          // 함수 몸통 끝