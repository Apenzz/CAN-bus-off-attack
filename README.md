# CAN Bus-Off Attack Simulation

A discrete-time simulation of the CAN Bus-Off attack written in C. The simulation models the error-handling mechanism and demonstrates how a malicious node can force a victim ECU off the bus by deliberately causing repeated frame collisions.

---

## Background

### CAN Error Handling

Every CAN node maintains two internal counters:

- **TEC** (Transmit Error Counter) — incremented by **+8** on each transmission error, decremented by **1** on each successful transmission
- **REC** (Receive Error Counter) — incremented by **+1** when an error is observed as a receiver, decremented by **1** on successful reception

Based on these counters, each node transitions through three states:

| State | Condition | Behaviour |
|---|---|---|
| `ERROR_ACTIVE` | TEC < 128 and REC < 128 | Normal operation; sends active error flags |
| `ERROR_PASSIVE` | TEC ≥ 128 or REC ≥ 128 | Reduced influence; sends passive error flags |
| `BUS_OFF` | TEC > 255 | Node disconnects from the bus entirely |

### The Bus-Off Attack

The attack exploits this mechanism. A collision on the CAN bus occurs when two nodes transmit a frame with the same ID but different data or DLC. Both transmitters increment their TEC by 8.

A standard ECU cannot prevent its TEC from rising. However, a custom or modified CAN controller can reset its own TEC to 0 after each collision. That's how the attacker works in this simulation.

The attack strategy is therefore:

1. Monitor the bus for the victim's frame ID
2. Immediately transmit a frame with the same ID but corrupted data
3. A collision occurs — both TECs increment by 8
4. The attacker resets its TEC to 0; the victim cannot
5. Repeat until the victim's TEC exceeds 255 and it enters `BUS_OFF`

Once in `BUS_OFF`, the victim ECU stops all communication. 

---

## What the Simulation Models

- A **victim ECU** that sends three periodic CAN frames:

  | ID | DLC | Data | Period |
  |---|---|---|---|
  | `0x101` | 8 | `FF FF FF FF FF FF FF FF` | 10 ticks |
  | `0x102` | 1 | `4C` | 20 ticks |
  | `0x103` | 2 | `D2 09` | 50 ticks |

- An **attacker ECU** that detects the victim's active transmission each tick, mirrors the frame ID, and corrupts the first byte (`data[0] ^= 0xFF`)

- A **CAN bus** that performs bitwise arbitration (lowest ID wins), detects collisions (same ID, different content), and notifies all nodes

- The **error counter rules**: +8 TEC on collision, −1 TEC on success, +1 REC for listening receivers on a collision

- The **attacker's TEC reset** after every collision it causes, simulating a custom controller

- A **CSV log** (`tec_log.csv`) recording `tick`, `victim_tec`, `attacker_tec`, and `victim_state` at every tick

---


## How to Build and Run

**Requirements:** `gcc`, `make`

```bash
# Build
make

# Build and run
make run

# Clean build artifacts
make clean
```

### Output

The simulation prints a progress report every 100 ticks and a full summary at the end.
It also writes `tec_log.csv` in the working directory.

---

## Visualising Results

**Requirements:** Python 3, `pandas`, `matplotlib`

```bash
pip install pandas matplotlib
python plot.py
```

This produces `tec_plot.png` showing victim and attacker TEC over time with vertical markers at the `ERROR_PASSIVE` and `BUS_OFF` transition points.

---

## Known Simplifications

The following aspects of real CAN are intentionally omitted or approximated to keep the simulation tractable. They represent gaps between this model and a fully realistic implementation.

### Timing model
Each simulation tick represents one complete frame transmission. Real CAN operates at bit-level resolution. The simulation has no concept of bit timing, propagation delay, or clock synchronisation.

### Attacker reaction time
The attacker detects and jams the victim's frame within the same tick it starts transmitting. In practice, the attacker must detect the start-of-frame bit, decode the ID field, and begin transmitting before the arbitration phase ends — all within a few microseconds. This timing constraint is not modelled.

### Error frames
When a collision is detected in real CAN, the detecting node transmits an **error frame** (6 dominant bits followed by 8 recessive bits). This consumes bus time and delays the next frame. The simulation skips error frames entirely; a collision is resolved instantaneously within a single tick.

### ACK field
Every successful CAN transmission requires at least one receiver to assert the ACK slot. The simulation does not model acknowledgement. Frames are considered transmitted without requiring any receiver response.

### Standard 11-bit IDs only
The simulation only uses CAN standard frames (11-bit IDs). CAN extended frames (29-bit IDs) are not modeled.
