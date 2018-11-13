#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>

#include "ShaderProgram.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <cmath>

#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
using namespace std;


#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

// 60 FPS (1.0f/60.0f) (update sixty times a second)
#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6

SDL_Window* displayWindow;
//global
ShaderProgram program;
glm::mat4 projectionMatrix;
glm::mat4 viewMatrix;
glm::mat4 modelMatrix;
float lastFrameTicks;
float ticks;
float elapsed;
glm::vec3 friction = {1,1,0};
glm::vec3 gravity = {0,-10, 0};

enum GameMode { STATE_MAIN_MENU, STATE_PLAY, STATE_GAME_OVER };
GameMode mode;


float lerp(float v0, float v1, float t) {
	return (1.0 - t)*v0 + t * v1;
}

GLuint LoadTexture(const char *filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}
	GLuint retTexture;
	glGenTextures(1, &retTexture);
	glBindTexture(GL_TEXTURE_2D, retTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	stbi_image_free(image);
	return retTexture;
}

class SheetSprite {
public:
	SheetSprite(){
		Sprite = LoadTexture(RESOURCE_FOLDER"spritesheet_aliens.png");
	}

	void Draw(ShaderProgram &p, int index) {
		glBindTexture(GL_TEXTURE_2D, Sprite);

		int spriteCountX = 5;
		int spriteCountY = 3;
		float u = (float)(((int)index) % spriteCountX) / (float)spriteCountX;
		float v = (float)(((int)index) / spriteCountX) / (float)spriteCountY;
		float spriteWidth = 1.0 / (float)spriteCountX;
		float spriteHeight = 1.0 / (float)spriteCountY;


		GLfloat texCoords[] = {
		u, v + spriteHeight,
		u + spriteWidth, v + spriteHeight,
		u + spriteWidth, v,
		u, v + spriteHeight,
		u + spriteWidth, v,
		u, v,
		};

		
		// draw our arrays
		glVertexAttribPointer(p.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(p.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(p.positionAttribute);
		glDisableVertexAttribArray(p.texCoordAttribute);
	}
	GLuint Sprite;
	float size;
	float u;
	float v;
};

void DrawText(ShaderProgram &p, std::string text, float size, float spacing) {
	float character_size = 1.0 / 16.0f;
	std::vector<float> vertexData;
	std::vector<float> texCoordData;

	for (int i = 0; i < text.size(); i++) {
		int spriteIndex = (int)text[i];
		float texture_x = (float)(spriteIndex % 16) / 16.0f;
		float texture_y = (float)(spriteIndex / 16) / 16.0f;

		vertexData.insert(vertexData.end(), {
		((size + spacing) * i) + (-0.5f * size), 0.5f * size,
		((size + spacing) * i) + (-0.5f * size), -0.5f * size,
		((size + spacing) * i) + (0.5f * size), 0.5f * size,
		((size + spacing) * i) + (0.5f * size), -0.5f * size,
		((size + spacing) * i) + (0.5f * size), 0.5f * size,
		((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			});
		texCoordData.insert(texCoordData.end(), {
		texture_x, texture_y,
		texture_x, texture_y + character_size,
		texture_x + character_size, texture_y,
		texture_x + character_size, texture_y + character_size,
		texture_x + character_size, texture_y,
		texture_x, texture_y + character_size,
			});
	}

	GLuint fontTexture = LoadTexture(RESOURCE_FOLDER"font.png");
	glBindTexture(GL_TEXTURE_2D, fontTexture);
	// draw this data (use the .data() method of std::vector to get pointer to data)
	// draw this yourself, use text.size() * 6 or vertexData.size()/2 to get number of vertices

	glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(p.positionAttribute);

	glVertexAttribPointer(p.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(p.texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, text.size() * 6);

	glDisableVertexAttribArray(p.positionAttribute);
	glDisableVertexAttribArray(p.texCoordAttribute);
}

class Entity {
public:
	Entity(float x, float y, float z, float width, float height, float depth, float vx, float vy, float vz, float ax, float ay, float az, int id, bool isstatic) {
		position = { x, y, z };
		size = { width, height, depth };
		velocity = { vx, vy, vz };
		acceleration = { ax, ay, az };
		textureID = id;
		isStatic = isstatic;
		collidedTop = false;
		collidedBottom = false;
		collidedLeft = false;
		collidedRight = false;
	};
	void Update(float elapsed) {
		if (isStatic == false) {
			if (collidedTop) {
				if (velocity[1] > 0) {
					velocity[1] *= -1;
				}
			}
			if (collidedBottom) {
				if (velocity[1] < 0) {
					velocity[1] *= -1;
				}
			}
			if (collidedLeft) {
				if (velocity[0] < 0) {
					velocity[0] *= -1;
				}
			}
			if (collidedRight) {
				if (velocity[0] > 0) {
					velocity[0] *= -1;
				}
			}

			position += velocity * elapsed;
		}
	};
	void Render(ShaderProgram &p) {
		float Vertices[] = {
			(position[0] - size[0]), (position[1] - size[1]),
			(position[0] + size[0]), (position[1] - size[1]),
			(position[0] + size[0]), (position[1] + size[1]),
			(position[0] - size[0]), (position[1] - size[1]),
			(position[0] + size[0]), (position[1] + size[1]),
			(position[0] - size[0]), (position[1] + size[1])
		};


		glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, Vertices);
		glEnableVertexAttribArray(p.positionAttribute);

		sprite.Draw(p, textureID);
	};
	bool CollidesWith(Entity &entity) {
		float px;
		float py;

		px = abs(position[0] - entity.position[0]);
		px -= (size[0] + entity.size[0]);
		py = abs(position[1] - entity.position[1]);
		py -= (size[1] + entity.size[1]);
		if (px <= 0 && py <= 0) {
			return true;
		}
		else {
			return false;
		}
	}

	SheetSprite sprite;
	int textureID;

	glm::vec3 position;
	glm::vec3 size;
	glm::vec3 velocity;
	glm::vec3 acceleration;

	bool isStatic;
	bool collidedTop;
	bool collidedBottom;
	bool collidedLeft;
	bool collidedRight;
};

std::vector<Entity> entity;

class Player {
public:
	Player() {
		position = { 1, 1, 0.0 };
		size = { 0.5, 0.5, 0.0 };
		velocity = { 0.0, 0.0, 0.0 };
		acceleration = { 0.0, 0.0, 0.0 };
		textureID = 6;

		collidedTop = false;
		collidedBottom = false;
		collidedLeft = false;
		collidedRight = false;
	}
	void Update(float elapsed) {
		
		velocity[0] = lerp(velocity[0], 0.0f, elapsed * friction[0]);
		velocity[1] = lerp(velocity[1], 0.0f, elapsed * friction[1]);

		velocity += acceleration * elapsed;
		velocity += gravity * elapsed;
		
		MapCollision();
		
		if (collidedTop) {
			if (velocity[1] > 0) {
				velocity[1] = 0;
			}
		}
		if (collidedBottom) {
			if (velocity[1] < 0) {
				velocity[1] = 0;
			}
		}
		if (collidedLeft) {
			if (velocity[0] < 0) {
				velocity[0] = 0;
			}
		}
		if (collidedRight) {
			if (velocity[0] > 0) {
				velocity[0] = 0;
			}
		}

		if (position[1] < 0) {
			//off map
			mode = STATE_GAME_OVER;
		}
		position += velocity * elapsed;

		viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3{ -position[0],-position[1], -position[2]});
		program.SetViewMatrix(viewMatrix);
	}
	void Render(ShaderProgram &p) {
		float Vertices[] = { 
			(position[0] - size[0]), (position[1] - size[1]),
			(position[0] + size[0]), (position[1] - size[1]),
			(position[0] + size[0]), (position[1] + size[1]),
			(position[0] - size[0]), (position[1] - size[1]),
			(position[0] + size[0]), (position[1] + size[1]),
			(position[0] - size[0]), (position[1] + size[1])
		};
		
		
		glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, Vertices);
		glEnableVertexAttribArray(p.positionAttribute);

		sprite.Draw(p, textureID);
	
	}
	bool CollidesWith(Entity &entity) {
		float px;
		float py;
		
		px = abs(position[0] - entity.position[0]);
		px -= (size[0] + entity.size[0]);
		py = abs(position[1] - entity.position[1]);
		py -= (size[1] + entity.size[1]);
		if (px <= 0 && py <=0) {
			return true;
		}
		else {
			return false;
		}
	}
	void MapCollision() {
		//Collide with floor/walls
		collidedTop = false;
		collidedBottom = false;
		collidedLeft = false;
		collidedRight = false;
		for (int i = 0; i < entity.size(); i++) {
			if (CollidesWith(entity[i]) && entity[i].textureID == 3) {
				glm::vec3 pen = abs(position - entity[i].position) - (size + entity[i].size);
				if (pen[1] > pen[0]) {
					if (position[1] < entity[i].position[1]) {
						position[1] += pen[1];
						collidedTop = true;
					}
					else {
						position[1] -= pen[1];
						collidedBottom = true;
					}
				}
				if(pen[1] < pen[0]) {
					if (position[0] > entity[i].position[0]) {
						position[0] -= pen[0];
						collidedLeft = true;
					}
					else {
						position[0] += pen[0];
						collidedRight = true;
					}
				}
			}
		}
	}


	SheetSprite sprite;
	int textureID;

	glm::vec3 position;
	glm::vec3 size;
	glm::vec3 velocity;
	glm::vec3 acceleration;

	bool collidedTop;
	bool collidedBottom;
	bool collidedLeft;
	bool collidedRight;
};


void spawnEntity(glm::vec3 &position, glm::vec3 &size, glm::vec3 &velocity, glm::vec3 &acceleration, int id, bool isStatic, std::vector<Entity> &entity) {
	Entity newEntity(position[0], position[1], position[2], size[0], size[1], size[2], velocity[0], velocity[1], velocity[2], acceleration[0], acceleration[1], acceleration[2], id,isStatic);
	entity.push_back(newEntity);
}


#define LEVEL_HEIGHT 20
#define LEVEL_WIDTH 50
unsigned int levelData[LEVEL_HEIGHT][LEVEL_WIDTH] =
{
//bottom of map
	//3 is floor/walls
	//8 is moving wall
  {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,0,0,0,0,0,0,0,0,0,0,0,0,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3},
  {3,0,0,0,0,0,3,8,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,8,3,0,0,0,0,0,3},
  {3,0,0,0,0,0,3,8,0,0,0,0,0,0,0,0,0,0,3,8,0,0,0,0,0,0,0,0,0,0,8,3,0,0,0,0,0,0,0,0,0,0,8,3,0,0,0,0,0,3},
  {3,0,0,0,0,0,3,8,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,8,3,0,0,0,0,0,3},
  {3,0,0,0,0,0,3,8,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,8,3,0,0,0,0,0,3},
  {3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3},
  {3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3},
  {3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3},
  {3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3},
  {3,0,0,0,0,0,0,0,8,3,3,3,3,3,3,3,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,3,3,3,3,3,3,3,8,0,0,0,0,0,0,0,3},
  {3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3},
  {3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3},
  {3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3},
  {3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3},
  {3,3,3,3,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,3,0,0,0,0,0,0,3,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,3,3,3,3},
  {3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3},
  {3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3},
  {3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3},
  {3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3},
  {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3},
};


void CreateMap() {
	glm::vec3 position;
	glm::vec3 size = { 0.5, 0.5, 0 };
	glm::vec3 velocity = { 1,0,0 };
	glm::vec3 acceleration{ 0,0,0 };

	for (int y = 0; y < LEVEL_HEIGHT; y++) {
		for (int x = 0; x < LEVEL_WIDTH; x++) {
			if (levelData[y][x] != 0) {
				position = { x, y, 0 };
				if (levelData[y][x] == 3) {
					spawnEntity(position, size, velocity, acceleration, 3,true, entity);
				}
				if (levelData[y][x] == 8) {
					spawnEntity(position, size, velocity, acceleration, 3,false, entity);
				}
			}
		}
	}
}
void EntityEntityCollision() {
	for (int i = 0; i < entity.size(); i++) {
		if (entity[i].isStatic == false) {
			entity[i].collidedTop = false;
			entity[i].collidedBottom = false;
			entity[i].collidedLeft = false;
			entity[i].collidedRight = false;
			for (int k = 0; k < entity.size(); k++) {
				if (entity[i].CollidesWith(entity[k]) && entity[k].textureID == 3) {
					glm::vec3 pen = abs(entity[i].position - entity[k].position) - (entity[i].size + entity[k].size);
					if (pen[1] > pen[0]) {
						if (entity[i].position[1] < entity[k].position[1]) {
							entity[i].position[1] += pen[1];
							entity[i].collidedTop = true;
						}
						else {
							entity[i].position[1] -= pen[1];
							entity[i].collidedBottom = true;
						}
					}
					if (pen[1] < pen[0]) {
						if (entity[i].position[0] > entity[k].position[0]) {
							entity[i].position[0] -= pen[0];
							entity[i].collidedLeft = true;
						}
						else {
							entity[i].position[0] += pen[0];
							entity[i].collidedRight = true;
						}
					}
				}
			}
		}
	}
}




void UpdateMap(float elapsed) {
	EntityEntityCollision();
	for (int i = 0; i < entity.size(); i++) {
		entity[i].Update(elapsed);
	}
}

void RenderMap(ShaderProgram &p) {
	for (int i = 0; i < entity.size(); i++) {
		entity[i].Render(p);
	}
}

void ProcessInput(Player &player) {
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	player.acceleration[0] = 0;

	if (keys[SDL_SCANCODE_W]) {	
		if (player.collidedBottom) {
			player.velocity[1] = 15;
		}	
	}
	if (keys[SDL_SCANCODE_W]) {
		if (player.collidedTop) {
			player.velocity[1] = -15;
		}
	}
	if (keys[SDL_SCANCODE_A]) {
		// go Left!
		player.acceleration[0] = -10;
		
	}
	if (keys[SDL_SCANCODE_D]) {
		// go Right!
		player.acceleration[0] = 10;
		
	}

}

void ProcessMenu() {
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_SPACE]) {
		// DO AN ACTION WHEN SPACE IS PRESSED!
		mode = STATE_PLAY;
	}
}

void RenderMenu() {
	glm::mat4 viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-9.5f, 0.0f, 1.0f));
	program.SetViewMatrix(viewMatrix);
	std::string text = "Press Space to Start";
	DrawText(program, text, 1, 0);
}


void RenderGameOver() {
	glm::mat4 viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-4.0f, 0.0f, 1.0f));
	program.SetViewMatrix(viewMatrix);
	std::string text = "Game Over";
	DrawText(program, text, 1, 0);
}


void Setup() {
	//Setup
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
	glewInit();
#endif

	glViewport(0, 0, 640, 360);

	
	program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	projectionMatrix = glm::mat4(1.0f);
	viewMatrix = glm::mat4(1.0f);
	modelMatrix = glm::mat4(1.0f);

	projectionMatrix = glm::ortho(-10.777f, 10.777f, -10.f, 10.0f, -1.0f, 1.0f);

	glUseProgram(program.programID);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	program.SetProjectionMatrix(projectionMatrix);
	program.SetViewMatrix(viewMatrix);
	program.SetModelMatrix(modelMatrix);
}

int main(int argc, char *argv[]){
	Setup();
	float accumulator = 0.0f;
	mode = STATE_MAIN_MENU;

	Player player;
	CreateMap();

    SDL_Event event;
    bool done = false;
    while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}

		ProcessInput(player);

		// get elapsed time
		ticks = (float)SDL_GetTicks() / 1000.0f;
		elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		elapsed += accumulator;
		if (elapsed < FIXED_TIMESTEP) {
			accumulator = elapsed;
			continue;
		}
		
		//Render();

		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		
		switch (mode) {
		case STATE_MAIN_MENU:
			ProcessMenu();
			RenderMenu();
			break;
		case STATE_PLAY:
			while (elapsed >= FIXED_TIMESTEP) {
				//Update(FIXED_TIMESTEP);
				UpdateMap(elapsed);
				player.Update(elapsed);
				elapsed -= FIXED_TIMESTEP;
			}
			accumulator = elapsed;
			RenderMap(program);
			player.Render(program);
			break;
		case STATE_GAME_OVER:
			RenderGameOver();
			break;
		}
		


		SDL_GL_SwapWindow(displayWindow);
		

		
    }
    
    SDL_Quit();
    return 0;
}

