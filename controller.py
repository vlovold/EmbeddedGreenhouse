Ts = 1          # samplingstid [s]
Kp = 28          # proporsjonalforsterkning
Ki = 1.8          # integralforsterkning
deadband = 1.0    # %RH

integral = 0.0

fan_min = 0
fan_max = 1400

def humidity_controller(hum_measured, hum_ref):
    global integral

    error = hum_measured - hum_ref

    if abs(error) < deadband:
        return 0

    if error > 0:
        integral += error * Ts

        u = Kp * error + Ki * integral

        u_sat = max(fan_min, min(fan_max, u))

        # Anti-windup
        if u != u_sat:
            integral -= error * Ts

        return int(u_sat)

    else:
        integral = 0.0
        return 0