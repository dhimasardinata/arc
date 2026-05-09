---- MODULE Consensus ----
EXTENDS Naturals, FiniteSets

CONSTANT Nodes
VARIABLES term, voted, committed

Init ==
    /\ term = 0
    /\ voted = {}
    /\ committed = FALSE

Vote(n) ==
    /\ n \in Nodes
    /\ n \notin voted
    /\ voted' = voted \cup {n}
    /\ term' = term
    /\ committed' = committed

Commit ==
    /\ Cardinality(voted) * 2 > Cardinality(Nodes)
    /\ committed' = TRUE
    /\ voted' = voted
    /\ term' = term

Next ==
    \/ \E n \in Nodes : Vote(n)
    \/ Commit

Spec == Init /\ [][Next]_<<term, voted, committed>>

NoSplitBrain == committed => Cardinality(voted) * 2 > Cardinality(Nodes)
MonotonicTerm == term >= 0

====
