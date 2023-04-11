#version 150

uniform ivec2 u_Dimensions;
uniform int u_Time;
uniform int u_BlckType;

in vec2 fs_UV;

out vec4 color;

uniform sampler2D u_Texture;

vec2 random2(vec2 p) {
    return fract(sin(vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3))))* 43758.54);
}

float surflet(vec2 P, vec2 gridPoint) {
    // Compute falloff function by converting linear distance to a polynomial (quintic smootherstep function)
    float distX = abs(P.x - gridPoint.x);
    float distY = abs(P.y - gridPoint.y);
    float tX = 1.0 - 6.0 * pow(distX, 5.0) + 15.0 * pow(distX, 4.0) - 10.0 * pow(distX, 3.0);
    float tY = 1.0 - 6.0 * pow(distY, 5.0) + 15.0 * pow(distY, 4.0) - 10.0 * pow(distY, 3.0);

    // Get the random vector for the grid point
    vec2 gradient = random2(gridPoint);
    // Get the vector from the grid point to P
    vec2 diff = P - gridPoint;
    // Get the value of our height field by dotting grid->P with our gradient
    float height = dot(diff, gradient);
    // Scale our height field (i.e. reduce it) by our polynomial falloff function
    return height * tX * tY;
}

float PerlinNoise(vec2 uv) {
    // Tile the space
    vec2 uvXLYL = floor(uv);
    vec2 uvXHYL = uvXLYL + vec2(1,0);
    vec2 uvXHYH = uvXLYL + vec2(1,1);
    vec2 uvXLYH = uvXLYL + vec2(0,1);

    return surflet(uv, uvXLYL) + surflet(uv, uvXHYL) + surflet(uv, uvXHYH) + surflet(uv, uvXLYH);
}

void main()
{
    // TODO Homework 5
    float perlinNoise = PerlinNoise(vec2(12.f * fs_UV.x + cos(u_Time * 0.2f), 10.f * fs_UV.y - sin(u_Time * 0.5f)));
    vec2 uv = perlinNoise/35.f + fs_UV;
    vec3 col = texture(u_Texture, uv).rgb;
    vec3 tinge =  vec3(0.2f);
    if (u_BlckType == 4) {
        tinge.b += 0.6f;
    } else if (u_BlckType == 7){
        tinge.r += 0.6f;
    } else {
        tinge = vec3(1.0f);
    }
    color = vec4(normalize(col + tinge),1);
}
