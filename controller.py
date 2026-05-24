import time

Ts = 1          # samplingstid [s]
Kp = 28         # proporsjonalforsterkning
Ki = 1.8        # integralforsterkning
deadband = 1.0  # %RH

integral = 0.0

fan_min = 100
fan_max = 1400

# Kickstart-variabler
fan_running = False
fan_kick_until = 0

def humidity_controller(hum_measured, hum_ref):

    global integral
    global fan_running
    global fan_kick_until

    error = hum_measured - hum_ref

    # Ingen regulering hvis vi er innenfor deadband
    if abs(error) < deadband:
        integral = 0.0
        fan_running = False
        return 0

    # Kun reguler hvis luftfuktigheten er for høy
    if error > 0:

        integral += error * Ts

        u = Kp * error + Ki * integral

        # Saturering
        u_sat = max(fan_min, min(fan_max, u))

        # Anti-windup
        if u != u_sat:
            integral -= error * Ts

        now = time.time()

        # Start kickstart hvis vifta starter fra stopp
        if not fan_running:
            fan_running = True
            fan_kick_until = now + 0.5

        # Kjør kickstart i 0.5 sekunder
        if now < fan_kick_until:
            return 250

        return int(u_sat)

    else:
        integral = 0.0
        fan_running = False
        return 0