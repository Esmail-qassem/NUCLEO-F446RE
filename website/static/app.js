"use strict";

// ──────────────────────────────────────────────────────────────────
//  Elements
// ──────────────────────────────────────────────────────────────────
const $ = id => document.getElementById(id);

const dotConn    = $("dot-conn");
const dotEsp     = $("dot-esp");
const lblConn    = $("lbl-conn");
const lblEsp     = $("lbl-esp");
const lblRtc     = $("lbl-rtc");
const lblTemp    = $("lbl-temp");
const lblLife    = $("lbl-life");
const lblEcuVer  = $("lbl-ecu-ver");
const lblSrvVer  = $("lbl-srv-ver");
const termPort   = $("term-port");
const terminal   = $("terminal");
const selPort    = $("sel-port");
const selBaud    = $("sel-baud");
const btnConnect = $("btn-connect");
const cmdBtns    = document.querySelectorAll(".cmd-btn");
const chBtns     = document.querySelectorAll(".ch-btn");
const cmdFeedback  = $("cmd-feedback");
const inpRaw     = $("inp-raw");
const btnRaw     = $("btn-raw");
const dropZone   = $("drop-zone");
const dropInner  = $("drop-inner");
const inpFile    = $("inp-file");
const fileInfo   = $("file-info");
const fiName     = $("fi-name");
const fiSize     = $("fi-size");
const btnClearFile = $("btn-clear-file");
const inpVersion = $("inp-version");
const btnUpload  = $("btn-upload");
const progressWrap = $("upload-progress");
const progressBar  = $("progress-bar");
const uploadFeedback = $("upload-feedback");
const chkScroll  = $("chk-scroll");
const btnClearTerm = $("btn-clear-term");
const flashToast = $("flash-toast");

// ──────────────────────────────────────────────────────────────────
//  State
// ──────────────────────────────────────────────────────────────────
let isConnected    = false;
let espIp          = null;
let selectedFile   = null;
let toastTimer     = null;
let activeChannel  = "both";   // "wire" | "wifi" | "both"
const MAX_TERM_LEN = 50_000;

// ──────────────────────────────────────────────────────────────────
//  SocketIO
// ──────────────────────────────────────────────────────────────────
const socket = io({ transports: ["websocket", "polling"] });

socket.on("connect",    () => console.log("[ws] connected"));
socket.on("disconnect", () => applyState({ connected: false }));
socket.on("state", applyState);
socket.on("terminal", ({ data }) => appendTerminal(data));

socket.on("flash_result", ({ version, status }) => {
  const ok  = status === "ok";
  const msg = ok
    ? `✅  Flash OK  —  v${version}`
    : `❌  Flash FAILED  —  v${version}`;
  showToast(msg, ok ? "ok" : "err");
  appendTerminal(`\n[OTA] ${msg}\n`);
});

// ──────────────────────────────────────────────────────────────────
//  Apply state from server
// ──────────────────────────────────────────────────────────────────
function applyState(s) {
  if (s.connected !== undefined) {
    isConnected = s.connected;
    dotConn.className  = "dot" + (isConnected ? " online" : "");
    lblConn.textContent = isConnected
      ? `${s.serial_port} · ${s.baud}`
      : "Disconnected";
    btnConnect.textContent = isConnected ? "Disconnect" : "Connect";
    termPort.textContent   = isConnected
      ? `(${s.serial_port} · ${s.baud})`
      : "(not connected)";
  }

  // ESP WiFi status
  if ("esp_ip" in s) {
    espIp = s.esp_ip;
    const up = !!espIp;
    dotEsp.className   = "dot" + (up ? " online" : "");
    lblEsp.textContent = up ? espIp : "No ESP";
  }

  updateCmdAvailability();

  if (s.rtc_time)      lblRtc.textContent  = s.rtc_time;
  if (s.temperature)   lblTemp.textContent = s.temperature;
  if (s.life_counter)  lblLife.textContent = s.life_counter;
  if (s.ecu_version)   lblEcuVer.textContent = s.ecu_version;
  if (s.server_version) {
    lblSrvVer.textContent = s.server_version;
    inpVersion.placeholder = nextPatch(s.server_version);
  }

  const mismatch = s.ecu_version && s.server_version &&
                   s.ecu_version !== "–" && s.server_version !== "–" &&
                   s.ecu_version !== s.server_version;
  lblSrvVer.style.color = mismatch ? "var(--yellow-h)" : "";
  lblEcuVer.style.color = mismatch ? "var(--yellow-h)" : "";
}

function updateCmdAvailability() {
  const wireOk = isConnected;
  const wifiOk = !!espIp;

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
//  Terminal
// ──────────────────────────────────────────────────────────────────
function appendTerminal(text) {
  terminal.textContent += text;
  if (terminal.textContent.length > MAX_TERM_LEN) {
    terminal.textContent = terminal.textContent.slice(-MAX_TERM_LEN / 2);
  }
  if (chkScroll.checked) terminal.scrollTop = terminal.scrollHeight;
}

btnClearTerm.addEventListener("click", () => { terminal.textContent = ""; });

// ──────────────────────────────────────────────────────────────────
//  Connect / disconnect
// ──────────────────────────────────────────────────────────────────
async function loadPorts() {
  try {
    const res   = await fetch("/api/ports");
    const ports = await res.json();
    selPort.innerHTML = "";
    ports.forEach(({ port, desc }) => {
      const opt = document.createElement("option");
      opt.value       = port;
      opt.textContent = `${port}  –  ${desc}`;
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
//  Control commands
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
    } catch (e) {
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

// Raw byte sender
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

dropZone.addEventListener("dragover", e => {
  e.preventDefault();
  dropZone.classList.add("drag-over");
});
dropZone.addEventListener("dragleave", () => dropZone.classList.remove("drag-over"));
dropZone.addEventListener("drop", e => {
  e.preventDefault();
  dropZone.classList.remove("drag-over");
  const file = e.dataTransfer.files[0];
  if (file) selectFile(file);
});
inpFile.addEventListener("change", () => {
  if (inpFile.files[0]) selectFile(inpFile.files[0]);
});

function selectFile(file) {
  selectedFile            = file;
  fiName.textContent      = file.name;
  fiSize.textContent      = formatBytes(file.size);
  fileInfo.style.display  = "flex";
  dropZone.style.display  = "none";
  const m = file.name.match(/(\d+\.\d+\.\d+)/);
  if (m && !inpVersion.value) inpVersion.value = m[1];
}

btnClearFile.addEventListener("click", () => {
  selectedFile           = null;
  inpFile.value          = "";
  fileInfo.style.display = "none";
  dropZone.style.display = "block";
});

btnUpload.addEventListener("click", async () => {
  if (!selectedFile) {
    setFeedback(uploadFeedback, "Select a .bin file first", "err"); return;
  }
  const version = inpVersion.value.trim();
  if (!version) {
    setFeedback(uploadFeedback, "Enter a version tag (e.g. 1.0.6)", "err"); return;
  }

  const form = new FormData();
  form.append("firmware", selectedFile);
  form.append("version",  version);

  btnUpload.disabled = true;
  progressWrap.style.display = "block";
  progressBar.style.width    = "0%";
  setFeedback(uploadFeedback, "Uploading…", "");

  try {
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
          setFeedback(uploadFeedback,
            `✅  v${data.version} uploaded  (${formatBytes(data.size)})`, "ok");
          setTimeout(() => { progressWrap.style.display = "none"; }, 1200);
        } else {
          setFeedback(uploadFeedback, data.error || "Upload failed", "err");
        }
      } catch {
        setFeedback(uploadFeedback, "Unexpected server response", "err");
      }
    };

    xhr.onerror = () => {
      btnUpload.disabled = false;
      setFeedback(uploadFeedback, "Network error", "err");
    };

    xhr.send(form);
  } catch (e) {
    btnUpload.disabled = false;
    setFeedback(uploadFeedback, `Error: ${e.message}`, "err");
  }
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
  if (n < 1024)         return `${n} B`;
  if (n < 1024 * 1024)  return `${(n / 1024).toFixed(1)} KB`;
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
  } catch { /* server not ready yet */ }
})();
