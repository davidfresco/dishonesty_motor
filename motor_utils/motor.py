import pyengine
import struct
import c_lib
import time
import subprocess

from datetime import datetime

"""
instantiates and returns a knower attached to the specified process id
a knower is basically a class that has support for organization of
memory addresses and multi-level pointers so you can easily read and
write to them. it's part of the pyengine library which I may not provide
because it isn't very well written or necessary for that matter.
"""
def get_knower(pid):
	knower = pyengine.Knower(
			"", quiet=True)
	knower.pid = pid
	knower.baseAddr = struct.unpack("q", pyengine.get_base_addr(pid))[0]
	return knower

"""
takes a dict that describes a module in the process environment block (PEB)
and turns it into a format similar to how I keep track of virtual address
descriptors (VADs) in case you want to manually add a static module's memory
space to a search
"""
def module_to_vad(module):
	return [int(module["DllBase"] / 0x1000),
			int(module["DllBase"] / 0x1000) + int(module["SizeOfImage"] / 0x1000)+1,
			"1",
			"111",
			int(module["SizeOfImage"] / 0x1000)+1,
			"0" * 64,
			module["Address"]]

"""
an internal function used to sort vads by starting address right before a search
"""
def _vadSort(vad):
	return vad[0]

"""
internal function to asynchronously get user input to narrow down a long list
of memory search results
"""
def _take_input(receiver):
	receiver.user_input = input("input:")

"""
this is the main object you use in the whole thang
youd say motor = Motor("Tetris.exe") to create a motor object attached to the
tetris process
"""
class Motor:

	def __init__(self, app_name, table=None):
		self.app_name = app_name
		self.table = table
		self.app_pid = pyengine.get_application_pid(app_name)
		print("motor pid:", hex(self.app_pid))


	"""
	since the results array can easily take up many gigabytes of data, it is
	stored internally in the c search library unless explicitly requested because
	it takes an obscene amount of memory if exported to the python runtime
	
	returns a list of two-element lists for each result in the current search
	results are in the format [address, value]
	"""
	def get_results(self):
		struct_code, target_length = pyengine.struct_types[self.var_type]
		raw_results = c_lib.get_results(target_length)
		results = [[rres[0], struct.unpack(struct_code, rres[1])[0]] for rres in raw_results]
		return results 


	"""
	this is supposed to tell the total number of bytes in a list of VADs but it
	ended up not really being useful so it might not even work properly
	"""
	def size_of_vads(self):
		num_bytes = 0
		for vad in self.vads:
			if vad[4] == 0:
				continue
			num_bytes += (vad[1] - vad[0])*0x1000
		return num_bytes


	"""
	sets self.vads to be a list of VADs that describe only dynamnically allocated
	(heap) memory. after calling update_vads you can manually add in static memory
	regions to self.vads depending on what you are searching for
	also it sorts the vads at the end for search optimization. if you are checking
	a new search to see which memory addresses are still valid, if you have n results
	and m vads youd need to do n*m comparisons vs n*log(m) which is much better as
	the number of vads in a process gets very large
	"""
	def update_vads(self):
		pure_vads = pyengine.get_process_memory_regions(self.app_pid)
		self.vads = []
		for vad in pure_vads:
			if (vad[2] == "1" and vad[3] == "100"):# or vad[3] == "111":
				self.vads.append(vad)
		print("scanning {0} bytes".format(f'{self.size_of_vads():,}'))
		self.vads.sort(key=_vadSort)


	"""
	rescans the results of an initial search to narrow it down

	method can be:
		"Exact Value"
		"Less Than Value"
		"Greater Than Value"
		"Value Between"
		"Increased Value"
		"Decreased Value"
		"Changed Value"
		"Unchanged Value"

	target is the value you are searching for if its an exact value or a greater than
		or less than search
	low and high are for between value searches

	for inc/dec/changed/unchanged searches you dont need to specify additional args
	"""
	def next_search(self, method, target=None, low=None, high=None):
		struct_code, target_length = pyengine.struct_types[self.var_type]
		self.start_time = datetime.now()
		print("starting at:", self.start_time)
		num_results = c_lib.next_search(
				self.app_pid,
				method,
				self.var_type,
				target_length,
				struct.pack(struct_code, target) if target else b'',
				struct.pack(struct_code, low) if low else b'',
				struct.pack(struct_code, high) if high else b'',
				self.vads)
		self.end_time = datetime.now()
		print("time elapsed:", (self.end_time - self.start_time).total_seconds())
		print("found", num_results, "matches")


	"""
	starts a new search for a memory location

	i think right now methods are limited to:
		"Exact Value"
		"Value Between"
		"Unchanged Value"

	var_type is:
		"INT" (signed)
		"FLOAT"
		"U_LONG" (64 bit, unsigned, for pointers basically)

	target is the value you are searching for if its an exact value or a greater than
		or less than search
	low and high are for between value searches
	
	for inc/dec/changed/unchanged searches you dont need to specify additional args

	vads is for if you want to specify a special list of vads. if you leave it as None
	it will default to whatever self.update_vads() does.
	"""
	def new_search(self, method, var_type, target=None, low=None, high=None, vads=None):
		self.var_type = var_type
		if vads:
			self.vads = vads
		else:
			self.update_vads()
		self.start_time = datetime.now()
		print("starting at:", self.start_time)
		struct_code, target_length = pyengine.struct_types[self.var_type]
		num_results = c_lib.read_all_mem(
				self.app_pid,
				self.vads,
				method,
				self.var_type,
				target_length,
				struct.pack(struct_code, target) if target else b'',
				struct.pack(struct_code, low) if low else b'',
				struct.pack(struct_code, high) if high else b'')
		self.end_time = datetime.now()
		print("time elapsed:", (self.end_time - self.start_time).total_seconds())
		print("found", num_results, "matches")


	"""
	in case you ONLY want to search the data stored in a list of particular modules, this
	function will perform a new search on a list of dll modules by name

	module_list is a list of module names to search

	i cant remember why i added this feature cuz youd need to reinitialize the vad list
	with these same modules for each subsequent search but whatever its in here now
	"""
	def search_modules(module_list, method, var_type, target=None, low=None, high=None):
		module_vads = []
		all_modules = pyengine.enumerate_modules(self.app_pid)
		for mod_name in all_modules.keys():
			if mod_name in module_list:
				mod_vad.append(module_to_vad(all_modules[mod_name]))

		new_search(method, var_type, target, low, high, module_vads)


	"""
	this fun guy takes a dictionary that represents an n-ary tree of pointer paths to
	a certain address and converts it to a list where each element is a list that
	describes a multi-level pointer. all paths converge to the same address.

	the root node of the tree is at address 0 i think and its children are all the 
	static memory addresses that point to valid mem. each child of a node means you
	follow the pointer at that node then add the value of the child as an offset and
	that gets you to the next node

	the tree structure isnt that important though because the whole point of this function
	is to product a list of pointer paths like cheat engine's pointer scan so you dont
	have to deal with the stupid tree anyway
	"""
	def ptr_tree_to_paths(self, tree):
		paths = []
		if not tree:
			return paths
		if 0 in tree.keys() and tree[0]:
			for key in tree[0].keys():
				next_paths = self.ptr_tree_to_paths(tree[0][key])
				for path in next_paths:
					paths.append([key] + path)
		else:
			for key in tree.keys():
				if tree[key]: #not a leaf node
					next_paths = self.ptr_tree_to_paths(tree[key])
					for path in next_paths:
						paths.append([key] + path)
				else: #leaf node
					paths.append([key])
		return paths


	"""
	performs a pointer scan for a certain address and optionally saves it to a file

	target is the address you are scanning for
	file_name is obvious
	max_depth is how many levels the multi-level pointers can be
	max_struct_size is the max offset we think we'll need
	"""
	def ptr_scan(self, target, file_name=None, max_depth=2, max_struct_size=4096):
		self.var_type = "U_LONG"
		self.update_vads()
		mods = pyengine.enumerate_modules(self.app_pid)
		self.vads.append(module_to_vad(mods[list(mods.keys())[0]]))
		print("static vad:\n", list(mods.keys())[0], mods[list(mods.keys())[0]])
		self.start_time = datetime.now()
		print("starting at:", self.start_time)
		ptr_dict = c_lib.ptr_scan_static(
				self.app_pid,
				self.vads,
				struct.pack("Q", target),
				max_depth,
				max_struct_size)
		paths = self.ptr_tree_to_paths(ptr_dict)
		self.ptr_list = paths
		self.end_time = datetime.now()
		print("time elapsed:", (self.end_time - self.start_time).total_seconds())
		if file_name:
			file = open("{0}.csv".format(file_name), "a+")
			for path in self.ptr_list:
				line = ""
				for offset in path:
					line += "{0}, ".format(offset)
				file.write("{0}\n".format(line))
			file.close()
		print("found", len(paths), "pointers")


	"""
	takes previous pointer scan data and re-checks it to narrow down the list of
	valid pointers after restarting the target program and re-locating the targed
	memory address that corresponds to the value you want

	in_file is the initial data you are re-scanning
	out_file is the new results file
	target is the new memory address the pointer paths should lead to
	"""
	def next_ptr_scan(self, in_file, out_file, target):
		paths = []
		knower = get_knower(self.app_pid)
		file = open("{0}.csv".format(in_file), "r")
		results = 0
		total_ptrs = 0
		for line in file.readlines():
			total_ptrs += 1
			path = [int(offset.split(",")[0]) for offset in line.split()]
			value = knower.readPtr(path, var_type)
			if value == target:
				paths.append(path)
				results += 1
		print("found", results, "out of", total_ptrs, "pointers")
		self.ptr_list = paths
		if out_file:
			file = open("{0}.csv".format(out_file), "a+")
			for path in self.ptr_list:
				line = ""
				for offset in path:
					line += "{0}, ".format(offset)
				file.write("{0}\n".format(line))
			file.close()


	"""
	function that just prevents a list of memory addresses from changing by
	setting them back to their initial value 50 times per second
	"""
	def freeze_values(self, results):
		knower = get_knower(self.app_pid)
		self.freezing_values = True
		while self.freezing_values:
			for res in results:
				knower.writeMem(res[0], res[1], self.var_type)
			time.sleep(0.02)


	"""
	if you have a long list of possible results to a search and you dont know which
	is the one you want, call motor.bin_search_results(motor.get_results()) to fix that.

	this will freeze half the values supplied in the results argument. you then go to
	the program and attempt to change the value. if it changes, you type "n" in the command
	prompt and if the value is frozen and does not change, you type a "y". eventually 
	there will only be only value left in the list, and it will print that address out.
	"""
	def bin_search_results(self, results):
		if len(results) == 0:
			print("didnt find it")
			return None
		mid = int(len(results) / 2)
		freeze_thread = Thread(target=self.freeze_values, args=(results[:mid], ), daemon=True)
		query_thread = Thread(target=_take_input, args=(self, ), daemon=True)
		freeze_thread.start()
		query_thread.start()
		query_thread.join()
		self.freezing_values = False
		freeze_thread.join()

		if "y" in self.user_input:
			if len(results) == 1:
				print("found at", hex(results[0][0]))
				return results[0]
			else:
				return self.bin_search_results(results[:mid])
		else:
			if len(results) == 1:
				print("found at", hex(results[0][0]))
				return results[0]
			else:
				return self.bin_search_results(results[mid:])


	"""
	separate function to call from the "visualize" function to run it asynchronously
	so you can keep using the actual program at the same time as running the
	visualizer
	"""
	def _visualizer_thread(self, address):
		cmd = "python visualizer\\visualizer.py {0} {1}".format(str(self.app_pid), str(address))
		proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)


	"""
	a really shoddy copy of the "browse this memory region" feature of cheat engine
	made in tkinter. its really slow and ugly but meh gui programming is not for me
	"""
	def visualize(self, address):
		thread = Thread(target = self._visualizer_thread, args=(address, ), daemon=True)
		thread.start()