/*
 * Copyright (c) 2017 Jeremy Pepper
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of crossfeed nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef DSP_H
#define DSP_H
#include <cmath>
#include <memory>
#ifdef HAS_ACCELERATE
#include <Accelerate/Accelerate.h>
#else
#include "kiss_fft.h"
#endif

namespace fdesign {
	template <typename value_type>
	class fft_context {
	private:
#if HAS_ACCELERATE
		void *context;
#else
        kiss_fft_cfg context;
#endif
	public:
		const int N, logN;
		fft_context(int N);
		~fft_context();
		void fft(value_type *result, const value_type *source);
		void polar(value_type *magnitudes, value_type *source);
	};

	template <>
	fft_context<float>::fft_context(int N) : N(N), logN((int)std::round(std::log2(N))) {
#if HAS_ACCELERATE
		context = vDSP_create_fftsetup(logN, FFT_RADIX2);
#else
        context = kiss_fft_alloc(N/2, 0, NULL, NULL);
#endif
	}

	template <>
	fft_context<float>::~fft_context() {
#if HAS_ACCELERATE
		vDSP_destroy_fftsetup((FFTSetup)context);
#else
        free(context);
#endif
	}

	template <>
	void fft_context<float>::fft(float *result, const float *source) {
		float scale = 0.5;
#if HAS_ACCELERATE
        float response_memory[N];
		DSPSplitComplex response = {response_memory, response_memory + N/2};
		vDSP_ctoz((DSPComplex *)source, 2, &response, 1, N/2);
		vDSP_fft_zrip((FFTSetup)context, &response, 1, logN, FFT_FORWARD);
		vDSP_ztoc(&response, 1, (DSPComplex *)result, 2, N/2);
		vDSP_vsmul(result, 1, &scale, result, 1, N);
#else
        kiss_fft_cpx fin[N/2];
        kiss_fft_cpx fout[N/2];
        
        for (int i = 0; i < N/2; i++)
        {
            fin[i].r = source[i * 2];
            fin[i].i = source[i * 2 + 1];
        }
        
        kiss_fft(context, fin, fout);
        
        for (int i = 0; i < N/2; i++)
        {
            result[i * 2] = fout[i].r * scale;
            result[i * 2 + 1] = fout[i].i * scale;
        }
#endif
	}

	template <>
	void fft_context<float>::polar(float *magnitudes, float *source) {
#if HAS_ACCELERATE
		vDSP_polar(magnitudes, 2, source, 2, N/2);
#else
        for (int i = 0; i < N/2; i++)
        {
            int n1 = i * 2;
            int n2 = i * 2 + 1;
            
            source[n1] = std::sqrt(magnitudes[n1] * magnitudes[n1] + magnitudes[n2] * magnitudes[n2]);
            source[n2] = std::atan2(magnitudes[n2], magnitudes[n1]);
        }
#endif
	}

	template <>
	fft_context<double>::fft_context(int N) : N(N), logN((int)std::round(std::log2(N))) {
#if HAS_ACCELERATE
		context = vDSP_create_fftsetupD(logN, FFT_RADIX2);
#else
        context = kiss_fft_alloc(N/2, 0, NULL, NULL);
#endif
	}

	template <>
	fft_context<double>::~fft_context() {
#if HAS_ACCELERATE
		vDSP_destroy_fftsetupD((FFTSetupD)context);
#else
        free(context);
#endif
	}

	template <>
	void fft_context<double>::fft(double *result, const double *source) {
		double scale = 0.5;
#if HAS_ACCELERATE
        double response_memory[N];
        DSPDoubleSplitComplex response = {response_memory, response_memory + N/2};
		vDSP_ctozD((DSPDoubleComplex *)source, 2, &response, 1, N/2);
		vDSP_fft_zripD((FFTSetupD)context, &response, 1, logN, FFT_FORWARD);
		vDSP_ztocD(&response, 1, (DSPDoubleComplex *)result, 2, N/2);
		vDSP_vsmulD(result, 1, &scale, result, 1, N);
#else
        kiss_fft_cpx fin[N/2];
        kiss_fft_cpx fout[N/2];
        
        for (int i = 0; i < N/2; i++)
        {
            fin[i].r = source[i * 2];
            fin[i].i = source[i * 2 + 1];
        }
        
        kiss_fft(context, fin, fout);
        
        for (int i = 0; i < N/2; i++)
        {
            result[i * 2] = fout[i].r * scale;
            result[i * 2 + 1] = fout[i].i * scale;
        }
#endif
	}

	template <>
	void fft_context<double>::polar(double *magnitudes, double *source) {
#if HAS_ACCELERATE
		vDSP_polarD(magnitudes, 2, source, 2, N/2);
#else
        for (int i = 0; i < N/2; i++)
        {
            int n1 = i * 2;
            int n2 = i * 2 + 1;
            
            source[n1] = std::sqrt(magnitudes[n1] * magnitudes[n1] + magnitudes[n2] * magnitudes[n2]);
            source[n2] = std::atan2(magnitudes[n2], magnitudes[n1]);
        }
#endif
	}

	template <typename value_type>
	void array_clear(value_type *array, int N);

	template <>
	void array_clear<float>(float *array, int N) {
#if HAS_ACCELERATE
		vDSP_vclr(array, 1, N);
#else
        for (int i = 0; i < N; i++)
            array[i] = 0;
#endif
	}

	template <>
	void array_clear<double>(double *array, int N) {
#if HAS_ACCELERATE
        vDSP_vclrD(array, 1, N);
#else
        for (int i = 0; i < N; i++)
            array[i] = 0;
#endif
	}

	template <typename value_type>
	void filter_apply(value_type *result, const value_type *signal, int signal_len, const value_type *filter, int filter_len) {
		int result_len = signal_len+filter_len-1;
		array_clear(result, result_len);
		for(int i=0;i<signal_len;++i) {
			for(int j=0;j<filter_len;++j) {
				result[i+j] += signal[i]*filter[j];
			}
		}
	}

	template <typename value_type>
	class gabor_context : public fft_context<value_type> {
	private:
		std::unique_ptr<value_type[]> window;
	public:
		const int time_period;
		const int window_len;
		const int window_spacing;
		gabor_context(int N, int time_period, int oversample) : fft_context<value_type>(N), time_period(time_period), window_len(6*time_period+1), window_spacing(time_period / oversample) {
			window.reset(new value_type[window_len]);
			array_clear(window.get(), window_len);
			int T = time_period/2;
			T *= T;
			value_type total = 0;
			for(int t=0;t<window_len;++t) {
				int n = t - window_len/2;
				window[t] = /* (1/sqrt(2*M_PI*T))* */exp((-(n*n))/(2.*T));
			}
		}
		void dgt(value_type *result, const value_type *source, int source_len) {
			value_type windowed[this->N];
			for(int t=0;t*window_spacing < source_len;++t) {
				for(int i=0;i<this->N;++i) {
					int wpos = (window_len/2)-((t*window_spacing)+i);
					value_type in = i+t*window_spacing < source_len ? source[i+t*window_spacing] : 0;
					value_type win = wpos > 0 && wpos < window_len ? window[wpos] : 0;
					windowed[i] = in*win;
				}
				this->fft(&result[t*this->N], windowed);
			}
		}
	};
}

#endif
