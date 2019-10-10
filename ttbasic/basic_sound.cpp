// SPDX-License-Identifier: MIT
// Copyright (c) 2017-2019 Ulrich Hecht

#include "basic.h"

bool event_play_enabled;
uint8_t event_play_proc_idx[SOUND_CHANNELS];

void BASIC_INT Basic::event_handle_play(int ch) {
  if (event_play_proc_idx[ch] == NO_PROC)
    return;
  init_stack_frame();
  push_num_arg(ch);
  do_call(event_play_proc_idx[ch]);
  event_play_enabled = false;
}

#ifdef HAVE_MML
BString mml_text[SOUND_CHANNELS];
#endif
/***bc snd PLAY
Plays a piece of music in Music Macro Language (MML) using the
wavetable synthesizer.
\usage PLAY [ch, ]mml$
\args
@ch	sound channel [`0` to `{SOUND_CHANNELS_m1}`, default: `0`]
@mml$	score in MML format
\bugs
* MML syntax undocumented.
* Fails silently if the synthesizer could not be initialized.
***/
void Basic::iplay() {
#ifdef HAVE_MML
  int32_t ch = 0;

  if (!is_strexp())
    if (getParam(ch, 0, SOUND_CHANNELS - 1, I_COMMA))
      return;

  sound.stopMml(ch);
  mml_text[ch] = istrexp();
  if (err)
    return;

  sound.playMml(ch, mml_text[ch].c_str());
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

/***bc snd SAY
Speaks the given text.
\usage SAY text$
\args
@text$	text to be spoken
\note
`text$` is assumed to be in English.

`SAY` cannot be used concurrently with the wavetable synthesizer
because they use different sample rates.
\bugs
* Does not support phonetic input.
* No possibility to change synthesizer parameters.
***/
void Basic::isay() {
  BString text = istrexp();
  if (err)
    return;
  ESP8266SAM *sam = sound.sam();
  if (!sam) {
    err = ERR_OOM;
    return;
  }
  sam->Say(text.c_str());
}

/***bf snd PLAY
Checks if a sound is playing a on wavetable synthesizer channel.
\usage p = PLAY(channel)
\args
@channel	sound channel +
                [`0` to `{SOUND_CHANNELS_m1}`, or `-1` to check all channels]
\ret `1` if sound is playing, `0` otherwise.
\ref PLAY SOUND
***/
num_t BASIC_FP Basic::nplay() {
#ifdef HAVE_MML
  int32_t a, b;
  if (checkOpen())
    return 0;
  if (getParam(a, -1, SOUND_CHANNELS - 1, I_CLOSE))
    return 0;
  if (a == -1) {
    b = 0;
    for (int i = 0; i < SOUND_CHANNELS; ++i) {
      b |= sound.isPlaying(i);
    }
    return b;
  } else
    return sound.isPlaying(a);
#else
  err = ERR_NOT_SUPPORTED;
  return 0;
#endif
}

/***bf snd INST$
Returns the name of the specified wavetable synthesizer instrument.
\usage name$ = INST$(num)
\args
@num	instrument number
\ret
Instrument name.

If an instrument is not defined in the current sound font, an empty
string is returned.
\ref SOUND_FONT
***/
BString Basic::sinst() {
#ifdef HAVE_TSF
  return sound.instName(getparam());
#else
  err = ERR_NOT_SUPPORTED;
  return BString();
#endif
}

/***bc snd BEEP
Produces a sound using the "beeper" sound engine.
\usage BEEP period[, volume [, env$]]
\args
@period	tone cycle period in samples [`0` (off) to `320`/`160`]
@volume	tone volume +
        [`0` to `15`, default: as set in system configuration]
@env$	volume envelope
	[characters from `CHR$(0)` to `CHR$(15)`, default: constant volume]
\note
The maximum value for `period` depends on the flavor of Engine BASIC;
it's 320 for the gaming build, and 160 for the network build.
\ref CONFIG
***/
void Basic::ibeep() {
  int32_t period;
  int32_t vol = CONFIG.beep_volume;
  BString env;
  uint8_t *env_ptr = NULL;

  if (getParam(period, 0, SOUND_BUFLEN, I_NONE))
    return;
  if (*cip == I_COMMA) {
    cip++;
    if (getParam(vol, 0, 15, I_NONE))
      return;
  }
  if (*cip == I_COMMA) {
    cip++;
    env = istrexp();
    if (err)
      return;
    env_ptr = (uint8_t *)env.c_str();
  }
  if (period == 0)
    sound.noBeep();
  else
    sound.beep(period, vol, env_ptr);
}

/***bc snd SOUND FONT
Sets the sound font to be used by the wavetable synthesizer.
\usage SOUND FONT file$
\args
@file$	name of the SF2 sound font file
\note
If a relative path is specified, Engine BASIC will first attempt
to load the sound font from `/sd`, then from `/flash`.

The default sound font name is `1mgm.sf2`.
\bugs
No sanity checks are performed before setting the sound font.
\ref INST$()
***/

/***bc snd SOUND
Generates a sound using the wavetable synthesizer.
\usage SOUND ch, inst, note[, len[, vel]]
\args
@ch	sound channel [`0` to `{SOUND_CHANNELS_m1}`]
@inst	instrument number
@note	note pitch
@len	note duration, milliseconds [default: `1000`]
@vel	note velocity [`0` to `1` (default)]
\ref INST$() SOUND_FONT
***/
void Basic::isound() {
#ifdef HAVE_TSF
  if (*cip == I_FONT) {
    ++cip;
    BString font = getParamFname();
    sound.setFontName(font);
  } else {
    int32_t ch, inst, note, len = 1000;
    num_t vel = 1;
    if (getParam(ch, 0, SOUND_CHANNELS - 1, I_COMMA))
      return;
    int instcnt = sound.instCount();
    if (!instcnt) {
      err = ERR_TSF;
      return;
    }
    if (getParam(inst, 0, instcnt - 1, I_COMMA))
      return;
    if (getParam(note, 0, INT32_MAX, I_NONE))
      return;
    if (*cip == I_COMMA) {
      ++cip;
      if (getParam(len, 0, INT32_MAX, I_NONE))
        return;
      if (*cip == I_COMMA) {
        ++cip;
        if (getParam(vel, 0, 1.0, I_NONE))
          return;
      }
    }
    sound.noteOn(ch, inst, note, vel, len);
  }
#else
  err = ERR_NOT_SUPPORTED;
#endif
}
