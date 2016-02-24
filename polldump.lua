--local bit = require("bit")
--[[
polldump.lua

MAKE SURE YOU USE A COMPATIBLE VERSION OF LSNES, ie one that supports on_latch

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

print ("SNESBot Lua Latch Dumper loaded")
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
	p1_input = 0;
	p1_old = 0;
	print ("Starting dum-dum-dum-dum dump" .. filename)
end

end_dump = function()
	print ("Writing output file:" .. filename)
	dumpfile:close();
	dumpfile = nil;
end

current = false;
iframes = 0;

function toBits(num, bits)
    -- returns a table of bits
    local t={} -- will contain the bits
    for b=bits,1,-1 do
        rest=math.fmod(num,2)
        t[b]=rest
        num=(num-rest)/2
    end
    if num==0 then return t else return {'Not enough bits to represent this number'}end
end

on_latch = function()
	if dumpfile then
		tframe = movie.get_frame(movie.find_frame(movie.currentframe()));
		--We start off with all buttons off
		p1_input = 0x0000
		for i = 0, 15, 1 do
			--Get the bit
			out = 0
			--Durr, I like my bits in the wrong order
			if tframe:get_button(1, 0, 15 - i) then
				out = bit.lshift (1, i)
			else
				out = bit.lshift (0, i)
			end
			p1_input = bit.bor (p1_input, out)
		end
		if (p1_input ~= p1_old) then
			print (iframes)
			print (p1_input)
			bits=toBits(p1_input, 16)
			print (table.concat(bits))
			p1_old = p1_input
			dumpfile:write(string.pack("L", iframes))
			dumpfile:write(string.pack("H<", p1_input))
		end
		iframes = iframes + 1;
	end
end

on_paint = function()
	if dumpfile then
		gui.text(0, 0, iframes, 0x00FF00, 0);
	end
end

