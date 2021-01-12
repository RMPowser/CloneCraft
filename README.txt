CloneCraft is a Minecraft clone that I wrote for my midterm project. I wrote the entire thing 
from scratch over the course of one month. It's written in C++ and uses the Vulkan graphics api
along with a few other quality-of-life libraries that help with things like creating a window 
and terrain generation.

When I started the project, I had zero experience with any graphics api, let alone Vulkan. Long
story short, Vulkan is not a beginners api. It was hard, but I learned a lot.

All of the projects dependancies should be inside the include folder and anything you need to link
to will be in the lib folder.

There are many unfinished aspects about the game such as the world only being one block thick, only
one block type, collision detection does not work in negative coordinate space, there is no frustum
or occulsion culling, and it's even possible for the vertex buffer to be empty when it's passed to
Vulkan causing the game to crash. There are many more problems than these but I just didn't have 
the time to address all of them in a single month.

Features:
	- full 3D graphics with Vulkan
	- KB/M controls
	- .OBJ file loading
	- texture mapping
	- 3D collision detection with AABBs
	- infinite terrain generation
	- ray casting
	- basic mining and building
	- flight mode to turn off gravity

Controls:
	look around		- MOUSE
	mine			- MOUSE LEFT
	place block		- MOUSE RIGHT
	move forward		- W
	strafe left		- A
	move backward		- S
	strafe right		- D
	jump			- SPACE
	speed modifier		- SHIFT
	
	toggle flying		- F
	
	While Flying:
		move up    	- SPACE
		move down  	- CTRL
	
