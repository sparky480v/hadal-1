const fs=require('fs');const buf=fs.readFileSync(process.argv[2]);
(async()=>{const m=await WebAssembly.compile(buf);console.log('imports',WebAssembly.Module.imports(m).map(x=>x.module+'.'+x.name).slice(0,20));
const i=await WebAssembly.instantiate(m,{env:new Proxy({},{get:()=>()=>0})});const e=i.exports;e.web_master(3000);e.web_boot();
const mem=e.memory;let t0=Date.now();let peak=0,sum=0,n=0;for(let k=0;k<44100*20/1024;k++){e.web_render(1024);const l=new Float32Array(mem.buffer,e.web_out_l(),1024);for(const x of l){peak=Math.max(peak,Math.abs(x));sum+=x*x;n++}}
console.log('speed x',(20000/(Date.now()-t0)).toFixed(1),'peak',peak.toFixed(3),'rms',Math.sqrt(sum/n).toFixed(4),'blits',e.web_blits());
const fb=new Uint16Array(mem.buffer,e.web_fb(),57600);let nz=0;for(const v of fb)if(v)nz++;console.log('nonblack px',nz)})()
