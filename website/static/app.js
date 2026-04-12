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
  }

  if ("esp_ip" in s) {
    espIp = s.esp_ip;
    const up = !!espIp;
    dotEsp.className   = "dot" + (up ? " online" : "");
    lblEsp.textContent = up ? espIp : "No ESP";
  }

  updateCmdAvailability();

  if (s.rtc_time)      lblRtc.textContent    = s.rtc_time;
  if (s.temperature)   lblTemp.textContent   = s.temperature;
  if (s.life_counter)  lblLife.textContent   = s.life_counter;
  if (s.ecu_version)   lblEcuVer.textContent = s.ecu_version;
  if (s.pwm_duty !== undefined) {
    $("inp-pwm").value = s.pwm_duty;
    $("lbl-pwm").textContent = s.pwm_duty + "%";
  }
  if (s.server_version) {
    lblSrvVer.textContent  = s.server_version;
    inpVersion.placeholder = nextPatch(s.server_version);
  }

  const mismatch = s.ecu_version && s.server_version &&
                   s.ecu_version !== "–" && s.server_version !== "–" &&
                   s.ecu_version !== s.server_version;
  lblSrvVer.style.color = mismatch ? "var(--yellow-h)" : "";
  lblEcuVer.style.color = mismatch ? "var(--yellow-h)" : "";
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
  });
});

// ──────────────────────────────────────────────────────────────────
//  Terminal
// ──────────────────────────────────────────────────────────────────
function appendTerminal(text) {
  terminal.textContent += text;
  if (terminal.textContent.length > MAX_TERM_LEN)
    terminal.textContent = terminal.textContent.slice(-MAX_TERM_LEN / 2);
  if (chkScroll.checked) terminal.scrollTop = terminal.scrollHeight;
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
        setFeedback(cmdFeedback, `→ ${cmd}  (0x${data.byte.slice(2).toUpperCase()})  ${via}`, "ok");
      } else {
        setFeedback(cmdFeedback, data.error, "err");
      }
    } catch {
      setFeedback(cmdFeedback, "Request failed", "err");
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
let chartTemp = null;
let chartLife = null;

const CHART_OPTS = {
  responsive: true,
  animation : false,
  plugins   : { legend: { display: false } },
  scales    : {
    x: {
      ticks: { color: "#8b949e", maxTicksLimit: 8, maxRotation: 0 },
      grid : { color: "#21262d" },
    },
    y: {
      ticks: { color: "#8b949e" },
      grid : { color: "#21262d" },
    },
  },
};

function buildCharts() {
  const ctxT = $("chart-temp").getContext("2d");
  const ctxL = $("chart-life").getContext("2d");

  chartTemp = new Chart(ctxT, {
    type: "line",
    data: { labels: [], datasets: [{ data: [], borderColor: "#f85149", backgroundColor: "rgba(248,81,73,0.1)", fill: true, tension: 0.3, pointRadius: 2 }] },
    options: { ...CHART_OPTS, scales: { ...CHART_OPTS.scales, y: { ...CHART_OPTS.scales.y, title: { display: true, text: "°C", color: "#8b949e" } } } },
  });

  chartLife = new Chart(ctxL, {
    type: "line",
    data: { labels: [], datasets: [{ data: [], borderColor: "#3fb950", backgroundColor: "rgba(63,185,80,0.1)", fill: true, tension: 0.3, pointRadius: 2 }] },
    options: { ...CHART_OPTS },
  });
}

async function loadMetrics() {
  const hours = $("sel-hours").value;
  try {
    const rows = await (await fetch(`/api/metrics?hours=${hours}`)).json();

    const labels = rows.map(r => {
      const d = new Date(r.ts);
      return `${d.getHours().toString().padStart(2,"0")}:${d.getMinutes().toString().padStart(2,"0")}`;
    });
    const temps  = rows.map(r => r.temperature);
    const lifes  = rows.map(r => r.life_counter);

    chartTemp.data.labels                  = labels;
    chartTemp.data.datasets[0].data        = temps;
    chartTemp.update();

    chartLife.data.labels                  = labels;
    chartLife.data.datasets[0].data        = lifes;
    chartLife.update();
  } catch (e) {
    console.error("Metrics load failed", e);
  }
}

$("btn-refresh-metrics").addEventListener("click", loadMetrics);
$("sel-hours").addEventListener("change", loadMetrics);

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
})();
