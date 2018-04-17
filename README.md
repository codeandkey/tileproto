### tileproto
#### a proof-of-concept high performance tile rendering engine
tileproto uses an experimental method to render massive tiled worlds at ridiculous speeds.
The primary concept used is dynamic compilation of tiled worlds into large chunk textures which can be rendered layer-by-layer in a single pass.
This dramatically reduces draw calls while increasing RAM usage by L*N^2*T bytes per active chunk, where L is the layer count, N is the chunk width, and T is the tile texture size in bytes.
Also, as it is costly to store compiled chunks on disk they are compiled dynamically as required. This is expensive and preferably performed in another thread.

#### usage
Use the arrow keys to move the camera around. Chunks are generated randomly as you move towards them.
