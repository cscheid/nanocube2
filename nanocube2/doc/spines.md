# Spines

A "spine" of an address is the sequence of nodes down the nanocube
DAG.

(FIXME show a simple example here)

## Sparse spines

Sometimes we are only interested in the branching events along the
path of a spine. A "sparse" spine is a subset of a spine containing
only nodes of refinement degree 2.

## Branching spines

If a spine is the set of all nodes reachable by following refinement
edges of the address, the branching spine is the set of all nodes
reachable by following either refinement or next-dimension edges.

### Implementation

A naive implementation of the branching spine algorithm would
traverse the nanocube data structure naively and adding the nodes to a
result array in a BFS manner. Unfortunately, because the nanocube DAG
is in fact a DAG (and not a tree), such a data structure would include
repeated subgraphs. In high-dimensional nanocubes this can be
prohibitively expensive.

As a result (and to be fair, this is ugly), the current implementation
of nanocubes includes, for each node, a "scratch" field that can be
used by traversals to mark already-visited nodes. This decision will
undoubtedbly introduce further complications if we decide to move to a
multi-threaded implementation.






