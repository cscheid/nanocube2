use nanocube;

struct Range {
    lo: usize,
    up: usize
};

impl Range {
    fn new_from_width(width: usize) -> Range
    {
        Range { lo: 0, hi: 1 << width };
    }
    fn contains(&self, other: &Range) -> bool
    {
        self.lo <= other.lo && other.hi <= self.hi
    }
    // NOTE: we are using a slightly weird expression below, and that's
    // to avoid (self.lo + self.hi)/2, which overflows for large ranges.
    // See the infamous Java binary search bug
    // https://research.googleblog.com/2006/06/extra-extra-read-all-about-it-nearly.html
    fn lo_subrange(&self) -> Range
    {
        Range { lo: self.lo,
                hi: self.lo + ((self.hi - self.lo) >> 1) };
    }
    fn hi_subrange(&self) -> Range
    {
        Range { lo: self.lo + ((self.hi - self.lo) >> 1),
                hi: self.hi };
    }
};

pub fn range_query<S>(nc: &Nanocube<S>, bounds: &Vec<(usize, usize)>) -> Monoid
    where S: Monoid + PartialOrd
{
    assert!(bounds.len() == nc.dims.len());
    let mut dim = 0;
    let mut dim_range_list      = Vec::<(Range, Option<usize>)>::new();
    let mut next_dim_range_list = Vec::<(Range, Option<usize>)>::new();
    dim_range_list.push((Range::new_from_width(nc.dims[dim].width),
                         nc.base_root));
    // process the links into further dimensions of the nanocube
    while dim < nc.dims.len() - 1 {
        assert!(next_dim_range_list.len() == 0);
        let query_range = Range { lo: bounds[dim].0, hi: bounds[dim].1 };

        while dim_range_list.len() > 0 {
            let (node_range, node_index) = dim_range_list.pop().expect("internal error");
            if node_index == None {
                continue;
            }
            let node_index = node_index.expect("internal error");
            let current_node = nc.get_node(dim, node_index);
            if (query_range.contains(current_range) ||
                (current_node.left == None && current_node.right == None)) {
                next_dim_range_list.push((Range::new_from_width(nc.dims[dim+1].width),
                                          current_node.next));
                continue;
            }
            let lo = current_range.lo_subrange();
            let hi = current_range.hi_subrange();
            if query_range.overlaps(lo) {
                dim_range_list.push((lo, current_node.left))
            }
            if query_range.overlaps(hi) {
                dim_range_list.push((hi, current_node.right))
            }
        }

        dim_range_list.append(next_dim_range_list);
    }

    let mut result = Monoid::mempty();
    
    // process the links into nanocube summaries
    while dim_range_list.len() > 0 {
        let (node_range, node_index) = dim_range_list.pop().expect("internal error");
        if node_index == None {
            continue;
        }
        let node_index = node_index.expect("internal error");
        let current_node = nc.get_node(dim, node_index);
        if (query_range.contains(current_range) ||
            (current_node.left == None && current_node.right == None)) {
            let summary_index = current_node.next.expect("internal error");
            result = result.mapply(nc.summaries.values[summary_index]);
            continue;
        }
        let lo = current_range.lo_subrange();
        let hi = current_range.hi_subrange();
        if query_range.overlaps(lo) {
            dim_range_list.push((lo, current_node.left))
        }
        if query_range.overlaps(hi) {
            dim_range_list.push((hi, current_node.right))
        }
    }
    result
}
    
