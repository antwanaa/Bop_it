#ifndef AUDIO_PLAYBACK_H
#define AUDIO_PLAYBACK_H

/* QSPI Flash function */
void Audio_Init();

/* Audio playback functions */
void play_bop_it_sample();
void play_twist_it_sample();
void play_blow_it_sample();
void play_success_sample();
void play_fail_sample();
void play_start_part1_sample();
void play_start_part2_sample();
void play_win_sample();

#endif