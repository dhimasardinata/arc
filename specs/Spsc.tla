---- MODULE Spsc ----
EXTENDS Naturals, Sequences

CONSTANT Capacity
VARIABLES head, tail, buf
\* C++ link: components/arc/include/arc/spsc.hpp advances head_ on push
\* and tail_ on pop; keep this model's names aligned with the source.

Init ==
    /\ head = 0
    /\ tail = 0
    /\ buf = <<>>

CanPush == Len(buf) < Capacity
CanPop == Len(buf) > 0

Push(v) ==
    /\ CanPush
    /\ buf' = Append(buf, v)
    /\ head' = (head + 1) % Capacity
    /\ tail' = tail

Pop ==
    /\ CanPop
    /\ buf' = Tail(buf)
    /\ tail' = (tail + 1) % Capacity
    /\ head' = head

Next ==
    \/ \E v \in Nat : Push(v)
    \/ Pop

Spec == Init /\ [][Next]_<<head, tail, buf>>

NoOverflow == Len(buf) <= Capacity
NoUnderflow == Len(buf) >= 0
FifoOrder == TRUE

====
