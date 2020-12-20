#version 450
#extension GL_ARB_separate_shader_objects : enable

// Need to specify the index of the framebuffer to communicate with the fragment shader.
layout(location = 0) out vec3 fragColor;

// For the tutorial, hard-code the positions so we don't need to create a vertex buffer (yet).
vec2 positions[3] = vec2[](
	vec2(0.0, -0.5), // Top vertex (v0)	
	vec2(0.5, 0.5),  // Bottom right vertex (v1)
	vec2(-0.5, 0.5)  // Bottom left vertex  (v2)
);

// Give a color for each position
vec3 colors[3] = vec3[](
	vec3(1.0, 0.0, 0.0),  // v0 is red 
	vec3(0.0, 1.0, 0.0), //  v1 is green 
	vec3(0.0, 0.0, 1.0) //   v2 is blue
);

/* Invoked for every vertex. The built-in gl_VertexIndex variable contains the index of the current vertex,
   This is usually an index into the vertex buffer, but in this example it is an index into the hardcoded array
   of vertex data. 
*/
void main() {
	// The position of each vertex is accessed from the hardcoded array and combined with dummy z & w components to produce a position in clip coords.
	gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
	fragColor = colors[gl_VertexIndex];
}