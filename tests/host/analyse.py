#!/usr/bin/env python3
# Sound checks: clicks, noisiness, level, and what makes a sea different from the others.
import numpy as np, sys, json
def feats(path, sr=44100):
    x=np.fromfile(path,dtype=np.float32).reshape(-1,2).mean(1).astype(float)
    n=len(x); rms=np.sqrt(np.mean(x**2))+1e-9
    # clicks: a jump far larger than the sound either side of it, in the 2nd difference, per minute
    d=np.abs(np.diff(x,2)); w=2205
    k=np.ones(w)/w; local=np.sqrt(np.convolve(d**2,k,'same'))+1e-5
    clicks=np.sum((d>12*local)&(d>.004)); 
    # spectrum
    N=8192; hop=N*2; S=[]
    win=np.hanning(N)
    for i in range(0,n-N,hop): S.append(np.abs(np.fft.rfft(x[i:i+N]*win))**2)
    S=np.array(S); P=S.mean(0)+1e-14; f=np.fft.rfftfreq(N,1/sr)
    c=np.sum(P*f)/np.sum(P)
    b=lambda a,z: 10*np.log10(np.sum(P[(f>=a)&(f<z)])+1e-14)
    t=P.sum()
    e=[10*np.log10(np.sum(P[(f>=a)&(f<z)])/t+1e-12) for a,z in ((20,100),(100,300),(300,1000),(1000,3000),(3000,9000))]
    sel=(f>1000)&(f<6000); flat=np.exp(np.mean(np.log(P[sel])))/np.mean(P[sel])
    # onsets and rhythm: the envelope's strongest periodicity between .2 and 4 s
    hopE=441; env=np.sqrt(np.convolve(x**2,np.ones(hopE)/hopE,'same'))[::hopE]; on=np.maximum(0,np.diff(env)); on-=on.mean()
    ac=np.correlate(on,on,'full')[len(on)-1:]; ac/=ac[0]+1e-12
    lo,hi=20,400; rhythm=float(ac[lo:hi].max()); period=(np.argmax(ac[lo:hi])+lo)/100
    onsets=np.sum(on>on.std()*3)/(n/sr)*60
    db=20*np.log10(env+1e-5); dyn=float(np.percentile(db,95)-np.percentile(db,10))
    return dict(rms=round(20*np.log10(rms),1),peak=round(20*np.log10(np.abs(x).max()+1e-9),1),clicks_min=round(clicks/(n/sr)*60,1),centroid=int(c),flat=round(float(flat),3),bands=[round(v,1) for v in e],rhythm=round(rhythm,2),period=round(period,2),onsets_min=int(onsets),dyn=round(dyn,1))
if __name__=='__main__':
    for p in sys.argv[1:]:
        print(p.split('/')[-1],json.dumps(feats(p)))
