/*  ModFM formant oscillator
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

/* bass formants */
const float bassf[] = {600,400,250,400,350,600,
                       1040,1620,1750,750,600,1040,
                       2250,2400,2600,2400,2400,2250,
                       2450,2800,3200,2650,2675,2450};


const float bassb[] = {60,40, 60, 40, 40, 60,
                        70, 80, 90, 80, 80, 70,
                        110, 100, 100, 100, 100, 110,
                         120, 120, 120, 120, 120, 120};
    
const float bassa[] =  {0.45,0.25,0.031,0.27,0.1,0.45,
                            0.35,0.35,0.15 ,0.1,0.032,0.35,
                            0.35,0.25,0.1,0.1,0.04,0.35};

const float tenorf[] = {650,400,290,400,350,650,
                            1080,1700,1870,800,600,1080,
                            2650,2600,2800,2600,2700,2650,
                            2900,3200,3250,2800,2900,2900};


const float tenorb[] = {80, 70, 40, 70, 40, 80,
                            90, 80, 90, 80, 60, 90,
                            120, 100, 100, 100, 100, 120,
                            130, 120, 120, 130, 120, 130};
    
const float tenora[] =  {0.5,0.2,0.18,0.32,0.1,0.5,
                         0.45,0.25,0.12,0.25,0.2,0.45,
                          0.4,0.2,0.1,0.25,0.2,0.4};
  

const float altof[] = {800,400,350,450,325,800,
                           1150,1600,1700,800,700,1150,
                           2800,2700,2700,2830,2530,2800,
                           3500,3300,3700,3500,3500,3500};


const float altob[] = {80, 60, 50, 70, 50, 80,
                        90, 80, 100, 80, 60, 90,
                           120, 120, 120, 100, 170, 120,
                          130, 150, 150, 130, 180, 130};
    
const float altoa[] =  {0.63,0.063,0.1,0.35,0.25,0.63,
                            0.1,0.031,0.031,0.15,0.031,0.1,
                            0.015,0.015,0.015,0.04,0.01,0.015};
  

const float soprf[] = {800,350,270,450,325,800,
                           1150,2000,2140,800,700,1150,
                           2900,2800,2950,2830,2700,2900,
                           3900,3600,3900,3800,3800,3900};


const float soprb[] = {80, 60, 60, 40, 50, 80,
                           90, 100, 90, 80, 60, 90,
                           120, 120, 100, 100, 170, 120,
                           130, 150, 120, 120, 180, 130};
    
const float sopra[] =  {0.5,0.1,0.25,0.28,0.15,0.5,
                        0.03,0.16,0.05,0.1,0.017,0.03,
                        0.1,0.01,0.05,0.1,0.01,0.01};



const float *frs[] = {(const float *) bassf, (const float *) tenorf,
                       (const float *) altof, (const float *) soprf}; 
const float *bws[] = {(const float *) bassb, (const float *) tenorb,
                       (const float *) altob, (const float *) soprb}; 
const float *amp[] = {(const float *) bassa, (const float *) tenora,
                       (const float *) altoa,  (const float *) sopra}; 


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
  float shft;
  int16_t smax;
  int16_t fno;
  float att, dec, amnt;
  float form;
  float scal;
  Env env;

  PSModFM() :  phase(0.f), sphase(0.f),shft(0.f), smax(0), fno(0), att(0.f),
               dec(0.f),amnt(0.f), form(0.f), scal(0.f), env() { };
    

  float formant(float ndx, float ff, float phase, float sphase, float mod) {
    const int m =  (uint32_t) ff;
    const float a = ff - m;
    float pc1 = phase * m + sphase;
    float pc2 = phase * (m + 1) + sphase;
    pc1 -= (uint32_t) pc1;
    pc2 -= (uint32_t) pc2; 
    return (a * osc_cosf(pc2) + (1.f - a) * osc_cosf(pc1)) * EXP(ndx * (mod - 1.f));
  }

  float mod_ndx(float fo, float kbw) {
    float g,gm;
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
  const float scal = obj.scal*10.f;
  const float amnt = obj.amnt*2.f;
  const int16_t fno = obj.fno;
  const int note = (params->pitch) >> 8;
  const float w0 = osc_w0f_for_note((params->pitch) >> 8, params->pitch & 0xFF);
  const float fo = w0 * k_samplerate;
  const float fo1 = 1.f/fo;
  const float ws = w0 * obj.shft * (1 + obj.smax);
  const float lfo = q31_to_f32(params->shape_lfo);
  float ff[4], ndx[4], bw, amps[3];
  float form = (lfo+obj.form)*5.f;

  const int sp1[] = {54, 52, 50, 48};
  const int sp2[] = {64, 62, 60, 58};
  const int sp3[] = {74, 72, 70, 68};
  
  float *bandwidth = (float *) (fno < 4 ? bws[fno] :
    (note < sp1[fno-4] ? bws[0] :
     (note < sp2[fno-4] ? bws[1] :
      (note < sp3[fno-4] ? bws[2] : bws[3]))));
  
  float *frequency =  (float *) (fno < 4 ? frs[fno] :
    (note < sp1[fno-4] ? frs[0] :
     (note < sp2[fno-4] ? frs[1] :
      (note < sp3[fno-4] ? frs[2] : frs[3]))));

  float *amplitudes = (float *)  (fno < 4 ? amp[fno] :
    (note < sp1[fno-4] ? amp[0] :
     (note < sp2[fno-4] ? amp[1] :
      (note < sp3[fno-4] ? amp[2] : amp[3]))));  

  while(form >= 5.f) form -= 5.f;
  while(form < 0)  form += 5.f;
  int n = (int) form;
  float frac = form - n;
  
  for (int k = 0; k < 4; k++) {
    bw = bandwidth[k*6+n] + frac*(bandwidth[k*6+n+1] - bandwidth[k*6+n]);
    ff[k] = frequency[k*6+n] + frac*(frequency[k*6+n+1] - frequency[k*6+n]);
    if(k) amps[k-1] = amplitudes[(k-1)*6+n] +
            frac*(amplitudes[(k-1)*6+n+1] - amplitudes[(k-1)*6+n]);
    ff[k] = ff[k] < fo ? 1. : ff[k]*fo1;
    float md = obj.mod_ndx(fo, bw);
    ndx[k] = md+scal;
  }
 
  float phase = obj.phase;
  float sphase = obj.sphase;
  
  for (int i = 0; i < frames; i++) {
    q31_t *__restrict y = (q31_t *) yn;
    float mod, e;
    e = 1.f + amnt * env.proc();
    mod = osc_cosf(phase);
    y[i] = f32_to_q31(.25f*(obj.formant(ndx[0],ff[0]*e,phase,sphase,mod) +
                       obj.formant(ndx[1],ff[1]*e,phase,sphase,mod)*amps[0] +
                       obj.formant(ndx[2],ff[2]*e,phase,sphase,mod)*amps[1] +
                       obj.formant(ndx[3],ff[3]*e,phase,sphase,mod)*amps[2]));
    phase += w0;
    phase -= (uint32_t) phase;
    sphase += ws;
    sphase -= (uint32_t) sphase;    
  }
  obj.phase = phase;
  obj.sphase = sphase;
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
    obj.fno = value;
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
    obj.form = valf;
    break;
  case k_user_osc_param_shiftshape:
    obj.scal = valf;
    break;
  default:
    break;
  }
}
