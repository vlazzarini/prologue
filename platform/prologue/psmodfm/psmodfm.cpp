/*  Phase Synchronous Modified Frequency Modulation oscillator
    Copyright 2020 Victor Lazzarini

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "userosc.h"
#define EXP(x) fasterexpf(x)
#define POW(x, y) fasterpowf(x, y)
#define POW2(x) fasterpow2f(x)

// Simple linear AR envelope
struct Env {
  float atti, deci, e;
  bool dflg;

  Env() : atti(1), deci(1), e(0.), dflg(false) { };

  float init(float att, float dec) {
    e = 0.f;
    atti = att > 0.f ? 1.f / (att * k_samplerate) : (e = 1.f);
    deci = dec > 0.f ? 1.f / (dec * k_samplerate) : 1.f;
    dflg = false;
  }

  void decay() { dflg = true; }

  float proc() {
    return (e = !dflg ? (e < 1.f ? e + atti : 1.f)
            : (e > 0.f ? e - deci : 0.f));
  }

  float val() { return e;}
};

struct PSModFM {
  float phase, sphase;
  float z;
  float ff, ffz;
  float lfo;
  float shft;
  int16_t smax;
  int16_t fmode;
  float att, dec, amnt;
  Env env;

  PSModFM() :  phase(0.f), sphase(0.f), z(0.f),
               ff(0.f), ffz(0.f), lfo(0.f), 
               shft(0.f), smax(0), fmode(0), att(0.f), dec(0.f),
               amnt(0.f), env() { };
    

  float synthesise(float ndx, float a, float pc1, float pc2, float pm) {
    return (a * osc_cosf(pc2) + (1.f - a) * osc_cosf(pc1)) *
      EXP(ndx * (osc_cosf(pm) - 1.f));
  }

  float mod_ndx(float fo, float ff) {
    float kbw, g,gm;
    kbw = ff / (.5f + 3.5f * z); // Q: 0.5 to 4
    g = POW2(-fo / (.29f * kbw));
    gm = 1. - g;
    return 2*g/(gm*gm);
  }
};

static PSModFM obj;

void OSC_INIT(uint32_t platform, uint32_t api) {
}

void OSC_CYCLE(const user_osc_param_t *const params, int32_t *yn,
               const uint32_t frames) {
  Env &env = obj.env;
  const float amnt = obj.amnt*32.f;
  const float fmax = 12000.f; // max formant freq
  const float w0 = osc_w0f_for_note((params->pitch) >> 8, params->pitch & 0xFF);
  const float fo = w0 * k_samplerate;
  const float fo_recip = w0 * k_samplerate_recipf;
  const float ws = w0 * obj.shft * (1 + obj.smax);
  const float ff =
    obj.fmode ? fmax*POW2((obj.ff - 1.)*(obj.fmode+1)) : fo * POW(fmax * fo_recip, obj.ff);
  const float ffmx = ff*(1.f + amnt * env.val());
  const float ndx = obj.mod_ndx(fo, ffmx < fmax ? ffmx : fmax);
  const float lfo = q31_to_f32(params->shape_lfo);
  float lfoz =  obj.lfo;
  float ffz =   obj.ffz;
  const float frameo1 = 1./frames;
  const float lfo_inc = (lfo - lfoz)*frameo1;
  const float ff_inc = (ff - ffz)*frameo1;
  float phase =  obj.phase;
  float sphase = obj.sphase;

  
 
  for (int i = 0; i < frames; i++) {
    q31_t *__restrict y = (q31_t *) yn;
    float ff_mod, a, pc1, pc2, e;
    int m, k = 0;
    e = 1.f + amnt * env.proc();
    ff_mod = (ffz + lfoz * ff)*e;
    ff_mod = (ff_mod < fmax ? (ff_mod > fo ? ff_mod : fo) : fmax) * fo_recip;
    m =  (uint32_t) ff_mod;
    a = ff_mod - m;
    pc1 = phase * m + sphase;
    pc1 -= (uint32_t) pc1;
    pc2 = phase * (m + 1) + sphase;
    pc2 -= (uint32_t) pc2;
    y[i] = f32_to_q31(obj.synthesise(ndx, a, pc1, pc2, phase));
    phase += w0;
    phase -= (uint32_t) phase;
    sphase += ws;
    sphase -= (uint32_t) sphase;
    lfoz += lfo_inc;
    ffz += ff_inc;
  }
  obj.phase = phase;
  obj.sphase = sphase;
  obj.lfo = lfoz;
  obj.ffz = ffz;
}

void OSC_NOTEON(const user_osc_param_t *const params) {
  const float att = POW(11.f, obj.att) - 1.f;
  const float dec = POW(11.f, obj.dec) - 1.f;
  obj.env.init(att, dec);
}

void OSC_NOTEOFF(const user_osc_param_t *const params) { obj.env.decay(); }

void OSC_PARAM(uint16_t index, uint16_t value) {
  const float valf = param_val_to_f32(value);
  switch (index) {
  case k_user_osc_param_id1:
    // mod freq shift max
    obj.smax = value;
    break;
  case k_user_osc_param_id2:
    // mod freq shift
    obj.shft = clip01f(value * 0.01f);
    break;
  case k_user_osc_param_id3:
    // freq tracking mode
    obj.fmode = value;
    break;
  case k_user_osc_param_id4:
    // env att
    obj.att = clip01f(value * 0.01f);
    break;
  case k_user_osc_param_id5:
    // env dec
    obj.dec = clip01f(value * 0.01f);
    break;
  case k_user_osc_param_id6:
    // env amount
    obj.amnt = clip01f(value * 0.01f);
    break;
  case k_user_osc_param_shape:
    // formant freq (0-1)
    obj.ff = valf;
    break;
  case k_user_osc_param_shiftshape:
    // formant Q (0 - 1)
    obj.z = valf;
    break;
  default:
    break;
  }
}
