// update present state register
always_ff @( posedge clk ) begin : sync
    if (reset)
        present_state <= {reset_state};
    else
        present_state <= next_state;
end