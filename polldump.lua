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
sudo ./snesbot -p -f filename.rec
Turn on SNES
]]--

print ("SNESBot lsnes Latch Dumper loaded")
dumpfile = nil;
start_dump = function(filename)
	file, err = io.open(filename ..".rec", "wb");
	if not file then
		error("Can't open output file: " .. err);
	end
	dumpfile = file;

	file, err = io.open(filename ..".sub", "wb");
	if not file then
		error("Can't open output file: " .. err);
	end
	sub_file = file;

	latch_counter = 0;
	p1_input = 0; --Set both the current and old inputs to off
	p1_old = 0;
	sub_index = 0;
	sub_frame, sub_length = subtitle.byindex (sub_index) --Get the first subtitle frame
	print (string.format("Starting dum-dum-dum-dum dump to %s.rec", filename))
end

end_dump = function()
	print ("Writing output file")
	dumpfile:close();
	dumpfile = nil;
	sub_file:close();
	sub_file = nil;
end

--Function to dump the subtitles to a separate file
on_frame = function()
	if dumpfile then
		--Don't try and playback if we can't find a sub_frame
		if sub_frame == nil then
			return
		end

		--Find which latch the subtitle starts
		if movie.currentframe() == sub_frame then
			sub_file:write(string.pack("L", latch_counter))
		end
		
		if movie.currentframe() == sub_frame + sub_length then
			subs = subtitle.get(sub_frame, sub_length);
			subs = string.gsub (subs, "\n", "\r\n");
			sub_length = string.len(subs);

			sub_file:write(string.pack("L", latch_counter));
			sub_file:write(string.pack("L", sub_length));
			sub_file:write(subs);

			sub_index = sub_index + 1;
			sub_frame, sub_length = subtitle.byindex (sub_index) --Get the next subtitle frame
		end	
	end
end

on_latch = function()
	if dumpfile then
		--Don't try and playback after the movie has finished
		if not pcall(function() tframe = movie.get_frame(movie.find_frame(movie.currentframe())) end) then
			end_dump()
		end
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

