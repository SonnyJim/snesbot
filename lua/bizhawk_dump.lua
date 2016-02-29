print ("SNESBot Bizhawk Latch Dumper loaded")
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
	print (string.format("Starting dum-dum-dum-dum dump to %s.rec", filename))
end

end_dump = function()
	print ("Writing output file")
	dumpfile:close();
	dumpfile = nil;
	sub_file:close();
	sub_file = nil;
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

store_bit = function (shift)
	out = 0;
	out = bit.lshift (1, shift);
	p1_input = bit.bor (p1_input, out);
end
on_latch = function()
	if dumpfile then
		p1_input = 0x0000;

		frame = emu.framecount ()
		if movie.getinput (frame)["P1 B"] then
			store_bit (15)
		end
		if movie.getinput (frame)["P1 Y"] then
			store_bit (14)
		end
		if movie.getinput (frame)["P1 Select"] then
			store_bit (13)
		end
		if movie.getinput (frame)["P1 Start"] then
			store_bit (12)
		end
		if movie.getinput (frame)["P1 Up"] then
			store_bit (11)
		end
		if movie.getinput (frame)["P1 Down"] then
			store_bit (10)
		end
		if movie.getinput (frame)["P1 Left"] then
			store_bit (9)
		end
		if movie.getinput (frame)["P1 Right"] then
			store_bit (8)
		end
		if movie.getinput (frame)["P1 A"] then
			store_bit (7)
		end
		if movie.getinput (frame)["P1 X"] then
			store_bit (6)
		end
		if movie.getinput (frame)["P1 L"] then
			store_bit (5)
		end
		if movie.getinput (frame)["P1 R"] then
			store_bit (4)
		end

		--Only store changes between input, not just all of it.
		if (p1_input ~= p1_old) then
			p1_old = p1_input
			--Output format is long unsigned int latch_counter, short int for button data
			byte1 = math.floor(latch_counter / 256);
			byte2 = latch_counter % 256;
			dumpfile:write(string.char(byte1, byte2));
			byte1 = math.floor(p1_input / 2^24)
			byte2 = math.floor((p1_input % 2^24) / 2^16)
  			byte3 = math.floor((p1_input % 2^16) / 2^8)
			byte4 = (p1_input % 2^8)
			dumpfile:write(string.char(byte1, byte2, byte3, byte4));
		end
		latch_counter = latch_counter + 1;
	end
end

event.oninputpoll(on_latch);
