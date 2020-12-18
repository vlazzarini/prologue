/*  Extended Modified Frequency Modulation oscillator
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
#define ONEOPI2 0.1591549f
#define MODMAX 15.f

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
  int reset;
  float phase, phasem;
  float lfo, ndx;
  float r, s;
  float car, mod, fine;
  float att, dec, amnt;
  Env env;

  PSModFM() :  reset(1), phase(0.f), phasem(0.f),lfo(0.f), ndx(0.f),
               r(0.f), s(1.f), car(1.f), mod(1.f), fine(1.f),
               att(0.f), dec(0.f), amnt(0.f), env() { };
    

  float synthesise(float k, float r, float s, float pc, float pm) {
    float ph = pc + s*k*ONEOPI2*osc_sinf(pm);
    ph = ph < 0.f ? ph - floor(ph) : ph - (uint32_t) ph;
    return EXP(r*k*(osc_cosf(pm)-1.f))*osc_cosf(ph);
  }
      


};

static PSModFM obj;

void OSC_INIT(uint32_t platform, uint32_t api) {
}

void OSC_CYCLE(const user_osc_param_t *const params, int32_t *yn,
               const uint32_t frames) {
  Env &env = obj.env;
  const float w0 = osc_w0f_for_note((params->pitch) >> 8, params->pitch & 0xFF);
  const float wc = w0*obj.car;
  const float wm = w0*obj.mod*obj.fine;
  const float amnt = obj.amnt*MODMAX;
  const float r = obj.r;
  const float s = obj.s;  
  const float ndx = obj.ndx*MODMAX;
  const float lfo = fabs(q31_to_f32(params->shape_lfo))*MODMAX;
  float lfoz = obj.reset ? 0.f : obj.lfo;
  const float frameo1 = 1./frames;
  const float lfo_inc = (lfo - lfoz)*frameo1;
  float phase = obj.reset ? 0.f : obj.phase;
  float phasem = obj.reset ? 0.f : obj.phasem;

  for (int i = 0; i < frames; i++) {
    q31_t *__restrict y = (q31_t *) yn;
    float m;
    m = amnt*env.proc() + ndx + lfo;
    y[i] = f32_to_q31(obj.synthesise(m < MODMAX ? m : MODMAX,r,s,phase,phasem));
    phase += wc;
    phase -= (uint32_t) phase;
    phasem += wm;
    phasem -= (uint32_t) phasem;
    lfoz += lfo_inc;
  }
  obj.phase = phase;
  obj.phasem = phasem;
  obj.lfo = lfoz;
  obj.reset = 0;
}

void OSC_NOTEON(const user_osc_param_t *const params) {
  const float att = POW(11.f, obj.att) - 1.f;
  const float dec = POW(11.f, obj.dec) - 1.f;
  obj.env.init(att, dec);
  obj.reset = 1;
}

void OSC_NOTEOFF(const user_osc_param_t *const params) { obj.env.decay(); }

void OSC_PARAM(uint16_t index, uint16_t value) {
  const float valf = param_val_to_f32(value);
  float vf;
  switch (index) {
  case k_user_osc_param_id1:
    // car ratio
    obj.car = (float)  (value + 1);
    break;
  case k_user_osc_param_id2:
    // mod ratio
    obj.mod =  (float)  (value + 1);
    break;
  case k_user_osc_param_id3:
    // mod fine
    obj.fine = (1.f  + clip01f(value * 0.01f));
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
    vf = valf*4.f;
    vf = vf < 4.f ?  vf - (uint32_t) vf : 1.;
    if(valf < 0.25f) {
      obj.r = vf;

      obj.s = 1.f - vf*2;
    } else if  (valf < 0.5f) {
      obj.r = 1.f;
      obj.s = vf - 1.f;
    } else if (valf < 0.75f) {
      obj.r = 1.f;
      obj.s = vf;
      } else {
      obj.r = 1.f - vf; 
      obj.s = 1.f;
    }
    break;
  case k_user_osc_param_shiftshape:
    // formant Q (0 - 1)
    obj.ndx = valf;
    break;
  default:
    break;
  }
}
