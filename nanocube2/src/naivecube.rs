use cube::Monoid;
use cube::Cube;

pub struct Naivecube<Summary> {
    widths: Vec<usize>,
    values: Vec<(Vec<usize>, Summary)>
}

impl <Summary: Monoid + PartialOrd> Naivecube<Summary> {
    pub fn add(&mut self,
               summary: Summary,
               addresses: &Vec<usize>)
    {
        assert!(addresses.len() == self.widths.len());
        self.values.push((addresses.clone(), summary))
    }

    pub fn new(widths: Vec<usize>) -> Naivecube<Summary> {
        assert!(widths.len() > 0);
        Naivecube {
            widths: widths,
            values: Vec::new()
        }
    }
}

impl <Summary: Monoid + PartialOrd> Cube<Summary> for Naivecube<Summary> {
    fn range_query(&self, bounds: &Vec<(usize, usize)>) -> Summary {
        let mut result = Summary::mempty();
        assert!(bounds.len() == self.widths.len());
        for &(ref position, ref summary) in &self.values {
            // let position = &t.0;
            // let summary = &t.1;
            let mut inside = true;
            for (coord, qbounds) in position.iter().zip(bounds.iter()) {
                let qlo = &qbounds.0;
                let qhi = &qbounds.1;
                if coord < qlo || qhi <= coord {
                    inside = false;
                    break;
                }
            }
            if inside {
                result = result.mapply(&summary);
            }
        }
        result
    }
}

pub fn smoke_test()
{
    let mut nc1 = Naivecube::new(vec![2, 2]);
    nc1.add(1, &vec![0, 0]);
    nc1.add(1, &vec![3, 3]);
}
