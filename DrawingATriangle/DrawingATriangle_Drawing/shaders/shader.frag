#version 450
#extension GL_ARB_separate_shader_objects : enable

// Need to specify inputs and outputs (and their index in the framebuffer)

// This input is coming from the vertex shader.
layout(location = 0) in vec3 fragColor;

// You must specify your own output variable for color unlike position for the vertex shader
layout(location = 0) out vec4 outColor;

// Called for every fragment. 
void main() {
	// Colors in GLSL are 4-component (R,G,B,A), all in the [0,1] range. 
	outColor = vec4(fragColor, 1.0);
}