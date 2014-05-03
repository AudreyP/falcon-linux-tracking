module pwm (
	input wire clk,
	input [7:0] pwm_input,
	output reg pwm_output
	);

	reg [2:0] state = 0;
	reg [9:0] two_ms_counter = 0;
	reg fifty_clk = 0;

	// set pwm frequency to ~20khz (period = 50us)
	// assuming 100MHz input clk, counterer will increment 500 times per clock cycle
	reg [18:0] counter = 0;
	reg [18:0] on_time = 0;

	always @(negedge clk) begin
		if (counter <= 200000)
			counter = counter + 1; //counter increments every 10 ns until 2 milliseconds reached
		else begin
			counter = 0;
			two_ms_counter = two_ms_counter + 1;
			if (two_ms_counter == 10) begin
				state = 0;
				two_ms_counter = 0;
			end
			
		end
		
		on_time = ((100000*pwm_input) / 254) + 100000;
			
		// state machine
		case (state) 
			0: begin
				//set default 1ms pulse; add to it (if necessary) in next state
				if (counter <= 100000 && two_ms_counter == 0) begin
					pwm_output = 1;
					state = 0;
				end else begin
					state = 1;
				end
			end
			1: begin
				if (counter < on_time && two_ms_counter == 0) begin
					pwm_output = 1;
					state = 1;
				end else begin
					pwm_output = 0;
				end
			end
		endcase
	end
	
endmodule