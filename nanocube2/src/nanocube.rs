use cube::Monoid;
use ref_counted_vec::RefCountedVec;
use std;
use std::fs;
use std::io::Write;
use std::path::Path;

//////////////////////////////////////////////////////////////////////////////

static DEBUG_ENABLED: bool = true;

macro_rules! debug_print {
    ($str:expr $(,$params:expr)*) => (
        if DEBUG_ENABLED {
            print!("{}:{} - ", file!(), line!());
            println!($str $(,$params)*);
        }
    )
}

macro_rules! debug_var {
    ($var:ident) => {
        debug_print!("{} = {:?}", stringify!($var), $var);
    };
}

//////////////////////////////////////////////////////////////////////////////

pub type NodePointerType = i32;

#[inline]
fn get_bit(value: usize, bit: usize) -> bool {
    (value >> bit) & 1 != 0
}

enum WhereToInsert {
    Left,
    Right,
    Next,
}

#[derive(Copy, Debug, Clone)]
pub struct NCDimNode {
    pub left: NodePointerType,
    pub right: NodePointerType,
    pub next: NodePointerType,

    // le sigh. It seems that the most convenient way to speed up
    // a lot of traversals is to flag some of the nodes along the way.
    // this is going to make concurrency really hard :/
    pub flags: i32,
}

#[derive(Clone)]
pub struct NCDim {
    pub nodes: RefCountedVec<NCDimNode>,
    pub width: usize,
}

// impl Clone for NCDim {
//     fn clone(&self) -> NCDim {
//         let result_nodes = RefCountedVec::new();
//         result_nodes.values.
//             extend(self.nodes.values.iter().map(|&d| d.clone()));
//         result_nodes.ref_counts.
//             extend_from_slice(&self.nodes.ref_counts);
//         result_nodes.free_list.
//             extend_from_slice(&self.nodes.free_list);
//         NCDim {
//             nodes: result_nodes,
//             width: self.width
//         }
//     }
// }

impl NCDim {
    #[inline]
    fn len(&self) -> usize {
        return self.nodes.len();
    }

    fn new(width: usize) -> NCDim {
        NCDim {
            nodes: RefCountedVec::new(),
            width: width,
        }
    }

    #[inline]
    pub fn at(&self, index: NodePointerType) -> &NCDimNode {
        return &self.nodes.values[index as usize];
    }

    #[inline]
    pub fn at_mut(&mut self, index: NodePointerType) -> &mut NCDimNode {
        return &mut self.nodes.values[index as usize];
    }
}

#[derive(Clone)]
pub struct Nanocube<Summary> {
    pub base_root: NodePointerType,
    pub dims: Vec<NCDim>,
    pub summaries: RefCountedVec<Summary>,
    release_list: Vec<(NodePointerType, usize)>,
}

impl<Summary: Monoid + PartialOrd + Copy> Nanocube<Summary> {
    pub fn new(widths: Vec<usize>) -> Nanocube<Summary> {
        debug_assert!(widths.len() > 0);
        Nanocube {
            base_root: -1,
            dims: widths.into_iter().map(|w| NCDim::new(w)).collect(),
            summaries: RefCountedVec::new(),
            release_list: Vec::new(),
        }
    }

    pub fn new_from_many(
        widths: Vec<usize>,
        summaries: &[Summary],
        points: &[Vec<usize>],
    ) -> Nanocube<Summary> {
        let mut nc = Nanocube::new(widths);
        nc.add_many(summaries, points);
        nc
    }

    #[inline]
    pub fn make_node_ref_not_summary_or_null(
        &mut self,
        node_index: NodePointerType,
        dim: usize,
    ) -> usize {
        debug_assert!(dim < self.dims.len());
        debug_assert!(node_index != -1);
        debug_print!("make_node_ref {:?} {}", node_index, dim);
        self.dims[dim].nodes.make_ref(node_index as isize)
    }

    #[inline]
    pub fn make_node_ref_not_null(&mut self, node_index: NodePointerType, dim: usize) -> usize {
        debug_assert!(node_index != -1);
        if dim == self.dims.len() {
            return self.summaries.make_ref(node_index as isize);
        }
        debug_print!("make_node_ref {:?} {}", node_index, dim);
        self.dims[dim].nodes.make_ref(node_index as isize)
    }

    #[inline]
    pub fn make_node_ref_not_summary(&mut self, node_index: NodePointerType, dim: usize) -> usize {
        debug_assert!(dim < self.dims.len());
        if node_index == -1 {
            return 0;
        }
        debug_print!("make_node_ref {:?} {}", node_index, dim);
        self.dims[dim].nodes.make_ref(node_index as isize)
    }

    #[inline]
    pub fn make_node_ref(&mut self, node_index: NodePointerType, dim: usize) -> usize {
        debug_print!("make_node_ref {:?} {}", node_index, dim);
        if node_index == -1 {
            return 0;
        }
        if dim == self.dims.len() {
            return self.summaries.make_ref(node_index as isize);
        }
        self.dims[dim].nodes.make_ref(node_index as isize)
    }

    pub fn flush_release_list(&mut self) {
        while self.release_list.len() > 0 {
            let (node_index, dim) = self.release_list.pop().unwrap();
            if dim == self.dims.len() {
                self.summaries.release_ref(node_index as isize);
                continue;
            }
            let new_ref_count = self.dims[dim].nodes.release_ref(node_index as isize);
            if new_ref_count != 0 {
                continue;
            }
            debug_print!("Node {},{} is free", dim, node_index);
            let node = self.dims[dim].at_mut(node_index);
            if node.left >= 0 {
                self.release_list.push((node.left, dim));
            }
            if node.right >= 0 {
                self.release_list.push((node.right, dim));
            }
            if node.next >= 0 {
                self.release_list.push((node.next, dim + 1));
            }
            node.left = -1;
            node.right = -1;
            node.next = -1;
        }
    }

    pub fn release_node_ref(&mut self, node_index: NodePointerType, dim: usize) {
        // FIXME I think this is probably better handled as a global parameter elsewhere, but..
        let release_thresh = 256;
        debug_print!("release_node_ref {:?} {}", node_index, dim);
        if node_index == -1 {
            return;
        }
        self.release_list.push((node_index, dim));
        if self.release_list.len() <= release_thresh {
            return;
        } else {
            self.flush_release_list();
        }
    }

    pub fn insert_fresh_node(
        &mut self,
        summary: Summary,
        addresses: &Vec<usize>,
        dim: usize,
        bit: usize,
    ) -> NodePointerType {
        let new_summary_ref = self.summaries.insert(summary) as NodePointerType;
        debug_print!("new summary inserted: {:?}", new_summary_ref);
        let mut next_node: NodePointerType = new_summary_ref;
        for d in (dim..self.dims.len()).rev() {
            let width = self.dims[d].width;
            let mut refine_node: NodePointerType = -1;
            let mut lo = if dim == d { bit } else { 0 };
            for b in (lo..width + 1).rev() {
                if b == width {
                    let node = NCDimNode {
                        left: -1,
                        right: -1,
                        next: next_node,
                        flags: 0,
                    };
                    refine_node = self.dims[d].nodes.insert(node) as NodePointerType;
                    debug_print!("insert node {:?} at {},{}", node, d, refine_node);
                    self.make_node_ref_not_null(next_node, d + 1);
                } else {
                    let where_to_insert = get_bit(addresses[d], width - b - 1);
                    let node = NCDimNode {
                        left: if where_to_insert { -1 } else { refine_node },
                        right: if where_to_insert { refine_node } else { -1 },
                        next: next_node,
                        flags: 0,
                    };
                    self.make_node_ref_not_summary_or_null(refine_node, d);
                    self.make_node_ref_not_null(next_node, d + 1);
                    refine_node = self.dims[d].nodes.insert(node) as NodePointerType;
                    debug_print!("insert node {:?} at {},{}", node, d, refine_node);
                }
            }
            next_node = refine_node;
        }
        next_node
    }

    #[inline]
    pub fn get_node(&self, dim: usize, node_index: NodePointerType) -> NCDimNode {
        self.dims[dim].at(node_index).clone()
    }

    #[inline]
    pub fn node_is_branching(&self, dim: usize, node_index: NodePointerType) -> bool {
        let &n = self.dims[dim].at(node_index);
        return n.left != -1 && n.right != -1;
    }

    pub fn node_is_leaf(&self, dim: usize, node_index: NodePointerType) -> bool {
        let &n = self.dims[dim].at(node_index);
        return n.left == -1 && n.right == -1;
    }

    pub fn node_is_unmarked(&self, dim: usize, node_index: NodePointerType) -> bool {
        let &n = self.dims[dim].at(node_index);
        return n.flags == 0;
    }

    pub fn mark_node(&mut self, dim: usize, node_index: NodePointerType) {
        self.dims[dim].at_mut(node_index).flags = 1;
    }

    pub fn unmark_node(&mut self, dim: usize, node_index: NodePointerType) {
        self.dims[dim].at_mut(node_index).flags = 0;
    }

    /// merge_cube takes a different nanocube and mutates self so that
    /// the following property holds: for every query with self,
    /// other, it's the case that

    /// ```
    /// let q1 = self.query(q);
    /// let q2 = other.query(q);
    /// self.merge_cube(other);
    /// let q3 = self.query(q);
    /// assert_equals!(q1.mapply(q2), q3);
    /// ```

    /// In other words, merge_cube acts like a mutable version of mapply

    /// FIXME figure out the space behavior of this operation when key spaces
    /// of both cubes overlap
    pub fn merge_cube(&mut self, other: &Nanocube<Summary>) {
        debug_assert!(other.dims.len() != self.dims.len());

        let mut offsets: Vec<usize> = (0..self.dims.len())
            .map(|dim| self.dims[dim].len())
            .collect();
        offsets.push(self.summaries.len());

        // merge the summaries array
        self.summaries.extend_from_array(&other.summaries);

        (0..self.dims.len()).map(|dim| {
            self.dims[dim].extend(other.dims[dim], |&val| {
                val + offsets[dim + 1];
            });
        });

        self.merge(self.base_root, other.base_root + offsets[0], 0);
    }

    pub fn merge(
        &mut self,
        node_1: NodePointerType,
        node_2: NodePointerType,
        dim: usize,
    ) -> NodePointerType {
        debug_print!("Will merge {:?}, {:?} in dim {}", node_1, node_2, dim);
        if node_1 == -1 {
            debug_print!("  trivial merge: {:?}", node_2);
            return node_2;
        }
        if node_2 == -1 {
            debug_print!("  trivial merge: {:?}", node_1);
            return node_1;
        }
        if dim == self.dims.len() {
            let new_summary = {
                let ref node_1_summary = self.summaries.values[node_1 as usize];
                let ref node_2_summary = self.summaries.values[node_2 as usize];
                node_1_summary.mapply(node_2_summary)
            };
            let new_summary_index = self.summaries.insert(new_summary) as NodePointerType;
            debug_print!("  summary insert: {:?}", new_summary_index);
            return new_summary_index;
        }
        debug_print!("  nontrivial merge");
        let node_1_node = self.get_node(dim, node_1);
        let node_2_node = self.get_node(dim, node_2);
        debug_print!("  left side of nontrivial merge");
        let left_merge = self.merge(node_1_node.left, node_2_node.left, dim);
        debug_print!("  right side of nontrivial merge");
        let right_merge = self.merge(node_1_node.right, node_2_node.right, dim);
        let node_1_index = left_merge;
        let node_2_index = right_merge;
        debug_print!("  next side of nontrivial merge");
        let next_merge = if node_1_index == -1 {
            if node_2_index == -1 {
                self.merge(node_1_node.next, node_2_node.next, dim + 1)
            } else {
                self.make_node_ref_not_summary_or_null(right_merge, dim);
                self.dims[dim].at(node_2_index).next
            }
        } else {
            if node_2_index == -1 {
                self.make_node_ref_not_summary_or_null(left_merge, dim);
                self.dims[dim].at(node_1_index).next
            } else {
                self.make_node_ref_not_summary_or_null(left_merge, dim);
                self.make_node_ref_not_summary_or_null(right_merge, dim);
                let node_1_merge_next_next = self.get_node(dim, node_1_index).next;
                let node_2_merge_next_next = self.get_node(dim, node_2_index).next;
                self.merge(node_1_merge_next_next, node_2_merge_next_next, dim + 1)
            }
        };
        let new_node = NCDimNode {
            left: left_merge,
            right: right_merge,
            next: next_merge,
            flags: 0,
        };
        self.make_node_ref_not_null(next_merge, dim + 1);
        let new_index = self.dims[dim].nodes.insert(new_node);
        debug_print!(
            "  Inserted merge node {:?} at {},{}",
            new_node,
            dim,
            new_index
        );
        new_index as NodePointerType
    }

    /*
    The spine of an address is the sequence of nodes down the directed
    graph of the nanocube that corresponds to the address.

    make_spine finds the "spine" of a given address.
     */
    fn make_spine(
        &self,
        addresses: &Vec<usize>,
        spine: &mut Vec<(NodePointerType, usize, usize, Option<bool>)>,
    ) {
        let mut current_node_index = self.base_root;
        let mut dim = 0;
        let mut bit = 0;
        let mut width = self.dims[dim].width;
        let mut where_to_insert = if bit == width || dim == self.dims.len() {
            None
        } else {
            Some(get_bit(addresses[dim], width - bit - 1))
        };
        debug_print!(
            "Pushing {:?} to spine",
            (current_node_index, dim, bit, where_to_insert)
        );
        spine.push((current_node_index, dim, bit, where_to_insert));
        while current_node_index != -1 && dim != self.dims.len() {
            let current_node = self.get_node(dim, current_node_index);
            debug_print!("Current node: {:?}", current_node);
            if bit == width {
                dim = dim + 1;
                if dim < self.dims.len() {
                    width = self.dims[dim].width;
                }
                bit = 0;
                current_node_index = current_node.next;
            } else {
                bit = bit + 1;
                current_node_index = if where_to_insert.unwrap() {
                    current_node.right
                } else {
                    current_node.left
                }
            }
            where_to_insert = if bit == width || dim == self.dims.len() {
                None
            } else {
                Some(get_bit(addresses[dim], width - bit - 1))
            };
            debug_print!(
                "Pushing {:?} to spine",
                (current_node_index, dim, bit, where_to_insert)
            );
            spine.push((current_node_index, dim, bit, where_to_insert));
        }
    }

    /*
    make_sparse_spine()
    make_sparse_branching_spine()

    // A "sparse" spine only includes nodes of degree 2
    // A "non-branching" spine only uses the next-dim pointers of the leaves of
    //   starting_address. A "branching" spine takes all next-dim pointers down
    //   the path.

    // A branching spine

     */

    fn make_sparse_spine(
        &self,
        addresses: &Vec<usize>,
        index: NodePointerType,
        dim: usize,
        spine: &mut Vec<(NodePointerType, usize, usize, Option<bool>)>,
    ) {
        let mut current_node_index = index;
        let mut dim = dim;
        let mut bit = 0;
        let mut width = self.dims[dim].width;
        let mut where_to_insert = if bit == width || dim == self.dims.len() {
            None
        } else {
            Some(get_bit(addresses[dim], width - bit - 1))
        };
        if self.node_is_branching(dim, current_node_index) {
            debug_print!(
                "Pushing {:?} to spine",
                (current_node_index, dim, bit, where_to_insert)
            );
            spine.push((current_node_index, dim, bit, where_to_insert));
        }
        while current_node_index != -1 && dim != self.dims.len() {
            let current_node = self.get_node(dim, current_node_index);
            debug_print!("Current node: {:?} {:?}", dim, current_node);
            if bit == width {
                dim = dim + 1;
                if dim < self.dims.len() {
                    width = self.dims[dim].width;
                }
                bit = 0;
                current_node_index = current_node.next;
            } else {
                bit = bit + 1;
                current_node_index = if where_to_insert.unwrap() {
                    current_node.right
                } else {
                    current_node.left
                }
            }
            where_to_insert = if bit == width || dim == self.dims.len() {
                None
            } else {
                Some(get_bit(addresses[dim], width - bit - 1))
            };
            if current_node_index != -1 && dim != self.dims.len() {
                // as a special case, the sparse spine includes
                // leaves of the last dimension (since these point
                // to the summary table which is what we eventually want
                // to use the sparse branching spine for)
                if self.node_is_branching(dim, current_node_index)
                    || (dim == self.dims.len() - 1 && self.node_is_leaf(dim, current_node_index))
                {
                    debug_print!(
                        "Pushing {:?} to spine",
                        (current_node_index, dim, bit, where_to_insert)
                    );
                    spine.push((current_node_index, dim, bit, where_to_insert));
                }
            }
        }
    }

    // This is the same as above, but we only push unmarked nodes (and
    // then we mark them.) make_branching_spine needs the marks to avoid
    // revisiting nodes.
    fn make_sparse_spine_marking(
        &mut self,
        addresses: &Vec<usize>,
        index: NodePointerType,
        dim: usize,
        spine: &mut Vec<(NodePointerType, usize, usize, Option<bool>)>,
    ) {
        let mut current_node_index = index;
        let mut dim = dim;
        let mut bit = 0;
        let mut width = self.dims[dim].width;
        let mut where_to_insert = if bit == width || dim == self.dims.len() {
            None
        } else {
            Some(get_bit(addresses[dim], width - bit - 1))
        };
        if self.node_is_branching(dim, current_node_index)
            && self.node_is_unmarked(dim, current_node_index)
        {
            self.mark_node(dim, current_node_index);
            debug_print!(
                "Pushing {:?} to spine",
                (current_node_index, dim, bit, where_to_insert)
            );
            spine.push((current_node_index, dim, bit, where_to_insert));
        }
        while current_node_index != -1 && dim != self.dims.len() {
            let current_node = self.get_node(dim, current_node_index);
            debug_print!("Current node: {:?} {:?}", dim, current_node);
            if bit == width {
                dim = dim + 1;
                if dim < self.dims.len() {
                    width = self.dims[dim].width;
                }
                bit = 0;
                current_node_index = current_node.next;
            } else {
                bit = bit + 1;
                current_node_index = if where_to_insert.unwrap() {
                    current_node.right
                } else {
                    current_node.left
                }
            }
            where_to_insert = if bit == width || dim == self.dims.len() {
                None
            } else {
                Some(get_bit(addresses[dim], width - bit - 1))
            };
            if current_node_index != -1
                && dim != self.dims.len()
                && self.node_is_unmarked(dim, current_node_index)
            {
                // as a special case, the sparse spine includes
                // leaves of the last dimension (since these point
                // to the summary table which is what we eventually want
                // to use the sparse branching spine for)
                if self.node_is_branching(dim, current_node_index)
                    || (dim == self.dims.len() - 1 && self.node_is_leaf(dim, current_node_index))
                {
                    self.mark_node(dim, current_node_index);
                    debug_print!(
                        "Pushing {:?} to spine",
                        (current_node_index, dim, bit, where_to_insert)
                    );
                    spine.push((current_node_index, dim, bit, where_to_insert));
                }
            }
        }
    }

    fn make_sparse_branching_spine(
        &mut self,
        addresses: &Vec<usize>,
        spine: &mut Vec<(NodePointerType, usize, usize, Option<bool>)>,
    ) {
        let spine_beg = spine.len();
        let root = self.base_root;
        self.make_sparse_spine_marking(addresses, root, 0, spine);
        let mut i = 0;
        while i < spine.len() {
            let dim = spine[i].1;
            let index = spine[i].0;
            if dim + 1 < self.dims.len() {
                let node = self.get_node(dim, index);
                debug_print!("new sparse branch: {:?} {:?}", node.next, dim + 1);
                self.make_sparse_spine_marking(addresses, node.next, dim + 1, spine);
            }
            i = i + 1;
        }
        for i in spine_beg..spine.len() {
            self.unmark_node(spine[i].1, spine[i].0);
        }
    }

    /*

    insert_new(self, summary, addresses, spine):

    Inserts a new summary into the nanocube.

    // the two return values are:
    // - the index of the merged inserted node in dims[dim];
    // - the index of the "fresh" node in dims[dim];

    Implementation:

    insert_new takes a mutable reference to a spine vector so that we
    avoid reallocating the spine at every call. Profiling showed this
    helps.

     */
    #[inline(never)]
    pub fn insert_new(
        &mut self,
        summary: Summary,
        addresses: &Vec<usize>,
        spine: &mut Vec<(NodePointerType, usize, usize, Option<bool>)>,
    ) -> (NodePointerType, NodePointerType) {
        // - first, we traverse down the path of existing nodes, collecting their addresses
        //     (call this the "spine")
        // - then, we call insert_fresh_node to create all the new fresh nodes
        // - finally, we walk back up the spine, merging the fresh insertion
        //   into it
        // let mut spine = Vec::<(NodePointerType, usize, usize, Option<bool>)>::with_capacity(128);

        // "- first, we traverse ..."
        let mut current_node_index = self.base_root;
        let mut dim = 0;
        let mut bit = 0;
        let mut width = self.dims[dim].width;
        let mut where_to_insert = if bit == width || dim == self.dims.len() {
            None
        } else {
            Some(get_bit(addresses[dim], width - bit - 1))
        };
        debug_print!(
            "Pushing {:?} to spine",
            (current_node_index, dim, bit, where_to_insert)
        );
        spine.push((current_node_index, dim, bit, where_to_insert));
        while current_node_index != -1 && dim != self.dims.len() {
            let current_node = self.get_node(dim, current_node_index);
            debug_print!("Current node: {:?}", current_node);
            if bit == width {
                dim = dim + 1;
                if dim < self.dims.len() {
                    width = self.dims[dim].width;
                }
                bit = 0;
                current_node_index = current_node.next;
            } else {
                bit = bit + 1;
                current_node_index = if where_to_insert.unwrap() {
                    current_node.right
                } else {
                    current_node.left
                }
            }
            where_to_insert = if bit == width || dim == self.dims.len() {
                None
            } else {
                Some(get_bit(addresses[dim], width - bit - 1))
            };
            debug_print!(
                "Pushing {:?} to spine",
                (current_node_index, dim, bit, where_to_insert)
            );
            spine.push((current_node_index, dim, bit, where_to_insert));
        }

        // "- then, we call insert_fresh_node ..."
        let (current_node_index, dim, bit, _where_to_insert) = spine.pop().unwrap();
        let mut result = if current_node_index == -1 {
            debug_print!(
                "inserting fresh node {:?} {} {} at {:?}",
                addresses,
                dim,
                bit,
                current_node_index
            );
            let fresh_insert = self.insert_fresh_node(summary, addresses, dim, bit);
            (fresh_insert, fresh_insert)
        } else {
            // if current_node_index != -1, then insertion "bottomed
            // out": there already existed a record with this exact
            // address. we need to insert a summary and merge
            // manually, as a special case
            debug_print!("insert bottomed out - inserting into summary array");
            debug_assert!(dim == self.dims.len());
            debug_assert!(bit == 0);
            let merged_summary =
                summary.mapply(&self.summaries.values[current_node_index as usize]);
            let new_insert = self.summaries.insert(summary) as NodePointerType;
            let merged_insert = self.summaries.insert(merged_summary) as NodePointerType;
            debug_print!("  new: {}", new_insert);
            debug_print!("  merged: {}", merged_insert);
            (merged_insert, new_insert)
        };

        // "- finally, we walk back up the spine..."
        while spine.len() > 0 {
            // loop invariant:
            //  - result = (merged_insert, fresh_insert)
            //  - merged_insert holds a reference to merged "dataset" so far,
            //  - fresh_insert holds a reference to singleton inserted item so far,
            //  - top of spine holds the first node that needs merging.

            let (current_node_index, dim, _bit, where_to_insert) = spine.pop().unwrap();
            debug_assert!(current_node_index != -1);
            let current_node = self.get_node(dim, current_node_index);

            // this is the big annoying case analysis for merging:
            let new_node = match (current_node.left, current_node.right, where_to_insert) {
                (-1, -1, Some(_)) => {
                    println!("(-1, -1, Some(_)) PANIC");
                    unreachable!();
                }
                (-1, -1, None) => {
                    debug_print!("case 7");
                    NCDimNode {
                        left: -1,
                        right: -1,
                        next: result.0, // self.get_node(dim, result.0).next
                        flags: 0,
                    }
                }
                (_a, -1, Some(false)) => {
                    debug_print!("case 1");
                    let a_union_c = result.0;
                    NCDimNode {
                        left: a_union_c,
                        right: -1,
                        next: self.get_node(dim, a_union_c).next,
                        flags: 0,
                    }
                }
                (_a, -1, Some(true)) => {
                    debug_print!("case 2");
                    let c = result.0;
                    let n1 = self.get_node(dim, current_node.left).next;
                    let n2 = self.get_node(dim, result.0).next;
                    let an_union_cn = self.merge(n1, n2, dim + 1);
                    NCDimNode {
                        left: current_node.left,
                        right: c,
                        next: an_union_cn,
                        flags: 0,
                    }
                }
                (_a, -1, None) => {
                    println!("(a, -1, None) PANIC");
                    unreachable!();
                }
                (-1, _b, Some(false)) => {
                    debug_print!("case 3");
                    let c = result.0;
                    let n1 = self.get_node(dim, current_node.right).next;
                    let n2 = self.get_node(dim, result.0).next;
                    let bn_union_cn = self.merge(n1, n2, dim + 1);
                    NCDimNode {
                        left: c,
                        right: current_node.right,
                        next: bn_union_cn,
                        flags: 0,
                    }
                }
                (-1, _b, Some(true)) => {
                    debug_print!("case 4");
                    let b_union_c = result.0;
                    NCDimNode {
                        left: -1,
                        right: b_union_c,
                        next: self.get_node(dim, b_union_c).next,
                        flags: 0,
                    }
                }
                (-1, _b, None) => {
                    println!("(-1, b, None) PANIC");
                    unreachable!();
                }
                (_a, _b, Some(false)) => {
                    debug_print!("case 5");
                    let a_union_c = result.0;
                    let c = result.1;
                    let an_union_bn = current_node.next;
                    let cn = self.get_node(dim, c).next;

                    // Here's a relatively clever bit. We need the next node to
                    // point to (a_next union b_next union c_next).
                    //
                    // (Eliding _next for what follows,) The
                    // straightforward way to do it is to compute (a union c)
                    // union b, since we will always have (a union
                    // c) from the recursion result, and we have b from the current node.  however,
                    // this is inefficient, because the union operation
                    // (merge) can be slow if both sides are complex.
                    //
                    // On the other hand, nanocubes are only well defined
                    // for commutative monoids, which means that (a union
                    // c) union b = (a union b) union c. So, if we keep
                    // track of the freshly inserted node c (which will be
                    // a simple structure since it contains a single
                    // value), then we union the old (a union b) with the new c
                    // and that's equivalent, and faster.
                    //
                    // And this is why we need to lug result.1 around
                    // everywhere.

                    // this is relatively fast
                    let an_union_bn_union_cn = self.merge(an_union_bn, cn, dim + 1);

                    // this is pretty slow so we don't do it.
                    // let an_union_bn_union_cn = self.merge(an_union_cn, bn, dim+1);
                    NCDimNode {
                        left: a_union_c,
                        right: current_node.right,
                        next: an_union_bn_union_cn,
                        flags: 0,
                    }
                }
                (_a, _b, Some(true)) => {
                    debug_print!("case 6");
                    // similar logic as the one explained above.

                    let b_union_c = result.0;
                    let c = result.1;
                    let an_union_bn = current_node.next;
                    let cn = self.get_node(dim, c).next;

                    // this is relatively fast
                    let an_union_bn_union_cn = self.merge(an_union_bn, cn, dim + 1);

                    // this is pretty slow so we don't do it.
                    // let an_union_bn_union_cn = self.merge(an_union_cn, bn, dim+1);
                    NCDimNode {
                        left: current_node.left,
                        right: b_union_c,
                        next: an_union_bn_union_cn,
                        flags: 0,
                    }
                }
                (_a, _b, None) => {
                    println!("(a, b, None) PANIC");
                    unreachable!();
                }
            };
            self.make_node_ref_not_summary(new_node.left, dim);
            self.make_node_ref_not_summary(new_node.right, dim);
            self.make_node_ref_not_null(new_node.next, dim + 1);
            let new_merged_index = self.dims[dim].nodes.insert(new_node);
            debug_print!(
                "insert merge node {:?} at {},{}",
                new_node,
                dim,
                new_merged_index
            );
            let new_fresh_node = match where_to_insert {
                Some(false) => {
                    let n = self.get_node(dim, result.1).next;
                    self.make_node_ref_not_summary(result.1, dim);
                    self.make_node_ref_not_null(n, dim + 1);
                    NCDimNode {
                        left: result.1,
                        right: -1,
                        next: self.get_node(dim, result.1).next,
                        flags: 0,
                    }
                }
                Some(true) => {
                    let n = self.get_node(dim, result.1).next;
                    self.make_node_ref_not_summary(result.1, dim);
                    self.make_node_ref_not_null(n, dim + 1);
                    NCDimNode {
                        left: -1,
                        right: result.1,
                        next: n,
                        flags: 0,
                    }
                }
                None => {
                    self.make_node_ref_not_null(result.1, dim + 1);
                    NCDimNode {
                        left: -1,
                        right: -1,
                        next: result.1,
                        flags: 0,
                    }
                }
            };
            let new_fresh_index = self.dims[dim].nodes.insert(new_fresh_node);
            debug_print!(
                "insert 'fresh' node {:?} at {},{}",
                new_fresh_node,
                dim,
                new_fresh_index
            );
            result = (
                new_merged_index as NodePointerType,
                new_fresh_index as NodePointerType,
            )
        }
        result
    }

    pub fn insert(
        &mut self,
        summary: Summary,
        addresses: &Vec<usize>,
        dim: usize,
        bit: usize,
        current_node_index: NodePointerType,
    ) -> (NodePointerType, NodePointerType)
// the two return values are:
    // - the index of the merged inserted node in dims[dim];
    // - the index of the "fresh" node in dims[dim];
    {
        if dim == self.dims.len() {
            // we bottomed out; just insert a summary and be done.
            debug_print!("insert bottomed out - inserting into summary array");
            let merged_summary =
                summary.mapply(&self.summaries.values[current_node_index as usize]);
            let new_summary_index = self.summaries.insert(summary);
            let merged_summary_index = self.summaries.insert(merged_summary);
            debug_print!("  new: {}", new_summary_index);
            debug_print!("  merged: {}", merged_summary_index);
            return (
                merged_summary_index as NodePointerType,
                new_summary_index as NodePointerType,
            );
        }
        if current_node_index == -1 {
            debug_print!(
                "inserting fresh node {:?} {} {} at {:?}",
                addresses,
                dim,
                bit,
                current_node_index
            );
            let fresh_insert = self.insert_fresh_node(summary, addresses, dim, bit);
            debug_print!("--- inserted fresh node.");
            return (fresh_insert, fresh_insert);
        }
        let width = self.dims[dim].width;
        let current_node = self.get_node(dim, current_node_index);
        let current_address = addresses[dim];
        debug_print!("Current node: {} {:?}", current_node_index, current_node);

        let (where_to_insert, result) = if bit == width {
            // recurse into next dimension through next node
            (
                None,
                self.insert(summary, addresses, dim + 1, 0, current_node.next),
            )
        } else {
            // recurse into current dimension through refinement nodes
            let w = get_bit(current_address, width - bit - 1);
            let refinement_node = if w {
                current_node.right
            } else {
                current_node.left
            };
            (
                Some(w),
                self.insert(summary, addresses, dim, bit + 1, refinement_node),
            )
        };

        let new_node = match (current_node.left, current_node.right, where_to_insert) {
            (-1, -1, Some(_)) => {
                println!("(-1, -1, Some(_)) PANIC");
                unreachable!();
            }
            (-1, -1, None) => {
                debug_print!("case 7");
                NCDimNode {
                    left: -1,
                    right: -1,
                    next: result.0, // self.get_node(dim, result.0).next
                    flags: 0,
                }
            }
            (_a, -1, Some(false)) => {
                debug_print!("case 1");
                let a_union_c = result.0;
                NCDimNode {
                    left: a_union_c,
                    right: -1,
                    next: self.get_node(dim, a_union_c).next,
                    flags: 0,
                }
            }
            (_a, -1, Some(true)) => {
                debug_print!("case 2");
                let c = result.0;
                let n1 = self.get_node(dim, current_node.left).next;
                let n2 = self.get_node(dim, result.0).next;
                let an_union_cn = self.merge(n1, n2, dim + 1);
                NCDimNode {
                    left: current_node.left,
                    right: c,
                    next: an_union_cn,
                    flags: 0,
                }
            }
            (_a, -1, None) => {
                println!("(a, -1, None) PANIC");
                unreachable!();
            }
            (-1, _b, Some(false)) => {
                debug_print!("case 3");
                let c = result.0;
                let n1 = self.get_node(dim, current_node.right).next;
                let n2 = self.get_node(dim, result.0).next;
                let bn_union_cn = self.merge(n1, n2, dim + 1);
                NCDimNode {
                    left: c,
                    right: current_node.right,
                    next: bn_union_cn,
                    flags: 0,
                }
            }
            (-1, _b, Some(true)) => {
                debug_print!("case 4");
                let b_union_c = result.0;
                NCDimNode {
                    left: -1,
                    right: b_union_c,
                    next: self.get_node(dim, b_union_c).next,
                    flags: 0,
                }
            }
            (-1, _b, None) => {
                println!("(-1, b, None) PANIC");
                unreachable!();
            }
            (_a, _b, Some(false)) => {
                debug_print!("case 5");
                let a_union_c = result.0;
                let c = result.1;
                let an_union_bn = current_node.next;
                let cn = self.get_node(dim, c).next;

                // Here's a relatively clever bit. We need the next node to
                // point to (a_next union b_next union c_next).
                //
                // (Eliding _next for what follows,) The
                // straightforward way to do it is to compute (a union c)
                // union b, since we will always have (a union
                // c) from the recursion result, and we have b from the current node.  however,
                // this is inefficient, because the union operation
                // (merge) can be slow if both sides are complex.
                //
                // On the other hand, nanocubes are only well defined
                // for commutative monoids, which means that (a union
                // c) union b = (a union b) union c. So, if we keep
                // track of the freshly inserted node c (which will be
                // a simple structure since it contains a single
                // value), then we union the old (a union b) with the new c
                // and that's equivalent, and faster.
                //
                // And this is why we need to lug result.1 around
                // everywhere.

                // this is relatively fast
                let an_union_bn_union_cn = self.merge(an_union_bn, cn, dim + 1);

                // this is pretty slow so we don't do it.
                // let an_union_bn_union_cn = self.merge(an_union_cn, bn, dim+1);
                NCDimNode {
                    left: a_union_c,
                    right: current_node.right,
                    next: an_union_bn_union_cn,
                    flags: 0,
                }
            }
            (_a, _b, Some(true)) => {
                debug_print!("case 6");
                // similar logic as the one explained above.

                let b_union_c = result.0;
                let c = result.1;
                let an_union_bn = current_node.next;
                let cn = self.get_node(dim, c).next;

                // this is relatively fast
                let an_union_bn_union_cn = self.merge(an_union_bn, cn, dim + 1);

                // this is pretty slow so we don't do it.
                // let an_union_bn_union_cn = self.merge(an_union_cn, bn, dim+1);
                NCDimNode {
                    left: current_node.left,
                    right: b_union_c,
                    next: an_union_bn_union_cn,
                    flags: 0,
                }
            }
            (_a, _b, None) => {
                println!("(a, b, None) PANIC");
                unreachable!();
            }
        };
        self.make_node_ref_not_summary(new_node.left, dim);
        self.make_node_ref_not_summary(new_node.right, dim);
        self.make_node_ref_not_null(new_node.next, dim + 1);
        let new_index = self.dims[dim].nodes.insert(new_node);
        // self.release_node_ref(result.0, dim);
        debug_print!("insert merge node {:?} at {},{}", new_node, dim, new_index);

        let new_orphan_node = match where_to_insert {
            Some(false) => {
                let n = self.get_node(dim, result.1).next;
                self.make_node_ref_not_summary(result.1, dim);
                self.make_node_ref_not_null(n, dim + 1);
                NCDimNode {
                    left: result.1,
                    right: -1,
                    next: self.get_node(dim, result.1).next,
                    flags: 0,
                }
            }
            Some(true) => {
                let n = self.get_node(dim, result.1).next;
                self.make_node_ref_not_summary(result.1, dim);
                self.make_node_ref_not_null(n, dim + 1);
                NCDimNode {
                    left: -1,
                    right: result.1,
                    next: n,
                    flags: 0,
                }
            }
            None => {
                self.make_node_ref_not_null(result.1, dim + 1);
                NCDimNode {
                    left: -1,
                    right: -1,
                    next: result.1,
                    flags: 0,
                }
            }
        };
        let new_orphan_index = self.dims[dim].nodes.insert(new_orphan_node);
        debug_print!(
            "insert 'orphan' node {:?} at {},{}",
            new_orphan_node,
            dim,
            new_orphan_index
        );
        (
            new_index as NodePointerType,
            new_orphan_index as NodePointerType,
        )
    }

    pub fn add_many(&mut self, summaries: &[Summary], address_list: &[Vec<usize>]) {
        let mut spine = Vec::with_capacity(64);
        for i in 0..address_list.len() {
            let ref addresses = address_list[i];
            let summary = summaries[i];
            let base = self.base_root;
            debug_print!("Will add. {:?}", addresses);
            // let (result_base, orphan_node) = self.insert(summary, addresses, 0, 0, base);
            let (result_base, orphan_node) = self.insert_new(summary, &addresses, &mut spine);
            self.make_node_ref(result_base, 0);
            debug_print!("Will release.");
            self.release_node_ref(base, 0);
            debug_print!("-----Done.");
            self.base_root = result_base;

            // This silly ditty is here because the orphan_node is convenient to carry
            // around during insertion, but is actually not needed anywhere else. In
            // order for us to clean it up easily, we first grab a reference to it,
            // and then we release it, so that the refcount gc can do its job.
            self.make_node_ref(orphan_node, 0);
            self.release_node_ref(orphan_node, 0);
        }
    }

    /*
    update(self, summary, addresses)

    A faster version of add() that can be used when addresses point
    to an already-existing summary in the nanocube. This function
    avoid creating any new structure, and simply updates the summary
    vector appropriately.

    THIS METHOD IS NOT FINISHED

    */
    pub fn update(&mut self, summary: Summary, addresses: &Vec<usize>) {
        debug_print!("Will update. {:?}", addresses);
        let mut spine = Vec::with_capacity(64);
        self.make_spine(addresses, &mut spine);
        // askdjhfaklsjdfhakls FIXME FIXME FIXME THIS NEEDS MORE THAN JUST THE "ROOT SPINE"
        // WE NEED TO FIND THE "SPINE BRANCHES" AS WELL. THE SPINE BRANCHES ARE THE SPINES
        // OF THE NEXT NODES OF EACH INTERNAL BRANCHING NODE IN THE ROOT SPINE.
    }

    pub fn add(&mut self, summary: Summary, addresses: &Vec<usize>) {
        let base = self.base_root;
        debug_print!("Will add. {:?}", addresses);
        let mut spine = Vec::with_capacity(64);
        // let (result_base, orphan_node) = self.insert(summary, addresses, 0, 0, base);
        let (result_base, orphan_node) = self.insert_new(summary, addresses, &mut spine);
        self.make_node_ref(result_base, 0);
        debug_print!("Will release.");
        self.release_node_ref(base, 0);
        debug_print!("-----Done.");
        self.base_root = result_base;

        // This silly ditty is here because the orphan_node is convenient to carry
        // around during insertion, but is actually not needed anywhere else. In
        // order for us to clean it up easily, we first grab a reference to it,
        // and then we release it, so that the refcount gc can do its job.
        self.make_node_ref(orphan_node, 0);
        self.release_node_ref(orphan_node, 0);
    }

    #[inline]
    pub fn get_summary_index(&self, node_index: NodePointerType, dim: usize) -> NodePointerType {
        let mut current_node_index = node_index;
        let mut dim = dim;
        while dim < self.dims.len() {
            if current_node_index == -1 {
                return -1;
            }
            let next_index = self.dims[dim].at(current_node_index).next;
            dim += 1;
            current_node_index = next_index;
        }
        return current_node_index;
    }

    pub fn report_size(&self) {
        debug_print!("Summary counts: {}", self.summaries.len());
        debug_print!("Dimension counts:");
        for dim in self.dims.iter() {
            debug_print!("  {}", dim.len());
        }
        debug_print!("");
    }

    pub fn debug_print(&self) {
        for ncdim in self.dims.iter() {
            debug_print!("--------");
            for node in ncdim.nodes.values.iter() {
                debug_print!("{:?} {:?} {:?}", node.left, node.right, node.next);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////

fn node_id(i: usize, dim: usize) -> String {
    format!("\"{}_{}\"", i, dim)
}

fn print_dot_ncdim<W: std::io::Write>(
    w: &mut W,
    dim: &NCDim,
    d: usize,
    _draw_next: bool,
) -> Result<(), std::io::Error> {
    writeln!(w, " subgraph cluster_{} {{", d).expect("Can't write to w");
    writeln!(w, " label=\"Dim. {}\";", d).expect("Can't write to w");
    for i in 0..dim.nodes.values.len() {
        if dim.nodes.ref_counts[i] == 0 {
            continue;
        }
        let next = match dim.nodes.values[i].next {
            -1 => format!("{}", "null"),
            s => format!("{}", s),
        };
        // writeln!(w, "  {} [label=\"{}:{} [{}]\"];", node_id(i, d), i, next, dim.nodes.ref_counts[i]).expect("Can't write to w");;
        writeln!(w, "  {} [label=\"{}:{}\"];", node_id(i, d), i, next).expect("Can't write to w");
    }
    writeln!(w, "}}").expect("Can't write to w");;
    for i in 0..dim.nodes.values.len() {
        let node = &dim.nodes.values[i];
        if node.left >= 0 {
            writeln!(
                w,
                "  {} -> {} [label=\"0\"];",
                node_id(i, d),
                node_id(node.left as usize, d)
            )
            .expect("Can't write to w");
        }
        if node.right >= 0 {
            writeln!(
                w,
                "  {} -> {} [label=\"1\"];",
                node_id(i, d),
                node_id(node.right as usize, d)
            )
            .expect("Can't write to w");
        }
    }
    Ok(())
}

pub fn print_dot<W: std::io::Write, Summary>(
    w: &mut W,
    nc: &Nanocube<Summary>,
) -> Result<(), std::io::Error> {
    writeln!(w, "digraph G {{").expect("Can't write to w");
    writeln!(w, "    splines=line;").expect("Can't write to w");;
    for i in 0..nc.dims.len() {
        print_dot_ncdim(w, &nc.dims[i], i, i < (nc.dims.len() - 1)).expect("Can't write to w");
    }
    writeln!(w, "}}").expect("Can't write to w");
    Ok(())
}

//////////////////////////////////////////////////////////////////////////////

pub fn write_dot_to_disk<Summary>(
    name: &str,
    nc: &Nanocube<Summary>,
) -> Result<(), std::io::Error> {
    let mut f = std::fs::File::create(name).expect("cannot create file");
    print_dot(&mut f, &nc)
}

pub fn write_txt_to_disk<Summary: std::fmt::Debug>(
    name: &str,
    nc: &Nanocube<Summary>,
) -> Result<(), std::io::Error> {
    let mut f = std::fs::File::create(name).expect("cannot create file");
    for ncdim in nc.dims.iter() {
        writeln!(f, "--------").expect("Can't write to w");
        for node in ncdim.nodes.values.iter() {
            writeln!(f, "{:?} {:?} {:?}", node.left, node.right, node.next)
                .expect("Can't write to w");
            // writeln!(f, "{:?} {:?} {:?} [{:?}]", node.left, node.right, node.next,
            //          ncdim.nodes.ref_counts[i]).expect("Can't write to w");
        }
    }
    writeln!(f, "Summaries").expect("Can't write to w");
    for (i, s) in nc.summaries.values.iter().enumerate() {
        writeln!(f, "{:?} {:?}", i, s).expect("Can't write to w");;
        // writeln!(f, "{:?} {:?} [{:?}]", i, s, nc.summaries.ref_counts[i]).expect("Can't write to w");;
    }
    Ok(())
}

pub fn make_nanocube(dims: Vec<usize>, data: &Vec<Vec<usize>>) -> Nanocube<usize> {
    let mut nc = Nanocube::<usize>::new(dims);
    for d in data {
        nc.add(1, d);
    }
    nc
}

pub fn test_nanocube(out: &str, dims: Vec<usize>, data: &Vec<Vec<usize>>) {
    let mut nc = Nanocube::<usize>::new(dims);
    for d in data {
        nc.add(1, d);
    }
    nc.flush_release_list();
    write_dot_to_disk(out, &nc).expect("Couldn't write");
}

fn write_animation(path_prefix: &str, dims: Vec<usize>, data: &Vec<Vec<usize>>) {
    let mut nc = Nanocube::<usize>::new(dims);
    for (i, d) in data.iter().enumerate() {
        nc.add(1, d);
        nc.flush_release_list();
        write_dot_to_disk(&format!("{}{}.dot", path_prefix, i), &nc).expect("Couldn't write");
    }
}

//////////////////////////////////////////////////////////////////////////////

pub fn ensure_dir(path: &std::path::Path) -> std::io::Result<()> {
    match std::fs::metadata(path) {
        Err(e) => std::fs::create_dir(path),
        Ok(attr) => {
            if attr.is_dir() {
                Ok(())
            } else {
                Err(std::io::Error::new(
                    std::io::ErrorKind::AlreadyExists,
                    "path exists",
                ))
            }
        }
    }
}

#[test]
pub fn it_doesnt_smoke() {
    ensure_dir(Path::new("test-output")).expect("path exists, bailing");

    write_animation(
        "test-output/example1-",
        vec![3, 3],
        &vec![vec![0, 0], vec![0, 2], vec![6, 4], vec![6, 6]],
    );

    write_animation(
        "test-output/example3-",
        vec![3, 3],
        &vec![vec![0, 0], vec![0, 2], vec![6, 4], vec![6, 2]],
    );

    write_animation(
        "test-output/example2-",
        vec![3, 3],
        &vec![vec![0, 0], vec![0, 2], vec![6, 4], vec![6, 6]],
    );

    // test_nanocube("test-output/out2.dot", vec![2,2],
    //               &vec![vec![0,0],
    //                     vec![1,3]]);
    // test_nanocube("test-output/out3.dot", vec![2,2],
    //               &vec![vec![0,0],
    //                     vec![0,0],
    //                     vec![0,0]]);
    // test_nanocube("test-output/out4.dot", vec![2,2],
    //               &vec![vec![0,0],
    //                     vec![0,3]]);
    // test_nanocube("test-output/out5.dot", vec![2,2],
    //               &vec![vec![0,0],
    //                     vec![0,1]]);
    // test_nanocube("test-output/out.dot", vec![2,2],
    //               &vec![vec![0,0],
    //                     vec![0,0]]);
    // test_nanocube("test-output/out1.dot", vec![2,2],
    //               &vec![vec![2,1]]);
    // test_nanocube("test-output/out2.dot", vec![2,2],
    //               &vec![vec![2,1],
    //                     vec![1,0]]);
    // test_nanocube("test-output/out0.dot", vec![2,2],
    //               &vec![vec![0,0]]);
    // test_nanocube("test-output/out1.dot", vec![2,2],
    //               &vec![vec![0,0],
    //                     vec![3,3]]);

    //////////////////////////////////////////////////////////////////////////
    // regression smoke tests

    test_nanocube(
        "/dev/null",
        vec![2, 2],
        &vec![vec![0, 0], vec![1, 0], vec![1, 1]],
    );
    test_nanocube(
        "/dev/null",
        vec![2, 2],
        &vec![vec![2, 1], vec![1, 0], vec![1, 1]],
    );
    test_nanocube(
        "/dev/null",
        vec![2, 2],
        &vec![vec![1, 0], vec![1, 3], vec![1, 2]],
    );
    test_nanocube("/dev/null", vec![2, 2], &vec![vec![0, 0]]);
    test_nanocube(
        "/dev/null",
        vec![2, 2],
        &vec![vec![0, 2], vec![1, 0], vec![2, 2], vec![0, 2]],
    );
    test_nanocube("/dev/null", vec![2, 2], &vec![vec![0, 2], vec![0, 2]]);
    test_nanocube("/dev/null", vec![2, 3], &vec![vec![2, 5], vec![0, 6]]);

    //////////////////////////////////////////////////////////////////////////

    // test_nanocube("test-output/out0.dot", vec![2,2],
    //               &vec![vec![0,0],
    //                     vec![0,0]]);

    // test_nanocube("test-output/out0.dot", vec![2,2],
    //               &vec![vec![0,3]]);

    // test_nanocube("test-output/out1.dot", vec![2,2],
    //               &vec![vec![0,3],
    //                     vec![2,3]]);

    // test_nanocube("test-output/out2.dot", vec![2,2],
    //               &vec![vec![0,3],
    //                     vec![2,3],
    //                     vec![2,3]]);

    // test_nanocube("test-output/out3.dot", vec![2,2],
    //               &vec![vec![0,3],
    //                     vec![2,3],
    //                     vec![2,3],
    //                     vec![0,3]]);

    // test_nanocube("test-output/out0.dot", vec![2,2],
    //               &vec![vec![1,0]]);
    // test_nanocube("test-output/out1.dot", vec![2,2],
    //               &vec![vec![1,0],
    //                     vec![1,3]]);

    //////////////////////////////////////////////////////////////////////////
    // self.update() smoke tests

    {
        let mut nc = make_nanocube(
            vec![3, 3],
            &vec![vec![0, 0], vec![0, 2], vec![6, 4], vec![6, 6]],
        );
        nc.flush_release_list();
        write_dot_to_disk("test-output/spine-test.dot", &nc).expect("couldn't write");
        {
            let mut spine = Vec::with_capacity(64);
            nc.make_spine(&vec![6, 6], &mut spine);
            debug_print!("{:?}", spine);
        }
        println!("-----");
        {
            let mut spine = Vec::with_capacity(64);
            let root = nc.base_root;
            nc.make_sparse_spine(&vec![6, 6], root, 0, &mut spine);
            debug_print!("{:?}", spine);
        }
        println!("-----");
        {
            let mut spine = Vec::with_capacity(64);
            nc.make_sparse_branching_spine(&vec![6, 6], &mut spine);
            debug_print!("{:?}", spine);
        }
    }
}

impl<Summary: Monoid + PartialOrd + Copy> Monoid for Nanocube<Summary> {
    fn mempty() -> Nanocube<Summary> {
        Nanocube::<Summary>::new(vec![1]); // this is an ugly hack in order for us to have a sensible empty element
    }

    fn mapply(&self, rhs: &Nanocube<Summary>) -> Nanocube<Summary> {
        let result = self.clone();
        result.merge_cube(rhs);
        result
    }
}
