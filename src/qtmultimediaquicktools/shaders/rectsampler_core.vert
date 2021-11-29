#version 150 core
uniform highp mat4 qt_Matrix;
uniform highp vec2 qt_videoSize;
in highp vec4 qt_VertexPosition;
in highp vec2 qt_VertexTexCoord;
out highp vec2 qt_TexCoord;

void main() {
    qt_TexCoord = vec2(qt_VertexTexCoord.x * qt_videoSize.x, qt_VertexTexCoord.y * qt_videoSize.y);
    gl_Position = qt_Matrix * qt_VertexPosition;
}
