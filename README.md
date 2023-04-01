## Milestone 1
---

### Procedural Terrain Generation - Swati Gupta
Used a combination of various noise functions to generate two types of biomes: Mountains and Grassland as well as to interpolate naturally between them.  
- For the Mountain biome, I use a form of fractal noise (this noise builds on top of the concept of fractals, which are shapes that 
repeat themselves at different scales and levels of detail). This is used to create natural-looking scale-invariant repetitive mountan height patterns.  
- For the Grassland biome, I use simple Perlin noise with a smaller amplitude and frequency.  
- For interpolating between these two types of biomes, I use a third, very low frequency Worley Noise function and 
use smoothstep along with LERP to do so. Initially, I was hardcoding the biome type based on (x,z) values and just LERP to interpolate in the middle zone, which gave me very abrupt biome switching results. Doing it using a noise function allowed for a more natural looking biome-to-biome transformation.  

Other Details  
- Mountain Biome is of type STONE. Above a fixed height, the topmost blocks in the mountain biome are set to be SNOW type.  
- Grassland Biome is of type DIRT. The topmost block of grassland biome is always set to Grass.  
- Filling in WATER type blocks upto a fixed water height level whenever the final interpolated height of any of the the biomes falls below this water level.  
- Note that both the biomes start above 128 (the base level). There was also a fixed STONEBED as filler below both the biomes (i.e. heights 0 to 128). But adding this was slowing down render speed quite a bit and as this is not really relevant to the actual terrain features and player navigation, we have commented it out for milestone-1.  

Some References:   
https://www.shadertoy.com/view/3lcGRX  
https://www.shadertoy.com/view/tlKXzh  
https://github.com/dafarry/python-fractal-landscape  

### Terrain Rendering and Chunking - Dhruv Gupta
Chunking and Mesh Generation - 
moved the iteration over the chunk coordinates from the terrain generation to chunk.cpp's createChunkVBOdata. Taking inspiration from Adam's implementation, I created a couple of structures - VertexData and BlockFace - to create a standard resource of each of the 6 faces' coordinates. For a given coordinate/block in the chunk, we iterate over the faces and create its VBO data based on whether the adjacent block is EMPTY or not. To obtain the adjacent block we go travel a unit distance in the face's normal and check if it enters a new chunk as well.
I've also create a colorMapFromBlock which returns the color based on the block type. This removes the need for the multiple if/switch statements while adding colors.

Interleaving - 
To enable interleaving, I created a vector (vboInter) that would store the interleaving data.
Challenges faced: the indexing was slightly difficult at first. I had to go into Shaderprogram and Drawable to figure out how the binding works.

Proximity-Based Rendering -
The chunk generation is done at the very start and then we render the chunks based on the current position - taking 1 chunk before and after in every x,z direction.


### Game Engine Tick Function and Player Physics - Shubh Agarwal

Wrote a custom implementation of the gridMarch algorithm so that it runs faster as I am checking for x, y and z axis separately. This enables me to implement the wall sliding feature. Implemented dynamics with a friction model so due to wind resistance, the player falls from the sky with a terminal(constant) velocity. The player can also run fast if shift is pressed. 

Added a small crosshair in the middle of the screen so that it's easier to visualise which block is the player pointing at. For placing a new block in the environment, I return the interfaceAxis also from the gridMarch function to decide which side of current cube should the new cube be drawn. Removing a cube is straightforward. Find the first cube that intersects the look vector*3 from the player's camera origin.  


