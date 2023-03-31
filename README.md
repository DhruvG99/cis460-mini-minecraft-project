## Milestone 1
---

### Terrain Rendering and Chunking - Dhruv

### Game Engine Tick Function and Player Physics - Shubh Agarwal

Wrote a custom implementation of the gridMarch algorithm so that it runs faster as I am checking for x, y and z axis separately. This enables me to implement the wall sliding feature. Implemented dynamics with a friction model so due to wind resistance, the player falls from the sky with a terminal(constant) velocity. The player can also run fast if shift is pressed. 

Added a small crosshair in the middle of the screen so that it's easier to visualise which block is the player pointing at. For placing a new block in the environment, I return the interfaceAxis also from the gridMarch function to decide which side of current cube should the new cube be drawn. Removing a cube is straightforward. Find the first cube that intersects the look vector*3 from the player's camera origin.  
