import serial
import time
import tkinter as tk

try:
    arduino = serial.Serial('COM5', 9600, timeout=1)
    time.sleep(2)
    print("Arduino connected successfully")
except Exception as e:
    print("Arduino connection failed:", e)
    arduino = None

def send_command(command):
    if arduino and arduino.is_open:
        try:
            arduino.write((command + '\n').encode())
            print(f"Sent command: {command}")
        except Exception as e:
            print("Failed to send command:", e)

def read_temps():
    if arduino and arduino.is_open:
        try:
            while arduino.in_waiting:
                line = arduino.readline().decode().strip()
                print(f"Received: {line}")
                
                # Parse: "R:25.5 A:22.3 B:23.1"
                if line.startswith("R:"):
                    parts = line.split()
                    for sec in parts:
                        if sec.startswith("R:"):
                            temp_val = sec.split(":")[1]
                            reactor_temp.set(temp_val + "°C")
                        elif sec.startswith("A:"):
                            temp_val = sec.split(":")[1]
                            precA_temp.set(temp_val + "°C")
                        elif sec.startswith("B:"):
                            temp_val = sec.split(":")[1]
                            precB_temp.set(temp_val + "°C")
        except Exception as e:
            print(f"Error reading temps: {e}")
    
    # Schedule this function to run again after 100ms
    root.after(100, read_temps)

def stop_all():
    send_command('STOP')

def set_reactor():
    temp = reactor_entry.get()
    send_command(f'R:{temp}')

def set_precA():
    temp = precA_entry.get()
    send_command(f'A:{temp}')

def set_precB():
    temp = precB_entry.get()
    send_command(f'B:{temp}')

root = tk.Tk()
root.title("Heater/PID control")
root.geometry("800x600")

reactor_temp = tk.StringVar(value="--")
precA_temp = tk.StringVar(value="--")
precB_temp = tk.StringVar(value="--")

if not arduino:
    warning_box = tk.Frame(root, bg="white", bd=3, relief="ridge", padx=10, pady=10)
    warning_box.grid(row=0, column=3, sticky="ne", padx=20, pady=20)
    tk.Label(warning_box, text="⚠️ Arduino not connected", fg="red", font=("Arial", 12, "bold"), bg="white").pack()
elif arduino:
    success = tk.Frame(root, bg="white", bd=3, relief="ridge", padx=20, pady=20)
    success.grid(row=0, column=3, sticky="ne", padx=20, pady=20)
    tk.Label(success, text="✅ Arduino connected", fg="green", font=("Arial", 12, "bold"), bg="white").pack()
    
    root.after(100, read_temps)

tk.Label(root, text="Reactor", font=("Arial", 14, "bold")).grid(row=1, column=0, padx=20, pady=10)
reactor_entry = tk.Entry(root, font=("Arial", 12), width=10)
reactor_entry.grid(row=1, column=1, padx=5)
reactor_entry.insert(0, "150")
tk.Button(root, text="Set", command=set_reactor, bg="blue", fg="white", font=("Arial", 12)).grid(row=1, column=2, padx=5)
tk.Label(root, text="Reactor Temp:", font=("Arial", 12)).grid(row=2, column=0, sticky="e")
tk.Label(root, textvariable=reactor_temp, font=("Arial", 16), fg="blue").grid(row=2, column=1, columnspan=2, sticky="w")
tk.Button(root, text="OFF", command=lambda: send_command('R:0'), bg="red", fg="white", font=("Arial", 12)).grid(row=1, column=3, padx=5)

tk.Label(root, text="Precursor A", font=("Arial", 14, "bold")).grid(row=3, column=0, padx=20, pady=10)
precA_entry = tk.Entry(root, font=("Arial", 12), width=10)
precA_entry.grid(row=3, column=1, padx=5)
precA_entry.insert(0, "120")
tk.Button(root, text="Set", command=set_precA, bg="blue", fg="white", font=("Arial", 12)).grid(row=3, column=2, padx=5)
tk.Label(root, text="Precursor A Temp:", font=("Arial", 12)).grid(row=4, column=0, sticky="e")
tk.Label(root, textvariable=precA_temp, font=("Arial", 16), fg="blue").grid(row=4, column=1, columnspan=2, sticky="w")
tk.Button(root, text="OFF", command=lambda: send_command('A:0'), bg="red", fg="white", font=("Arial", 12)).grid(row=3, column=3, padx=5)


tk.Label(root, text="Precursor B", font=("Arial", 14, "bold")).grid(row=5, column=0, padx=20, pady=10)
precB_entry = tk.Entry(root, font=("Arial", 12), width=10)
precB_entry.grid(row=5, column=1, padx=5)
precB_entry.insert(0, "110")
tk.Button(root, text="Set", command=set_precB, bg="blue", fg="white", font=("Arial", 12)).grid(row=5, column=2, padx=5)
tk.Label(root, text="Precursor B Temp:", font=("Arial", 12)).grid(row=6, column=0, sticky="e")
tk.Label(root, textvariable=precB_temp, font=("Arial", 16), fg="blue").grid(row=6, column=1, columnspan=2, sticky="w")
tk.Button(root, text="OFF", command=lambda: send_command('B:0'), bg="red", fg="white", font=("Arial", 12)).grid(row=5, column=3, padx=5)


tk.Button(root, text="STOP ALL", command=stop_all, bg="red", fg="white", font=("Arial", 14, "bold"), width=20).grid(row=7, column=0, columnspan=3, pady=20)

root.mainloop()

# Cleanup
if arduino and arduino.is_open:
    arduino.close()