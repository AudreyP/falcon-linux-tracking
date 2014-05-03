module pwm_main(
	input wire clk,
	input wire RxD,
//	output wire TxD,
	output wire motor_one_pwm,
	output wire motor_two_pwm
	);
	
	parameter InternalClkFrequency = 100000000;
	//instantiate serial comms modules
	wire RxD_data_ready;
	wire [7:0] RxD_data;
	
	reg [31:0] timeout;
	
	async_receiver #(.ClkFrequency(InternalClkFrequency)) 
		asyncRX(
		.clk(clk),
		.RxD(RxD),
		.RxD_data_ready(RxD_data_ready),
		.RxD_data(RxD_data),
		.RxD_endofpacket(),
		.RxD_idle()
		);
	
//	wire TxD_data_ready;
//	reg [7:0] TxD_data;
	
	/*async_transmit #(.ClkFrequency(InternalClkFrequency)) 
		asyncTX(
		.clk(clk),
		.TxD_start(TxD_start),
		.TxD_data(TxD_data),
		.TxD(TxD),
		.TxD_busy(),
		.state()
		);*/
	
	reg [7:0] pwm_input_one = 127;
	reg [7:0] pwm_input_two = 127;
	reg alt; //purposefully only 1 bit
	parameter SYNC = 8'hff;
	reg [3:0] state;
	
	
	
	// upon valid receipt of serial data and sync signal, alternate input to motors.
	always @(posedge clk) begin
		//kill motors if new data not received after half a second
		if (timeout > 50000000) begin 
			pwm_input_one = 127;
			pwm_input_two = 127;
		end
	
		if (RxD_data_ready) begin	
			case (state)
				0: begin
					// IDLE until sync byte received
					if (RxD_data == SYNC) begin
						state = 1;
					end else begin
						state = 0;
					end
				end
				1: begin
					timeout = 0;
					// receive serial data for right motor (sent from Pi first)
					pwm_input_one = RxD_data;
					state = 2;
				end
				2: begin
					// receive serial data for left motor (sent from Pi second)
					pwm_input_two = RxD_data;
					state = 0;
				end
			endcase
		end else begin
			timeout = timeout + 1;
		end
	end	
	
	// instantiate pwm modules for motor control
	pwm motor_one(
		.clk(clk),
		.pwm_input(pwm_input_one),
		.pwm_output(motor_one_pwm)
		);
	
	pwm motor_two(
		.clk(clk),
		.pwm_input(pwm_input_two),
		.pwm_output(motor_two_pwm)
		);
		
endmodule
