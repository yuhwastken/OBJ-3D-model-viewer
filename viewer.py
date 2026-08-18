import pyvista as pv
import numpy as np
import time
import serial
import sys
import glob

def serial_ports():
    """ Lists serial port names

        :raises EnvironmentError:
            On unsupported or unknown platforms
        :returns:
            A list of the serial ports available on the system
    """
    if sys.platform.startswith('win'):
        ports = ['COM%s' % (i + 1) for i in range(256)]
    elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
        # this excludes your current terminal "/dev/tty"
        ports = glob.glob('/dev/tty[A-Za-z]*')
    elif sys.platform.startswith('darwin'):
        ports = glob.glob('/dev/tty.*')
    else:
        raise EnvironmentError('Unsupported platform')

    result = []
    for port in ports:
        try:
            s = serial.Serial(port)
            s.close()
            result.append(port)
        except (OSError, serial.SerialException):
            pass
    return result

ports = serial_ports()
print(ports)
PORT = ports[int(input("Select serial port: "))-1]
BAUD_RATE = 115200
ser = serial.Serial(PORT, BAUD_RATE, timeout=1)
print(f"Listening on {PORT}... Press Ctrl+C to stop.")

ax,ay,az = 0.01,0.01,0.01
pitch = 0
roll = 0
yaw = 0
height = 0
pv.global_theme.allow_empty_mesh = True

object_mesh = pv.read("Untitled.obj")
object_text = pv.read_texture("heart.jpg")
object_mesh = object_mesh.rotate_x(-90,inplace=False)

plane_mesh = pv.Plane()

plot = pv.Plotter()
plot.add_mesh(object_mesh.copy(),name="object",texture=object_text)
plot.add_mesh(plane_mesh.copy(),name="plane",color="orange",opacity=0.4)
last = time.time()

def rotation_matrix(r,p,y):
    x = np.cos(y)*np.sin(p)*np.cos(r)+np.sin(y)*np.sin(r)
    y = np.sin(y)*np.sin(p)*np.cos(r)-np.cos(y)*np.sin(r)
    z = np.cos(p)*np.cos(r)
    return [x,y,z]

def py_callback(step):
    global ax,ay,az,gx,gy,gz,roll,pitch,yaw,last,height
    current = time.time()
    dt = current-last
    last = current
    if ser.in_waiting > 0:
        raw_data = ser.readline().decode('utf-8').strip()
        data = raw_data.split(',')
        if len(data)==6:
            ax,ay,az,gx,gy,gz = data
            ax,ay,az,gx,gy,gz = float(ax),float(ay),float(az),np.radians(float(gx)),np.radians(float(gy)),np.radians(float(gz))
            roll = 0.9*(roll+gx*dt)+0.1*round(np.arctan2(ay,az),2)
            pitch = 0.9*(pitch+gy*dt)+0.1*round(np.arctan2(-ax,np.sqrt(ay**2 + az**2)),2)
            yaw = yaw + (gz*dt)
            ser.reset_input_buffer()
        elif len(data)==7:
            if (height+1 > 30) or (height-1 < -30):
                height=0
            else:
                height += int(data[0])
                print(height)
            ax,ay,az,gx,gy,gz = data[1:]
            ax,ay,az,gx,gy,gz = float(ax),float(ay),float(az),np.radians(float(gx)),np.radians(float(gy)),np.radians(float(gz))
            roll = 0.9*(roll+gx*dt)+0.1*round(np.arctan2(ay,az),2)
            pitch = 0.9*(pitch+gy*dt)+0.1*round(np.arctan2(-ax,np.sqrt(ay**2 + az**2)),2)
            yaw = yaw + (gz*dt)
            ser.reset_input_buffer()

    clipped_object = object_mesh.clip(normal=rotation_matrix(roll,pitch,yaw),origin =(0,0,0.1*height))
    cur_plane = plane_mesh.copy()
    cur_plane.rotate_x(np.degrees(roll),inplace=True)
    cur_plane.rotate_y(np.degrees(pitch),inplace=True)
    cur_plane.rotate_z(np.degrees(yaw),inplace=True)
    cur_plane.translate((0,0,0.1*height),inplace=True)
    plot.add_mesh(clipped_object,name="object",texture=object_text)
    plot.add_mesh(cur_plane,name="plane",color="orange",opacity=0.4)

    plot.render()

plot.add_timer_event(max_steps=10000, duration=25, callback=py_callback)
plot.show()
