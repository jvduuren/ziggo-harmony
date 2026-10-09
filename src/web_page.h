#pragma once

#include <Arduino.h>

const char kIndexHtml[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Ziggo Remote Bridge</title>
<style>
:root{--bg:#f6f7f9;--card:#fff;--fg:#1b1f24;--muted:#6a737d;--line:#e3e6ea;--accent:#e36209;--ok:#1a7f37;--bad:#cf222e}
@media (prefers-color-scheme:dark){:root{--bg:#0f1115;--card:#171a20;--fg:#e6e8eb;--muted:#9aa4af;--line:#2a2f37;--accent:#f0883e;--ok:#3fb950;--bad:#f85149}}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--fg);font:15px/1.45 system-ui,-apple-system,Segoe UI,Roboto,sans-serif}
main{max-width:760px;margin:0 auto;padding:16px}
h1{font-size:20px;margin:4px 0 12px}h2{font-size:16px;margin:0 0 10px}
.card{background:var(--card);border:1px solid var(--line);border-radius:10px;padding:14px;margin-bottom:12px}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(160px,1fr));gap:8px 16px}
.k{color:var(--muted);font-size:12px}.v{font-weight:600;word-break:break-all}
.ok{color:var(--ok)}.bad{color:var(--bad)}.muted{color:var(--muted)}
table{width:100%;border-collapse:collapse}td{padding:6px 4px;border-top:1px solid var(--line);vertical-align:middle}
td.code{font:12px ui-monospace,Menlo,Consolas,monospace;color:var(--muted);word-break:break-all}
td.act{white-space:nowrap;text-align:right}
button{font:inherit;font-size:13px;padding:5px 10px;border-radius:6px;border:1px solid var(--line);background:var(--card);color:var(--fg);cursor:pointer}
button.primary{background:var(--accent);border-color:var(--accent);color:#fff}button.danger{color:var(--bad)}
#banner{display:none;background:var(--accent);color:#fff}#banner button{float:right;background:transparent;color:#fff;border-color:#fff}
textarea{width:100%;min-height:110px;font:12px ui-monospace,Menlo,Consolas,monospace;background:var(--bg);color:var(--fg);border:1px solid var(--line);border-radius:6px;padding:8px}
.row{display:flex;gap:8px;flex-wrap:wrap;align-items:center}pre{margin:0;font-size:12px;white-space:pre-wrap;word-break:break-all}
</style>
</head>
<body>
<main>
<h1>Ziggo Remote Bridge</h1>

<div class="card" id="banner"><button onclick="post('/api/learn/cancel')">Cancel</button>
Press the button for <b id="learnKey"></b> on your remote now&hellip;</div>

<div class="card"><div class="grid">
<div><div class="k">Box (Bluetooth)</div><div class="v" id="ble">&hellip;</div></div>
<div><div class="k">Last IR code</div><div class="v" id="ir">&ndash;</div></div>
<div><div class="k">Wi-Fi</div><div class="v" id="wifi">&ndash;</div></div>
<div><div class="k">Firmware</div><div class="v" id="ver">&ndash;</div></div>
</div></div>

<div class="card"><h2>Buttons</h2>
<p class="muted">Click <b>Learn</b>, then press the matching button on your IR remote (e.g. Harmony).
<b>Test</b> sends the key to the box directly.</p>
<table id="keys"></table></div>

<div class="card"><h2>Share or restore mapping</h2>
<textarea id="map" spellcheck="false"></textarea>
<div class="row" style="margin-top:8px"><button onclick="loadMap()">Show current</button>
<button onclick="saveMap()">Import</button>
<button class="danger" onclick="confirm('Forget all learned IR codes?')&&post('/api/clear-all').then(load)">Clear all</button></div></div>

<div class="card"><h2>Maintenance</h2>
<div class="row">
<button class="danger" onclick="confirm('Forget the paired box? You will need to pair again.')&&post('/api/unpair')">Unpair box</button>
<button class="danger" onclick="confirm('Forget Wi-Fi settings and restart?')&&post('/api/wifi-reset')">Reset Wi-Fi</button>
</div>
<form id="fw" style="margin-top:12px" class="row"><input type="file" name="firmware" accept=".bin">
<button class="primary">Update firmware</button><span id="fwstate" class="muted"></span></form>
<details style="margin-top:12px"><summary class="muted">Messages from the box</summary><pre id="box"></pre></details>
<details id="logbox" style="margin-top:8px"><summary class="muted">Log</summary><pre id="log"></pre></details>
</div>
</main>
<script>
const $=id=>document.getElementById(id);
const post=(u,body)=>fetch(u,{method:'POST',body});
const esc=s=>String(s).replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));
async function load(){
  const keys=await (await fetch('/api/keys')).json();
  $('keys').innerHTML=keys.map(k=>`<tr><td>${esc(k.label)}</td><td class="code">${k.ir?esc(k.ir):'<span class="muted">not learned</span>'}</td>
  <td class="act"><button onclick="post('/api/send?key=${k.id}')">Test</button>
  <button class="primary" onclick="post('/api/learn?key=${k.id}')">Learn</button>
  ${k.ir?`<button class="danger" onclick="post('/api/clear?key=${k.id}').then(load)">&times;</button>`:''}</td></tr>`).join('');
}
let learning=null;
async function status(){
  try{
    const s=await (await fetch('/api/status')).json();
    const b=s.ble;
    $('ble').innerHTML=b.state==='connected'?`<span class="ok">connected</span> <span class="muted">${esc(b.peer)}</span>`
      :`<span class="bad">${esc(b.state)}</span> <span class="muted">${b.bonds?'paired, waiting for box':'not paired'}</span>`;
    $('ir').textContent=s.ir.last?`${s.ir.last}${s.ir.key?' → '+s.ir.key:' (unknown)'}`:'–';
    $('wifi').textContent=`${s.wifi.ssid} (${s.wifi.ip}, ${s.wifi.rssi} dBm)`;
    $('ver').textContent=`${s.version}, up ${Math.round(s.uptime/60)} min`;
    $('box').textContent=s.box||'none yet';
    $('banner').style.display=s.learning?'block':'none';
    if(s.learning)$('learnKey').textContent=s.learning;
    if(learning&&!s.learning)load();
    learning=s.learning;
    if($('logbox').open)$('log').textContent=await (await fetch('/api/log')).text();
  }catch(e){$('ble').textContent='offline';}
}
async function loadMap(){$('map').value=await (await fetch('/api/map')).text();}
async function saveMap(){const r=await post('/api/map',$('map').value);alert(await r.text());load();}
$('fw').onsubmit=async e=>{e.preventDefault();const f=new FormData($('fw'));$('fwstate').textContent='uploading…';
  const r=await fetch('/update',{method:'POST',body:f});$('fwstate').textContent=await r.text();};
load();status();setInterval(status,1000);
</script>
</body>
</html>
)rawliteral";
