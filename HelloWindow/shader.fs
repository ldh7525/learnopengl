#version 330 core
out vec4 FragColor;                   // 최종 픽셀 색

in vec3 ourColor;                     // 보간된 색 입력
in vec3 pos;


void main()                           // 진입점
{
    FragColor = vec4(pos, 1.0);  // 알파 1.0 붙여 출력
}
