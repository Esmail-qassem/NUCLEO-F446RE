"""
Real-time STM32 MPU-6050 visualizer
Uses a complementary filter (accel + gyro) for smooth orientation.

Install deps:  pip install pyserial matplotlib numpy
Usage:         python visualizer.py          (edit COM_PORT below first)
"""

import math
import threading
import time

import matplotlib.pyplot as plt
import matplotlib.animation as animation
import numpy as np
from mpl_toolkits.mplot3d.art3d import Poly3DCollection
import serial

# ── configuration ──────────────────────────────────────────────────────────────
COM_PORT  = "COM5"   # <-- change to your port (check Device Manager)
BAUD_RATE = 115200
DT        = 0.05     # 50 ms = 20 Hz sample rate

# Complementary filter weight: 0.98 trusts gyro for fast motion,
# 0.02 trusts accel for slow drift correction.
ALPHA = 0.98
# ───────────────────────────────────────────────────────────────────────────────

state = {
    "roll": 0.0, "pitch": 0.0,
    "ax": 0, "ay": 0, "az": 0,
    "gx": 0, "gy": 0, "gz": 0,
    "connected": False,
}
lock = threading.Lock()


# ── serial reader thread ────────────────────────────────────────────────────────
def serial_reader():
    roll  = 0.0
    pitch = 0.0

    while True:
        try:
            ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=1)
            print(f"Connected to {COM_PORT}")
            with lock:
                state["connected"] = True

            while True:
                raw = ser.readline()
                try:
                    line = raw.decode("utf-8", errors="ignore").strip()
                except Exception:
                    continue

                if not line.startswith("$DATA:"):
                    continue

                parts = line[6:].split(",")
                if len(parts) != 6:
                    continue

                try:
                    ax, ay, az, gx, gy, gz = [int(p) for p in parts]
                except ValueError:
                    continue

                # Convert from x100 units
                ax_g  = ax / 100.0   # g
                ay_g  = ay / 100.0
                az_g  = az / 100.0
                gx_ds = gx / 100.0   # deg/s  (bias already subtracted on MCU)
                gy_ds = gy / 100.0
                gz_ds = gz / 100.0

                # Accel-only roll/pitch (noisy but drift-free)
                accel_roll  = math.degrees(math.atan2(ay_g, az_g))
                accel_pitch = math.degrees(math.atan2(-ax_g, math.sqrt(ay_g**2 + az_g**2)))

                # Complementary filter: blend gyro integration with accel
                roll  = ALPHA * (roll  + gx_ds * DT) + (1 - ALPHA) * accel_roll
                pitch = ALPHA * (pitch + gy_ds * DT) + (1 - ALPHA) * accel_pitch

                with lock:
                    state.update({"roll": roll, "pitch": pitch,
                                  "ax": ax, "ay": ay, "az": az,
                                  "gx": gx, "gy": gy, "gz": gz})

        except serial.SerialException as e:
            with lock:
                state["connected"] = False
            print(f"Serial error: {e} — retrying in 2 s")
            time.sleep(2)


# ── 3-D board geometry ──────────────────────────────────────────────────────────
def rotation_matrix(roll_deg, pitch_deg):
    r = math.radians(roll_deg)
    p = math.radians(pitch_deg)
    Rx = np.array([[1, 0,           0          ],
                   [0, math.cos(r), -math.sin(r)],
                   [0, math.sin(r),  math.cos(r)]])
    Ry = np.array([[ math.cos(p), 0, math.sin(p)],
                   [ 0,           1, 0           ],
                   [-math.sin(p), 0, math.cos(p)]])
    return Ry @ Rx


def board_verts(R):
    w, h, d = 1.5, 2.5, 0.08          # Nucleo-like proportions
    corners = np.array([
        [-w, -h, -d], [ w, -h, -d], [ w,  h, -d], [-w,  h, -d],
        [-w, -h,  d], [ w, -h,  d], [ w,  h,  d], [-w,  h,  d],
    ])
    return (R @ corners.T).T


def board_faces(v):
    return [
        [v[0], v[1], v[2], v[3]],   # bottom
        [v[4], v[5], v[6], v[7]],   # top  (green side)
        [v[0], v[1], v[5], v[4]],
        [v[2], v[3], v[7], v[6]],
        [v[0], v[3], v[7], v[4]],
        [v[1], v[2], v[6], v[5]],
    ]


# ── matplotlib setup ────────────────────────────────────────────────────────────
fig = plt.figure(figsize=(13, 6))
fig.patch.set_facecolor("#1e1e1e")

ax3d = fig.add_subplot(121, projection="3d")
ax3d.set_facecolor("#1e1e1e")

ax_txt = fig.add_subplot(122)
ax_txt.set_facecolor("#1e1e1e")
ax_txt.axis("off")
info = ax_txt.text(0.05, 0.95, "", transform=ax_txt.transAxes,
                   fontsize=12, verticalalignment="top",
                   fontfamily="monospace", color="white")


def animate(_frame):
    with lock:
        roll  = state["roll"]
        pitch = state["pitch"]
        ax_v  = state["ax"];  ay_v = state["ay"];  az_v = state["az"]
        gx_v  = state["gx"];  gy_v = state["gy"];  gz_v = state["gz"]
        conn  = state["connected"]

    ax3d.cla()
    ax3d.set_facecolor("#1e1e1e")
    ax3d.set_xlim(-3, 3); ax3d.set_ylim(-3, 3); ax3d.set_zlim(-3, 3)
    ax3d.set_xlabel("X", color="white"); ax3d.set_ylabel("Y", color="white")
    ax3d.set_zlabel("Z", color="white")
    ax3d.tick_params(colors="white")
    ax3d.xaxis.pane.fill = False
    ax3d.yaxis.pane.fill = False
    ax3d.zaxis.pane.fill = False
    title_color = "lime" if conn else "red"
    ax3d.set_title(f"Roll: {roll:+.1f}°   Pitch: {pitch:+.1f}°",
                   color=title_color, fontsize=11)

    R = rotation_matrix(roll, pitch)
    v = board_verts(R)
    face_colors = ["#2ecc71", "#27ae60", "#1a8a44", "#1a8a44", "#1a8a44", "#1a8a44"]
    poly = Poly3DCollection(board_faces(v), alpha=0.85,
                            facecolor=face_colors, edgecolor="#0d5c2e", linewidth=0.5)
    ax3d.add_collection3d(poly)

    # Body-frame axes
    for vec, col in zip(R.T, ["#e74c3c", "#2ecc71", "#3498db"]):
        ax3d.quiver(0, 0, 0, *vec * 2.2, color=col, linewidth=2, arrow_length_ratio=0.2)

    status = "CONNECTED" if conn else "DISCONNECTED"
    txt = (
        f"  Status : {status}\n\n"
        f"  {'':6}  {'Accel':>8}   {'Gyro':>9}\n"
        f"  {'─'*35}\n"
        f"  X  :  {ax_v/100:+7.2f} g   {gx_v/100:+7.2f} °/s\n"
        f"  Y  :  {ay_v/100:+7.2f} g   {gy_v/100:+7.2f} °/s\n"
        f"  Z  :  {az_v/100:+7.2f} g   {gz_v/100:+7.2f} °/s\n"
        f"\n"
        f"  Roll   : {roll:+.2f}°\n"
        f"  Pitch  : {pitch:+.2f}°\n"
        f"  Yaw    :  N/A  (no magnetometer)\n"
    )
    info.set_text(txt)


# ── start ───────────────────────────────────────────────────────────────────────
t = threading.Thread(target=serial_reader, daemon=True)
t.start()

ani = animation.FuncAnimation(fig, animate, interval=50, cache_frame_data=False)
plt.tight_layout()
plt.show()
