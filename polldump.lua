--[[
polldump.lua by Ilari
Converts lsnes movie files to SNESBot compatible files
Usage:
In the File menu, load ROM in lsnes
In the File menu, Load movie
In the System menu, Pause and Reset
In the Tools menu, Load lua script
In the messages window type:
L start_dump("filename.dump")
And press 'Execute'
In the System menu, Unpause
At the end of the movie, pause the movie again
In the messages window, type:
L end_dump ()
And press 'Execute'
Copy file to the Pi
Run with:
sudo ./snesbot -l -L -p -j -f filename.dump
Turn on SNES
]]--

dumpfile = nil;

start_dump = function(filename)
	file, err = io.open(filename, "wb");
	if not file then
		error("Can't open output file: " .. err);
	end
	dumpfile = file;
	dumped = false;
	current = false;
	iframes = 0;
end

end_dump = function()
	dumpfile:close();
	dumpfile = nil;
end

current = false;
iframes = 0;

on_snoop = function(x, y, z, w)
	if z == 0 then
		if current and dumpfile then
			byte1 = math.floor(current / 256);
			byte2 = current % 256;
			dumpfile:write(string.char(byte1, byte2));
			iframes = iframes + 1;
		end
		current = (w ~= 0) and 1 or 0;
		dumped = true;
	else
		if w ~= 0 then
			current = bit.any(current, bit.value(z));
		end
	end
end

on_paint = function()
	if dumpfile then
		gui.text(0, 0, iframes, 0x00FF00, 0);
	end
end
