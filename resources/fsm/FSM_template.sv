module FSM (
    input logic clk, reset,
    // inputs
    {inputs},
    // outputs
    {outputs}
);

timeunit 1ns;
timeprecision 10ps;

// state declaration
{state_decl}

// state register
{state_reg}

// next state logic
{next_state}

endmodule
