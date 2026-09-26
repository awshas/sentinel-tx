# Sentinel-TX — Requirements

Specifies the intended behaviour and measurement quality of the **Sentinel-TX**
industrial process transmitter.

## Conventions

### Modal verbs

| Verb | Meaning |
|---|---|
| **shall** | Binding requirement. Verifiable; its absence is a defect. |
| **should** | Goal or recommendation. Not binding, not verified. |
| **will** | Statement of fact about the environment, not an obligation on this system. |
| **may** | Permitted option. |

Only **shall** statements are requirements.

### Requirement format

    REQ_Cnn:

        <EARS statement using "shall">

        Rationale: <why this requirement exists>
        Verification: <Test | Analysis | Inspection | Demonstration>
        Method: <how it is verified>

`C` is the category digit; `nn` is a two-digit number within that category.

| Category | Range |
|---|---|
| Standards and interfaces | 1xx |
| Functionality | 2xx |
| Measurement quality | 3xx |
| Communication | 4xx |
| Fault handling and safety | 5xx |

### Statement patterns (EARS)

| Pattern | Form |
|---|---|
| Ubiquitous | The \<system\> shall \<response\>. |
| Event-driven | When \<trigger\>, the \<system\> shall \<response\>. |
| State-driven | While \<state\>, the \<system\> shall \<response\>. |
| Unwanted behaviour | If \<trigger\>, then the \<system\> shall \<response\>. |
| Optional feature | Where \<feature\>, the \<system\> shall \<response\>. |

`When` introduces an expected trigger; `if` introduces a fault or undesired
condition.

### Verification methods

| Method | Meaning |
|---|---|
| **Test** | Exercise the system and measure against a stated criterion. |
| **Analysis** | Calculation, modelling or worst-case analysis. |
| **Inspection** | Examine source, schematic or document. |
| **Demonstration** | Observe operation without instrumentation. |

### Identifier policy

Identifiers are permanent. A withdrawn requirement is marked *Deleted* and its
number is never reused. Gaps in the sequence are expected.

### Example

    REQ_999:

        While the device is in run mode, the Sentinel-TX shall toggle the
        heartbeat output at 1 Hz ±1 %.

        Rationale: Provides an external indication that the main loop is live.
        Verification: Test
        Method: Oscilloscope on the heartbeat pin, frequency measured over
                60 s.

---

## 1xx — Standards and interfaces

### REQ_101:

    Where the 4-20 mA current output is fitted, the Sentinel-TX shall signal
    a detected sensor fault by driving the loop current to <= 3.6 mA for
    under-range faults or >= 21.0 mA for over-range faults.

    Rationale: NAMUR NE43 clause 5. Both levels lie outside the 3.8-20.5 mA
    valid measuring range, so a fault cannot be mistaken for a process value.
    Scoped to the current-loop output, which is a Project 1 stretch goal.
    Verification: Test
    Method: Precision shunt and DMM in series with the loop; inject open and
            short sensor faults and record the resulting loop current.

### REQ_102:

    The Sentinel-TX 0-10 V output shall map the 0-150 degC measuring range to
    0.5-9.5 V, and shall signal a detected sensor fault by driving the output
    to 0.25 V +/-0.05 V for under-range faults or 9.75 V +/-0.05 V for
    over-range faults.

    Rationale: A 0-10 V interface has no live zero: 0 V is indistinguishable
    from minimum process value, a severed cable and an unpowered transmitter.
    Restricting the signal band to 0.5-9.5 V creates out-of-band regions for
    fault annunciation. This is NE43-inspired, NOT NE43 compliant; compliance
    requires a current loop. Cost: 10 % of output span, and correspondingly
    reduced resolution at the receiving PLC.
    Verification: Test
    Method: DMM on the output; sweep simulated Pt1000 resistance across and
            beyond the measuring range, record output voltage at each point.

---

## 2xx — Functionality

### REQ_201:

    While the external high-speed oscillator (HSE) is running, the
    Sentinel-TX shall operate with a system clock of 170 MHz ±0.5 %.

    Rationale: Maximum SYSCLK for STM32G474 in voltage scaling range 1 boost
    mode. Baud-rate and timer accuracy derive from it. The tolerance is only
    achievable from the crystal (24 MHz, 20 ppm on the NUCLEO-G474RE). The
    internal HSI16 used by the REQ_202 fallback is specified at
    15.88-16.08 MHz at 30 degC plus -1 to +1 % drift over 0-85 degC
    (DS12288 Rev 6, Table 43), so the fallback cannot meet it and is
    excluded by the "While" clause.
    Verification: Test
    Method: MCO output routed to a pin, frequency measured on a counter or
            scope against a known reference.

### REQ_202:

    If the external high-speed oscillator fails to stabilise within 100 ms of
    reset, then the Sentinel-TX shall continue operation using the internal
    high-speed oscillator and shall record a clock-source fault.

    Rationale: A stalled boot on crystal failure is an unacceptable field
    failure mode; degraded timing accuracy is preferable to a dead device.
    Verification: Test
    Method: Remove or ground the HSE crystal, confirm boot completes and the
            fault is reported by the `status` console command.

### REQ_203:

    The Sentinel-TX shall store two-point calibration data (offset and gain)
    in non-volatile memory, and shall apply the stored values after a power
    cycle without operator intervention.

    Rationale: Calibration is performed once at commissioning; loss on power
    cycle would require a field recalibration visit.
    Verification: Test
    Method: Apply known calibration via console, power-cycle, read back and
            confirm measurement accuracy is retained.

### REQ_204:

    If the stored calibration record fails its CRC check, then the Sentinel-TX
    shall apply factory default calibration values and shall report a
    calibration fault.

    Rationale: A partially written or corrupted calibration record must not
    produce a silently wrong process value. Defaults plus an annunciated fault
    is the safe degradation path.
    Verification: Test
    Method: Corrupt the calibration page over SWD, reset, confirm defaults are
            applied and the fault is reported.

### REQ_205:

    When a `save` command is issued and power is interrupted at any point
    during the write, the Sentinel-TX shall on the next boot present either
    the previous valid calibration record or the factory defaults, and shall
    never present a partially written record as valid.

    Rationale: Flash writes are not atomic. This is the same power-fail
    integrity property required at larger scale in Projects 3 and 9.
    Verification: Test
    Method: Scripted power interruption at 20 randomised points during the
            write sequence; confirm valid state on every subsequent boot.

---

## 3xx — Measurement quality

### REQ_301:

    The Sentinel-TX shall convert Pt1000 resistance to temperature with an
    error not exceeding +/-0.2 degC over the range 0-150 degC.

    Rationale: Comparable to commercial transmitters in this class
    (Endress+Hauser iTEMP, PR electronics 5334).
    Verification: Test
    Method: Decade resistance box substituted for the Pt1000, set to
            resistances corresponding to 0, 25, 50, 75, 100, 125 and 150 degC
            per IEC 60751. Reported temperature compared against the IEC 60751
            curve. Box accuracy shall be at least 4x better than the
            requirement. This verifies the conversion chain only; Pt1000
            element tolerance is a separate line in the system error budget.

### REQ_302:

    The Sentinel-TX analog output shall exhibit peak-to-peak ripple not
    exceeding 5 mV, measured at 50 % duty cycle, into a resistive load of
    >= 100 kOhm, over a 20 MHz measurement bandwidth.

    Rationale: A 12-bit PLC analog input spanning 0-10 V has an LSB of
    2.44 mV. Ripple below approximately 2 LSB is not resolvable by the
    receiving equipment. 50 % duty is specified because RC filter ripple is
    proportional to D(1-D) and is therefore worst at mid-scale.
    Verification: Test
    Method: Oscilloscope, AC-coupled, 1x probe with ground spring (not the
            alligator lead), bandwidth limit engaged, triggered from the PWM
            signal rather than from the ripple. Scope noise floor recorded
            separately with the probe tip shorted to its own ground ring; the
            noise floor shall be at least 4x below the limit for the result to
            be valid. If averaging is used, the count shall be recorded.

### REQ_303:

    The Sentinel-TX shall produce an updated temperature measurement at a rate
    of at least 10 Hz.

    Rationale: Process temperature loops typically require sub-second
    response; 10 Hz provides margin against PLC scan rates.
    Verification: Test
    Method: GPIO toggled on each completed measurement, period measured on a
            scope over 60 s including worst-case jitter.

---

## 4xx — Communication

### REQ_401:

    While the service console is operating at 921600 baud, the Sentinel-TX
    shall lose zero characters in 10^6 characters received.

    Rationale: Character loss in a configuration interface produces silently
    malformed commands. The bound states what the test can evidence; it is not
    a claim of zero loss in the limit.
    Verification: Test
    Method: Host script streams a continuous incrementing byte sequence
            without waiting for acknowledgement, so the receive buffer is
            loaded under sustained flow. Device echoes the sequence; host
            checks for discontinuities rather than comparing counts, since two
            compensating errors would pass a count comparison. Minimum 10^6
            characters, result reported in text form.

### REQ_402:

    When a valid console command is received, the Sentinel-TX shall begin
    transmitting its response within 100 ms.

    Rationale: Bounds console responsiveness for the host-side calibration
    tool, which must distinguish a slow response from a lost one.
    Verification: Test
    Method: Host script timestamps command transmission and first response
            byte over 1000 iterations; worst case recorded.

---

## 5xx — Fault handling and safety

### REQ_501:

    While any fault condition defined in REQ_502 through REQ_507 is active,
    the Sentinel-TX shall provide a visual fault indication.

    Rationale: Local annunciation allows a technician to identify a faulty
    unit without connecting a console.
    Verification: Test
    Method: Induce each fault in turn per its own method; confirm the
            indication is active throughout and clears on fault removal.

### REQ_502:

    If the Pt1000 sensor connection becomes open circuit, then the Sentinel-TX
    shall report a sensor-open fault on the console within 500 ms.

    Rationale: An open RTD reads as infinite resistance, which linearises to
    an implausibly high temperature. Undetected, this drives the process in
    the wrong direction.
    Verification: Test
    Method: Device running normally with a simulated sensor; open the
            connection during operation using a relay or switch. Time measured
            from the electrical transition to a GPIO toggled when the fault is
            latched, captured on a scope triggered from the relay drive.
            Starting the device already faulted verifies detection but not the
            500 ms deadline and is not sufficient.

### REQ_503:

    If the Pt1000 sensor connection becomes open circuit, then the Sentinel-TX
    shall activate the visual fault indication within 500 ms.

    Rationale: As REQ_502; separated because console and visual paths can fail
    independently.
    Verification: Test
    Method: As REQ_502, with the scope capturing the indication drive signal.

### REQ_504:

    If the Pt1000 sensor connection becomes short circuit, then the
    Sentinel-TX shall report a sensor-short fault on the console within
    500 ms.

    Rationale: A short reads as near-zero resistance, below the valid Pt1000
    range for any physically plausible process temperature.
    Verification: Test
    Method: As REQ_502, shorting the sensor terminals during operation.

### REQ_505:

    If the Pt1000 sensor connection becomes short circuit, then the
    Sentinel-TX shall activate the visual fault indication within 500 ms.

    Rationale: As REQ_504; separated for the same reason as REQ_503.
    Verification: Test
    Method: As REQ_503, shorting the sensor terminals during operation.

### REQ_506:

    If any sensor fault is active, then the Sentinel-TX shall drive the analog
    output to the applicable fault level defined in REQ_102 within 500 ms.

    Rationale: The receiving PLC has no other channel through which to learn
    the measurement is invalid. Without this the PLC continues to act on a
    stale or nonsensical value.
    Verification: Test
    Method: DMM or scope on the analog output; induce each sensor fault during
            operation and record the settled output level and the transition
            time.

### REQ_507:

    If the application fails to refresh the independent watchdog within its
    configured period, then the Sentinel-TX shall reset within 50 ms of the
    missed refresh deadline.

    Rationale: Bounds the duration for which a hung device can hold a stale
    output. The bound derives from the IWDG timeout configuration and must be
    consistent with REQ_506.
    Verification: Test
    Method: Console command deliberately suspends watchdog refresh. GPIO
            toggled immediately before the suspension and again early in
            Reset_Handler; interval measured on a scope.

### REQ_508:

    While a fault condition is latched, the Sentinel-TX shall retain the fault
    indication until an explicit clear command is received, even if the
    underlying condition has cleared.

    Rationale: An intermittent fault that self-clears before a technician
    arrives is the hardest class of field failure to diagnose. Latching
    preserves the evidence.
    Verification: Test
    Method: Induce a fault, remove it, confirm the indication persists;
            issue the clear command and confirm it releases.

---

## Traceability

Status is tracked here rather than in the requirement statements, so that the
specification and the implementation record have independent lifecycles.

| Requirement | Test artefact | Result | Evidence |
|---|---|---|---|
| REQ_101 | — | Not verified | — |
| REQ_102 | — | Not verified | — |
| REQ_201 | — | Not verified | — |
| REQ_202 | — | Not verified | — |
| REQ_203 | — | Not verified | — |
| REQ_204 | — | Not verified | — |
| REQ_205 | — | Not verified | — |
| REQ_301 | — | Not verified | — |
| REQ_302 | — | Not verified | — |
| REQ_303 | — | Not verified | — |
| REQ_401 | — | Not verified | — |
| REQ_402 | — | Not verified | — |
| REQ_501 | — | Not verified | — |
| REQ_502 | — | Not verified | — |
| REQ_503 | — | Not verified | — |
| REQ_504 | — | Not verified | — |
| REQ_505 | — | Not verified | — |
| REQ_506 | — | Not verified | — |
| REQ_507 | — | Not verified | — |
| REQ_508 | — | Not verified | — |