#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <Arduino.h>

const char WEBPAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Thermostat</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,BlinkMacSystemFont,sans-serif;background:#f5f5f5;padding:16px;max-width:480px;margin:0 auto}
h1{font-size:1.5rem;margin-bottom:16px;color:#333}
h2{font-size:1.1rem;margin:16px 0 8px;color:#555}
.card{background:#fff;border-radius:8px;padding:16px;margin-bottom:12px;box-shadow:0 1px 3px rgba(0,0,0,0.1)}
.status-grid{display:grid;grid-template-columns:1fr 1fr;gap:12px}
.status-item{text-align:center}
.status-value{font-size:1.8rem;font-weight:600;color:#333}
.status-label{font-size:0.8rem;color:#888;margin-top:4px}
.relay-on{color:#22c55e}
.relay-off{color:#888}
.override-badge{display:inline-block;padding:2px 8px;border-radius:4px;font-size:0.75rem;margin-left:8px}
.override-on{background:#22c55e;color:#fff}
.override-off{background:#ef4444;color:#fff}
.btn{display:inline-block;padding:10px 16px;border:none;border-radius:6px;font-size:0.9rem;cursor:pointer;margin:4px}
.btn-primary{background:#3b82f6;color:#fff}
.btn-success{background:#22c55e;color:#fff}
.btn-danger{background:#ef4444;color:#fff}
.btn-secondary{background:#e5e7eb;color:#333}
.btn:active{opacity:0.8}
.override-btns{display:flex;gap:8px;flex-wrap:wrap}
.schedule{display:flex;justify-content:space-between;align-items:center;padding:8px 0;border-bottom:1px solid #eee}
.schedule:last-child{border-bottom:none}
.schedule-info{flex:1}
.schedule-time{font-weight:500}
.schedule-temp{color:#666;font-size:0.9rem}
input,select{padding:8px;border:1px solid #ddd;border-radius:4px;font-size:1rem;width:100%}
.form-row{display:flex;gap:8px;margin-bottom:8px}
.form-row>*{flex:1}
label{display:block;font-size:0.8rem;color:#666;margin-bottom:4px}
.time{color:#888;font-size:0.9rem}
.error{color:#ef4444}
</style>
</head>
<body>
<h1>Thermostat <span class="time" id="time">--:--</span></h1>

<div class="card">
<div class="status-grid">
<div class="status-item">
<div class="status-value" id="temp">--</div>
<div class="status-label">Temperature °C</div>
</div>
<div class="status-item">
<div class="status-value" id="humidity">--</div>
<div class="status-label">Humidity %</div>
</div>
<div class="status-item">
<div class="status-value" id="relay">--</div>
<div class="status-label">Relay</div>
</div>
<div class="status-item">
<div class="status-value" id="schedule">--</div>
<div class="status-label">Schedule</div>
</div>
</div>
</div>

<div class="card">
<h2>Override</h2>
<div class="override-btns">
<button class="btn btn-success" onclick="setOverride('on')">Force ON</button>
<button class="btn btn-danger" onclick="setOverride('off')">Force OFF</button>
<button class="btn btn-secondary" onclick="setOverride('clear')">Clear</button>
</div>
</div>

<div class="card">
<h2>Schedules</h2>
<div id="schedules"></div>
<button class="btn btn-primary" onclick="showAddSchedule()" style="margin-top:8px">+ Add Schedule</button>
</div>

<div class="card" id="schedule-form" style="display:none">
<h2 id="form-title">Add Schedule</h2>
<div class="form-row">
<div><label>Slot</label><select id="sch-idx"></select></div>
</div>
<div class="form-row">
<div><label>From</label><input type="time" id="sch-from" value="08:00"></div>
<div><label>To</label><input type="time" id="sch-to" value="22:00"></div>
</div>
<div class="form-row">
<div><label>Heat ON below</label><input type="number" id="sch-open" step="0.5" value="20"></div>
<div><label>Heat OFF above</label><input type="number" id="sch-close" step="0.5" value="22"></div>
</div>
<div style="margin-top:8px">
<button class="btn btn-primary" onclick="saveSchedule()">Save</button>
<button class="btn btn-secondary" onclick="hideForm()">Cancel</button>
</div>
</div>

<div class="card">
<h2>Settings</h2>
<div class="form-row">
<div><label>Timezone (UTC offset)</label><input type="number" id="timezone" min="-12" max="14"></div>
<div style="display:flex;align-items:flex-end"><button class="btn btn-primary" onclick="saveTimezone()">Save</button></div>
</div>
</div>

<script>
let schedules=[];
function $(id){return document.getElementById(id)}

function updateStatus(){
fetch('/api/status').then(r=>r.json()).then(d=>{
$('temp').textContent=d.valid?d.temperature.toFixed(1):'--';
$('humidity').textContent=d.valid?d.humidity.toFixed(0):'--';
$('relay').textContent=d.relay?'ON':'OFF';
$('relay').className='status-value '+(d.relay?'relay-on':'relay-off');
$('schedule').textContent=d.schedule>=0?'#'+d.schedule:'None';
$('time').textContent=d.time;
if(d.override!=='none'){
$('relay').innerHTML+='<span class="override-badge override-'+d.override+'">'+d.override.toUpperCase()+'</span>';
}
}).catch(()=>{});
}

function updateSchedules(){
fetch('/api/schedules').then(r=>r.json()).then(data=>{
schedules=data;
let html='';
if(data.length===0)html='<p style="color:#888">No schedules configured</p>';
data.forEach(s=>{
html+='<div class="schedule"><div class="schedule-info">';
html+='<div class="schedule-time">#'+s.index+': '+s.from+' - '+s.to+'</div>';
html+='<div class="schedule-temp">Heat ON below '+s.open+'°C, OFF above '+s.close+'°C</div>';
html+='</div>';
html+='<button class="btn btn-secondary" onclick="editSchedule('+s.index+')">Edit</button>';
html+='<button class="btn btn-danger" onclick="delSchedule('+s.index+')">Del</button></div>';
});
$('schedules').innerHTML=html;
}).catch(()=>{});
}

function updateConfig(){
fetch('/api/config').then(r=>r.json()).then(d=>{
$('timezone').value=d.timezone;
}).catch(()=>{});
}

function setOverride(state){
fetch('/api/override',{method:'POST',body:JSON.stringify({state:state})})
.then(()=>updateStatus());
}

function showAddSchedule(){
$('form-title').textContent='Add Schedule';
let opts='';
for(let i=0;i<8;i++){
let used=schedules.find(s=>s.index===i);
opts+='<option value="'+i+'"'+(used?' disabled':'')+'>Slot '+i+(used?' (used)':'')+'</option>';
}
$('sch-idx').innerHTML=opts;
$('sch-from').value='08:00';
$('sch-to').value='22:00';
$('sch-open').value='20';
$('sch-close').value='22';
$('schedule-form').style.display='block';
}

function editSchedule(idx){
let s=schedules.find(x=>x.index===idx);
if(!s)return;
$('form-title').textContent='Edit Schedule #'+idx;
$('sch-idx').innerHTML='<option value="'+idx+'">Slot '+idx+'</option>';
$('sch-from').value=s.from;
$('sch-to').value=s.to;
$('sch-open').value=s.open;
$('sch-close').value=s.close;
$('schedule-form').style.display='block';
}

function hideForm(){$('schedule-form').style.display='none';}

function saveSchedule(){
let data={
index:parseInt($('sch-idx').value),
from:$('sch-from').value,
to:$('sch-to').value,
open:parseFloat($('sch-open').value),
close:parseFloat($('sch-close').value)
};
fetch('/api/schedule',{method:'POST',body:JSON.stringify(data)})
.then(r=>{if(!r.ok)throw new Error();hideForm();updateSchedules();})
.catch(()=>alert('Failed to save schedule'));
}

function delSchedule(idx){
if(!confirm('Delete schedule #'+idx+'?'))return;
fetch('/api/schedule?index='+idx,{method:'DELETE'})
.then(()=>updateSchedules());
}

function saveTimezone(){
let tz=parseInt($('timezone').value);
fetch('/api/config',{method:'POST',body:JSON.stringify({timezone:tz})})
.then(r=>{if(!r.ok)throw new Error();alert('Timezone saved');})
.catch(()=>alert('Failed to save'));
}

updateStatus();
updateSchedules();
updateConfig();
setInterval(updateStatus,5000);
</script>
</body>
</html>
)rawliteral";

#endif
