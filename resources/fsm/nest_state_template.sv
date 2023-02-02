always_comb begin : comb
    {outputs_defaults}
    next_state = present_state;
    case (present_state)
        {cases}
        default : begin // if the design gets into a non-onehot state, just turn it off.
            next_state = {default_state};
        end
    endcase
end