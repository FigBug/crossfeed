/*
 * Copyright (c) 2013 Jeremy Pepper
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

#include "crossfeed.h"
#include <string.h>

static const float kernel_8000[] = {
    0.8763579130, -0.5805998445,
};

static const float kernel_11025[] = {
    0.9483570457, -0.1237868592, -0.5424064398,
};

static const float kernel_16000[] = {
    0.9560604692, -0.0439161919, -0.1358038932, -0.4919319153,
};

static const float kernel_22050[] = {
    0.9717733860, -0.0401228145, -0.0436810628, -0.1528297365, -0.4400325418,
};

static const float kernel_32000[] = {
    0.9797983170, -0.0247756578, -0.0253159832, -0.0395310111, -0.0519143902, -0.1750136167, -0.3683812916,
};

static const float kernel_37800[] = {
    0.9834311008, -0.0187791381, -0.0189585295, -0.0236973725, -0.0253315605, -0.0407797173, -0.0587460436, -0.1836037338, -0.3325985074,
};

static const float kernel_44056[] = {
    0.9845736623, -0.0155907851, -0.0178482905, -0.0183826648, -0.0234144107, -0.0263074208, -0.0431270078, -0.0667571425, -0.1897651553, -0.2980891466,
};

static const float kernel_44100[] = {
    0.9845703244, -0.0155883674, -0.0178523716, -0.0183844436, -0.0234241486, -0.0263205972, -0.0431552231, -0.0668175668, -0.1898029149, -0.2978620529,
};

static const float kernel_47250[] = {
    0.9863407016, -0.0149254613, -0.0151630556, -0.0174527522, -0.0182038192, -0.0233944859, -0.0269670859, -0.0444734097, -0.0707989335, -0.1916704774, -0.2818990946,
};

static const float kernel_48000[] = {
    0.9864154458, -0.0148524623, -0.0151097234, -0.0174051225, -0.0182127487, -0.0234390832, -0.0271747112, -0.0448399670, -0.0717705265, -0.1920349747, -0.2782397866,
};

static const float kernel_50000[] = {
    0.9866725802, -0.0146328239, -0.0149040138, -0.0172626916, -0.0181766041, -0.0235424917, -0.0276779458, -0.0458027422, -0.0742682368, -0.1928016245, -0.2686870396,
};

static const float kernel_50400[] = {
    0.9868457317, -0.0132913291, -0.0145996250, -0.0148791429, -0.0172472540, -0.0181876719, -0.0235758740, -0.0277961828, -0.0460085459, -0.0747763738, -0.1929272562, -0.2668201327,
};

static const float kernel_88200[] = {
    0.9920045733, -0.0081339637, -0.0084169386, -0.0086013228, -0.0089902263, -0.0092607271, -0.0098341256, -0.0102718966, -0.0111861089, -0.0119803548, -0.0135877309, -0.0152284242, -0.0184706226, -0.0224333983, -0.0303510539, -0.0421869755, -0.0669840872, -0.1088913009, -0.1722959429, -0.1384861916,
};

static const float kernel_96000[] = {
    0.9926081896, -0.0075246124, -0.0077289417, -0.0078936629, -0.0081709018, -0.0083940942, -0.0087880893, -0.0091180159, -0.0097118560, -0.0102479970, -0.0112097915, -0.0121685937, -0.0138866305, -0.0158295482, -0.0193291456, -0.0238990616, -0.0324138403, -0.0454851724, -0.0709896907, -0.1120689511, -0.1636260599, -0.1210290343,
};

static const float kernel_176400[] = {
    0.9756238461, -0.0272996202, -0.0303747673, -0.0284479242, -0.0246019531, -0.0248113442, -0.0278908722, -0.0276220813, -0.0233108141, -0.0205958951, -0.0218199138, -0.0224530455, -0.0190727673, -0.0146292858, -0.0129663050, -0.0127072223, -0.0104541564, -0.0062395171, -0.0024387678, 0.0003043792, 0.0027835192, 0.0054231589, 0.0088404203, 0.0137609616, 0.0183477756, 0.0197170842, 0.0195225142, 0.0230890736, 0.0295875575, 0.0306768361, 0.0236023739, 0.0178110972, 0.0188751873, 0.0151938098, -0.0065959562, -0.0381721593, -0.0597930886, -0.0650577918, -0.0555361398, -0.0267984103,
};

static const float kernel_192000[] = {
    0.9957793951, -0.0042529940, -0.0042936588, -0.0043312632, -0.0043765539, -0.0044197347, -0.0044716918, -0.0045221476, -0.0045838049, -0.0046442472, -0.0047141630, -0.0047885645, -0.0048747342, -0.0049641728, -0.0050696530, -0.0051812925, -0.0053134533, -0.0054549766, -0.0056228149, -0.0058088298, -0.0060280575, -0.0062736506, -0.0065682987, -0.0069045764, -0.0073094252, -0.0077845287, -0.0083643645, -0.0090559060, -0.0099158781, -0.0109688807, -0.0123044215, -0.0139841484, -0.0161668882, -0.0189968832, -0.0227641948, -0.0277801156, -0.0345608816, -0.0435925797, -0.0553293787, -0.0691306964, -0.0816807896, -0.0838362649, -0.0637937710, -0.0253506508,
};

static const float kernel_352800[] = {
    0.9972188473, -0.0027943747, -0.0028002940, -0.0028133704, -0.0028203218, -0.0028335198, -0.0028410403, -0.0028551868, -0.0028634979, -0.0028799027, -0.0028896593, -0.0029063807, -0.0029174255, -0.0029341672, -0.0029462972, -0.0029675465, -0.0029808537, -0.0030004918, -0.0030168116, -0.0030421198, -0.0030586580, -0.0030830537, -0.0031036716, -0.0031324737, -0.0031553456, -0.0031864820, -0.0032133614, -0.0032492713, -0.0032795288, -0.0033185147, -0.0033542577, -0.0033985567, -0.0034396113, -0.0034908250, -0.0035387108, -0.0035980875, -0.0036532616, -0.0037214952, -0.0037883739, -0.0038681114, -0.0039468212, -0.0040401574, -0.0041344753, -0.0042449166, -0.0043603983, -0.0044927965, -0.0046313675, -0.0047926763, -0.0049640713, -0.0051621585, -0.0053751864, -0.0056210477, -0.0058888965, -0.0061979075, -0.0065390454, -0.0069339224, -0.0073762308, -0.0078891506, -0.0084693227, -0.0091450214, -0.0099194413, -0.0108256303, -0.0118738394, -0.0131076453, -0.0145483632, -0.0162494350, -0.0182418004, -0.0205896851, -0.0233262926, -0.0265033673, -0.0301112104, -0.0341009870, -0.0382736251, -0.0422559828, -0.0453460701, -0.0465203635, -0.0445179604, -0.0384043753, -0.0283658840, -0.0164501220, -0.0061756251,
};

int crossfeed_init(crossfeed_t *filter, int samplerate) {
	memset(filter, 0, sizeof(crossfeed_t));
	switch(samplerate) {
        case 8000:
            filter->filter = kernel_8000;
            filter->delay = 0;
            filter->len = sizeof(kernel_8000)/sizeof(float);
        case 11025:
            filter->filter = kernel_11025;
            filter->delay = 0;
            filter->len = sizeof(kernel_11025)/sizeof(float);
        case 16000:
            filter->filter = kernel_16000;
            filter->delay = 0;
            filter->len = sizeof(kernel_16000)/sizeof(float);
        case 22050:
            filter->filter = kernel_22050;
            filter->delay = 0;
            filter->len = sizeof(kernel_22050)/sizeof(float);
        case 32000:
            filter->filter = kernel_32000;
            filter->delay = 0;
            filter->len = sizeof(kernel_32000)/sizeof(float);
        case 37800:
            filter->filter = kernel_37800;
            filter->delay = 0;
            filter->len = sizeof(kernel_37800)/sizeof(float);
        case 44056:
            filter->filter = kernel_44056;
            filter->delay = 0;
            filter->len = sizeof(kernel_44056)/sizeof(float);
        case 44100:
            filter->filter = kernel_44100;
            filter->delay = 0;
            filter->len = sizeof(kernel_44100)/sizeof(float);
        case 47250:
            filter->filter = kernel_47250;
            filter->delay = 0;
            filter->len = sizeof(kernel_47250)/sizeof(float);
        case 48000:
            filter->filter = kernel_48000;
            filter->delay = 0;
            filter->len = sizeof(kernel_48000)/sizeof(float);
        case 50000:
            filter->filter = kernel_50000;
            filter->delay = 0;
            filter->len = sizeof(kernel_50000)/sizeof(float);
        case 50400:
            filter->filter = kernel_50400;
            filter->delay = 0;
            filter->len = sizeof(kernel_50400)/sizeof(float);
        case 88200:
            filter->filter = kernel_88200;
            filter->delay = 0;
            filter->len = sizeof(kernel_88200)/sizeof(float);
        case 96000:
            filter->filter = kernel_96000;
            filter->delay = 0;
            filter->len = sizeof(kernel_96000)/sizeof(float);
        case 176400:
            filter->filter = kernel_176400;
            filter->delay = 0;
            filter->len = sizeof(kernel_176400)/sizeof(float);
        case 192000:
            filter->filter = kernel_192000;
            filter->delay = 0;
            filter->len = sizeof(kernel_192000)/sizeof(float);
        case 352800:
            filter->filter = kernel_352800;
            filter->delay = 0;
            filter->len = sizeof(kernel_352800)/sizeof(float);
	default:
		return -1;
	}
	return 0;
}

static inline void crossfeed_process_sample(crossfeed_t *filter, float left, float right,
                                            float *oleft, float *oright) {
	float mid = (left + right) / 2;
	float side = (left - right) / 2;
	float oside = 0;
	filter->mid[(filter->pos + filter->delay) % filter->len] = mid;
	filter->side[filter->pos] = side;
	if(!filter->bypass) {
		for(unsigned int i=0;i<filter->len;++i) {
			oside += filter->side[(filter->pos + filter->len - i) % filter->len] * filter->filter[i];
		}
	} else {
		oside = filter->side[(filter->pos + filter->len - filter->delay) % filter->len];
	}
	*oleft = filter->mid[filter->pos] + oside;
	*oright = filter->mid[filter->pos] - oside;
	filter->pos = (filter->pos + 1) % filter->len;
}

void crossfeed_filter(crossfeed_t *filter, float *input, float *output, unsigned int size) {
	for(unsigned int i=0;i<size;++i) {
		crossfeed_process_sample(filter, input[i*2], input[i*2+1], &output[i*2],
		                         &output[i*2+1]);
	}
}

void crossfeed_filter_inplace_noninterleaved(crossfeed_t *filter, float *left, float *right,
                                             unsigned int size) {
	for(unsigned int i=0;i<size;++i) {
		crossfeed_process_sample(filter, left[i], right[i], &left[i],
		                         &right[i]);
	}
}
