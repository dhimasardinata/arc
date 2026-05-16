---- MODULE Roles ----
EXTENDS Naturals, FiniteSets

CONSTANT Producers
VARIABLES exposed, pushes, pops

Init ==
    /\ exposed = {"producer", "consumer"}
    /\ pushes = [p \in Producers |-> 0]
    /\ pops = 0

Push(p) ==
    /\ p \in Producers
    /\ pushes' = [pushes EXCEPT ![p] = @ + 1]
    /\ pops' = pops
    /\ exposed' = exposed

Pop ==
    /\ pops' = pops + 1
    /\ pushes' = pushes
    /\ exposed' = exposed

Next ==
    \/ \E p \in Producers : Push(p)
    \/ Pop

Spec == Init /\ [][Next]_<<exposed, pushes, pops>>

EndpointOnly == exposed \subseteq {"producer", "consumer"}
NoRootMutation == "root_push" \notin exposed /\ "root_pop" \notin exposed
NoCounterRollback == pops >= 0 /\ \A p \in Producers : pushes[p] >= 0

====
