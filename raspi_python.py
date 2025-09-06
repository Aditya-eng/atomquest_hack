import serial, time, json

serial_port = '/dev/ttyUSB0'   # change if needed
baud = 115200

ser = serial.Serial(serial_port, baud, timeout=1)
time.sleep(2)

# thresholds (tune these experimentally)
OBSTACLE_CLOSE = 18    # cm -> treat as immediate obstacle
OBSTACLE_WARN  = 50    # cm -> start caution
EDGE_TIMEOUT   = 900   # Arduino returns 999 on timeout -> treat as edge

def send(cmd):
    ser.write(cmd.encode())

def decide_and_act(data):
    f = int(data.get('F', 999))
    l = int(data.get('L', 999))
    r = int(data.get('R', 999))
    print(f"F={f}cm L={l}cm R={r}cm")

    # Edge detection (no echo or very large)
    if f >= EDGE_TIMEOUT:
        print("EDGE detected! Backing off and turning")
        send('S'); time.sleep(0.05)
        send('B'); time.sleep(0.4)   # back for 400ms
        send('S'); time.sleep(0.05)
        send('L'); time.sleep(0.45)  # turn left ~ adjust timing to get ~90deg
        send('S'); time.sleep(0.05)
        return

    # obstacle ahead close
    if f > 0 and f < OBSTACLE_CLOSE:
        print("Obstacle close -> evasive")
        send('S'); time.sleep(0.05)
        send('B'); time.sleep(0.3)
        send('S'); time.sleep(0.05)
        # choose which way to turn based on side sensors
        if l > r:
            send('L'); time.sleep(0.35)
        else:
            send('R'); time.sleep(0.35)
        send('S'); time.sleep(0.05)
        return

    # mild caution zone: slow or steer away from closer side
    if f < OBSTACLE_WARN:
        # if left is clearer, nudge left; else nudge right
        if l > r:
            send('L'); time.sleep(0.12)
            send('F')
        else:
            send('R'); time.sleep(0.12)
            send('F')
        return

    # clear: go forward
    send('F')

# main loop
buf = ""
while True:
    try:
        line = ser.readline().decode(errors='ignore').strip()
        if not line:
            continue
        # expect {"F":xx,"L":yy,"R":zz}
        if line.startswith('{') and line.endswith('}'):
            try:
                data = json.loads(line)
                decide_and_act(data)
            except Exception as e:
                print("parse err", e, line)
    except KeyboardInterrupt:
        send('S')
        break
    except Exception as e:
        print("err", e)
        time.sleep(0.5)
