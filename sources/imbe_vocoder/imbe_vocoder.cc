/*
 * Project 25 IMBE Encoder/Decoder Fixed-Point implementation
 * Developed by Pavel Yazev E-mail: pyazev@gmail.com
 * Version 1.0 (c) Copyright 2009
 */

#include <stdio.h>
#include <stdlib.h>

#include "imbe_vocoder.h"
#include "crossplatfprmfft.h"

imbe_vocoder::imbe_vocoder (void) :
	prev_pitch(0),
	prev_prev_pitch(0),
	prev_e_p(0),
	prev_prev_e_p(0),
	seed(1),
	num_harms_prev1(0),
	num_harms_prev2(0),
	num_harms_prev3(0),
	fund_freq_prev(0),
	th_max(0),
	dc_rmv_mem(0)
{
	fft = new CrossplatfprmFft();

	memset(pitch_est_buf, 0, sizeof(pitch_est_buf));
	memset(pitch_ref_buf, 0, sizeof(pitch_ref_buf));
	memset(pe_lpf_mem, 0, sizeof(pe_lpf_mem));
	memset(fft_buf, 0, sizeof(fft_buf));
	memset(sa_prev1, 0, sizeof(sa_prev1));
	memset(sa_prev2, 0, sizeof(sa_prev2));
	memset(uv_mem, 0, sizeof(uv_mem));
	memset(ph_mem, 0, sizeof(ph_mem));
	memset(vu_dsn_prev, 0, sizeof(vu_dsn_prev));
	memset(sa_prev3, 0, sizeof(sa_prev3));
	memset(v_uv_dsn, 0, sizeof(v_uv_dsn));

	memset(&my_imbe_param, 0, sizeof(IMBE_PARAM));

	decode_init(&my_imbe_param);
	encode_init();
}

imbe_vocoder::~imbe_vocoder() {

	delete fft;
}

void imbe_vocoder::excuteFft(Word16 * x) {

	fft->directTransform((Cmplx16 *)x);
}

void imbe_vocoder::excuteIfft(Word16 * x) {

	fft->inverseTransform((Cmplx16 *)x);
}
