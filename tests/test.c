// Copyright (C) 2019 Arc676/Alessandro Vinciguerra <alesvinciguerra@gmail.com>
// Based on work by Ciro Santilli available at
// https://github.com/cirosantilli/cpp-cheat/blob/70b22ac36f92e93c94f951edb8b5af7947546525/opengl/offscreen.c

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation (version 3).

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.

#include "glrecorder.h"

/* Model. */
static double angle;
static double delta_angle;

static void model_init() {
	angle = 0;
	delta_angle = 1;
}

static int model_update() {
	angle += delta_angle;
	return 0;
}

static int model_finished() {
	return nframes >= max_nframes;
}

static void draw_scene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	glRotatef(angle, 0.0f, 0.0f, -1.0f);
	glBegin(GL_TRIANGLES);
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex3f( 0.0f,  0.5f, 0.0f);
	glColor3f(0.0f, 1.0f, 0.0f);
	glVertex3f(-0.5f, -0.5f, 0.0f);
	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex3f( 0.5f, -0.5f, 0.0f);
	glEnd();
}

static void init()  {
	int glget;

	if (offscreen) {
		/*  Framebuffer */
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);

		/* Color renderbuffer. */
		glGenRenderbuffers(1, &rbo_color);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo_color);
		/* Storage must be one of: */
		/* GL_RGBA4, GL_RGB565, GL_RGB5_A1, GL_DEPTH_COMPONENT16, GL_STENCIL_INDEX8. */
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB565, width, height);
		glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo_color);

		/* Depth renderbuffer. */
		glGenRenderbuffers(1, &rbo_depth);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo_depth);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
		glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo_depth);

		glReadBuffer(GL_COLOR_ATTACHMENT0);

		/* Sanity check. */
		assert(glCheckFramebufferStatus(GL_FRAMEBUFFER));
		glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &glget);
		assert(width < (unsigned int)glget);
		assert(height < (unsigned int)glget);
	} else {
		glReadBuffer(GL_BACK);
	}

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glEnable(GL_DEPTH_TEST);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	time0 = glutGet(GLUT_ELAPSED_TIME);
	model_init();
	ffmpeg_encoder_start("tmp.mpg", AV_CODEC_ID_MPEG1VIDEO, 25, width, height);
}

static void display() {
	char filename[SCREENSHOT_MAX_FILENAME];
	draw_scene();
	if (offscreen) {
		glFlush();
	} else {
		glutSwapBuffers();
	}
	processFrame();
	if (model_finished())
		exit(EXIT_SUCCESS);
}

static void idle() {
	while (model_update());
	glutPostRedisplay();
}

static void deinit()  {
	printf("FPS = %f\n", 1000.0 * nframes / (double)(glutGet(GLUT_ELAPSED_TIME) - time0));
	free(pixels);
	ffmpeg_encoder_finish();
	free(rgb);
	if (offscreen) {
		glDeleteFramebuffers(1, &fbo);
		glDeleteRenderbuffers(1, &rbo_color);
		glDeleteRenderbuffers(1, &rbo_depth);
	}
}

int main(int argc, char **argv) {
	int arg;
	GLint glut_display;

	/* CLI args. */
	glutInit(&argc, argv);
	arg = 1;
	if (argc > arg) {
		offscreen = (argv[arg][0] == '1');
	} else {
		offscreen = 1;
	}
	arg++;
	if (argc > arg) {
		max_nframes = strtoumax(argv[arg], NULL, 10);
	}
	arg++;
	if (argc > arg) {
		width = strtoumax(argv[arg], NULL, 10);
	}
	arg++;
	if (argc > arg) {
		height = strtoumax(argv[arg], NULL, 10);
	}

	/* Work. */
	if (offscreen) {
		/* TODO: if we use anything smaller than the window, it only renders a smaller version of things. */
		/*glutInitWindowSize(50, 50);*/
		glutInitWindowSize(width, height);
		glut_display = GLUT_SINGLE;
	} else {
		glutInitWindowSize(width, height);
		glutInitWindowPosition(100, 100);
		glut_display = GLUT_DOUBLE;
	}
	glutInitDisplayMode(glut_display | GLUT_RGBA | GLUT_DEPTH);
	glutCreateWindow(argv[0]);
	if (offscreen) {
		/* TODO: if we hide the window the program blocks. */
		/*glutHideWindow();*/
	}
	init();
	glutDisplayFunc(display);
	glutIdleFunc(idle);
	atexit(deinit);
	glutMainLoop();
	return EXIT_SUCCESS;
}
