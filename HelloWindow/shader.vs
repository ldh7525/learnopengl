#version 330 core
layout (location = 0) in vec3 aPos;   // 0번 속성: 정점 위치
layout (location = 1) in vec3 aColor; // 1번 속성: 정점 색
uniform float offset;                          // [추가] CPU가 넣어 주는 가로 이동량 (main 밖, 위쪽에 선언해야 함)

out vec3 ourColor;                    // 프래그먼트 셰이더로 넘길 색

out vec3 pos;

void main()                           // 진입점
{
    gl_Position = vec4(aPos.x, aPos.yz, 1.0); // [변경] x에만 offset을 더해 오른쪽으로 이동 (vec4: float + vec2 + float = 4성분)
    pos= aPos;
    ourColor = aColor;                         // 입력받은 정점 색을 그대로 넘김
}
