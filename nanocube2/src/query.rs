use nanocube::{Nanocube, NodePointerType};
use cube::Cube;
use cube::Monoid;

struct Range {
    lo: usize,
    up: usize
}

impl Range {
    fn new_from_width(width: usize) -> Range
    {
        Range { lo: 0, up: 1 << width }
    }
    fn contains(&self, other: &Range) -> bool
    {
        self.lo <= other.lo && other.up <= self.up
    }
    fn not_overlaps(&self, other: &Range) -> bool
    {
        other.up <= self.lo || self.up <= other.lo
    }
    fn overlaps(&self, other: &Range) -> bool
    {
        !self.not_overlaps(other)
    }
    // NOTE: we are using a slightly weird expression below, and that's
    // to avoid (self.lo + self.up)/2, which overflows for large ranges.
    // See the infamous Java binary search bug
    // https://research.googleblog.com/2006/06/extra-extra-read-all-about-it-nearly.html
    fn lo_subrange(&self) -> Range
    {
        Range { lo: self.lo,
                up: self.lo + ((self.up - self.lo) >> 1) }
    }
    fn up_subrange(&self) -> Range
    {
        Range { lo: self.lo + ((self.up - self.lo) >> 1),
                up: self.up }
    }
}

//////////////////////////////////////////////////////////////////////////////

fn nanocube_range_query<S>(nc: &Nanocube<S>, bounds: &Vec<(usize, usize)>) -> S
    where S: Monoid + PartialOrd
{
    assert!(bounds.len() == nc.dims.len());
    let mut dim = 0;
    let mut dim_range_list      = Vec::<(Range, NodePointerType)>::new();
    let mut next_dim_range_list = Vec::<(Range, NodePointerType)>::new();
    dim_range_list.push((Range::new_from_width(nc.dims[dim].width),
                         nc.base_root));
    // process the links into further dimensions of the nanocube
    while dim < nc.dims.len() - 1 {
        assert!(next_dim_range_list.len() == 0);
        let query_range = Range { lo: bounds[dim].0, up: bounds[dim].1 };

        while dim_range_list.len() > 0 {
            let (current_range, node_index) = dim_range_list.pop().expect("internal error");
            if node_index == -1 {
                continue;
            }
            // let node_index = node_index.expect("internal error");
            let current_node = nc.get_node(dim, node_index);
            if query_range.contains(&current_range) ||
                (current_node.left == -1 && current_node.right == -1) {
                next_dim_range_list.push((Range::new_from_width(nc.dims[dim+1].width),
                                          current_node.next));
                continue;
            }
            let lo = current_range.lo_subrange();
            let hi = current_range.up_subrange();
            if query_range.overlaps(&lo) {
                dim_range_list.push((lo, current_node.left))
            }
            if query_range.overlaps(&hi) {
                dim_range_list.push((hi, current_node.right))
            }
        }

        dim_range_list.append(&mut next_dim_range_list);
        dim += 1;
    }

    let mut result = S::mempty();
    let query_range = Range { lo: bounds[dim].0, up: bounds[dim].1 };
    
    // process the links into nanocube summaries
    while dim_range_list.len() > 0 {
        let (current_range, node_index) = dim_range_list.pop().expect("internal error");
        if node_index == -1 {
            continue;
        }
        let current_node = nc.get_node(dim, node_index);
        if query_range.contains(&current_range) ||
            (current_node.left == -1 && current_node.right == -1) {
            result = result.mapply(&nc.summaries.values[current_node.next as usize]);
            continue;
        }
        let lo = current_range.lo_subrange();
        let hi = current_range.up_subrange();
        if query_range.overlaps(&lo) {
            dim_range_list.push((lo, current_node.left))
        }
        if query_range.overlaps(&hi) {
            dim_range_list.push((hi, current_node.right))
        }
    }
    result
}
    
impl <Summary: Monoid + PartialOrd> Cube<Summary> for Nanocube<Summary> {
    fn range_query(&self, bounds: &Vec<(usize, usize)>) -> Summary {
        nanocube_range_query(self, bounds)
    }
}
