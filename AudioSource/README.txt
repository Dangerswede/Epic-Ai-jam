AudioSource - drop raw sound files here (WAV preferred, 44.1/48 kHz, 16-bit, mono for voice/SFX).

Name each file EXACTLY as listed, then run Scripts\import_audio.py (or ask Claude to). It imports into
Content/Retake/Audio/Sounds, wires the slot and adds a row stub to ASSETS.md (fill in source + license!).
Missing files are simply silent, so they can arrive in any order.
Rules: no third-party IP, English only, every external sound listed in ASSETS.md.

=== MINIMUM SET (41 files) ===

DIRECTOR VOICE - fixed lines (6)
  VoiceAction.wav           "Action!"                      every take
  VoiceCut.wav              "Cut!"                         every cut
  VoiceThatsAWrap.wav       "That's a wrap!"               reaching the goal (not in scene 16)
  VoicePrintIt.wav          "Print it!"                    results screen
  VoiceFromTheTop.wav       "From the top!"                take 1 after a restart
  VoiceNewStuntDouble.wav   "Get me a new stunt double!"   take ends in a stunt death

DIRECTOR VOICE - story lines (14 scene lines + 1 cut line), calm/tired, about Jack, never "you"
  VoiceStory01.wav  "Alright, people. Same as yesterday. Same as every day."
  VoiceStory02.wav  "Has anyone seen Jack? ...Fine. Roll it anyway."
  VoiceStory03.wav  "Somebody check that gap rig. Somebody. Anybody?"
  VoiceStory04.wav  "Take forty-something. I stopped counting. Action."
  VoiceStory05.wav  "Has anyone seen Jack?"
  VoiceStory06.wav  "Jack loved the saloon gags. Roll it."
  VoiceStory07.wav  "What time is it? ...Never mind. Action."
  VoiceStory08.wav  "Same as yesterday. Same as every day."
  VoiceStory09.wav  "Nobody's supposed to be out here tonight."
  (scene 10: silent)
  VoiceStory11.wav  "Again. In the dark, this time."
  VoiceStory12.wav  "Somebody check that rig. Anybody?"
  VoiceStory13.wav  "I've watched this a thousand times."
  VoiceStory14.wav  "One more. Jack always said one more."
  VoiceStory15.wav  "We should have used the net."
  (scene 16: silent)
  VoiceStoryCut.wav "Again. It has to be perfect this time."   (some CUTs from scene 9 on)

DIRECTOR VOICE - ending of scene 16 (2)
  VoiceStoryWrap1.wav  "You don't have to do it again, Jack."   (calm)
  VoiceStoryWrap2.wav  "That's a wrap."                         (calm, quiet)

SFX (13)
  SfxClapperboard.wav   take starts
  SfxPlateOn.wav        pressure plate / timed switch on
  SfxPlateOff.wav       pressure plate off
  SfxLever.wav          lever pulled
  SfxGateOpen.wav       gate opens
  SfxGateClose.wav      gate closes
  SfxExplosiveArm.wav   fuse lit
  SfxExplosion.wav      explosion
  SfxGuardAlert.wav     guard spots someone
  SfxGuardCatch.wav     guard catches someone
  SfxCratePush.wav      crate starts moving
  SfxGoal.wav           goal reached
  SfxUIClick.wav        menu buttons

MUSIC - 2 loops (loop automatically)
  MusicMenu.wav         calm 70s loop: menu, briefing, Director's Cut and results
  MusicRollingLoop.wav  tense loop while a take is rolling (the scene 16 wrap drops it to silence)

=== OPTIONAL (only if there is time) ===
  MusicBriefing.wav  MusicDirectorsCut.wav (one-shot)  MusicResults.wav   - replace MusicMenu in those moments

Volume sliders (Music / SFX / Voice) are in the main menu (call sheet).
