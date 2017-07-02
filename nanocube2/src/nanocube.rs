use ref_counted_vec::RefCountedVec;

fn get_bit(value: usize, bit: usize) -> bool
{
  (value >> bit) & 1 != 0
}

pub struct NCDimNode {
    left: Option<usize>,
    right: Option<usize>,
    next: Option<usize>
}

pub struct NCDim {
    nodes: RefCountedVec<NCDimNode>,
    width: usize
}

impl NCDim {
    fn len(&self) -> usize {
        return self.nodes.len();
    }

    fn new(width: usize) -> NCDim {
        NCDim {
            nodes: RefCountedVec::new(),
            width: width
        }
    }
}

impl NCDim {
    pub fn at(&self, index: usize) -> &NCDimNode {
        return &self.nodes.values[index];
    }

    pub fn at_mut(&mut self, index: usize) -> &mut NCDimNode {
        return &mut self.nodes.values[index];
    }
}

pub struct Nanocube<Summary> {
    base_root: Option<usize>,
    dims: Vec<NCDim>,
    summaries: RefCountedVec<Summary>
}

impl <Summary> Nanocube<Summary> {

    pub fn new(widths: Vec<usize>) -> Nanocube<Summary> {
        assert!(widths.len() > 0);
        Nanocube {
            base_root: None,
            dims: widths.into_iter().map(|w| NCDim::new(w)).collect(),
            summaries: RefCountedVec::new()
        }
    }
    
    pub fn make_node_ref(&mut self, node_index: Option<usize>, dim: usize) -> usize {
        match node_index {
            None => 0,
            Some(v) if dim == self.dims.len() => self.summaries.make_ref(v),
            Some(v) /* otherwise */           => {
                assert!(v < self.dims[dim].len());
                self.dims[dim].nodes.make_ref(v)
            }
        }
    }
    pub fn insert_fresh_node(&mut self,
                             summary: Summary, addresses: &Vec<usize>, dim: usize, bit: usize)
                             -> (Option<usize>, Option<usize>) {
        assert!(self.dims.len() == addresses.len());
        if dim == self.dims.len() {
            let new_ref = self.summaries.insert(summary);
            (Some(new_ref), Some(new_ref))
        } else {
            let width = self.dims[dim].width;
            if bit == width {
                let next_dim_result = self.insert_fresh_node(summary, addresses, dim+1, 0);
                let new_node_at_current_dim = NCDimNode {
                    left: None,
                    right: None,
                    next: next_dim_result.0
                };
                self.make_node_ref(next_dim_result.0, dim+1);
                let new_index = self.dims[dim].nodes.insert(new_node_at_current_dim);
                return (Some(new_index), next_dim_result.1);
            } else {
                let where_to_insert = get_bit(addresses[dim], width-bit-1);
                let recursion_result = self.insert_fresh_node(summary, addresses, dim, bit+1);
                let left_recursion_result = recursion_result.0.expect("fresh node should have returned Some");
                let next = self.dims[dim].at(left_recursion_result).next;
                let new_refinement_node = NCDimNode {
                    left:  if !where_to_insert { recursion_result.0 } else { None },
                    right: if  where_to_insert { recursion_result.0 } else { None },
                    next:  next
                };
                self.make_node_ref(next, dim+1);
                self.make_node_ref(recursion_result.0, dim);
                let new_index = self.dims[dim].nodes.insert(new_refinement_node);
                return (Some(new_index), recursion_result.1);
            }
        }
    }
}

pub fn smoke_test()
{
    let nc = Nanocube::<usize>::new(vec![2]);
    println!("{:?}", nc.base_root);
}
