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

// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#ifndef GLRECORDER_H
#define GLRECORDER_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <GL/glext.h>

#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>

// Describes possible states in which the encoder might find itself,
// including all possible causes of error
typedef enum EncoderState {
	SUCCESS,
	CODEC_NOT_FOUND,
	CODEC_ALLOC_FAILED,
	OPEN_CODEC_FAILED,
	OPEN_FILE_FAILED,
	VIDEO_FRAME_ALLOC_FAILED,
	RAW_BUFFER_ALLOC_FAILED,
	FRAME_ENCODE_FAILED
} EncoderState;

// Encapsulates encoder state information and recording parameters
typedef struct RecorderParameters {
	unsigned int height;
	unsigned int width;
	unsigned int currentFrame;
	AVFrame* frame;
	AVCodecContext* codecCtx;
	AVPacket pkt;
	GLubyte* pixels;
	uint8_t* rgb;
	FILE* file;
	struct SwsContext* swsCtx;
} RecorderParameters;

/**
 * Prepares the video encoder and recording parameters for writing
 * to disk
 * @param params The parameters to use while recording
 * @param filename The output filename for the recording
 * @param codecID The video codec to use
 * @param fps The desired framerate
 * @return The encoder state post-initialization
 */
EncoderState glrecorder_startEncoder(RecorderParameters* params, const char *filename, int codecID, int fps);

/**
 * Writes any remaining frames to disk and stops the encoder
 * @param params The recording parameters
 * @return The state of the encoder post-termination
 */
EncoderState glrecorder_stopEncoder(RecorderParameters* params);

/**
 * Initializes a set of recording parameters
 * @param width The frame width
 * @param height The frame height
 * @return A newly allocated set of recording parameters with the given dimensions
 */
RecorderParameters* glrecorder_initParams(unsigned int width, unsigned int height);

/**
 * Deallocates the space allocated to a set of recording parameters
 * @params The recording parameters to deallocate
 */
void glrecorder_freeParams(RecorderParameters* params);

/**
 * Saves the current OpenGL frame as the next frame in the output
 * video file
 * @param params The recording parameters and current encoder state
 * @return The state of the encoder after recording the frame
 */
EncoderState glrecorder_recordFrame(RecorderParameters* params);

/**
 * Converts encoder states to strings
 * @param state The relevant encoder state
 * @return A human readable description of the state
 */
char* glrecorder_stateToString(EncoderState state);

#endif
