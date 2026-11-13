// Power cycle in the wasm build: GLOBAL set to headphones and message off, kept, restored into a fresh boot.
const fs=require('fs');const buf=fs.readFileSync('app/build/emu/app.wasm');
const run=(e,ms)=>{for(let i=0;i<ms/23;i++)e.web_render(1024)};
const tap=(e,b)=>{e.web_buttons(1<<b);run(e,120);e.web_buttons(0);run(e,120)};
const png=(e,n)=>fs.writeFileSync(n,Buffer.from(new Uint16Array(e.memory.buffer,e.web_fb(),57600).slice().buffer));
(async()=>{let i=await WebAssembly.instantiate(buf,{});let e=i.instance.exports;e.web_master(3000);e.web_boot();run(e,1200);png(e,'/tmp/claude-0/p0.raw');
tap(e,5);e.web_enc(3,-1);run(e,150);e.web_enc(0,1);run(e,150);e.web_enc(3,1);run(e,200);tap(e,5);run(e,300);
console.log('store len',e.web_store_len(1));const b=new Uint8Array(e.memory.buffer,e.web_store(1),e.web_store_len(1)).slice();console.log('panel',[...b.slice(4,9)]);
i=await WebAssembly.instantiate(buf,{});e=i.instance.exports;new Uint8Array(e.memory.buffer,e.web_store(1),b.length).set(b);e.web_store_set_len(1,b.length);e.web_master(3000);e.web_boot();run(e,1200);png(e,'/tmp/claude-0/p1.raw');
tap(e,5);run(e,400);png(e,'/tmp/claude-0/p2.raw');
b[5]=1;i=await WebAssembly.instantiate(buf,{});e=i.instance.exports;new Uint8Array(e.memory.buffer,e.web_store(1),b.length).set(b);e.web_store_set_len(1,b.length);e.web_master(3000);e.web_boot();run(e,1200);png(e,'/tmp/claude-0/p3.raw');})()
