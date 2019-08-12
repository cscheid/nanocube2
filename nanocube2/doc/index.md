# Notes on nanocube2

This is a new implementation of the nanocubes data structure. This
implementation tries to be easier to follow and maintain, while
hopefully not losing too much in efficiency (eg. compared to the
[original C++ implementation](https://github.com/laurolins/nanocube))

## The basics

A nanocube is a DAG data structure where each node has (up to) three
separate edges. Two of these edges are "refinement" edges, and the
other edge is the "next-dimension" edge. (Whenever it's clear by context,
we'll call it the "next" edge). A nanocube has a single, distinguished
"root" node, from which all other nodes are reachable.

We think of the nodes in this graph as being partitioned under a
"dimension" variable, where nodes in the 0-th dimension do not have a
next edge, and next edges of nodes in the (i)-th dimension point to
nodes in the (i-1)-th dimension. In addition, refinement edges always
point to nodes in the same dimension. Finally, nodes in the 0th
dimension have a value associated with them, which will represent the
aggregation being tracked. Because this is an aggregation, this value
must be in a commutative monoid.

We associate each refinement edge with either a 0 or a 1. By
concatenating these digits for each of the dimensions, we get a
tuple of binary words associated with each node. Now take each of
these words to mean a prefix of a set of possible binary words.  This
means each node in a nanocube is associated with a tuple of
prefixes, which we think of as *ranges* for each dimension.

With that sense in mind, we can derive the first law that nanocube
data structures must hold. Let v be the aggregation value associated
with a range (w1, w2, ... wn). Remember that w1 through wn are binary
words representing prefixes. We will denote this relation by the
"value" function:
  
  value (w1, w2, ... wn) = v
  
Now denote path concatenation using "+": if w1 = "000", then w1 +
"1" = "0001". Any valid nanocube must be such that

  value (w1 + "0", w2, ... wn) <> value (w1 + "1", w2, ... wn) == value (w1, w2, ... wn)

Here, we're using "<>" to mean the monoid aggregation operation. The
analogous law must hold for splitting w2 through wn. We can think of
the tuple of paths as a subset of a space, and concatenating "0"
and "1" to any one path partitions it along that dimension. The law is
stating simply that the aggregation of the subpartitions must be the
same as the overall aggregation. We assume that if at any point in
traversing the nanocube, we don't find a refinement edge along a node,
the value of that tuple of paths is the identity of the monoid.

From now on, we will call a tuple of paths an *address*.

Let a *dataset* be a set of (address, value) pairs where the addresses
are all "full": each word along a dimension has the same length.  We
will say that a nanocube *models* a dataset when, for every possible
address _a_, _value a_ equals the aggregation of all values in the
dataset such that the words in _a_ are prefixes of the respective word
in the pair.

It's easiest to explain how a nanocube is built and laid out by using
algebraic data types. This looks very much like Haskell, but hopefully
will be easy enough to read even if you don't know Haskell.

    data Nanocube value = 
	    -- a "Leaf" is a node in the 0-th dimension, holds aggregation value
        Leaf value
		-- Null is an empty sentinel node
      | Null
	    -- a Node holds references to two refinement nodes, and a reference
		-- to a next-dimension node
	  | Node (Nanocube value) (Nanocube value) (Nanocube value)

Now, the function "value" is easy to write:

    value :: [[Int]] -> Nanocube value -> value

    -- if we hit an empty node, our aggregation is empty
	value _  Null     = monoid_identity
	
	-- if we hit the zeroth dimension, we just grab the aggregation result
	value [] (Leaf v) = v
	
	-- if 
	value ([] : rest) (Node _l _r n) = value rest n
	value ((0 : dim_rest) : address_rest) (Node l _r _next) = value (dim_rest : address_rest) l
	value ((1 : dim_rest) : address_rest) (Node _l r _next) = value (dim_rest : address_rest) r
	

* [spines](spines.md)

