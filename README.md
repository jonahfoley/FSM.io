# FSM.io

Foobar is a Python library for dealing with word pluralization.

## How To Use

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
default rhombus shape in draw.io

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

## Installation

Use the package manager [pip](https://pip.pypa.io/en/stable/) to install foobar.

## License

[MIT](https://choosealicense.com/licenses/mit/)