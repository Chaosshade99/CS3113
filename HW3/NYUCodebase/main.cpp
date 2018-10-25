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

enum GameMode { STATE_MAIN_MENU, STATE_PLAY, STATE_GAME_OVER };
GameMode mode;


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

/*
class SheetSprite {
public:
	SheetSprite();
	SheetSprite(unsigned int texture, float t_u, float t_v, float t_width, float t_height, float t_size) {
		textureID = texture;
		u = t_u;
		v = t_v;
		width = t_width;
		height = t_height;
		size = t_size;
	};

	void Draw(ShaderProgram &program) {
		glBindTexture(GL_TEXTURE_2D, textureID);
		GLfloat texCoords[] = {
		u, v + height,
		u + width, v,
		u, v,
		u + width, v,
		u, v + height,
		u + width, v + height
		};
		float aspect = width / height;
		float vertices[] = {
		-0.5f * size * aspect, -0.5f * size,
		 0.5f * size * aspect, 0.5f * size,
		 -0.5f * size * aspect, 0.5f * size,
		 0.5f * size * aspect, 0.5f * size,
		 -0.5f * size * aspect, -0.5f * size ,
		 0.5f * size * aspect, -0.5f * size };
		// draw our arrays
	};

	float size;
	unsigned int textureID;
	float u;
	float v;
	float width;
	float height;
};
*/

class Enemy {
public:
	Enemy() {
		ModelMatrix = glm::mat4(1.0f);
		//Sprite = LoadTexture(RESOURCE_FOLDER"alien.png");
		Sprite = LoadTexture(RESOURCE_FOLDER"spritesheet_aliens.png");
		//mySprite = SheetSprite(Sprite, 425.0f / 1024.0f, 468.0f / 1024.0f, 93.0f / 1024.0f, 84.0f / 1024.0f, 0.2f);

		width = 1.0;
		height = 1.0;
		velocity = 3.0;
		direction_x = 1;
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

	void DrawSpriteSheetSprite(ShaderProgram &p) {
		glBindTexture(GL_TEXTURE_2D, Sprite);
		p.SetProjectionMatrix(projectionMatrix);
		p.SetViewMatrix(viewMatrix);
		p.SetModelMatrix(ModelMatrix);

		int index = 6;
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

		float vertices[] = {
		(-width + x), (-height + y),
		(width + x), (-height + y),
		(width + x), (height + y),
		(-width + x), (-height + y),
		(width + x), (height + y),
		(-width + x), (height + y)
		};
		// draw our arrays
		glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(p.positionAttribute);

		glVertexAttribPointer(p.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(p.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(p.positionAttribute);
		glDisableVertexAttribArray(p.texCoordAttribute);
	}

	void Update() {
		x += velocity * direction_x * elapsed;
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

class Arrow {
public:
	Arrow() {
		ModelMatrix = glm::mat4(1.0f);
		Sprite = LoadTexture(RESOURCE_FOLDER"laser.png");
		width = 0.25;
		height = 0.75;
		velocity = 50.0;
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

class Archer {
public:
	Archer() {
		ModelMatrix = glm::mat4(1.0f);
		ArcherSprite = LoadTexture(RESOURCE_FOLDER"alien.png");
		x = 0.0;
		y = -28.0;
		width = 1.0;
		height = 1.0;
		velocity = 25.0;
		direction_x = 0;
		direction_y = 0;
	};
	
	void Draw(ShaderProgram &p) {

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




void shootArrow(Archer &p, std::vector<Arrow> &shots) {
	Arrow newArrow;
	newArrow.x = p.x;
	newArrow.y = p.y;
	newArrow.direction_y = 1;
	newArrow.timeAlive = 0.0f;
	shots.push_back(newArrow);
}

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

void spawnMob(float x, float y, std::vector<Enemy> &mobs) {
	Enemy newMob;
	newMob.x = x;
	newMob.y = y;
	mobs.push_back(newMob);
}

void spawnWave(std::vector<Enemy> &mobs) {
	for (int x = -21; x < 21; x += 3) {
		for (int y = 19; y < 30; y += 3) {
			spawnMob(x, y, mobs);
		}
	}
	
	
}

bool shouldRemoveEnemy(Enemy mob) {
	if (mob.dead == true) {
		return true;
	}
	else {
		return false;
	}
}


void allMobsSpeedUp(std::vector<Enemy> &mobs) {
	for (int i = 0; i < mobs.size(); i++) {
		mobs[i].velocity += 0.5;
	}
}

void allMobsMoveLeft(std::vector<Enemy> &mobs) {
	for (int i = 0; i < mobs.size(); i++) {
		mobs[i].direction_x = -1;
	}
}
void allMobsMoveRight(std::vector<Enemy> &mobs) {
	for (int i = 0; i < mobs.size(); i++) {
		mobs[i].direction_x = 1;
	}
}
void allMobsMoveDown(std::vector<Enemy> &mobs) {
	for (int i = 0; i < mobs.size(); i++) {
		mobs[i].y += -0.8;
	}
}

void BulletEnemyCollision(std::vector<Arrow> &shots, std::vector<Enemy> &mobs) {
	float px;
	float py;
	for (int i = 0; i < shots.size(); i++) {
		for (int k = 0; k < mobs.size(); k++) {
			px = abs(mobs[k].x - shots[i].x);
			px -= (mobs[k].width + shots[i].width + 0.75) / 2;
			py = abs(mobs[k].y - shots[i].y);
			py -= (mobs[k].height + shots[i].height) / 2;
			if (px <= 0 && py <= 0) {
				mobs[k].dead = true;
				shots[i].dead = true;
				allMobsSpeedUp(mobs);
			}
		}
	}
}





class GameState {
public:
	GameState() {
		spawnWave(mobs);
	};
	Archer player;
	std::vector<Enemy> mobs;
	std::vector<Arrow> shots;
};


void RenderGame(GameState &state) {
	// render all the entities in the game
	// render score and other UI elements
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	for (int i = 0; i < state.shots.size(); i++) {
		state.shots[i].Draw(program);
	}
	for (int i = 0; i < state.mobs.size(); i++) {
		//state.mobs[i].Draw(program);
		state.mobs[i].DrawSpriteSheetSprite(program);
	}
	state.player.Draw(program);
}
void UpdateGame(GameState &state, float elapsed) {
	// move all the entities based on time elapsed and their velocity
	for (int i = 0; i < state.mobs.size(); i++) {
		if (state.mobs[i].x > 30) {
			allMobsMoveLeft(state.mobs);
			allMobsMoveDown(state.mobs);
		}
		if (state.mobs[i].x < -30) {
			allMobsMoveRight(state.mobs);
			allMobsMoveDown(state.mobs);
		}
	}

	BulletEnemyCollision(state.shots, state.mobs);
	state.shots.erase(std::remove_if(state.shots.begin(), state.shots.end(), shouldRemoveBullet), state.shots.end());
	state.mobs.erase(std::remove_if(state.mobs.begin(), state.mobs.end(), shouldRemoveEnemy), state.mobs.end());


	for (int i = 0; i < state.shots.size(); i++) {
		state.shots[i].Update();
	}
	for (int i = 0; i < state.mobs.size(); i++) {
		state.mobs[i].Update();
	}


	state.player.Update();

}
void ProcessInput(GameState &state) {
	const Uint8 *keys = SDL_GetKeyboardState(NULL);

	/*
	if (keys[SDL_KEYDOWN]) {
		if (keys[SDL_SCANCODE_W]) {
				// DO AN ACTION WHEN SPACE IS PRESSED!
			shootArrow(state.player, state.shots);
		}
	}
	*/

	state.player.direction_x = 0;
	if (keys[SDL_SCANCODE_A]) {
		// go Left!
		state.player.direction_x = -1;
		if (state.player.x <= -30) {
			state.player.direction_x = 0;
		}
	}
	if (keys[SDL_SCANCODE_D]) {
		// go Right!
		state.player.direction_x = 1;
		if (state.player.x >= 30) {
			state.player.direction_x = 0;
		}
	}

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

	projectionMatrix = glm::ortho(-30.777f, 30.777f, -30.f, 30.0f, -1.0f, 1.0f);

	glUseProgram(program.programID);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

}


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
	glm::mat4 ModelMatrix = glm::translate(modelMatrix, glm::vec3(-20.0f, 0.0f, 1.0f));

	p.SetProjectionMatrix(projectionMatrix);
	p.SetViewMatrix(viewMatrix);
	p.SetModelMatrix(ModelMatrix);
	

	glBindTexture(GL_TEXTURE_2D, fontTexture);
	// draw this data (use the .data() method of std::vector to get pointer to data)
	// draw this yourself, use text.size() * 6 or vertexData.size()/2 to get number of vertices

	glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(p.positionAttribute);

	glVertexAttribPointer(p.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(p.texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, text.size()* 6 );

	glDisableVertexAttribArray(p.positionAttribute);
	glDisableVertexAttribArray(p.texCoordAttribute);



}

void ProcessEvents() {
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_SPACE]) {
		// DO AN ACTION WHEN SPACE IS PRESSED!
		mode = STATE_PLAY;	
	}
}

void Update(float elapsed) {
	// move stuff and check for collisions

}

void Render() {
	// for all game elements
	// setup transforms, render sprites
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	
	std::string text = "Press Space to Start";
	DrawText(program, text,3,-1);
}





int main(int argc, char *argv[]){
	Setup();
	float lastFrameTicks = 0.0f;
	GameState playState;

	mode = STATE_MAIN_MENU;

    SDL_Event event;
    bool done = false;
    while (!done) {
		ticks = (float)SDL_GetTicks() / 1000.0f;
		elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.type == SDL_KEYUP) {
				if (event.key.keysym.scancode == SDL_SCANCODE_W) {
					// DO AN ACTION WHEN SPACE IS PRESSED!
					shootArrow(playState.player, playState.shots);
				}
			}
		}
	
		switch (mode) {
		case STATE_MAIN_MENU:
			ProcessEvents();
			Update(elapsed);
			Render();
			break;
		case STATE_PLAY:
			ProcessInput(playState);
			//Update
			UpdateGame(playState, elapsed);
			//Render
			RenderGame(playState);
			break;
		}

	
		SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}

