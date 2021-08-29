import tkinter
import time
import threading
import os
import sys
import pyengine

from pynput.mouse import Listener

canvas_width = 950
canvas_height = 800

TOP_BAR_HEIGHT = 40
ADDRESS_BAR_WIDTH = 170
BYTE_CONTAINER_SIZE = 20

last_data = os.urandom(0x1000)
new_data = last_data
max_container_groups = 0
last_change_from_scroll = False

real_use = False
pid = 0
start_addr = 0
if len(sys.argv) == 3:
	if "0x" in sys.argv[1]:
		pid = int(sys.argv[1], 16)
		start_addr = int(sys.argv[2], 16)
	else:
		pid = int(sys.argv[1])
		start_addr = int(sys.argv[2])
	new_data = pyengine.efi_read_bytes(pid, start_addr, 0x1000)
	real_use = True
else:
	new_data = os.urandom(0x1000)

def get_new_bytes_data(pid, start_addr):
	global new_data, last_data, max_container_groups
	if len(sys.argv) == 3:
		new_data = pyengine.efi_read_bytes(pid, start_addr, 0x1000)
	else:
		new_data = os.urandom(0x1000)
	output = ""
	for i in range(0x1000):
		output += "{0} ".format(f'{new_data[i]:02x}')
	return output

def get_address_list(start_addr):
	global max_container_groups
	output = ""
	for i in range(0, int(0x1000/(max_container_groups*8))):
		output += "0x{0}\n".format(f'{(start_addr + (i * 8 * max_container_groups)):016x}')
	return output

def add_highlights(data_box):
	global new_data, last_data, last_change_from_scroll
	if last_change_from_scroll:
		last_data = new_data
		last_change_from_scroll = False
		return
	change_ranges = []
	last_range_start = 0
	in_range = False
	for i in range(0x1000):
		if not new_data[i] == last_data[i]:
			if not in_range:
				last_range_start = i
				in_range = True
		else:
			if in_range:
				in_range = False
				change_ranges.append([last_range_start, i])
	for r in change_ranges:
		data_box.tag_add(
				"changed_data",
				"1.{0}".format(r[0]*3),
				"1.{0}".format(r[1]*3-1))
	last_data = new_data

def update_containers(root, canvas, data_box, address_box):
	global pid, start_addr, max_container_groups
	while True:
		max_container_groups = int((canvas_width - ADDRESS_BAR_WIDTH) / (8 * 24))

		new_bytes = get_new_bytes_data(pid, start_addr)
		data_box.config(state=tkinter.NORMAL, width=(max_container_groups * 24))
		data_box.delete("1.0", tkinter.END)
		data_box.insert(tkinter.END, new_bytes)
		add_highlights(data_box)
		data_box.config(state=tkinter.DISABLED)

		addr_list = get_address_list(start_addr)
		address_box.config(state=tkinter.NORMAL)
		address_box.delete("1.0", tkinter.END)
		address_box.insert(tkinter.END, addr_list)
		address_box.config(state=tkinter.DISABLED)

		time.sleep(0.1)

def on_scroll(x, y, dx, dy):
	global start_addr, max_container_groups, last_data, new_data, last_change_from_scroll
	if dy > 0: #scrolled up
		start_addr -= (max_container_groups * 8)
	if dy < 0: #scrolled up
		start_addr += (max_container_groups * 8)
	last_change_from_scroll = True

master = tkinter.Tk()
canvas = tkinter.Canvas(master, height=canvas_height - TOP_BAR_HEIGHT, width=canvas_width)

data_box = tkinter.Text(master,
		height=canvas_height - TOP_BAR_HEIGHT,
		width=canvas_width - ADDRESS_BAR_WIDTH)
data_box.tag_configure("changed_data", background="red")

address_box = tkinter.Text(master,
		height=canvas_height - TOP_BAR_HEIGHT,
		width=18)
for i in range(0, 9):
	canvas.create_line(
			ADDRESS_BAR_WIDTH+(12*8*i),
			TOP_BAR_HEIGHT,
			ADDRESS_BAR_WIDTH+(12*8*i),
			TOP_BAR_HEIGHT-20)

container_update_thread = threading.Thread(
		target=update_containers,
		args=(master, canvas, data_box, address_box),
		daemon=True)
container_update_thread.start()

listener = Listener(on_scroll=on_scroll)
listener.start()

data_box.place(x=ADDRESS_BAR_WIDTH, y=TOP_BAR_HEIGHT)
address_box.place(x=5, y=TOP_BAR_HEIGHT)
canvas.pack()
master.mainloop()