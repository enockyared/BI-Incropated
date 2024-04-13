import serial
import time
import tkinter as tk
from tkinter import messagebox

def send_to_arduino(serial_port, data):
    serial_port.write(data.encode())
    time.sleep(0.1)  # Delay to ensure complete transmission

def send_data():
    # Get user input from the entry fields
    device_id = id_entry.get()
    wifi_username = username_entry.get()
    wifi_password = password_entry.get()

    # Construct the data packet
    data_packet = device_id + "\n" + wifi_username + "\n" +  wifi_password + "\n"

    # Send data to Arduino
    send_to_arduino(ser, data_packet)
    messagebox.showinfo("Success", "Data sent to Arduino successfully.")

def on_closing():
    # Close the serial connection before closing the GUI
    if messagebox.askokcancel("Quit", "Do you want to quit?"):
        ser.close()
        root.destroy()

# Configure the serial port
ser = serial.Serial('COM11', 9600)  # Change 'COM3' to the appropriate port
time.sleep(2)  # Wait for the serial connection to initialize

# Create the main window
root = tk.Tk()
root.title("Arduino Data Sender")
root.protocol("WM_DELETE_WINDOW", on_closing)

# Create and pack the widgets
id_label = tk.Label(root, text="ID:")
id_label.grid(row=0, column=0, padx=5, pady=5)

id_entry = tk.Entry(root)
id_entry.grid(row=0, column=1, padx=5, pady=5)

username_label = tk.Label(root, text="WiFi Username:")
username_label.grid(row=1, column=0, padx=5, pady=5)

username_entry = tk.Entry(root)
username_entry.grid(row=1, column=1, padx=5, pady=5)

password_label = tk.Label(root, text="WiFi Password:")
password_label.grid(row=2, column=0, padx=5, pady=5)

password_entry = tk.Entry(root)
password_entry.grid(row=2, column=1, padx=5, pady=5)

send_button = tk.Button(root, text="Send Data", command=send_data)
send_button.grid(row=3, column=0, columnspan=2, padx=5, pady=5, sticky="WE")

# Start the Tkinter event loop
root.mainloop()
