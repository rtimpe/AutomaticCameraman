#ifndef _FRAME_DISPLAYER_H_INCLUDED_
#define _FRAME_DISPLAYER_H_INCLUDED_

#include "FrameDisplayer.h"
#include <pthread.h>

FrameDisplayer *fd;


extern int save_images;
extern char * images_folder;


//////////////////////////////////////////////////////////////////////////////////////////////////////////

GLvoid key_press(unsigned char key, int x, int y) {
	switch (key) {
		case 'q':
			if (fd->_userExit != NULL)
				fd->_userExit();
			break;

		case 'v':
		    if (fd->_toggleRecord != NULL)
		        fd->_toggleRecord();
		    break;
	}
	glutPostRedisplay();
}


GLvoid display(void) {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	// Set Projection Matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, fd->_width, fd->_height, 0);

	// Switch to Model View Matrix
	//glMatrixMode(GL_MODELVIEW);
	//glLoadIdentity();

	// Draw first texture quad
	glBindTexture(GL_TEXTURE_2D, fd->textureId0);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex2f(fd->_width, 0.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex2f(fd->_width, fd->_height);
	glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, fd->_height);
	glEnd();


	// Draw second texture quad
	glBindTexture(GL_TEXTURE_2D, fd->textureId1);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex2f(fd->_width, 0.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex2f(fd->_width, fd->_height);
	glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, fd->_height);
	glEnd();

	glFlush();
	glutSwapBuffers();
}


GLvoid reshape(GLint w, GLint h) {
	glViewport(0, 0, w, h);
}




GLvoid idle() {


	Frame *frame;
	int frameNum;

	static IplImage * src1 = 0;
	static IplImage * src2 = 0;
	static IplImage * dst = 0;

	// Update the textures, remember the background image has no alpha
	frameNum = fd->_bgPool->acquire(&frame, fd->_lastBgFrameNum, false, false);
	if (frameNum > 0 && frameNum != fd->_lastBgFrameNum) {
		fd->_lastBgFrameNum = frameNum;
		glBindTexture(GL_TEXTURE_2D, fd->textureId0);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, fd->_width, fd->_height, GL_BGR, GL_UNSIGNED_BYTE, frame->_bgr->imageData);

		fd->_bgPool->release(frame);
	}

	frameNum = fd->_fgPool->acquire(&frame, fd->_lastFgFrameNum, false, false);
	if (frameNum > 0 && frameNum != fd->_lastFgFrameNum) {
		fd->_lastFgFrameNum = frameNum;
		glBindTexture(GL_TEXTURE_2D, fd->textureId1);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, fd->_width, fd->_height, GL_BGRA, GL_UNSIGNED_BYTE, frame->_bgr->imageData);

		fd->_fgPool->release(frame);
	}

	// Update display
	glutPostRedisplay();
}


void *displayer_fxn(void *arg) {

	IplImage * img;
	int argc = 1;
	char *argv[1];
	argv[0] = (char *)"foo";

	// Create GLUT Window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(fd->_width, fd->_height);
	fd->_window = glutCreateWindow(fd->_title);
	//glutFullScreen();

	// Initialize OpenGL
	glClearColor (0.0, 0.0, 0.0, 0.0);

	// Setup both images (textures)
	glGenTextures(1, &fd->textureId0);
	//glGenTextures(1, &fd->textureId1);

	// textureId0 is the background (video frame), in 3 channels (no alpha)
	img = cvCreateImage(cvSize(1280, 720), IPL_DEPTH_8U, 3);
	cvSet(img, cvScalar(0, 0, 0, 0));
	glBindTexture(GL_TEXTURE_2D, fd->textureId0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fd->_width, fd->_height, 0, GL_BGR, GL_UNSIGNED_BYTE, img->imageData);
	cvReleaseImage(&img);

	// textureId1 is the foreground (overlay frame), in 4 channels (with alpha)
	img = cvCreateImage(cvSize(1280, 720), IPL_DEPTH_8U, 4);
	cvSet(img, cvScalar(0, 0, 0, 0));
	glBindTexture(GL_TEXTURE_2D, fd->textureId1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fd->_width, fd->_height, 0, GL_BGRA, GL_UNSIGNED_BYTE, img->imageData);
	cvReleaseImage(&img);

	// Set up callbacks
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(key_press);
	glutIdleFunc(idle);

	// Start loop (never returns from this)
	glutMainLoop();

	// Terminate program at this point
    printf("TERMINATOR X!\n");

	return NULL;
}


FrameDisplayer::FrameDisplayer(char title[], int w, int h, FramePool *bgPool, 
	FramePool *fgPool, void (*userExit)(void), void (*toggleRecord)(void)) :
			_window(0), textureId0(0), textureId1(0), _lastBgFrameNum(0), _lastFgFrameNum(0) {
	// Save
	_bgPool = bgPool;
	_fgPool = fgPool;
	_title = title;
	_userExit = userExit;
	_toggleRecord = toggleRecord;
	_width = w;
	_height = h;
	fd = this;
}


bool FrameDisplayer::init() {
	// Initialize locals
	_lastBgFrameNum = 0;
	_lastFgFrameNum = 0;

	return true;
}


void FrameDisplayer::start() {
	// Create and start the GL thread
	pthread_t displayer; 
	pthread_create(&displayer, NULL, displayer_fxn, NULL);
}


void FrameDisplayer::stop() {
	//glutDestroyWindow(_window);
	//exit(0);
}

#endif // #ifndef _FRAME_DISPLAYER_H_INCLUDED_

