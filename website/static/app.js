"use strict";

// ──────────────────────────────────────────────────────────────────
//  Elements
// ──────────────────────────────────────────────────────────────────
const $ = id => document.getElementById(id);

const dotConn      = $("dot-conn");
const dotEsp       = $("dot-esp");
const lblConn      = $("lbl-conn");
const lblEsp       = $("lbl-esp");
const lblRtc       = $("lbl-rtc");
const lblTemp      = $("lbl-temp");
const lblLife      = $("lbl-life");
const lblEcuVer    = $("lbl-ecu-ver");
const lblSrvVer    = $("lbl-srv-ver");
const termPort     = $("term-port");
const terminal     = $("terminal");
const selPort      = $("sel-port");
const selBaud      = $("sel-baud");
const btnConnect   = $("btn-connect");
const cmdBtns      = document.querySelectorAll(".cmd-btn");
const chBtns       = document.querySelectorAll(".ch-btn");
const cmdFeedback  = $("cmd-feedback");
const inpRaw       = $("inp-raw");
const btnRaw       = $("btn-raw");
const dropZone     = $("drop-zone");
const inpFile      = $("inp-file");
const fileInfo     = $("file-info");
const fiName       = $("fi-name");
const fiSize       = $("fi-size");
const btnClearFile = $("btn-clear-file");
const inpVersion   = $("inp-version");
const btnUpload    = $("btn-upload");
const progressWrap = $("upload-progress");
const progressBar  = $("progress-bar");
const uploadFeedback = $("upload-feedback");
const chkScroll    = $("chk-scroll");
const btnClearTerm = $("btn-clear-term");
const flashToast   = $("flash-toast");
const alertBanner  = $("alert-banner");
const alertMsg     = $("alert-msg");
const alertIcon    = $("alert-icon");
const tbodyHistory = $("tbody-history");
const tbodyBoot    = $("tbody-boot");

// ──────────────────────────────────────────────────────────────────
//  State
// ──────────────────────────────────────────────────────────────────
let isConnected  = false;
let espIp        = null;
let selectedFile = null;
let toastTimer   = null;
let activeChannel = "both";
const MAX_TERM_LEN = 50_000;

// ──────────────────────────────────────────────────────────────────
//  SocketIO
// ──────────────────────────────────────────────────────────────────
const socket = io({ transports: ["websocket", "polling"] });

socket.on("connect",    () => console.log("[ws] connected"));
socket.on("disconnect", () => applyState({ connected: false }));
socket.on("state",      applyState);
socket.on("terminal",   ({ data }) => appendTerminal(data));
socket.on("firmware_history", renderHistory);
socket.on("boot_history",     renderBootHistory);

socket.on("flash_result", ({ version, status }) => {
  const ok  = status === "ok";
  const msg = ok ? `✅  Flash OK — v${version}` : `❌  Flash FAILED — v${version}`;
  showToast(msg, ok ? "ok" : "err");
  appendTerminal(`\n[OTA] ${msg}\n`);
});

socket.on("alert", ({ kind, message, ts }) => {
  const icons = { HIGH_TEMP: "🌡", LIFE_STALL: "💀", ESP_OFFLINE: "📡" };
  alertIcon.textContent = icons[kind] || "⚠";
  alertMsg.textContent  = `[${ts}] ${message}`;
  alertBanner.className = `alert-banner alert-${kind.toLowerCase()}`;
  alertBanner.style.display = "flex";
  // Browser notification (if permitted)
  if (Notification.permission === "granted") {
    new Notification("ECU Alert", { body: message, icon: "" });
  }
});

// ──────────────────────────────────────────────────────────────────
//  Apply state
// ──────────────────────────────────────────────────────────────────
function applyState(s) {
  if (s.connected !== undefined) {
    isConnected = s.connected;
    dotConn.className      = "dot" + (isConnected ? " online" : "");
    lblConn.textContent    = isConnected ? `${s.serial_port} · ${s.baud}` : "Disconnected";
    btnConnect.textContent = isConnected ? "Disconnect" : "Connect";
    termPort.textContent   = isConnected ? `(${s.serial_port} · ${s.baud})` : "(not connected)";

    // Overview stat card
    $("ov-conn-val").textContent = isConnected ? (s.serial_port || "Connected") : "Disconnected";
    $("ov-conn-sub").textContent = isConnected ? `${s.baud} baud` : "No UART link";
  }

  if ("esp_ip" in s) {
    espIp = s.esp_ip;
    const up = !!espIp;
    dotEsp.className   = "dot" + (up ? " online" : "");
    lblEsp.textContent = up ? espIp : "No ESP";
  }

  updateCmdAvailability();

  if (s.rtc_time) {
    lblRtc.textContent    = s.rtc_time;
    $("ov-rtc").textContent = s.rtc_time;
  }

  if (s.temperature) {
    lblTemp.textContent = s.temperature;
    const rawT = parseInt(s.temperature);
    $("ov-temp-val").textContent = isNaN(rawT) ? s.temperature : rawT + "°C";
    $("ov-temp-sub").textContent = isNaN(rawT) ? "Internal STM32"
      : rawT > 55 ? "⚠ High — check cooling" : "Normal range";
    $("ov-temp-val").style.color = (!isNaN(rawT) && rawT > 55) ? "var(--red-h)" : "var(--text)";
    // Push to live sparkline
    if (chartTempLive && !isNaN(rawT)) {
      const t = new Date().toLocaleTimeString();
      chartTempLive.data.labels.push(t);
      chartTempLive.data.datasets[0].data.push(rawT);
      if (chartTempLive.data.labels.length > 30) {
        chartTempLive.data.labels.shift();
        chartTempLive.data.datasets[0].data.shift();
      }
      chartTempLive.update("none");
    }
  }

  if (s.life_counter) {
    lblLife.textContent = s.life_counter;
    const n = parseInt(s.life_counter);
    $("ov-life-val").textContent = isNaN(n) ? s.life_counter : fmtUptime(n);
    $("ov-life-sub").textContent = isNaN(n) ? "Life counter" : `Tick #${n}`;
  }

  if (s.ecu_version) {
    lblEcuVer.textContent         = s.ecu_version;
    $("ov-ver-val").textContent   = s.ecu_version;
  }

  if (s.pwm_duty !== undefined) {
    $("inp-pwm").value          = s.pwm_duty;
    $("lbl-pwm").textContent    = s.pwm_duty + "%";
    $("ov-pwm").textContent     = s.pwm_duty + "%";
  }

  if (s.server_version) {
    lblSrvVer.textContent          = s.server_version;
    inpVersion.placeholder         = nextPatch(s.server_version);
    $("ov-srv-ver").textContent    = s.server_version;
  }

  const mismatch = s.ecu_version && s.server_version &&
                   s.ecu_version !== "–" && s.server_version !== "–" &&
                   s.ecu_version !== s.server_version;
  lblSrvVer.style.color = mismatch ? "var(--yellow-h)" : "";
  lblEcuVer.style.color = mismatch ? "var(--yellow-h)" : "";
  if (s.ecu_version) {
    $("ov-ver-sub").textContent  = mismatch ? "⚠ Mismatch with server" : "Matches server";
    $("ov-ver-val").style.color  = mismatch ? "var(--yellow-h)" : "var(--text)";
  }

  updateHealth(s);
}

function updateHealth(s) {
  let score = 0;
  const set = (id, dotId, ok, val) => {
    $(id).textContent            = val;
    $(dotId).style.background    = ok ? "var(--accent)" : "var(--red-h)";
  };

  // UART (25 pts)
  if (isConnected) { score += 25; set("hi-uart-val","hi-uart", true, "OK"); }
  else             { set("hi-uart-val","hi-uart", false, "Offline"); }

  // WiFi (15 pts)
  if (espIp) { score += 15; set("hi-wifi-val","hi-wifi", true, espIp); }
  else       { set("hi-wifi-val","hi-wifi", false, "No ESP"); }

  // Temp (25 pts)
  const rawT = parseInt((s.temperature || "").replace(/[^-\d]/g, ""));
  if (!isNaN(rawT)) {
    const ok = rawT < 55;
    score += ok ? 25 : 10;
    set("hi-temp-val","hi-temp", ok, rawT + "°C");
  } else { $("hi-temp-val").textContent = "–"; }

  // Life counter (20 pts)
  const life = parseInt(s.life_counter);
  if (!isNaN(life) && life > 0) { score += 20; set("hi-life-val","hi-life", true, "Running"); }
  else if (!isNaN(life))        { set("hi-life-val","hi-life", false, "Stalled"); }
  else                          { $("hi-life-val").textContent = "–"; }

  // Version match (15 pts)
  const mismatch = s.ecu_version && s.server_version &&
    s.ecu_version !== "–" && s.server_version !== "–" &&
    s.ecu_version !== s.server_version;
  if (s.ecu_version && s.ecu_version !== "–") {
    const ok = !mismatch;
    score += ok ? 15 : 5;
    set("hi-ver-val","hi-ver", ok, ok ? "Match" : "Mismatch");
  } else { $("hi-ver-val").textContent = "–"; }

  $("health-score").textContent = isConnected ? score : "–";
  const bar = $("health-bar");
  bar.style.width = isConnected ? score + "%" : "0%";
  bar.style.background = score >= 80 ? "var(--accent)"
                       : score >= 50 ? "var(--yellow-h)"
                       : "var(--red-h)";
  $("health-score").style.color = score >= 80 ? "var(--accent)"
                                : score >= 50 ? "var(--yellow-h)"
                                : "var(--red-h)";
  $("health-label").textContent = !isConnected ? "Connect ECU to see health"
    : score >= 80 ? "All systems nominal"
    : score >= 50 ? "Some issues detected"
    : "Critical issues";
}

function updateCmdAvailability() {
  const wireOk = isConnected, wifiOk = !!espIp;
  cmdBtns.forEach(b => {
    switch (activeChannel) {
      case "wire": b.disabled = !wireOk; break;
      case "wifi": b.disabled = !wifiOk; break;
      case "both": b.disabled = !wireOk && !wifiOk; break;
    }
  });
  btnRaw.disabled = !isConnected;
}

function nextPatch(ver) {
  const parts = ver.split(".");
  if (parts.length < 3) return "1.0.0";
  parts[2] = String(parseInt(parts[2] || 0) + 1);
  return parts.join(".");
}

// ──────────────────────────────────────────────────────────────────
//  Alert banner
// ──────────────────────────────────────────────────────────────────
function dismissAlert() {
  alertBanner.style.display = "none";
}

// Request browser notification permission on load
if ("Notification" in window && Notification.permission === "default") {
  Notification.requestPermission();
}

// ──────────────────────────────────────────────────────────────────
//  Alert config form
// ──────────────────────────────────────────────────────────────────
$("btn-save-alerts").addEventListener("click", async () => {
  const payload = {
    temp_threshold: parseInt($("inp-temp-thresh").value),
    life_stall_s  : parseInt($("inp-life-stall").value),
  };
  try {
    const res = await fetch("/api/alert_config", {
      method : "POST",
      headers: { "Content-Type": "application/json" },
      body   : JSON.stringify(payload),
    });
    if (res.ok) setFeedback($("alert-cfg-feedback"), "✅ Alert config saved", "ok");
    else        setFeedback($("alert-cfg-feedback"), "Save failed", "err");
  } catch {
    setFeedback($("alert-cfg-feedback"), "Network error", "err");
  }
});

// Load current alert config
(async () => {
  try {
    const cfg = await (await fetch("/api/alert_config")).json();
    $("inp-temp-thresh").value = cfg.temp_threshold ?? 55;
    $("inp-life-stall").value  = cfg.life_stall_s   ?? 60;
  } catch {}
})();

// ──────────────────────────────────────────────────────────────────
//  Channel selector
// ──────────────────────────────────────────────────────────────────
chBtns.forEach(btn => {
  btn.addEventListener("click", () => {
    chBtns.forEach(b => b.classList.remove("active"));
    btn.classList.add("active");
    activeChannel = btn.dataset.ch;
    updateCmdAvailability();
  });
});

// ──────────────────────────────────────────────────────────────────
//  Tab navigation
// ──────────────────────────────────────────────────────────────────
document.querySelectorAll(".tab-btn").forEach(btn => {
  btn.addEventListener("click", () => {
    document.querySelectorAll(".tab-btn").forEach(b => b.classList.remove("active"));
    document.querySelectorAll(".tab-content").forEach(c => c.classList.remove("active"));
    btn.classList.add("active");
    $(btn.dataset.tab).classList.add("active");
    if (btn.dataset.tab === "tab-metrics") loadMetrics();
    if (btn.dataset.tab === "tab-imu")     initImu();
    if (btn.dataset.tab === "tab-oled")    initOled();
  });
});

// ──────────────────────────────────────────────────────────────────
//  Terminal
// ──────────────────────────────────────────────────────────────────
function appendTerminal(text) {
  terminal.textContent += text;
  if (terminal.textContent.length > MAX_TERM_LEN)
    terminal.textContent = terminal.textContent.slice(-MAX_TERM_LEN / 2);
  if (chkScroll.checked)
    requestAnimationFrame(() => { terminal.scrollTop = terminal.scrollHeight; });
}

btnClearTerm.addEventListener("click", () => { terminal.textContent = ""; });

// ──────────────────────────────────────────────────────────────────
//  Connect / Disconnect
// ──────────────────────────────────────────────────────────────────
async function loadPorts() {
  try {
    const ports = await (await fetch("/api/ports")).json();
    selPort.innerHTML = "";
    ports.forEach(({ port, desc }) => {
      const opt = document.createElement("option");
      opt.value = port; opt.textContent = `${port}  –  ${desc}`;
      selPort.appendChild(opt);
    });
    if (!ports.length) {
      const opt = document.createElement("option");
      opt.value = "COM5"; opt.textContent = "COM5 (default)";
      selPort.appendChild(opt);
    }
  } catch {
    const opt = document.createElement("option");
    opt.value = "COM5"; opt.textContent = "COM5";
    selPort.appendChild(opt);
  }
}

btnConnect.addEventListener("click", async () => {
  if (isConnected) {
    await fetch("/api/disconnect", { method: "POST" });
  } else {
    await fetch("/api/connect", {
      method : "POST",
      headers: { "Content-Type": "application/json" },
      body   : JSON.stringify({ port: selPort.value, baud: parseInt(selBaud.value) }),
    });
  }
});

// ──────────────────────────────────────────────────────────────────
//  Commands
// ──────────────────────────────────────────────────────────────────
cmdBtns.forEach(btn => {
  btn.addEventListener("click", async () => {
    const cmd = btn.dataset.cmd;
    try {
      const res  = await fetch("/api/command", {
        method : "POST",
        headers: { "Content-Type": "application/json" },
        body   : JSON.stringify({ cmd, channel: activeChannel }),
      });
      const data = await res.json();
      if (data.ok) {
        const via = channelLabel(data.wire, data.wifi);
        const msg = `→ ${cmd}  (0x${data.byte.slice(2).toUpperCase()})  ${via}`;
        setFeedback(cmdFeedback, msg, "ok");
        setFeedback($("ov-cmd-feedback"), msg, "ok");
      } else {
        setFeedback(cmdFeedback, data.error, "err");
        setFeedback($("ov-cmd-feedback"), data.error, "err");
      }
    } catch {
      setFeedback(cmdFeedback, "Request failed", "err");
      setFeedback($("ov-cmd-feedback"), "Request failed", "err");
    }
  });
});

function channelLabel(wire, wifi) {
  if (wire && wifi) return "[Wire + WiFi]";
  if (wire)         return "[Wire]";
  if (wifi)         return "[WiFi]";
  return "[no channel]";
}

inpRaw.addEventListener("keydown", e => { if (e.key === "Enter") btnRaw.click(); });
btnRaw.addEventListener("click", () => {
  const hex = inpRaw.value.trim();
  if (!hex) return;
  socket.emit("send_raw", { byte: hex, channel: "wire" });
  setFeedback(cmdFeedback, `→ raw 0x${hex.toUpperCase()} [Wire]`, "ok");
  inpRaw.value = "";
});

// ──────────────────────────────────────────────────────────────────
//  Firmware upload
// ──────────────────────────────────────────────────────────────────
dropZone.addEventListener("click", () => inpFile.click());
dropZone.addEventListener("dragover", e => { e.preventDefault(); dropZone.classList.add("drag-over"); });
dropZone.addEventListener("dragleave", () => dropZone.classList.remove("drag-over"));
dropZone.addEventListener("drop", e => {
  e.preventDefault(); dropZone.classList.remove("drag-over");
  if (e.dataTransfer.files[0]) selectFile(e.dataTransfer.files[0]);
});
inpFile.addEventListener("change", () => { if (inpFile.files[0]) selectFile(inpFile.files[0]); });

function selectFile(file) {
  selectedFile = file;
  fiName.textContent = file.name;
  fiSize.textContent = formatBytes(file.size);
  fileInfo.style.display = "flex";
  dropZone.style.display = "none";
  const m = file.name.match(/(\d+\.\d+\.\d+)/);
  if (m && !inpVersion.value) inpVersion.value = m[1];
}

btnClearFile.addEventListener("click", () => {
  selectedFile = null; inpFile.value = "";
  fileInfo.style.display = "none"; dropZone.style.display = "block";
});

btnUpload.addEventListener("click", async () => {
  if (!selectedFile) { setFeedback(uploadFeedback, "Select a .bin file first", "err"); return; }
  const version = inpVersion.value.trim();
  if (!version) { setFeedback(uploadFeedback, "Enter a version tag (e.g. 1.0.6)", "err"); return; }

  const form = new FormData();
  form.append("firmware", selectedFile);
  form.append("version",  version);

  btnUpload.disabled = true;
  progressWrap.style.display = "block";
  progressBar.style.width = "0%";
  setFeedback(uploadFeedback, "Uploading…", "");

  const xhr = new XMLHttpRequest();
  xhr.open("POST", "/upload");
  xhr.upload.onprogress = e => {
    if (e.lengthComputable)
      progressBar.style.width = `${(e.loaded / e.total * 100).toFixed(0)}%`;
  };
  xhr.onload = () => {
    btnUpload.disabled = false;
    progressBar.style.width = "100%";
    try {
      const data = JSON.parse(xhr.responseText);
      if (data.ok) {
        setFeedback(uploadFeedback, `✅  v${data.version} uploaded  (${formatBytes(data.size)})`, "ok");
        setTimeout(() => { progressWrap.style.display = "none"; }, 1200);
      } else {
        setFeedback(uploadFeedback, data.error || "Upload failed", "err");
      }
    } catch { setFeedback(uploadFeedback, "Server error", "err"); }
  };
  xhr.onerror = () => { btnUpload.disabled = false; setFeedback(uploadFeedback, "Network error", "err"); };
  xhr.send(form);
});

// ──────────────────────────────────────────────────────────────────
//  Firmware History
// ──────────────────────────────────────────────────────────────────
function renderHistory(rows) {
  if (!rows || !rows.length) {
    tbodyHistory.innerHTML = `<tr><td colspan="6" class="muted" style="text-align:center">No history yet</td></tr>`;
    return;
  }
  tbodyHistory.innerHTML = rows.map(r => {
    const result = r.flash_result || "pending";
    const badge  = result === "ok"      ? "badge-ok"
                 : result === "failed"  ? "badge-err"
                 : "badge-pending";
    const canRollback = result !== "pending";
    return `<tr>
      <td class="muted">#${r.id}</td>
      <td><span class="ver-tag">v${r.version}</span></td>
      <td class="muted">${r.uploaded_at}</td>
      <td class="muted">${r.size ? formatBytes(r.size) : "–"}</td>
      <td><span class="badge ${badge}">${result}</span></td>
      <td>${canRollback
        ? `<button class="btn btn-secondary btn-sm"
             onclick="doRollback('${r.version}')">Rollback</button>`
        : "–"}</td>
    </tr>`;
  }).join("");
}

// ──────────────────────────────────────────────────────────────────
//  Boot history (#9)
// ──────────────────────────────────────────────────────────────────
function renderBootHistory(rows) {
  const REASON_ICON = { SFT: "🔄", POR: "⚡", PIN: "📌", IWDG: "🐕", WWDG: "🪟", UNK: "❓" };
  if (!rows || !rows.length) {
    tbodyBoot.innerHTML = `<tr><td colspan="4" class="muted" style="text-align:center">No boot events yet</td></tr>`;
    return;
  }
  tbodyBoot.innerHTML = rows.map(r => {
    const icon = REASON_ICON[r.reason] || "❓";
    return `<tr>
      <td class="muted">#${r.id}</td>
      <td class="muted">${r.ts}</td>
      <td>${icon} <strong>${r.reason}</strong></td>
      <td><span class="ver-tag">v${r.version}</span></td>
    </tr>`;
  }).join("");
}

// ──────────────────────────────────────────────────────────────────
//  PWM slider (#5)
// ──────────────────────────────────────────────────────────────────
$("inp-pwm").addEventListener("input", () => {
  $("lbl-pwm").textContent = $("inp-pwm").value + "%";
});

$("btn-pwm").addEventListener("click", async () => {
  const duty = parseInt($("inp-pwm").value);
  try {
    const res  = await fetch("/api/pwm", {
      method : "POST",
      headers: { "Content-Type": "application/json" },
      body   : JSON.stringify({ duty, channel: activeChannel }),
    });
    const data = await res.json();
    if (data.ok) setFeedback(cmdFeedback, `→ PWM duty = ${data.duty}%`, "ok");
    else         setFeedback(cmdFeedback, data.error || "PWM failed", "err");
  } catch {
    setFeedback(cmdFeedback, "PWM request failed", "err");
  }
});

async function doRollback(version) {
  if (!confirm(`Restore v${version} as active OTA firmware?`)) return;
  try {
    const res  = await fetch(`/api/rollback/${version}`, { method: "POST" });
    const data = await res.json();
    if (data.ok) showToast(`✅ Rolled back to v${version}`, "ok");
    else         showToast(`❌ Rollback failed: ${data.error}`, "err");
  } catch {
    showToast("Rollback request failed", "err");
  }
}

// ──────────────────────────────────────────────────────────────────
//  Metrics charts
// ──────────────────────────────────────────────────────────────────
let chartTemp     = null;
let chartLife     = null;
let chartStack    = null;
let chartCpuLoad  = null;
let chartTempLive = null;

const TASK_LABELS = ["T0 5ms", "T1 10ms", "T2 20ms", "T3 50ms", "T4 100ms", "T5 1000ms"];

const MUTED  = "#4a5568";
const GRID   = "#1e2d42";

const CHART_OPTS = {
  responsive: true,
  animation : false,
  plugins   : { legend: { display: false } },
  scales    : {
    x: {
      ticks: { color: MUTED, maxTicksLimit: 8, maxRotation: 0, font: { size: 10 } },
      grid : { color: GRID },
    },
    y: {
      ticks: { color: MUTED, font: { size: 10 } },
      grid : { color: GRID },
    },
  },
};

function buildCharts() {
  const ctxT  = $("chart-temp").getContext("2d");
  const ctxL  = $("chart-life").getContext("2d");
  const ctxS  = $("chart-stack").getContext("2d");
  const ctxC  = $("chart-cpu").getContext("2d");
  const ctxTL = $("chart-temp-live").getContext("2d");

  /* Live sparkline on overview */
  chartTempLive = new Chart(ctxTL, {
    type: "line",
    data: {
      labels  : [],
      datasets: [{
        data           : [],
        borderColor    : "#ef4444",
        backgroundColor: "rgba(239,68,68,.08)",
        fill           : true,
        tension        : 0.4,
        pointRadius    : 0,
        borderWidth    : 2,
      }],
    },
    options: {
      responsive: true,
      animation : false,
      plugins   : {
        legend : { display: false },
        tooltip: { callbacks: { label: ctx => ` ${ctx.parsed.y} °C` } },
      },
      scales: {
        x: { display: false },
        y: {
          ticks: { color: MUTED, font: { size: 10 }, callback: v => v + "°" },
          grid : { color: GRID },
        },
      },
    },
  });

  chartTemp = new Chart(ctxT, {
    type: "line",
    data: { labels: [], datasets: [{ data: [], borderColor: "#ef4444", backgroundColor: "rgba(239,68,68,.08)", fill: true, tension: 0.4, pointRadius: 2, borderWidth: 2 }] },
    options: { ...CHART_OPTS, scales: { ...CHART_OPTS.scales, y: { ...CHART_OPTS.scales.y, title: { display: true, text: "°C", color: MUTED } } } },
  });

  chartCpuLoad = new Chart(ctxC, {
    type: "bar",
    data: {
      labels  : TASK_LABELS,
      datasets: [{
        data           : [0, 0, 0, 0, 0, 0],
        backgroundColor: ["#3b82f6","#00d9a3","#f59e0b","#8b5cf6","#ef4444","#f97316"],
        borderRadius   : 6,
        borderWidth    : 0,
      }],
    },
    options: {
      responsive: true,
      animation : false,
      plugins: {
        legend : { display: false },
        tooltip: { callbacks: { label: ctx => ` CPU load: ${ctx.parsed.y}%` } },
      },
      scales: {
        x: { ticks: { color: MUTED, font: { size: 11 } }, grid: { color: GRID } },
        y: {
          min  : 0,
          max  : 100,
          ticks: { color: MUTED, callback: v => v + "%", font: { size: 10 } },
          grid : { color: GRID },
          title: { display: true, text: "CPU load %", color: MUTED },
        },
      },
    },
  });

  chartStack = new Chart(ctxS, {
    type: "bar",
    data: {
      labels  : TASK_LABELS,
      datasets: [{
        data           : [0, 0, 0, 0, 0, 0],
        backgroundColor: ["#3b82f6","#00d9a3","#f59e0b","#8b5cf6","#ef4444","#f97316"],
        borderRadius   : 6,
        borderWidth    : 0,
      }],
    },
    options: {
      responsive: true,
      animation : false,
      plugins   : {
        legend : { display: false },
        tooltip: { callbacks: { label: ctx => ` Stack used: ${ctx.parsed.y}%` } },
      },
      scales: {
        x: { ticks: { color: MUTED, font: { size: 11 } }, grid: { color: GRID } },
        y: {
          min  : 0,
          max  : 100,
          ticks: { color: MUTED, callback: v => v + "%", font: { size: 10 } },
          grid : { color: GRID },
          title: { display: true, text: "Stack used %", color: MUTED },
        },
      },
    },
  });

  chartLife = new Chart(ctxL, {
    type: "line",
    data: { labels: [], datasets: [{ data: [], borderColor: "#00d9a3", backgroundColor: "rgba(0,217,163,.08)", fill: true, tension: 0.4, pointRadius: 2, borderWidth: 2 }] },
    options: {
      ...CHART_OPTS,
      scales: {
        ...CHART_OPTS.scales,
        y: {
          ...CHART_OPTS.scales.y,
          title: { display: true, text: "Uptime", color: MUTED },
          ticks: {
            color: MUTED,
            callback: v => fmtUptime(v),
          },
        },
      },
      plugins: {
        legend: { display: false },
        tooltip: {
          callbacks: {
            label: ctx => " Uptime: " + fmtUptime(ctx.parsed.y),
          },
        },
      },
    },
  });
}

function fmtUptime(s) {
  if (s == null || isNaN(s)) return "–";
  const h = Math.floor(s / 3600);
  const m = Math.floor((s % 3600) / 60);
  const sec = s % 60;
  if (h > 0) return `${h}h ${m.toString().padStart(2,"0")}m`;
  if (m > 0) return `${m}m ${sec.toString().padStart(2,"0")}s`;
  return `${sec}s`;
}

function fmtLabel(tsStr, spanHours) {
  const d = new Date(tsStr);
  const hh = d.getHours().toString().padStart(2,"0");
  const mm = d.getMinutes().toString().padStart(2,"0");
  const ss = d.getSeconds().toString().padStart(2,"0");
  if (spanHours > 24) {
    const mo = (d.getMonth()+1).toString().padStart(2,"0");
    const dd = d.getDate().toString().padStart(2,"0");
    return `${mo}/${dd} ${hh}:${mm}`;
  }
  return `${hh}:${mm}:${ss}`;
}

async function loadMetrics() {
  const hours = parseInt($("sel-hours").value);
  try {
    const rows = await (await fetch(`/api/metrics?hours=${hours}`)).json();

    const labels = rows.map(r => fmtLabel(r.ts, hours));
    const temps  = rows.map(r => r.temperature);
    const lifes  = rows.map(r => r.life_counter);

    chartTemp.data.labels           = labels;
    chartTemp.data.datasets[0].data = temps;
    chartTemp.update();

    chartLife.data.labels           = labels;
    chartLife.data.datasets[0].data = lifes;
    chartLife.update();
  } catch (e) {
    console.error("Metrics load failed", e);
  }
}

$("btn-refresh-metrics").addEventListener("click", loadMetrics);
$("sel-hours").addEventListener("change",          loadMetrics);

socket.on("cpu_load", data => {
  const vals = TASK_LABELS.map((_, i) => data[`T${i}`] ?? 0);
  chartCpuLoad.data.datasets[0].data = vals;
  chartCpuLoad.update();
  $("lbl-cpu-ts").textContent = new Date().toLocaleTimeString();
});

socket.on("stack_usage", data => {
  const vals = TASK_LABELS.map((_, i) => data[`T${i}`] ?? 0);
  chartStack.data.datasets[0].data = vals;
  chartStack.update();
  $("lbl-stack-ts").textContent = new Date().toLocaleTimeString();
});

$("btn-clear-life-chart").addEventListener("click", () => {
  chartLife.data.labels           = [];
  chartLife.data.datasets[0].data = [];
  chartLife.update();
});

// ──────────────────────────────────────────────────────────────────
//  Toast
// ──────────────────────────────────────────────────────────────────
function showToast(msg, cls) {
  clearTimeout(toastTimer);
  flashToast.textContent   = msg;
  flashToast.className     = `flash-toast ${cls}`;
  flashToast.style.display = "block";
  toastTimer = setTimeout(() => { flashToast.style.display = "none"; }, 4000);
}

// ──────────────────────────────────────────────────────────────────
//  Utilities
// ──────────────────────────────────────────────────────────────────
function setFeedback(el, msg, cls) {
  el.textContent = msg;
  el.className   = `feedback ${cls}`;
}

function formatBytes(n) {
  if (n < 1024)        return `${n} B`;
  if (n < 1024 * 1024) return `${(n / 1024).toFixed(1)} KB`;
  return `${(n / 1024 / 1024).toFixed(2)} MB`;
}

// ──────────────────────────────────────────────────────────────────
//  IMU — Three.js 3D board + complementary filter
// ──────────────────────────────────────────────────────────────────
let imuScene = null;
let imuRoll  = 0;
let imuPitch = 0;
const IMU_ALPHA = 0.98;
const IMU_DT    = 0.05;   // 50 ms sample time

function initImu() {
  if (imuScene) return;   // already initialised

  const canvas   = $("imu-canvas");
  const renderer = new THREE.WebGLRenderer({ canvas, antialias: true, alpha: true });
  renderer.setPixelRatio(window.devicePixelRatio);

  const w = canvas.parentElement.clientWidth - 32;
  const h = Math.round(w * 0.55);
  renderer.setSize(w, h);
  canvas.style.height = h + "px";

  const scene  = new THREE.Scene();
  scene.background = new THREE.Color(0x0d1117);
  imuScene = scene;

  const camera = new THREE.PerspectiveCamera(45, w / h, 0.1, 100);
  camera.position.set(0, 2, 5);
  camera.lookAt(0, 0, 0);

  // Board mesh — Nucleo-like proportions (wide, thin)
  const geo  = new THREE.BoxGeometry(3, 0.12, 5);
  const mats = [
    new THREE.MeshPhongMaterial({ color: 0x1a5c2e }),  // right
    new THREE.MeshPhongMaterial({ color: 0x1a5c2e }),  // left
    new THREE.MeshPhongMaterial({ color: 0x2ecc71 }),  // top (PCB green)
    new THREE.MeshPhongMaterial({ color: 0x1a5c2e }),  // bottom
    new THREE.MeshPhongMaterial({ color: 0x1a5c2e }),  // front
    new THREE.MeshPhongMaterial({ color: 0x1a5c2e }),  // back
  ];
  const board = new THREE.Mesh(geo, mats);
  scene.add(board);

  // Axis arrows
  scene.add(new THREE.ArrowHelper(new THREE.Vector3(1,0,0), new THREE.Vector3(0,0,0), 2, 0xf85149, 0.3, 0.15));
  scene.add(new THREE.ArrowHelper(new THREE.Vector3(0,1,0), new THREE.Vector3(0,0,0), 2, 0x3fb950, 0.3, 0.15));
  scene.add(new THREE.ArrowHelper(new THREE.Vector3(0,0,1), new THREE.Vector3(0,0,0), 2, 0x58a6ff, 0.3, 0.15));

  // Lighting
  scene.add(new THREE.AmbientLight(0xffffff, 0.5));
  const dLight = new THREE.DirectionalLight(0xffffff, 1);
  dLight.position.set(3, 5, 3);
  scene.add(dLight);

  // Animate
  (function render() {
    requestAnimationFrame(render);
    board.rotation.x = THREE.MathUtils.degToRad(imuRoll);
    board.rotation.z = THREE.MathUtils.degToRad(-imuPitch);
    renderer.render(scene, camera);
  })();
}

socket.on("mpu", ({ ax, ay, az, gx, gy, gz }) => {
  const ax_g  = ax / 100;
  const ay_g  = ay / 100;
  const az_g  = az / 100;
  const gx_ds = gx / 100;
  const gy_ds = gy / 100;

  const accelRoll  = Math.atan2(ay_g, az_g)                              * 180 / Math.PI;
  const accelPitch = Math.atan2(-ax_g, Math.sqrt(ay_g*ay_g + az_g*az_g)) * 180 / Math.PI;

  imuRoll  = IMU_ALPHA * (imuRoll  + gx_ds * IMU_DT) + (1 - IMU_ALPHA) * accelRoll;
  imuPitch = IMU_ALPHA * (imuPitch + gy_ds * IMU_DT) + (1 - IMU_ALPHA) * accelPitch;

  const fmt = v => (v >= 0 ? "+" : "") + v.toFixed(2);
  $("imu-ax").textContent    = fmt(ax_g);
  $("imu-ay").textContent    = fmt(ay_g);
  $("imu-az").textContent    = fmt(az_g);
  $("imu-gx").textContent    = fmt(gx_ds);
  $("imu-gy").textContent    = fmt(gy_ds);
  $("imu-gz").textContent    = fmt(gz / 100);
  $("imu-roll").textContent  = imuRoll.toFixed(1)  + "°";
  $("imu-pitch").textContent = imuPitch.toFixed(1) + "°";
});

// ──────────────────────────────────────────────────────────────────
//  Build
// ──────────────────────────────────────────────────────────────────
const btnBuild     = $("btn-build");
const buildLog     = $("build-log");
const buildStatus  = $("build-status");

btnBuild.addEventListener("click", async () => {
  buildLog.textContent = "";
  buildLog.style.display = "block";
  buildStatus.textContent = "Building...";
  btnBuild.disabled = true;

  try {
    const res  = await fetch("/api/build", { method: "POST" });
    const data = await res.json();
    if (!data.ok) {
      buildStatus.textContent = data.error;
      btnBuild.disabled = false;
    }
  } catch {
    buildStatus.textContent = "Request failed";
    btnBuild.disabled = false;
  }
});

socket.on("build_log", ({ msg, ts }) => {
  buildLog.textContent += `[${ts}] ${msg}\n`;
  buildLog.scrollTop = buildLog.scrollHeight;
});

socket.on("build_done", ({ ok }) => {
  btnBuild.disabled = false;
  buildStatus.textContent = ok ? "✅ Build successful" : "❌ Build failed";
  buildStatus.style.color  = ok ? "var(--green)"      : "var(--red)";
});

// ──────────────────────────────────────────────────────────────────
//  UART Flash tab
// ──────────────────────────────────────────────────────────────────
let ufFile = null;

const ufDropZone     = $("uf-drop-zone");
const ufInpFile      = $("uf-inp-file");
const ufFileInfo     = $("uf-file-info");
const ufFiName       = $("uf-fi-name");
const ufFiSize       = $("uf-fi-size");
const ufBtnClear     = $("uf-btn-clear");
const ufLog          = $("uf-log");
const ufProgressWrap = $("uf-progress-wrap");
const ufProgressBar  = $("uf-progress-bar");
const btnUartFlash   = $("btn-uart-flash");

ufDropZone.addEventListener("click", () => ufInpFile.click());
ufDropZone.addEventListener("dragover",  e => { e.preventDefault(); ufDropZone.classList.add("drag-over"); });
ufDropZone.addEventListener("dragleave", () => ufDropZone.classList.remove("drag-over"));
ufDropZone.addEventListener("drop", e => {
  e.preventDefault(); ufDropZone.classList.remove("drag-over");
  if (e.dataTransfer.files[0]) ufSelectFile(e.dataTransfer.files[0]);
});
ufInpFile.addEventListener("change", () => { if (ufInpFile.files[0]) ufSelectFile(ufInpFile.files[0]); });

function ufSelectFile(file) {
  ufFile = file;
  ufFiName.textContent = file.name;
  ufFiSize.textContent = formatBytes(file.size);
  ufFileInfo.style.display = "flex";
  ufDropZone.style.display  = "none";
}

ufBtnClear.addEventListener("click", () => {
  ufFile = null; ufInpFile.value = "";
  ufFileInfo.style.display = "none";
  ufDropZone.style.display  = "block";
});

function ufAppendLog(ts, msg) {
  ufLog.textContent += `[${ts}] ${msg}\n`;
  ufLog.scrollTop = ufLog.scrollHeight;
}

socket.on("uart_flash_log", ({ msg, ts }) => {
  ufAppendLog(ts, msg);
  appendTerminal(`[UART Flash] ${msg}\n`);
});

socket.on("uart_flash_progress", ({ pct, sent, total }) => {
  ufProgressBar.style.width = pct + "%";
});

socket.on("uart_flash_done", ({ ok }) => {
  btnUartFlash.disabled       = false;
  $("btn-uart-flash-local").disabled = false;
  if (ok) {
    ufProgressBar.style.width = "100%";
    setTimeout(() => { ufProgressWrap.style.display = "none"; }, 2000);
    showToast("Transfer complete — check terminal for CRC / verification result", "ok");
  } else {
    ufProgressWrap.style.display = "none";
    showToast("UART Flash encountered an error", "err");
  }
});

btnUartFlash.addEventListener("click", async () => {
  if (!isConnected) {
    ufAppendLog(new Date().toLocaleTimeString(), "ERROR: ECU not connected via wire");
    return;
  }
  if (!confirm("This will reset the ECU and flash new firmware via UART2. Continue?")) return;

  ufLog.textContent = "";
  ufProgressWrap.style.display = "block";
  ufProgressBar.style.width    = "0%";
  btnUartFlash.disabled = true;

  const form = new FormData();
  if (ufFile) form.append("firmware", ufFile);

  try {
    const res  = await fetch("/api/uart_flash", { method: "POST", body: form });
    const data = await res.json();
    if (!data.ok) {
      ufAppendLog(new Date().toLocaleTimeString(), `ERROR: ${data.error}`);
      ufProgressWrap.style.display = "none";
      btnUartFlash.disabled = false;
    }
  } catch {
    ufAppendLog(new Date().toLocaleTimeString(), "ERROR: Request to server failed");
    ufProgressWrap.style.display = "none";
    btnUartFlash.disabled = false;
  }
});

// ──────────────────────────────────────────────────────────────────
//  UART Flash — local APP_BTLD.bin button
// ──────────────────────────────────────────────────────────────────

async function loadBtldInfo() {
  try {
    const d = await (await fetch("/api/app_btld_info")).json();
    if (d.exists) {
      $("btld-local-info").textContent =
        `${formatBytes(d.size)}  ·  modified ${d.modified}`;
      $("btld-local-info").style.color = "";
    } else {
      $("btld-local-info").textContent = "File not found — run Build first";
      $("btld-local-info").style.color = "var(--red-h)";
    }
  } catch {
    $("btld-local-info").textContent = "Could not read file info";
    $("btld-local-info").style.color = "var(--red-h)";
  }
}

loadBtldInfo();  /* load on page open */

$("btn-uart-flash-local").addEventListener("click", async () => {
  if (!isConnected) {
    ufAppendLog(new Date().toLocaleTimeString(), "ERROR: ECU not connected via wire");
    return;
  }
  if (!confirm("Flash APP/Tools/APP_BTLD.bin to ECU via UART2?")) return;

  ufLog.textContent = "";
  ufProgressWrap.style.display = "block";
  ufProgressBar.style.width    = "0%";
  $("btn-uart-flash-local").disabled = true;
  btnUartFlash.disabled              = true;

  try {
    const res  = await fetch("/api/uart_flash_local", { method: "POST" });
    const data = await res.json();
    if (!data.ok) {
      ufAppendLog(new Date().toLocaleTimeString(), `ERROR: ${data.error}`);
      ufProgressWrap.style.display = "none";
      $("btn-uart-flash-local").disabled = false;
      btnUartFlash.disabled              = false;
    } else {
      ufAppendLog(new Date().toLocaleTimeString(),
        `Flashing APP/Tools/APP_BTLD.bin  (${formatBytes(data.size)})`);
    }
  } catch {
    ufAppendLog(new Date().toLocaleTimeString(), "ERROR: Request to server failed");
    ufProgressWrap.style.display = "none";
    $("btn-uart-flash-local").disabled = false;
    btnUartFlash.disabled              = false;
  }
});

// ──────────────────────────────────────────────────────────────────
//  Snake game controls (#OLED tab)
//  Same bytes as `stm game` in scripts/stm.sh — UART2_ISR reads the
//  raw ASCII char and maps it to MOVE_*/GAME_RESET.
// ──────────────────────────────────────────────────────────────────
const SNAKE_KEYS = {
  w: "77", a: "61", s: "73", d: "64", r: "72",
  i: "69", j: "6a",
  ArrowUp: "77", ArrowLeft: "61", ArrowDown: "73", ArrowRight: "64",
};

const snakeFeedback = $("snake-feedback");
const chkSnakeKbd   = $("chk-snake-kbd");

function sendSnakeByte(hex, label) {
  if (!isConnected) {
    snakeFeedback.textContent = "Not connected — open UART first";
    return;
  }
  socket.emit("send_raw", { byte: hex, channel: "wire" });
  snakeFeedback.textContent = `→ ${label}  [0x${hex.toUpperCase()}]`;
}

document.querySelectorAll(".dpad-btn").forEach(btn => {
  btn.addEventListener("click", () => sendSnakeByte(SNAKE_KEYS[btn.dataset.key], btn.title));
});

document.addEventListener("keydown", e => {
  if (!chkSnakeKbd.checked) return;
  if (!$("tab-oled").classList.contains("active")) return;
  const tag = (e.target.tagName || "").toLowerCase();
  if (tag === "input" || tag === "textarea" || tag === "select") return;

  const key = e.key.length === 1 ? e.key.toLowerCase() : e.key;
  const hex = SNAKE_KEYS[key];
  if (!hex) return;
  e.preventDefault();
  sendSnakeByte(hex, key);
});

// ──────────────────────────────────────────────────────────────────
//  OLED simulation
// ──────────────────────────────────────────────────────────────────
const OLED_W      = 128;
const OLED_H      = 64;
const OLED_PAGES  = 8;
const OLED_SCALE  = 4;

let oledFrames    = null;
let oledFrameIdx  = 0;
let oledTimer     = null;
let oledRunning   = false;
let oledFps       = 16;
let oledLastRender= 0;

/* Build a full 1024-byte OLED buffer from a 512-byte Nyan Cat frame.
   The sprite occupies pages 2–5 (rows 16–47) on the 128×64 display. */
function buildOledBuffer(frame, yOffsetPages) {
  const buf = new Uint8Array(1024);
  const start = yOffsetPages * OLED_W;
  for (let i = 0; i < 512; i++) buf[start + i] = frame[i];
  return buf;
}

/* Render a 1024-byte page-format buffer to the OLED canvas.
   Page format: buf[page*128 + col] → 8 vertical pixels, bit0=top. */
function renderOledBuffer(buf, ctx) {
  /* Background — very dark green tint like a real OLED */
  ctx.fillStyle = "#060c06";
  ctx.fillRect(0, 0, OLED_W * OLED_SCALE, OLED_H * OLED_SCALE);

  const gap = 1; /* 1px gap between pixels for authentic OLED look */
  const sz  = OLED_SCALE - gap;

  for (let page = 0; page < OLED_PAGES; page++) {
    for (let col = 0; col < OLED_W; col++) {
      const byte = buf[page * OLED_W + col];
      if (byte === 0) continue; /* skip blank columns for performance */
      for (let bit = 0; bit < 8; bit++) {
        if ((byte >> bit) & 1) {
          const px = col * OLED_SCALE;
          const py = (page * 8 + bit) * OLED_SCALE;
          /* Glow behind pixel */
          ctx.fillStyle = "rgba(0,255,120,.12)";
          ctx.fillRect(px - 1, py - 1, OLED_SCALE + 2, OLED_SCALE + 2);
          /* Pixel */
          ctx.fillStyle = "#00ff88";
          ctx.fillRect(px, py, sz, sz);
        }
      }
    }
  }
}

function oledTick() {
  if (!oledFrames || !oledRunning) return;
  const canvas = $("oled-canvas");
  if (!canvas) return;
  const ctx = canvas.getContext("2d");
  const buf = buildOledBuffer(oledFrames[oledFrameIdx], 2);
  renderOledBuffer(buf, ctx);
  $("oled-frame-num").textContent = oledFrameIdx;
  oledFrameIdx = (oledFrameIdx + 1) % oledFrames.length;
}

function oledSetFps(fps) {
  oledFps = fps;
  $("oled-fps").textContent = fps;
  if (oledTimer) { clearInterval(oledTimer); oledTimer = null; }
  if (oledRunning) oledTimer = setInterval(oledTick, 1000 / fps);
}

async function initOled() {
  /* Already initialised — just make sure it's running */
  if (oledFrames) {
    if (!oledRunning) { oledRunning = true; oledSetFps(oledFps); }
    return;
  }

  const canvas = $("oled-canvas");
  const ctx    = canvas.getContext("2d");
  ctx.fillStyle = "#060c06";
  ctx.fillRect(0, 0, canvas.width, canvas.height);
  ctx.fillStyle = "#4a5568";
  ctx.font = "14px monospace";
  ctx.textAlign = "center";
  ctx.fillText("Loading frames…", canvas.width / 2, canvas.height / 2);

  try {
    const res  = await fetch("/api/oled_frames");
    const data = await res.json();
    if (data.error) throw new Error(data.error);
    oledFrames  = data.frames;
    oledRunning = true;
    oledSetFps(oledFps);
  } catch (e) {
    ctx.fillStyle = "#ef4444";
    ctx.fillText("Failed: " + e.message, canvas.width / 2, canvas.height / 2);
  }
}

/* Play / Pause */
$("btn-oled-toggle").addEventListener("click", () => {
  if (oledRunning) {
    oledRunning = false;
    clearInterval(oledTimer); oledTimer = null;
    $("btn-oled-toggle").textContent = "▶ Play";
  } else {
    oledRunning = true;
    oledSetFps(oledFps);
    $("btn-oled-toggle").textContent = "⏸ Pause";
  }
});

/* Speed slider */
$("oled-speed").addEventListener("input", () => {
  oledSetFps(parseInt($("oled-speed").value));
});

// ──────────────────────────────────────────────────────────────────
//  Boot
// ──────────────────────────────────────────────────────────────────
(async function init() {
  await loadPorts();
  for (const opt of selPort.options) {
    if (opt.value === "COM5") { selPort.value = "COM5"; break; }
  }
  try {
    const s = await (await fetch("/api/state")).json();
    applyState(s);
  } catch {}

  buildCharts();

  try {
    const rows = await (await fetch("/api/firmware_history")).json();
    renderHistory(rows);
  } catch {}

  try {
    const rows = await (await fetch("/api/boot_history")).json();
    renderBootHistory(rows);
  } catch {}

  try {
    const t = await (await fetch("/api/tunnel_url")).json();
    if (t.url && t.url.startsWith("https")) {
      const el = document.createElement("a");
      el.href        = t.url;
      el.target      = "_blank";
      el.textContent = "🌐 " + t.url;
      el.style.cssText = "font-size:11px;color:var(--accent);font-family:var(--font-mono);text-decoration:none";
      document.querySelector(".header-left").appendChild(el);
    }
  } catch {}
})();
