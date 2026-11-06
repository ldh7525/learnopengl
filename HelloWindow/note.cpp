//// =====================================================================
////  LearnOpenGL - Shaders 장 / 덩어리 5: Shader 클래스로 main 정리 (이 장 최종 코드, 화면은 덩어리 3과 동일)
////  필요한 파일: main.cpp(이 파일) + shader.h + shader.vs + shader.fs
////
////  ★ 표시는 "이 덩어리에서 새로 나온 것"에만 붙였다.
////   ★ 1) #include "shader.h" : 직접 만든 헤더 포함
////   ★ 2) Shader ourShader("shader.vs", "shader.fs") : 파일 읽기·컴파일·링크가 한 줄로
////   ★ 3) ourShader.use() : glUseProgram 대신 클래스 메서드
////  (정점 데이터, VAO/VBO, 속성 설정은 덩어리 3과 완전히 같아서 ★ 없음. 화면 결과도 같다)
//// =====================================================================
//
//#include <glad/glad.h>                 // OpenGL 함수 로더
//#include <GLFW/glfw3.h>                // 창·입력 라이브러리
//#include "shader.h"                    // ★ 직접 만든 셰이더 클래스 (큰따옴표 = 내 프로젝트 폴더의 파일)
//#include <iostream>                    // 콘솔 출력용
//
//void framebuffer_size_callback(GLFWwindow* window, int width, int height); // 창 크기 변경 콜백 선언
//void processInput(GLFWwindow* window);                                       // 키 입력 처리 함수 선언
//
//const unsigned int SCR_WIDTH = 800;    // 창 너비
//const unsigned int SCR_HEIGHT = 600;   // 창 높이
//
//int main()
//{
//    glfwInit();                                                         // GLFW 초기화
//    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);                      // OpenGL 3.x 요청
//    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);                      // OpenGL x.3 요청
//    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);      // core 프로파일
//#ifdef __APPLE__
//    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);                // macOS 호환
//#endif
//
//    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL); // 창 생성
//    if (window == NULL)                                                 // 실패 시
//    {
//        std::cout << "Failed to create GLFW window" << std::endl;       // 오류 출력
//        glfwTerminate();                                                // 정리
//        return -1;                                                      // 종료
//    }
//    glfwMakeContextCurrent(window);                                     // 컨텍스트 지정
//    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);  // 콜백 등록
//
//    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))            // GLAD 로드 (Shader 생성 전에 필수)
//    {
//        std::cout << "Failed to initialize GLAD" << std::endl;          // 실패 시 출력
//        return -1;                                                      // 종료
//    }
//
//    // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
//    // ★ [새 내용 2] Shader 객체 만들기: 덩어리 3까지의 셰이더 코드 수십 줄이 이 한 줄로
//    // ★
//    // ★ 예전에는 main() 안에 직접 있었던 것:
//    // ★   셰이더 소스 문자열 → glCreateShader → glShaderSource → glCompileShader → 오류 확인
//    // ★   → glCreateProgram → glAttachShader → glLinkProgram → 오류 확인 → glDeleteShader
//    // ★ 이제는 이 한 줄이 shader.h 의 생성자를 호출해서 전부 처리한다.
//    // ★   (셰이더 소스는 shader.vs / shader.fs 파일에서 읽어 온다)
//    // ★
//    // ★ 주의 1: 생성자 안에서 gl 함수를 쓰므로, 반드시 위의 GLAD 로드 "다음"에 만들어야 한다.
//    // ★ 주의 2: 파일 경로는 "프로그램을 실행한 위치" 기준이다. 못 찾으면 FILE_NOT_SUCCESFULLY_READ 가 뜬다.
//    // ★ 직접 해볼 실험: C++ 를 다시 컴파일하지 않고 shader.fs 의 색 계산만 고쳐서 프로그램을 다시 실행해 보기
//    // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
//    Shader ourShader("shader.vs", "shader.fs");                         // ★ 파일 읽기·컴파일·링크를 한 줄로 처리
//
//    float vertices[] = {                                                // 정점당 [위치 3개 | 색 3개]
//        // 위치               // 색
//         0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,                          // 오른쪽 아래 = 빨강
//        -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,                          // 왼쪽 아래 = 초록
//         0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f                           // 위 = 파랑
//    };
//
//    unsigned int VBO, VAO;                                              // 객체 ID
//    glGenVertexArrays(1, &VAO);                                         // VAO 생성
//    glGenBuffers(1, &VBO);                                              // VBO 생성
//    glBindVertexArray(VAO);                                             // VAO 바인딩
//    glBindBuffer(GL_ARRAY_BUFFER, VBO);                                 // VBO 바인딩
//    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // 데이터 복사
//    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0); // 위치 속성 (오프셋 0)
//    glEnableVertexAttribArray(0);                                       // 0번 속성 활성화
//    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); // 색 속성 (오프셋 12바이트)
//    glEnableVertexAttribArray(1);                                       // 1번 속성 활성화
//
//    while (!glfwWindowShouldClose(window))                              // 렌더 루프
//    {
//        processInput(window);                                           // 입력 처리
//        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);                           // 배경색 지정
//        glClear(GL_COLOR_BUFFER_BIT);                                   // 화면 지우기
//
//        // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
//        // ★ [새 내용 3] use() 와 uniform 설정 자리
//        // ★
//        // ★ glUseProgram(shaderProgram) 대신 ourShader.use() 를 쓴다. (안에서 하는 일은 같다)
//        // ★ 이 예제에는 uniform 이 없지만, 있다면 use() 바로 다음에 setter 를 부른다:
//        // ★     ourShader.setFloat("이름", 값);
//        // ★   (use() 로 프로그램을 활성화한 뒤여야 uniform 값이 들어간다는 걸 덩어리 2에서 배움)
//        // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
//        ourShader.use();                                                // ★ glUseProgram 대신 클래스 메서드로 활성화
//        // uniform이 있다면 여기서 ourShader.setFloat("이름", 값); 처럼 설정
//
//        float timeValue = glfwGetTime();                                // 실행 후 경과 시간(초)
//        float greenValue = sin(timeValue) / 2.0f + 0.5f;                // sin(-1~1)을 0~1 범위로 변환
//        int vertexColorLocation = glGetUniformLocation(shaderProgram, "ourColor"); // uniform 위치 조회 (-1이면 못 찾음)
//        glUniform4f(vertexColorLocation, 0.0f, greenValue, 0.0f, 1.0f); // 현재 활성 프로그램의 uniform에 float 4개 설정
//
//
//        glBindVertexArray(VAO);                                         // VAO 바인딩
//        glDrawArrays(GL_TRIANGLES, 0, 3);                               // 삼각형 그리기
//        glfwSwapBuffers(window);                                        // 버퍼 교체
//        glfwPollEvents();                                               // 이벤트 처리
//    }
//
//    glDeleteVertexArrays(1, &VAO);                                      // VAO 해제
//    glDeleteBuffers(1, &VBO);                                           // VBO 해제
//    glDeleteProgram(ourShader.ID);                                      // 클래스가 가진 프로그램 해제 (shaderProgram 변수 대신 ID 멤버를 씀)
//    glfwTerminate();                                                    // GLFW 종료
//    return 0;                                                           // 정상 종료
//}
//
//void processInput(GLFWwindow* window)
//{
//    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)              // ESC 누르면
//        glfwSetWindowShouldClose(window, true);                         // 창 닫기
//}
//
//void framebuffer_size_callback(GLFWwindow* window, int width, int height)
//{
//    glViewport(0, 0, width, height);                                    // 뷰포트 갱신
//}