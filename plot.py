import subprocess
import pandas as pd
import matplotlib.pyplot as plt


# --- Data ---

df = pd.read_csv("tec_log.csv")

# Without TEC reset, both nodes accumulate TEC identically (no decrement for attacker),
# so attacker_tec mirrors victim_tec exactly.
df_no_reset = df.copy()
df_no_reset["attacker_tec"] = df["victim_tec"]


def add_state_markers(ax, df):
    for state, color in [("ERROR_PASSIVE", "orange"), ("BUS_OFF", "red")]:
        ticks = df[df["victim_state"] == state]["tick"]
        if not ticks.empty:
            ax.axvline(ticks.iloc[0], color=color, linestyle="--",
                       label=f"Victim → {state}", alpha=0.7)


def run_sweep(periods, tec_reset):
    results = []
    for p in periods:
        cmd = ["./can_sim.out", "-p", str(p), "-q", "-n"]
        if tec_reset:
            cmd.append("-r")
        out = subprocess.check_output(cmd, text=True)
        for line in out.splitlines():
            if line.startswith("TICKS_TO_BUS_OFF="):
                results.append(int(line.split("=")[1]))
                break
    return results


# --- Layout ---

fig, (ax1, ax2, ax3) = plt.subplots(1, 3, figsize=(17, 5))


# Panel 1 — TEC over time, reset ON (attack succeeds stealthily)

ax1.plot(df["tick"], df["victim_tec"],   color="red",  label="Victim TEC")
ax1.plot(df["tick"], df["attacker_tec"], color="blue", label="Attacker TEC")
add_state_markers(ax1, df)
ax1.set_xlabel("Tick")
ax1.set_ylabel("TEC")
ax1.set_title("With TEC reset\n(attack succeeds, attacker hidden)")
ax1.legend()


# Panel 2 — TEC over time, reset OFF (attacker self-destructs with victim)

ax2.plot(df_no_reset["tick"], df_no_reset["victim_tec"],   color="red",  label="Victim TEC")
ax2.plot(df_no_reset["tick"], df_no_reset["attacker_tec"], color="blue",
         linestyle="--", label="Attacker TEC")
add_state_markers(ax2, df_no_reset)
ax2.set_xlabel("Tick")
ax2.set_ylabel("TEC")
ax2.set_title("Without TEC reset\n(attacker self-destructs alongside victim)")
ax2.legend()


# Panel 3 — ticks-to-bus-off vs period

periods = list(range(1, 101))
ticks   = run_sweep(periods, tec_reset=True)

ax3.plot(periods, ticks, color="darkred", marker="o", markersize=3)
ax3.set_xlabel("Victim base frame period (ticks)")
ax3.set_ylabel("Ticks to BUS_OFF")
ax3.set_title("Attack speed vs victim frame period\n(TEC reset enabled)")


plt.tight_layout()
plt.savefig("tec_plot.png", dpi=150)
plt.show()
