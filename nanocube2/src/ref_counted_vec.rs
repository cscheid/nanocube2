// vector of reference-counted values. We do all reference counting manually
// for now.

use std::collections::HashMap;

#[derive(Debug)]
pub struct RefCountedVec<T> {
    values: Vec<T>,
    ref_counts: Vec<usize>,
    free_list: Vec<usize>
}

fn sorted_array_has_no_duplicates(v: &Vec<usize>) -> bool {
    if v.len() <= 1 {
        return true;
    }
    for i in 0..v.len()-1 {
        if v[i] == v[i+1] {
            return false;
        }
    }
    true
}

impl <T> RefCountedVec<T> {
    pub fn new() -> RefCountedVec<T> {
        RefCountedVec {
            values: Vec::new(),
            ref_counts: Vec::new(),
            free_list: Vec::new()
        }
    }


    pub fn at(&self, index: usize) -> &T {
        &self.values[index]
    }

    pub fn at_mut(&mut self, index: usize) -> &mut T {
        &mut self.values[index]
    }
    
    pub fn insert(&mut self, value: T) -> usize {
        if self.free_list.len() > 0 {
            let free_index = self.free_list.pop().expect("internal error");
            self.values[free_index] = value;
            free_index
        } else {
            self.values.push(value);
            self.ref_counts.push(0);
            self.values.len() - 1
        }
    }

    pub fn make_ref(&mut self, index: usize) -> usize {
        assert!(index < self.values.len());
        assert!(self.ref_counts.len() == self.values.len());
        self.ref_counts[index] += 1;
        self.ref_counts[index]
    }

    pub fn release_ref(&mut self, index: usize) -> usize {
        assert!(index < self.values.len());
        assert!(self.ref_counts.len() == self.values.len());
        assert!(self.ref_counts[index] > 0);
        self.ref_counts[index] -= 1;
        if self.ref_counts[index] == 0 {
            self.free_list.push(index);
        }
        self.ref_counts[index]
    }

    pub fn compact(&mut self) -> HashMap<usize, usize> {
        let mut result = HashMap::new();
        if self.free_list.len() == 0 {
            return result;
        }
        self.free_list.sort();
        assert!(sorted_array_has_no_duplicates(&self.free_list));
        let mut values_i = self.values.len() - 1;
        let mut holes_b = 0;
        let mut holes_e = self.free_list.len();
        let mut holes_l = holes_e - 1;

        // while we still have unpatched holes and we haven't yet
        // hit the beginning of the holes array:
        while holes_b != holes_e {
            if self.ref_counts[values_i] == 0 {
                assert!(values_i == self.free_list[holes_l]);
                holes_l -= 1;
                holes_e -= 1;
                self.values.pop();
                self.ref_counts.pop();
                continue;
            }
            assert!(self.ref_counts[self.free_list[holes_b]] == 0);
            assert!(self.ref_counts[values_i] != 0);
            assert!(self.free_list[holes_b] < values_i);

            // patch furthest unpatched hole with back of values array
            self.values.swap    (self.free_list[holes_b], values_i);
            self.ref_counts.swap(self.free_list[holes_b], values_i);

            // update transposition map of compaction
            result.insert(values_i, self.free_list[holes_b]);
            self.values.pop();
            self.ref_counts.pop();

            holes_b += 1;
            if values_i == 0 {
                break;
            } else {
                values_i -= 1;
            }
        }

        self.free_list.clear();
        assert!(self.free_list.len() == 0);
        result
    }
}
