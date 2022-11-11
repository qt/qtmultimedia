uniform sampler2DRect rgbTexture;
uniform lowp float opacity;
varying highp vec2 qt_TexCoord;

void main()
{
    gl_FragColor = texture2DRect(rgbTexture, qt_TexCoord) * opacity;
}
