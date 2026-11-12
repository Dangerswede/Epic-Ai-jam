# Retake – prioriterad lista efter intervjun (12 nov kväll)

Underlag: intervju med Markus 12 nov (svaren finns i bilagan längst ner). Redan beslutat och byggs i annan tråd: roller per take, tidsfönster 4–6 s, takes 15–20 s, ombyggnad scen 1–4, Manny i 70-talsdräkt + Tripo-hjälm. Listan nedan bygger ovanpå det.

**Markus bedömning:** Fun är svagast just nu (och Fun räknas dubbelt vid lika). Därför står det som ger fun först.
**Röd tråd:** 70-talsfilm i allt (text, färg, ljud, video).
**Feature freeze:** lör 14 nov 18:00. Stryk nerifrån och upp om tiden tar slut.

| # | Uppgift | Varför (Markus ord) | Kriterier | Storlek | När |
|---|---------|---------------------|-----------|---------|-----|
| F1 | Kamerapunkter i Director's Cut | Replay var tråkigast och hade konstiga vinklar | Fun, Tema, Visuals | M–L | fre |
| F2 | Svårighetskurva + en ny mekanik per scen | Roligast var att se vad nästa bana bjuder på | Fun, Level Design, Gameplay | M (med ombyggnaden) | fre |
| F3 | Explosioner som smäller | Explosioner kändes väldigt platta | Fun, Visuals | S–M | fre |
| F4 | 70-tals poppande texter | Texter ska animeras och poppa, tänk 70-tal | Visuals, Tema | S | fre |
| F5 | Ljudpass | Det finns inget ljud alls än | Audio | M | fre kväll–lör fm |
| F6 | 70-tals filmlook | Röd tråd 70-tal | Visuals, Tema | S | lör fm |
| F7 | Location scout + one-shot-scen | Gå runt och planera i vissa set, andra är one-shot | Fun, Ambition | S–M | lör fm |
| F8 | Överraskning mitt i scenen | Gillade förslaget | Fun, Ambition | M, osäker | bara om tid |
| V | Video som 70-talstrailer | Markus valde trailer | Alla | – | sön |

---

## F1 – Kamerapunkter: replayen blir betyget
Idé från Markus: det är en filminspelning, så kamerorna är bestämda och man får poäng för att synas på rätt plats.
- Varje scen får 3–5 fasta kameror (`ACameraActor` + en "mark" på golvet, som en tejpmarkering på ett filmset) med tidsfönster, t.ex. "Shot 2: 0:06–0:09, någon står på X".
- Markeringarna syns i världen under inspelning (tejp-kryss + liten kameraikon), så spelaren kan planera för dem.
- Director's Cut klipper mellan just de kamerorna. En poängmätare ("Director's rating") fylls på medan man ser replayen, när en take står på rätt mark i rätt bild. Träff = "PRINT IT!", miss = "...".
- Mätaren ger stjärnorna (ersätter eller ingår i Director's notes från P9).
- Replayen blir kort (bara kamerapunkternas fönster, 10–20 s) och går att hoppa över med Space.
- Klart när: replayen visar bara fasta vinklar, poängen tickar upp synligt och en domare förstår varför de fick 2 av 3 stjärnor.
- Fallback: en fast kamera per scen + ett slutbetyg utan animerad mätare.

## F2 – Svårighetskurva och en ny mekanik per scen
- Scen 1–2 ska gå fort (under 2 min totalt), de lär ut kontroller och roller.
- Scen 3 och framåt: varje scen introducerar exakt en ny sak (t.ex. tidsbrytare, vakt, explosiv, crate, fruset spöke) och kombinerar den med något gammalt.
- Sista scenen kräver att man planerar ordningen på takes, inte bara reaktion.
- Görs tillsammans med ombyggnaden av scen 1–4 (skicka till byggtråden).

## F3 – Explosioner som smäller
- Niagara-eldboll + rökpelare + gnistor/skräp, kort vit blixt, kameraskak, 0,3 s slow-mo (återanvänd P5:s slow-mo), fysikimpuls på närliggande props.
- Ljud: dov smäll + eko (från F5).
- Klart när: en explosion syns tydligt i en 1080p-skärmdump och känns i video.

## F4 – 70-tals poppande texter
- "CUT! Take N", "ACTION!", "PRINT IT!", "SCRAPPED!" m.fl. skalar in med översväng (0 → 1,3 → 1), lutar lite, skuggad kontur, ev. clapperboard som slår ihop.
- 70-talsfont med öppen licens (t.ex. Google Fonts, OFL) i `ASSETS.md`.
- Allt i UMG-animationer styrda från C++ (`URetakeHUDWidget`).

## F5 – Ljudpass
Ljudsystemet och `Scripts\import_audio.py` finns redan (filnamn = slot).
- Regissörsröst: "Quiet on set!", "Action!", "Cut!", "Print it!", "That's a wrap!", "Scrapped!", "Again!". Kan genereras med ElevenLabs (koppling finns här) eller fal.
- Effekter: clapperboard, explosion, tryckplatta, grind, spak, steg, film som rullar i projektor.
- Musik: 70-tals funk/action-loop. Bonus: ett lager till per take (bas → trummor → brass) så musiken byggs på när fler takes spelar.
- Alla källor i `ASSETS.md`. Allt på engelska.

## F6 – 70-tals filmlook
- Post-process: filmkorn, varm färggradering, lätt vinjett, svarta filmränder i Director's Cut.
- Valfritt: repor/flimmer bara i replayen, så den ser ut som en gammal filmrulle.

## F7 – Location scout och one-shot
- Före take 1 i vissa scener: "LOCATION SCOUT", fri rörelse utan inspelning, man ser kamerapunkterna (F1). Starta med Space.
- En scen (t.ex. bonusscenen The Gap) markeras "ONE-SHOT": ingen scout och max en take, för en tydlig kontrast.

## F8 – Överraskning mitt i scenen (bara om tid)
- Regissören skriver om manuset vid take 3: "REWRITE!", en ny vakt eller ett flyttat mål. De gamla takes måste fortfarande fungera, så man tänker om.
- Stryks först.

## V – Video (söndag)
Markus valde trailer: en 70-talstrailer, 30–60 s, 1080p. Ett trailerröst-intro ("One stuntman. One scene. Forty-seven takes."), gameplay-klipp med "CUT!"-stämplar, och sist en Director's Cut-replay med poängmätaren. Avsluta med titelkort "RETAKE".

---

## Bilaga: intervjusvar (12 nov kväll)

1. Roligast: nyfikenheten på vad nästa bana bjuder på för klurighet. Första banorna får gå fort, sedan måste det bli klurigare och mer komplext.
2. Tråkigast: titta på replays (Director's Cut). Gav inget och hade konstiga kameravinklar.
3. Replay: fasta kameravinklar (som riktig filminspelning). Poäng för att synas på rätt plats i bild. Poängen fylls på medan replayen spelas, så det blir kul att se hur bra man lyckades.
4. Klurighet: ny mekanik per bana, eller att man tvingas fundera på hur takes ska läggas upp. Vissa set kan man gå runt i och planera (location scout), andra är "one-shot" utan planering. Gillar även: kort/hoppbar replay, höjdpunkter, överraskningar mitt i scenen.
5. Ljud: finns inget än, så inga önskemål. (Audio är 1/7 av poängen, ljudsystem + import_audio.py finns, väntar på filer.)
6. Juice: explosioner kändes väldigt platta. Texter som dyker upp ska animeras och poppa mer. Stil: 70-talsfilm.
7. Svagast idag: Fun. "Det är inte ett spel om det inte finns fun." (Fun är tiebreak x2.)
8. Video: 70-talstrailer (Markus bekräftade).
