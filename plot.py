import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("tec_log.csv")

fig, ax = plt.subplots()
ax.plot(df["tick"], df["victim_tec"], label="Victim TEC", color="red")
ax.plot(df["tick"], df["attacker_tec"], label="Attacker TEC", color="blue")

for state, color in [("ERROR_PASSIVE", "orange"), ("BUS_OFF", "red")]:
    ticks = df[df["victim_state"] == state]["tick"]
    if not ticks.empty:
        ax.axvline(ticks.iloc[0], color=color, linestyle="--", label=f"Victim -> {state}")

ax.set_xlabel("Tick")
ax.set_ylabel("TEC")
ax.set_title("Bus-Off Attack: TEC over time")
ax.legend()
plt.tight_layout()
plt.savefig("tec_plot.png", dpi=150)
plt.show()
