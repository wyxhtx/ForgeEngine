#version 450

layout (location = 0) in vec2 inUV;

layout (binding = 0) uniform sampler2D samplerFont;

layout (location = 0) out vec4 outColor;



void main(void) {
    float color = texture(samplerFont, inUV).r;

    if (color < 0.1) {
        discard;
    }
    
	outColor  = vec4(color, color, color, 0.0);
}