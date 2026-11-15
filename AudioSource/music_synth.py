import numpy as np, wave, sys
SR=44100
rng=np.random.default_rng(7)
def m2f(m): return 440*2**((m-69)/12)
def lowpass(x,fc,q=0.0):
    X=np.fft.rfft(x); f=np.fft.rfftfreq(len(x),1/SR)
    H=1/np.sqrt(1+(f/fc)**4); return np.fft.irfft(X*H,len(x))
def highpass(x,fc):
    X=np.fft.rfft(x); f=np.fft.rfftfreq(len(x),1/SR)
    H=(f/fc)**2/np.sqrt(1+(f/fc)**4); return np.fft.irfft(X*H,len(x))
class Buf:
    def __init__(s,n): s.n=n; s.L=np.zeros(n); s.R=np.zeros(n)
    def add(s,start,sig,pan=0.0,g=1.0):
        idx=(np.arange(len(sig))+int(start))%s.n   # circular -> seamless loop
        l=np.cos((pan+1)*np.pi/4)*g; r=np.sin((pan+1)*np.pi/4)*g
        np.add.at(s.L,idx,sig*l); np.add.at(s.R,idx,sig*r)
def env(n,a,d,s,rl,hold):
    t=np.arange(n)/SR; e=np.where(t<a,t/a,np.where(t<a+d,1-(1-s)*(t-a)/d,s))
    e=np.where(t>hold,e*np.exp(-(t-hold)/rl),e); return e
def rhodes(f,dur):
    n=int((dur+1.2)*SR); t=np.arange(n)/SR
    mi=1.8*np.exp(-t*6)+0.15
    sig=np.sin(2*np.pi*f*t+mi*np.sin(2*np.pi*f*t))+0.25*np.sin(2*np.pi*2*f*t)*np.exp(-t*3)
    sig+=0.08*np.sin(2*np.pi*f*14*t)*np.exp(-t*40)  # tine ping
    sig*=np.exp(-t*1.4)*np.minimum(1,t/0.004)
    rel=np.where(t>dur,np.exp(-(t-dur)*8),1); trem=1+0.15*np.sin(2*np.pi*4.5*t)
    return sig*rel*trem
def bass(f,dur,bright=1.0):
    n=int((dur+0.15)*SR); t=np.arange(n)/SR
    sig=np.sin(2*np.pi*f*t)+0.35*np.sin(2*np.pi*2*f*t)*bright+0.12*np.sin(2*np.pi*3*f*t)*bright
    e=np.minimum(1,t/0.006)*np.exp(-t*2.2)*np.where(t>dur,np.exp(-(t-dur)*40),1)
    return sig*e
def noise_hit(dur,fc_lo,fc_hi,decay):
    n=int(dur*SR); x=rng.standard_normal(n); x=highpass(lowpass(x,fc_hi),fc_lo)
    t=np.arange(n)/SR; return x*np.exp(-t*decay)*np.minimum(1,t/0.002)
def kick(g=1):
    n=int(0.35*SR); t=np.arange(n)/SR; f=50+90*np.exp(-t*30)
    return g*np.sin(2*np.pi*np.cumsum(f)/SR)*np.exp(-t*9)
def saw(f,n):
    t=np.arange(n)/SR; out=np.zeros(n)
    for k in range(1,14): out+=np.sin(2*np.pi*k*f*t*(1+0.0007*np.sin(2*np.pi*0.3*t)))/k
    return out
def finish(b,name,target=0.8):
    L,R=b.L,b.R; pk=max(abs(L).max(),abs(R).max()); L,R=L/pk*1.2,R/pk*1.2
    L,R=np.tanh(L),np.tanh(R); pk=max(abs(L).max(),abs(R).max()); L,R=L/pk*target,R/pk*target
    d=(np.stack([L,R],1)*32767).astype('<i2')
    w=wave.open(name,'wb'); w.setnchannels(2); w.setsampwidth(2); w.setframerate(SR); w.writeframes(d.tobytes()); w.close()

def menu(name):
    bpm=74; beat=60/bpm; bars=16; n=int(round(bars*4*beat*SR)); b=Buf(n)
    prog=[(50,[53,57,60,64]),(55,[53,57,59,64]),(48,[52,55,59,62]),(57,[55,60,64,67]),
          (50,[53,57,60,65]),(55,[53,59,62,65]),(52,[55,59,62,67]),(57,[56,60,64,67])]  # Dm9 G13 Cmaj9 Am7 Dm G7 Em7 A7
    for bar in range(bars):
        root,ch=prog[bar%8]; t0=bar*4*beat
        # rhodes: chord on 1, soft re-strike on 2&
        for i,m in enumerate(ch):
            b.add((t0+i*0.012)*SR, rhodes(m2f(m),beat*1.6)*0.22, pan=-0.3+0.2*i)
            b.add((t0+2.5*beat+i*0.01)*SR, rhodes(m2f(m),beat*1.4)*0.13, pan=-0.3+0.2*i)
        # bass: root, fifth, approach
        for bt,mm,d in [(0,root-12,1.4),(2,root-5,0.9),(3.5,root-13 if bar%2 else root-10,0.4)]:
            b.add((t0+bt*beat)*SR, bass(m2f(mm),d*beat)*0.5, pan=0.0)
        # brushes: swish on 2 and 4, soft ticks swing
        for bt in [1,3]: b.add((t0+bt*beat)*SR, noise_hit(0.35,2500,7000,9)*0.07, pan=0.25)
        for bt in [0,0.66,1.66,2,2.66,3.66]: b.add((t0+bt*beat)*SR, noise_hit(0.08,5000,11000,45)*0.05, pan=0.35)
        for bt in [0,2.5]: b.add((t0+bt*beat)*SR, kick(0.35), pan=0)
        # simple melody on vibraphone-ish tone every other bar
    mel=[(0,69),(1.5,72),(2,74),(4,72),(6,69),(8,67),(9.5,69),(10,71),(12,72),(14,74),(16,76),(18,74),(19,72),(20,69),(24,67),(26,69)]
    for start in [16*beat*2, 16*beat*6]:
        for bt,m in mel:
            if bt<32: 
                t=np.arange(int(1.6*SR))/SR; s=np.sin(2*np.pi*m2f(m)*t)*np.exp(-t*2.5)*(1+0.3*np.sin(2*np.pi*5*t))*np.minimum(1,t/0.01)
                b.add((start+bt*beat)*SR, s*0.12, pan=0.3)
    # tape hiss
    hiss=lowpass(rng.standard_normal(n),6000)*0.004; b.L+=hiss; b.R+=np.roll(hiss,999)
    finish(b,name,0.7)

def rolling(name):
    bpm=112; beat=60/bpm; bars=16; n=int(round(bars*4*beat*SR)); b=Buf(n); s16=beat/4
    roots=[45,45,48,43]  # A A C G per 2 bars... per bar cycle of 4
    pat=[(0,0,0.9),(3,0,0.5),(6,12,0.4),(8,0,0.9),(10,10,0.5),(11,7,0.5),(14,3,0.4)]  # 16th idx, interval, len in 16ths*
    for bar in range(bars):
        r=roots[bar%4]; t0=bar*4*beat
        for i,iv,ln in pat:
            b.add((t0+i*s16)*SR, bass(m2f(r-12+iv),ln*s16*1.6,1.6)*0.55, pan=0)
        for k in range(16):
            acc=0.06 if k%2 else 0.035
            if k in (6,14): b.add((t0+k*s16)*SR, noise_hit(0.18,6000,12000,18)*0.05, pan=0.4)  # open hat
            else: b.add((t0+k*s16)*SR, noise_hit(0.05,7000,13000,70)*acc, pan=0.4)
        for bt in [0,1.75,2.5]: b.add((t0+bt*beat)*SR, kick(0.55 if bar>=4 else 0.3), pan=0)
        if bar>=4:
            for bt in [1,3]: b.add((t0+bt*beat)*SR, noise_hit(0.2,900,5000,22)*0.16, pan=-0.05)  # snare
        if bar>=8 and bar%2==1:  # stab
            for m in [57,60,64]: b.add((t0+3.5*beat)*SR, rhodes(m2f(m),0.2)*0.12, pan=-0.4)
    # low string pad: A minor / F / C / G sustained per bar, filtered saw, swelling
    padch=[[57,60,64],[57,60,64],[55,60,64],[55,59,62]]
    for bar in range(bars):
        nb=int(4*beat*SR); t=np.arange(nb)/SR; sig=np.zeros(nb)
        for m in padch[bar%4]: sig+=saw(m2f(m-12),nb)
        sig=lowpass(sig,700+600*(bar%4)/3)*np.sin(np.pi*np.arange(nb)/nb)**0.6
        g=0.05+0.04*(bar>=8)
        b.add(bar*4*beat*SR, sig*g, pan=-0.35); b.add(bar*4*beat*SR+300, sig*g, pan=0.35)
    # ticking clock high pulse on quarters in bars 12-15
    for bar in range(12,16):
        for bt in range(4):
            t=np.arange(int(0.05*SR))/SR; s=np.sin(2*np.pi*1800*t)*np.exp(-t*120)
            b.add((bar*4*beat+bt*beat)*SR, s*0.08, pan=0.2)
    finish(b,name,0.75)
menu(sys.argv[1]); rolling(sys.argv[2])
