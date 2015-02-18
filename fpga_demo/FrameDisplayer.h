#ifndef FRAME_DISPLAYER_PUBLIC
#define FRAME_DISPLAYER_PUBLIC

#include "Public.h"
#include "FramePool.h"
#include <GL/glut.h>

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
class FrameDisplayer
{
public:
	FrameDisplayer(char title[], int w, int h, FramePool *bgPool, 
		FramePool *fgPool, void (*userExit)(void), void (*toggleRecord)(void));
	~FrameDisplayer(){};
	bool init();
	void start();
	void stop();

	FramePool *_bgPool;									// FramePool for background images
	FramePool *_fgPool;									// FramePool for foreground images
	void (*_userExit)(void);							// Function pointer, called when user quits
	void (*_toggleRecord)(void);                        // Function pointer for starting/stopping video recording
	char *_title;										// GLUT window title
	GLint _window;										// GLUT window
	GLuint textureId0;									// GLUT texture id
	GLuint textureId1;									// GLUT texture id
	int _lastBgFrameNum;								// Last background image number
	int _lastFgFrameNum;								// Last foreground image number
	int _width;											// Width of FramePool images
	int _height;										// Height of FramePool images
};


#endif



