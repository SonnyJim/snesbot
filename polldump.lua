--[[
polldump.lua
Converts lsnes movie files to SNESBot compatible files

MAKE SURE YOU USE A COMPATIBLE VERSION OF LSNES, ie one that supports on_latch

Currently only supports one controller, because, well my hardware only supports one controller ;)

Usage:
In the File menu, load ROM in lsnes
In the File menu, Load movie
Pause and rewind the movie
In the Tools menu, Load lua script
In the messages window type:
L start_dump("filename")
And press 'Execute'
In the System menu, Unpause
At the end of the movie, pause the movie again
In the messages window, type:
L end_dump ()
And press 'Execute'
Copy file to the Pi
Run with:
sudo ./snesbot -l -L -p -j -f filename.rec
Turn on SNES
]]--

print ("SNESBot Lua Latch Dumper loaded")
dumpfile = nil;

start_dump = function(filename)
	file, err = io.open(filename ..".rec", "wb");
	if not file then
		error("Can't open output file: " .. err);
	end
	dumpfile = file;
	latch_counter = 0;
	p1_input = 0; --Set both the current and old inputs to off
	p1_old = 0;
	print (string.format("Starting dum-dum-dum-dum dump to %s.rec", filename))
end

end_dump = function()
	print ("Writing output file")
	dumpfile:close();
	dumpfile = nil;
end

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
			--Clear the 'bit'
			out = 0
			--Durr, I like my bits in the wrong order
			if tframe:get_button(1, 0, 15 - i) then
				out = bit.lshift (1, i)
			else
				out = bit.lshift (0, i)
			end
			--Store the 'bit'
			p1_input = bit.bor (p1_input, out)
		end
		--Only store changes between input, not just all of it.
		if (p1_input ~= p1_old) then
			p1_old = p1_input
			--Output format is long unsigned int latch_counter, short int for button data
			dumpfile:write(string.pack("L", latch_counter))
			dumpfile:write(string.pack("H<", p1_input))
		end
		latch_counter = latch_counter + 1;
	end
end

on_paint = function()
	if dumpfile then
		gui.text(0, 0, latch_counter, 0x00FF00, 0);
	end
end

