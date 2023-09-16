#version 460

layout(location = 0) out vec2 OutTexCoord;

void main()
{
	const vec2 vertexPositions[] = {
		vec2(-1.0, -1.0),
		vec2(1.0, -1.0),
		vec2(1.0, 1.0),
		vec2(-1.0, 1.0)
	};

	const int indices[]=
	{
		0,1,2,2,3,0
	};
	
	const vec2 texCoords[] =
	{
		vec2(0.0, 0.0),
		vec2(1.0, 0.0),
		vec2(1.0, 1.0),
		vec2(0.0, 1.0)
	};

	OutTexCoord = texCoords[indices[gl_VertexIndex]];
	OutTexCoord.y *= -1.0;

	gl_Position = vec4(vertexPositions[indices[gl_VertexIndex]], 0.0, 1.0);
}