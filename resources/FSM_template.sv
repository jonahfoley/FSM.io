module FSM (
    input wire clk,
reset
);

timeunit 1ns;
timeprecision 10ps;

// state declaration
{}

// state register

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
        default : begin // if the design gets into a non-onehot state, just turn it off.
            next_state = OFF;
        end
    endcase
end

endmodule
