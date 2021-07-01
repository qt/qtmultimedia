#extension GL_OES_EGL_image_external : require
precision highp float;
precision highp int;

struct buf
{
    mat4 matrix;
    mat4 colorMatrix;
    float opacity;
    float width;
};

uniform buf ubuf;

uniform samplerExternalOES plane1Texture;

varying vec2 texCoord;

void main()
{
    gl_FragColor = texture2D(plane1Texture, texCoord).rgba * ubuf.opacity;
}
