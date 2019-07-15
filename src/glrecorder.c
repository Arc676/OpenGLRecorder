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

#include "glrecorder.h"

/* Adapted from: https://github.com/cirosantilli/cpp-cheat/blob/19044698f91fefa9cb75328c44f7a487d336b541/ffmpeg/encode.c */

void glrecorder_ffmpegEncoderSetFrameYUVFromRGB(RecorderParameters* params) {
	unsigned int width = params->codecCtx->width;
	unsigned int height = params->codecCtx->height;
	const int inLinesize[1] = { 4 * params->codecCtx->width };
	params->swsCtx = sws_getCachedContext(params->swsCtx,
			width, height, AV_PIX_FMT_RGB32,
			width, height, AV_PIX_FMT_YUV420P,
			0, NULL, NULL, NULL);
	sws_scale(params->swsCtx, (const uint8_t * const *)&(params->rgb), inLinesize, 0,
			height, params->frame->data, params->frame->linesize);
}

EncoderState glrecorder_startEncoder(RecorderParameters* params, const char* filename, int codec_id, int fps) {
	avcodec_register_all();
	AVCodec* codec = avcodec_find_encoder(codec_id);
	if (!codec) {
		return CODEC_NOT_FOUND;
	}
	AVCodecContext* c = avcodec_alloc_context3(codec);
	if (!c) {
		return CODEC_ALLOC_FAILED;
	}
	c->bit_rate = 400000;
	c->width = params->width;
	c->height = params->height;
	c->time_base.num = 1;
	c->time_base.den = fps;
	c->gop_size = 10;
	c->max_b_frames = 1;
	c->pix_fmt = AV_PIX_FMT_YUV420P;
	if (codec_id == AV_CODEC_ID_H264) {
		av_opt_set(c->priv_data, "preset", "slow", 0);
	}
	if (avcodec_open2(c, codec, NULL) < 0) {
		return OPEN_CODEC_FAILED;
	}
	FILE* file = fopen(filename, "wb");
	if (!file) {
		return OPEN_FILE_FAILED;
	}
	AVFrame* frame = av_frame_alloc();
	if (!frame) {
		return VIDEO_FRAME_ALLOC_FAILED;
	}
	frame->format = c->pix_fmt;
	frame->width  = c->width;
	frame->height = c->height;
	int ret = av_image_alloc(frame->data, frame->linesize, c->width, c->height, c->pix_fmt, 32);
	if (ret < 0) {
		return RAW_BUFFER_ALLOC_FAILED;
	}
	params->frame = frame;
	params->codecCtx = c;
	params->file = file;
	return SUCCESS;
}

EncoderState glrecorder_stopEncoder(RecorderParameters* params) {
	uint8_t endcode[] = { 0, 0, 1, 0xb7 };
	int gotOutput, ret;
	do {
		fflush(stdout);
		ret = avcodec_encode_video2(params->codecCtx, &(params->pkt), NULL, &gotOutput);
		if (ret < 0) {
			return FRAME_ENCODE_FAILED;
		}
		if (gotOutput) {
			fwrite(params->pkt.data, 1, params->pkt.size, params->file);
			av_packet_unref(&(params->pkt));
		}
	} while (gotOutput);
	fwrite(endcode, 1, sizeof(endcode), params->file);
	fclose(params->file);
	avcodec_close(params->codecCtx);
	av_free(params->codecCtx);
	av_freep(&(params->frame->data[0]));
	av_frame_free(&(params->frame));
	return SUCCESS;
}

EncoderState glrecorder_encodeFrame(RecorderParameters* params) {
	glrecorder_ffmpegEncoderSetFrameYUVFromRGB(params);
	av_init_packet(&(params->pkt));
	params->pkt.data = NULL;
	params->pkt.size = 0;
	int gotOutput;
	int ret = avcodec_encode_video2(params->codecCtx, &(params->pkt), params->frame, &gotOutput);
	if (ret < 0) {
		return FRAME_ENCODE_FAILED;
	}
	if (gotOutput) {
		fwrite(params->pkt.data, 1, params->pkt.size, params->file);
		av_packet_unref(&(params->pkt));
	}
	return SUCCESS;
}

void glrecorder_encoderReadRGB(RecorderParameters* params) {
	unsigned int width = params->width;
	unsigned int height = params->height;
	size_t i, j, k, cur_gl, cur_rgb, nvals;
	const size_t format_nchannels = 4;
	nvals = format_nchannels * width * height;
	params->pixels = realloc(params->pixels, nvals * sizeof(GLubyte));
	params->rgb = realloc(params->rgb, nvals * sizeof(uint8_t));
	/* Get RGBA to align to 32 bits instead of just 24 for RGB. May be faster for FFmpeg. */
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, params->pixels);
	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			cur_gl  = format_nchannels * (width * (height - i - 1) + j);
			cur_rgb = format_nchannels * (width * i + j);
			for (k = 0; k < format_nchannels; k++) {
				params->rgb[cur_rgb + k] = params->pixels[cur_gl + k];
			}
		}
	}
}

EncoderState glrecorder_recordFrame(RecorderParameters* params) {
	params->frame->pts = params->currentFrame;
	EncoderState state = glrecorder_encoderReadRGB(params);
	if (state != SUCCESS) {
		return state;
	}
	state = glrecorder_encodeFrame(params);
	if (state != SUCCESS) {
		return state;
	}
	params->currentFrame++;
	return SUCCESS;
}

RecorderParameters* glrecorder_initParams(unsigned int width, unsigned int height) {
	RecorderParameters* params = malloc(sizeof(RecorderParameters));
	memset(params, 0, sizeof(RecorderParameters));
	params->height = height;
	params->width = width;
	return params;
}

void glrecorder_freeParams(RecorderParameters* params) {
	free(params->pixels);
	free(params->rgb);
	free(params);
}

char* glrecorder_stateToString(EncoderState state) {
	switch (state) {
	case SUCCESS:
		return "Encoder working fine";
	case CODEC_NOT_FOUND:
		return "Codec not found";
	case CODEC_ALLOC_FAILED:
		return "Could not allocate video codec context";
	case OPEN_CODEC_FAILED:
		return "Could not open video codec";
	case OPEN_FILE_FAILED:
		return "Could not open output file";
	case VIDEO_FRAME_ALLOC_FAILED:
		return "Could not allocate video frame";
	case RAW_BUFFER_ALLOC_FAILED:
		return "Could not allocate raw picture buffer";
	case FRAME_ENCODE_FAILED:
		return "Error encoding frame";
	default:
		return "Unknown state";
	}
}
