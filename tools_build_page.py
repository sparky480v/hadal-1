import base64,re,sys
p=open('web/emu.src.html').read();w=base64.b64encode(open('app/build/emu/app.wasm','rb').read()).decode()
swap='true' if '--swap' in sys.argv else 'false'
p=p.replace('/*{{WASM}}*/',w).replace('/*{{SWAP}}*/false',swap)
open('app/build/emu/hadal-emulator.html','w').write(p);print(len(p))
