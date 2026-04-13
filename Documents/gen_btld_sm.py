"""
Generate Bootloader State Machine PDF — NUCLEO-F446RE
Run:  python gen_btld_sm.py
"""
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch

OUTPUT = "BTLD_State_Machine.pdf"

fig, ax = plt.subplots(figsize=(11.69, 16.54))  # A3-ish portrait
fig.patch.set_facecolor("#ffffff")
ax.set_facecolor("#ffffff")
ax.set_xlim(0, 10)
ax.set_ylim(0, 15)
ax.axis("off")

# ── Colours ──────────────────────────────────────────────────────
C_IDLE   = "#e8f5e9"
C_RX     = "#e3f2fd"
C_CHECK  = "#fff3e0"
C_FLASH  = "#fce4ec"
C_OK     = "#c8e6c9"
C_FAIL   = "#ffcdd2"
C_EDGE   = "#37474f"
C_TITLE  = "#1a237e"
C_TIMEOUT = "#ffecb3"

def state_box(x, y, w, h, label, sublabel, color, bold=False):
    box = FancyBboxPatch((x - w/2, y - h/2), w, h,
                         boxstyle="round,pad=0.15", linewidth=1.5,
                         edgecolor=C_EDGE, facecolor=color)
    ax.add_patch(box)
    weight = "bold" if bold else "semibold"
    ax.text(x, y + 0.12, label, ha="center", va="center",
            fontsize=10, fontweight=weight, color="#212121")
    if sublabel:
        ax.text(x, y - 0.22, sublabel, ha="center", va="center",
                fontsize=7, color="#616161", style="italic")

def arrow(x1, y1, x2, y2, label="", color=C_EDGE, style="-|>", curved=False):
    if curved:
        arrowprops = dict(arrowstyle=style, color=color, lw=1.3,
                          connectionstyle="arc3,rad=0.3")
    else:
        arrowprops = dict(arrowstyle=style, color=color, lw=1.3)
    ax.annotate("", xy=(x2, y2), xytext=(x1, y1), arrowprops=arrowprops)
    if label:
        mx, my = (x1 + x2) / 2, (y1 + y2) / 2
        ax.text(mx, my, label, ha="center", va="center",
                fontsize=7, color="#b71c1c" if "FAIL" in label or "NAK" in label else "#1b5e20",
                fontweight="bold",
                bbox=dict(boxstyle="round,pad=0.15", fc="white", ec="none", alpha=0.85))

# ── Title ────────────────────────────────────────────────────────
ax.text(5, 14.6, "NUCLEO-F446RE  Bootloader State Machine", ha="center",
        fontsize=16, fontweight="bold", color=C_TITLE)
ax.text(5, 14.25, "Binary Mode  |  UART2 @ 115200  |  APP @ 0x08008000",
        ha="center", fontsize=9, color="#546e7a")

# ── Protocol box ─────────────────────────────────────────────────
proto_text = (
    "Packet layout (little-endian):\n"
    "[0x55 sync] [MagicNumber 4B] [CRC32 4B] [ImageSize 4B] [Firmware data...]"
)
ax.text(5, 13.65, proto_text, ha="center", va="center", fontsize=8,
        fontfamily="monospace", color="#37474f",
        bbox=dict(boxstyle="round,pad=0.4", fc="#f5f5f5", ec="#bdbdbd", lw=1))

# ── States ───────────────────────────────────────────────────────
# Row positions (y)
Y0 = 12.6   # WAIT_SYNC
Y1 = 11.2   # RECEIVE_HEADER
Y2 = 9.8    # RECEIVE_DATA
Y3 = 8.4    # CRC_CHECK
Y4 = 7.0    # ERASE_FLASH
Y5 = 5.6    # PROGRAM_FLASH
Y6 = 4.2    # VERIFY
Y7 = 2.8    # RESET (success)

X_MAIN = 5
X_FAIL = 8.2

state_box(X_MAIN, Y0, 3.2, 0.7, "WAIT_SYNC",
          "sync_received == 0  |  wait for 0x55", C_IDLE, bold=True)

state_box(X_MAIN, Y1, 3.2, 0.7, "RECEIVE_HEADER",
          "bin_index 0-11  |  parse Magic, CRC, Size", C_RX)

state_box(X_MAIN, Y2, 3.2, 0.7, "RECEIVE_DATA",
          "bin_index 12 .. ImageSize+12", C_RX)

state_box(X_MAIN, Y3, 3.2, 0.7, "CRC_CHECK",
          "CRC32(bin_buffer+12, ImageSize) == received_crc?", C_CHECK)

state_box(X_MAIN, Y4, 3.2, 0.7, "ERASE_FLASH",
          "FlashDrv_EraseSector(2)  +  sector(3)", C_FLASH)

state_box(X_MAIN, Y5, 3.2, 0.7, "PROGRAM_FLASH",
          "FlashDrv_ProgramBufferAligned(0x08008000, ...)", C_FLASH)

state_box(X_MAIN, Y6, 3.2, 0.7, "VERIFY",
          "Check SP [0x2000xxxx] + ResetHandler [0x0800xxxx]", C_CHECK)

state_box(X_MAIN, Y7, 3.2, 0.7, "RESET (ACK)",
          "Send 0xAC to ESP  |  SCB_AIRCR reset", C_OK, bold=True)

# ── Failure / retry state ────────────────────────────────────────
state_box(X_FAIL, Y3, 2.0, 0.7, "RETRY",
          "Send NAK (0xFA)\nretry_count++", C_FAIL)

state_box(X_FAIL, Y6, 2.0, 0.7, "RETRY",
          "Send NAK (0xFA)\nretry_count++", C_FAIL)

# ── FATAL state ──────────────────────────────────────────────────
state_box(X_FAIL, 1.5, 2.0, 0.7, "FATAL",
          "Send 0xFB  |  retry >= 3  |  abort", C_FAIL, bold=True)

# ── TIMEOUT state ────────────────────────────────────────────────
state_box(1.8, Y3, 2.0, 0.7, "TIMEOUT",
          "ms_ticks > 20 000  |  SCB_AIRCR reset", C_TIMEOUT)

# ── Arrows (main flow) ──────────────────────────────────────────
arrow(X_MAIN, Y0 - 0.35, X_MAIN, Y1 + 0.35, "byte == 0x55")
arrow(X_MAIN, Y1 - 0.35, X_MAIN, Y2 + 0.35, "bin_index == 12")
arrow(X_MAIN, Y2 - 0.35, X_MAIN, Y3 + 0.35, "bin_index == ImageSize+12")
arrow(X_MAIN, Y3 - 0.35, X_MAIN, Y4 + 0.35, "CRC OK")
arrow(X_MAIN, Y4 - 0.35, X_MAIN, Y5 + 0.35, "sectors erased")
arrow(X_MAIN, Y5 - 0.35, X_MAIN, Y6 + 0.35, "flash_done = 1")
arrow(X_MAIN, Y6 - 0.35, X_MAIN, Y7 + 0.35, "SP & Reset valid")

# ── Arrows (failure paths) ──────────────────────────────────────
arrow(X_MAIN + 1.6, Y3, X_FAIL - 1.0, Y3, "CRC FAIL", color="#c62828")
arrow(X_MAIN + 1.6, Y6, X_FAIL - 1.0, Y6, "VERIFY FAIL", color="#c62828")

# Retry loops back to WAIT_SYNC
arrow(X_FAIL, Y3 + 0.35, X_FAIL, Y0 + 0.0,
      "retry < 3", color="#e65100", curved=True)
arrow(X_FAIL, Y6 + 0.35, X_FAIL - 0.0, Y0 + 0.0,
      "retry < 3", color="#e65100", curved=True)

# Retry → FATAL
arrow(X_FAIL, Y3 - 0.35, X_FAIL, 1.85, "retry >= 3", color="#c62828")
arrow(X_FAIL, Y6 - 0.35, X_FAIL, 1.85, "retry >= 3", color="#c62828", curved=True)

# Timeout arrow
arrow(2.8, Y3, 2.8, Y3, "")  # placeholder
ax.annotate("", xy=(1.8, Y3 + 0.35), xytext=(3.4, Y2 - 0.1),
            arrowprops=dict(arrowstyle="-|>", color="#f57f17", lw=1.3,
                            connectionstyle="arc3,rad=-0.2"))
ax.text(2.2, Y2 + 0.2, "20s no data", fontsize=7, color="#f57f17",
        fontweight="bold", rotation=0)

# ── Legend ────────────────────────────────────────────────────────
legend_y = 0.7
ax.text(0.5, legend_y, "Legend:", fontsize=9, fontweight="bold", color="#37474f")
for i, (c, lbl) in enumerate([
    (C_IDLE,  "Idle / Init"),
    (C_RX,    "Receiving"),
    (C_CHECK, "Validation"),
    (C_FLASH, "Flash Operation"),
    (C_OK,    "Success"),
    (C_FAIL,  "Error / Retry"),
    (C_TIMEOUT, "Timeout"),
]):
    x = 2.2 + i * 1.15
    ax.add_patch(FancyBboxPatch((x, legend_y - 0.13), 0.3, 0.26,
                 boxstyle="round,pad=0.05", fc=c, ec=C_EDGE, lw=0.8))
    ax.text(x + 0.4, legend_y, lbl, fontsize=6.5, va="center", color="#37474f")

# ── Footer ───────────────────────────────────────────────────────
ax.text(5, 0.15, "Auto-generated from Parse.c  |  BIN_MODE  |  3 retries max  |  ACK=0xAC  NAK=0xFA  FATAL=0xFB",
        ha="center", fontsize=7, color="#9e9e9e")

plt.tight_layout(pad=0.5)
plt.savefig(OUTPUT, dpi=150, bbox_inches="tight")
print(f"Saved: {OUTPUT}")
