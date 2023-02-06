module char_processor_fsm (
    input wire clk,
    input wire reset,
    input wire [7:0] chars_in [0:1],
    input wire [0:1] chars_in_valid,
    output logic off, pass, lower, upper 
);

timeunit 1ns;
timeprecision 10ps;

logic got_two_valid, got_off, got_pass, got_upper, got_lower, mode_changed;
logic [3:0] got_esc_sequence;

// a one hot encoding allows us to update state based on detected esc sequence, and yields greater f_max
typedef enum logic[3:0] {
    OFF   = 4'b0001, 
    PASS  = 4'b0010, 
    UPPER = 4'b0100, 
    LOWER = 4'b1000
} state_t; 

state_t present_state, next_state;

// update present state register
always_ff @( posedge clk ) begin : sync
    if (reset)
        present_state <= OFF;
    else
        present_state <= next_state;
end

// two valid input chars in a row
assign got_two_valid = &chars_in_valid;

// to check whether we detected each of the escape sequences on the input
assign got_off    = got_two_valid ? (chars_in[1] == "#" & chars_in[0] == "O") : 0;
assign got_pass  = got_two_valid ? (chars_in[1] == "#" & chars_in[0] == "P") : 0;
assign got_upper = got_two_valid ? (chars_in[1] == "#" & chars_in[0] == "U") : 0;
assign got_lower = got_two_valid ? (chars_in[1] == "#" & chars_in[0] == "L") : 0;
assign got_esc_sequence = {got_lower, got_upper, got_pass, got_off};

// if any of the escape sequence flags are one, there is a mode change
assign mode_changed = |got_esc_sequence;

// next state logic
always_comb begin : comb
    off   = '0;
    pass  = '0;
    upper = '0;
    lower = '0;
    next_state = present_state;
    case (present_state)
        OFF : begin
            off = '1;
            if (mode_changed)
            begin
                next_state = state_t'(got_esc_sequence);
            end
        end
        PASS : begin
            pass = '1;
            if (mode_changed)
            begin
                next_state = state_t'(got_esc_sequence);
            end
        end
        UPPER : begin
            upper = '1;
            if (mode_changed)
            begin
                next_state = state_t'(got_esc_sequence);
            end
        end
        LOWER : begin
            lower = '1;
            if (mode_changed)
            begin
                next_state = state_t'(got_esc_sequence);
            end
        end
        default : begin // if the design gets into a non-onehot state, just turn it off.
            next_state = OFF;
        end
    endcase
end

endmodule
