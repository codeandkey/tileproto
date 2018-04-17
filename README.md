![alt text](https://github.com/molecuul/tileproto/raw/master/screen.png "screenshot")
### tileproto
#### a proof-of-concept high performance tile rendering engine
tileproto is a collection of some demos of techniques for rendering and computing with 2D voxel-based worlds.
#### chunk pretexturing demo
The chunk pretexturing demo implements tile rendering by splitting the world into discrete square chunks of tiles. When a chunk needs to be displayed it is "compiled" and rendered into a large static texture. The large texture is then reused to render the entire chunk with one quad instead of rendering tiles independently as their own quads. This allows all chunks to render with the same speed regardless of tile complexity. The downfall and overhead of this method is that when a chunk has to be compiled it injects all of the draws for contained tiles into the current frame, causing a spike in API calls and a stutter in the frame if the GPU is not fast enough.

Variable chunk sizes and culling/threading methods can be tweaked for maximum performance.
#### usage
Each demo has its own controls which are displayed on the screen.
