#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <SDL_mixer.h>

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
Mix_Chunk *someSound;



float lastFrameTicks;
float ticks;
float elapsed;
float PI = 3.1415926f;


enum GameMode {
				STATE_MENU,
				STATE_MENU1, STATE_PREP1, STATE_PLAY_LV1,
				STATE_MENU2, STATE_PREP2, STATE_PLAY_LV2,
				STATE_MENU3, STATE_PREP3, STATE_PLAY_LV3,
				STATE_VICTORY, STATE_GAME_OVER };
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


//Animation
float lerp(float from, float to, float t) {
	return (1.0 - t)*from + t * to;
}
float mapValue(float value, float srcMin, float srcMax, float dstMin, float dstMax) {
	float retVal = dstMin + ((value - srcMin) / (srcMax - srcMin) * (dstMax - dstMin));
	if (retVal < dstMin) {
		retVal = dstMin;
	}
	if (retVal > dstMax) {
		retVal = dstMax;
	}
	return retVal;
}

//animation end

class Explosion {
public:
	Explosion(float x, float y) {
		position = { x, y, 0.0f };
		size = { 5.0, 5.0, 0.0 };
		sprite = LoadTexture(RESOURCE_FOLDER"fire.png");
		sprite2 = LoadTexture(RESOURCE_FOLDER"fire2.png");
		dead = false;
		timeAlive = 0.0f;

	};
	void Update(float elapsed) {	
		timeAlive += elapsed;
		if (timeAlive >= 1.0f) {
			dead = true;
		}

		float animationValue = mapValue(timeAlive, 5.0f, 10.0f, 0.0f, 1.0f);
		modelMatrix = glm::mat4(1.0f);
		size[0] = lerp(0.5, 3.0, timeAlive);
		size[1] = lerp(0.5, 3.0, timeAlive);

		ExplosionModelMatrix = glm::translate(modelMatrix, glm::vec3(position[0], position[1], position[2]));

	};
	void Render(ShaderProgram &p) {
		program.SetModelMatrix(ExplosionModelMatrix);
		glBindTexture(GL_TEXTURE_2D, sprite);
		float Vertices[] = {
			(-size[0]), (-size[1]),
			(size[0]), (-size[1]),
			(size[0]), (size[1]),
			(-size[0]), (-size[1]),
			(size[0]), (size[1]),
			(-size[0]), (size[1])
		};
		glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, Vertices);
		glEnableVertexAttribArray(p.positionAttribute);

		float TexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(p.texCoordAttribute, 2, GL_FLOAT, false, 0, TexCoords);
		glEnableVertexAttribArray(p.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(p.positionAttribute);
		glDisableVertexAttribArray(p.texCoordAttribute);

		glBindTexture(GL_TEXTURE_2D, sprite2);
		float Vertices2[] = {
			(-size[0] * 3 / 4), (-size[1] * 3 / 4),
			(size[0] * 3 / 4), (-size[1] * 3 / 4),
			(size[0] * 3 / 4), (size[1] * 3 / 4),
			(-size[0] * 3 / 4), (-size[1] * 3 / 4),
			(size[0] * 3 / 4), (size[1] * 3 / 4),
			(-size[0] * 3 / 4), (size[1] * 3 / 4)
		};
		glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, Vertices2);
		glEnableVertexAttribArray(p.positionAttribute);

		float TexCoords2[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(p.texCoordAttribute, 2, GL_FLOAT, false, 0, TexCoords2);
		glEnableVertexAttribArray(p.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(p.positionAttribute);
		glDisableVertexAttribArray(p.texCoordAttribute);
	};

	glm::mat4 ExplosionModelMatrix;
	GLuint sprite;
	GLuint sprite2;
	glm::vec3 position;
	glm::vec3 size;

	bool dead;
	float timeAlive;
};

void spawnExplosion(std::vector<Explosion> &explosion, float x, float y) {
	Explosion newExplosion(x,y);
	explosion.push_back(newExplosion);
};


bool shouldRemoveExplosion(Explosion &explosion) {
	if (explosion.dead) {
		return true;
	}
	else {
		return false;
	}
};

void UpdateExplosion(std::vector<Explosion> &explosion, float &elapsed) {
	explosion.erase(std::remove_if(explosion.begin(), explosion.end(), shouldRemoveExplosion), explosion.end());

	for (int i = 0; i < explosion.size(); i++) {
		explosion[i].Update(elapsed);
	}
};

void RenderExplosion(std::vector<Explosion> &explosion, ShaderProgram &p) {
	for (int i = 0; i < explosion.size(); i++) {
		explosion[i].Render(p);
	}
};

//Background
class Terrain {
public:
	Terrain(float &x, float &y) {
		position = { x, y, 0.0 };
		size = { 2.5, 2.5, 0.0 };

		TerrainMatrix = glm::translate(modelMatrix, glm::vec3(position[0], position[1], position[2]));
		sprite = LoadTexture(RESOURCE_FOLDER"terrainTile2.png");
	}
	void Render(ShaderProgram &p) {

		program.SetModelMatrix(TerrainMatrix);
		glBindTexture(GL_TEXTURE_2D, sprite);

		float VerticesBody[] = {
			(-size[0]), (-size[1]),
			(size[0]), (-size[1]),
			(size[0]), (size[1]),
			(-size[0]), (-size[1]),
			(size[0]), (size[1]),
			(-size[0]), (size[1])
		};

		glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, VerticesBody);
		glEnableVertexAttribArray(p.positionAttribute);

		float bodyTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(p.texCoordAttribute, 2, GL_FLOAT, false, 0, bodyTexCoords);
		glEnableVertexAttribArray(p.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(p.positionAttribute);
		glDisableVertexAttribArray(p.texCoordAttribute);
	}
	

	glm::mat4 TerrainMatrix;
	GLuint sprite;

	glm::vec3 position;
	glm::vec3 size;

};

void InsertTerrain(std::vector<Terrain> &terrain, float x, float y) {
	Terrain newTerrain(x, y);
	terrain.push_back(newTerrain);
};

void RenderTerrain(std::vector<Terrain> &terrain, ShaderProgram &p) {
	for (int i = 0; i < terrain.size(); i++) {
		terrain[i].Render(p);
	}
};

class Laser {
public:
	Laser(float x, float y, float &refAngle) {
		position = { x, y, 0.0f };
		size = { 0.5, 0.5, 0.0 };
		velocity = 20.f;
		angle = refAngle;
		sprite = LoadTexture(RESOURCE_FOLDER"laser.png");
		float horizontal = sin(angle * (3.1415926f / 180.0f));
		float vertical = cos(angle * (3.1415926f / 180.0f));
		position[0] += (1.5 * horizontal);
		position[1] += (1.5 * vertical);
		dead = false;
		timeAlive = 0.0f;

		
	};
	void Update(float elapsed) {
		float horizontal = sin(angle * (3.1415926f / 180.0f));
		float vertical = cos(angle * (3.1415926f / 180.0f));
		position[0] += (velocity * horizontal * elapsed);
		position[1] += (velocity * vertical * elapsed);
		timeAlive += elapsed;

		laserModelMatrix = glm::translate(modelMatrix, glm::vec3(position[0], position[1], position[2]));
		float Angle = -angle * (3.1415926f / 180.0f);
		laserModelMatrix = glm::rotate(laserModelMatrix, Angle, glm::vec3(0.0f, 0.0f, 1.0f));
	};
	void Render(ShaderProgram &p){
		program.SetModelMatrix(laserModelMatrix);
		glBindTexture(GL_TEXTURE_2D, sprite);

		float Vertices[] = {
			(-size[0]), (-size[1]),
			(size[0]), (-size[1]),
			(size[0]), (size[1]),
			(-size[0]), (-size[1]),
			(size[0]), (size[1]),
			(-size[0]), (size[1])
		};

		glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, Vertices);
		glEnableVertexAttribArray(p.positionAttribute);

		float TexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(p.texCoordAttribute, 2, GL_FLOAT, false, 0, TexCoords);
		glEnableVertexAttribArray(p.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(p.positionAttribute);
		glDisableVertexAttribArray(p.texCoordAttribute);
	};
	glm::mat4 laserModelMatrix;

	GLuint sprite;
	glm::vec3 position;
	glm::vec3 size;
	float velocity;
	float angle;
	bool dead;
	float timeAlive;
};

class Player {
public:
	Player() {
		position = { 0, 0, 0.0 };
		size = { 1.5, 1.5, 0.0 };
		velocity = 0.0f;
		angle = 0.0f;
		turrentAngle = 0.0f;
		spriteBody = LoadTexture(RESOURCE_FOLDER"PlayerBody.png");
		spriteHead = LoadTexture(RESOURCE_FOLDER"PlayerTurrent.png");
		spriteHP = LoadTexture(RESOURCE_FOLDER"green_hp.png");
		spriteHPfont = LoadTexture(RESOURCE_FOLDER"HP_font.png");
		spriteReload = LoadTexture(RESOURCE_FOLDER"red_reload.png");
		spriteReloadfont = LoadTexture(RESOURCE_FOLDER"Reload_font.png");
		reload = 1.0f;
		hp = 100;
	}

	void Update(float elapsed) {

		reload -= elapsed;
		float horizontal = sin(angle * (3.1415926f / 180.0f));
		float vertical = cos(angle * (3.1415926f / 180.0f));
		position[0] += (velocity * horizontal * elapsed);
		position[1] += (velocity * vertical * elapsed);

		playermodelMatrixBody = glm::translate(modelMatrix, glm::vec3(position[0], position[1], position[2]));
		float bodyAngle = -angle * (3.1415926f / 180.0f);
		playermodelMatrixBody = glm::rotate(playermodelMatrixBody, bodyAngle, glm::vec3(0.0f, 0.0f, 1.0f));

		playermodelMatrixHead = glm::translate(modelMatrix, glm::vec3(position[0], position[1], position[2]));
		float headAngle = -turrentAngle * (3.1415926f / 180.0f);
		playermodelMatrixHead = glm::rotate(playermodelMatrixHead, headAngle, glm::vec3(0.0f, 0.0f, 1.0f));

		viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3{ -position[0],-position[1], -position[2] });
		program.SetViewMatrix(viewMatrix);
	}
	
	void Render(ShaderProgram &p) {
		RenderHud(p);
		program.SetModelMatrix(playermodelMatrixBody);
		glBindTexture(GL_TEXTURE_2D, spriteBody);

		float VerticesBody[] = {
			(-size[0]), (-size[1]),
			(size[0]), (-size[1]),
			(size[0]), (size[1]),
			(-size[0]), (-size[1]),
			(size[0]), (size[1]),
			(-size[0]), (size[1])
		};

		glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, VerticesBody);
		glEnableVertexAttribArray(p.positionAttribute);

		float bodyTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(p.texCoordAttribute, 2, GL_FLOAT, false, 0, bodyTexCoords);
		glEnableVertexAttribArray(p.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(p.positionAttribute);
		glDisableVertexAttribArray(p.texCoordAttribute);

		//
		program.SetModelMatrix(playermodelMatrixHead);
		glBindTexture(GL_TEXTURE_2D, spriteHead);

		float VerticesHead[] = {
			(-size[0] * 2 / 3), (-size[1] * 2 / 3),
			(size[0] * 2 / 3), (-size[1] * 2 / 3),
			(size[0] * 2 / 3), (size[1] * 5 / 3),
			(-size[0] * 2 / 3), (-size[1] * 2 / 3),
			(size[0] * 2 / 3), (size[1] * 5 / 3),
			(-size[0] * 2 / 3), (size[1] * 5 / 3)
		};

		glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, VerticesHead);
		glEnableVertexAttribArray(p.positionAttribute);

		float headTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(p.texCoordAttribute, 2, GL_FLOAT, false, 0, headTexCoords);
		glEnableVertexAttribArray(p.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(p.positionAttribute);
		glDisableVertexAttribArray(p.texCoordAttribute);
	}

	void RenderHud(ShaderProgram &p) {
		//issue: speed up game
		hudMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(position[0] - 18.0f, position[1] - 18.0f, 0.0f));
		program.SetModelMatrix(hudMatrix);

		//Words HP
		glBindTexture(GL_TEXTURE_2D, spriteHPfont);

		float HPFont[] = {
			(-1), (-1),
			(1), (-1),
			(1), (1),
			(-1), (-1),
			(1), (1),
			(-1), (1)
		};

		glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, HPFont);
		glEnableVertexAttribArray(p.positionAttribute);

		float HPFontTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(p.texCoordAttribute, 2, GL_FLOAT, false, 0, HPFontTexCoords);
		glEnableVertexAttribArray(p.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(p.positionAttribute);
		glDisableVertexAttribArray(p.texCoordAttribute);
		
		glBindTexture(GL_TEXTURE_2D, spriteHP);
		float percent = hp / 5.0;

		//HP bar
		float HP[] = {
			(2.0), (-1.0),
			(percent + 2.0), (-1.0),
			(percent + 2.0), (1.0),
			(2.0), (-1.0),
			(percent +2.0), (1.0),
			(2.0), (1.0)
		};

		glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, HP);
		glEnableVertexAttribArray(p.positionAttribute);

		float HPTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(p.texCoordAttribute, 2, GL_FLOAT, false, 0, HPTexCoords);
		glEnableVertexAttribArray(p.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(p.positionAttribute);
		glDisableVertexAttribArray(p.texCoordAttribute);

		//Reload
		glBindTexture(GL_TEXTURE_2D, spriteReloadfont);

		float MaxReload[] = {
			(24.0), (-1.0),
			(29.0), (-1.0),
			(29.0), (1.0),
			(24.0), (-1.0),
			(29.0), (1.0),
			(24.0), (1.0)
		};

		glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, MaxReload);
		glEnableVertexAttribArray(p.positionAttribute);

		float MaxReloadTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(p.texCoordAttribute, 2, GL_FLOAT, false, 0, MaxReloadTexCoords);
		glEnableVertexAttribArray(p.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(p.positionAttribute);
		glDisableVertexAttribArray(p.texCoordAttribute);
	

		if (reload <= 0) {
			glBindTexture(GL_TEXTURE_2D, spriteHP);

			float MaxReload[] = {
				(30.0), (-1.0),
				(5 + 30.0), (-1.0),
				(5 + 30.0), (1.0),
				(30.0), (-1.0),
				(5 + 30.0), (1.0),
				(30.0), (1.0)
			};

			glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, MaxReload);
			glEnableVertexAttribArray(p.positionAttribute);

			float MaxReloadTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
			glVertexAttribPointer(p.texCoordAttribute, 2, GL_FLOAT, false, 0, MaxReloadTexCoords);
			glEnableVertexAttribArray(p.texCoordAttribute);

			glDrawArrays(GL_TRIANGLES, 0, 6);
			glDisableVertexAttribArray(p.positionAttribute);
			glDisableVertexAttribArray(p.texCoordAttribute);
		}
		else {
			glBindTexture(GL_TEXTURE_2D, spriteReload);

			float MaxReload[] = {
				(30.0), (-1.0),
				(5 - reload * 5 + 30.0), (-1.0),
				(5 - reload * 5 + 30.0), (1.0),
				(30.0), (-1.0),
				(5 - reload * 5 + 30.0), (1.0),
				(30.0), (1.0)
			};

			glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, MaxReload);
			glEnableVertexAttribArray(p.positionAttribute);

			float MaxReloadTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
			glVertexAttribPointer(p.texCoordAttribute, 2, GL_FLOAT, false, 0, MaxReloadTexCoords);
			glEnableVertexAttribArray(p.texCoordAttribute);

			glDrawArrays(GL_TRIANGLES, 0, 6);
			glDisableVertexAttribArray(p.positionAttribute);
			glDisableVertexAttribArray(p.texCoordAttribute);
		}
	}
	
	bool CollidesWithLaser(Laser &laser) {
		float radius = sqrt((size[0] + laser.size[0]) * (size[1] + laser.size[1]));
		float x = position[0] - laser.position[0];
		float y = position[1] - laser.position[1];
		float distance = sqrt(pow(x, 2) + pow(y, 2));

		if (distance <= radius) {
			return true;
		}
		else {
			return false;
		}
	}


	glm::mat4 playermodelMatrixBody;
	glm::mat4 playermodelMatrixHead;
	glm::mat4 hudMatrix;

	GLuint spriteBody;
	GLuint spriteHead;
	GLuint spriteHP;
	GLuint spriteHPfont;
	GLuint spriteReload;
	GLuint spriteReloadfont;

	glm::vec3 position;
	glm::vec3 size;
	float velocity;
	float angle;
	float turrentAngle;
	float reload;

	int hp;
};

//TURRENT ENEMY

class Turrent {
public:
	Turrent(float &x, float &y) {
		position = { x, y, 0.0 };
		size = { 1.5, 1.5, 0.0 };
		angle = 180.0f;
		turrentAngle = 0.0f;
		spriteBody = LoadTexture(RESOURCE_FOLDER"TurrentBody.png");
		spriteHead = LoadTexture(RESOURCE_FOLDER"Turrent.png");
		spriteHP = LoadTexture(RESOURCE_FOLDER"green_hp.png");
		dead = false;
		reload = 1.0f;
		hp = 2;
	}
	void Update(float elapsed) {
		if (turrentAngle > 360) {
			turrentAngle += -360;
		}
		if (turrentAngle < -360) {
			turrentAngle += 360;
		}
		reload -= elapsed;

		modelMatrixBody = glm::translate(modelMatrix, glm::vec3(position[0], position[1], position[2]));
		float bodyAngle = -angle * (3.1415926f / 180.0f);
		modelMatrixBody = glm::rotate(modelMatrixBody, bodyAngle, glm::vec3(0.0f, 0.0f, 1.0f));

		modelMatrixHead = glm::translate(modelMatrix, glm::vec3(position[0], position[1], position[2]));
		float headAngle = -turrentAngle * (3.1415926f / 180.0f);
		modelMatrixHead = glm::rotate(modelMatrixHead, headAngle, glm::vec3(0.0f, 0.0f, 1.0f));
	}
	void Render(ShaderProgram &p) {
		

		program.SetModelMatrix(modelMatrixBody);
		glBindTexture(GL_TEXTURE_2D, spriteBody);

		float VerticesBody[] = {
			(-size[0]), (-size[1]),
			(size[0]), (-size[1]),
			(size[0]), (size[1]),
			(-size[0]), (-size[1]),
			(size[0]), (size[1]),
			(-size[0]), (size[1])
		};

		glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, VerticesBody);
		glEnableVertexAttribArray(p.positionAttribute);

		float bodyTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(p.texCoordAttribute, 2, GL_FLOAT, false, 0, bodyTexCoords);
		glEnableVertexAttribArray(p.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(p.positionAttribute);
		glDisableVertexAttribArray(p.texCoordAttribute);

		//
		program.SetModelMatrix(modelMatrixHead);
		glBindTexture(GL_TEXTURE_2D, spriteHead);

		float VerticesHead[] = {
			(-size[0] * 2 / 3), (-size[1] * 2 / 3),
			(size[0] * 2 / 3), (-size[1] * 2 / 3),
			(size[0] * 2 / 3), (size[1] * 5 / 3),
			(-size[0] * 2 / 3), (-size[1] * 2 / 3),
			(size[0] * 2 / 3), (size[1] * 5 / 3),
			(-size[0] * 2 / 3), (size[1] * 5 / 3)
		};

		glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, VerticesHead);
		glEnableVertexAttribArray(p.positionAttribute);

		float headTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(p.texCoordAttribute, 2, GL_FLOAT, false, 0, headTexCoords);
		glEnableVertexAttribArray(p.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(p.positionAttribute);
		glDisableVertexAttribArray(p.texCoordAttribute);

		RenderHud(p);

	}
	void RenderHud(ShaderProgram &p) {
		//issue: speed up game
		hpMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(position[0], position[1] + size[1], 0.0f));
		program.SetModelMatrix(hpMatrix);


		glBindTexture(GL_TEXTURE_2D, spriteHP);
		

		float HP[] = {
			(-1.0), (-0.25),
			(hp - 1.0), (-0.25),
			(hp - 1.0), (0.25),
			(-1.0), (-0.25),
			(hp - 1.0), (0.25),
			(-1.0), (0.25)
		};

		glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, HP);
		glEnableVertexAttribArray(p.positionAttribute);

		float HPTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(p.texCoordAttribute, 2, GL_FLOAT, false, 0, HPTexCoords);
		glEnableVertexAttribArray(p.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(p.positionAttribute);
		glDisableVertexAttribArray(p.texCoordAttribute);
	}



	bool CollidesWithLaser(Laser &laser) {
		float radius = sqrt((size[0] + laser.size[0]) * (size[1] + laser.size[1]));
		float x = position[0] - laser.position[0];
		float y = position[1] - laser.position[1];
		float distance = sqrt(pow(x, 2) + pow(y, 2));

		if (distance <= radius) {
			return true;
		}
		else {
			return false;
		}
	}
	bool CollidesWithPlayer(Player &player) {
		float radius1 = sqrt(size[0] * size[1]);
		float radius2 = sqrt(player.size[0] * player.size[1]);
		float x = position[0] - player.position[0];
		float y = position[1] - player.position[1];
		float distance = sqrt(pow(x, 2) + pow(y, 2));

		if (distance <= radius1 + radius2) {
			return true;
		}
		else {
			return false;
		}
	}
	float PlayerPen(Player &player) {
		float radius1 = sqrt(size[0] * size[1]);
		float radius2 = sqrt(player.size[0] * player.size[1]);
		float x = position[0] - player.position[0];
		float y = position[1] - player.position[1];
		float distance = sqrt(pow(x, 2) + pow(y, 2));

		float penetration = fabs(distance - radius1 - radius2);
		return penetration;
	}


	glm::mat4 modelMatrixBody;
	glm::mat4 modelMatrixHead;
	glm::mat4 hpMatrix;

	GLuint spriteBody;
	GLuint spriteHead;
	GLuint spriteHP;

	glm::vec3 position;
	glm::vec3 size;
	float angle;
	float turrentAngle;
	bool dead;
	float reload;
	int hp;
};

void TurrentTargetNeutral(Turrent &e) {
	float angle = e.angle;
	if (e.turrentAngle != angle) {
		if (e.turrentAngle - angle > 180) {
			e.turrentAngle += -360;
		}
		if (e.turrentAngle - angle < -180) {
			e.turrentAngle += 360;
		}
		if (e.turrentAngle < angle) {
			e.turrentAngle += 2.0f;
		}
		else {
			e.turrentAngle -= 2.0f;
		}
		if (angle - 2 < e.turrentAngle && e.turrentAngle < angle + 2) {
			e.turrentAngle = angle;
		}
	}
}

void TurrentshootLaser(Turrent &e, std::vector<Laser> &shots) {
	someSound = Mix_LoadWAV("Fire2.ogg");
	Mix_PlayChannel(-1, someSound, 0);

	Laser newShot(e.position[0], e.position[1], e.turrentAngle);
	shots.push_back(newShot);
};


void TurrentTargetPlayer(Turrent &e, Player &p, std::vector<Laser> &s) {
	float x = p.position[0] - e.position[0];
	float y = p.position[1] - e.position[1];
	float angle = -atan2(y, x) / (3.1415926f / 180.0f) + 90;
	if (e.turrentAngle != angle) {
		if (e.turrentAngle - angle > 180) {
			e.turrentAngle += -360;
		}
		if (e.turrentAngle - angle < -180) {
			e.turrentAngle += 360;
		}
		if (e.turrentAngle < angle) {
			e.turrentAngle += 2.0f;
		}
		else {
			e.turrentAngle -= 2.0f;
		}
	}
	if (angle - 2 < e.turrentAngle && e.turrentAngle < angle + 2) {
		//e.turrentAngle = angle;
		if (e.reload <= 0) {
			TurrentshootLaser(e, s);
			e.reload = 2.0f;
		}
	}
	//e.turrentAngle = angle;
};

void spawnNewEnemyTurrent(std::vector<Turrent> &enemies, float x, float y) {
	Turrent newEnemy(x, y);
	enemies.push_back(newEnemy);
};

bool shouldRemoveTurrent(Turrent &turrent) {
	if (turrent.dead) {
		someSound = Mix_LoadWAV("Explosion.ogg");
		Mix_PlayChannel(-1, someSound, 0);
		return true;
	}
	else {
		return false;
	}
}

void UpdateEnemyTurrent(std::vector<Turrent> &enemies, Player &player, std::vector<Laser> &shots, float elapsed) {
	enemies.erase(std::remove_if(enemies.begin(), enemies.end(), shouldRemoveTurrent), enemies.end());

	for (int i = 0; i < enemies.size(); i++) {
		if (abs(player.position[0] - enemies[i].position[0]) + abs(player.position[1] - enemies[i].position[1]) <= 25.0) {
			TurrentTargetPlayer(enemies[i], player, shots);
		}
		else {
			TurrentTargetNeutral(enemies[i]);
		}
		enemies[i].Update(elapsed);
	}
}

void RenderEnemyTurrent(std::vector<Turrent> &enemies, ShaderProgram &p) {
	for (int i = 0; i < enemies.size(); i++) {
		enemies[i].Render(p);
	}
};

//TANK ENEMY

class Tank {
public:
	Tank(float &x, float &y) {
		position = { x, y, 0.0 };
		size = { 1.5, 1.5, 0.0 };
		velocity = 0.0f;
		acceleration = 0.0f;
		angle = 180.0f;
		turrentAngle = 0.0f;
		spriteBody = LoadTexture(RESOURCE_FOLDER"TankBody.png");
		spriteHead = LoadTexture(RESOURCE_FOLDER"Turrent.png");
		spriteHP = LoadTexture(RESOURCE_FOLDER"green_hp.png");
		dead = false;
		reload = 1.0f;
		hp = 4;
	}
	void Update(float elapsed) {
		float horizontal = sin(angle * (3.1415926f / 180.0f));
		float vertical = cos(angle * (3.1415926f / 180.0f));
		position[0] += (velocity * horizontal * elapsed);
		position[1] += (velocity * vertical * elapsed);
		if (turrentAngle > 360) {
			turrentAngle += -360;
		}
		if (turrentAngle < -360) {
			turrentAngle += 360;
		}
		reload -= elapsed;
	

		modelMatrixBody = glm::translate(modelMatrix, glm::vec3(position[0], position[1], position[2]));
		float bodyAngle = -angle * (3.1415926f / 180.0f);
		modelMatrixBody = glm::rotate(modelMatrixBody, bodyAngle, glm::vec3(0.0f, 0.0f, 1.0f));

		modelMatrixHead = glm::translate(modelMatrix, glm::vec3(position[0], position[1], position[2]));
		float headAngle = -turrentAngle * (3.1415926f / 180.0f);
		modelMatrixHead = glm::rotate(modelMatrixHead, headAngle, glm::vec3(0.0f, 0.0f, 1.0f));
	}
	void Render(ShaderProgram &p) {

		program.SetModelMatrix(modelMatrixBody);
		glBindTexture(GL_TEXTURE_2D, spriteBody);

		float VerticesBody[] = {
			(-size[0]), (-size[1]),
			(size[0]), (-size[1]),
			(size[0]), (size[1]),
			(-size[0]), (-size[1]),
			(size[0]), (size[1]),
			(-size[0]), (size[1])
		};

		glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, VerticesBody);
		glEnableVertexAttribArray(p.positionAttribute);

		float bodyTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(p.texCoordAttribute, 2, GL_FLOAT, false, 0, bodyTexCoords);
		glEnableVertexAttribArray(p.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(p.positionAttribute);
		glDisableVertexAttribArray(p.texCoordAttribute);

		//
		program.SetModelMatrix(modelMatrixHead);
		glBindTexture(GL_TEXTURE_2D, spriteHead);

		float VerticesHead[] = {
			(-size[0] * 2 / 3), (-size[1] * 2 / 3),
			(size[0] * 2 / 3), (-size[1] * 2 / 3),
			(size[0] * 2 / 3), (size[1] * 5 / 3),
			(-size[0] * 2 / 3), (-size[1] * 2 / 3),
			(size[0] * 2 / 3), (size[1] * 5 / 3),
			(-size[0] * 2 / 3), (size[1] * 5 / 3)
		};

		glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, VerticesHead);
		glEnableVertexAttribArray(p.positionAttribute);

		float headTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(p.texCoordAttribute, 2, GL_FLOAT, false, 0, headTexCoords);
		glEnableVertexAttribArray(p.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(p.positionAttribute);
		glDisableVertexAttribArray(p.texCoordAttribute);

		RenderHud(p);

	}
	void RenderHud(ShaderProgram &p) {
		//issue: speed up game
		hpMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(position[0], position[1] + size[1], 0.0f));
		program.SetModelMatrix(hpMatrix);


		glBindTexture(GL_TEXTURE_2D, spriteHP);
		


		float HP[] = {
			(-1.0), (-0.25),
			(hp * 0.5 -1.0), (-0.25),
			(hp * 0.5 - 1.0), (0.25),
			(-1.0), (-0.25),
			(hp * 0.5 - 1.0), (0.25),
			(-1.0), (0.25)
		};

		glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, HP);
		glEnableVertexAttribArray(p.positionAttribute);

		float HPTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(p.texCoordAttribute, 2, GL_FLOAT, false, 0, HPTexCoords);
		glEnableVertexAttribArray(p.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(p.positionAttribute);
		glDisableVertexAttribArray(p.texCoordAttribute);
	}


	bool CollidesWithLaser(Laser &laser) {
		float radius = sqrt((size[0] + laser.size[0]) * (size[1] + laser.size[1]));
		float x = position[0] - laser.position[0] ;
		float y = position[1] - laser.position[1];
		float distance = sqrt(pow(x , 2) + pow(y, 2));

		if (distance <= radius) {
			return true;
		}
		else {
			return false;
		}
	}

	bool CollidesWithPlayer(Player &player) {
		float radius1 = sqrt(size[0] *size[1]);
		float radius2 = sqrt(player.size[0] * player.size[1]);
		float x = position[0] - player.position[0];
		float y = position[1] - player.position[1];
		float distance = sqrt(pow(x, 2) + pow(y, 2));

		if (distance <= radius1 +radius2) {
			return true;
		}
		else {
			return false;
		}
	}
	float PlayerPen(Player &player) {
		float radius1 = sqrt(size[0] * size[1]);
		float radius2 = sqrt(size[0] * player.size[1]);
		float x = position[0] - player.position[0];
		float y = position[1] - player.position[1];
		float distance = sqrt(pow(x, 2) + pow(y, 2));

		float penetration = fabs(distance - radius1 - radius2);
		return penetration;
	}

	bool CollidesWithTank(Tank &tank) {
		float radius1 = sqrt(size[0] * size[1]);
		float radius2 = sqrt(tank.size[0] * tank.size[1]);
		float x = position[0] - tank.position[0];
		float y = position[1] - tank.position[1];
		float distance = sqrt(pow(x, 2) + pow(y, 2));

		if (distance <= radius1 + radius2) {
			return true;
		}
		else {
			return false;
		}
	}
	float TankPen(Tank &tank) {
		float radius1 = sqrt(size[0] * size[1]);
		float radius2 = sqrt(tank.size[0] * tank.size[1]);
		float x = position[0] - tank.position[0];
		float y = position[1] - tank.position[1];
		float distance = sqrt(pow(x, 2) + pow(y, 2));

		float penetration = fabs(distance - radius1 - radius2);
		return penetration;
	}
	bool CollidesWithTurrent(Turrent &turrent) {
		float radius1 = sqrt(size[0] * size[1]);
		float radius2 = sqrt(turrent.size[0] * turrent.size[1]);
		float x = position[0] - turrent.position[0];
		float y = position[1] - turrent.position[1];
		float distance = sqrt(pow(x, 2) + pow(y, 2));

		if (distance <= radius1 + radius2) {
			return true;
		}
		else {
			return false;
		}
	}
	float TurrentPen(Turrent &turrent) {
		float radius1 = sqrt(size[0] * size[1]);
		float radius2 = sqrt(turrent.size[0] * turrent.size[1]);
		float x = position[0] - turrent.position[0];
		float y = position[1] - turrent.position[1];
		float distance = sqrt(pow(x, 2) + pow(y, 2));

		float penetration = fabs(distance - radius1 - radius2);
		return penetration;
	}



	

	glm::mat4 modelMatrixBody;
	glm::mat4 modelMatrixHead;
	glm::mat4 hpMatrix;

	GLuint spriteBody;
	GLuint spriteHead;
	GLuint spriteHP;

	glm::vec3 position;
	glm::vec3 size;
	float velocity;
	float acceleration;
	float angle;
	float turrentAngle;
	bool dead;
	float reload;
	int hp;
};

void TankTurnToPlayer(Tank &e, Player &p) {
	float x = p.position[0] - e.position[0];
	float y = p.position[1] - e.position[1];
	float angle = -atan2(y, x) / (3.1415926f / 180.0f) + 90;
	if (e.angle != angle) {
		if (e.angle - angle > 180) {
			e.angle += -360;
		}
		if (e.angle - angle < -180) {
			e.angle += 360;
		}
		if (e.angle < angle) {
			e.angle += 2.0f;
		}
		else {
			e.angle -= 2.0f;
		}
		if (angle - 2 < e.angle && e.angle < angle + 2) {
			e.angle = angle;
		}
	}
}


void TankTargetNeutral(Tank &e) {
	float angle = e.angle;
	if (e.turrentAngle != angle) {
		if (e.turrentAngle - angle > 180) {
			e.turrentAngle += -360;
		}
		if (e.turrentAngle - angle < -180) {
			e.turrentAngle += 360;
		}
		if (e.turrentAngle < angle) {
			e.turrentAngle += 2.0f;
		}
		else {
			e.turrentAngle -= 2.0f;
		}
		if (angle - 2 < e.turrentAngle && e.turrentAngle < angle + 2) {
			e.turrentAngle = angle;
		}
	}
}
void TankshootLaser(Tank &e, std::vector<Laser> &shots) {
	someSound = Mix_LoadWAV("Fire2.ogg");
	Mix_PlayChannel(-1, someSound, 0);

	Laser newShot(e.position[0], e.position[1], e.turrentAngle);
	shots.push_back(newShot);
};

void TankTargetPlayer(Tank &e, Player &p, std::vector<Laser> &s) {
	float x = p.position[0] - e.position[0];
	float y = p.position[1] - e.position[1];
	float angle = -atan2(y, x) / (3.1415926f / 180.0f) + 90;
	if (e.turrentAngle != angle) {
		if (e.turrentAngle - angle > 180) {
			e.turrentAngle += -360;
		}
		if (e.turrentAngle - angle < -180) {
			e.turrentAngle += 360;
		}
		if (e.turrentAngle < angle) {
				e.turrentAngle += 2.0f;
		}
		else {
				e.turrentAngle -= 2.0f;
		}
	}
	if (angle - 2 < e.turrentAngle && e.turrentAngle < angle + 2) {
		//e.turrentAngle = angle;
		if (e.reload <= 0) {
			TankshootLaser(e, s);
			e.reload = 2.0f;
		}
	}
	//e.turrentAngle = angle;
};

void spawnNewEnemyTank(std::vector<Tank> &enemies, float x,float y) {
	Tank newEnemy(x, y);
	enemies.push_back(newEnemy);
};

bool shouldRemoveTank(Tank &tank) {
	if (tank.dead) {
		someSound = Mix_LoadWAV("Explosion.ogg");
		Mix_PlayChannel(-1, someSound, 0);
		return true;
	}
	else {
		return false;
	}
}

void UpdateEnemyTankAI(std::vector<Tank> &enemies, Player &player, std::vector<Laser> &shots, float elapsed) {
	enemies.erase(std::remove_if(enemies.begin(), enemies.end(), shouldRemoveTank), enemies.end());

	for (int i = 0; i < enemies.size(); i++) {
		enemies[i].velocity = 0;
		if (abs(player.position[0] - enemies[i].position[0]) + abs(player.position[1] - enemies[i].position[1]) >= 25.0) {
			TankTurnToPlayer(enemies[i], player);
		}
		if (abs(player.position[0] - enemies[i].position[0]) + abs(player.position[1] - enemies[i].position[1]) >= 17.0) {
			enemies[i].velocity = 5;
		}
		if (abs(player.position[0] - enemies[i].position[0]) + abs(player.position[1] - enemies[i].position[1]) <= 13.0) {
			TankTurnToPlayer(enemies[i], player);
			enemies[i].velocity = -5;
		}


		if (abs(player.position[0] - enemies[i].position[0]) + abs(player.position[1] - enemies[i].position[1]) <= 22.5) {
			TankTargetPlayer(enemies[i], player, shots);
		}
		else {
			TankTargetNeutral(enemies[i]);
		}
	}
}
void UpdateEnemyTankPos(std::vector<Tank> &enemies, float elapsed) {
	for (int i = 0; i < enemies.size(); i++) {
		enemies[i].Update(elapsed);
	}
}

void RenderEnemyTank(std::vector<Tank> &enemies, ShaderProgram &p) {
	for (int i = 0; i < enemies.size(); i++) {
		enemies[i].Render(p);
	}
};

//Wall
class Wall {
public:
	Wall(float &x, float &y) {
		position = { x, y, 0.0 };
		size = { 2.5, 2.5, 0.0 };
		sprite = LoadTexture(RESOURCE_FOLDER"metalPanel.png");


	}
	void Update(float &elapsed) {
		WallMatrix = glm::translate(modelMatrix, glm::vec3(position[0], position[1], position[2]));
	}
	void Render(ShaderProgram &p) {

		program.SetModelMatrix(WallMatrix);
		glBindTexture(GL_TEXTURE_2D, sprite);

		float VerticesBody[] = {
			(-size[0]), (-size[1]),
			(size[0]), (-size[1]),
			(size[0]), (size[1]),
			(-size[0]), (-size[1]),
			(size[0]), (size[1]),
			(-size[0]), (size[1])
		};

		glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, VerticesBody);
		glEnableVertexAttribArray(p.positionAttribute);

		float bodyTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(p.texCoordAttribute, 2, GL_FLOAT, false, 0, bodyTexCoords);
		glEnableVertexAttribArray(p.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(p.positionAttribute);
		glDisableVertexAttribArray(p.texCoordAttribute);
	}
	bool CollidesWithLaser(Laser &laser) {
		float px = abs(position[0] - laser.position[0]);
		px -= (size[0] + laser.size[0]);
		float py = abs(position[1] - laser.position[1]);
		py -= (size[1] + laser.size[1]);
		if (px <= 0 && py <= 0) {
			return true;
		}
		else {
			return false;
		}
	}
	bool CollidesWithPlayer(Player &player) {
		float radius = sqrt(player.size[0] * player.size[1]);
		float px = abs(position[0] - player.position[0]);
		px -= (size[0] + radius);
		float py = abs(position[1] - player.position[1]);
		py -= (size[1] + radius);
		if (px <= 0 && py <= 0) {
			return true;
		}
		else {
			return false;
		}
	}
	float PlayerPen(Player &player) {
		//float radius = sqrt(pow(player.size[0], 2) + pow(player.size[1], 2));
		float radius = sqrt(player.size[0] * player.size[1]);
		float x = position[0] - player.position[0];
		float y = position[1] - player.position[1];
		float distance = sqrt(pow(x, 2) + pow(y, 2));

		float penetration = fabs(distance - radius - size[0]);
		return penetration;
	}

	bool CollidesWithTank(Tank &tank) {
		float radius = sqrt(tank.size[0] * tank.size[1]);
		float px = abs(position[0] - tank.position[0]);
		px -= (size[0] + radius);
		float py = abs(position[1] - tank.position[1]);
		py -= (size[1] + radius);
		if (px <= 0 && py <= 0) {
			return true;
		}
		else {
			return false;
		}
	}
	float TankPen(Tank &tank) {
		//float radius = sqrt(pow(player.size[0], 2) + pow(player.size[1], 2));
		float radius = sqrt(tank.size[0] * tank.size[1]);
		float x = position[0] - tank.position[0];
		float y = position[1] - tank.position[1];
		float distance = sqrt(pow(x, 2) + pow(y, 2));

		float penetration = fabs(distance - radius - size[0]);
		return penetration;
	}


	glm::mat4 WallMatrix;
	GLuint sprite;

	glm::vec3 position;
	glm::vec3 size;

};

void spawnNewWall(std::vector<Wall> &wall, float x, float y) {
	Wall newWall(x, y);
	wall.push_back(newWall);
};

void UpdateWall(std::vector<Wall> &wall, float &elapsed) {
	for (int i = 0; i < wall.size(); i++) {
		wall[i].Update(elapsed);
	}
}

void RenderWall(std::vector<Wall> &wall, ShaderProgram &p) {
	for (int i = 0; i < wall.size(); i++) {
		wall[i].Render(p);
	}
};

//

void shootLaser(Player &p, std::vector<Laser> &shots) {
	someSound = Mix_LoadWAV("Fire.ogg");
	Mix_PlayChannel(-1, someSound, 0);

	Laser newShot(p.position[0], p.position[1], p.turrentAngle);
	shots.push_back(newShot);
};

bool shouldRemoveLaser(Laser &laser) {
	if (laser.dead) {
		return true;
	}
	if (laser.timeAlive > 1.5) {
		return true;
	}
	else {
		return false;
	}
}

void UpdateLaser(std::vector<Laser> &shots, float elapsed) {
	shots.erase(std::remove_if(shots.begin(), shots.end(), shouldRemoveLaser), shots.end());

	for (int i = 0; i < shots.size(); i++) {
		shots[i].Update(elapsed);
	}
};
void RenderLaser(std::vector<Laser> &shots, ShaderProgram &p) {
	for (int i = 0; i < shots.size(); i++) {
		shots[i].Render(p);
	}
};

void ProcessInput(Player &player, std::vector<Laser> &shots) {
	//Body Control
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	player.velocity = 0;
	
	if (keys[SDL_SCANCODE_SPACE]) {
		if (player.reload <= 0) {
			shootLaser(player, shots);
			player.reload = 1.0f;
		}
	}
	if (keys[SDL_SCANCODE_W]) {
		player.velocity = 7.5;
	}
	if (keys[SDL_SCANCODE_S]) {
		player.velocity = -5 ;
	}

	if (keys[SDL_SCANCODE_A]) {
		player.angle += -2.f;	
		player.turrentAngle += -2.f;
	}
	if (keys[SDL_SCANCODE_D]) {
		player.angle += 2.f;
		player.turrentAngle += 2.f;
	}
	//Turrent Control
	if (keys[SDL_SCANCODE_Q]) {
		player.turrentAngle += -2.f;
	}
	if (keys[SDL_SCANCODE_E]) {
		player.turrentAngle += 2.f;
	}


	if (player.angle >= 180.0f) {
		player.angle = -180.0f;
	}
	if (player.angle < -180.0f) {
		player.angle = 180.0f;
	}

	if (player.turrentAngle >= 180.0f) {
		player.turrentAngle = -180.0f;
	}
	if (player.turrentAngle < -180.0f) {
		player.turrentAngle = 180.0f;
	}
}
void ProcessMenu() {
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_SPACE]) {
		// DO AN ACTION WHEN SPACE IS PRESSED!
		mode = STATE_MENU1;
	}
}

void RenderMenu() {
	program.SetViewMatrix(glm::mat4(1.0f));

	glm::mat4 textMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-5.0f, 15.0f, 1.0f));
	program.SetModelMatrix(textMatrix);
	std::string text = "Controls:";
	DrawText(program, text, 2.0, -1.0);

	textMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-18.5f, 5.0f, 1.0f));
	program.SetModelMatrix(textMatrix);
	text = "WASD to Move.";
	DrawText(program, text, 2.0, -.5);

	textMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-18.5f, 0.0f, 1.0f));
	program.SetModelMatrix(textMatrix);
	text = "Q&E to Rotate Turrent.";
	DrawText(program, text, 2.0, -.5);

	textMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-18.5f, -5.0f, 1.0f));
	program.SetModelMatrix(textMatrix);
	text = "Space to Shoot.";
	DrawText(program, text, 2.0, -.5);

	textMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-18.5f, -10.0f, 1.0f));
	program.SetModelMatrix(textMatrix);
	text = "ESC to Quit.";
	DrawText(program, text, 2.0, -.5);

	textMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-10.0f, -15.0f, 1.0f));
	program.SetModelMatrix(textMatrix);
	text = "Hold Space to Start";
	DrawText(program, text, 2.0, -1.0);
}

void ProcessMenu1() {
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_RETURN]) {
		// DO AN ACTION WHEN SPACE IS PRESSED!
		mode = STATE_PREP1;
	}
}

void RenderMenu1() {
	program.SetViewMatrix(glm::mat4(1.0f));

	glm::mat4 textMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-5.0f, 15.0f, 1.0f));
	program.SetModelMatrix(textMatrix);
	std::string text = "LEVEL 1:";
	DrawText(program, text, 2.0, -1.0);

	textMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-18.5f, -5.0f, 1.0f));
	program.SetModelMatrix(textMatrix);
	text = "Objective: Eliminate All Enemy.";
	DrawText(program, text, 2.0, -1.0);

	textMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-18.5f, 10.0f, 1.0f));
	program.SetModelMatrix(textMatrix);
	text = "Hostile Base Located";
	DrawText(program, text, 2.0, -1.0);

	textMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-18.5f, 5.0f, 1.0f));
	program.SetModelMatrix(textMatrix);
	text = "Destroy All Fortifications";
	DrawText(program, text, 2.0, -1.0);


	textMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-10.0f, -15.0f, 1.0f));
	program.SetModelMatrix(textMatrix);
	text = "Hold Enter to Start";
	DrawText(program, text, 2.0, -1.0);
}

void ProcessMenu2() {
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_RETURN]) {
		// DO AN ACTION WHEN SPACE IS PRESSED!
		mode = STATE_PREP2;
	}
}

void RenderMenu2() {
	program.SetViewMatrix(glm::mat4(1.0f));

	glm::mat4 textMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-5.0f, 15.0f, 1.0f));
	program.SetModelMatrix(textMatrix);
	std::string text = "LEVEL 2:";
	DrawText(program, text, 2.0, -1.0);

	textMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-18.5f, -5.0f, 1.0f));
	program.SetModelMatrix(textMatrix);
	text = "Objective: Eliminate All Enemy.";
	DrawText(program, text, 2.0, -1.0);

	textMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-18.5f, 10.0f, 1.0f));
	program.SetModelMatrix(textMatrix);
	text = "Enemy Ambush Detected";
	DrawText(program, text, 2.0, -1.0);

	textMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-18.5f, 5.0f, 1.0f));
	program.SetModelMatrix(textMatrix);
	text = "Search And Destroy";
	DrawText(program, text, 2.0, -1.0);

	textMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-10.0f, -15.0f, 1.0f));
	program.SetModelMatrix(textMatrix);
	text = "Hold Enter to Start";
	DrawText(program, text, 2.0, -1.0);
}

void ProcessMenu3() {
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_RETURN]) {
		// DO AN ACTION WHEN SPACE IS PRESSED!
		mode = STATE_PREP3;
	}
}

void RenderMenu3() {
	program.SetViewMatrix(glm::mat4(1.0f));

	glm::mat4 textMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-5.0f, 15.0f, 1.0f));
	program.SetModelMatrix(textMatrix);
	std::string text = "LEVEL 3:";
	DrawText(program, text, 2.0, -1.0);

	textMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-18.5f, -5.0f, 1.0f));
	program.SetModelMatrix(textMatrix);
	text = "Objective: SURVIVE";
	DrawText(program, text, 2.0, -1.0);

	textMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-18.5f, 10.0f, 1.0f));
	program.SetModelMatrix(textMatrix);
	text = "Warning";
	DrawText(program, text, 2.0, -1.0);

	textMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-18.5f, 5.0f, 1.0f));
	program.SetModelMatrix(textMatrix);
	text = "Multiple Hostiles Inbound";
	DrawText(program, text, 2.0, -1.0);

	textMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-10.0f, -15.0f, 1.0f));
	program.SetModelMatrix(textMatrix);
	text = "Hold Enter to Start";
	DrawText(program, text, 2.0, -1.0);
}

void RenderVictory() {
	program.SetViewMatrix(glm::mat4(1.0f));

	glm::mat4 textMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-8.0f, 0.0f, 1.0f));
	program.SetModelMatrix(textMatrix);
	std::string text = "You Win";
	DrawText(program, text, 3.0, 0);

	textMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-7.5f, -5.0f, 1.0f));
	program.SetModelMatrix(textMatrix);
	text = "Hold Enter to Exit";
	DrawText(program, text, 2.0, -1.0);
}

void RenderGameOver() {
	program.SetViewMatrix(glm::mat4(1.0f));

	glm::mat4 textMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-11.0f, 0.0f, 1.0f));
	program.SetModelMatrix(textMatrix);
	std::string text = "Game Over";
	DrawText(program, text, 3.0, 0);

	textMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-7.5f, -5.0f, 1.0f));
	program.SetModelMatrix(textMatrix);
	text = "Hold Enter to Exit";
	DrawText(program, text, 2.0, -1.0);
}

class Level {
public:
	Level() {
	
	}
	void LaserHitDetection() {
		//check collisions
		for (int i = 0; i < shots.size(); i++) {
			for (int w = 0; w < walls.size(); w++) {
				if (walls[w].CollidesWithLaser(shots[i])) {
					shots[i].dead = true;
					
				}
			}
			for (int e1 = 0; e1 < tanks.size(); e1++) {
				if (tanks[e1].CollidesWithLaser(shots[i])) {
					if (!shots[i].dead) {
						shots[i].dead = true;
						someSound = Mix_LoadWAV("Blow.ogg");
						Mix_PlayChannel(-1, someSound, 0);

						if (tanks[e1].angle - 35 <= shots[i].angle && shots[i].angle <= tanks[e1].angle + 35) {
							tanks[e1].hp -= 2;
						}
						else {
							tanks[e1].hp -= 1;
						}
						if (tanks[e1].hp <= 0) {
							tanks[e1].dead = true;
							spawnExplosion(explode, tanks[e1].position[0], tanks[e1].position[1]);
						}
					}
				}
			}
			for (int e2 = 0; e2 < turrents.size(); e2++) {
				if (turrents[e2].CollidesWithLaser(shots[i])) {
					if (!shots[i].dead) {
						shots[i].dead = true;
						someSound = Mix_LoadWAV("Blow.ogg");
						Mix_PlayChannel(-1, someSound, 0);

						turrents[e2].hp -= 1;
					}
					if (turrents[e2].hp <= 0) {
						turrents[e2].dead = true;
						spawnExplosion(explode, turrents[e2].position[0], turrents[e2].position[1]);
					}
				}
			}
		}
		for (int i = 0; i < eshots.size(); i++) {
			for (int w = 0; w < walls.size(); w++) {
				if (walls[w].CollidesWithLaser(eshots[i])) {
					eshots[i].dead = true;
				}
			}
		}
	}

	void TankHitDetection() {
		for (int e1 = 0; e1 < tanks.size(); e1++) {
			for (int e2 = e1; e2 < tanks.size(); e2++) {
				if (tanks[e2].CollidesWithTank(tanks[e1])) {
					float pen = -tanks[e2].TankPen(tanks[e1]) - 0.2f;

					float x = tanks[e1].position[0] - tanks[e2].position[0];
					float y = tanks[e1].position[1] - tanks[e2].position[1];
					float angle = -atan2(y, x) / (3.1415926f / 180.0f) + 270;

					float horizontal = sin(angle * (3.1415926f / 180.0f));
					float vertical = cos(angle * (3.1415926f / 180.0f));
					tanks[e1].position[0] += (pen * horizontal * elapsed);
					tanks[e1].position[1] += (pen * vertical * elapsed);

					tanks[e2].position[0] += (-pen * horizontal * elapsed);
					tanks[e2].position[1] += (-pen * vertical * elapsed);

				}
			}

			for (int t = 0; t < turrents.size(); t++) {
				if (tanks[e1].CollidesWithTurrent(turrents[t])) {
					tanks[e1].velocity = 0;
					float pen = -tanks[e1].TurrentPen(turrents[t]) - 0.2f;

					float x = tanks[e1].position[0] - turrents[t].position[0];
					float y = tanks[e1].position[1] - turrents[t].position[1];
					float angle = -atan2(y, x) / (3.1415926f / 180.0f) + 270;

					float horizontal = sin(angle * (3.1415926f / 180.0f));
					float vertical = cos(angle * (3.1415926f / 180.0f));
					tanks[e1].position[0] += (pen * horizontal * elapsed);
					tanks[e1].position[1] += (pen * vertical * elapsed);
				}
			}

			for (int w = 0; w < walls.size(); w++) {
				if (walls[w].CollidesWithTank(tanks[e1])) {
					tanks[e1].velocity = 0;
					float pen = -walls[w].TankPen(tanks[e1]) - 0.2f;

					float x = tanks[e1].position[0] - walls[w].position[0];
					float y = tanks[e1].position[1] - walls[w].position[1];
					float angle = -atan2(y, x) / (3.1415926f / 180.0f) + 270;

					float horizontal = sin(angle * (3.1415926f / 180.0f));
					float vertical = cos(angle * (3.1415926f / 180.0f));
					tanks[e1].position[0] += (pen * horizontal * elapsed);
					tanks[e1].position[1] += (pen * vertical * elapsed);
				}
			}
		}
	}

	void PlayerHitDetection(Player &player) {
		for (int w = 0; w < walls.size(); w++) {
			if (walls[w].CollidesWithPlayer(player)) {
				player.velocity = 0;
				float pen = -walls[w].PlayerPen(player) - 0.2f;

				float x = player.position[0] - walls[w].position[0];
				float y = player.position[1] - walls[w].position[1];
				float angle = -atan2(y, x) / (3.1415926f / 180.0f) + 270;

				float horizontal = sin(angle * (3.1415926f / 180.0f));
				float vertical = cos(angle * (3.1415926f / 180.0f));
				player.position[0] += (pen * horizontal * elapsed);
				player.position[1] += (pen * vertical * elapsed);
			}
		}
		for (int e1 = 0; e1 < tanks.size(); e1++) {
			if (tanks[e1].CollidesWithPlayer(player)) {
				player.velocity = 0;
				tanks[e1].velocity = 0;

				float pen = -tanks[e1].PlayerPen(player) - 0.2f;

				float x = player.position[0] - tanks[e1].position[0];
				float y = player.position[1] - tanks[e1].position[1];
				float angle = -atan2(y, x) / (3.1415926f / 180.0f) + 270;

				float horizontal = sin(angle * (3.1415926f / 180.0f));
				float vertical = cos(angle * (3.1415926f / 180.0f));
				player.position[0] += (pen * horizontal * elapsed);
				player.position[1] += (pen * vertical * elapsed);
			}
		}
		for (int e2 = 0; e2 < turrents.size(); e2++) {
			if (turrents[e2].CollidesWithPlayer(player)) {
				player.velocity = 0;
				float pen = -turrents[e2].PlayerPen(player) - 0.2f;

				float x = player.position[0] - turrents[e2].position[0];
				float y = player.position[1] - turrents[e2].position[1];
				float angle = -atan2(y, x) / (3.1415926f / 180.0f) + 270;

				float horizontal = sin(angle * (3.1415926f / 180.0f));
				float vertical = cos(angle * (3.1415926f / 180.0f));
				player.position[0] += (pen * horizontal * elapsed);
				player.position[1] += (pen * vertical * elapsed);
			}
		}
		
	}
	void ELaserHitDetection(Player &player) {
		//check collisions
		for (int i = 0; i < eshots.size(); i++) {
			if (player.CollidesWithLaser(eshots[i])) {
				eshots[i].dead = true;

				//hit Rear
				someSound = Mix_LoadWAV("Hammer.ogg");
				Mix_PlayChannel(-1, someSound, 0);

				if (player.angle - 35 <= eshots[i].angle && eshots[i].angle <= player.angle + 35) {
					player.hp -= 10;
				}
				else {
					player.hp -= 5;
				}
			}
		}
	}
	void Update(float &elapsed, Player &player) {
		UpdateExplosion(explode, elapsed);
		UpdateWall(walls, elapsed);
		LaserHitDetection();
		ELaserHitDetection(player);
		UpdateLaser(shots, elapsed);
		UpdateLaser(eshots, elapsed);

		
		UpdateEnemyTankAI(tanks, player, eshots, elapsed);
		TankHitDetection();
		UpdateEnemyTankPos(tanks, elapsed);
		TankHitDetection();

		UpdateEnemyTurrent(turrents, player, eshots, elapsed);
	}
	void Render(ShaderProgram &p) {
		RenderTerrain(terrain, p);
		RenderExplosion(explode, p);
		RenderWall(walls, p);
		RenderLaser(shots, p);
		RenderLaser(eshots, p);
		RenderEnemyTank(tanks, p);
		RenderEnemyTurrent(turrents, p);
	}

	std::vector<Terrain> terrain;
	std::vector<Explosion> explode;
	std::vector<Wall> walls;
	std::vector<Tank> tanks;
	std::vector<Turrent> turrents;
	std::vector<Laser> shots;
	std::vector<Laser> eshots;
};

//Maps:
#define MAP1_HEIGHT 19
#define MAP1_WIDTH 15 
unsigned int Map1Data[MAP1_HEIGHT][MAP1_WIDTH] =
{
	//each grid is 5 space
	// 0 = Null, 1 = Wall, 5 = turrents, 6 = tanks, 9 = player

	//Bottom
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
	{1,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,1,1,0,0,0,1,1,1,0,0,0,1,1,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,1,5,0,0,0,0,5,0,0,0,0,5,1,1},
	{1,1,1,1,0,0,1,1,1,0,0,1,1,1,1},
	{1,5,0,0,0,0,1,5,1,0,0,0,0,5,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,5,0,0,0,0,0,0,0,0,0,0,0,5,1},
	{1,1,0,0,1,1,0,0,0,1,1,0,0,1,1},
	{1,5,0,0,0,0,0,0,0,0,0,0,0,5,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,5,0,0,0,5,1,5,1,5,0,0,0,5,1},
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
	//Top
};

void CreateMap1(Player &player, Level &level) {
	for (int y = 0; y < MAP1_HEIGHT; y++) {
		for (int x = 0; x < MAP1_WIDTH; x++) {

			InsertTerrain(level.terrain, x * 5, y * 5);

			if (Map1Data[y][x] != 0) {
				if (Map1Data[y][x] == 1) {
					spawnNewWall(level.walls, x * 5, y * 5);
				}
				if (Map1Data[y][x] == 5) {
					spawnNewEnemyTurrent(level.turrents, x * 5, y * 5);
				}
				if (Map1Data[y][x] == 6) {
					spawnNewEnemyTank(level.tanks, x * 5, y * 5);
				}
				if (Map1Data[y][x] == 9) {
					player.position[0] = x * 5;
					player.position[1] = y * 5;
				}
			}
		}
	}
}

//LV2
#define MAP2_HEIGHT 15
#define MAP2_WIDTH 15 
unsigned int Map2Data[MAP2_HEIGHT][MAP2_WIDTH] =
{
	//each grid is 5 space
	// 0 = Null, 1 = Wall, 5 = turrents, 6 = tanks, 9 = player

	//Bottom
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
	{1,0,6,0,0,0,0,0,0,0,0,0,6,0,1},
	{1,6,0,0,0,0,0,0,0,0,0,0,0,6,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,1,0,0,0,1,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,1,0,0,0,1,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,6,0,0,0,0,0,0,0,0,0,0,0,6,1},
	{1,0,6,0,0,0,0,0,0,0,0,0,6,0,1},
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
	//Top
};

void CreateMap2(Player &player, Level &level) {
	for (int y = 0; y < MAP2_HEIGHT; y++) {
		for (int x = 0; x < MAP2_WIDTH; x++) {

			InsertTerrain(level.terrain, x * 5, y * 5);

			if (Map2Data[y][x] != 0) {
				if (Map2Data[y][x] == 1) {
					spawnNewWall(level.walls, x * 5, y * 5);
				}
				if (Map2Data[y][x] == 5) {
					spawnNewEnemyTurrent(level.turrents, x * 5, y * 5);
				}
				if (Map2Data[y][x] == 6) {
					spawnNewEnemyTank(level.tanks, x * 5, y * 5);
				}
				if (Map2Data[y][x] == 9) {
					player.position[0] = x * 5;
					player.position[1] = y * 5;
				}
			}
		}
	}
}

//LV3
#define MAP3_HEIGHT 21
#define MAP3_WIDTH 21 
unsigned int Map3Data[MAP3_HEIGHT][MAP3_WIDTH] =
{
	//each grid is 5 space
	// 0 = Null, 1 = Wall, 5 = turrents, 6 = tanks, 9 = player

	//Bottom
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
	{1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1},
	{1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1},
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
	//Top
};

void CreateMap3(Player &player, Level &level) {
	for (int y = 0; y < MAP3_HEIGHT; y++) {
		for (int x = 0; x < MAP3_WIDTH; x++) {

			InsertTerrain(level.terrain, x * 5, y * 5);

			if (Map3Data[y][x] != 0) {
				if (Map3Data[y][x] == 1) {
					spawnNewWall(level.walls, x * 5, y * 5);
				}
				if (Map3Data[y][x] == 5) {
					spawnNewEnemyTurrent(level.turrents, x * 5, y * 5);
				}
				if (Map3Data[y][x] == 6) {
					spawnNewEnemyTank(level.tanks, x * 5, y * 5);
				}
				if (Map3Data[y][x] == 9) {
					player.position[0] = x * 5;
					player.position[1] = y * 5;
				}
			}
		}
	}
}



void Setup() {
	//Setup
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
#ifdef _WINDOWS
	glewInit();
#endif

	glViewport(0, 0, 640, 360);

	
	program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	projectionMatrix = glm::mat4(1.0f);
	viewMatrix = glm::mat4(1.0f);
	modelMatrix = glm::mat4(1.0f);
	

	projectionMatrix = glm::ortho(-20.f, 20.f, -20.f, 20.0f, -1.0f, 1.0f);

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
	mode = STATE_MENU;


	Player player;
	Level level1;
	Level level2;
	Level level3;
	int wave;


	Mix_Music *music;
	music = Mix_LoadMUS("Scene2.ogg");
	Mix_PlayMusic(music, -1);

    SDL_Event event;
    bool done = false;
    while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.type == SDL_KEYDOWN) {
				if (mode == STATE_GAME_OVER || mode == STATE_VICTORY) {
					if (event.key.keysym.scancode == SDL_SCANCODE_RETURN) {
						done = true;
					}
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
					// DO AN ACTION WHEN ESC IS PRESSED!
					done = true;
				}
			}
		}

		

		// get elapsed time
		ticks = (float)SDL_GetTicks() / 1000.0f;
		elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		elapsed += accumulator;
		if (elapsed < FIXED_TIMESTEP) {
			accumulator = elapsed;
			continue;
		}
		
		

		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		if (player.hp <= 0 && mode != STATE_GAME_OVER) {
			someSound = Mix_LoadWAV("Explosion.ogg");
			Mix_PlayChannel(-1, someSound, 0);

			Mix_HaltMusic();
			someSound = Mix_LoadWAV("Gameover2.ogg");
			Mix_PlayChannel(-1, someSound, 0);
			mode = STATE_GAME_OVER;
		}

		switch (mode) {
		case STATE_MENU:
			ProcessMenu();
			RenderMenu();
			break;
		case STATE_MENU1:
			ProcessMenu1();
			RenderMenu1();
			break;
		case STATE_PREP1:
			music = Mix_LoadMUS("Battle4.ogg");
			Mix_PlayMusic(music, -1);

			CreateMap1(player, level1);
			player.angle = 0;
			player.turrentAngle = 0;
			player.reload = 1;
			mode = STATE_PLAY_LV1;
			break;
		case STATE_PLAY_LV1:
			if (level1.turrents.empty() && level1.tanks.empty()) {
				Mix_HaltMusic();
				music = Mix_LoadMUS("Scene2.ogg");
				Mix_PlayMusic(music, -1);

				mode = STATE_MENU2;
			}
			ProcessInput(player, level1.shots);
			while (elapsed >= FIXED_TIMESTEP) {
				level1.PlayerHitDetection(player);
				player.Update(elapsed);
				level1.Update(elapsed, player);
				elapsed -= FIXED_TIMESTEP;
			}
			accumulator = elapsed;
			level1.Render(program);
			player.Render(program);
			program.SetModelMatrix(modelMatrix);
			break;
		case STATE_MENU2:
			ProcessMenu2();
			RenderMenu2();
			break;
		case STATE_PREP2:
			music = Mix_LoadMUS("Battle8.ogg");
			Mix_PlayMusic(music, -1);

			CreateMap2(player, level2);
			if (player.hp < 100) {
				player.hp += 25;
				if (player.hp > 100) {
					player.hp = 100;
				}
			}
			player.angle = 0;
			player.turrentAngle = 0;
			player.reload = 1;
			mode = STATE_PLAY_LV2;
			break;
		case STATE_PLAY_LV2:
			if (level2.turrents.empty() && level2.tanks.empty()) {
				Mix_HaltMusic();
				music = Mix_LoadMUS("Scene2.ogg");
				Mix_PlayMusic(music, -1);

				mode = STATE_MENU3;
			}
			ProcessInput(player, level2.shots);
			while (elapsed >= FIXED_TIMESTEP) {
				level2.PlayerHitDetection(player);
				player.Update(elapsed);
				level2.Update(elapsed, player);
				elapsed -= FIXED_TIMESTEP;
			}
			accumulator = elapsed;
			level2.Render(program);
			player.Render(program);
			program.SetModelMatrix(modelMatrix);
			break;
		case STATE_MENU3:
			ProcessMenu3();
			RenderMenu3();
			break;
		case STATE_PREP3:
			music = Mix_LoadMUS("Battle7.ogg");
			Mix_PlayMusic(music, -1);
			wave = 0;

			CreateMap3(player, level3);

			if (player.hp < 100) {
				player.hp += 25;
				if (player.hp > 100) {
					player.hp = 100;
				}
			}
			player.angle = 0;
			player.turrentAngle = 0;
			player.reload = 1;
			mode = STATE_PLAY_LV3;
			break;
		case STATE_PLAY_LV3:
			if (wave == 10 && level3.turrents.empty() && level3.tanks.empty()) {
				Mix_HaltMusic();
				someSound = Mix_LoadWAV("Victory.ogg");
				Mix_PlayChannel(-1, someSound, 0);
				mode = STATE_VICTORY;
			}
			if (level3.turrents.empty() && level3.tanks.empty()) {
				wave += 1;

				if (wave == 1){
					spawnNewEnemyTank(level3.tanks, 45, 95);
					spawnNewEnemyTank(level3.tanks, 50, 95);
					spawnNewEnemyTank(level3.tanks, 55, 95);
					spawnNewEnemyTank(level3.tanks, 60, 95);
					spawnNewEnemyTank(level3.tanks, 65, 95);
				}
				else if (wave == 2) {
					spawnNewEnemyTank(level3.tanks, 45, 5);
					spawnNewEnemyTank(level3.tanks, 50, 5);
					spawnNewEnemyTank(level3.tanks, 55, 5);
					spawnNewEnemyTank(level3.tanks, 60, 5);
					spawnNewEnemyTank(level3.tanks, 65, 5);
				}
				else if (wave == 3) {
					spawnNewEnemyTank(level3.tanks, 5, 45);
					spawnNewEnemyTank(level3.tanks, 5, 50);
					spawnNewEnemyTank(level3.tanks, 5, 55);
					spawnNewEnemyTank(level3.tanks, 5, 60);
					spawnNewEnemyTank(level3.tanks, 5, 65);
				}
				else if (wave == 4) {
					spawnNewEnemyTank(level3.tanks, 95, 45);
					spawnNewEnemyTank(level3.tanks, 95, 50);
					spawnNewEnemyTank(level3.tanks, 95, 55);
					spawnNewEnemyTank(level3.tanks, 95, 60);
					spawnNewEnemyTank(level3.tanks, 95, 65);
				}
				else if (wave == 5) {
					wave = 10;

					spawnNewEnemyTank(level3.tanks, 50, 95);
					spawnNewEnemyTank(level3.tanks, 60, 95);

					spawnNewEnemyTank(level3.tanks, 50, 5);
					spawnNewEnemyTank(level3.tanks, 60, 5);

					spawnNewEnemyTank(level3.tanks, 5, 50);
					spawnNewEnemyTank(level3.tanks, 5, 60);

					spawnNewEnemyTank(level3.tanks, 95, 50);
					spawnNewEnemyTank(level3.tanks, 96, 60);
				}
			}
			ProcessInput(player, level3.shots);
			while (elapsed >= FIXED_TIMESTEP) {



				level3.PlayerHitDetection(player);
				player.Update(elapsed);
				level3.Update(elapsed, player);
				elapsed -= FIXED_TIMESTEP;
			}
			accumulator = elapsed;
			level3.Render(program);
			player.Render(program);
			program.SetModelMatrix(modelMatrix);
			break;
		case STATE_VICTORY:
			RenderVictory();
			break;
		case STATE_GAME_OVER:
			RenderGameOver();
			break;
		}

		SDL_GL_SwapWindow(displayWindow);

    }
    
	Mix_FreeChunk(someSound);
	Mix_FreeMusic(music);

    SDL_Quit();
    return 0;
}

