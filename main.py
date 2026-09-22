import rp2
import time
from imu import MPU6050
from machine import Pin, I2C

@rp2.asm_pio(in_shiftdir=rp2.PIO.SHIFT_LEFT)
def scroll_reader():
    wrap_target()
    wait(0, pin, 0)
    in_(pins, 2)
    push(noblock)
    wait(1, pin, 0) [10]
    wrap()

pin_a = Pin(14, Pin.IN, Pin.PULL_UP)
pin_b = Pin(15, Pin.IN, Pin.PULL_UP)

sm = rp2.StateMachine(
    0, 
    scroll_reader, 
    freq=2000, 
    in_base=pin_a  # Sets Pin 14 as base. Pin 15 is automatically read via in_(pins, 2)
)

sm.active(1)
print("PIO State Machine running...")

scroll_position = 0

i2c = I2C(0, sda=Pin(0), scl=Pin(1), freq=400000)
imu = MPU6050(i2c)

while True:
    while sm.rx_fifo():
        direction_data = sm.get()
        
        if direction_data == 2:
            scroll_position += 1
            print("-1,",end="")
        else:
            scroll_position -= 1
            print("1,",end="")
    ax = round(imu.accel.x, 2)
    ay = round(imu.accel.y, 2)
    az = round(imu.accel.z, 2)
    gx=round(imu.gyro.x)
    gy=round(imu.gyro.y)
    gz=round(imu.gyro.z)
    print(f"{ax},{ay},{az},{gx},{gy},{gz}")
    
    
    time.sleep(0.04)
            
