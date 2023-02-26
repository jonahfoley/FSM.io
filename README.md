# FSM.io

FSM.io is an application which can be used to transform a state-machine made in Draw.io into an equivalent SystemVerilog application. State machines see frequent use in the field of FPGA and ASIC design to build controllers, however can lead to bugs when written by hand due to the complex combinatorial logic used to deduce the next state. This can lead to hard-to-debug problems upon simulation of the design as part of a larger system. By using this graphical approach it becomes easier to see when there is an error in the state machine and makes debugging easier to achieve. Furtheremore, a graphical representation of the state machine is auto documenting of the hardware, enabling other designers in your team to instantly understand the relationship between states in the controller. Finally, since draw.io files are represented using XML, this particular approach lends itself to simple version control, which is often a problem when working with vendor specific tools.

## How To Use

Upon building the application as specified in section:Installation, add the executable found in FSM.io/build/bin to the path. From there, the program can be invoked as:

```
> FSM.io --diagram=<path to draw.io diagram> --outfile=<path to output file>
```

There are 2 command line arguments, specified as follows:

| Argument  | Specifier   | Required?  | Function                                            |
| --------- | ----------- |----------- | --------------------------------------------------- |
| --diagram | -d          | Yes        | specified the Draw.io diagram you wish to convert   |
| --outfile | -o          | No         | Specified the output file you wish to write the result to. If not specified, the output will be printed to the console |

FSM.io puts some constraints on the way diagrams should be made so that when the program is run it can deduce information about the state machine. Such attributes are things like the states, decision blocks, default state, state outputs and state names.

### States

Each state in your state machine should be of the default rectangle shape in draw.io

![alt text](https://github.com/jonah766/FSM.io/blob/main/resources/fsm/rectangle.png?raw=true)

This will represent one case in the resulting case statement. By default states are assigned a defaul name of the form s\<number\>,
with no outputs. Within each state you may provide the following specifiers:

| Identifier                         | function                       | example                    |
| ---------------------------------- | ------------------------------ |--------------------------- |
| $STATE=\<state name\>              | specify state name             | $STATE=IDLE                |
| $OUTPUTS={\<comma separate list\>} | specify state outputs          | $OUTPUTS={START_P,START_D} |
| {\<comma separate list\>}          | specify state outpust (simple) | {READY}                    |
| $DEFAULT                           | spcify state as default state  | $DEFAULT                   |

Any of these identifiers may be chained together in a ';' separated list. For example, a state may look like any of the following:

![alt text](https://github.com/jonah766/FSM.io/blob/main/resources/fsm/state_examples.png?raw=true)

### Decision Blocks

A decision block is used to infer an if-statement inside the FSM case statement. Each decision block should be instanced using the
default rhombus shape in Draw.io

![alt text](https://github.com/jonah766/FSM.io/blob/main/resources/fsm/rhombus.png?raw=true)

Each decision block can have as many inputs as desired (from other states or other decision blocks), but has only two outputs: one path indicating the 'success' path and one path indicating the 'failure' path. The success path is taken when the predicate inside the decision block is satisfied. e.g.

![alt text](https://github.com/jonah766/FSM.io/blob/main/resources/fsm/decision_examples.png?raw=true)

As shown above the true path *must* be specified by writing one of {1,True,true,yes,Yes} on the arrow of the true path, and the false path *must* be specified by writing either {0,False,false,no,No} on the false path.

Inside the state you must specify the predicate for the decision block, this can take one of two forms:
 
| Identifier                            | function                                                                   | example |
| --------------------------------------| -------------------------------------------------------------------------- | ------- |
| \<variable\> \<comparator\> \<value\> | Takes the true path if the expression is true                              | X < 1   |
| \<variable\>                          | evaluates the \<variable\> as a boolean expression (i.e. true if non-zero) | BEGIN   |

The \<comparator\> can be one of {==, !=, <=, <, >=, >}.

Furthermore, you can freely connect together any number of decision blocks in order to create nest if-statements, e.g. 

![alt text](https://github.com/jonah766/FSM.io/blob/main/resources/fsm/nested_rhombus.png?raw=true)

This would map to an if-else statement of the form

```
if(A==2'b01) begin
    if(B) begin
        next_state = ...
    end else begin
        next_state = ...
    end
end else begin
    if(Count==10) begin
        next_state = ...
    end else begin
        next_state = ...
    end
end
```

### Arrows

All arrows used in your FSM diagram should be of the default arrow type provided by Draw.io. In order to ensure a connection between two elements of the state machine arrows must NOT be floating. Its source connection and its target connection should both both be anchored to their respective elements within the diagram. This is pictured below.

![alt text](https://github.com/jonah766/FSM.io/blob/main/resources/fsm/state_connection.png?raw=true)

Each anchor point on a shape can be indicated by one of the blue crosses on the edge of the shape. Successful connection of an arrow with an anchor point is indicated by the green focus. Two arrows may be connection to/from the same anchor point. If additional anchor points are required they can be added to the shape from within Draw.io. 

## Installation

Use the package manager [pip](https://pip.pypa.io/en/stable/) to install foobar.

## Example Diagrams

A range of example diagrams can be seen in the /resources folder. In the below example we see a larger state machine. In addition to the states and decision blocks the text highlighted in red is used to describe the function of the state/signal. These bits of text will NOT be included in the resulting code, and are purely for documentation (they need not be red either!).

![alt text](https://github.com/jonah766/FSM.io/blob/main/resources/fsm/ARP_Cache_FSM.png?raw=true)

The resulting code which is produced from this state machine is as follows

```
module fsm (
  input logic clk, reset,
  input logic CHECK_IP, TIMEOUT, TIMEOUT, IP_MATCH, IP_COLLISION, REQUEST_TIMEOUT, RESPONSE_RECEIVED,
  output logic CLEAR_EXPIRED, REQUEST_SUCCESS, MAC_REQUEST_ERROR, ARP_READY, REQUEST_SUCCESS, BEGIN_TIMER, WRITE_PAIR, COMPARE_IP, REQUEST_MAC
);

typedef enum {
  CLEANUP_CACHE, FOUND_IP, HANDLE_REQUEST_TIMEOUT, IDLE, HANDLE_MAC_RESPONSE, READ_RAM_IP, MISSING_IP
} state_t;

state_t present_state, next_state;

always_ff @( posedge clk ) begin : sync
  if (reset)
    present_state <= IDLE;
  else
    present_state <= next_state;
end

always_comb begin : comb
  CLEAR_EXPIRED = '0;
  REQUEST_SUCCESS = '0;
  MAC_REQUEST_ERROR = '0;
  ARP_READY = '0;
  REQUEST_SUCCESS = '0;
  BEGIN_TIMER = '0;
  WRITE_PAIR = '0;
  COMPARE_IP = '0;
  REQUEST_MAC = '0;
  next_state = present_state;
  case (present_state)
    CLEANUP_CACHE : begin
      CLEAR_EXPIRED = '1;
      next_state = IDLE;
    end
    FOUND_IP : begin
      REQUEST_SUCCESS = '1;
      next_state = IDLE;
    end
    HANDLE_REQUEST_TIMEOUT : begin
      MAC_REQUEST_ERROR = '1;
      next_state = IDLE;
    end
    IDLE : begin
      ARP_READY = '1;
      if(CHECK_IP) begin
        next_state = READ_RAM_IP;
      end else begin
        if(TIMEOUT) begin
          next_state = CLEANUP_CACHE;
        end else begin
          next_state = IDLE;
        end
      end
    end
    HANDLE_MAC_RESPONSE : begin
      REQUEST_SUCCESS = '1;
      BEGIN_TIMER = '1;
      WRITE_PAIR = '1;
      if(TIMEOUT) begin
        next_state = CLEANUP_CACHE;
      end else begin
        next_state = IDLE;
      end
    end
    READ_RAM_IP : begin
      COMPARE_IP = '1;
      if(IP_MATCH) begin
        if(IP_COLLISION) begin
          next_state = MISSING_IP;
        end else begin
          next_state = FOUND_IP;
        end
      end else begin
        next_state = MISSING_IP;
      end
    end
    MISSING_IP : begin
      REQUEST_MAC = '1;
      if(REQUEST_TIMEOUT) begin
        next_state = HANDLE_REQUEST_TIMEOUT;
      end else begin
        if(RESPONSE_RECEIVED) begin
          next_state = HANDLE_MAC_RESPONSE;
        end else begin
          next_state = MISSING_IP;
        end
      end
    end
    default : begin
      next_state = IDLE;
    end
  endcase
end

endmodule
```

## License

[MIT](https://choosealicense.com/licenses/mit/)