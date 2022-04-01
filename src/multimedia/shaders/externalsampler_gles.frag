#extension GL_OES_EGL_image_external : require
precision highp float;
precision highp int;

struct buf
{
    mat4 matrix;
    mat4 colorMatrix;
    float opacity;
    float width;
    float masteringWhite;
    float maxLum;
};

uniform buf ubuf;

uniform samplerExternalOES plane1Texture;

varying vec2 texCoord;

void main()
{
    gl_FragColor = texture2D(plane1Texture, texCoord).rgba * ubuf.opacity;

#ifdef QMM_OUTPUTSURFACE_LINEAR
    fragColor = pow(fragColor, 2.2);
#endif
}
