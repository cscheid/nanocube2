use naivecube::Naivecube;
use nanocube::Nanocube;
use nanocube;
use std::cmp;
use rand;
use rand::distributions::{IndependentSample, Range};
use cube::Cube;
use timeit;

fn generate_random_dataset(widths: &Vec<usize>, n: usize) -> Vec<Vec<usize>>
{
    let mut rng = rand::thread_rng();
    let mut result = Vec::new();
    let dims: Vec<Range<usize>> = widths
        .iter()
        .map(|w| Range::new(0, (1 as usize) << w))
        .collect();
    for _ in 0..n {
        result.push(dims.iter().map(|r| r.ind_sample(&mut rng)).collect());
    }
    result
}

fn generate_random_ranges(widths: &Vec<usize>, n: usize) -> Vec<Vec<(usize, usize)>>
{
    let mut rng = rand::thread_rng();
    let mut result = Vec::new();
    let dims: Vec<Range<usize>> = widths
        .iter()
        .map(|w| Range::new(0, (1 as usize) << w))
        .collect();
    for _ in 0..n {
        result.push(dims.iter().map(|r| {
            let v1 = r.ind_sample(&mut rng);
            let v2 = r.ind_sample(&mut rng);
            (cmp::min(v1, v2),
             cmp::max(v1, v2) + 1)
        }).collect());
    }
    result
}

pub fn check_nanocube_and_naivecube_equivalence()
{
    let nruns = 100;
    let width = vec![10,10];
    let npoints = 100;
    let nranges = 5;
    let mut failed = false;

    let nloops = 1000;
    let sec = timeit_loops!(nloops, {
        for _ in 0..nruns {
            let data = generate_random_dataset(&width, npoints);
            let mut naivecube = Naivecube::<usize>::new(width.clone());
            let mut nanocube = Nanocube::<usize>::new(width.clone());

            for point in &data {
                naivecube.add(1, point);
                nanocube.add(1, point);
            }

            let ranges = generate_random_ranges(&width, nranges);

            for range in ranges {
                let naive_result = naivecube.range_query(&range);
                let nc_result = nanocube.range_query(&range);
                if naive_result != nc_result {
                    println!("mismatch!");
                    println!("data: {:?}", &data);
                    println!("query region: {:?}", &range);
                    println!("naive result: {:?}", &naive_result);
                    println!("nanocube res: {:?}", &nc_result);
                    nanocube::write_dot_to_disk("out/bad_nc.dot", &nanocube)
                        .expect("internal error");
                    println!("{:?}", nanocube.summaries.values);
                    failed = true;
                }
            }
        }
        // if !failed {
        //     println!("Passed {} tests.", nruns);
        // }
    });

    println!("{} insertions in {} secs (rate: {}).\n",
             nloops * nruns * npoints,
             (nloops as f64) * sec,
             ((nruns * npoints) as f64) / sec);
}

pub fn run()
{
    check_nanocube_and_naivecube_equivalence();
}
