--[[
Script to dump input from snes9x movie files
--]]

file, err = io.open("s9x_input.dump", "wb");
if not file then
	error("Can't open output file: " .. err);
end

current_input = 0;
local input_table={}
current_frame = 0;
running = 1;

emu.registerafter (function (dump_input)
	current_frame = current_frame + 1
	if not emu.lagged() and running == 1 then
		input_table = joypad.get();
		R = input_table.R and 1 or 0 ;
		L = input_table.L and 1 or 0 ;
		X = input_table.X and 1 or 0 ;
		A = input_table.A and 1 or 0 ;
		right = input_table.right and 1 or 0 ;
		left = input_table.left and 1 or 0 ;
		down = input_table.down and 1 or 0 ;
		up = input_table.up and 1 or 0 ;
		start = input_table.start and 1 or 0 ;
		select = input_table.select and 1 or 0 ;
		Y = input_table.Y and 1 or 0 ;
		B = input_table.B and 1 or 0 ;
		--Store them in a bitfield	
		current_input = 
		bit.lshift (B, 0) +
		bit.lshift (Y, 1) +
		bit.lshift (select, 2) +
		bit.lshift (start, 3) +
		bit.lshift (up, 4) +
		bit.lshift (down, 5)  +
		bit.lshift (left, 6) +
		bit.lshift (right, 7) +
		bit.lshift (A, 8) +
		bit.lshift (X, 9) +
		bit.lshift (L, 10) +
		bit.lshift (R, 11);

		print (current_input);
		byte1 = math.floor(current_input / 256);
		byte2 = current_input % 256;
		file:write(string.char(byte1, byte2));
	end
	if current_frame == movie.length() then 
		file:close();
		running = 0;
		print("Finished");
	end


end)
