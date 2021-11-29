#version 150 core
uniform sampler2DRect rgbTexture;
uniform lowp float opacity;
in highp vec2 qt_TexCoord;
out vec4 fragColor;

void main()
{
    fragColor = texture(rgbTexture, qt_TexCoord) * opacity;
}
