AudioSource - drop raw sound files here (WAV preferred, 44.1/48 kHz, 16-bit).

Name each file EXACTLY like the slot it belongs to, then run Scripts\import_audio.py
(or ask Claude to). It imports into Content/Retake/Audio/Sounds, assigns the slot in
DA_RetakeAudio and adds a row stub to ASSETS.md (fill in source + license!).

Empty slots are simply silent, so add sounds in any order.

DIRECTOR VOICE (English)
  VoiceAction.wav          "Action!"
  VoiceCut.wav             "Cut!"
  VoiceTakeTwo.wav         "Take two!"   (played before "Action!")
  VoiceTakeThree.wav       "Take three!"
  VoiceTakeFour.wav        "Take four!"
  VoiceTakeFive.wav        "Take five!"
  VoiceThatsAWrap.wav      "That's a wrap!"
  VoicePrintIt.wav         "Print it!"   (results screen)
  VoiceFromTheTop.wav      "From the top!" (after Scene failed)
  VoiceNewStuntDouble.wav  "Get me a new stunt double!" (cut by explosion/laser/fall)

MUSIC (Music* loop automatically, except MusicDirectorsCut)
  MusicMenu.wav  MusicBriefing.wav  MusicRollingLoop.wav  MusicDirectorsCut.wav  MusicResults.wav

SFX
  SfxClapperboard.wav  SfxPlateOn.wav  SfxPlateOff.wav  SfxLever.wav
  SfxGateOpen.wav  SfxGateClose.wav  SfxExplosiveArm.wav  SfxExplosion.wav
  SfxGuardAlert.wav  SfxGuardCatch.wav  SfxCratePush.wav  SfxGoal.wav  SfxUIClick.wav

Volume sliders (Music / SFX / Voice) are in the main menu (call sheet).
Rules: no third-party IP, everything in English, list every external sound in ASSETS.md.
