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

#include <cstdlib>
#include <ctime>

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif






SDL_Window* displayWindow;

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

int main(int argc, char *argv[]){

	//Setup
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
    glewInit();
#endif
	glViewport(0, 0, 640, 360);

	ShaderProgram program;
	program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	glm::mat4 projectionMatrix = glm::mat4(1.0f);
	glm::mat4 viewMatrix = glm::mat4(1.0f);
	glm::mat4 modelMatrix = glm::mat4(1.0f);

	projectionMatrix = glm::ortho(-10.777f, 10.777f, -10.0f, 10.0f, -1.0f, 1.0f);

	glUseProgram(program.programID);
	//Pre Setup End

	glm::mat4 LeftBoardMatrix = glm::mat4(1.0f);
	//LeftBoardMatrix = glm::scale(LeftBoardMatrix, glm::vec3(0.15f, 0.75f, 1.0f));

	glm::mat4 RightBoardMatrix = glm::mat4(1.0f);
	//RightBoardMatrix = glm::scale(RightBoardMatrix, glm::vec3(0.15f, 0.75f, 1.0f));

	glm::mat4 BallMatrix = glm::mat4(1.0f);
	//BallMatrix = glm::scale(BallMatrix, glm::vec3(0.25f, 0.25f, 1.0f));

	GLuint Board = LoadTexture(RESOURCE_FOLDER"elementStone020.png");
	GLuint Ball = LoadTexture(RESOURCE_FOLDER"ballBlue.png");
	//

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	float lastFrameTicks = 0.0f;

	float Left_x = -10.0;
	float Left_y = 0.0;
	float Right_x = 10.0;
	float Right_y = 0.0;
	float Ball_x = 0.0;
	float Ball_y = 0.0;
	float Ball_Dir_x = -0.0075;
	float Ball_Dir_y = 0.0033;

    SDL_Event event;
    bool done = false;
    while (!done) {
		//time
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

      
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}

		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	


		const Uint8 *keys = SDL_GetKeyboardState(NULL);
		if (keys[SDL_SCANCODE_UP]) {
			// go UP!
			if (Right_y <= (10 - 2.5)) {
				Right_y = Right_y + 0.03;
			}
			if (Left_y <= (10 - 2.5)) {
				Left_y = Left_y + 0.03;
			}
		}
		if (keys[SDL_SCANCODE_W]) {
			// go UP!
			if (Right_y <= (10 - 2.5)) {
				Right_y = Right_y + 0.03;
			}
			if (Left_y <= (10 - 2.5)) {
				Left_y = Left_y + 0.03;
			}
		}
		if (keys[SDL_SCANCODE_DOWN]) {
			// go down
			if (Right_y >= (-10 + 2.5)) {
				Right_y = Right_y - 0.03;
			}
			if (Left_y >= (-10 + 2.5)) {
				Left_y = Left_y - 0.03;
			}
		}
	
		if (keys[SDL_SCANCODE_S]) {
			// go UP!
			if (Right_y >= (-10 + 2.5)) {
				Right_y = Right_y - 0.03;
			}
			if (Left_y >= (-10 + 2.5)) {
				Left_y = Left_y - 0.03;
			}
		}
		
		Ball_x = (Ball_x + Ball_Dir_x);
		Ball_y = (Ball_y + Ball_Dir_y);

		if (Ball_x <= -10.777) {
			//Lose;
			done = true;
			
		}
		if (Ball_x >= 10.777) {
			//Lose;

			done = true;
		}
		if (Ball_y <= -10) {
			Ball_Dir_y = (Ball_Dir_y * -1);
		}
		if (Ball_y >= 10) {
			Ball_Dir_y = (Ball_Dir_y * -1);
		}


		if (Ball_x <= Left_x + 1 && Ball_y >= Left_y -2.5 && Ball_y <= Left_y + 2.5) {
			Ball_Dir_x = 0.01;

			//Ball_Dir_x = (Ball_Dir_x * -1);
			//Ball_Dir_y = (Ball_Dir_y * -1 );
			

		}
		if (Ball_x >= Right_x - 1 && Ball_y >= Right_y - 2.5 && Ball_y <= Right_y + 2.5) {
			Ball_Dir_x = -0.01;
			
			//Ball_Dir_x = (Ball_Dir_x * -1);
			//Ball_Dir_y = (Ball_Dir_y * -1);


		}

		//Render
	
		glBindTexture(GL_TEXTURE_2D, Board);
		program.SetProjectionMatrix(projectionMatrix);
		program.SetViewMatrix(viewMatrix);

		//Right Board
		program.SetModelMatrix(RightBoardMatrix);

		float RightVertices[] = { (-0.5 + Right_x), (-2.5 + Right_y), (0.5 + Right_x), (-2.5 + Right_y), (0.5 + Right_x), (2.5 + Right_y), (-0.5 + Right_x), (-2.5 + Right_y), (0.5 + Right_x), (2.5 + Right_y), (-0.5 + Right_x), (2.5 + Right_y) };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, RightVertices);
		glEnableVertexAttribArray(program.positionAttribute);


		float RightCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, RightCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);
		
		//Left Board
		program.SetModelMatrix(LeftBoardMatrix);

		float LeftVertices[] = { (-0.5 + Left_x), (-2.5 + Left_y), (0.5 + Left_x), (-2.5 + Left_y), (0.5 + Left_x), (2.5 + Left_y), (-0.5 + Left_x), (-2.5 + Left_y), (0.5 + Left_x), (2.5 + Left_y), (-0.5 + Left_x), (2.5 + Left_y) };
		
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, LeftVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float LeftCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, LeftCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		//
		//Ball
		glBindTexture(GL_TEXTURE_2D, Ball);
		program.SetModelMatrix(BallMatrix);

		float BallVertices[] = { (-0.5 + Ball_x), (-0.5 + Ball_y), (0.5 + Ball_x), (-0.5 + Ball_y), (0.5 + Ball_x), (0.5 + Ball_y), (-0.5 + Ball_x), (-0.5 + Ball_y), (0.5 + Ball_x), (0.5 + Ball_y), (-0.5 + Ball_x), (0.5 + Ball_y) };

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, BallVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float BallCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, BallCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);
		

		SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}
