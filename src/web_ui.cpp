#include "web_ui.h"

const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 Web Oscilloscope</title>
<style>
:root{--bg:#0d1117;--panel:#161b22;--grid:#263040;--trace:#3fb950;--text:#c9d1d9;--muted:#8b949e;--trig:#d29922}
body{margin:0;background:var(--bg);color:var(--text);font:14px system-ui,sans-serif}
main{max-width:960px;margin:auto;padding:12px}
canvas{width:100%;background:var(--panel);border-radius:6px;display:block}
.controls{display:flex;flex-wrap:wrap;gap:10px;margin:10px 0;align-items:end}
label{display:flex;flex-direction:column;font-size:12px;color:var(--muted);gap:4px}
select,input,button{background:var(--panel);color:var(--text);border:1px solid var(--grid);border-radius:4px;padding:6px}
button{cursor:pointer}
.stats{display:grid;grid-template-columns:repeat(auto-fit,minmax(120px,1fr));gap:8px}
.stat{background:var(--panel);padding:8px;border-radius:6px}.stat b{display:block;font-size:18px}
</style></head><body><main>
<h2>ESP32 Web Oscilloscope</h2>
<canvas id="scope" width="960" height="400"></canvas>
<div class="controls">
<label>Sample rate (Hz)<select id="rate"><option>1000</option><option>5000</option><option>10000</option><option selected>20000</option><option>50000</option></select></label>
<label>Trigger<select id="edge"><option value="rising">Rising</option><option value="falling">Falling</option><option value="none">Free run</option></select></label>
<label>Level (V)<input id="level" type="number" step="0.1" value="1.6"></label>
<label>Volts/div<select id="vdiv"><option>0.2</option><option selected>0.5</option><option>1</option></select></label>
<button id="run">Stop</button><button id="single">Single</button>
</div>
<div class="stats">
<div class="stat">Vpp<b id="vpp">-</b></div><div class="stat">Vmin / Vmax<b id="vmm">-</b></div>
<div class="stat">Vavg<b id="vavg">-</b></div><div class="stat">Vrms<b id="vrms">-</b></div>
<div class="stat">Frequency<b id="freq">-</b></div><div class="stat">Duty cycle<b id="duty">-</b></div>
</div></main>
<script>
const VREF=3.1,FULL=4095,cv=document.getElementById('scope'),cx=cv.getContext('2d');
let running=true,busy=false;const $=id=>document.getElementById(id);
const toV=r=>r*VREF/FULL;
function measure(v,rate){
  const mn=Math.min(...v),mx=Math.max(...v),avg=v.reduce((a,b)=>a+b)/v.length;
  const rms=Math.sqrt(v.reduce((a,b)=>a+b*b,0)/v.length),mid=(mn+mx)/2;
  const ups=[];let high=0;
  for(let i=1;i<v.length;i++){if(v[i-1]<mid&&v[i]>=mid)ups.push(i);if(v[i]>=mid)high++}
  const f=(mx-mn)>0.05&&ups.length>1?rate*(ups.length-1)/(ups[ups.length-1]-ups[0]):null;
  return{mn,mx,avg,rms,f,duty:high/(v.length-1)};
}
function draw(v,rate,trig){
  const W=cv.width,H=cv.height,vdiv=+$('vdiv').value,divs=8,lvl=+$('level').value;
  cx.clearRect(0,0,W,H);cx.strokeStyle='#263040';cx.lineWidth=1;
  for(let i=0;i<=10;i++){cx.beginPath();cx.moveTo(i*W/10,0);cx.lineTo(i*W/10,H);cx.stroke()}
  for(let i=0;i<=divs;i++){cx.beginPath();cx.moveTo(0,i*H/divs);cx.lineTo(W,i*H/divs);cx.stroke()}
  const y=x=>H-x/(vdiv*divs)*H;
  cx.strokeStyle='#d29922';cx.setLineDash([4,4]);
  cx.beginPath();cx.moveTo(0,y(lvl));cx.lineTo(W,y(lvl));cx.stroke();
  cx.beginPath();cx.moveTo(W/4,0);cx.lineTo(W/4,H);cx.stroke();cx.setLineDash([]);
  cx.strokeStyle='#3fb950';cx.lineWidth=2;cx.beginPath();
  v.forEach((s,i)=>{const px=i*W/(v.length-1);i?cx.lineTo(px,y(s)):cx.moveTo(px,y(s))});cx.stroke();
  cx.fillStyle='#8b949e';cx.font='12px monospace';
  cx.fillText(`${(v.length/rate*100).toFixed(2)} ms/div   ${vdiv} V/div   ${trig?'TRIG':'AUTO'}`,8,16);
}
async function acquire(){
  if(busy)return;busy=true;
  try{
    const lvl=Math.round(+$('level').value/VREF*FULL);
    const r=await fetch(`/capture?rate=${$('rate').value}&edge=${$('edge').value}&level=${lvl}`);
    const d=await r.json(),v=d.samples.map(toV),m=measure(v,d.rate);
    draw(v,d.rate,d.triggered);
    $('vpp').textContent=(m.mx-m.mn).toFixed(3)+' V';$('vmm').textContent=`${m.mn.toFixed(2)} / ${m.mx.toFixed(2)} V`;
    $('vavg').textContent=m.avg.toFixed(3)+' V';$('vrms').textContent=m.rms.toFixed(3)+' V';
    $('freq').textContent=m.f?(m.f>=1000?(m.f/1000).toFixed(3)+' kHz':m.f.toFixed(1)+' Hz'):'-';
    $('duty').textContent=m.f?(m.duty*100).toFixed(1)+' %':'-';
  }catch(e){console.error(e)}
  busy=false;
}
async function loop(){while(running){await acquire();await new Promise(r=>setTimeout(r,50))}}
$('run').onclick=()=>{running=!running;$('run').textContent=running?'Stop':'Run';if(running)loop()};
$('single').onclick=()=>{running=false;$('run').textContent='Run';acquire()};
loop();
</script></body></html>)HTML";
