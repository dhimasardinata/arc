---- MODULE Spsc ----
EXTENDS Naturals, Sequences

CONSTANT Capacity
VARIABLES head, tail, buf

Init ==
    /\ head = 0
    /\ tail = 0
    /\ buf = <<>>

CanPush == Len(buf) < Capacity
CanPop == Len(buf) > 0

Push(v) ==
    /\ CanPush
    /\ buf' = Append(buf, v)
    /\ tail' = (tail + 1) % Capacity
    /\ head' = head

Pop ==
    /\ CanPop
    /\ buf' = Tail(buf)
    /\ head' = (head + 1) % Capacity
    /\ tail' = tail

Next ==
    \/ \E v \in Nat : Push(v)
    \/ Pop

Spec == Init /\ [][Next]_<<head, tail, buf>>

NoOverflow == Len(buf) <= Capacity
NoUnderflow == Len(buf) >= 0
FifoOrder == TRUE

====
