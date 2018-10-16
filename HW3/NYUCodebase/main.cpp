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

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;
//global
ShaderProgram program;
glm::mat4 projectionMatrix;
glm::mat4 viewMatrix;
glm::mat4 modelMatrix;
float lastFrameTicks;
float ticks;
float elapsed;



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


class Enemy {
public:
	Enemy() {
		ModelMatrix = glm::mat4(1.0f);
		Sprite = LoadTexture(RESOURCE_FOLDER"alien.png");
		x = 0;
		y = 8;
		width = 1.0;
		height = 1.0;
		velocity = 25.0;
		direction_x = 0;
		direction_y = 0;
	};

	void Draw(ShaderProgram &p) {

		glBindTexture(GL_TEXTURE_2D, Sprite);
		p.SetProjectionMatrix(projectionMatrix);
		p.SetViewMatrix(viewMatrix);
		p.SetModelMatrix(ModelMatrix);

		float Vertices[] = { (-width + x), (-height + y), (width + x), (-height + y), (width + x), (height + y), (-width + x), (-height + y), (width + x), (height + y), (-width + x), (height + y) };
		glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, Vertices);
		glEnableVertexAttribArray(p.positionAttribute);

		float Coords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(p.texCoordAttribute, 2, GL_FLOAT, false, 0, Coords);
		glEnableVertexAttribArray(p.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(p.positionAttribute);
		glDisableVertexAttribArray(p.texCoordAttribute);
	};

	void Update() {
		
	}

	glm::mat4 ModelMatrix;
	GLuint Sprite;
	bool dead;
	float x;
	float y;
	float rotation;
	int textureID;

	float width;
	float height;
	float velocity;
	float direction_x;
	float direction_y;
};

std::vector<Enemy> mobs;

void spawnMob(float x) {
	Enemy newMob;
	newMob.x = x;
	mobs.push_back(newMob);
}

bool shouldRemoveEnemy(Enemy mob) {
	if (mob.dead == true) {
		return true;
	}
	else {
		return false;
	}
}


class Arrow {
public:
	Arrow() {
		ModelMatrix = glm::mat4(1.0f);
		Sprite = LoadTexture(RESOURCE_FOLDER"laser.png");
		width = 0.5;
		height = 0.5;
		velocity = 25.0;
		direction_y = 1;
	};

	void Draw(ShaderProgram &p) {

		glBindTexture(GL_TEXTURE_2D, Sprite);
		p.SetProjectionMatrix(projectionMatrix);
		p.SetViewMatrix(viewMatrix);
		p.SetModelMatrix(ModelMatrix);

		float Vertices[] = { (-width + x), (-height + y), (width + x), (-height + y), (width + x), (height + y), (-width + x), (-height + y), (width + x), (height + y), (-width + x), (height + y) };
		glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, Vertices);
		glEnableVertexAttribArray(p.positionAttribute);


		float Coords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(p.texCoordAttribute, 2, GL_FLOAT, false, 0, Coords);
		glEnableVertexAttribArray(p.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(p.positionAttribute);
		glDisableVertexAttribArray(p.texCoordAttribute);

	};

	void Update() {
		y += velocity * direction_y * elapsed;
		timeAlive += 0.1 * elapsed;
	}

	glm::mat4 ModelMatrix;
	GLuint Sprite;
	bool dead;
	float x;
	float y;
	float timeAlive;
	int textureID;
	float width;
	float height;
	float velocity;
	float direction_y;
};

bool shouldRemoveBullet(Arrow bullet) {
	if (bullet.dead == true) {
		return true;
	}
	if (bullet.timeAlive > 2.0) {
		return true;
	}
	else {
		return false;
	}
}

std::vector<Arrow> shots;

void BulletEnemyCollision() {
	float px;
	float py;
	for (int i = 0; i < shots.size(); i++) {
		for (int i = 0; i < mobs.size(); i++) {
			px = abs(mobs[i].x - shots[i].x);
			px -= (mobs[i].width + shots[i].width) / 2;
			py = abs(mobs[i].y - shots[i].y);
			py -= (mobs[i].height + shots[i].height) / 2;
			if (px <= 0 && py <= 0 ) {
				mobs[i].dead = true;
				shots[i].dead = true;
			}
		}
	}

}

class Archer {
public:
	Archer() {
		ModelMatrix = glm::mat4(1.0f);
		ArcherSprite = LoadTexture(RESOURCE_FOLDER"alien.png");
		x = 0;
		y = -8;
		width = 1.0;
		height = 1.0;
		velocity = 25.0;
		direction_x = 0;
		direction_y = 0;
	};

	void Draw(ShaderProgram &p){

		glBindTexture(GL_TEXTURE_2D, ArcherSprite);
		p.SetProjectionMatrix(projectionMatrix);
		p.SetViewMatrix(viewMatrix);
		p.SetModelMatrix(ModelMatrix);

		float Vertices[] = { (-width + x), (-height + y), (width + x), (-height + y), (width + x), (height + y), (-width + x), (-height + y), (width + x), (height + y), (-width + x), (height + y) };
		glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, Vertices);
		glEnableVertexAttribArray(p.positionAttribute);

		float Coords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(p.texCoordAttribute, 2, GL_FLOAT, false, 0, Coords);
		glEnableVertexAttribArray(p.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(p.positionAttribute);
		glDisableVertexAttribArray(p.texCoordAttribute);
	};

	void Update() {
		x += velocity * direction_x * elapsed;
	}

	void shootArrow() {
		Arrow newArrow;
		newArrow.x = x;
		newArrow.y = y;
		newArrow.direction_y = 1;
		newArrow.timeAlive = 0.0f;
		shots.push_back(newArrow);
	}

	
	glm::mat4 ModelMatrix;
	GLuint ArcherSprite;
	float x;
	float y;
	float rotation;
	int textureID;

	float width;
	float height;
	float velocity;
	float direction_x;
	float direction_y;
};

Archer player;

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

}



void ProcessEvents() {
	// our SDL event loop
	// check input events
	


	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	player.direction_x = 0;
	if (keys[SDL_SCANCODE_A]) {
		// go Left!
		player.direction_x = -1;
	}
	if (keys[SDL_SCANCODE_D]) {
		// go Right!
		player.direction_x = 1;
	}
	


}

void Update() {
	// move stuff and check for collisions
	ticks = (float)SDL_GetTicks() / 1000.0f;
	elapsed = ticks - lastFrameTicks;
	lastFrameTicks = ticks;

	player.Update();

	BulletEnemyCollision();
	shots.erase(std::remove_if(shots.begin(), shots.end(), shouldRemoveBullet), shots.end());
	mobs.erase(std::remove_if(mobs.begin(), mobs.end(), shouldRemoveEnemy), mobs.end());


	for (int i = 0; i < shots.size(); i++) {
		shots[i].Update();
	}
	for (int i = 0; i < mobs.size(); i++) {
		mobs[i].Update();
	}

}

void Render() {
	// for all game elements
	// setup transforms, render sprites
	
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	player.Draw(program);
	for (int i = 0; i < shots.size(); i++) {
		shots[i].Draw(program);
	}
	for (int i = 0; i < mobs.size(); i++) {
		mobs[i].Draw(program);
	}


}


int main(int argc, char *argv[]){
	Setup();
	float lastFrameTicks = 0.0f;

		
    SDL_Event event;
    bool done = false;
    while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}else if (event.type == SDL_KEYUP) {
				if (event.key.keysym.scancode == SDL_SCANCODE_W) {
					player.shootArrow();
					spawnMob(player.x);
				}
			}

		}
		
		ProcessEvents();
		//Update
		Update();
	
		//Render
		Render();

	
		SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}
