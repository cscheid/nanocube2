// we're not using add here because I can't make the trait implementations
// and declarations work like I want them to.
pub trait Monoid<RHS = Self> {
    fn mapply(&self, rhs: &RHS) -> Self;
    fn mempty() -> Self;
}

pub trait Cube<Summary: Monoid + PartialOrd + Copy> {
    fn range_query(&self, bounds: &Vec<(usize, usize)>) -> Summary;
}

impl Monoid for usize {
    fn mapply(&self, rhs: &usize) -> usize {
        self + rhs
    }
    fn mempty() -> usize {
        0
    }
}
